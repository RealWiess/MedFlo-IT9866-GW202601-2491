#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "ite/ite_usbex.h"
#include "bootloader.h"
#include "config.h"
#include "ite/itp.h"
#if defined(CFG_ENABLE_UART_CLI) || defined(CFG_OPENRTOS_CLI)
#include "ite/cli.h"
#endif

static pthread_t loadImageTask;
static bool upgradeCheckFail = false;
static bool blLcdOn = false;
static bool blLcdConsoleOn = false;
static bool stop_led = false;

#if defined(CFG_ENABLE_UART_CLI)
extern bool cliQuit;
//char tftppara[128] = "tftp://192.168.191.102/ITEPKG03.PKG";
#endif //end of #if defined (CFG_ENABLE_UART_CLI)

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
extern bool fatInited;
extern bool driveInited;
#endif

#if defined(CFG_UPGRADE_FROM_UART_BOOT_TIME)
#if defined(CFG_UPGRADE_UART0)
#define UPGRADE_UART_PORT       ITP_DEVICE_UART0
#define UPGRADE_UART_BAUDRATE   CFG_UART0_BAUDRATE
#define UPGRADE_UART_DEVICE     itpDeviceUart0
#elif defined(CFG_UPGRADE_UART1)
#define UPGRADE_UART_PORT       ITP_DEVICE_UART1
#define UPGRADE_UART_BAUDRATE   CFG_UART1_BAUDRATE
#define UPGRADE_UART_DEVICE     itpDeviceUart1
#else
#define UPGRADE_UART_PORT       ITP_DEVICE_UART0
#define UPGRADE_UART_BAUDRATE   CFG_UART0_BAUDRATE
#define UPGRADE_UART_DEVICE     itpDeviceUart0
#endif

#define UPGRADE_PATTERN		0x1A
#define ACK20				0x14
#define ACK50				0x32
#define ACK100				0x64
#define ACK150				0x96
#define ACK200				0xC8
#define ACK210				0xD2
#define ACK211				0xD3
#define ACK220				0xDC
#define ACK221				0xDD

//the total check times is CHECK_NUM or CHECK_NUM+1
#define CHECK_NUM			4
#define RETRY_SIZE			5
#define RETRY_CHECKSUM		1
#define RETRY_DATA			1

#define CHECK_TIME			10

typedef enum UPGRADE_UART_STATE_TAG
{
    WAIT_SIZE,
	WAIT_SIZE_ACK,
	READY,
	WAIT_CHECKSUM,
	WAIT_CHECK_ACK,
	FINISH,
	FAIL,
} UPGRADE_UART_STATE;

static ITCArrayStream UartStream;
static UPGRADE_UART_STATE gState = WAIT_SIZE;
static UPGRADE_UART_STATE gTimeState[CHECK_TIME];
static int gSec = 0;
static unsigned int gTotalSize = 0;
static unsigned int gSecSize[CHECK_TIME];
#endif

#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
static bool usbInited = false;
#endif

#ifdef CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
extern unsigned char gDoReMapFlow;
#endif

static void BeginLoadImage(void)
{
    if (!upgradeCheckFail)
    {
        pthread_create(&loadImageTask, NULL, LoadImage, NULL);
    }
}

static void EndLoadImage(bool doBoot)
{
    if (!upgradeCheckFail)
    {
        pthread_join(loadImageTask, NULL);
        if (!doBoot)
            ReleaseImage();
    }
}

#ifdef CFG_UPGRADE_USB_DEVICE

static bool DetectUsbDeviceMode(void)
{
    bool ret = false;
    LOG_INFO "Detect USB device mode...\r\n" LOG_END

    // init card device
#if  !defined(_WIN32) && (defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_USBH_CD_MST) || defined(CFG_MSC_ENABLE) || defined(CFG_RAMDISK_ENABLE))
    itpRegisterDevice(ITP_DEVICE_CARD, &itpDeviceCard);
    ioctl(ITP_DEVICE_CARD, ITP_IOCTL_INIT, NULL);
#endif

    // init usb
#if defined(CFG_USB0_ENABLE) || defined(CFG_USB1_ENABLE) || defined(CFG_USBHCC)
    itpRegisterDevice(ITP_DEVICE_USB, &itpDeviceUsb);
    if (ioctl(ITP_DEVICE_USB, ITP_IOCTL_INIT, NULL) != -1)
        usbInited = true;
#endif

#if defined(CFG_USBD_ACM) || defined(CFG_USBD_CD_CDCACM)
    if (usbInited)
    {
        int timeout = CFG_UPGRADE_USB_DETECT_TIMEOUT;
        ITPUsbInfo usbInfo = {0};
        usbInfo.host = false;

        itpRegisterDevice(ITP_DEVICE_USBDACM, &itpDeviceUsbdAcm);
        ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_INIT, NULL);

        itpRegisterDevice(ITP_DEVICE_USBD, &itpDeviceUsbd);
        ioctl(ITP_DEVICE_USBD, ITP_IOCTL_INIT, NULL);

        while (!ret)
        {
            ioctl(ITP_DEVICE_USB, ITP_IOCTL_GET_INFO, (void*)&usbInfo);
            if (usbInfo.b_device)
            {
                ret = true;
                break;
            }

            timeout -= 10;
            if (timeout <= 0)
            {
                LOG_INFO "USB ACM device not connected.\n" LOG_END
                break;
            }
            usleep(10000);
        }
    }
#endif // defined(CFG_USBD_ACM) || defined(CFG_USBD_CD_CDCACM)
    return ret;
}
#endif // CFG_UPGRADE_USB_DEVICE

static void InitFileSystem(void)
{
    // init card device
#if  !defined(CFG_UPGRADE_USB_DEVICE) && !defined(_WIN32) && (defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST) || defined(CFG_RAMDISK_ENABLE))
    itpRegisterDevice(ITP_DEVICE_CARD, &itpDeviceCard);
    ioctl(ITP_DEVICE_CARD, ITP_IOCTL_INIT, NULL);
#endif

    // init usb
#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    if (!usbInited)
    {
        itpRegisterDevice(ITP_DEVICE_USB, &itpDeviceUsb);
        if (ioctl(ITP_DEVICE_USB, ITP_IOCTL_INIT, NULL) != -1)
            usbInited = true;
    }
#endif

    // init fat
#ifdef CFG_FS_FAT
#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
    itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
#else
    if (!fatInited)
    {
        itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
        ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
        ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
    }
#endif
#endif
#ifdef CFG_FS_LFS
    itpRegisterDevice(ITP_DEVICE_LFS, &itpLittlefsDevice.dev);
    ioctl(ITP_DEVICE_LFS, ITP_IOCTL_INIT, NULL);
    ioctl(ITP_DEVICE_LFS, ITP_IOCTL_ENABLE, NULL);
#endif // CFG_FS_LFS

    // init drive table
#if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS)
#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
    itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
#else
    if (!driveInited)
    {
        itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
        ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
    }
#endif

#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_ENABLE, NULL);
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT_TASK, NULL);
#endif // defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
#endif // defined(CFG_FS_FAT)

    // mount disks on booting
#ifdef CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
    if(!ioctl(ITP_DEVICE_NAND, ITP_IOCTL_NAND_CHECK_REMAP, NULL))
    {
        ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, NULL);
    }
#endif

#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    // wait msc is inserted
    if (usbInited)
    {
        ITPDriveStatus* driveStatusTable;
        ITPDriveStatus* driveStatus = NULL;
        int i, timeout = CFG_UPGRADE_USB_DETECT_TIMEOUT;
        bool found = false;
    #if (defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC)) || (defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC))
        static const int diskTable[] = { ITP_DISK_SD0, ITP_DISK_SD1, -1 };
    #else
        static const int diskTable[] = { -1 };
    #endif // (defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC)) || (defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC))

    #ifdef CFG_NET_WIFI
        ITPUsbInfo usbInfo;
        usbInfo.host = true;
    #endif // CFG_NET_WIFI

        // mount usb disks on booting
        ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, (void*)diskTable);

        while (!found)
        {
            for (i = 0; i < 2; i++)
            {
                if (ioctl(ITP_DEVICE_USB, ITP_IOCTL_IS_CONNECTED, (void*)(USB0 + i)))
                {
                #ifdef CFG_NET_WIFI
                    usbInfo.usbIndex = i;
                    ioctl(ITP_DEVICE_USB, ITP_IOCTL_GET_INFO, (void*)&usbInfo);
                    if (usbInfo.ctxt && usbInfo.type == USB_DEVICE_TYPE_MSC)
                    {
                #endif // CFG_NET_WIFI
                        found = true;
                        break;
                #ifdef CFG_NET_WIFI
                    }
                #endif
                }
            }

            if (found)
            {
                break;
            }
            else
            {
                timeout -= 1;
                if (timeout <= 0)
                {
                    LOG_INFO "USB device not found.\n" LOG_END
#ifdef CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
                    gDoReMapFlow = 0;
#endif
                    return;
                }
                usleep(1000);
            }
        }

        found = false;
        timeout = CFG_UPGRADE_USB_TIMEOUT;

        ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

        while (!found)
        {
            for (i = 0; i < ITP_MAX_DRIVE; i++)
            {
                driveStatus = &driveStatusTable[i];
                if (driveStatus->disk >= ITP_DISK_MSC00 && driveStatus->disk <= ITP_DISK_MSC17 && driveStatus->avail)
                {
                    LOG_DBG "drive[%d]:usb disk=%d\n", i, driveStatus->disk LOG_END
                    found = true;
                    usleep(100*1000);
                }
            }
            if (!found)
            {
                timeout -= 100;
                if (timeout <= 0)
                {
                    LOG_INFO "USB disk not found.\n" LOG_END
                    break;
                }
                usleep(100000);
            }
        }
    }
#if (defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC)) || (defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC))
    else
    {
        static const int diskTable[] = { ITP_DISK_SD0, ITP_DISK_SD1, -1 };
        // mount sd disks on booting
        ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, (void*)diskTable);
    }
#endif // (defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC)) || (defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC))
#else
    // mount disks on booting
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, NULL);

#endif //  defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
}

#if !defined(CFG_LCD_ENABLE) && (CFG_CHIP_FAMILY == 970)
static void* UgLedTask(void* arg)
{
    int gpio_pin = 15;
    ithGpioSetOut(16);
    ithGpioSetMode(16,ITH_GPIO_MODE0);
    ithGpioSetOut(15);
    ithGpioSetMode(15,ITH_GPIO_MODE0);
    stop_led = false;

    for(;;)
    {
        if(stop_led == true)
        {
            ithGpioSet(15);
            ithGpioSet(16);
            while(1)
                usleep(500000);
        }
        ithGpioClear(gpio_pin);
        if(gpio_pin==16)
            gpio_pin = 15;
        else
            gpio_pin = 16;
        ithGpioSet(gpio_pin);
        usleep(500000);
    }
}
#endif

#if (CFG_CHIP_FAMILY == 970)
static void DetectKey(void)
{
    int ret;
    int phase = 0;
    int time_counter = 0;
    int key_counter = 0;
    bool key_pressed;
    bool key_released;
    ITPKeypadEvent ev;

    while (1)
    {
        key_pressed = key_released = false;
        ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
        if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == sizeof (ITPKeypadEvent))
        {
            if (ev.code == 0)
            {
                if (ev.flags & ITP_KEYPAD_DOWN)
                    key_pressed = true;
                else if (ev.flags & ITP_KEYPAD_UP)
                    key_released = true;
            }
        }

        if (phase == 0)
        {
            if (key_pressed)
            {
                printf("key detected\n");
                phase = 1;
            }
            else
                break;
        }
        else if (phase == 1)
        {
            if (key_released)
                break;
            if (time_counter > 100)
            {
                phase = 2;
                ithGpioSetOut(16);
                ithGpioSetMode(16, ITH_GPIO_MODE0);
                ithGpioSetOut(15);
                ithGpioSetMode(15, ITH_GPIO_MODE0);
            }
        }
        else if (phase == 2)
        {
            if (key_pressed)
            {
                ithGpioSet(15);
                key_counter++;
            }
            if (key_released)
                ithGpioClear(15);

            if (time_counter > 200)
            {
                ithGpioSet(16);
                ithGpioClear(15);
                phase = 3;
            }

            // blink per 6*50000 us
            if ((time_counter/6)%2)
                ithGpioSet(16);
            else
                ithGpioClear(16);
        }
        else if (phase == 3)
        {
            printf("key_counter: %d\n", key_counter);
            if (key_counter == 1)
            {
                // do reset
                InitFileSystem();
                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, NULL);
                ret = ugResetFactory();
                #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
                ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
                #endif

                exit(ret);
            }
            if (key_counter == 2)
            {
                // dump addressbook.xml
                InitFileSystem();
                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, NULL);
                CopyUclFile();
            }
            ithGpioClear(16);
            break;
        }

        usleep(50000);
        time_counter++;
    }
}
#endif

static void InitLcdConsole(void)
{
    // output messages to LCD console
#if defined(CFG_LCD_ENABLE) && defined(CFG_BL_LCD_CONSOLE)
    if (!blLcdOn)
    {
    #if !defined(CFG_BL_SHOW_LOGO)
        extern uint32_t __lcd_base_a;
        extern uint32_t __lcd_base_b;
        extern uint32_t __lcd_base_c;

        itpRegisterDevice(ITP_DEVICE_SCREEN, &itpDeviceScreen);
        ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_RESET, NULL);
        ithLcdSetBaseAddrA((uint32_t) &__lcd_base_a);
        ithLcdSetBaseAddrB((uint32_t) &__lcd_base_b);

    #ifdef CFG_BACKLIGHT_ENABLE
        itpRegisterDevice(ITP_DEVICE_BACKLIGHT, &itpDeviceBacklight);
        ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_INIT, NULL);
    #endif // CFG_BACKLIGHT_ENABLE

    #endif // !defined(CFG_BL_SHOW_LOGO)

        ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);
        ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_RESET, NULL);
        blLcdOn = true;
    }
    if (!blLcdConsoleOn)
    {
        itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceLcdConsole);
        itpRegisterDevice(ITP_DEVICE_LCDCONSOLE, &itpDeviceLcdConsole);
        ioctl(ITP_DEVICE_LCDCONSOLE, ITP_IOCTL_INIT, NULL);
        ioctl(ITP_DEVICE_LCDCONSOLE, ITP_IOCTL_CLEAR, NULL);
        blLcdConsoleOn = true;
    }
#endif // defined(CFG_LCD_ENABLE) && defined(BL_LCD_CONSOLE)
}

static void DoUpgrade(void)
{
    ITCStream* upgradeFile;

    LOG_INFO "Do Upgrade...\r\n" LOG_END

    upgradeFile = OpenUpgradePackage();
    if (upgradeFile)
    {
        int ret = 0;

    #if !defined(CFG_LCD_ENABLE) && (CFG_CHIP_FAMILY == 970)
        //---light on red/green led task
        pthread_t task;
        pthread_create(&task, NULL, UgLedTask, NULL);
        //------
    #endif
        InitLcdConsole();
        EndLoadImage(false);

        if (ugCheckCrc(upgradeFile, NULL))
        {
            LOG_ERR "Upgrade failed.\n" LOG_END
            ShowUpgradeFail();
            while (1)
                sleep(10);
        }
        else
            ret = ugUpgradePackage(upgradeFile);

    #ifdef CFG_UPGRADE_DELETE_PKGFILE_AFTER_FINISH
        DeleteUpgradePackage();
    #endif

    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        LOG_INFO "Flushing NOR cache...\n" LOG_END
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    #endif

        if (ret)
        {
            LOG_INFO "Upgrade failed.\n" LOG_END
            ShowUpgradeFail();
            while (1)
                sleep(10);
        }
        else
        {
            stop_led = true;
            LOG_INFO "Upgrade finished.\n" LOG_END
        }
    #if defined(CFG_UPGRADE_DELAY_AFTER_FINISH) && CFG_UPGRADE_DELAY_AFTER_FINISH > 0
        sleep(CFG_UPGRADE_DELAY_AFTER_FINISH);
    #endif

        exit(ret);
        while (1);
    }
}

#ifdef CFG_BOOT_TESTBIN_ENABLE
static void DoBootTestBin(void)
{
    ITCStream* testBinFile;

    LOG_INFO "Do Test Bin Booting...\r\n" LOG_END

    testBinFile = OpenTestBin();
    if (testBinFile)
    {
        EndLoadImage(false);

        BootTestBin(testBinFile);
    }
}
#endif

void* BootloaderMain(void* arg)
{
    int ret = 0;

#if defined(CFG_UPGRADE_PRESSKEY) || defined(CFG_UPGRADE_RESET_FACTORY) || defined(CFG_UPGRADE_RECOVERY)
    ITPKeypadEvent ev;
#endif

#ifdef CFG_WATCHDOG_ENABLE
    ioctl(ITP_DEVICE_WATCHDOG, ITP_IOCTL_DISABLE, NULL);
#endif

    ithMemDbgDisable(ITH_MEMDBG0);
    ithMemDbgDisable(ITH_MEMDBG1);

    if (ugUpgradeCheck())
        upgradeCheckFail = true;

    BeginLoadImage();

#ifdef CFG_BL_SHOW_LOGO
    ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);
    ShowLogo();
    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_RESET, NULL);
    blLcdOn = true;
#endif // CFG_BL_SHOW_LOGO

#ifdef CFG_UPGRADE_USB_DEVICE
    if (DetectUsbDeviceMode())
    {
        LOG_INFO "Do USB Device Commands\n" LOG_END
        EndLoadImage(false);
        InitFileSystem();
        DoUsbDeviceCommands();
    }
#endif // CFG_UPGRADE_USB_DEVICE

#if defined(CFG_UPGRADE_PRESSKEY) || defined(CFG_UPGRADE_RESET_FACTORY) || defined(CFG_UPGRADE_RECOVERY)
    ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
    if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == sizeof (ITPKeypadEvent))
    {
        int key = ev.code, delay = 0;

#endif // defined(CFG_UPGRADE_PRESSKEY) || defined(CFG_UPGRADE_RESET_FACTORY) || defined(CFG_UPGRADE_RECOVERY)

    #ifdef CFG_UPGRADE_RECOVERY
        if (key == CFG_UPGRADE_RECOVERY_PRESSKEY_NUM)
        {
            struct timeval time = ev.time;

            // detect key pressed time
            for (;;)
            {
                if (ev.flags & ITP_KEYPAD_UP)
                    break;

                ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
                if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == 0)
                    continue;

                delay += itpTimevalDiff(&time, &ev.time);
                time = ev.time;

                LOG_DBG "recovery key: time=%ld.%ld,code=%d,down=%d,up=%d,repeat=%d,delay=%d\r\n",
                    ev.time.tv_sec,
                    ev.time.tv_usec / 1000,
                    ev.code,
                    (ev.flags & ITP_KEYPAD_DOWN) ? 1 : 0,
                    (ev.flags & ITP_KEYPAD_UP) ? 1 : 0,
                    (ev.flags & ITP_KEYPAD_REPEAT) ? 1 : 0,
                    delay
                LOG_END

                if (delay >= CFG_UPGRADE_RECOVERY_PRESSKEY_DELAY)
                    break;
            };

            if (delay >= CFG_UPGRADE_RECOVERY_PRESSKEY_DELAY)
            {
                ITCStream* upgradeFile;
            #ifdef CFG_UPGRADE_RECOVERY_LED
                int fd = open(":led:" CFG_UPGRADE_RECOVERY_LED_NUM, O_RDONLY);
                ioctl(fd, ITP_IOCTL_FLICKER, (void*)500);
            #endif

                InitLcdConsole();

                LOG_INFO "Do Recovery...\r\n" LOG_END

                InitFileSystem();

                upgradeFile = OpenRecoveryPackage();
                if (upgradeFile)
                {
                    EndLoadImage(false);

                    if (ugCheckCrc(upgradeFile, NULL))
                        LOG_ERR "Recovery failed.\n" LOG_END
                    else
                        ret = ugUpgradePackage(upgradeFile);

                    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
                        LOG_INFO "Flushing NOR cache...\n" LOG_END
                        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
                    #endif

                    if (ret)
                        LOG_INFO "Recovery failed.\n" LOG_END
                    else
                        LOG_INFO "Recovery finished.\n" LOG_END

                    exit(ret);
                }
                else
                {
                #ifdef CFG_UPGRADE_RECOVERY_LED
                    ioctl(fd, ITP_IOCTL_OFF, NULL);
                #endif
                }
                while (1);
            }
        }
    #endif // CFG_UPGRADE_RECOVERY

    #ifdef CFG_UPGRADE_RESET_FACTORY
        if (key == CFG_UPGRADE_RESET_FACTORY_PRESSKEY_NUM)
        {
            struct timeval time = ev.time;

            // detect key pressed time
            for (;;)
            {
                if (ev.flags & ITP_KEYPAD_UP)
                    break;

                ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
                if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == 0)
                    continue;

                delay += itpTimevalDiff(&time, &ev.time);
                time = ev.time;

                LOG_DBG "reset key: time=%ld.%ld,code=%d,down=%d,up=%d,repeat=%d,delay=%d\r\n",
                    ev.time.tv_sec,
                    ev.time.tv_usec / 1000,
                    ev.code,
                    (ev.flags & ITP_KEYPAD_DOWN) ? 1 : 0,
                    (ev.flags & ITP_KEYPAD_UP) ? 1 : 0,
                    (ev.flags & ITP_KEYPAD_REPEAT) ? 1 : 0,
                    delay
                LOG_END

                if (delay >= CFG_UPGRADE_RESET_FACTORY_PRESSKEY_DELAY)
                    break;
            };

            if (delay >= CFG_UPGRADE_RESET_FACTORY_PRESSKEY_DELAY)
            {
                InitLcdConsole();

                LOG_INFO "Do Reset...\r\n" LOG_END

                EndLoadImage(false);

                InitFileSystem();
                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, NULL);

                ret = ugResetFactory();

            #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
                LOG_INFO "Flushing NOR cache...\n" LOG_END
                ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
            #endif

                if (ret)
                    LOG_INFO "Reset failed.\n" LOG_END
                else
                    LOG_INFO "Reset finished.\n" LOG_END

                exit(ret);
                while (1);
            }
        }
    #endif // CFG_UPGRADE_RESET_FACTORY

    #ifdef CFG_UPGRADE_PRESSKEY
        if (key == CFG_UPGRADE_PRESSKEY_NUM)
        {
            struct timeval time = ev.time;

            // detect key pressed time
            for (;;)
            {
                if (ev.flags & ITP_KEYPAD_UP)
                    break;

                ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
                if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == 0)
                    continue;

                delay += itpTimevalDiff(&time, &ev.time);
                time = ev.time;

                LOG_DBG "upgrade key: time=%ld.%ld,code=%d,down=%d,up=%d,repeat=%d,delay=%d\r\n",
                    ev.time.tv_sec,
                    ev.time.tv_usec / 1000,
                    ev.code,
                    (ev.flags & ITP_KEYPAD_DOWN) ? 1 : 0,
                    (ev.flags & ITP_KEYPAD_UP) ? 1 : 0,
                    (ev.flags & ITP_KEYPAD_REPEAT) ? 1 : 0,
                    delay
                LOG_END

                if (delay >= CFG_UPGRADE_PRESSKEY_DELAY)
                    break;
            };

            if (delay >= CFG_UPGRADE_PRESSKEY_DELAY)
            {
                InitFileSystem();
                DoUpgrade();
            }
        }
    #endif // CFG_UPGRADE_PRESSKEY
#if defined(CFG_UPGRADE_PRESSKEY) || defined(CFG_UPGRADE_RESET_FACTORY) || defined(CFG_UPGRADE_RECOVERY)
    }
#endif

#if (CFG_CHIP_FAMILY == 970)
    DetectKey();
#endif

#if !defined(CFG_UPGRADE_PRESSKEY) && defined(CFG_UPGRADE_OPEN_FILE)
    InitFileSystem();
    DoUpgrade();
#endif

#ifdef CFG_ENABLE_UART_CLI
	cliInit();
	while (!cliQuit)
		sleep(1);
#endif

#if defined(CFG_UPGRADE_FROM_UART_BOOT_TIME)
	//Upgrade FW by Uart
	DetectUartPattern();
#endif

    if (upgradeCheckFail)
    {
        InitLcdConsole();

#ifdef CFG_UPGRADE_BACKUP_PACKAGE

        LOG_INFO "Last upgrade failed, try to restore from internal package...\r\n" LOG_END

    #ifndef CFG_UPGRADE_USB_DEVICE

        // init fat
    #ifdef CFG_FS_FAT
    #ifndef CFG_UPGRADE_BACKUP_RAW_DATA
        itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
        ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
        ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
    #else
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
    #endif
    #endif // CFG_FS_FAT
    #ifdef CFG_FS_LFS
        itpRegisterDevice(ITP_DEVICE_LFS, &itpLittlefsDevice.dev);
        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_INIT, NULL);
        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_ENABLE, NULL);
    #endif // CFG_FS_LFS

        // mount disks on booting
        ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, NULL);

    #endif // !CFG_UPGRADE_USB_DEVICE

        RestorePackage();

#else
    ShowLastUpgradeFail();
    sleep(10);

#endif // CFG_UPGRADE_BACKUP_PACKAGE
    }

#ifdef CFG_BOOT_TESTBIN_ENABLE
    #if !defined(CFG_UPGRADE_OPEN_FILE)
    InitFileSystem();
    #endif
    DoBootTestBin();
#endif
    LOG_INFO "Do Booting...\r\n" LOG_END
    EndLoadImage(true);
    BootImage();

    return NULL;
}
/*
#if (CFG_CHIP_PKG_IT9079)
void
ithCodecPrintfWrite(
    char* string,
    int length)
{
    // this is a dummy function for linking with library itp_boot. (itp_swuart_codec.c)
}

#endif
*/
