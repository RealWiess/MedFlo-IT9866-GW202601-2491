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

static bool blLcdOn = false;
static bool blLcdConsoleOn = false;
static pthread_t loadImageTask;
static bool usbInited = false;
#ifndef CFG_UPGRADE_USB_DETECT_TIMEOUT
#define CFG_UPGRADE_USB_DETECT_TIMEOUT 500
#endif

bool upgradeCheckFail = false;

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
extern bool fatInited;
extern bool driveInited;
#endif

#ifdef CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
extern unsigned char gDoReMapFlow;
#endif

#ifdef CFG_BL_SHOW_LOGO
void BootloaderShowLogo()
{
	ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);
    ShowLogo();
    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_RESET, NULL);
    blLcdOn = true;
}
#endif

void BeginLoadImage(void)
{
    if (!upgradeCheckFail)
    {
        pthread_create(&loadImageTask, NULL, LoadImage, NULL);
    }
}

void EndLoadImage(bool doBoot)
{
    if (!upgradeCheckFail)
    {
        pthread_join(loadImageTask, NULL);
        if (!doBoot)
            ReleaseImage();
    }
}

void InitFileSystem(void)
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

void InitLcdConsole(void)
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

void DoUpgrade(void)
{
    ITCStream* upgradeFile;

    LOG_INFO "Do Upgrade...\r\n" LOG_END

    upgradeFile = OpenUpgradePackage();
    if (upgradeFile)
    {
        int ret = 0;

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
            LOG_INFO "Upgrade finished.\n" LOG_END
        }
    #if defined(CFG_UPGRADE_DELAY_AFTER_FINISH) && CFG_UPGRADE_DELAY_AFTER_FINISH > 0
        sleep(CFG_UPGRADE_DELAY_AFTER_FINISH);
    #endif

        exit(ret);
        while (1);
    }
}

#ifdef CFG_UPGRADE_USB_DEVICE
bool DetectUsbDeviceMode(void)
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

#ifdef CFG_BOOT_TESTBIN_ENABLE
void DoBootTestBin(void)
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

