#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "curl/curl.h"
#include "ite/ite_usbex.h"
#include "bootloader.h"
#include "config.h"
#include "ite/itp.h"

static pthread_t loadImageTask;
static bool upgradeCheckFail = false;
static bool blLcdOn = false;
static bool blLcdConsoleOn = false;
#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
static bool usbInited = false;
#endif

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
extern bool fatInited;
extern bool driveInited;
#endif

#ifdef CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
extern unsigned char gDoReMapFlow;
#endif

#ifdef CFG_BL_DLM_ENABLE
#if defined (CFG_UPGRADE_IMAGE_NOR)
static void LoadBLDLModule(void)
{
    #include <ssp/mmp_axispi.h>
    extern char __dlm_start[];
    extern char __dlm_bss_start[];
    extern char __dlm_end[];

	uint8_t header[20];
	uint8_t *content_buf;
	uint32_t header_size, content_size;

	NorRead(SPI_0, SPI_CSN_0, CFG_UPGRADE_BL_DLM_PACKAGE_POS, header, 20);
	header_size = ((uint32_t)header[12] << 24) | ((uint32_t)header[13] << 16) | ((uint32_t)header[14] << 8) | (uint32_t)header[15];
	content_size = ((uint32_t)header[16] << 24) | ((uint32_t)header[17] << 16) | ((uint32_t)header[18] << 8) | (uint32_t)header[19];

	content_buf = memalign(32, ITH_ALIGN_UP(content_size,32));
	NorRead(SPI_0, SPI_CSN_0, CFG_UPGRADE_BL_DLM_PACKAGE_POS + header_size, content_buf, content_size);

	// hardware decompress
	if (strncmp(content_buf, "SMAZ", 4) == 0)
	{
		uint8_t dcpsMode = ITP_DCPS_UCL_MODE;
        ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, NULL);
		ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_SET_MODE, &dcpsMode);

        if (write(ITP_DEVICE_DECOMPRESS, content_buf+8, content_size) == content_size)
        {
        	int dlm_size = 0;
			ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_GET_SIZE, &dlm_size);
            read(ITP_DEVICE_DECOMPRESS, __dlm_start, dlm_size);
        }

        ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_EXIT, NULL);
	}
	free(content_buf);

    ithInvalidateICache();
    memset(__dlm_bss_start, 0, __dlm_end - __dlm_bss_start);
}
#elif defined (CFG_UPGRADE_IMAGE_NAND)
static void LoadBLDLModule(void)
{
	#include <ssp/mmp_axispi.h>
    extern char __dlm_start[];
    extern char __dlm_bss_start[];
    extern char __dlm_end[];

	#define MAX_HEADER_SIZE     0x10000
	#define HEADER_LEN_OFFSET   12

	uint32_t ret, headersize, alignsize, blockcount, blocksize, contentsize, pos, gapsize;
    uint8_t *rombuf = NULL, *content_buf;
    int fd = -1;

	fd = open(":nand", O_RDONLY, 0);
    LOG_DBG "nand fd: 0x%X\n", fd LOG_END

	if (fd == -1)
    {
        LOG_ERR "open device error: %d\n", fd LOG_END
        goto end;
    }

	if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        LOG_ERR "get block size error\n" LOG_END
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        LOG_ERR "get gap size error:\n" LOG_END
        goto end;
    }

	// read rom header size
    pos = CFG_UPGRADE_BL_DLM_PACKAGE_POS / (blocksize + gapsize);
    blockcount = CFG_UPGRADE_BL_DLM_PACKAGE_POS % (blocksize + gapsize);
    if (blockcount > 0)
    {
        LOG_WARN "rom position 0x%X and block size 0x%X are not aligned; align to 0x%X\n", CFG_UPGRADE_IMAGE_POS, blocksize, (pos * blocksize) LOG_END
    }

    LOG_DBG "blocksize=%d pos=0x%X align=%d\n", blocksize, pos, blockcount LOG_END

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        LOG_ERR "seek to rom position %d error\n", pos LOG_END
        goto end;
    }

    alignsize = ITH_ALIGN_UP(MAX_HEADER_SIZE, blocksize);
    rombuf = malloc(alignsize);
    if (!rombuf)
    {
        LOG_ERR "out of memory %d\n", alignsize LOG_END
        goto end;
    }

    blockcount = alignsize / blocksize;
    ret = read(fd, rombuf, blockcount);
    if (ret != blockcount)
    {
        LOG_ERR "read rom header error: %d != %d\n", ret, blockcount LOG_END
        goto end;
    }

    headersize = *(uint32_t*)(rombuf + HEADER_LEN_OFFSET);
    headersize = itpBetoh32(headersize);
    LOG_DBG "rom header size: %d\n", headersize LOG_END

	contentsize = *(uint32_t*)(rombuf + HEADER_LEN_OFFSET + 4);
	contentsize = itpBetoh32(contentsize);
	LOG_DBG "compress rom, content size: %d\n", contentsize LOG_END

	content_buf = rombuf + headersize;

	// hardware decompress
	if (strncmp(content_buf, "SMAZ", 4) == 0)
	{
		uint8_t dcpsMode = ITP_DCPS_UCL_MODE;
        ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, NULL);
		ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_SET_MODE, &dcpsMode);

        if (write(ITP_DEVICE_DECOMPRESS, content_buf+8, contentsize) == contentsize)
        {
        	int dlm_size = 0;
			ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_GET_SIZE, &dlm_size);
            read(ITP_DEVICE_DECOMPRESS, __dlm_start, dlm_size);
        }

        ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_EXIT, NULL);
	}
	
	ithInvalidateICache();
    memset(__dlm_bss_start, 0, __dlm_end - __dlm_bss_start);
end:
    if (fd != -1)
        close(fd);
	if(rombuf)
		free(rombuf);
}
#endif
#endif

#ifdef CFG_BL_SHOW_LOGO
static void BootloaderShowLogo()
{
	ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);
    ShowLogo();
    ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_RESET, NULL);
    blLcdOn = true;
}

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
#endif

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

        InitLcdConsole();
#ifdef CFG_BL_SHOW_LOGO		
        EndLoadImage(false);
#endif
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

static void ProcessStorageUpgrade()
{
	InitFileSystem();
    DoUpgrade();
}

#ifdef CFG_NET_WIFI
#include "wifiMgr.h"

/* WIFI Globel Varibles */
WIFI_MGR_SETTING        gWifiSetting;
static   int            gWifiPowerOn = 0;
unsigned int            CallBack_Connection_Finish = 0;

/* Connect Info */
#define SSID  (CFG_APP_STA_SSID)
#define PW    (CFG_APP_STA_PW)
#define SEC           "6"

#if defined (CFG_SD0_ENABLE) && defined (CFG_SD1_ENABLE)
#define SD_PORT_NUM 1
#elif defined (CFG_SD0_ENABLE)
#define SD_PORT_NUM 0
#elif defined (CFG_SD1_ENABLE)
#define SD_PORT_NUM 1
#endif

extern void wifi_drv_on(int nSD);

static void WifiFirstPowerOn(void)
{
    wifi_drv_on(SD_PORT_NUM);
}

static void WifiPowerOn(void)
{
    if (gWifiPowerOn==0){
        WifiFirstPowerOn();
        gWifiPowerOn++;
    } else {
        //wifi_on(RTW_MODE_STA);
        wifiMgr_init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
    }

}

static void itpInit_Wifi(void)
{
    int err = 0;
	
    printf("====>itpInit_Wifi start\n");

#ifdef CFG_RTC_ENABLE
    itpRegisterDevice(ITP_DEVICE_RTC, &itpDeviceRtc);
    ioctl(ITP_DEVICE_RTC, ITP_IOCTL_INIT, NULL);
#endif // CFG_RTC_ENABLE

    // init card device
#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)
    itpRegisterDevice(ITP_DEVICE_CARD, &itpDeviceCard);
    ioctl(ITP_DEVICE_CARD, ITP_IOCTL_INIT, NULL);
#endif

    // init sdio device
#ifdef CFG_SDIO_ENABLE
    //RTK WIFI excute power/regist process for 9860
    #if defined (CFG_GPIO_SD_WIFI_POWER_ENABLE) && defined (CFG_NET_WIFI_SDIO_VND_RTK)
    ithWIFICardPowerProcess();
    #endif

	sleep(1);

#ifndef CFG_NET_WIFI_SDIO_POWER_ON_OFF_USER_DEFINED
    itpRegisterDevice(ITP_DEVICE_SDIO, &itpDeviceSdio);
	printf("====>SDIO init start\n");
    err = ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);
#endif
#endif

    // init sd0 device
#ifdef CFG_SD0_STATIC
    itpRegisterDevice(ITP_DEVICE_SD0, &itpDeviceSd0);
    printf("====>SD0 init start\n");
    ioctl(ITP_DEVICE_SD0, ITP_IOCTL_INIT, NULL);
#endif

    // init sd1 device
#ifdef CFG_SD1_STATIC
    itpRegisterDevice(ITP_DEVICE_SD1, &itpDeviceSd1);
    printf("====>SD1 init start\n");
    ioctl(ITP_DEVICE_SD1, ITP_IOCTL_INIT, NULL);
#endif

    // enable gpio interrupt
    ithIntrEnableIrq(ITH_INTR_GPIO);

#ifdef CFG_NET_WIFI
    itpRegisterDevice(ITP_DEVICE_WIFI, &itpDeviceWifi);
#endif

    // init WiFi device (MHD)
    // Don't init wifi here if control from user
    #if !defined (CFG_NET_WIFI_SDIO_POWER_ON_OFF_USER_DEFINED)
	if(err == 0) {
        printf("====>WIFI init start\n");
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_INIT, NULL);

        #if defined(CFG_NET_WIFI_SDIO_VND_RTK)
        itpRegisterDevice(ITP_DEVICE_WIFI_NGPL, &itpDeviceWifiNgpl);
        #endif
    } else {
        printf("====>itpInit SD init ERR\n");
	}
    #endif

    // init socket device
#if defined(CFG_NET_ENABLE) && !defined(CFG_NET_WIFI_REDEFINE) && \
        !defined(CFG_USBD_NCM) && !defined(CFG_NET_WIFI_SDIO_VND_MHD)
    itpRegisterDevice(ITP_DEVICE_SOCKET, &itpDeviceSocket);
    ioctl(ITP_DEVICE_SOCKET, ITP_IOCTL_INIT, NULL);
#endif
}

static void PreSettingWifi(void)
{
    int rc = 0;
    memset(&gWifiSetting.setting, 0, sizeof (ITPEthernetSetting));

    gWifiSetting.setting.index  = 0;
    gWifiSetting.setting.dhcp   = 1;
    gWifiSetting.setting.autoip = 0;

    for (int i = 0; i < 4; i++){
        gWifiSetting.setting.ipaddr[i] = 0;
        gWifiSetting.setting.netmask[i]= 0;
        gWifiSetting.setting.gw[i]     = 0;
    }

    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL);
}

int CallbackFucntion(int nState)
{
    switch (nState)
    {
        case WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH:
            if (CallBack_Connection_Finish < 1)
                CallBack_Connection_Finish = 1;
            printf("\n[TEST]WifiCallback connection finish[%d] \n", CallBack_Connection_Finish);
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT:
            if (CallBack_Connection_Finish > 0)
                CallBack_Connection_Finish = 0;
            printf("[TEST]WifiCallback disconnection finish[%d] \n", CallBack_Connection_Finish);
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S:
            printf("[TEST]WifiCallback connection disconnect 30s \n");
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_RECONNECTION:
            printf("[TEST]WifiCallback connection reconnection \n");
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT:
            printf("[TEST]WifiCallback connection temp disconnect \n");
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL:
            printf("[TEST]WifiCallback connecting fail, please check ssid,password,secmode \n");
			WifiMgr_Sta_Disconnect();
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_CANCEL:
            printf("[TEST]WifiCallback connecting end to sleep/cancel \n");
        break;

        default:
            printf("[TEST]WifiCallback unknown %d state  \n",nState);
        break;
    }
}

struct MemoryStruct
{
  char *memory;
  size_t size;
};

static ITCArrayStream arrayStream;
static size_t throw_away(void *ptr, size_t size, size_t nmemb, void *data)
{
    (void)ptr;
    (void)data;
    /* we are not interested in the headers itself,
     so we only return the size we would have saved ... */
    return (size_t)(size * nmemb);
}

static int GetPackageSize(char* ftpurl)
{
    CURL* curl = NULL;
    CURLcode res = CURLE_OK;
    double filesize = 0.0;

    curl = curl_easy_init();
    if (!curl)
    {
        printf("curl_easy_init() fail.\n");
        goto end;
    }

    curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, throw_away);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);

    /* some servers don't like requests that are made without a user-agent
     field, so we provide one */ 
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 15L);

#ifndef NDEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

    res = curl_easy_perform(curl);
    if (CURLE_OK != res)
    {
        printf("curl_easy_perform() fail: %d\n", res);
        goto end;
    }

    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
    if ((CURLE_OK == res) && (filesize > 0.0))
    {
        printf("filesize: %0.0f bytes\n", filesize);
    }
    else
    {
        printf("curl_easy_getinfo(CURLINFO_CONTENT_LENGTH_DOWNLOAD) fail: %d, filesize: %0.0f bytes\n", res, filesize);
        filesize = 0.0;
        goto end;
    }

end:
    if (curl)
        curl_easy_cleanup(curl);

    return (int)filesize;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    assert(mem->memory);

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;

    return realsize;
}

static ITCStream* DownloadPackage(char* ftpurl, int filesize)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(filesize);    /* will be grown as needed by the realloc above */ 
    chunk.size = 0;                     /* no data at this point */ 

    /* init the curl session */ 
    curl = curl_easy_init();
    if (!curl)
    {
        printf("curl_easy_init() fail.\n");
        goto error;
    }

    /* specify URL to get */ 
    curl_easy_setopt(curl, CURLOPT_URL, ftpurl);

    /* send all data to this function  */ 
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
     field, so we provide one */ 
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 15L);
//#ifndef NDEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//#endif

    /* get it! */
    res = curl_easy_perform(curl);
    if (CURLE_OK != res)
    {
        printf("curl_easy_perform() fail: %d\n", res);
        goto error;
    }
    else
    {
        printf("%lu bytes retrieved\n", (long)chunk.size);
    }

    curl_easy_cleanup(curl);

    itcArrayStreamOpen(&arrayStream, chunk.memory, chunk.size);

    return &arrayStream.stream;

error:
    if (curl)
        curl_easy_cleanup(curl);

    free(chunk.memory);

    return NULL;
}

static ITCStream* OpenDownloadPackage(char* path)
{
    int filesize;
    ITCStream* fwStream = NULL;
    int retry = 10;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    printf("ftp url: %s\n", path);

    // get file size first
    while (retry-- >= 0)
    {
        filesize = GetPackageSize(path);
        if (filesize > 0)
            break;
    }

    // download firmware
    while (retry-- >= 0)
    {
        fwStream = DownloadPackage(path, filesize);
        if (fwStream)
            break;
    };

    curl_global_cleanup();
    return fwStream;
}
#endif

static void ProcessWifiUpgrade(void)
{
	int ret = 0;
	unsigned long mscnt = 0;
	ITCStream* upgradeFile;
	
#if defined(CFG_BL_DLM_ENABLE)
	LoadBLDLModule();	
#endif

#ifdef CFG_NET_WIFI
	WifiPowerOn();
	itpInit_Wifi();
    usleep(5*100*1000);

    gWifiSetting.wifiCallback = CallbackFucntion;
    PreSettingWifi();

    WifiMgr_Sta_Switch(WIFIMGR_SWITCH_ON);
    WifiMgr_Init(0, 0, gWifiSetting);

    //Connect infomation
	printf("====>ProcessWifiUpgrade: Connecting\n");
	WifiMgr_Sta_Connect(SSID, PW, SEC);

    /* Wait the first connection done. */
    while (!CallBack_Connection_Finish)
    {
        usleep(100*1000);
        mscnt += 100;
        putchar('-');
        fflush(stdout);
    }

    printf("====>ProcessWifiUpgrade: Connected\n");

	while(!WifiMgr_Get_Sta_Connect_Complete()) {
        printf("=\n");
        usleep(100*1000);
    }

	InitFileSystem();
	upgradeFile = OpenDownloadPackage(CFG_UPGRADE_URL);
	if(upgradeFile)
	{
		InitLcdConsole();
		if (ugCheckCrc(upgradeFile, NULL))
	        LOG_ERR "Recovery failed.\n" LOG_END
	    else
	        ret = ugUpgradePackage(upgradeFile);
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

		exit(ret);
        while (1);
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

	#if defined(CFG_FS_FAT) || defined(CFG_FS_LFS)
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
	// mount disks on booting
	ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, NULL);
	#endif
	#endif // !CFG_UPGRADE_USB_DEVICE

	RestorePackage();
#endif
}

void* BootloaderMain(void* arg)
{
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
#endif

#ifdef CFG_GPIO_STORAGE_UPGRADE
    ithGpioSetMode(CFG_GPIO_STORAGE_UPGRADE, ITH_GPIO_MODE0);
    ithGpioSetIn(CFG_GPIO_STORAGE_UPGRADE);
    ithGpioEnable(CFG_GPIO_STORAGE_UPGRADE);

    if(!ithGpioGet(CFG_GPIO_STORAGE_UPGRADE))
    {
        ProcessStorageUpgrade();
    }
#endif
#ifdef CFG_GPIO_WIFI_UPGRADE
    ithGpioSetMode(CFG_GPIO_WIFI_UPGRADE, ITH_GPIO_MODE0);
    ithGpioSetIn(CFG_GPIO_WIFI_UPGRADE);
    ithGpioEnable(CFG_GPIO_WIFI_UPGRADE);

    if(!ithGpioGet(CFG_GPIO_WIFI_UPGRADE))
    {
        ProcessWifiUpgrade();
    }
#endif

#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
	if (ugUpgradeCheck())
        upgradeCheckFail = true;
#endif

	if(!upgradeCheckFail)
	{
		//load image
#ifndef CFG_BL_SHOW_LOGO
		LoadImageDirect();
#else
		BeginLoadImage();
		BootloaderShowLogo();
		EndLoadImage(true);
#endif
	}
	else
	{
#ifdef CFG_UPGRADE_BACKUP_PACKAGE
		InitLcdConsole();
		ProcessBackupPackageUpdate();
#endif
	}

	LOG_INFO "Do Booting...\r\n" LOG_END
	BootImage();
	return NULL;
}

