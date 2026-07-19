#include <sys/ioctl.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "curl/curl.h"
#include "bootloader.h"
#include "config.h"

#ifdef CFG_NET_ENABLE
#include "ite/itp.h"
#include <time.h>
#include <sys/ioctl.h>
#include <pthread.h>

/*=====================  WiFi ====================*/
#ifdef CFG_NET_WIFI
#include "wifiMgr.h"

/* Connect Info */
#define SSID "IOT_PZ_2072"
#define PW   "12345678"
#define SEC  "7"

/* WIFI Globel Varibles */
WIFI_MGR_SETTING        gWifiSetting;
static   int            gWifiPowerOn = 0;
unsigned int            CallBack_Connection_Finish = 0;

#if defined (CFG_NET_WIFI_SDIO_VND_MHD)
#if defined (CFG_SD0_ENABLE) && defined (CFG_SD1_ENABLE)
#define SD_PORT_NUM 1
#elif defined (CFG_SD0_ENABLE)
#define SD_PORT_NUM 0
#elif defined (CFG_SD1_ENABLE)
#define SD_PORT_NUM 1
#endif

extern void wifi_drv_on(int nSD);

void WifiFirstPowerOn(void)
{
    wifi_drv_on(SD_PORT_NUM);
}

void WifiPowerOn(void)
{
    if (gWifiPowerOn==0){
        WifiFirstPowerOn();
        gWifiPowerOn++;
    } else {
        //wifi_on(RTW_MODE_STA);
        WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
    }
}
#endif

static void PreSettingWifi(void)
{
	memset(&gWifiSetting.setting, 0, sizeof (ITPEthernetSetting));

	gWifiSetting.setting.index = 0;
    gWifiSetting.setting.dhcp   = 1;
    gWifiSetting.setting.autoip = 0;

    for (int i = 0; i < 4; i++){
        gWifiSetting.setting.ipaddr[i] = 0;
        gWifiSetting.setting.netmask[i]= 0;
        gWifiSetting.setting.gw[i]     = 0;
    }

    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL);
}


static int wifiCallbackFucntion(int nState)
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

void* WifiTask(void* arg)
{
	//Connect infomation
	sleep(5);
	WifiMgr_Sta_Connect(SSID, PW, SEC);
	usleep(500*1000);

    while(!ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_IS_AVAIL, NULL)){
        usleep(100*1000);
    }

	for (;;)
	{
		sleep(1);
	}
	return NULL;
}
#endif

/*===================== Ethernet ====================*/
#ifdef CFG_NET_ETHERNET
#define DHCP_TIMEOUT_MSEC (5 * 1000) //5sec
static bool networkIsReady, networkToReset;

static void ResetEthernet(void)
{
    ITPEthernetSetting setting;
    ITPEthernetInfo info;
	unsigned long       mscnt = 0;
    char buf[16], *saveptr;

    printf("====>ResetEthernet\n");
    memset(&setting, 0, sizeof (ITPEthernetSetting));

    setting.index = 0;

    // dhcp
    setting.dhcp = 0;

    // autoip
    setting.autoip = 0;

    // ipaddr
    strcpy(buf, "192.168.190.61");
    setting.ipaddr[0] = atoi(strtok_r(buf, ".", &saveptr));
    setting.ipaddr[1] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.ipaddr[2] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.ipaddr[3] = atoi(strtok_r(NULL, " ", &saveptr));

    // netmask
    strcpy(buf, "255.255.255.0");
    setting.netmask[0] = atoi(strtok_r(buf, ".", &saveptr));
    setting.netmask[1] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.netmask[2] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.netmask[3] = atoi(strtok_r(NULL, " ", &saveptr));

    // gateway
    strcpy(buf, "192.168.190.1");
    setting.gw[0] = atoi(strtok_r(buf, ".", &saveptr));
    setting.gw[1] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.gw[2] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.gw[3] = atoi(strtok_r(NULL, " ", &saveptr));

    ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_RESET, &setting);

    printf("Wait ethernet cable to plugin");
	while (!ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_CONNECTED, NULL))
	{
		sleep(1);
		putchar('.');
		fflush(stdout);
	}

	printf("\nWait DHCP settings");
	while (!ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_AVAIL, NULL))
	{
		usleep(100000);
		mscnt += 100;

		putchar('.');
		fflush(stdout);

		if (mscnt >= DHCP_TIMEOUT_MSEC)
		{
			printf("\nDHCP timeout, use default settings\n");
            setting.dhcp = setting.autoip = 0;
            ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_RESET, &setting);
			break;
		}
	}
	puts("");

	if (ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_AVAIL, NULL))
	{
        char* ip;

        info.index = 0;
        ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_GET_INFO, &info);
        ip = ipaddr_ntoa((const ip_addr_t*)&info.address);

        printf("Ethernet IP address: %s\n", ip);

		networkIsReady = true;
	}
}

static void* EthTask(void* arg)
{
	ResetEthernet();

	for (;;)
	{
		networkIsReady = ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_CONNECTED, NULL);
		if (networkToReset)
		{
			ResetEthernet();
			networkToReset = false;
		}

		usleep(100*1000);
	}
	return NULL;
}

void PreSettingEth(void)
{
    networkIsReady = false;
    networkToReset = false;
}
#endif

static void itpInit_partial(void)
{
    int err = 0;

    // init ethernet device
#if defined(CFG_NET_ETHERNET) \
    || defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM) || defined(CFG_USBD_NCM)
    itpRegisterDevice(ITP_DEVICE_ETHERNET, &itpDeviceEthernet);
    ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_INIT, NULL);
#endif

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

    itpRegisterDevice(ITP_DEVICE_SDIO, &itpDeviceSdio);
    printf("====>SDIO init start\n");
    err = ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);
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

    // init SDIO type Wifi device
#if defined(CFG_NET_WIFI) && defined(CFG_NET_WIFI_SDIO)
    itpRegisterDevice(ITP_DEVICE_WIFI, &itpDeviceWifi);

    if(err == 0) {
        printf("====>itpInit itpRegisterDevice(ITP_DEVICE_WIFI_NGPL)\n");
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_INIT, NULL);

        #if defined(CFG_NET_WIFI_SDIO_VND_RTK)
        itpRegisterDevice(ITP_DEVICE_WIFI_NGPL, &itpDeviceWifiNgpl);
        #endif
    } else {
        printf("====>itpInit SDIO init err\n");
			}
#endif

    // init socket device
#if defined(CFG_NET_ENABLE) && !defined(CFG_NET_WIFI_REDEFINE) && \
        !defined(CFG_USBD_NCM) && !defined(CFG_NET_WIFI_SDIO_VND_MHD)
    itpRegisterDevice(ITP_DEVICE_SOCKET, &itpDeviceSocket);
    ioctl(ITP_DEVICE_SOCKET, ITP_IOCTL_INIT, NULL);
#endif
}

void NetworkInit(void)
{
    pthread_t task_eth, task_wifi;
    pthread_attr_t attr_eth, attr_wifi;

#ifdef CFG_NET_ETHERNET
    printf("====>Ethernet NetworkInit\n");
    PreSettingEth();

    pthread_attr_init(&attr_eth);
    pthread_create(&task_eth, &attr_eth, EthTask, NULL);
#endif

#ifdef CFG_NET_WIFI
    #ifdef CFG_NET_WIFI_SDIO_VND_MHD
    WifiPowerOn();
    #endif

	gWifiSetting.wifiCallback = wifiCallbackFucntion;
    PreSettingWifi();

    WifiMgr_Sta_Switch(1);
    WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);

    pthread_attr_init(&attr_wifi);
    pthread_create(&task_wifi, &attr_wifi, WifiTask, NULL);
#endif
}
#endif

static ITCArrayStream arrayStream;
extern char tftppara[128];


struct FtpBuf
{
    uint8_t* buf;
    uint32_t pos;
};

static size_t FtpWrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
    struct FtpBuf* out = (struct FtpBuf*)stream;
    size_t s;

    //LOG_DBG "FtpWrite(0x%X,%d,%d,0x%X)\n", buffer, size, nmemb, stream LOG_END

    assert(out->buf);
    s = size * nmemb;
    memcpy(&out->buf[out->pos], buffer, s);
    out->pos += s;

	LOG_DBG "FtpWrite(size: %d, pos: %d)\n", (int)s, out->pos LOG_END
    return s;
}

ITCStream* OpenRecoveryPackage(void)
{
    CURL *curl;
    CURLcode res;
    struct FtpBuf ftpBuf;
    unsigned long mscnt = 0;

    itpInit_partial();

    NetworkInit();

#if defined(CFG_NET_ETHERNET)
	while (!networkIsReady)
#elif defined(CFG_NET_WIFI)
    /* Wait the first connection done. */
    while (!CallBack_Connection_Finish)
#endif
	{
        usleep(100*1000);
        mscnt += 100;
        putchar('-');
        fflush(stdout);
	}

    printf("====>Connected Finish in bootloader\n");

	ftpBuf.buf = malloc(0x1000000);
	if (!ftpBuf.buf)
	{
		LOG_ERR "malloc fail.\n" LOG_END
		return NULL;
	}
    ftpBuf.pos = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl)
    {
        LOG_ERR "curl_easy_init() fail.\n" LOG_END
        goto error;
    }
#if defined (CFG_UPGRADE_RECOVERY_TFTP)
    curl_easy_setopt(curl, CURLOPT_URL, "tftp://" CFG_UPGRADE_RECOVERY_SERVER_ADDR "/" CFG_UPGRADE_FILENAME);
#elif defined (CFG_ENABLE_UART_CLI)
	printf("\ntftppara=%s\n", tftppara);
	curl_easy_setopt(curl, CURLOPT_URL, tftppara);
#else
    curl_easy_setopt(curl, CURLOPT_URL, "ftp://192.168.1.2" "/" CFG_UPGRADE_FILENAME);
#endif
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FtpWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpBuf);

#ifndef NDEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    res = curl_easy_perform(curl);
    /* always cleanup */
    curl_easy_cleanup(curl);
    if (CURLE_OK != res)
    {
        LOG_ERR "curl fail: %d\n", res LOG_END
        goto error;
    }
    curl_global_cleanup();
    itcArrayStreamOpen(&arrayStream, ftpBuf.buf, ftpBuf.pos);
    return &arrayStream.stream;

error:
    curl_global_cleanup();
    return NULL;
}
