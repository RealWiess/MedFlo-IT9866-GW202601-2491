#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "ite/ite_usbex.h"
#include "bootloader.h"
#include "main_upgrade_api.h"
#include "config.h"
#include "ite/itp.h"
#if defined(CFG_ENABLE_UART_CLI) || defined(CFG_OPENRTOS_CLI)
#include "ite/cli.h"
#endif

#define STORAGE_UPGRADE_SUPPORT 		(1 << 0)
#define CLI_UPGRADE_SUPPORT     		(1 << 1)
#define UART_UPGRADE_SUPPORT    		(1 << 2)
#define USB_DEVICE_UPGRADE_SUPPORT  	(1 << 3)
#define PRESSKEY_UPGRADE_SUPPORT		(1 << 4)
#define RESET_FACTORY_SUPPORT			(1 << 5)
#define RECOVERY_UPGRADE_SUPPORT		(1 << 6)
#define TESTBIN_UPGRADE_SUPPORT			(1 << 7)

extern bool upgradeCheckFail;
#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
extern bool fatInited;
extern bool driveInited;
#endif

#if defined(CFG_ENABLE_UART_CLI)
extern bool cliQuit;
//char tftppara[128] = "tftp://192.168.191.102/ITEPKG03.PKG";
#endif //end of #if defined (CFG_ENABLE_UART_CLI)

static void ProcessStorageUpgrade()
{
	InitFileSystem();
    DoUpgrade();
}

static void ProcessCliUpgrade()
{
#ifdef CFG_ENABLE_UART_CLI
	cliInit();
	while (!cliQuit)
		sleep(1);
#endif
}

static void ProcessUartUpgrade()
{	
#ifdef CFG_UPGRADE_FROM_UART_BOOT_TIME
	//Upgrade FW by Uart
	DetectUartPattern();
#endif
}

static void ProcessUsbDeviceUpgrade()
{
#ifdef CFG_UPGRADE_USB_DEVICE
		if (DetectUsbDeviceMode())
		{
			LOG_INFO "Do USB Device Commands\n" LOG_END
			EndLoadImage(false);
			InitFileSystem();
			DoUsbDeviceCommands();
		}
#endif // CFG_UPGRADE_USB_DEVICE
}

static void ProcessPressKeyUpgrade()
{
#ifdef CFG_UPGRADE_PRESSKEY
	ITPKeypadEvent ev;
	ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
    if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == sizeof (ITPKeypadEvent))
    {
        int key = ev.code, delay = 0;
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
    }	
#endif
}

static void ProcessResetFactory()
{
#ifdef CFG_UPGRADE_RESET_FACTORY
	int ret = 0;
	ITPKeypadEvent ev;
	ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
	if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == sizeof (ITPKeypadEvent))
	{
		int key = ev.code, delay = 0;
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
	}
#endif
}

static void ProcessRecoveryUpdate()
{
#ifdef CFG_UPGRADE_RECOVERY
	int ret = 0;
	ITPKeypadEvent ev;
	ioctl(ITP_DEVICE_KEYPAD, ITP_IOCTL_PROBE, NULL);
	if (read(ITP_DEVICE_KEYPAD, &ev, sizeof (ITPKeypadEvent)) == sizeof (ITPKeypadEvent))
	{
		int key = ev.code, delay = 0;
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
	}
#endif
}

static void ProcessBackupPackageUpdate()
{
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
#endif
}

static void ProcessTestBinUpdate()
{
	InitFileSystem();
#ifdef CFG_BOOT_TESTBIN_ENABLE	
	DoBootTestBin();
#endif
}

static void getUpgradeMode(uint32_t* pUpgradeMode)
{
	*pUpgradeMode = 0x0;
	
#ifdef CFG_UPGRADE_OPEN_FILE
	*pUpgradeMode |= STORAGE_UPGRADE_SUPPORT;
#endif

#ifdef CFG_ENABLE_UART_CLI
	*pUpgradeMode |= CLI_UPGRADE_SUPPORT;
#endif
	
#ifdef CFG_UPGRADE_FROM_UART_BOOT_TIME
	*pUpgradeMode |= UART_UPGRADE_SUPPORT;
#endif

#ifdef CFG_UPGRADE_USB_DEVICE
	*pUpgradeMode |= USB_DEVICE_UPGRADE_SUPPORT;
#endif

#ifdef CFG_UPGRADE_PRESSKEY
	*pUpgradeMode |= PRESSKEY_UPGRADE_SUPPORT;
#endif
	
#ifdef CFG_UPGRADE_RESET_FACTORY
	*pUpgradeMode |= RESET_FACTORY_SUPPORT;
#endif
	
#ifdef CFG_UPGRADE_RECOVERY
	*pUpgradeMode |= RECOVERY_UPGRADE_SUPPORT;
#endif

#ifdef CFG_BOOT_TESTBIN_ENABLE
	*pUpgradeMode = TESTBIN_UPGRADE_SUPPORT;
#endif
}

void* BootloaderMain(void* arg)
{
	uint32_t upgradeMode = 0;
#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    UG_UPGRADE_FLOW_TAG ugUpgradeCheck_ret = UG_UPGRADE_FLOW_UNKNOWN;
#if defined(CFG_NAND_RESERVED_SIZE)
    uint32_t FAT_OFFSET = 0x40000;
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_NAND_RESERVED_SIZE + FAT_OFFSET;
#elif defined(CFG_NOR_RESERVED_SIZE)
    uint32_t FAT_OFFSET = 0x10000;
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_NOR_RESERVED_SIZE + FAT_OFFSET;
#elif defined(CFG_SD0_RESERVED_SIZE)
    uint32_t FAT_OFFSET = 0x2000;
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_SD0_RESERVED_SIZE + FAT_OFFSET;
#elif defined(CFG_SD1_RESERVED_SIZE)
    uint32_t FAT_OFFSET = 0x2000;
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_SD1_RESERVED_SIZE + FAT_OFFSET;
#else
    #error "unknown device."
#endif
#endif

#ifdef CFG_WATCHDOG_ENABLE
    ioctl(ITP_DEVICE_WATCHDOG, ITP_IOCTL_DISABLE, NULL);
#endif

    ithMemDbgDisable(ITH_MEMDBG0);
    ithMemDbgDisable(ITH_MEMDBG1);

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    ugUpgradeCheck_ret = ugUpgradeCheck();

    if (ugUpgradeCheck_ret == UG_UPGRADE_FLOW_FINISH)
    {
        printf("[upgrading check] upgrading finish!\n");
    }
    else if (ugUpgradeCheck_ret == UG_UPGRADE_FLOW_FW)
    {
        printf("[upgrading check] upgrading FW fail!\n");
        if (ugCopyRawData(CFG_UPGRADE_IMAGE_POS,
                CFG_UPGRADE_BACKUP_RAW_DATA_FW_POS,
                CFG_UPGRADE_BACKUP_RAW_DATA_FW_SIZE))
        {
            printf("ugCopyRawData fail_0\n");
            return NULL;
        }

        if (ugUpgradeTag(UG_UPGRADE_FLOW_FINISH))
        {
            printf("ugUpgradeTag fail_0\n");
            return NULL;
        }
    }
    else if (ugUpgradeCheck_ret == UG_UPGRADE_FLOW_FAT_P0)
    {
        printf("[upgrading check] upgrading FAT_P0 fail!\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
#if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
#endif
        }

        if (ugCopyRawData(RESERVED_SIZE_AND_FAT_OFFSET,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION0_POS,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION0_SIZE))
        {
            printf("ugCopyRawData fail_1_0\n");
            return NULL;
        }

        if (ugUpgradeTag(UG_UPGRADE_FLOW_FINISH))
        {
            printf("ugUpgradeTag fail_1_0\n");
            return NULL;
        }
    }
    else if (ugUpgradeCheck_ret == UG_UPGRADE_FLOW_FAT_P1)
    {
        printf("[upgrading check] upgrading FAT_P1 fail!\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
#if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
#endif
        }

        if (ugCopyRawData(RESERVED_SIZE_AND_FAT_OFFSET + CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION0_SIZE,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION1_POS,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION1_SIZE))
        {
            printf("ugCopyRawData fail_1_1\n");
            return NULL;
        }

        if (ugUpgradeTag(UG_UPGRADE_FLOW_FINISH))
        {
            printf("ugUpgradeTag fail_1_1\n");
            return NULL;
        }
    }
    else if (ugUpgradeCheck_ret == UG_UPGRADE_FLOW_FAT_P2)
    {
        printf("[upgrading check] upgrading FAT_P2 fail!\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
#if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
#endif
        }

        if (ugCopyRawData(RESERVED_SIZE_AND_FAT_OFFSET + CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION0_SIZE + CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION1_SIZE,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION2_POS,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION2_SIZE))
        {
            printf("ugCopyRawData fail_1_2\n");
            return NULL;
        }

        if (ugUpgradeTag(UG_UPGRADE_FLOW_FINISH))
        {
            printf("ugUpgradeTag fail_1_2\n");
            return NULL;
        }
    }
    else if (ugUpgradeCheck_ret == UG_UPGRADE_FLOW_FAT_P3)
    {
        printf("[upgrading check] upgrading FAT_P3 fail!\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
#if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
#endif
        }

        if (ugCopyRawData(RESERVED_SIZE_AND_FAT_OFFSET + CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION0_SIZE + CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION1_SIZE + CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION2_SIZE + FAT_OFFSET,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION3_POS,
                CFG_UPGRADE_BACKUP_RAW_DATA_FAT_PARTITION3_SIZE))
        {
            printf("ugCopyRawData fail_1_3\n");
            return NULL;
        }

        if (ugUpgradeTag(UG_UPGRADE_FLOW_FINISH))
        {
            printf("ugUpgradeTag fail_1_3\n");
            return NULL;
        }
    }
    else
    {
        printf("[upgrading check] unknown upgrading fail!\n");
        printf("[upgrading check] must do upgrading PKG again!\n");
    }
#else
	if (ugUpgradeCheck())
        upgradeCheckFail = true;
#endif

	//load image
	BeginLoadImage();
	
#ifdef CFG_BL_SHOW_LOGO
	BootloaderShowLogo();
#endif
	getUpgradeMode(&upgradeMode);
	
	//upgrade pkg
	if (upgradeMode & STORAGE_UPGRADE_SUPPORT)
	{
		ProcessStorageUpgrade();
	}
	
	if (upgradeMode & CLI_UPGRADE_SUPPORT)
	{
		ProcessCliUpgrade();
	}
	
	if (upgradeMode & UART_UPGRADE_SUPPORT)
	{
		ProcessUartUpgrade();
	}

	if (upgradeMode & USB_DEVICE_UPGRADE_SUPPORT)
	{
		ProcessUsbDeviceUpgrade();
	}

	if(upgradeMode & PRESSKEY_UPGRADE_SUPPORT)
	{
		ProcessPressKeyUpgrade();
	}

	if(upgradeMode & RESET_FACTORY_SUPPORT)
	{
		ProcessResetFactory();
	}

	if(upgradeMode & RECOVERY_UPGRADE_SUPPORT)
	{
		ProcessRecoveryUpdate();
	}

	if(upgradeMode & TESTBIN_UPGRADE_SUPPORT)
	{
		ProcessTestBinUpdate();
	}

	if (upgradeCheckFail)
	{
		InitLcdConsole();
#ifdef CFG_UPGRADE_BACKUP_PACKAGE
		ProcessBackupPackageUpdate();
#else
		ShowLastUpgradeFail();
		sleep(10);
#endif
	}

	LOG_INFO "Do Booting...\r\n" LOG_END
    EndLoadImage(true);
    BootImage();

    return NULL;
}
