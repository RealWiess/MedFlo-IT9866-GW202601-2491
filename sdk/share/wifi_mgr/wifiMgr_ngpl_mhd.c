#if defined(CFG_NET_WIFI_SDIO_VND_MHD)
#include <sys/ioctl.h>
#include "lwip/dhcp.h"
#include "wifiMgr.h"
#include "ite/ite_wifi.h"
#include "mhd_api.h"
#include "mhd_ite_utils.h"



/* Global Variable */
static pthread_t                         ClientModeTask, ProcessTask;

static sem_t                             semConnectStart, semConnectStop;
static pthread_mutex_t                   mutexALWiFi, mutexIni, mutexMode;

#if WIFIMGR_EXCUTE_WPS
static sem_t                             semWPSStart, semWPSStop;
static pthread_mutex_t                   mutexALWiFiWPS;
#endif

static sem_t                             semSwitchModeStart, semSwitchModeStop;


static WIFIMGR_CONNSTATE_E               wifi_conn_state    = WIFIMGR_CONNSTATE_STOP;
static WIFIMGR_ECODE_E                   wifi_conn_ecode    = WIFIMGR_ECODE_SET_DISCONNECT;

static WIFI_MGR_SCANAP_LIST              gWifiMgrApList[WIFI_SCAN_LIST_NUMBER] = {0};

static WIFI_MGR_SETTING                  gWifiMgrSetting    = {0};
static struct net_device_info            gScanApInfo        = {0};

static struct timeval                   tvDHCP1 = {0, 0}, tvDHCP2     = {0, 0};

static int gDisableAutoReconnect = 0;
static int gSoftAP_5G=0;
static int gReConnecting = 0;

/* WifiMgr flags */
static WIFI_MGR_VAR                      gWifiMgrVar        =
{
    .WIFI_Mode              = WIFIMGR_MODE_CLIENT,
    .MP_Mode                = 0,
    .Need_Set               = false,
    .Pre_Scan               = false,
    .Start_Scan             = false,
    .WIFI_Init_Ready        = false,
    .SoftAP_Hidden          = false,
    .SoftAP_Init_Ready      = false,
    .WIFI_Terminate         = false,
    .WPA_Terminate          = false,
    .Cancel_Connect         = false,
    .Cancel_WPS             = false,
    .Is_WIFI_Available      = false,
    .IS_WifiMgr_Switch_Mode_Thread  = false,
    .IS_WifiMgr_Main_Process_Thread = false
};

#define FORMAT_IPADDR(x) ((unsigned char *)&x)[3], ((unsigned char *)&x)[2], ((unsigned char *)&x)[1], ((unsigned char *)&x)[0]

//#define Carplay_Example
#ifdef Carplay_Example
#define MY_OUI               "\x00\xA0\x40"
#define MY_OUI_TYPE          2
#define MY_IE_DATA           "MY-IE"
#define MY_VENDOR_IE_BEACON  0x1

#endif


/* ================================================= */
/*                                                   */
/* Static Functions                                  */
/*                                                   */
/* ================================================= */

static char *_WifiMgr_Sta_Get_Security_String_By_Index(uint8_t security)
{
    return ( security == MHD_SECURE_OPEN ) ? "OPEN":
           ( security == MHD_WPA_PSK_AES ) ? "WPA PSK TKIP":
           ( security == MHD_WPA2_PSK_AES ) ? "WPA2 PSK AES":
           ( security == MHD_WEP_OPEN ) ? "WEP OPEN":
           ( security == MHD_WEP_SHARED ) ? "WEP_SHARED":
           ( security == MHD_WPA_PSK_TKIP ) ? "WPA PSK TKIP":
           ( security == MHD_WPA_PSK_MIXED ) ? "WPA PSK MIXED":
           ( security == MHD_WPA2_PSK_TKIP ) ? "WPA2 PSK TKIP":
           ( security == MHD_WPA2_PSK_MIXED ) ? "WPA2 PSK MIXED":
           ( security == MHD_IBSS_OPEN ) ? "ADHOC" :
           ( security == MHD_WPS_OPEN ) ? "ADHOC" :
           ( security == MHD_WPS_AES ) ? "WPS AES" :
           ( security == MHD_WPA_ENT_TKIP ) ? "WPA ENT TKIP" :
           ( security == MHD_WPA_ENT_AES ) ? "WPA ENT AES" :
           ( security == MHD_WPA_ENT_MIXED ) ? "WPA ENT MIXED" :
           ( security == MHD_WPA2_ENT_AES ) ? "WPA2 ENT AES" :
           ( security == MHD_WPA2_ENT_TKIP ) ? "WPA2 ENT TKIP" :
           ( security == MHD_WPA2_ENT_MIXED ) ? "WPA2 ENT MIXED" :
           "UNKNOWN";
}


static void _WifiMgr_Sta_Scan_Result_Print( mhd_ap_info_t *results, int num )
{
    int k;
    mhd_ap_info_t *record;
    char ssid[WIFI_SSID_MAXLEN+1];

    record = &results[0];
    for ( k = 0; k < num; k++, record++ )
    {
        /* Print SSID */
        printf("\n[%03d]\n", k+1 );
		memset(ssid, 0, sizeof(ssid));
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
		memcpy(ssid, record->ssid.value, record->ssid.length);
#else
		memcpy(ssid, record->ssid, sizeof(record->ssid));
#endif
        printf("       SSID          : %s \n", ssid );

        /* Print other network characteristics */
        printf("       BSSID         : %02X:%02X:%02X:%02X:%02X:%02X\n",
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
                                                  (uint8_t)record->bssid.octet[0], (uint8_t)record->bssid.octet[1], (uint8_t)record->bssid.octet[2],
                                                  (uint8_t)record->bssid.octet[3], (uint8_t)record->bssid.octet[4], (uint8_t)record->bssid.octet[5] );
#else
                                                  (uint8_t)record->bssid[0], (uint8_t)record->bssid[1], (uint8_t)record->bssid[2],
                                                  (uint8_t)record->bssid[3], (uint8_t)record->bssid[4], (uint8_t)record->bssid[5] );
#endif
        printf("       RSSI          : %ddBm\n", (int)record->rssi );
		printf("       Security value: %d\n", record->security);
        printf("       Security      : %s\n", _WifiMgr_Sta_Get_Security_String_By_Index(record->security));
        printf("       Channel       : %d\n", (int)record->channel );
    }
}



static void
_WifiMgr_Create_Worker_Thread(pthread_t task, void *(*start_routine)(void *), void *arg)
{
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
    pthread_create(&task, &attr, start_routine, arg);
}

void _WifiMgr_Sta_Link_Up_Cb(void)
{
    printf("_WifiMgr_Sta_Link_Up_Cb \n");
    return;
}

void _WifiMgr_Sta_Link_Down_Cb(void)
{
    int ret;
    char *ssid, *password;
    int retry_times;
    unsigned long      security_type;
    int nChannel;
	uint32_t nIp, gateway, netmask;

    ssid            = gWifiMgrVar.Ssid;
    password        = gWifiMgrVar.Password;
    security_type = 0;
    nChannel = 0;
    retry_times = 9;
    ret =0 ;
    gReConnecting = 0;

	if((ret = mhd_sta_disconnect( 1 )) != 0)
		return ;
    gReConnecting = 1;
    printf("--------------- _WifiMgr_Sta_Link_Down_Cb  ------------------------- %d %d \n",gReConnecting,gDisableAutoReconnect);
	mhd_sta_network_down();
    if (gDisableAutoReconnect==0) {
        printf("_WifiMgr_Sta_Link_Down_Cb begin %d retry \n",retry_times);

        //try to connect
        do {
            ret = mhd_sta_connect(ssid, NULL, security_type, password, nChannel );
            if (gReConnecting==0){
                printf("_WifiMgr_Sta_Link_Down_Cb disable reconnect \n");
            }

            if(ret !=0)
                sleep(3);

            retry_times--;

        } while (ret !=0 && (retry_times > 0 ) && gReConnecting==1);
    }
    if(ret==0 && gDisableAutoReconnect==0){
        mhd_sta_network_up(0, 0, 0);
        nIp = htonl(mhd_sta_ipv4_ipaddr());
        printf("Ip addr: %u.%u.%u.%u\n", FORMAT_IPADDR(nIp));
        netmask = htonl(mhd_sta_ipv4_netmask());
        printf("netmask: %u.%u.%u.%u\n", FORMAT_IPADDR(netmask));
        gateway = htonl(mhd_sta_ipv4_gateway());
        printf("gateway: %u.%u.%u.%u\n", FORMAT_IPADDR(gateway));

    }
    if (gDisableAutoReconnect==0) {

        if(ret !=0){
            mhd_sta_disconnect( 1 );
            mhd_sta_network_down();
            if (gWifiMgrSetting.wifiCallback)
                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S);

            printf("_WifiMgr_Sta_Link_Down_Cb end %d \n",ret);
        }
    } else {
        mhd_sta_disconnect( 1 );
        mhd_sta_network_down();
        printf("_WifiMgr_Sta_Link_Down_Cb end %d \n",ret);

    }
    gReConnecting = 0;
}



// sta connect to ap
static int
_WifiMgr_Sta_Connect_Process(void)
{
    ITPEthernetSetting setting;
    unsigned long tick1 = xTaskGetTickCount();
    unsigned long tick3;

    int nRet = WIFIMGR_ECODE_OK, cRet = 0;
    int dhcp_available = 0;
    unsigned long connect_cnt = 0, retry_connect_cnt = 0;
    char *ssid, *password;
    WIFI_MGR_SCANAP_LIST pList[64];

    unsigned long      security_type;
    int nChannel;
	uint32_t nIp, gateway, netmask;


    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    ssid            = gWifiMgrVar.Ssid;
    password        = gWifiMgrVar.Password;
//    security_type   = gWifiMgrVar.SecurityMode;
#if  defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
    security_type = 2;
#else

#endif

#ifdef CFG_NET_WIFI_MP_MODE
    if (gWifiMgrVar.MP_Mode) {
        printf("[WIFIMGR] Is MP mode, connect to default SSID.\r\n");
        // SSID
        snprintf(ssid,                  32, "%s", CFG_NET_WIFI_MP_SSID);
        // Password
        snprintf(password, 64, "%s", CFG_NET_WIFI_MP_PASSWORD);
    }
#endif
       // change connect mechanism, not scan first
	printf("mhd_sta_connect %s %s %lu \n",ssid,password,security_type);
#if  defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)

#else
	security_type = 0;
#endif
	nChannel = 0;
    //CMD: api 0 [SSID] [Password]
    cRet = mhd_sta_connect(ssid, NULL, security_type, password, nChannel );
    wifi_set_mode(0);

    printf("[%s] mhd_sta_connect results: %d #line %d \n", __FUNCTION__, cRet ,__LINE__);
    if (cRet == 1006){
        printf("[%s] 1006 , Reconnect #line %d \n", __FUNCTION__,__LINE__);
        mhd_sta_disconnect(1);
        mhd_sta_network_down();
        cRet = mhd_sta_connect(ssid, NULL, security_type, password, nChannel );
    }

    if (cRet != 0 ) {


        printf("[%s] wifi connect retry failed, quit this SSID(%s)\n", __FUNCTION__, ssid);
        nRet = WIFIMGR_ECODE_CONNECT_ERROR;
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);

        goto end;
    }


    /* Connect OK!! Reset count and go to DHCP phase */
    retry_connect_cnt = 0;

    // dhcp
    setting.dhcp        = gWifiMgrSetting.setting.dhcp;

    // autoip
    setting.autoip      = gWifiMgrSetting.setting.autoip;

    // ipaddr
    setting.ipaddr[0]   = gWifiMgrSetting.setting.ipaddr[0];
    setting.ipaddr[1]   = gWifiMgrSetting.setting.ipaddr[1];
    setting.ipaddr[2]   = gWifiMgrSetting.setting.ipaddr[2];
    setting.ipaddr[3]   = gWifiMgrSetting.setting.ipaddr[3];

    // netmask
    setting.netmask[0]  = gWifiMgrSetting.setting.netmask[0];
    setting.netmask[1]  = gWifiMgrSetting.setting.netmask[1];
    setting.netmask[2]  = gWifiMgrSetting.setting.netmask[2];
    setting.netmask[3]  = gWifiMgrSetting.setting.netmask[3];

    // gateway
    setting.gw[0]       = gWifiMgrSetting.setting.gw[0];
    setting.gw[1]       = gWifiMgrSetting.setting.gw[1];
    setting.gw[2]       = gWifiMgrSetting.setting.gw[2];
    setting.gw[3]       = gWifiMgrSetting.setting.gw[3];

    printf("[WIFIMGR] ssid     = %s\r\n", ssid);
    printf("[WIFIMGR] password = %s\r\n", password);
    printf("[WIFIMGR] security_type = %s \r\n",_WifiMgr_Sta_Get_Security_String_By_Index(security_type));

    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    if (strlen(ssid) == 0)
    {
        printf("[WIFIMGR]%s() L#%d: Error! Wifi setting has no SSID\r\n", __FUNCTION__, __LINE__);
        nRet = WIFIMGR_ECODE_NO_SSID;
        goto end;
    }

#if defined(CFG_NET_ETHERNET) && defined(CFG_NET_WIFI)
    printf("[WIFIMGR] check wifi netif %d \n",ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_STATUS, NULL));
    // Check if the wifi netif is exist
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_STATUS, NULL) == 0) {
        printf("[WIFIMGR]%s() L#%d: wifi need to add netif !\r\n", __FUNCTION__, __LINE__);
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, NULL);
    }
#endif

    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    // Wait for connecting...
    printf("[WIFIMGR] Wait for connecting\n");
    if (setting.dhcp) {
//        ip_addr_set_zero(&xnetif[0].ip_addr);
        // Wait for DHCP setting...
        printf("[WIFIMGR] Wait for DHCP setting");

//        dhcp_start(&xnetif[0]);
        tick3 = xTaskGetTickCount();
//        unsigned char *ip = LwIP_GetIP(&xnetif[0]);
        printf("\r\n\nIP set zero.\nGot IP after %lu ms.\n", (tick3-tick1));

        mhd_sta_network_up(0, 0, 0);
        connect_cnt = WIFI_CONNECT_DHCP_COUNT;
        while (connect_cnt)
        {
            nIp = htonl(mhd_sta_ipv4_ipaddr());
            if (nIp!=0)
            {
                printf("\r\n[WIFIMGR] DHCP setting OK\r\n");
                dhcp_available = 1;
                gWifiMgrVar.Pre_Scan = false;
                break;
            }
            putchar('.');
            fflush(stdout);
            connect_cnt--;
            if (connect_cnt == 0)
            {
                printf("\r\n[WIFIMGR]%s() L#%d: DHCP timeout! connect fail!\r\n", __FUNCTION__, __LINE__);
                nRet = WIFIMGR_ECODE_DHCP_ERROR;
                goto end;
            }

            if (gWifiMgrVar.Cancel_Connect || gWifiMgrVar.WIFI_Terminate)
            {
                goto end;
            }

            usleep(100000);


        }
        nIp = htonl(mhd_sta_ipv4_ipaddr());
        printf("Ip addr: %u.%u.%u.%u\n", FORMAT_IPADDR(nIp));
        netmask = htonl(mhd_sta_ipv4_netmask());
        printf("netmask: %u.%u.%u.%u\n", FORMAT_IPADDR(netmask));
        gateway = htonl(mhd_sta_ipv4_gateway());
        printf("gateway: %u.%u.%u.%u\n", FORMAT_IPADDR(gateway));


        if (gWifiMgrVar.Cancel_Connect)
        {
            goto end;
        }
    }
    else
    {
        printf("[WIFIMGR] Manual setting IP\n");
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);
        dhcp_available = 1;
        ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_GET_INFO, NULL);
    }
    printf("mhd_sta_register_link_callback \n");

    if (dhcp_available)
    {
        //usleep(1000*1000*5); //workaround random miss frames issue for cisco router

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);



        printf("wifi_set_autoreconnect \n");
//        wifi_set_autoreconnect(1);

        //while (!ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_IS_AVAIL, NULL)){
        //    usleep(100*1000);
        //}

        //ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_GET_INFO, NULL);
    }

    // start dhcp count
    gettimeofday(&tvDHCP1, NULL);


end:
    if (gWifiMgrVar.Cancel_Connect)
    {
        printf("[WIFIMGR]%s() L#%d: End. Cancel_Connect is set.\r\n", __FUNCTION__, __LINE__);

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_CANCEL);
    }

	/* Sava last time connect info otherwise WIFI can't reconnect for wake up */
    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_SAVE_INFO);

    {
        if (WifiMgr_Get_Sta_Connect_Complete() > 0)
            MGR_INFO(">>>>>>>>>> [Now in STA(Connect done)] Switch Mode Stop: post\n");
        else
            MGR_INFO(">>>>>>>>>> [Now in STA(Connect cancel)] Switch Mode Stop: post\n");

        sem_post(&semSwitchModeStop); /* SW Done post point_1 */
    }

    return nRet;
}


static int
_WifiMgr_Sta_Connect_Post(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready)
        return WIFIMGR_ECODE_NOT_INIT;

    pthread_mutex_lock(&mutexALWiFi);
    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        gWifiMgrVar.Need_Set = false;
        sem_post(&semConnectStart);
    }
    pthread_mutex_unlock(&mutexALWiFi);

    return nRet;
}


static inline int
_WifiMgr_Sta_Compare_AP_MAC(int list_1, int list_2)
{
    if (gWifiMgrApList[list_1].apMacAddr != NULL && gWifiMgrApList[list_2].apMacAddr != NULL){
        return (gWifiMgrApList[list_1].apMacAddr[0] == gWifiMgrApList[list_2].apMacAddr[0]) && \
                       (gWifiMgrApList[list_1].apMacAddr[1] == gWifiMgrApList[list_2].apMacAddr[1])&&\
                       (gWifiMgrApList[list_1].apMacAddr[2] == gWifiMgrApList[list_2].apMacAddr[2])&&\
                       (gWifiMgrApList[list_1].apMacAddr[3] == gWifiMgrApList[list_2].apMacAddr[3])&&\
                       (gWifiMgrApList[list_1].apMacAddr[4] == gWifiMgrApList[list_2].apMacAddr[4])&&\
                       (gWifiMgrApList[list_1].apMacAddr[5] == gWifiMgrApList[list_2].apMacAddr[5]);
    } else {
        return -1;
    }
}


static inline void
_WifiMgr_Sta_Entry_Info(const char* ssid, const char* password, const uint32_t secumode)
{

    if (ssid){
        // SSID
        if(strlen(ssid)<WIFI_SSID_MAXLEN+1){
            strcpy(gWifiMgrVar.Ssid,ssid);
        } else {
            printf("[Wifi mgr]_WifiMgr_Sta_Entry_Info can't copy ssid %d \n",__LINE__);
        }
    }

    if (password){
        // Password
        if(strlen(password)<WIFI_PASSWORD_MAXLEN){
            strcpy(gWifiMgrVar.Password,password);
        } else {
            printf("[Wifi mgr]_WifiMgr_Sta_Entry_Info can't copy password %d \n",__LINE__);
        }

    }

    if (secumode){
        // Security mode
        gWifiMgrVar.SecurityMode = secumode;
    }
}


static void
_WifiMgr_Sta_List_Swap(int x, int y)
{
    WIFI_MGR_SCANAP_LIST temp;

    memcpy(&temp,&gWifiMgrApList[x],sizeof(WIFI_MGR_SCANAP_LIST));
    memcpy(&gWifiMgrApList[x],&gWifiMgrApList[y],sizeof(WIFI_MGR_SCANAP_LIST));
    memcpy(&gWifiMgrApList[y],&temp,sizeof(WIFI_MGR_SCANAP_LIST));
}


static void
_WifiMgr_Sta_List_Sort_Insert(int size)
{
    int i,j;
    for(i = 0; i < size; i++){
        for(j = i; j > 0; j--){
            if(gWifiMgrApList[j].rfQualityRSSI > gWifiMgrApList[j - 1].rfQualityRSSI){
                _WifiMgr_Sta_List_Swap(j, j-1);
            }
        }
    }
}


static int
_WifiMgr_Sta_Remove_Same_SSID(int size)
{
    int i, j;
    WIFI_MGR_SCANAP_LIST gWifiMgrTempApList[WIFI_SCAN_LIST_NUMBER] = {0};

    if (size < 1){
        return size;
    }

    for (i=size-1 ; i>0 ; i--){
        for (j = i ; j >=0 ; j --){
            if (strcmp(gWifiMgrApList[i].ssidName , gWifiMgrApList[j].ssidName)==0 && i!=j){
                //set power =0 , if the same ssid
#if WIFIMGR_REMOVE_ALL_SAME_SSID
                if (gWifiMgrApList[i].rfQualityQuant < gWifiMgrApList[j].rfQualityQuant ? 1:0)
                    gWifiMgrApList[i].rfQualityQuant = 0;
                else
                    gWifiMgrApList[j].rfQualityQuant = 0;
#else
                if (_WifiMgr_Sta_Compare_AP_MAC(i, j))
                    gWifiMgrApList[i].rfQualityQuant = 0;
#endif
            }
        }
    }

    for (i = 0 , j =0 ; i < size ; i ++){
        if (gWifiMgrApList[i].rfQualityQuant > 0){
            memcpy(&gWifiMgrTempApList[j], &gWifiMgrApList[i], sizeof(WIFI_MGR_SCANAP_LIST));
            j++;
        }
    }

#if 0
    printf("RemoveSameSsid %d \n",j);
    for (i = 0; i < j; i++)
    {
        printf("[Wifi mgr] ssid = %32s, securityOn = %ld, securityMode = %ld, avgQuant = %d, avgRSSI = %d , <%02x:%02x:%02x:%02x:%02x:%02x>\r\n", gWifiMgrTempApList[i].ssidName, gWifiMgrTempApList[i].securityOn, gWifiMgrTempApList[i].securityMode,gWifiMgrTempApList[i].rfQualityQuant, gWifiMgrTempApList[i].rfQualityRSSI,
        gWifiMgrTempApList[i].apMacAddr[0], gWifiMgrTempApList[i].apMacAddr[1], gWifiMgrTempApList[i].apMacAddr[2], gWifiMgrTempApList[i].apMacAddr[3], gWifiMgrTempApList[i].apMacAddr[4], gWifiMgrTempApList[i].apMacAddr[5]);
    }
    printf("RemoveSameSsid -----\n\n");
#endif
    memset(gWifiMgrApList, 0,                  sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
    memcpy(gWifiMgrApList, gWifiMgrTempApList, sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
    return j;
}


static int
_WifiMgr_Sta_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    BOOLEAN_SET_TRUE(gWifiMgrVar.Cancel_WPS);

    //In order to terminate [_WifiMgr_Sta_Thread]
    sem_post(&semConnectStart);

    //Avoid terminate [_WifiMgr_Sta_Thread] not well
    usleep(1000);

    if (gWifiMgrVar.Cancel_Connect == false)
        WifiMgr_Sta_Cancel_Connect();

    //Clean connection info in Client mode
    //if (gWifiMgrSetting.wifiCallback && (gWifiMgrVar.Client_On_Off == WIFIMGR_SWITCH_OFF))
    //    gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_CLEAN_INFO);

    return nRet;
}



static int
_WifiMgr_Sta_Scan_Process(struct net_device_info *apInfo)
{
    int scan_result = 0;
    //int nWifiState = 0;
    int i = 0;
    int nHideSsid = 0;
    mhd_ap_info_t results[WIFI_SCAN_LIST_NUMBER];
    int num = sizeof(results)/sizeof(mhd_ap_info_t);
#if WIFIMGR_SHOW_SCAN_LIST
    int nTemp ;
#endif

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        printf("scanWifiAp  !WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    memset(apInfo, 0, sizeof(struct net_device_info));

    printf("[Wifi mgr]%s() Start to SCAN AP ==========================\r\n", __FUNCTION__);

	//scan_result = wifi_scan_networks(_WifiMgr_Sta_Scan_Result_Handler, apInfo);
    mhd_start_scan();

    mhd_get_scan_results(results, &num);

    _WifiMgr_Sta_Scan_Result_Print(results, num);

    if(num > 0 ){
		gWifiMgrVar.Pre_Scan = true;
	}else{
        printf("\n\rERROR: wifi scan failed, result code(%d).\n", scan_result);
        return WIFIMGR_ECODE_NOT_INIT;
    }

    gWifiMgrVar.Start_Scan = true;
/*
    while (1)
    {
        nWifiState = (int)gWifiMgrVar.Start_Scan;
        //printf("[Presentation]%s() nWifiState=0x%X\r\n", __FUNCTION__, nWifiState);
        if (nWifiState == 0)
        {
            // scan finish
            printf("[Wifi mgr]%s() Scan AP Finish!\r\n", __FUNCTION__);
            break;
        }
        usleep(100 * 1000);
    }
*/
    printf("[Wifi mgr]%s() ScanApInfo.apCnt = %d #line %d \r\n", __FUNCTION__, num, __LINE__);

#if 0//WIFIMGR_SHOW_SCAN_LIST
    for (i = 0; i < apInfo->apCnt; i++)
    {
        printf("[Wifi mgr] ssid = %32s (%d), securityOn = %ld, securityMode = 0x%x , avgQuant = %d, avgRSSI = %d , <%02x:%02x:%02x:%02x:%02x:%02x>\r\n",
			apInfo->apList[i].ssidName, strlen(apInfo->apList[i].ssidName),apInfo->apList[i].securityOn, apInfo->apList[i].securityMode,apInfo->apList[i].rfQualityQuant, apInfo->apList[i].rfQualityRSSI,
        apInfo->apList[i].apMacAddr[0], apInfo->apList[i].apMacAddr[1], apInfo->apList[i].apMacAddr[2], apInfo->apList[i].apMacAddr[3], apInfo->apList[i].apMacAddr[4], apInfo->apList[i].apMacAddr[5]);
    }
#endif

    for (i = 0; i < num; i++)
    {
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
        unsigned int ssid_len = strlen(results[i].ssid.value);
#else
        unsigned int ssid_len = strlen(results[i].ssid);
#endif
       // printf(" Ssid %s len %d rssi %d \n",results[i].ssid,ssid_len,results[i].rssi);
        /* Avoid the SSID length is shorter than 32, and the RSSI is less than 0. */
        if (ssid_len > 0 && ssid_len < 33 && (int)results[i].rssi < 0)
        {
            gWifiMgrApList[i].channelId = results[i].channel;
           // gWifiMgrApList[i].operationMode = results[i].security;

           /* Signal Quality */
           if ((int)results[i].rssi <= -100)
               gWifiMgrApList[i].rfQualityQuant  = 0;
           else if((int)results[i].rssi >= -30)
               gWifiMgrApList[i].rfQualityQuant  = 100;
           else
               gWifiMgrApList[i].rfQualityQuant  = 150 + 1.67*((int)results[i].rssi);

            //gWifiMgrApList[i].rfQualityQuant = apInfo->apList[i].rfQualityQuant;
            gWifiMgrApList[i].rfQualityRSSI = results[i].rssi;
            gWifiMgrApList[i].securityMode = results[i].security;
            /* For security */
            //memcpy(gWifiMgrApList[i].apMacAddr, apInfo->apList[i].apMacAddr, 6);
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
            memcpy(gWifiMgrApList[i].ssidName,  results[i].ssid.value, results[i].ssid.length);
            memcpy(gWifiMgrApList[i].apMacAddr,  results[i].bssid.octet, 6);
#else
            memcpy(gWifiMgrApList[i].ssidName,  results[i].ssid, 32);
            memcpy(gWifiMgrApList[i].apMacAddr,  results[i].bssid, 6);
#endif
        } else {
            nHideSsid ++;
        }
    }

#if WIFIMGR_SHOW_SCAN_LIST
    nTemp = 0;
    nTemp = num - nHideSsid;

    _WifiMgr_Sta_List_Sort_Insert(nTemp);

    nTemp = _WifiMgr_Sta_Remove_Same_SSID(nTemp);

    for (i = 0; i < nTemp; i++)
    {
        printf("[Wifi mgr] SSID = %32s, securityMode =  %16s, avgQuant = %4d %%, power = %4d dBm , <%02X:%02X:%02X:%02X:%02X:%02X>\r\n",
			gWifiMgrApList[i].ssidName,
			//gWifiMgrApList[i].securityMode,
			_WifiMgr_Sta_Get_Security_String_By_Index(results[i].security),
			gWifiMgrApList[i].rfQualityQuant, gWifiMgrApList[i].rfQualityRSSI,
        	gWifiMgrApList[i].apMacAddr[0], gWifiMgrApList[i].apMacAddr[1], gWifiMgrApList[i].apMacAddr[2],
        	gWifiMgrApList[i].apMacAddr[3], gWifiMgrApList[i].apMacAddr[4], gWifiMgrApList[i].apMacAddr[5]);
    }
#endif

    printf("[Wifi mgr]%s() End to SCAN AP ============================\r\n", __FUNCTION__);
    return num;
}


static void*
_WifiMgr_Sta_Thread(void* arg)
{
    int nRet = WIFIMGR_ECODE_OK;

    if ( WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP)
        goto end;

    MGR_TRACE("====>\n");

    BOOLEAN_SET_TRUE(gWifiMgrVar.IS_WifiMgr_Sta_Thread);
    while (1)
    {
        sem_wait(&semConnectStart);
        MGR_INFO("_WifiMgr_Sta_Thread\n");

        if (gWifiMgrVar.WIFI_Terminate) {
            BOOLEAN_SET_FALSE(gWifiMgrVar.IS_WifiMgr_Sta_Thread);
            printf("[Wifi mgr]terminate _WifiMgr_Sta_Thread(0) \n");
            break;
        }

        if (gWifiMgrVar.Need_Set){
            wifi_conn_state = WIFIMGR_CONNSTATE_SETTING;
            printf("[WIFIMGR] START to Set!\r\n");
            wifi_conn_ecode = WIFIMGR_ECODE_OK;


            gWifiMgrVar.Need_Set = false;
            printf("[WIFIMGR] Set finish!\r\n");
        }
        usleep(1000);

		if (strcmp(gWifiMgrVar.Ssid, "") == 0){
			nRet = WIFIMGR_ECODE_NO_SSID;
		} else {
			nRet = WIFIMGR_ECODE_OK;
		}

        if (nRet == WIFIMGR_ECODE_OK) {
            wifi_conn_state = WIFIMGR_CONNSTATE_CONNECTING;

            BOOLEAN_SET_TRUE(gWifiMgrVar.Cancel_WPS);
            wifi_conn_ecode = _WifiMgr_Sta_Connect_Process();
            BOOLEAN_SET_FALSE(gWifiMgrVar.Cancel_WPS);
        }
        wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
        usleep(1000);
    }
end:
    MGR_ERROR("Detach [_WifiMgr_Sta_Thread] pthread due to it's %s\n",
        ( WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP) ? "useless in AP mode" : "terminated in STA mode");
    sem_destroy(&semConnectStop);
    sem_destroy(&semConnectStart);
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    BOOLEAN_SET_FALSE(gWifiMgrVar.IS_WifiMgr_Sta_Thread);
    return NULL;
}


static inline void
_WifiMgr_HostAP_Entry_Info(char* ssid, char* password)
{
    if (ssid){
        // SSID
        snprintf(gWifiMgrVar.ApSsid, WIFI_SSID_MAXLEN, ssid);
    }
    if (password){
        // Password
        snprintf(gWifiMgrVar.ApPassword, WIFI_PASSWORD_MAXLEN, password);
    }
}


static int
_WifiMgr_HostAP_Init(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    char softap_ssid[] = "iteAP";
    char softap_passwd[] = "12345678";
    char *softap_security = "2";        //0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
    char *softap_channel = "2";
    char *p;
    char const *argv[] = {"api", "6", softap_ssid, softap_passwd, softap_security, softap_channel};
    unsigned char security =2;

    //printf("_WifiMgr_HostAP_Init %s %s , %s %s , %d %d \n",gWifiMgrVar.ApSsid,gWifiMgrVar.ApPassword,softap_ssid,softap_passwd,strlen(gWifiMgrVar.ApPassword),strtol(argv[4], &p, 10));
    if (strlen(gWifiMgrVar.ApPassword) ==0)
        security=0;
    if (gWifiMgrVar.SoftAP_Hidden){
        printf("[Wifimgr]_WifiMgr_HostAP_Init mhd_softap_set_hidden %d \n",__LINE__);
        mhd_softap_set_hidden(1);
    } else {

    }

    //CMD: api 6 [SSID] [Password] [security] [Channel]
    // for test ......
    // mhd_softap_start(argv[2], argv[3], strtol(argv[4], &p, 10), strtol(argv[5], &p, 10));

#ifdef Carplay_Example
    // security wpa3 sae
    security = MHD_AP_WPA3_SAE;
    mhd_softap_add_custom_ie(MY_OUI, MY_OUI_TYPE, MY_IE_DATA, sizeof(MY_IE_DATA) - 1, MY_VENDOR_IE_BEACON);
#endif

    MGR_INFO("AP mode init: %s/%s\n", gWifiMgrVar.ApSsid, gWifiMgrVar.ApPassword);
#if defined(CFG_NET_WIFI_SDIO_NGPL_AP6256) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6203) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
    if (gSoftAP_5G==5){
       // set 5G channel
       printf("mhd_softap_start 5G\n");
       nRet = mhd_softap_start(gWifiMgrVar.ApSsid, gWifiMgrVar.ApPassword, security, 48);
    } else {
       nRet = mhd_softap_start(gWifiMgrVar.ApSsid, gWifiMgrVar.ApPassword, security, strtol(argv[5], &p, 10));
    }
#else
    nRet = mhd_softap_start(gWifiMgrVar.ApSsid, gWifiMgrVar.ApPassword, security, strtol(argv[5], &p, 10));
#endif
    MGR_INFO("AP mode done(%d)\n", nRet);
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, NULL);

    mhd_softap_start_dhcpd(0xc0a80101);
    wifi_set_mode(1);

    dhcps_init();

    if (nRet == WIFIMGR_ECODE_OK)
        BOOLEAN_SET_TRUE(gWifiMgrVar.SoftAP_Init_Ready);

    usleep(1000);

    return nRet;
}


static int
_WifiMgr_HostAP_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    dhcps_deinit();
    usleep(1000*10);

    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_DISABLE, NULL);
    usleep(1000*1000);

    return nRet;
}

#if WIFIMGR_EXCUTE_WPS
static int
_WifiMgr_WPS_Connect_Post(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexALWiFi);
    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        gWifiMgrVar.Need_Set = true;
        sem_post(&semWPSStart);
//        sem_post(&semConnectStart);
    }
    pthread_mutex_unlock(&mutexALWiFi);

    return nRet;
}


static int
_WifiMgr_WPS_Init(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    struct net_device_config netCfg = {0};
    unsigned long connect_cnt = 0;
    int is_connected = 0, dhcp_available = 0;
    ITPWifiInfo wifiInfo;
    ITPEthernetSetting setting;

    struct net_device_config wpsNetCfg = {0};
    int len = 0;
    char ssid[WIFI_SSID_MAXLEN];
    char password[WIFI_PASSWORD_MAXLEN];

    if (gWifiMgrVar.Cancel_WPS) {
        goto end;
    }

    netCfg.operationMode = WLAN_MODE_STA;
    memset(netCfg.ssidName, 0, sizeof(netCfg.ssidName));
    netCfg.securitySuit.securityMode = WLAN_SEC_WPS;

    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL)) {

        usleep(1000*100);
        // dhcp
        setting.dhcp = 0;

        // autoip
        setting.autoip = 0;

        // ipaddr
        setting.ipaddr[0] =0;
        setting.ipaddr[1] = 0;
        setting.ipaddr[2] = 0;
        setting.ipaddr[3] = 0;

        // netmask
        setting.netmask[0] = 0;
        setting.netmask[1] = 0;
        setting.netmask[2] = 0;
        setting.netmask[3] = 0;

        // gateway
        setting.gw[0] = 0;
        setting.gw[1] = 0;
        setting.gw[2] = 0;
        setting.gw[3] = 0;

        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);

    }


    if (gWifiMgrVar.Cancel_WPS)
    {
        goto end;
    }

    // Wait for connecting...
    printf("[WIFIMGR WPS] Wait for connecting");
    connect_cnt = WIFI_CONNECT_COUNT;
    while (connect_cnt)
    {
        putchar('.');
        fflush(stdout);
        connect_cnt--;
        if (connect_cnt == 0) {
            printf("\r\n[WIFIMGR WPS]%s() L#%ld: Timeout! Cannot connect to WIFI AP!\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if (gWifiMgrVar.Cancel_WPS) {
            goto end;
        }

        usleep(100000);
    }

    if (!is_connected) {
        printf("[WIFIMGR WPS]%s() L#%ld: Error! Cannot connect to WiFi AP!\r\n", __FUNCTION__, __LINE__);
        nRet = WIFIMGR_ECODE_CONNECT_ERROR;
        goto end;
    }

    if (gWifiMgrVar.Cancel_WPS) {
        goto end;
    }

    // Wait for DHCP setting...
    printf("[WIFIMGR WPS] Wait for DHCP setting");
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_START_DHCP, NULL);
    connect_cnt = WIFI_CONNECT_COUNT;
    while (connect_cnt)
    {
        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_AVAIL, NULL)) {
            printf("\r\n[WIFIMGR WPS] DHCP setting OK\r\n");
            dhcp_available = 1;
            break;
        }
        putchar('.');
        fflush(stdout);
        connect_cnt--;
        if (connect_cnt == 0) {
            printf("\r\n[WIFIMGR WPS]%s() L#%ld: DHCP timeout! connect fail!\r\n", __FUNCTION__, __LINE__);
            nRet = WIFIMGR_ECODE_DHCP_ERROR;
            goto end;
        }

        if (gWifiMgrVar.Cancel_WPS) {
            goto end;
        }
        usleep(100000);
    }

    if (dhcp_available)
    {
        // trim the " char
        memset(ssid, 0, WIFI_SSID_MAXLEN);
        len = strlen(wpsNetCfg.ssidName);
        memcpy(ssid, wpsNetCfg.ssidName + 1, len - 2);
        memset(password, 0, WIFI_PASSWORD_MAXLEN);
        len = strlen(wpsNetCfg.securitySuit.preShareKey);
        memcpy(password, wpsNetCfg.securitySuit.preShareKey + 1, len - 2);

        printf("[WIFIMGR WPS] WPS Info:\r\n");
        printf("[WIFIMGR WPS] WPS SSID     = %s\r\n", ssid);
        printf("[WIFIMGR WPS] WPS Password = %s\r\n", password);
        printf("[WIFIMGR WPS] WPS Security = %ld\r\n", wpsNetCfg.securitySuit.securityMode);
    }

    end:

    if (gWifiMgrVar.Cancel_WPS)
    {
        printf("[WIFIMGR WPS]%s() L#%ld: End. gWifiMgrVar.Cancel_WPS is set.\r\n", __FUNCTION__, __LINE__);
    }

    return nRet;
}


static void*
_WifiMgr_WPS_Thread(void* arg)
{
    int nRet = WIFIMGR_ECODE_OK;

    while (1)
    {
        sem_wait(&semWPSStart);
        if (gWifiMgrVar.WIFI_Terminate) {
            printf("[Wifi mgr]terminate WifiMgr_WPS_ThreadFunc \n");
            break;
        }

        printf("[WIFIMGR] START to Connect WPS!\r\n");
        wifi_conn_ecode = _WifiMgr_WPS_Init();
        printf("[WIFIMGR] Connect WPS finish!\r\n");


        usleep(1000);
    }

    return NULL;
}
#endif


static WIFIMGR_CONNSTATE_E
_WifiMgr_Main_Process_Station(WIFIMGR_CONNSTATE_E state)
{
    int nRet = 0;
    int nWiFiConnState = 0, nWiFiConnEcode = 0;
    //long temp_disconn_time = 0;
    static struct timeval tv1 = {0, 0}, tv2 = {0, 0};
    static struct timeval tv3_temp = {0, 0}, tv4_temp = {0, 0};
    WIFIMGR_CONNSTATE_E gWifi_connstate = state;

    /* === Phase 1: Watch Connection Process State  ===*/
    if (gWifiMgrVar.Is_First_Connect) {
        // First time connect when the system start up
        nRet = _WifiMgr_Sta_Connect_Post();
        if (nRet == WIFIMGR_ECODE_OK) {
            gWifi_connstate = WIFIMGR_CONNSTATE_CONNECTING;
        }
        BOOLEAN_SET_FALSE(gWifiMgrVar.Is_First_Connect);

        goto end;
    }

    if (gWifi_connstate == WIFIMGR_CONNSTATE_SETTING ||
        gWifi_connstate == WIFIMGR_CONNSTATE_CONNECTING)
    {
        nRet = WifiMgr_Get_Connect_State(&nWiFiConnState, &nWiFiConnEcode);
        if (nWiFiConnState == WIFIMGR_CONNSTATE_STOP) {
            gWifi_connstate = WIFIMGR_CONNSTATE_STOP;
            // the connecting was finish
            if (nWiFiConnEcode == WIFIMGR_ECODE_OK) {
                if (!WifiMgr_Get_Sta_Connect_Complete()) {
                    // fail, restart the timer
                    gettimeofday(&tv1, NULL);
                }
            } else {
                MGR_ERROR("nWiFiConnEcode = 0%d\r\n", nWiFiConnEcode);

                // connection has error
                if (nWiFiConnEcode == WIFIMGR_ECODE_NO_INI_FILE) {
                    BOOLEAN_SET_TRUE(gWifiMgrVar.No_Config_File);
                }
                if (nWiFiConnEcode == WIFIMGR_ECODE_NO_SSID) {
                    BOOLEAN_SET_TRUE(gWifiMgrVar.No_SSID);
                } else {
                    // fail, restart the timer
                    gettimeofday(&tv1, NULL);
                }
            }
        }
        goto end;
    }

    /* === Phase 1.5: Watch Sleep State  ===*/
    nRet = WifiMgr_Get_Connect_State(&nWiFiConnState, &nWiFiConnEcode);
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) && gWifiMgrVar.Is_Temp_Disconnect)
        BOOLEAN_SET_FALSE(gWifiMgrVar.Is_Temp_Disconnect);


    /* === Phase 2: Watch Avaliable State  ===*/
    if (WifiMgr_Get_Sta_Avalible())
    {
        if (gWifiMgrVar.Is_Temp_Disconnect) {
            BOOLEAN_SET_FALSE(gWifiMgrVar.Is_Temp_Disconnect);     // reset
            MGR_TRACE("WiFi auto re-connected!\r\n");
            if (gWifiMgrSetting.wifiCallback)
                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_RECONNECTION);
        }

        if (!gWifiMgrVar.Is_WIFI_Available) {
            // prev is not available, curr is available
            BOOLEAN_SET_TRUE(gWifiMgrVar.Is_WIFI_Available);
            BOOLEAN_SET_FALSE(gWifiMgrVar.No_Config_File);
            BOOLEAN_SET_FALSE(gWifiMgrVar.No_SSID);
            MGR_TRACE("WiFi auto re-connected!\r\n");
        }

        gettimeofday(&tvDHCP2, NULL);
        if (itpTimevalDiff(&tvDHCP1, &tvDHCP2) > WIFIMGR_DHCP_RENEW_MSEC) {
            //MGR_INFO("====>Send DHCP Discover!!!!!\n");
            //MGR_INFO("DHCP renew %d \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2));
            //ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RENEW_DHCP, NULL);
            gettimeofday(&tvDHCP1, NULL);
            gettimeofday(&tvDHCP2, NULL);
        } else {
            //printf("DHCP wait renew  %d \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2));
        }
    } else {
        if (gWifiMgrVar.Is_WIFI_Available) {
            if (!gWifiMgrVar.Is_Temp_Disconnect && nWiFiConnEcode == WIFIMGR_ECODE_OK)
            {
              #if 0
                // first time detect
                BOOLEAN_SET_TRUE(gWifiMgrVar.Is_Temp_Disconnect);
                gettimeofday(&tv3_temp, NULL);
                MGR_TRACE("WiFi temporary disconnected!%d %d\r\n", nWiFiConnState,nWiFiConnEcode);
                if (gWifiMgrSetting.wifiCallback)
                    gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT);
             #endif
            } else if (nWiFiConnEcode == WIFIMGR_ECODE_OK) {
             #if 0
                gettimeofday(&tv4_temp, NULL);
                temp_disconn_time = itpTimevalDiff(&tv3_temp, &tv4_temp);
                MGR_TRACE("temp disconnect time = %ld sec. %d %d\r\n", temp_disconn_time / 1000 , nWiFiConnState,nWiFiConnEcode);
                if (temp_disconn_time >= WIFIMGR_TEMPDISCONN_MSEC) {
                    MGR_TRACE("WiFi temporary disconnected over %ld sec. Services should be shut down.\r\n", temp_disconn_time / 1000);
                    BOOLEAN_SET_FALSE(gWifiMgrVar.Is_Temp_Disconnect);     // reset

                    if (gWifiMgrSetting.wifiCallback)
                        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S);

                    // prev is available, curr is not available
                    BOOLEAN_SET_FALSE(gWifiMgrVar.Is_WIFI_Available);
                }
            #endif
            }
        } else {
            // prev is not available, curr is not available
            if (gWifiMgrVar.No_Config_File || gWifiMgrVar.No_SSID) {
                // has no data to connect, skip
                goto end;
            }

            nRet = WifiMgr_Get_Connect_State(&nWiFiConnState, &nWiFiConnEcode);
            switch (nWiFiConnState)
            {
                case WIFIMGR_CONNSTATE_STOP:
                    gettimeofday(&tv2, NULL);
                    if (itpTimevalDiff(&tv1, &tv2) >= WIFIMGR_RECONNECT_MSEC) {
                        //nRet = _WifiMgr_Sta_Connect_Post();
                        if (nRet == WIFIMGR_ECODE_OK) {
                            gWifi_connstate = WIFIMGR_CONNSTATE_CONNECTING;
                        }
                    }
                    break;
                case WIFIMGR_CONNSTATE_SETTING:
                    break;
                case WIFIMGR_CONNSTATE_CONNECTING:
                    break;
            }
        }
    }

    end:
        return gWifi_connstate;
    //nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
}


static WIFIMGR_CONNSTATE_E
_WifiMgr_Main_Process_HostAP(WIFIMGR_CONNSTATE_E state)
{
    WIFIMGR_CONNSTATE_E gWifi_connstate = state;

    if (!gWifiMgrVar.SoftAP_Init_Ready) {
        MGR_INFO("HostAP Init not Ready, start to init\r\n");
        gWifi_connstate = WIFIMGR_CONNSTATE_STOP;
    } else {
        /* Work around: Can not broadcast beacon while BT is working in AP mode. */
        int get_scan_count = 0;
        WIFI_MGR_SCANAP_LIST pList[64];

        get_scan_count = WifiMgr_Get_Scan_AP_Info(pList);
        MGR_INFO("Scan count(%d)\n", get_scan_count);

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);
    }

    usleep(1000);

    MGR_INFO(">>>>>>>>>> [Now in HostAP] Switch Mode Stop\n");
    sem_post(&semSwitchModeStop); /* SW Done post point_2 */

    return gWifi_connstate;
}


static void*
_WifiMgr_Main_Process_Thread(void *arg)
{
    int nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
	//int nCount =0;
	//uint32_t nIp, gateway, netmask;
	//int ret;

    WIFIMGR_CONNSTATE_E gWifi_connstate = WIFIMGR_CONNSTATE_STOP;


    BOOLEAN_SET_TRUE(gWifiMgrVar.Is_First_Connect);
    BOOLEAN_SET_TRUE(gWifiMgrVar.IS_WifiMgr_Main_Process_Thread);
    BOOLEAN_SET_FALSE(gWifiMgrVar.Is_Temp_Disconnect);
    BOOLEAN_SET_FALSE(gWifiMgrVar.No_Config_File);
    BOOLEAN_SET_FALSE(gWifiMgrVar.No_SSID);

    //usleep(1000);
    MGR_TRACE("\n");

    while (1)
    {
        nCheckCnt--;
        if (gWifiMgrVar.WIFI_Terminate) {
            MGR_INFO("terminate WifiMgr_Process_ThreadFunc \n");
            BOOLEAN_SET_FALSE(gWifiMgrVar.IS_WifiMgr_Main_Process_Thread);
            break;
        }
#if 0
        nCount++;
        if (nCount==10000){

            printf("get info RSSI %d\n",mhd_sta_get_rssi());
            ret = mhd_sta_get_connection();
                if ( ret == 1)
                    printf(("WiFi sta connected!\n"));
                else if (ret == 0)
                    printf(("WiFi sta disconnected!\n"));
                else
                    printf(("Failed to get WiFi sta connection!\n"));

            nIp = htonl(mhd_sta_ipv4_ipaddr());
            printf("Ip addr: %u.%u.%u.%u\n", FORMAT_IPADDR(nIp));
            netmask = htonl(mhd_sta_ipv4_netmask());
            printf("netmask: %u.%u.%u.%u\n", FORMAT_IPADDR(netmask));
            gateway = htonl(mhd_sta_ipv4_gateway());
            printf("gateway: %u.%u.%u.%u\n", FORMAT_IPADDR(gateway));
            nCount =0;
            //printf("WifiMgr_Get_HostAP_Device_Number %d \n",WifiMgr_Get_HostAP_Device_Number());
        }
#endif
        if (nCheckCnt == 0) {
            switch (WifiMgr_Get_WIFI_Mode()) {
                case WIFIMGR_MODE_CLIENT:
                    /* === Client Mode === */
                    gWifi_connstate = _WifiMgr_Main_Process_Station(gWifi_connstate);
                    nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
                    break;

                case WIFIMGR_MODE_SOFTAP:
                    /* === AP Mode === */
                    gWifi_connstate = _WifiMgr_Main_Process_HostAP(gWifi_connstate);
                    break;

                case WIFIMGR_MODE_P2P:
                    /* === P2P Mode === */

                    break;


                case WIFIMGR_MODE_MAX:
                    /* === 1. AP Mode === */
                    gWifi_connstate = _WifiMgr_Main_Process_HostAP(gWifi_connstate);

                    /* === 2. Client Mode === */
                    gWifi_connstate = _WifiMgr_Main_Process_Station(gWifi_connstate);
                    nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
                    break;

                default:
                    MGR_ERROR("Unknow mode\n");
            }
        }

        usleep(1000);
    }

    MGR_ERROR("Detach [_WifiMgr_Main_Process_Thread] pthread\n");
    return NULL;
}


static int gCountT = 0;
static WIFI_MGR_SETTING gWifiSetting;
static  struct timeval gSwitchStart, gSwitchEnd;
static  struct timeval gSwitchCount;

int
_WifiMgr_Sta_HostAP_Switch_Calculate_Time(void)
{
    if (itpTimevalDiff(&gSwitchCount,&gSwitchEnd)> 1000){
        printf(" %d , %d  \n",itpTimevalDiff(&gSwitchCount,&gSwitchEnd)+(gCountT*1000),itpTimevalDiff(&gSwitchStart,&gSwitchEnd));
        gCountT++;
        gettimeofday(&gSwitchCount, NULL);
    }
	return 0;
}


static void
_WifiMgr_Switch_Mode_Init(void)
{
    int nRet;

    /* Set Semaphore */
    nRet = sem_init(&semSwitchModeStart, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semSwitchModeStart sem_init() fail!\r\n");
        //nRet = WIFIMGR_ECODE_SEM_INIT;
        return;
    }

    nRet = sem_init(&semSwitchModeStop, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semSwitchModeStop sem_init() fail!\r\n");
        //nRet = WIFIMGR_ECODE_SEM_INIT;
        return;
    }

    BOOLEAN_SET_TRUE(gWifiMgrVar.IS_WifiMgr_Switch_Mode_Thread);
}


static unsigned int SW_CNT = 0;
static void*
_WifiMgr_Sta_HostAP_Switch_Thread(void *arg)
{
    int nTemp;

    _WifiMgr_Switch_Mode_Init();

    for (;;) {

        sem_wait(&semSwitchModeStart);

        /* ==== Uninstall APP/Driver/IO ==== */
        if (!gWifiMgrVar.WIFI_Init_Ready) {
            //return WIFIMGR_ECODE_NOT_INIT;
            return NULL;
        }

        gWifiMgrVar.WIFI_Mode = WifiMgr_Get_WIFI_Mode();
        MGR_INFO("================ [%u] SW START (%s Mode Now) ================>\n",
            SW_CNT++, (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_CLIENT) ? "Client":"AP");

        WifiMgr_Terminate();
        sleep(1);


        switch (WifiMgr_Get_WIFI_Mode())
        {
            case WIFIMGR_MODE_CLIENT:
                /* === STA  -> AP === */
                MGR_TRACE("Terminate STA done, prepare to resume HostAP...\n");
                wifi_switch();
                if (gSoftAP_5G==5)
                    nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 5, gWifiSetting);
                else
                    nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
                break;

            case WIFIMGR_MODE_SOFTAP:
                /* === AP -> STA === */
                MGR_TRACE("Terminate HostAP done, prepare to resume Client...\n");
                wifi_switch();
                nTemp = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
                break;

            case WIFIMGR_MODE_MAX:
                MGR_INFO("Not support concurrent mode\n");
                break;

            default:
                    MGR_ERROR("Unknow mode\n");

        }

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_SWITCH_CLIENT_SOFTAP_FINISH);

        /* Wait connection set done */
        sem_wait(&semSwitchModeStop); /* Switch Done need post */
        MGR_INFO("<================ SW DONE (%s Mode Now) ================\n\n\n",
            (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_CLIENT) ? "Client":"AP");
    }

    return NULL;
}



static int
_WifiMgr_Enable_PowerSave(void)
{
//    return mhd_sta_set_powersave(0,0);
	return 0;
}


static int
_WifiMgr_Disable_PowerSave(void)
{
    //return mhd_sta_set_powersave();;
	return 0;
}

uint32_t WifiMgr_Secu_ITE_To_MHD(const char* ite_security_enum)
{
    uint32_t security;

    /* Translate ITE WIFI security enum to MHD WIFI security enum*/
    if (strcmp(ite_security_enum, ITE_WIFI_SEC_OPEN) == 0)
        security = MHD_SECURE_OPEN;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_TKIP_PSK) == 0)
        security = MHD_WPA_PSK_TKIP;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_AES_PSK) == 0)
        security = MHD_WPA_PSK_AES;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_AES_PSK) == 0)
        security = MHD_WPA2_PSK_AES;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_TKIP_PSK) == 0)
        security = MHD_WPA2_PSK_TKIP;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_MIXED_PSK) == 0)
        security = MHD_WPA2_PSK_MIXED;
    else
        security = MHD_WPA2_PSK_MIXED;

    printf("WifiMgr_Secu_ITE_To_MHD: ITE(%s) -> MHD(%u)\n", ite_security_enum, security);

    return security;
}


char* WifiMgr_Secu_MHD_To_ITE(uint32_t security)
{
    static char ite_security_enum[3];

    /* Translate 8189ftv WIFI security enum to ITE WIFI security enum*/
    if (security == MHD_SECURE_OPEN)
        strcpy(ite_security_enum, "0");
    else if (security == MHD_WPA_PSK_TKIP)
        strcpy(ite_security_enum, "2");
    else if (security == MHD_WPA_PSK_AES)
        strcpy(ite_security_enum, "3");
    else if (security == MHD_WPA2_PSK_AES)
        strcpy(ite_security_enum, "4");
    else if (security == MHD_WPA2_PSK_TKIP)
        strcpy(ite_security_enum, "5");
    else if (security == MHD_WPA2_PSK_MIXED)
        strcpy(ite_security_enum, "6");
    else if (security == MHD_WPS_OPEN)
        strcpy(ite_security_enum, "8");
    else
        strcpy(ite_security_enum, "NA");

    printf("WifiMgr_Secu_MHD_To_ITE: MHD(%u) -> ITE(%s)\n", security, ite_security_enum);

    return ite_security_enum;
}


/* ================================================= */




/* ================================================= */
/*                                                   */
/* Static Functions                                  */
/*                                                   */
/* ================================================= */


int
WifiMgr_Get_Connect_State(int *conn_state, int *e_code)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        *conn_state = 0;
        *e_code = WIFIMGR_ECODE_NOT_INIT;
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexALWiFi);
    *conn_state = wifi_conn_state;
    *e_code = wifi_conn_ecode;
    pthread_mutex_unlock(&mutexALWiFi);

    return nRet;
}


int
WifiMgr_Get_MAC_Address(unsigned char cMac[6])
{
	mhd_mac_t mac;

//    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_GET_INFO, &wifiInfo);
    if (gWifiMgrVar.WIFI_Mode == WIFIMGR_MODE_CLIENT){
        mhd_sta_get_mac_address(&mac);
    } else if (gWifiMgrVar.WIFI_Mode == WIFIMGR_MODE_SOFTAP){
#if defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
    
#else
        mhd_softap_get_mac_address(&mac);
#endif
    }

    cMac[0] = (unsigned char)mac.octet[0];
    cMac[1] = (unsigned char)mac.octet[1];
    cMac[2] = (unsigned char)mac.octet[2];
    cMac[3] = (unsigned char)mac.octet[3];
    cMac[4] = (unsigned char)mac.octet[4];
    cMac[5] = (unsigned char)mac.octet[5];

    printf("WifiMgr_Get_MAC_Address %0x:%0x:%0x:%0x:%0x:%0x   \n",cMac[0],cMac[1],cMac[2],cMac[3],cMac[4],cMac[5]);

    return 0;
}


int
WifiMgr_Get_WIFI_Mode(void)
{
    return gWifiMgrVar.WIFI_Mode;
}


int
WifiMgr_Get_Scan_AP_Info(WIFI_MGR_SCANAP_LIST* pList)
{
    int nApCount;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        printf("WifiMgr_Get_Scan_AP_Info  !WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexMode);


    nApCount = _WifiMgr_Sta_Scan_Process(&gScanApInfo);
    memcpy(pList,gWifiMgrApList,sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);

    pthread_mutex_unlock(&mutexMode);

    printf("WifiMgr_Get_Scan_AP_Info %d  \n",nApCount);

    return nApCount;

}


int
WifiMgr_Get_HostAP_Ready(void)
{
//    printf("WifiMgr_Get_HostAP_Ready: %d \n", wifi_is_ready_to_transceive(RTW_AP_INTERFACE));
//    return wifi_is_ready_to_transceive(RTW_AP_INTERFACE);
	return 0;
}


int
WifiMgr_Get_HostAP_Device_Number(void)
{
    int ret;
    uint32_t count = 32;
    mhd_mac_t mac_list[8];


    ret = mhd_softap_get_mac_list( (mhd_mac_t *)mac_list, &count );
    if ( ret == 0 )
    {
        for (int i=0; i<count; i++)
        {
            printf("MAC[%d]: %02X:%02X:%02X:%02X:%02X:%02X\n", i,
                  mac_list[i].octet[0], mac_list[i].octet[1], mac_list[i].octet[2],
                  mac_list[i].octet[3], mac_list[i].octet[4], mac_list[i].octet[5]);
        }
    }

    printf("\n\rClient Num: %u", count);
    printf("\n\r");

    return count;
}


int
WifiMgr_Sta_Is_Available(int* is_available)
{
    uint32_t nIp=0;
    //char msg_wifi[16];

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        *is_available = 0;
        return 0;
    }

    nIp = htonl(mhd_sta_ipv4_ipaddr());


    //sprintf(msg_wifi, "%u.%u.%u.%u",FORMAT_IPADDR(nIp));

    //printf("WifiMgr_Sta_Is_Available %d %s %u.%u.%u.%u\n",nIp,msg_wifi,FORMAT_IPADDR(nIp));
    if (nIp)
        return 1;

    return WIFIMGR_ECODE_OK;
}

int
WifiMgr_Get_Sta_Avalible(void)
{
    uint32_t nIp=0;
    //char msg_wifi[16];


    nIp = htonl(mhd_sta_ipv4_ipaddr());


    if (nIp)
        return 1;

    return WIFIMGR_ECODE_OK;

}

int
WifiMgr_Get_Sta_Connect_Complete(void)
{
/*
      Confirm 3 conditions of connection :
        1. SSID len > 0
        2. SW stack ready (DHCP OK)
        3. Already hook IP into netif
*/
    uint32_t nIp=0;

    nIp = htonl(mhd_sta_ipv4_ipaddr());


    if (nIp)
        return 1;

    return WIFIMGR_ECODE_OK;

}



int
WifiMgr_Sta_Connect(const char* ssid, const char* password, const char* secumode)
{
    uint32_t security;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (gWifiMgrVar.WIFI_Mode != WIFIMGR_MODE_CLIENT){
        printf("[Wifi mgr] WifiMgr_Sta_Connect need client mode to connect %d  #line %d  \n",gWifiMgrVar.WIFI_Mode,__LINE__);
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (gWifiMgrVar.Cancel_Connect)
        return WIFIMGR_ECODE_CONNECT_ERROR;

    security = WifiMgr_Secu_ITE_To_MHD(secumode);

    _WifiMgr_Sta_Entry_Info(ssid, password, security);

    if (gReConnecting == 1){
        // disable reconnect
        printf("WifiMgr_Sta_Connect disable reconnect  \n");
        gReConnecting = 0;
        sleep(3);
    }
    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        gWifiMgrVar.Need_Set = false;
        sem_post(&semConnectStart);
    }
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);

    return WIFIMGR_ECODE_OK;
}


int
WifiMgr_Sta_Disconnect(void)
{

    //ITPEthernetSetting setting;

    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (wifi_conn_state == WIFIMGR_CONNSTATE_CONNECTING){
        WifiMgr_Sta_Cancel_Connect();
    }

	printf("WifiMgr_Sta_Disconnect gReConnecting %d \n",gReConnecting);
    if (gReConnecting == 1){
        gReConnecting = 0;
        printf("WifiMgr_Sta_Disconnect  wait cb function\n");
        sleep(3);
    }
    if((nRet = mhd_sta_disconnect( 1 )) != 0)
        return 1;
    mhd_sta_network_down();
#if 0
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL)) {

        //nRet = wifi_disconnect();

        usleep(1000*100);
        // dhcp
        setting.dhcp = 0;

        // autoip
        setting.autoip = 0;

        // ipaddr
        setting.ipaddr[0] = 0;
        setting.ipaddr[1] = 0;
        setting.ipaddr[2] = 0;
        setting.ipaddr[3] = 0;

        // netmask
        setting.netmask[0] = 0;
        setting.netmask[1] = 0;
        setting.netmask[2] = 0;
        setting.netmask[3] = 0;

        // gateway
        setting.gw[0] = 0;
        setting.gw[1] = 0;
        setting.gw[2] = 0;
        setting.gw[3] = 0;

        //ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);
    }
#endif

    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode = WIFIMGR_ECODE_SET_DISCONNECT;
    usleep(1000*100);
    WifiMgr_Sta_Not_Cancel_Connect();
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);

    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT);

    MGR_INFO("==== WifiMgr_Sta_Disconnect end ====\n");

    return nRet;
}

extern int mhd_sta_wowl_init( const char *pattern_string, int offset );
extern int mhd_sta_wowl_enable( void );
extern int mhd_sta_wowl_status( void );
extern int mhd_sta_wowl_disable( void );

static char *WOWL_PATTERN_STRING = "0x00000000";
static int WOWL_PATTERN_OFFSET = 10; //52;
static int gWOWLInit = 0;

void WifiMgr_Sta_Wowl_Disable(void)
{
    int nRet;
    nRet = mhd_sta_wowl_disable();
}


int
WifiMgr_Sta_Sleep_Disconnect(void)
{

    int nRet = WIFIMGR_ECODE_OK;
    uint32_t value = 0;
    

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }
#if defined(CFG_NET_WIFI_SDIO_MHD_WOWL)
    printf("WifiMgr_Sta_Sleep_Disconnect mhd_sta_wowl_enable \n");
    if (gWOWLInit==0){
        printf("WifiMgr_Sta_Sleep_Disconnect mhd_sta_wowl_init \n");
        mhd_sta_wowl_init(WOWL_PATTERN_STRING, WOWL_PATTERN_OFFSET );
    }
    
    mhd_sta_wowl_enable();
    gWOWLInit++;

    #ifdef CFG_POWER_SLEEP
    value = ithGpioGet(CFG_POWER_WAKEUP_GPIO_PIN);
    if (value)
    {
        printf("PIN_HIGH %d\n",CFG_POWER_WAKEUP_GPIO_PIN);
    } else {
        printf("PIN_LOW %d\n",CFG_POWER_WAKEUP_GPIO_PIN);
    }
    usleep(200*1000);
    #endif
#else    

    if((nRet = mhd_sta_disconnect( 1 )) != 0){
        printf("WifiMgr_Sta_Sleep_Disconnect error %d \n",nRet);
    }
    mhd_sta_network_down();

	printf("WifiMgr_Sta_Sleep_Disconnect %d \n",nRet);
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode = WIFIMGR_ECODE_SET_DISCONNECT;
    //usleep(1000*100);

#endif
    return nRet;
}


void
WifiMgr_Sta_Switch(int status)
{
	gWifiMgrVar.Client_On_Off = status;
}


void
WifiMgr_Sta_Cancel_Connect(void)
{
    gWifiMgrVar.Cancel_Connect = true;
}


void
WifiMgr_Sta_Not_Cancel_Connect(void)
{
    gWifiMgrVar.Cancel_Connect = false;
}


void
WifiMgr_HostAP_Set_Hidden(void)
{
    gWifiMgrVar.SoftAP_Hidden = true;
}

int WifiMgr_Set_APMode_5G(int nOnOff)
{
    if (nOnOff==0){
        gSoftAP_5G = 0;
    } else if (nOnOff == 1){
        gSoftAP_5G = 5;
    }
	return 0;
}

int
WifiMgr_Init(WIFIMGR_MODE_E init_mode, int mp_mode,WIFI_MGR_SETTING wifiSetting)
{
    int nRet = WIFIMGR_ECODE_OK;
    int i;

    itpRegisterDevice(ITP_DEVICE_WIFI, &itpDeviceWifi);
    itpRegisterDevice(ITP_DEVICE_SOCKET, &itpDeviceSocket);

    while(gWifiMgrVar.WIFI_Terminate){
         printf("WifiMgr not finished yet \n");
         usleep(200*1000);
    }

    wifi_conn_state         = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode         = WIFIMGR_ECODE_SET_DISCONNECT;
    gWifiMgrVar.Need_Set    = false;
    gWifiMgrVar.MP_Mode     = mp_mode;
    gWifiMgrVar.WIFI_Mode   = init_mode;

    gWifiMgrSetting.wifiCallback = wifiSetting.wifiCallback;


    if (init_mode == WIFIMGR_MODE_CLIENT){
        _WifiMgr_Sta_Entry_Info(wifiSetting.ssid, wifiSetting.password, wifiSetting.secumode);
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL);
    } else if (init_mode == WIFIMGR_MODE_SOFTAP){
        gSoftAP_5G = mp_mode;
        printf("[Wifimgr] WIFI start AP: SSID(%s)/PW(%s) mp_mode %d \n", wifiSetting.ap_ssid, wifiSetting.ap_password,mp_mode);
        _WifiMgr_HostAP_Entry_Info(wifiSetting.ap_ssid, wifiSetting.ap_password);
        _WifiMgr_HostAP_Init();
    } else if (init_mode == WIFIMGR_MODE_P2P){
        printf("WifiMgr_Init p2p \n");
        WifiMgr_P2P_Enable(1);
        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);

        
    }

    // default select dhcp
    gWifiMgrSetting.setting.dhcp = 1;
    if (wifiSetting.setting.ipaddr[0]>0){
        memcpy(&gWifiMgrSetting.setting,&wifiSetting.setting,sizeof(ITPEthernetSetting));
    }

    /* ====  Semaphore initialization ==== */
    nRet = sem_init(&semConnectStart, 0, 0);
    if (nRet == -1) {
        printf("[WIFIMGR] ERROR, semConnectStart sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }

    nRet = sem_init(&semConnectStop, 0, 0);
    if (nRet == -1) {
        printf("[WIFIMGR] ERROR, semConnectStop sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }

#if WIFIMGR_EXCUTE_WPS
    nRet = sem_init(&semWPSStart, 0, 0);
    if (nRet == -1) {
        printf("[WIFIMGR] ERROR, semWPSStart sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }

    nRet = sem_init(&semWPSStop, 0, 0);
    if (nRet == -1) {
        printf("[WIFIMGR] ERROR, semWPSStop sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }
#endif

    /* ====  Mutually Exclusive initialization ==== */
    nRet = pthread_mutex_init(&mutexALWiFi, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexALWiFi pthread_mutex_init() fail! nRet = %d\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexIni, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexIni pthread_mutex_init() fail! nRet = %d\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexMode, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexMode pthread_mutex_init() fail! nRet = %d\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

#if WIFIMGR_EXCUTE_WPS
    nRet = pthread_mutex_init(&mutexALWiFiWPS, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexALWiFiWPS pthread_mutex_init() fail! nRet = %d\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }
#endif

    /* ====  Create necessary thread ==== */
    MGR_INFO("==== Create thread ==== \n");

    /* create STA mode tasklet thread */
    {
        pthread_t          sta_task;
        pthread_attr_t     sta_attr;

        //MGR_INFO("==== Create [STA mode tasklet] thread ==== \n");
        pthread_attr_init(&sta_attr);
        pthread_attr_setdetachstate(&sta_attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&sta_attr, WIFI_STACK_SIZE);
        nRet = pthread_create(&sta_task, &sta_attr, _WifiMgr_Sta_Thread, NULL);
        if (nRet)
            MGR_ERROR(" create [STA mode] thread fail! 0x%08X \n", nRet );
    }

    /* create WifiMgr main process tasklet thread */
    {
        pthread_t          main_proc_task;
        pthread_attr_t     main_proc_attr;

        //MGR_INFO("==== Create [WifiMgr main process] thread ==== \n");
        pthread_attr_init(&main_proc_attr);
        pthread_attr_setdetachstate(&main_proc_attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&main_proc_attr, WIFI_STACK_SIZE);
        nRet = pthread_create(&main_proc_task, &main_proc_attr, _WifiMgr_Main_Process_Thread, NULL);
        if (nRet)
            MGR_ERROR(" create [WifiMgr main process] thread fail! 0x%08X \n", nRet );
    }

    /* create Wifi mode switch tasklet thread */
    if (gWifiMgrVar.IS_WifiMgr_Switch_Mode_Thread == false) {/* Create task only first time */
        pthread_t          sw_task;
        pthread_attr_t     sw_attr;
        mhd_sta_register_link_callback(_WifiMgr_Sta_Link_Up_Cb, _WifiMgr_Sta_Link_Down_Cb );

        /* Not daemon thread, do not kill it. */
        //MGR_INFO("==== Create [Wifi mode switch] thread ==== \n");
        pthread_attr_init(&sw_attr);
        pthread_attr_setdetachstate(&sw_attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&sw_attr, WIFI_STACK_SIZE);
        nRet = pthread_create(&sw_task, &sw_attr, _WifiMgr_Sta_HostAP_Switch_Thread, NULL);
        if (nRet)
            MGR_ERROR(" create [Wifi mode switch] thread fail! 0x%08X \n", nRet );
    }

    gWifiMgrVar.WIFI_Mode = init_mode;
    printf("[WIFIMGR] %s() L#%d: WIFI Mode = %d\r\n", __FUNCTION__, __LINE__, gWifiMgrVar.WIFI_Mode);

    BOOLEAN_SET_FALSE(gWifiMgrVar.Cancel_WPS);
    BOOLEAN_SET_FALSE(gWifiMgrVar.Cancel_Connect);
    BOOLEAN_SET_FALSE(gWifiMgrVar.WIFI_Terminate);
    BOOLEAN_SET_TRUE(gWifiMgrVar.WIFI_Init_Ready);

	return nRet;

err_end:
    pthread_mutex_destroy(&mutexMode);
    pthread_mutex_destroy(&mutexIni);
    pthread_mutex_destroy(&mutexALWiFi);
    sem_destroy(&semSwitchModeStart);
    sem_destroy(&semSwitchModeStop);
    sem_destroy(&semConnectStop);
    sem_destroy(&semConnectStart);
#if WIFIMGR_EXCUTE_WPS
    pthread_mutex_destroy(&mutexALWiFiWPS);
    sem_destroy(&semWPSStop);
    sem_destroy(&semWPSStart);
#endif

    return nRet;
}



int WifiMgr_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    int nWait = 0;

    MGR_TRACE("Terminate start ====>\n");
    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    /* Set some necessary flags in terminate scenes */
    BOOLEAN_SET_FALSE(gWifiMgrVar.Start_Scan);
    BOOLEAN_SET_FALSE(gWifiMgrVar.IS_WifiMgr_Sta_Thread);
    BOOLEAN_SET_TRUE(gWifiMgrVar.WIFI_Terminate);

#if WIFIMGR_EXCUTE_WPS
    sem_post(&semWPSStart);
#endif

    switch (WifiMgr_Get_WIFI_Mode())
    {
        case WIFIMGR_MODE_CLIENT:
            _WifiMgr_Sta_Terminate();
            break;

        case WIFIMGR_MODE_SOFTAP:
            _WifiMgr_HostAP_Terminate();
            break;

        default:
            MGR_ERROR("Unknow mode is not support terminated\n");
    }

    {
        int val;
        sem_getvalue(&semSwitchModeStop, &val);
        if (val > 0)
            sem_wait(&semSwitchModeStop);
    }

    usleep(2000);

    /* Destory part of mutex and sem */
    pthread_mutex_destroy(&mutexMode);
    pthread_mutex_destroy(&mutexIni);
    pthread_mutex_destroy(&mutexALWiFi);
#if WIFIMGR_EXCUTE_WPS
    pthread_mutex_destroy(&mutexALWiFiWPS);
    sem_destroy(&semWPSStop);
    sem_destroy(&semWPSStart);
#endif

    /* Initialize these flags */
    BOOLEAN_SET_FALSE(gWifiMgrVar.WIFI_Init_Ready);
    BOOLEAN_SET_FALSE(gWifiMgrVar.WIFI_Terminate);
    BOOLEAN_SET_FALSE(gWifiMgrVar.SoftAP_Hidden);
    BOOLEAN_SET_FALSE(gWifiMgrVar.SoftAP_Init_Ready);

    do {
        MGR_TRACE("WifiMgr_Main_Process_Thread[%d]/ WifiMgr_Sta_Thread[%d]\n",
            gWifiMgrVar.IS_WifiMgr_Main_Process_Thread,
            gWifiMgrVar.IS_WifiMgr_Sta_Thread);
        usleep(200*1000);
        nWait++;
        if (nWait>10)
            break;

    } while (gWifiMgrVar.IS_WifiMgr_Main_Process_Thread == true || gWifiMgrVar.IS_WifiMgr_Sta_Thread == true);

    //WifiMgr_Sta_Not_Cancel_Connect();
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);
    sleep(1);
    MGR_TRACE("<==== Terminate end\n");

    return nRet;
}

uint32_t WifiMgr_Get_WIFI_IP(void)
{
    switch (WifiMgr_Get_WIFI_Mode())
    {
        case WIFIMGR_MODE_CLIENT:
            /* === STA=== */
            MGR_TRACE("STA , WifiMgr_Get_WIFI_IP...\n");
            return mhd_sta_ipv4_ipaddr();
            break;

        case WIFIMGR_MODE_SOFTAP:
            /* === AP === */
            MGR_TRACE("AP , WifiMgr_Get_WIFI_IP...\n");
            return 0x0101a8c0;
            break;

        default:
                MGR_ERROR("Unknow mode\n");

    }

}

uint32_t WifiMgr_Get_WIFI_GW(void)
{
    switch (WifiMgr_Get_WIFI_Mode())
    {
        case WIFIMGR_MODE_CLIENT:
            /* === STA=== */
            MGR_TRACE("STA , WifiMgr_Get_WIFI_GW...\n");
            return mhd_sta_ipv4_gateway();
            break;

        case WIFIMGR_MODE_SOFTAP:
            /* === AP === */
            MGR_TRACE("AP , WifiMgr_Get_WIFI_GW...\n");
            return 0x0101a8c0;
            break;

        default:
                MGR_ERROR("Unknow mode\n");

    }

}

uint32_t WifiMgr_Get_WIFI_Mask(void)
{
    switch (WifiMgr_Get_WIFI_Mode())
    {
        case WIFIMGR_MODE_CLIENT:
            /* === STA=== */
            MGR_TRACE("STA , WifiMgr_Get_WIFI_Mask...\n");
            return mhd_sta_ipv4_netmask();
            break;

        case WIFIMGR_MODE_SOFTAP:
            /* === AP === */
            MGR_TRACE("AP , WifiMgr_Get_WIFI_Mask...\n");
            return 0x00ffffff;
            break;

        default:
                MGR_ERROR("Unknow mode\n");

    }

}

int
WifiMgr_HostAP_First_Start(void)
{
    printf("WifiMgr_HostAP_First_Start \n");

    sleep(1);

}


int
WifiMgr_Sta_HostAP_Switch(WIFI_MGR_SETTING wifiSetting)
{
    memset(&gWifiSetting, 0,            sizeof(WIFI_MGR_SETTING));
    memcpy(&gWifiSetting, &wifiSetting, sizeof(WIFI_MGR_SETTING));

    //MGR_INFO(">>>>>>>>>> [Stress testing] Start to Switch WIFI Mode <<<<<<<<<\n");
    //MGR_INFO(">>>>>>>>>> Switch Mode Start: post\n\n");
    sem_post(&semSwitchModeStart);

    return WIFIMGR_ECODE_OK;
}

// disable auto reconnect
int WifiMgr_Disable_Auto_Reconnect(int nDisable)
{
    printf("[Wifi mgr mhd]WifiMgr_Disable_Auto_Reconnect %d, #line %d \n",nDisable,__LINE__);
    gDisableAutoReconnect = nDisable;
	return 0;
}

// 1: enable 0: disable , default disable
void WifiMgr_P2P_Enable(int nP2p)
{
    wifi_enable_p2p(nP2p);
}


#endif
