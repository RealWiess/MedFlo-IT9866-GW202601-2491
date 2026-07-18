#if defined CFG_NET_WIFI_SDIO_NGPL_ATBM6031
#include <sys/ioctl.h>
#include "lwip/dhcp.h"
#include "wifiMgr.h"
#include "ite/ite_wifi.h"
#include "mhd_api.h"
#include "atbm_ext_api.h"

/* Extern Function */
extern void         dhcps_init(void);
extern void         dhcps_deinit(void);
extern struct netif xnetif[2];
extern TickType_t   xTaskGetTickCount(void);

/* Global Variable */
static pthread_t                         ClientModeTask, ProcessTask;

static sem_t                             semConnectStart, semConnectStop;
static pthread_mutex_t                   mutexALWiFi, mutexIni, mutexMode;

static sem_t                             semWPSStart, semWPSStop;
static pthread_mutex_t                   mutexALWiFiWPS;

static WIFIMGR_CONNSTATE_E               wifi_conn_state    = WIFIMGR_CONNSTATE_STOP;
static WIFIMGR_ECODE_E                   wifi_conn_ecode    = WIFIMGR_ECODE_SET_DISCONNECT;

static WIFI_MGR_SCANAP_LIST              gWifiMgrApList[WIFI_SCAN_LIST_NUMBER] = {0};

static WIFI_MGR_SETTING                  gWifiMgrSetting    = {0};
static struct net_device_info            gScanApInfo        = {0};

static struct timeval                   tvDHCP1 = {0, 0}, tvDHCP2     = {0, 0};

// PhoneIP is the source side of Miracast
static uint32_t                         PhoneIP = 0x0;

extern atbm_uint8 wifi_event_status;

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
    .Is_WIFI_Available      = false
};

#define FORMAT_IPADDR(x) ((unsigned char *)&x)[3], ((unsigned char *)&x)[2], ((unsigned char *)&x)[1], ((unsigned char *)&x)[0]
extern struct atbm_sdio_func *wifi_sdio_func;

void wifi_off()
{
	atbm_wifi_hw_deinit();
}
int temp_sta_get_mac_address( mhd_mac_t *mac )
{
    atbm_wifi_get_mac_address(mac->octet);
    return 0;
}

int temp_softap_get_mac_address( mhd_mac_t *mac )
{
    atbm_wifi_get_mac_address_vif(1,mac->octet);
}

int temp_sta_network_up( uint32_t ip, uint32_t gateway, uint32_t netmask )
{
    //netif_set_up(p_netif);
    return 0;
}
uint32_t temp_sta_ipv4_ipaddr( void )
{
    uint32_t ip;
    ip = (xnetif[0].ip_addr.addr);
    return ip;
}


uint32_t temp_sta_ipv4_netmask( void )
{
    uint32_t ip;
    ip = (xnetif[0].netmask.addr);
    return ip;
}

uint32_t temp_sta_ipv4_gateway( void )
{
    uint32_t ip;
    ip = (xnetif[0].gw.addr);
    return ip;
}
/* ================================================= */
/*
/* Static Functions */
/*
/* ================================================= */

static char *_WifiMgr_Sta_Get_Security_String_By_Index(uint8_t security)
{

    return ( security == ATBM_KEY_NONE ) ? "OPEN":
           ( security == ATBM_KEY_WEP ) ? "WEP OPEN":
           ( security == ATBM_KEY_WEP_SHARE ) ? "WEP_SHARED":
           ( security == ATBM_KEY_WPA ) ? "WPA PSK TKIP":
           ( security == ATBM_KEY_WPA2 ) ? "WPA2 PSK AES":
           ( security == ATBM_KEY_MIX ) ? "WPA PSK MIXED":
           ( security == ATBM_KEY_SAE ) ? "WPA3-SAE":
           ( security == ATBM_KEY_MAX ) ? " WPA2_ENTERPRISE":
           "UNKNOWN";
}


static void _WifiMgr_Sta_Scan_Result_Print( mhd_ap_info_t *results, int num )
{
    int k;
    mhd_ap_info_t *record;

    record = &results[0];
    for ( k = 0; k < num; k++, record++ )
    {
        /* Print SSID */
        printf("\n[%03d]\n", k+1 );
        printf("       SSID          : %s\n", record->ssid );

        /* Print other network characteristics */
        printf("       BSSID         : %02X:%02X:%02X:%02X:%02X:%02X\n",
                                                  (uint8_t)record->bssid[0], (uint8_t)record->bssid[1], (uint8_t)record->bssid[2],
                                                  (uint8_t)record->bssid[3], (uint8_t)record->bssid[4], (uint8_t)record->bssid[5] );
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


// sta connect to ap
static int
_WifiMgr_Sta_Connect_Process(void)
{
    ITPEthernetSetting setting;
    unsigned long tick1 = xTaskGetTickCount();
    unsigned long tick2, tick3;

    int nRet = WIFIMGR_ECODE_OK, cRet = 0;
    int is_connected = 0, dhcp_available = 0;
    int phase = 0, nSecurity = -1, nAPCount = 0;
    bool is_ssid_match = false;
    unsigned long connect_cnt = 0, retry_connect_cnt = 0, retry_dhcp_cnt = 0;
    char *ssid, *password;
    WIFI_MGR_SCANAP_LIST pList[64];

    unsigned long      security_type;
    int 				ssid_len;
    int 				password_len;
    int 				key_id;
    void				*semaphore;
    unsigned char *ip;
    int nChannel;
	uint32_t nIp, gateway, netmask;


    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    ssid            = gWifiMgrVar.Ssid;
    password        = gWifiMgrVar.Password;
//    security_type   = gWifiMgrVar.SecurityMode;

    ssid_len = strlen((const char *)ssid);
    password_len = strlen((const char *)password);
    key_id = 0;
    semaphore = NULL;



    if (gWifiMgrVar.Cancel_Connect) {
        nRet = WIFIMGR_ECODE_CONNECT_ERROR;
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);

        goto end;
    }
    if(atbm_wifi_get_current_mode() != ATBM_WIFI_STA_MODE)
	{
		atbm_wifi_on(ATBM_WIFI_STA_MODE);
	}
    //CMD: api 0 [SSID] [Password]
    cRet = atbm_wifi_sta_join_ap(ssid, NULL, 0, 0, password);

    printf("[%s] atbm_sta_connect results: %d #line %d \n", __FUNCTION__, cRet ,__LINE__);

    if (cRet != 0 ) {
         printf("[%s] wifi connect retry failed, quit this SSID(%s)\n", __FUNCTION__, ssid);
        nRet = WIFIMGR_ECODE_CONNECT_ERROR;
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);
        goto end;
    }

    /* Another connection error code */
    if (cRet != 0 ) {
        printf("[%s] wifi connect have another errors - RTW Error Code(%d)\n", __FUNCTION__, cRet);
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
        printf("[WIFIMGR]%s() L#%ld: Error! Wifi setting has no SSID\r\n", __FUNCTION__, __LINE__);
        nRet = WIFIMGR_ECODE_NO_SSID;
        goto end;
    }

#if defined(CFG_NET_ETHERNET) && defined(CFG_NET_WIFI)
    printf("[WIFIMGR] check wifi netif %d \n",ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_STATUS, NULL));
    // Check if the wifi netif is exist
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_STATUS, NULL) == 0) {
        printf("[WIFIMGR]%s() L#%ld: wifi need to add netif !\r\n", __FUNCTION__, __LINE__);
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, NULL);
    }
#endif

    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    // Wait for connecting...
    printf("[WIFIMGR] Wait for connecting\n");
    if (setting.dhcp) {
       // ip_addr_set_zero(&xnetif[0].ip_addr);
        // Wait for DHCP setting...
        printf("[WIFIMGR] Wait for DHCP setting");

        //dhcp_start(&xnetif[0]);
        tick3 = xTaskGetTickCount();
       // unsigned char *ip = LwIP_GetIP(&xnetif[0]);
        printf("\r\n\nIP set zero.\nGot IP after %dms.\n", (tick3-tick1));

       // mhd_sta_network_up(0, 0, 0);
        connect_cnt = WIFI_CONNECT_DHCP_COUNT;
	 int cnt=0;
	 while(1)
	  {
	  	 cnt++;
		 sleep(1);
		 if(atbm_wifi_isconnected(0))
		 {
		 	break;
		 }
		 else if(cnt == WIFI_CONNECT_DHCP_COUNT)
	 	{
	 	    atbm_wifi_sta_disjoin_ap();
            usleep(100000);
			printf("\r\n[WIFIMGR] DHCP setting error\r\n");
			goto end;
	 	}
	  }
        while (connect_cnt)
        {
            nIp = htonl(temp_sta_ipv4_ipaddr());
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
                printf("\r\n[WIFIMGR]%s() L#%ld: DHCP timeout! connect fail!\r\n", __FUNCTION__, __LINE__);
                nRet = WIFIMGR_ECODE_DHCP_ERROR;
                goto end;
            }

            if (gWifiMgrVar.Cancel_Connect || gWifiMgrVar.WIFI_Terminate)
            {
                goto end;
            }

            usleep(100000);


        }
        nIp = htonl(temp_sta_ipv4_ipaddr());
        printf("Ip addr: %u.%u.%u.%u\n", FORMAT_IPADDR(nIp));
        netmask = htonl(temp_sta_ipv4_netmask());
        printf("netmask: %u.%u.%u.%u\n", FORMAT_IPADDR(netmask));
        gateway = htonl(temp_sta_ipv4_gateway());
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
        printf("[WIFIMGR]%s() L#%ld: End. Cancel_Connect is set.\r\n", __FUNCTION__, __LINE__);

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_CANCEL);
    }

	/* Sava last time connect info otherwise WIFI can't reconnect for wake up */
    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_SAVE_INFO);

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
        if(strlen(ssid)<WIFI_SSID_MAXLEN){
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
    int i, j, mac_cmp;
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

static int atbm_scan_ap( mhd_ap_info_t *results, int *num )
{
	int i;
	char *buff;
	buff = malloc(sizeof(char)*2732);
	WLAN_SCAN_RESULT *result = (WLAN_SCAN_RESULT *)buff;
		memset(buff, 0, 2732);
	WLAN_BSS_INFO *info = result->bss_info;
	if(atbm_wifi_get_current_mode() != 0)
	{
		atbm_wifi_on(ATBM_WIFI_STA_MODE);
	}
	atbm_wifi_scan_network(buff,2732);
	*num = result->count;
	for(i =0;i < result->count; i++)
	{
		memset(results->ssid,0,32);
		memcpy(results->ssid,(char *)info->SSID,info->SSID_len);
		memcpy(results->bssid,info->BSSID,6);
		results->channel =info->chanspec;
		results->security = info->security;
		results->rssi= info->RSSI;
		//printf("\nAPIndex:%d\n",i);
		//printf("SSID:%s\n",(char *)info->SSID);
		//printf("channel=%d\n",info->chanspec);
		//printf("security:%d\n",info->security);

		//printf("BSSID:%x-%x-%x-%x-%x-%x\n",info->BSSID[0],info->BSSID[1],info->BSSID[2],info->BSSID[3],info->BSSID[4],info->BSSID[5]);
		info++;
		results++;
	}
	free(buff);
	return 0;
}



static int
_WifiMgr_Sta_Scan_Process(struct net_device_info *apInfo)
{
    int nRet = 0, scan_result;
    int nWifiState = 0;
    int i = 0;
    int nHideSsid = 0;
    mhd_ap_info_t results[WIFI_SCAN_LIST_NUMBER];
    int num = sizeof(results)/sizeof(mhd_ap_info_t);
    int nTemp ;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        printf("scanWifiAp  !WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    memset(apInfo, 0, sizeof(struct net_device_info));

    printf("[Wifi mgr]%s() Start to SCAN AP ==========================\r\n", __FUNCTION__);

    atbm_scan_ap(results, &num);


    _WifiMgr_Sta_Scan_Result_Print(results, num);

    if(num > 0 ){
		gWifiMgrVar.Pre_Scan = true;
	}else{
        printf("\n\rERROR: wifi scan failed, result code(%d).\n", scan_result);
        return WIFIMGR_ECODE_NOT_INIT;
    }

    gWifiMgrVar.Start_Scan = true;

    printf("[Wifi mgr]%s() ScanApInfo.apCnt = %ld #line %d \r\n", __FUNCTION__, num, __LINE__);

    for (i = 0; i < num; i++)
    {
        unsigned int ssid_len = strlen(results[i].ssid);
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
            memcpy(gWifiMgrApList[i].ssidName,  results[i].ssid, 32);
            memcpy(gWifiMgrApList[i].apMacAddr,  results[i].bssid, 6);
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

    while (1)
    {
        printf("====>ClientModeThreadFunc\n");
        sem_wait(&semConnectStart);

        if (gWifiMgrVar.WIFI_Terminate) {
            printf("[Wifi mgr]terminate _WifiMgr_Sta_Thread(0) \n");
            break;
        }

        if (gWifiMgrVar.Need_Set){
            wifi_conn_state = WIFIMGR_CONNSTATE_SETTING;
            printf("[WIFIMGR] START to Set!\r\n");
            wifi_conn_ecode = nRet = WIFIMGR_ECODE_OK;

            gWifiMgrVar.Need_Set = false;
            printf("[WIFIMGR] Set finish!\r\n");
        }
        usleep(1000);
        printf("====>nRet: %d\n", nRet);

		if (strcmp(gWifiMgrVar.Ssid, "") == 0)
			nRet = WIFIMGR_ECODE_NO_SSID;
		else
			nRet = WIFIMGR_ECODE_OK;

        if (nRet == WIFIMGR_ECODE_OK) {
            wifi_conn_state = WIFIMGR_CONNSTATE_CONNECTING;

            printf("[WIFIMGR] START to Connect!\r\n");

            gWifiMgrVar.Cancel_WPS = true;
            wifi_conn_ecode = _WifiMgr_Sta_Connect_Process();
            gWifiMgrVar.Cancel_WPS = false;
            printf("[WIFIMGR] Connect finish!\r\n");

        }
        wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
        usleep(1000);
    }
end:
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
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
int atbm_softap_start( const unsigned char *ssid, const char *password, uint8_t security, uint8_t channel ,bool hidden)
{
	return atbm_wifi_ap_create(ssid, (password && password[0]) ? 0x4 : 0, 0, password, channel, hidden);

}

static int
_WifiMgr_HostAP_Init(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    ITPWifiInfo wifiInfo;
    char softap_ssid[] = "iteAP";
    char softap_passwd[] = "12345678";
    char *softap_security = "2";        //0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
    char *softap_channel = "2";
    char client_IP_Addr[] = "192.168.1.2";
    char *p;
	char *argv[] = {"api", "6", softap_ssid, softap_passwd, softap_security, softap_channel};
    int chan_id = 7;

printf("_WifiMgr_HostAP_Init %s %s , %s %s \n",gWifiMgrVar.ApSsid,gWifiMgrVar.ApPassword,softap_ssid,softap_passwd);
    if (gWifiMgrVar.SoftAP_Hidden){
        printf("[Wifimgr]_WifiMgr_HostAP_Init mhd_softap_set_hidden %d \n",__LINE__);
        //mhd_softap_set_hidden(1);????
    } else {

    }

    //CMD: api 6 [SSID] [Password] [security] [Channel]
    // for test ......

    wifi_set_mode(1);
    //atbm_softap_start(argv[2], argv[3], strtol(argv[4], &p, 10), strtol(argv[5], &p,10), gWifiMgrVar.SoftAP_Hidden);
   	atbm_wifi_ap_create(gWifiMgrVar.ApSsid,(gWifiMgrVar.ApPassword && gWifiMgrVar.ApPassword[0]) ? 0x4 : 0,0,gWifiMgrVar.ApPassword,chan_id,0);
  // mhd_softap_start(gWifiMgrVar.ApSsid, gWifiMgrVar.ApPassword, strtol(argv[4], &p, 10), strtol(argv[5], &p, 10));
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, &xnetif[0]);

    //mhd_softap_start_dhcpd(0xc0a80101);


    //dhcps_init();

    usleep(1000);

    return nRet;
}


static int
_WifiMgr_HostAP_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    dhcps_deinit();
    usleep(1000*10);

//    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_DISABLE, NULL);
    usleep(1000*1000);

    return nRet;
}


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

static void*
_WifiMgr_Main_Process_Thread(void *arg)
{
    int nRet = 0;
    int bIsAvail = 0, nWiFiConnState = 0, nWiFiConnEcode = 0;
    int nPlayState = 0;
    WIFIMGR_CONNSTATE_E gWifi_connstate = WIFIMGR_CONNSTATE_STOP;

    int nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
    static struct timeval tv1 = {0, 0}, tv2 = {0, 0};
    static struct timeval tv3_temp = {0, 0}, tv4_temp = {0, 0};
    long temp_disconn_time = 0;
    int wifi_mode_now = 0, is_softap_ready = 1;


    gWifiMgrVar.Is_First_Connect = true;
    gWifiMgrVar.Is_Temp_Disconnect = false;
    gWifiMgrVar.No_Config_File = false;
    gWifiMgrVar.No_SSID = false;

    usleep(20000);
    printf("====>ProcessThreadFunc\n");

    while (1)
    {
        nCheckCnt--;
        if (gWifiMgrVar.WIFI_Terminate) {
            printf("[Wifi mgr]terminate WifiMgr_Process_ThreadFunc \n");
            break;
        }

        usleep(1000);
        //printf("_WifiMgr_Main_Process_Thread %d %d \n",nCheckCnt,WifiMgr_Get_WIFI_Mode);
        if (nCheckCnt == 0) {
            wifi_mode_now = WifiMgr_Get_WIFI_Mode();

            if (wifi_mode_now == WIFIMGR_MODE_SOFTAP){
                // Soft AP mode
                if (!gWifiMgrVar.SoftAP_Init_Ready) {
                    printf("[Main]%s() L#%ld: is_softap_ready=%ld\r\n", __FUNCTION__, __LINE__, is_softap_ready);
                    if (is_softap_ready) {
                        gWifiMgrVar.SoftAP_Init_Ready = true;
                        gWifi_connstate = WIFIMGR_CONNSTATE_STOP;
                        if (gWifiMgrSetting.wifiCallback)
                            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);
                    }
                }
            }
            else
            {
                // Client mode
                if (gWifiMgrVar.Is_First_Connect) {
                    // First time connect when the system start up
                    nRet = _WifiMgr_Sta_Connect_Post();
                    if (nRet == WIFIMGR_ECODE_OK) {
                        gWifi_connstate = WIFIMGR_CONNSTATE_CONNECTING;
                    }
                    gWifiMgrVar.Is_First_Connect = false;

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
                            nRet = WifiMgr_Sta_Is_Available(&bIsAvail);
                            if (!bIsAvail) {
                                // fail, restart the timer
                                gettimeofday(&tv1, NULL);
                            }
                        } else {
                            printf("[WIFIMGR]%s() L#%ld: Error! nWiFiConnEcode = 0%ld\r\n", __FUNCTION__, __LINE__, nWiFiConnEcode);

                            // connection has error
                            if (nWiFiConnEcode == WIFIMGR_ECODE_NO_INI_FILE) {
                                gWifiMgrVar.No_Config_File = true;
                            }
                            if (nWiFiConnEcode == WIFIMGR_ECODE_NO_SSID) {
                                gWifiMgrVar.No_SSID = true;
                            } else {
                                // fail, restart the timer
                                gettimeofday(&tv1, NULL);
                            }
                        }
                    }
                    goto end;
                }

                nRet = WifiMgr_Sta_Is_Available(&bIsAvail);
                nRet = WifiMgr_Get_Connect_State(&nWiFiConnState, &nWiFiConnEcode);
        	if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) && gWifiMgrVar.Is_Temp_Disconnect)
        		gWifiMgrVar.Is_Temp_Disconnect = false;

                if (bIsAvail)
                {
                    if (gWifiMgrVar.Is_Temp_Disconnect) {
                        gWifiMgrVar.Is_Temp_Disconnect = false;     // reset
                        printf("[WIFIMGR]%s() L#%ld: WiFi auto re-connected!\r\n", __FUNCTION__, __LINE__);
                        if (gWifiMgrSetting.wifiCallback)
                            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_RECONNECTION);
                    }

                    if (!gWifiMgrVar.Is_WIFI_Available) {
                        // prev is not available, curr is available
                        gWifiMgrVar.Is_WIFI_Available = true;
                        gWifiMgrVar.No_Config_File = false;
                        gWifiMgrVar.No_SSID = false;
                        printf("[WIFIMGR]%s() L#%ld: WiFi auto re-connected!\r\n", __FUNCTION__, __LINE__);
                    }
                     gettimeofday(&tvDHCP2, NULL);
                     if (itpTimevalDiff(&tvDHCP1, &tvDHCP2) > WIFIMGR_DHCP_RENEW_MSEC) {
                         printf("====>Send DHCP Discover!!!!!\n");
                         printf("DHCP renew %d \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2));
                         ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RENEW_DHCP, NULL);
                         gettimeofday(&tvDHCP1, NULL);
                         gettimeofday(&tvDHCP2, NULL);
                     } else {
                    //     printf("DHCP wait renew  %d \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2));
                     }


                } else {
                    if (gWifiMgrVar.Is_WIFI_Available){
                        if (!gWifiMgrVar.Is_Temp_Disconnect && nWiFiConnEcode == WIFIMGR_ECODE_OK)
                        {
                            // first time detect
                            gWifiMgrVar.Is_Temp_Disconnect = true;
                            gettimeofday(&tv3_temp, NULL);
                            printf("[WIFIMGR]%s() L#%ld: WiFi temporary disconnected!%d %d\r\n", __FUNCTION__, __LINE__,nWiFiConnState,nWiFiConnEcode);
                            if (gWifiMgrSetting.wifiCallback)
                                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT);
                        } else if (nWiFiConnEcode == WIFIMGR_ECODE_OK){
                            gettimeofday(&tv4_temp, NULL);
                            temp_disconn_time = itpTimevalDiff(&tv3_temp, &tv4_temp);
                            printf("[WIFIMGR]%s() L#%ld: temp disconnect time = %ld sec. %d %d\r\n", __FUNCTION__, __LINE__, temp_disconn_time / 1000 , nWiFiConnState,nWiFiConnEcode);
                            if (temp_disconn_time >= WIFIMGR_TEMPDISCONN_MSEC) {
                                printf("[WIFIMGR]%s() L#%ld: WiFi temporary disconnected over %ld sec. Services should be shut down.\r\n", __FUNCTION__, __LINE__, temp_disconn_time / 1000);
                                gWifiMgrVar.Is_Temp_Disconnect = false;     // reset

                                if (gWifiMgrSetting.wifiCallback)
                                    gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S);

                                // prev is available, curr is not available
                                gWifiMgrVar.Is_WIFI_Available = false;
                            }
                        }
                    }
                    else
                    {
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
            }

    end:
            nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
        }
    }
    return NULL;
}

static int
_WifiMgr_Enable_PowerSave(void)
{
//    return mhd_sta_set_powersave(0,0);
}


static int
_WifiMgr_Disable_PowerSave(void)
{

    //return mhd_sta_set_powersave();;
}

uint32_t WifiMgr_Secu_ITE_To_MHD(const char* ite_security_enum)
{
    uint32_t security;
   #if 0
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

    printf("WifiMgr_Secu_ITE_To_MHD: ITE(%s) -> MHD(%d)\n", ite_security_enum, security);
#endif
    return security;
}


char* WifiMgr_Secu_MHD_To_ITE(uint32_t security)
{
    static char ite_security_enum[3];
#if 0
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

    printf("WifiMgr_Secu_MHD_To_ITE: MHD(%d) -> ITE(%s)\n", security, ite_security_enum);
#endif
    return ite_security_enum;
}


/* ================================================= */




/* ================================================= */
/*
/* Static Functions */
/*
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

    ITPWifiInfo wifiInfo;
	mhd_mac_t mac;

//    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_GET_INFO, &wifiInfo);

    temp_sta_get_mac_address(&mac);

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
}


int
WifiMgr_Get_HostAP_Device_Number(void)
{
    int stacount = 0;
    int client_number;
    int ret;
    uint32_t count = 0;
    mhd_mac_t mac_list[8];


    ret = temp_softap_get_mac_list( (mhd_mac_t *)mac_list, &count );
    if ( ret == 0 )
    {
        for (int i=0; i<count; i++)
        {
            printf("MAC[%d]: %02X:%02X:%02X:%02X:%02X:%02X\n", i,
                  mac_list[i].octet[0], mac_list[i].octet[1], mac_list[i].octet[2],
                  mac_list[i].octet[3], mac_list[i].octet[4], mac_list[i].octet[5]);
        }
    }

    printf("\n\rClient Num: %d", count);
    printf("\n\r");

    return count;
}


int
WifiMgr_Sta_Is_Available(int* is_available)
{
    int nRet = WIFIMGR_ECODE_OK;
    int is_connected = 0, is_avail = 0;
    uint32_t nIp=0;
    char msg_wifi[16];

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        *is_available = 0;
        return 0;
    }

    nIp = htonl(temp_sta_ipv4_ipaddr());


    //sprintf(msg_wifi, "%u.%u.%u.%u",FORMAT_IPADDR(nIp));

    //printf("WifiMgr_Sta_Is_Available %d %s %u.%u.%u.%u\n",nIp,msg_wifi,FORMAT_IPADDR(nIp));
    if (nIp)
        return 1;

    return WIFIMGR_ECODE_OK;
}

int
WifiMgr_Get_Sta_Avalible(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    int is_connected = 0, is_avail = 0;
    uint32_t nIp=0;
    char msg_wifi[16];


    nIp = htonl(temp_sta_ipv4_ipaddr());


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

    nIp = htonl(temp_sta_ipv4_ipaddr());


    if (nIp)
        return 1;

    return WIFIMGR_ECODE_OK;

}



int
WifiMgr_Sta_Connect(const char* ssid, const char* password, const char* secumode)
{
    int nRet = WIFIMGR_ECODE_OK;
    uint32_t security;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (gWifiMgrVar.WIFI_Mode != WIFIMGR_MODE_CLIENT){
        printf("[Wifi mgr] WifiMgr_Sta_Connect need client mode to connect %d  #line %d  \n",gWifiMgrVar.WIFI_Mode,__LINE__);
        return WIFIMGR_ECODE_NOT_INIT;
    }

    security = WifiMgr_Secu_ITE_To_MHD(secumode);

    _WifiMgr_Sta_Entry_Info(ssid, password, security);

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

    ITPEthernetSetting setting;

    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (wifi_conn_state == WIFIMGR_CONNSTATE_CONNECTING){
        WifiMgr_Sta_Cancel_Connect();
    }

	printf("WifiMgr_Sta_Disconnect \n");
    if((nRet = atbm_wifi_sta_disjoin_ap()) != 0)
       return 1;
 // mhd_sta_network_down();?????
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
    printf("WifiMgr_Sta_Disconnect end \n");
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode = WIFIMGR_ECODE_SET_DISCONNECT;
    usleep(1000*300);
    WifiMgr_Sta_Not_Cancel_Connect();
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);

    return nRet;
}


int
WifiMgr_Sta_Sleep_Disconnect(void)
{
    ITPEthernetSetting setting;

    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }
    if((nRet = atbm_wifi_sta_disjoin_ap()) != 0)
       return 1;

	printf("WifiMgr_Sta_Sleep_Disconnect %d \n",nRet);
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode = WIFIMGR_ECODE_SET_DISCONNECT;
    usleep(1000*100);

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



int
WifiMgr_Init(WIFIMGR_MODE_E init_mode, int mp_mode,WIFI_MGR_SETTING wifiSetting)
{
    int nRet = WIFIMGR_ECODE_OK;
    pthread_attr_t attr, attr1,attr2;

    while(gWifiMgrVar.WIFI_Terminate){
         printf("WifiMgr not finished yet \n");
         usleep(200*1000);
    }

    wifi_conn_state         = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode         = WIFIMGR_ECODE_SET_DISCONNECT;
    gWifiMgrVar.Need_Set    = false;
    gWifiMgrVar.MP_Mode     = mp_mode;

    gWifiMgrSetting.wifiCallback = wifiSetting.wifiCallback;

    if (init_mode ==WIFIMGR_MODE_CLIENT){
        _WifiMgr_Sta_Entry_Info(wifiSetting.ssid, wifiSetting.password, wifiSetting.secumode);
    } else if (init_mode ==WIFIMGR_MODE_SOFTAP){
        printf("[Wifimgr] WIFI start AP: SSID(%s)/PW(%s) \n", wifiSetting.ap_ssid, wifiSetting.ap_password);
        _WifiMgr_HostAP_Entry_Info(wifiSetting.ap_ssid, wifiSetting.ap_password);
        _WifiMgr_HostAP_Init();
    }

    // default select dhcp
    gWifiMgrSetting.setting.dhcp = 1;
    if (wifiSetting.setting.ipaddr[0]>0){
        memcpy(&gWifiMgrSetting.setting,&wifiSetting.setting,sizeof(ITPEthernetSetting));
    }

    // init semaphore
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

    // init mutex
    nRet = pthread_mutex_init(&mutexALWiFi, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexALWiFi pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexALWiFiWPS, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexALWiFiWPS pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexIni, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexIni pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexMode, NULL);
    if (nRet != 0) {
        printf("[WIFIMGR] ERROR, mutexMode pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    // create thread
    printf("[WifiMgr] Create thread \n");

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
    pthread_create(&ClientModeTask, &attr, _WifiMgr_Sta_Thread, NULL);

    pthread_attr_init(&attr2);
    pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr2, WIFI_STACK_SIZE);
    pthread_create(&ProcessTask, &attr2, _WifiMgr_Main_Process_Thread, NULL);

    gWifiMgrVar.WIFI_Mode = init_mode;
    printf("[WIFIMGR] %s() L#%ld: WIFI Mode = %ld\r\n", __FUNCTION__, __LINE__, gWifiMgrVar.WIFI_Mode);

    WifiMgr_Sta_Not_Cancel_Connect();
    gWifiMgrVar.Cancel_WPS      = false;

    gWifiMgrVar.WIFI_Init_Ready = true;
    gWifiMgrVar.WIFI_Terminate  = false;
end:
    return nRet;

err_end:
    pthread_mutex_destroy(&mutexMode);
    pthread_mutex_destroy(&mutexIni);
    pthread_mutex_destroy(&mutexALWiFiWPS);
    sem_destroy(&semWPSStop);
    sem_destroy(&semWPSStart);
    pthread_mutex_destroy(&mutexALWiFi);
    sem_destroy(&semConnectStop);
    sem_destroy(&semConnectStart);

    return nRet;
}



int WifiMgr_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    printf("WifiMgr_Terminate \n");
    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    gWifiMgrVar.WPA_Terminate = true;
    if (gWifiMgrVar.WIFI_Mode == WIFIMGR_MODE_SOFTAP) {
        dhcps_deinit();
        usleep(1000*10);

       // ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_DISABLE, NULL);
        usleep(1000*1000);

    } else {
        //client mode: clean "gWifiSetting" at network.c
        if (gWifiMgrSetting.wifiCallback && (gWifiMgrVar.Client_On_Off == WIFIMGR_SWITCH_OFF))
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_CLEAN_INFO);

        gWifiMgrVar.Cancel_WPS = true;
    }

    gWifiMgrVar.WIFI_Terminate = true;


    sem_post(&semWPSStart);
    sem_post(&semConnectStart);

    usleep(2000);

    pthread_mutex_destroy(&mutexMode);
    pthread_mutex_destroy(&mutexIni);
    pthread_mutex_destroy(&mutexALWiFiWPS);
    sem_destroy(&semWPSStop);
    sem_destroy(&semWPSStart);
    pthread_mutex_destroy(&mutexALWiFi);
    sem_destroy(&semConnectStop);
    sem_destroy(&semConnectStart);

    gWifiMgrVar.WIFI_Init_Ready = false;

    gWifiMgrVar.WIFI_Terminate = false;
    gWifiMgrVar.WPA_Terminate = false;
    gWifiMgrVar.SoftAP_Hidden = false;
    gWifiMgrVar.SoftAP_Init_Ready = false;
    WifiMgr_Sta_Not_Cancel_Connect();
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);
    sleep(1);
    printf("WifiMgr_Terminate ~~~~~\n");
    return nRet;
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
}


static void*
_WifiMgr_Sta_HostAP_Switch_Thread(void *arg)
{
    int nTemp;
    int channel=7;
    int ret;

    printf("WifiMgr_Sta_HostAP_Switch mode  %d\n",gWifiMgrVar.WIFI_Mode);

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        //return WIFIMGR_ECODE_NOT_INIT;
        return NULL;
    }

    gWifiMgrVar.WIFI_Mode = WifiMgr_Get_WIFI_Mode();

    WifiMgr_Terminate();

    gCountT = 0;
    gettimeofday(&gSwitchCount, NULL);

    if (gWifiMgrVar.WIFI_Mode == WIFIMGR_MODE_SOFTAP){
        // init client mode
        printf("WifiMgr_Sta_HostAP_Switch init client  \n");

        atbm_wifi_off(ATBM_WIFI_AP_MODE);
        atbm_SleepMs(100);

        nTemp = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
    } else {
        // init softap mode
		printf("WifiMgr_Sta_HostAP_Switch init softap  \n");
        if(atbm_wifi_isconnected(0))
        {
            atbm_wifi_sta_disjoin_ap();
            atbm_SleepMs(100);
        }
        nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
    }

    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_SWITCH_CLIENT_SOFTAP_FINISH);

    return 0;

}


int
WifiMgr_HostAP_First_Start(void)
{
    printf("WifiMgr_HostAP_First_Start \n");

    //wifi_off();
    sleep(1);
    //wifi_on(RTW_MODE_AP);
}


int
WifiMgr_Sta_HostAP_Switch(WIFI_MGR_SETTING wifiSetting)
{
    pthread_t task;
    pthread_attr_t attr;

    gettimeofday(&gSwitchStart, NULL);

    memcpy(&gWifiSetting,&wifiSetting,sizeof(WIFI_MGR_SETTING));

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
    pthread_create(&task, &attr, _WifiMgr_Sta_HostAP_Switch_Thread, NULL);
}

// disable auto reconnect
int WifiMgr_Disable_Auto_Reconnect(int nDisable)
{
    printf("[Wifi mgr mhd]WifiMgr_Disable_Auto_Reconnect %d, #line %d \n",nDisable,__LINE__);
    //gDisableAutoReconnect = nDisable;
}


uint32_t WifiMgr_Get_WIFI_IP(void)
{
    return temp_sta_ipv4_ipaddr();
}

uint32_t WifiMgr_Get_WIFI_GW(void)
{
    return temp_sta_ipv4_gateway();
}

uint32_t WifiMgr_Get_WIFI_Mask(void)
{
    return temp_sta_ipv4_netmask();
}

/*
    ATBM_P20_NOT_INITIALIZED=0,
    ATBM_P2P_CONNECTED_EVENT=1,
    ATBM_P2P_DISCONECT_EVENT=2,
    ATBM_P2P_FIND_EVENT=3,
    ATBM_P2P_STOP_EVENT=4,
    ATBM_P2P_START_EVENT=5,
    ATBM_P2P_4_WAY_HANDSHAKE=6,
    ATBM_P2P_WSC_EXCHAGE=7,
    ATBM_P2P_GO_NEGOTIATION_CONFIRM=8,
    ATBM_P2P_GO_NEGOTIATION_ERROR=9,
*/
uint8_t WifiMgr_P2P_Get_Status(void)
{
#if 1
    if (wifi_event_status == ATBM_P20_NOT_INITIALIZED)
        printf("WifiMgr_P2P_Get_Status ATBM_P20_NOT_INITIALIZED \n");
    else if (wifi_event_status == ATBM_P2P_CONNECTED_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_CONNECTED_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_DISCONECT_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_DISCONECT_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_FIND_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_FIND_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_STOP_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_STOP_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_START_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_START_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_4_WAY_HANDSHAKE)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_4_WAY_HANDSHAKE \n");
    else if (wifi_event_status == ATBM_P2P_WSC_EXCHAGE)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_WSC_EXCHAGE \n");
    else if (wifi_event_status == ATBM_P2P_GO_NEGOTIATION_CONFIRM)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_GO_NEGOTIATION_CONFIRM \n");
    else if (wifi_event_status == ATBM_P2P_GO_NEGOTIATION_ERROR)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_GO_NEGOTIATION_CONFIRM \n");
    else
        printf("WifiMgr_P2P_Get_Status UNKNOWN \n");
#endif

    return wifi_event_status;
}

bool WifiMgr_P2P_IsConnected(void)
{
#if 1
    if (wifi_event_status == ATBM_P20_NOT_INITIALIZED)
        printf("WifiMgr_P2P_Get_Status ATBM_P20_NOT_INITIALIZED \n");
    else if (wifi_event_status == ATBM_P2P_CONNECTED_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_CONNECTED_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_DISCONECT_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_DISCONECT_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_FIND_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_FIND_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_STOP_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_STOP_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_START_EVENT)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_START_EVENT \n");
    else if (wifi_event_status == ATBM_P2P_4_WAY_HANDSHAKE)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_4_WAY_HANDSHAKE \n");
    else if (wifi_event_status == ATBM_P2P_WSC_EXCHAGE)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_WSC_EXCHAGE \n");
    else if (wifi_event_status == ATBM_P2P_GO_NEGOTIATION_CONFIRM)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_GO_NEGOTIATION_CONFIRM \n");
    else if (wifi_event_status == ATBM_P2P_GO_NEGOTIATION_ERROR)
        printf("WifiMgr_P2P_Get_Status ATBM_P2P_GO_NEGOTIATION_CONFIRM \n");
    else
        printf("WifiMgr_P2P_Get_Status UNKNOWN \n");
#endif

    if(wifi_event_status == ATBM_P2P_CONNECTED_EVENT)
        return true;

    return false;
}

bool WifiMgr_P2P_IsDisconnected(void)
{
    if(wifi_event_status == ATBM_P2P_STOP_EVENT)
        return true;

    return false;
}

void WifiMgr_P2P_SetPhoneIP(uint32_t addr)
{
    PhoneIP = addr;
}

uint32_t WifiMgr_P2P_GetPhoneIP(void)
{
    return PhoneIP;
}

uint32_t WifiMgr_P2P_GetIteIP(void)
{
    return WifiMgr_Get_WIFI_IP();
}

uint32_t WifiMgr_P2P_GC_Start(void)
{
    atbm_wifi_p2p_start();
    atbm_wifi_p2p_find_accept(7);

    return 0;
}

uint32_t WifiMgr_P2P_GO_Start(void)
{
    atbm_wifi_p2p_start();
    atbm_wifi_p2p_go_start();

    return 0;
}


uint32_t WifiMgr_P2P_Stop(void)
{
    atbm_wifi_p2p_find_stop(0);
    atbm_wifi_p2p_stop();

    return 0;
}

unsigned int WifiMgr_P2P_SetPhoneIP_Is_P2PConnected(unsigned char *dev_addr)
{


}

unsigned int WifiMgr_P2P_SetDevice_Mapping(uint32_t addr, uint32_t p2pDevice)
{

}

unsigned int WifiMgr_P2P_GetDevice_Mapping(uint32_t addr)
{


}





#endif
