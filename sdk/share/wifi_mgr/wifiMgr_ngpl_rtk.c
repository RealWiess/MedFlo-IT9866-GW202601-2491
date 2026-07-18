// Copyright (c) 1996-2023, ITE Tech. Inc., Ltd
#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
#include "wifiMgr.h"

extern struct netif xnetif[NET_IF_NUM];
extern int          gScanStart;
extern u8           auto_reconnect_running;

/* ==== Global Variables ==== */
static int                               gSoftAP_5G = 0;

static sem_t                             semConnectStart, semConnectStop;
static pthread_mutex_t                   mutexALWiFi;
static pthread_mutex_t                   mutexScan, mutexConnect, mutexDisconnect;


#if WIFIMGR_EXCUTE_WPS
static sem_t                             semWPSStart, semWPSStop;
static pthread_mutex_t                   mutexALWiFiWPS;
#endif

static sem_t                             semSwitchModeStart, semSwitchModeStop;


static WIFIMGR_CONNSTATE_E               wifi_conn_state    = WIFIMGR_CONNSTATE_STOP;
static WIFIMGR_WORKSTATE_E               wifi_work_state    = WIFIMGR_WORKSTATE_FAILED;
static WIFIMGR_ECODE_E                   wifi_conn_ecode    = WIFIMGR_ECODE_SET_DISCONNECT;

static WIFI_MGR_SCANAP_LIST              gWifiMgrApList[WIFI_SCAN_LIST_NUMBER] = {0};

static WIFI_MGR_SETTING                  gWifiMgrSetting    = {0}, gWifiSetting    = {0};

static struct net_device_info            gScanApInfo        = {0};

static struct timeval                   tvDHCP1 = {0, 0}, tvDHCP2 = {0, 0}, tv3_temp = {0, 0};

// PhoneIP is the source side of Miracast
static uint32_t                         PhoneIP = 0x0;
static int gStaConnectProcess =0;

/* ==== WifiMgr flags initialization ==== */
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
    .Cancel_Connect         = false,
    .Cancel_WPS             = false,
    .Is_WIFI_Available      = false,
    .Is_Temp_Disconnect     = false,
    .IS_WifiMgr_Sta_Thread  = false,
    .IS_WifiMgr_Switch_Mode_Thread  = false,
    .IS_WifiMgr_Main_Process_Thread = false
};

extern int wifi_set_scan_reorderchan(__u8 * channel_list, __u8 length);
extern int wifi_set_pscan_chan(__u8 * channel_list,__u8 * pscan_config, __u8 length);

/* ================================================= */
/*                                                   */
/* Static Functions                                  */
/*                                                   */
/* ================================================= */

//Check some negative flags for terminating pthread
static int
_WifiMgr_Negative_Condition(void)
{
    int set_break = 0;

    if (gWifiMgrVar.Cancel_Connect   || \
        gWifiMgrVar.WIFI_Terminate   || \
        !gWifiMgrVar.WIFI_Init_Ready )
        set_break = 1;

    return set_break;
}

void
_WifiMgr_Sta_Set_Ecode(WIFIMGR_ECODE_E ecode)
{
    pthread_mutex_lock(&mutexConnect);
    if (gWifiMgrVar.WIFI_Init_Ready)
        wifi_conn_ecode = ecode;
    else
        wifi_conn_ecode = WIFIMGR_ECODE_NOT_INIT;
    pthread_mutex_unlock(&mutexConnect);
}

void
_WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_E conn_state)
{
    pthread_mutex_lock(&mutexConnect);
    if (gWifiMgrVar.WIFI_Init_Ready)
        wifi_conn_state = conn_state;
    else
        wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    pthread_mutex_unlock(&mutexConnect);
}


static void
_WifiMgr_Sta_Set_MP_Mode(char *ssid, char *password, rtw_security_t security)
{
#ifdef CFG_NET_WIFI_MP_MODE
    if (gWifiMgrVar.MP_Mode) {
        MGR_INFO("Is MP mode, connect to default SSID.\r\n");
        // SSID
        snprintf(ssid,  sizeof(gWifiMgrVar.Ssid),   "%s", CFG_NET_WIFI_MP_SSID);
        // Password
        snprintf(password,  RTW_MAX_PSK_LEN,    "%s", CFG_NET_WIFI_MP_PASSWORD);
#ifdef DTMF_DEC_HAS_SECUMODE
        // Security mode
        snprintf(security,  sizeof(gWifiMgrVar.SecurityMode),   "%s", CFG_NET_WIFI_MP_SECURITY);
#endif
    }
#endif
}

static WIFIMGR_CONNSTATE_E
_WifiMgr_Sta_Connect_Process_Dhcp(ITPEthernetSetting setting)
{
    WIFIMGR_CONNSTATE_E rc;
    u32_t           DHCP_COUNT = WIFI_CONNECT_DHCP_COUNT;
    bool            DHCP_DONE = false;
    err_t           DHCP_ERR;
    rtw_security_t  SEC = gWifiMgrVar.SecurityMode;

    if (_WifiMgr_Negative_Condition())
        return WIFIMGR_CONNSTATE_STOP;

    // Wait for connecting...
    MGR_TRACE("Wait for connecting, DHCP %s\n", setting.dhcp ? "Enable":"Disable");
    if (setting.dhcp) {
        // Wait for DHCP setting...
        MGR_INFO("Wait for DHCP setting");

        ip_addr_set_zero(&xnetif[0].ip_addr);
#ifdef CFG_NET_LWIP_2
        if (!netif_is_up(&xnetif[0])) {
            netif_set_up(&xnetif[0]);
        }
#endif

        DHCP_ERR = dhcp_start(&xnetif[0]);
        if (DHCP_ERR != ERR_OK) {
            MGR_ERROR(" Start DHCP negotiation failed!\r\n");
            return WIFIMGR_CONNSTATE_DHCP_FAILED;
        }

        while (DHCP_COUNT) {
        	u8_t *ip = LwIP_GetIP(&xnetif[0]);

            if (ip[0]!=0) {
                MGR_INFO("\r\nDHCP setting OK\r\n");
                BOOLEAN_SET_TRUE(DHCP_DONE);
                BOOLEAN_SET_FALSE(gWifiMgrVar.Pre_Scan);
                break;
            }
            putchar('.');
            fflush(stdout);
            DHCP_COUNT--;
            if (DHCP_COUNT == 0) {
                /* DHCP timeout */
                MGR_ERROR("DHCP timeout, connection is failed!\r\n");
                //WifiMgr_Sta_Cancel_Connect();
                break;
            }

            if (_WifiMgr_Negative_Condition() > 0)
                break;

            usleep(100*1000);
        }
    } else {
        MGR_INFO("Manual setting IP\n");
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);
        DHCP_DONE = true;
        BOOLEAN_SET_TRUE(gWifiMgrVar.Is_WIFI_Available);
        ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_GET_INFO, NULL);
    }

    if (_WifiMgr_Negative_Condition() > 0) {
        rc = WIFIMGR_CONNSTATE_STOP;
        MGR_ERROR("Cancel connection point 6\n");

        return rc;
    }

    if (DHCP_DONE) {
        MGR_INFO("wifi_set_autoreconnect \n");
        wifi_set_autoreconnect(2);

        // Wait for WiFi availability
        int timeout = 1000; // Timeout in milliseconds
        while (timeout > 0 && !ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_IS_AVAIL, NULL)){
            usleep(100*1000);
            timeout -= 100;
        }

        if (timeout <= 0) {
            MGR_ERROR("WiFi availability timeout!\r\n");
            return WIFIMGR_CONNSTATE_DHCP_FAILED;
        }

        BOOLEAN_SET_TRUE(gWifiMgrVar.Is_WIFI_Available);

        ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_GET_INFO, NULL);

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);

        rc = WIFIMGR_CONNSTATE_DHCP_DONE;
    } else {
        MGR_ERROR("DHCP phase failed!\r\n");
        rc = WIFIMGR_CONNSTATE_DHCP_FAILED;
    }

    return rc;
}

static WIFIMGR_CONNSTATE_E
_WifiMgr_Sta_Connect_Process_Link(
    char                *ssid,
	char                *password,
	rtw_security_t	    type,
	bool                match )
{
    if (ssid == NULL || password == NULL) {
        return WIFIMGR_CONNSTATE_STOP;
    }

    int                 ssid_len = strlen((const char *)ssid);
    int                 password_len = strlen((const char *)password);
    int 				key_id = 0;
    void				*semaphore = NULL;
    rtw_result_t        result = RTW_PENDING;
#if (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)
    u8 num_channel = 22;
    u8 channel_reorder[]= {36,40,44,48,149,153,157,161,165,1,6,11,2,3,4,5,7,8,9,10,12,13};//set channel order
#elif (defined CFG_NET_WIFI_SDIO_NGPL_8723DS)
    u8 num_channel = 13;
    u8 channel_reorder[]= {1,6,11,2,3,4,5,7,8,9,10,12,13};//set channel order    
#else

#endif

    if ( match ) {
    #if (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS) || (defined CFG_NET_WIFI_SDIO_NGPL_8723DS)
        wifi_set_scan_reorderchan(channel_reorder, num_channel);
    #else

    #endif

        result = wifi_connect(ssid,
                    type,
                    password,
                    ssid_len,
                    password_len,
                    key_id,
                    semaphore);
#if (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS) || (defined CFG_NET_WIFI_SDIO_NGPL_8723DS)

#else
        if (result != RTW_SUCCESS)
            WifiMgr_Sta_Disconnect();
#endif

        if (_WifiMgr_Negative_Condition() > 0) {
            _WifiMgr_Sta_Set_Ecode(WIFIMGR_ECODE_CONNECT_ERROR);
            MGR_ERROR("Cancel connection point 4\n");

            return WIFIMGR_CONNSTATE_STOP;
        }


        /* Connection Retry Handler */
        switch ( result ) {
            case RTW_SUCCESS:
                /* Connect OK!! Reset count and go to DHCP phase */
                MGR_INFO("wifi_connect results: success\n");
                break;

            case RTW_TIMEOUT:
                /* If SSID can be found but connection was TIMEOUT, try SCAN and CONNECT again. */
                MGR_ERROR("Join bss timeout, maybe signal were too weak\n");
                BOOLEAN_SET_FALSE(gWifiMgrVar.Pre_Scan); //Renew the scan list,  avoid security changing suddenly.
                _WifiMgr_Sta_Set_Ecode(WIFIMGR_ECODE_CONNECT_ERROR);
                return WIFIMGR_CONNSTATE_RETRY_SCAN;

            case RTW_ERROR:
                /* If SSID can be found but connection was ERROR, try SCAN and CONNECT again. */
                MGR_ERROR("Maybe it were AP issues(AP power off/Dirty channel/Error PW/...etc)\n");
                BOOLEAN_SET_FALSE(gWifiMgrVar.Pre_Scan); //Renew the scan list,  avoid security changing suddenly.
                _WifiMgr_Sta_Set_Ecode(WIFIMGR_ECODE_CONNECT_ERROR);
                return WIFIMGR_CONNSTATE_RETRY_SCAN;

            default:
                /* Another connection error code, just try CONNECT again */
                MGR_ERROR("wifi connect have another errors - RTW Error Code(%d)\n", result);
                _WifiMgr_Sta_Set_Ecode(WIFIMGR_ECODE_CONNECT_ERROR);
                return WIFIMGR_CONNSTATE_RETRY_CONNECT;

        }

    } else {

        MGR_INFO("SSID is NOT match, don't do connection process\n");

        /* SSID can NOT be found, you can handle something if you want(ex: goto retry_scan).  */
        if (!WIFIMGR_RECONNTECT_INFINITE) {
            /* If SSID can NOT be found, try to scan again. */
            BOOLEAN_SET_FALSE(gWifiMgrVar.Pre_Scan); //Renew the scan list,  avoid security changing suddenly.
            MGR_ERROR("SSID can NOT be found\n");

            return WIFIMGR_CONNSTATE_RETRY_SCAN;
        } else {
            /* Enforce to connect to hidden AP */
            result = wifi_connect(ssid,
    			type,
    			password,
    			ssid_len,
    			password_len,
    			key_id,
    			semaphore);

            if (result != RTW_SUCCESS) {
                MGR_ERROR("Enforce to connect to hidden AP failed, quit this SSID(%s)\n", ssid);
                /* Give up..., Reset count and go to DHCP phase */
                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);

                return WIFIMGR_CONNSTATE_STOP;
            }

        }

    }

    /* Connect phase complete */
    return WIFIMGR_CONNSTATE_CONNECT_DONE;
}

// sta connect to ap
static WIFIMGR_ECODE_E
_WifiMgr_Sta_Connect_Process(void)
{
    WIFIMGR_CONNSTATE_E sRet = WIFIMGR_CONNSTATE_STOP;
    WIFIMGR_ECODE_E eRet = WIFIMGR_ECODE_OK;
    int nAPCount = 0;
    bool MATCH = false;
    u32_t RECONNECT = WIFIMGR_RECONNTECT_TIME, REOFFER = WIFIMGR_REOFFER_DHCP_TIME;
    WIFI_MGR_SCANAP_LIST pList[64];
#if (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS) || (defined CFG_NET_WIFI_SDIO_NGPL_8723DS)

#else
        u8 num_channel = 3;
        u8 channel_list[3] = {1,6,11};
        u8 pscan_config[3] = {PSCAN_ENABLE,PSCAN_ENABLE,PSCAN_ENABLE};
#endif

	gStaConnectProcess = 1;
    if (_WifiMgr_Negative_Condition() > 0) {
        eRet = WIFIMGR_ECODE_CONNECT_ERROR;
        MGR_ERROR("Cancel connection point 1\n");

        goto _END;
    }

    u8_t *ssid = gWifiMgrVar.Ssid;
    u8_t *password = gWifiMgrVar.Password;
    rtw_security_t security_type = gWifiMgrVar.SecurityMode;

    if (strlen(ssid) == 0) {
        eRet = WIFIMGR_ECODE_NO_SSID;
        MGR_ERROR("Wifi setting has no SSID\r\n");
        goto _END;
    }

    _WifiMgr_Sta_Set_MP_Mode(ssid, password, security_type);


_SCAN:

    if ( _WifiMgr_Negative_Condition() > 0 ) {
        eRet = WIFIMGR_ECODE_CONNECT_ERROR;
        MGR_ERROR("Cancel connection point 2\n");

        goto _END;
    }

    if ( RECONNECT > 0 )
        usleep(100*1000);

    /* === Connection STEP 1: Get AP list in currently === */
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_SCANNING);
    if(gWifiMgrVar.Pre_Scan) {
        /* Select SSID after user do scanning on touch panel */
        memcpy(pList, gWifiMgrApList, sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
        nAPCount = gScanApInfo.apCnt;
    } else {
        /* Get new SSID list while the Wifimgr want to reconnect old SSID */
        #if (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS) || (defined CFG_NET_WIFI_SDIO_NGPL_8723DS)

        #else
        if (strcmp(gWifiMgrVar.Ssid, "") != 0) {
            nAPCount = WifiMgr_Get_Scan_AP_Info(pList);
        } else {
            eRet = WIFIMGR_ECODE_NO_SSID;
            goto _END;
        }
        #endif
    }
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_SCAN_DONE);


    /* === Connection STEP 2: Find out the SSID you want to connect === */
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_SETTING);
    #if (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS) || (defined CFG_NET_WIFI_SDIO_NGPL_8723DS)
        MATCH = true;
        security_type = RTW_SECURITY_WPA2_AES_PSK;
    #else
    for (int i = 0; i < nAPCount; i++) {
		/* Find the match SSID */
        if (pList[i].ssidName != NULL && strcmp(ssid, pList[i].ssidName) == 0) {
            MATCH = true;
            MGR_TRACE("Wanna connect to [%s], list is matched [%s(0x%X)] channel[%d]\n",
                ssid, pList[i].ssidName, pList[i].securityMode, pList[i].channelId);

            channel_list[1] = pList[i].channelId;
            wifi_set_pscan_chan(channel_list, pscan_config, num_channel);
            wifi_set_autoreconnect(0);

            if (pList[i].securityMode != 0){
                security_type = pList[i].securityMode;
            } else {
                security_type = RTW_SECURITY_OPEN;
            }
            gWifiMgrVar.SecurityMode = security_type ;

            break;
        }
    }
    #endif
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_SET_DONE);


_CONNECT:

    if ( _WifiMgr_Negative_Condition() > 0 ) {
        eRet = WIFIMGR_ECODE_CONNECT_ERROR;
        MGR_ERROR("Cancel connection point 3\n");

        goto _END;
    }

    /* === Connection STEP 3: Link to AP if the SSID is matched. === */
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_CONNECTING);
    sRet = _WifiMgr_Sta_Connect_Process_Link(ssid, password, security_type, MATCH);
    _WifiMgr_Sta_Set_Connect_State(sRet);

    if ( sRet == WIFIMGR_CONNSTATE_CONNECT_DONE)
       goto _DHCP;

    if ( RECONNECT > 0) {
        if (sRet == WIFIMGR_CONNSTATE_RETRY_SCAN) {
            BOOLEAN_SET_FALSE(gWifiMgrVar.Pre_Scan);
            MGR_ERROR("AP Linking timeout!! Try to Re-scan and Re-connect[%u]\n", RECONNECT);
            RECONNECT--;
            goto _SCAN;
        } else if (sRet == WIFIMGR_CONNSTATE_RETRY_CONNECT) {
            MGR_ERROR("AP Linking timeout!! Try to Re-connect[%u]\n", RECONNECT);
            RECONNECT--;
            goto _CONNECT;
            }
        } else {
            /* Retry to scan or connect when it over times */
            if (WIFIMGR_RECONNTECT_INFINITE) {
                BOOLEAN_SET_FALSE(gWifiMgrVar.Pre_Scan); //Renew the scan list,  avoid security changing suddenly.
                goto _SCAN;
        }

        eRet = WIFIMGR_ECODE_CONNECT_ERROR;
        _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_CONNECT_FAILED);
            MGR_ERROR("wifi connect retry failed, quit this SSID(%s)\n", ssid);
        goto _END;
        }


_DHCP:

    if (_WifiMgr_Negative_Condition() > 0) {
        eRet = WIFIMGR_ECODE_CONNECT_ERROR;
        MGR_ERROR("Cancel connection point 5\n");

        goto _END;
    }

    /*  DHCP phase */
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_DHCP_PROCESSING);
    sRet = _WifiMgr_Sta_Connect_Process_Dhcp(gWifiMgrSetting.setting);
    _WifiMgr_Sta_Set_Connect_State(sRet);
    if (sRet == WIFIMGR_CONNSTATE_DHCP_FAILED && REOFFER > 0) {
        MGR_ERROR("Connect process will retry %d times).\r\n", REOFFER);
        REOFFER--;

        goto _CONNECT;
    } else if (sRet == WIFIMGR_CONNSTATE_DHCP_FAILED && REOFFER == 0) {
        eRet = WIFIMGR_ECODE_DHCP_ERROR;
        goto _END;
    }

    // start dhcp count
    gettimeofday(&tvDHCP1, NULL);


_END:

    /* Set the connection state from the different return value */
    if (eRet == WIFIMGR_ECODE_OK) {
        _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_DONE);
    } else if (eRet == WIFIMGR_ECODE_NO_SSID) {
        _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_SCAN_FAILED);
    } else if (eRet == WIFIMGR_ECODE_CONNECT_ERROR) {
        _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_CONNECT_FAILED);
        if (gWifiMgrSetting.wifiCallback) {
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);
        }
    } else if (eRet == WIFIMGR_ECODE_DHCP_ERROR) {
        _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_DHCP_FAILED);
    }

    if (gWifiMgrVar.Cancel_Connect) {
        MGR_INFO("Cancel_Connect is set.\r\n");

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_CANCEL);
    }

	/* Sava last time connect info otherwise WIFI can't reconnect for wake up */
    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_SAVE_INFO);

    /* Handle sem about switching mode */
    {
        if (WifiMgr_Get_Sta_Connect_Complete() > 0)
            MGR_INFO(">>>>>>>>>> [Now in STA(Connect done)]\n");
        else
            MGR_INFO(">>>>>>>>>> [Now in STA(Connect cancel)]\n");

        int val;
        sem_getvalue(&semSwitchModeStop, &val);
        if (val < 1) {
            sem_post(&semSwitchModeStop); /* SW Done post point_1 */
            MGR_INFO("Post sem[Switch Mode Stop] in point_1\n");
        }
    }

	gStaConnectProcess = 0;
    return eRet;
}


static int
_WifiMgr_Sta_Connect_Pre(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready)
        return WIFIMGR_ECODE_NOT_INIT;

    pthread_mutex_lock(&mutexConnect);
    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        BOOLEAN_SET_FALSE(gWifiMgrVar.Need_Set);
        sem_post(&semConnectStart);
    }
    pthread_mutex_unlock(&mutexConnect);

    return nRet;
}


static inline int
_WifiMgr_Sta_Compare_AP_MAC(int list_1, int list_2)
{
    if (gWifiMgrApList[list_1].apMacAddr != NULL && gWifiMgrApList[list_2].apMacAddr != NULL) {
        for (int i = 0; i < 6; i++) {
            if (gWifiMgrApList[list_1].apMacAddr[i] != gWifiMgrApList[list_2].apMacAddr[i]) {
                return 0;
            }
        }
        return 1;
    } else {
        return -1;
    }
}


static inline void
_WifiMgr_Sta_Entry_Info(const char* ssid, const char* password, const rtw_security_t secumode)
{

    if (ssid){
        // SSID
        if(strlen(ssid) < WIFI_SSID_MAXLEN + 1){
            strlcpy(gWifiMgrVar.Ssid, ssid, strlen(ssid) + 1);
        } else {
            MGR_ERROR("Can't copy ssid\n");
        }
    }

    if (password){
        // Password
        if(strlen(password) < WIFI_PASSWORD_MAXLEN + 1){
            strlcpy(gWifiMgrVar.Password, password, strlen(password) + 1);
        } else {
            MGR_ERROR("Can't copy password\n");
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

    memset(gWifiMgrApList, 0,                  sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
    memcpy(gWifiMgrApList, gWifiMgrTempApList, sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
    return j;
}


static void
_WifiMgr_Sta_Scan_Result_Print( rtw_scan_result_t* record )
{
    MGR_INFO( "%s\n ", __FUNCTION__);
    MGR_TRACE( "%s\t ", ( record->bss_type == RTW_BSS_TYPE_ADHOC ) ? "Adhoc" : "Infra" );
    // MGR_TRACE( MAC_FMT, MAC_ARG(record->BSSID.octet) );
    MGR_TRACE( " %d\t ", record->signal_strength );
    MGR_TRACE( " %d\t  ", record->channel );
    MGR_TRACE( " %d\t  ", record->wps_type );
    MGR_TRACE( " %s\t\t ", ( record->security == RTW_SECURITY_OPEN ) ? "Open" :
        (record->security == RTW_SECURITY_WEP_PSK ) ? "WEP" :
        (record->security == RTW_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
        (record->security == RTW_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
        (record->security == RTW_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
        (record->security == RTW_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
        (record->security == RTW_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
        (record->security == RTW_SECURITY_WPA_WPA2_MIXED_PSK ) ? "WPA/WPA2 PSK" :
        (record->security == RTW_SECURITY_WPA_TKIP_8021X ) ? "WPA TKIP 8021X" :
        (record->security == RTW_SECURITY_WPA_AES_8021X ) ? "WPA AES 8021X" :
        (record->security == RTW_SECURITY_WPA2_AES_8021X ) ? "WPA2 AES 8021X" :
        (record->security == RTW_SECURITY_WPA2_TKIP_8021X ) ? "WPA2 TKIP 8021X" :
        (record->security == RTW_SECURITY_WPA_WPA2_MIXED_8021X ) ? "WPA/WPA2 8021X" : "Unknown");

    MGR_TRACE( " %s ", record->SSID.val );
    MGR_INFO( "\r\n" );
}


static rtw_result_t
_WifiMgr_Sta_Scan_Result_Handler( rtw_scan_handler_result_t* malloced_scan_result )
{
	unsigned char convert_quant;
    void *_user_data;
    struct net_device_info *apInfo = NULL;

    //	netDeviceInfo->apList[i].rfQualityRSSI = iwEvt.u.qual.level;
    //MGR_TRACE("====>%s: scan_complete(%d)\n", __FUNCTION__, malloced_scan_result->scan_complete);

    if (malloced_scan_result->scan_complete != RTW_TRUE) {
        rtw_scan_result_t* record = &malloced_scan_result->ap_details;
        record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */

        _user_data = malloced_scan_result->user_data;
        apInfo = (struct net_device_info *)_user_data;

		/* SSID */
        if (record->SSID.len > 0 && record->SSID.len <= WIFI_SSID_MAXLEN) {
            memcpy(apInfo->apList[apInfo->apCnt].ssidName, record->SSID.val ,WIFI_SSID_MAXLEN);
        }

		/* MAC */
		memcpy(apInfo->apList[apInfo->apCnt].apMacAddr, record->BSSID.octet, 6*sizeof(unsigned char));

		/* Power Level in dBm*/
        if (record->signal_strength < 0)
            apInfo->apList[apInfo->apCnt].rfQualityRSSI =  record->signal_strength;

		/* Signal Quality */
		if (record->signal_strength <= -100)
			convert_quant = 0;
		else if(record->signal_strength >= -30)
			convert_quant = 100;
		else
			convert_quant = 150 + 1.67*(record->signal_strength);

		apInfo->apList[apInfo->apCnt].rfQualityQuant = convert_quant;

		/* Security */
        apInfo->apList[apInfo->apCnt].securityMode = record->security; //unsigned long

		/* Channel */
        apInfo->apList[apInfo->apCnt].channelId = record->channel ;

        MGR_INFO("No %d: %s ["MAC_FMT"] ch %d\n",
            apInfo->apCnt, apInfo->apList[apInfo->apCnt].ssidName,
            MAC_ARG(apInfo->apList[apInfo->apCnt].apMacAddr),apInfo->apList[apInfo->apCnt].channelId );
        apInfo->apCnt ++;
        //RTW_API_INFO( "%d\t ", ++ApNum );
        //_WifiMgr_Sta_Scan_Result_Print(record);
    } else{
        MGR_INFO("====>scan complete\n");
    }
    BOOLEAN_SET_FALSE(gWifiMgrVar.Start_Scan);
    return RTW_SUCCESS;
}


static int
_WifiMgr_Sta_Scan_Process(struct net_device_info *apInfo)
{
    int nRet = 0, scan_result;
    int i = 0;
    int nHideSsid = 0;
    bool scanning = false;
    if (!gWifiMgrVar.WIFI_Init_Ready) {
        MGR_ERROR("!WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (gWifiMgrVar.WIFI_Terminate) {
        MGR_ERROR("WIFI_Terminate \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    BOOLEAN_SET_TRUE(gWifiMgrVar.Start_Scan);

    //printf("disable autoconnect %d \n",auto_reconnect_running);
    wifi_set_autoreconnect(0);

    usleep(200*1000);
    memset(apInfo, 0, sizeof(struct net_device_info));

    MGR_INFO("Start to SCAN AP ==========================\r\n");

	scan_result = wifi_scan_networks(_WifiMgr_Sta_Scan_Result_Handler, apInfo);

    if(scan_result == RTW_SUCCESS){
		BOOLEAN_SET_TRUE(gWifiMgrVar.Pre_Scan);
	}else{
        MGR_ERROR("wifi scan failed, result code(%d).\n", scan_result);
        return WIFIMGR_ECODE_NOT_INIT;
    }

    while (1)
    {
        scanning = gWifiMgrVar.Start_Scan;
        //MGR_TRACE("[Presentation]%s() %s\r\n", __FUNCTION__, (scanning == true) ? "Now scanning":"Scan done");
        if (scanning == false)
        {
            // scan finish
            MGR_INFO("Scan AP Finish!\r\n");
            break;
        }

        if (gScanStart == 0)
        {
            // scan finish
            MGR_INFO("Scan AP Finish(wifi_scan_done_hdl)\r\n");
            break;
        }

        usleep(100 * 1000);
    }

    MGR_INFO("AP Cnt = %ld\r\n", apInfo->apCnt);

    for (i = 0; i < apInfo->apCnt; i++)
    {
        unsigned int ssid_len = strlen(apInfo->apList[i].ssidName);
        /* Avoid the SSID length is shorter than 32, and the RSSI is less than 0. */
        if (ssid_len > 0 && ssid_len < 33 && apInfo->apList[i].rfQualityRSSI < 0)
        {
            gWifiMgrApList[i].channelId = apInfo->apList[i].channelId;
            gWifiMgrApList[i].operationMode = apInfo->apList[i].operationMode ;
            gWifiMgrApList[i].rfQualityQuant = apInfo->apList[i].rfQualityQuant;
            gWifiMgrApList[i].rfQualityRSSI = apInfo->apList[i].rfQualityRSSI;
            gWifiMgrApList[i].securityMode = apInfo->apList[i].securityMode;
            /* For security */
            memcpy(gWifiMgrApList[i].apMacAddr, apInfo->apList[i].apMacAddr, 6);
            memcpy(gWifiMgrApList[i].ssidName,  apInfo->apList[i].ssidName, 32);
        } else {
            nHideSsid ++;
        }
    }

#if WIFIMGR_SHOW_SCAN_LIST
    apInfo->apCnt = apInfo->apCnt - nHideSsid;

    _WifiMgr_Sta_List_Sort_Insert(apInfo->apCnt);

    apInfo->apCnt = _WifiMgr_Sta_Remove_Same_SSID(apInfo->apCnt);

    for (i = 0; i < apInfo->apCnt; i++)
    {
        MGR_INFO("SSID = %32s, securityMode =  %16s, avgQuant = %4d %%, power = %4d dBm , <%02X:%02X:%02X:%02X:%02X:%02X>\r\n",
			gWifiMgrApList[i].ssidName,
			//gWifiMgrApList[i].securityMode,
			((gWifiMgrApList[i].securityMode == RTW_SECURITY_OPEN ) ? "Open" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WEP_PSK ) ? "WEP" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_WPA2_MIXED_PSK ) ? "WPA/WPA2 PSK" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_TKIP_8021X ) ? "WPA TKIP 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_AES_8021X ) ? "WPA AES 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_AES_8021X ) ? "WPA2 AES 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_TKIP_8021X ) ? "WPA2 TKIP 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_WPA2_MIXED_8021X ) ? "WPA/WPA2 8021X" : "Unknown"),
			gWifiMgrApList[i].rfQualityQuant, gWifiMgrApList[i].rfQualityRSSI,
        	gWifiMgrApList[i].apMacAddr[0], gWifiMgrApList[i].apMacAddr[1], gWifiMgrApList[i].apMacAddr[2],
        	gWifiMgrApList[i].apMacAddr[3], gWifiMgrApList[i].apMacAddr[4], gWifiMgrApList[i].apMacAddr[5]);
    }
#endif
    //MGR_INFO("wifi_is_connected_to_ap %d \n",wifi_is_connected_to_ap());
    if (wifi_is_connected_to_ap() ==0){
        wifi_set_autoreconnect(2);
    }
    MGR_INFO("End to SCAN AP ============================\r\n");
    return apInfo->apCnt;
}

static int
_WifiMgr_Sta_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    int val;

    BOOLEAN_SET_TRUE(gWifiMgrVar.Cancel_WPS);

    if (gWifiMgrVar.Cancel_Connect == false)
        WifiMgr_Sta_Cancel_Connect();

    //In order to terminate [_WifiMgr_Sta_Thread]
    sem_getvalue(&semConnectStart, &val);
    if (val < 1)
        sem_post(&semConnectStart);

    //Clean connection info in Client mode
    if (gWifiMgrSetting.wifiCallback && (gWifiMgrVar.Client_On_Off == WIFIMGR_SWITCH_OFF))
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_CLEAN_INFO);

    return nRet;
}


static void*
_WifiMgr_Sta_Thread(void* arg)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP || WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_P2P) {
        goto end;
    }

    BOOLEAN_SET_TRUE(gWifiMgrVar.IS_WifiMgr_Sta_Thread);

    while (1) {
        sem_wait(&semConnectStart);

        if (gWifiMgrVar.WIFI_Terminate) {
            MGR_ERROR("Terminate [_WifiMgr_Sta_Thread] 1 \n");
            break;
        }

        MGR_INFO("[_WifiMgr_Sta_Thread]\n");

        if (gWifiMgrVar.Need_Set) {
            _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_SETTING);
            MGR_TRACE("START to Set!\r\n");
            wifi_conn_ecode = nRet = WIFIMGR_ECODE_OK;

            BOOLEAN_SET_FALSE(gWifiMgrVar.Need_Set);
            MGR_TRACE("Set finish!\r\n");
        }
        usleep(1000);

        if (strlen(gWifiMgrVar.Ssid) == 0) {
			nRet = WIFIMGR_ECODE_NO_SSID;
        } else {
			nRet = WIFIMGR_ECODE_OK;
        }

        if (nRet == WIFIMGR_ECODE_OK) {
            _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_CONNECTING);

            BOOLEAN_SET_TRUE(gWifiMgrVar.Cancel_WPS);
            wifi_conn_ecode = _WifiMgr_Sta_Connect_Process();
            BOOLEAN_SET_FALSE(gWifiMgrVar.Cancel_WPS);
        }

        if (gWifiMgrVar.WIFI_Terminate) {
            MGR_ERROR("Terminate [_WifiMgr_Sta_Thread] 2 \n");
            break;
        }

        _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_STOP);
        usleep(1000);
    }

end:
    MGR_INFO("Detach [_WifiMgr_Sta_Thread] pthread due to it's %s\n",
              (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP) ? "useless in AP mode" : "terminated in STA mode");

    if (sem_destroy(&semConnectStart) == 0) {
        int val;
        sem_getvalue(&semConnectStart, &val);
        if (val > 0) {
            MGR_INFO("semConnectStart still exists (%d)\n", val);
        } else {
            MGR_INFO("semConnectStart destroyed successfully (%d)\n", val);
        }
    }

    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_STOP);
    BOOLEAN_SET_FALSE(gWifiMgrVar.IS_WifiMgr_Sta_Thread);
    sem_post(&semConnectStop);

    return NULL;
}


static inline void
_WifiMgr_HostAP_Entry_Info(char* ssid, char* password)
{
    if (ssid){
        // SSID
        strlcpy(gWifiMgrVar.ApSsid, ssid, strlen(ssid) + 1);
    }
    if (password){
        // Password
        strlcpy(gWifiMgrVar.ApPassword, password, strlen(password) + 1);
    }
}


static int
_WifiMgr_HostAP_Init(void)
{
    int nRet = WIFIMGR_ECODE_NOT_INIT;
    int ssid_len, pw_len, ch;
    ITPWifiInfo wifiInfo;

    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, NULL);

    ssid_len = strlen(gWifiMgrVar.ApSsid);
    pw_len = strlen(gWifiMgrVar.ApPassword);
    ch       = 4; /* Set default channel */

    if (gWifiMgrVar.SoftAP_Hidden){
        //wifi_start_ap_with_hidden_ssid("AP_Mode",RTW_SECURITY_WPA2_AES_PSK,"12345678",7,8,4);
        nRet = wifi_start_ap_with_hidden_ssid(gWifiMgrVar.ApSsid,
            RTW_SECURITY_WPA2_AES_PSK,
            gWifiMgrVar.ApPassword,
            ssid_len, pw_len, ch);
    } else {
        //wifi_start_ap("AP_Mode",RTW_SECURITY_WPA2_AES_PSK,"12345678",7,8,4);
        if (pw_len >= 8) {
#if (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)
            if (gSoftAP_5G == 5){
                ch = 48;
                MGR_INFO("HostAP init for 5GHz\n");
            } else {
                MGR_INFO("HostAP init for 2.4GHz\n");
            }
#else
                MGR_INFO("HostAP init for 2.4GHz\n");
#endif

#if 0 //(defined CFG_NET_WIFI_SDIO_NGPL_8821CS)
                printf("Wifimgr, _WifiMgr_HostAP_Init , 8821 add wpa3 soft ap mode \n");
                wifi_set_mfp_support(1);
                nRet = wifi_start_ap(gWifiMgrVar.ApSsid,
                        RTW_SECURITY_WPA3_AES_PSK,
                        gWifiMgrVar.ApPassword,
                        ssid_len, pw_len, ch);
#else                

                nRet = wifi_start_ap(gWifiMgrVar.ApSsid,
                    RTW_SECURITY_WPA2_AES_PSK,
                    gWifiMgrVar.ApPassword,
                    ssid_len, pw_len, ch);
#endif
        } else if ((pw_len > 0) && (pw_len < 8)) {
            MGR_INFO("No Enough PW\n");
        } else {
            nRet = wifi_start_ap(gWifiMgrVar.ApSsid,
                RTW_SECURITY_OPEN,
                NULL,
                ssid_len, 0, ch);
        }
    }
    dhcps_init();

    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);

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
    BOOLEAN_SET_FALSE(gWifiMgrVar.SoftAP_Init_Ready);
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

    pthread_mutex_lock(&mutexALWiFiWPS);
    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        BOOLEAN_SET_TRUE(gWifiMgrVar.Need_Set);
        sem_post(&semWPSStart);
//        sem_post(&semConnectStart);
    }
    pthread_mutex_unlock(&mutexALWiFiWPS);

    return nRet;
}


static int
_WifiMgr_WPS_Init(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    struct net_device_config netCfg = {0};
    uint64_t connect_cnt = 0;
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
    MGR_INFO("[WPS] Wait for connecting");
    connect_cnt = WIFI_CONNECT_COUNT;
    while (connect_cnt)
    {
        putchar('.');
        fflush(stdout);
        connect_cnt--;
        if (connect_cnt == 0) {
            MGR_ERROR("\r\n[WPS] Timeout! Cannot connect to WIFI AP!\r\n");
            break;
        }

        if (gWifiMgrVar.Cancel_WPS) {
            goto end;
        }

        usleep(100000);
    }

    if (!is_connected) {
        MGR_ERROR("[WPS] Error! Cannot connect to WiFi AP!\r\n");
        nRet = WIFIMGR_ECODE_CONNECT_ERROR;
        goto end;
    }

    if (gWifiMgrVar.Cancel_WPS) {
        goto end;
    }

    // Wait for DHCP setting...
    MGR_INFO("[WPS] Wait for DHCP setting");
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_START_DHCP, NULL);
    connect_cnt = WIFI_CONNECT_COUNT;
    while (connect_cnt)
    {
        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_AVAIL, NULL)) {
            MGR_INFO("\r\n[WPS] DHCP setting OK\r\n");
            dhcp_available = 1;
            break;
        }
        putchar('.');
        fflush(stdout);
        connect_cnt--;
        if (connect_cnt == 0) {
            MGR_ERROR("\r\n[WPS] DHCP timeout! connect fail!\r\n");
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

        MGR_INFO("[WPS] WPS Info:\r\n");
        MGR_INFO("[WPS] WPS SSID     = %s\r\n", ssid);
        MGR_INFO("[WPS] WPS Password = %s\r\n", password);
        MGR_INFO("[WPS] WPS Security = %ld\r\n", wpsNetCfg.securitySuit.securityMode);
    }

    end:

    if (gWifiMgrVar.Cancel_WPS)
    {
        MGR_INFO("[WPS] gWifiMgrVar.Cancel_WPS is set.\r\n");
    }

    return nRet;
}


static void*
_WifiMgr_WPS_Thread(void* arg)
{
    int nRet = WIFIMGR_ECODE_OK;

    if ( WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP)
        goto end;

    while (1)
    {
        sem_wait(&semWPSStart);
        if (gWifiMgrVar.WIFI_Terminate) {
            MGR_INFO("terminate WifiMgr_WPS_ThreadFunc \n");
            break;
        }

        MGR_TRACE("START to Connect WPS!\r\n");
        wifi_conn_ecode = _WifiMgr_WPS_Init();
        MGR_TRACE("Connect WPS finish!\r\n");


        usleep(1000);
    }

end:
    MGR_ERROR("Detach [_WifiMgr_WPS_Thread] pthread\n");
    sem_destroy(&semWPSStart);
    sem_destroy(&semWPSStop);
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_STOP);

    return NULL;
}
#endif

static WIFIMGR_CONNSTATE_E
_WifiMgr_Main_Process_Station(WIFIMGR_CONNSTATE_E state)
{
    int nRet = 0;
    int nWiFiConnState = 0;
    int nWiFiConnEcode = 0;
    int32_t temp_disconn_time = 0;
    struct timeval tv1 = {0, 0}, tv2 = {0, 0};
    struct timeval tv4_temp = {0, 0};
    WIFIMGR_CONNSTATE_E gWifi_connstate = state;

    /* === Phase 1: Watch Connection Process State  ===*/
    if (gWifiMgrVar.Is_First_Connect) {
        // First time connect when the system start up
        nRet = _WifiMgr_Sta_Connect_Pre();
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
                MGR_ERROR("nWiFiConnEcode = %d\r\n", nWiFiConnEcode);

                /* If STA switch to AP mode and SSID is NULL */
                sem_post(&semSwitchModeStop); /* SW Done post point_3 */

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
    if ((ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) == 1) &&
        gWifiMgrVar.Is_Temp_Disconnect)
    {
        BOOLEAN_SET_FALSE(gWifiMgrVar.Is_Temp_Disconnect);
    }


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
            MGR_INFO("====>Send DHCP Discover!!!!!\n");
            MGR_INFO("DHCP renew %ld min \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2)/60/1000);
            ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RENEW_DHCP, NULL);
            gettimeofday(&tvDHCP1, NULL);
            gettimeofday(&tvDHCP2, NULL);
        } else {
            //printf("DHCP wait renew  %d \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2));
        }
    } else {
        if (gWifiMgrVar.Is_WIFI_Available) {
            if (!gWifiMgrVar.Is_Temp_Disconnect && nWiFiConnEcode == WIFIMGR_ECODE_OK)
            {
                // first time detect
                BOOLEAN_SET_TRUE(gWifiMgrVar.Is_Temp_Disconnect);
                gettimeofday(&tv3_temp, NULL);
                MGR_TRACE("WiFi temporary disconnected!%d %d\r\n", nWiFiConnState,nWiFiConnEcode);
                if (gWifiMgrSetting.wifiCallback)
                    gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT);
            } else if (nWiFiConnEcode == WIFIMGR_ECODE_OK) {
                gettimeofday(&tv4_temp, NULL);
                temp_disconn_time = itpTimevalDiff(&tv3_temp, &tv4_temp);
                MGR_TRACE("temp disconnect time = %d sec. %d %d\r\n", temp_disconn_time / 1000 , nWiFiConnState,nWiFiConnEcode);
                if (temp_disconn_time >= WIFIMGR_TEMPDISCONN_MSEC) {
                    MGR_TRACE("WiFi temporary disconnected over %ld sec. Services should be shut down.\r\n", temp_disconn_time / 1000);
                    BOOLEAN_SET_FALSE(gWifiMgrVar.Is_Temp_Disconnect);     // reset

                    if (gWifiMgrSetting.wifiCallback)
                        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S);

                    // prev is available, curr is not available
                    BOOLEAN_SET_FALSE(gWifiMgrVar.Is_WIFI_Available);
                }
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
                        //nRet = _WifiMgr_Sta_Connect_Pre();
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
        //int get_scan_count = 0;
        //WIFI_MGR_SCANAP_LIST pList[64];

        //get_scan_count = WifiMgr_Get_Scan_AP_Info(pList);
        //MGR_INFO("Scan count(%d)\n", get_scan_count);

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
    WIFIMGR_CONNSTATE_E gWifi_connstate = WIFIMGR_CONNSTATE_STOP;


    BOOLEAN_SET_TRUE(gWifiMgrVar.Is_First_Connect);
    BOOLEAN_SET_TRUE(gWifiMgrVar.IS_WifiMgr_Main_Process_Thread);
    BOOLEAN_SET_FALSE(gWifiMgrVar.Is_Temp_Disconnect);
    BOOLEAN_SET_FALSE(gWifiMgrVar.No_Config_File);
    BOOLEAN_SET_FALSE(gWifiMgrVar.No_SSID);

    usleep(20000);
    MGR_TRACE("\n");

    while (1)
    {
        nCheckCnt--;
        if (gWifiMgrVar.WIFI_Terminate) {
            MGR_INFO("terminate WifiMgr_Process_ThreadFunc \n");
            BOOLEAN_SET_FALSE(gWifiMgrVar.IS_WifiMgr_Main_Process_Thread);
            break;
        }

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


/* ============================================ */

static void
_WifiMgr_Switch_Mode_Init(void)
{
    int nRet;

    /* Set Semaphore */
    nRet = sem_init(&semSwitchModeStart, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semSwitchModeStart sem_init() fail!\r\n");
        sem_destroy(&semSwitchModeStart);
        nRet = WIFIMGR_ECODE_SEM_INIT;
        return;
    }

    nRet = sem_init(&semSwitchModeStop, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semSwitchModeStop sem_init() fail!\r\n");
        sem_destroy(&semSwitchModeStop);
        nRet = WIFIMGR_ECODE_SEM_INIT;
        return;
    }

    BOOLEAN_SET_TRUE(gWifiMgrVar.IS_WifiMgr_Switch_Mode_Thread);
}

static inline void
_WifiMgr_Switch_Mode_PwrSeq_Down(void)
{
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_EXIT, NULL);
    ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_EXIT, NULL);

#if defined(CFG_GPIO_SD_WIFI_POWER_ENABLE)

#if defined(CFG_BUILD_BLUETOOTH)
    MGR_INFO("Not support the Power Sequence down by using BT\n");
#else
    ithLockMutex(ithStorMutex);

    /* ==== PWR Sequence ==== */
    ithWIFICardPowerProcess();
    /* ================== */

    ithUnlockMutex (ithStorMutex);
#endif

#else
    MGR_INFO("Not support the Power Sequence down of WIFI\n");
#endif

}


static void*
_WifiMgr_Sta_HostAP_Switch_Thread(void *arg)
{
    int nTemp = -1;

    _WifiMgr_Switch_Mode_Init();

    for (;;) {
        if (sem_wait(&semSwitchModeStart) != 0) {
            // Handle semaphore wait error
            // e.g., log the error or exit the thread
            perror("sem_wait");
            return NULL;
        }

        // ==== Uninstall APP/Driver/IO ====
        if (!gWifiMgrVar.WIFI_Init_Ready) {
            return NULL;
        }

        gWifiMgrVar.WIFI_Mode = WifiMgr_Get_WIFI_Mode();
        MGR_INFO("================ SW START (%s Mode Now) ================>\n",
            (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_CLIENT) ? "Client" : "AP");

        WifiMgr_Terminate();
        usleep(200 * 1000);
#if (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)
		wifi_off();		

#else        
        _WifiMgr_Switch_Mode_PwrSeq_Down();

        /* ==== Install APP/Driver/IO ==== */
        if (ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL) != 0) {
            MGR_ERROR("ioctl\n");
            return NULL;
        }
#endif
        switch (WifiMgr_Get_WIFI_Mode()) {
            case WIFIMGR_MODE_CLIENT:
                /* === STA  -> AP === */
                MGR_TRACE("Terminate STA done, prepare to resume HostAP...\n");
                if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESUME, (void*)RTW_MODE_AP) != 0) {
                    MGR_ERROR("ioctl\n");
                    return NULL;
                }
                usleep(1000);
                if (gSoftAP_5G == 5)
                    nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 5, gWifiSetting);
                else
                    nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
                break;

            case WIFIMGR_MODE_SOFTAP:
                /* === AP -> STA === */
                MGR_TRACE("Terminate HostAP done, prepare to resume Client...\n");
                if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESUME, (void*)RTW_MODE_STA) != 0) {
                    MGR_ERROR("ioctl\n");
                    return NULL;
                }
                usleep(200 * 1000);
                nTemp = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
                break;

            case WIFIMGR_MODE_MAX:
                MGR_INFO("Not support concurrent mode\n");
                break;

            default:
                MGR_ERROR("Unknown mode\n");
        }

        if (gWifiMgrSetting.wifiCallback) {
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_SWITCH_CLIENT_SOFTAP_FINISH);
        }

        /* Wait connection set done */
        if (sem_wait(&semSwitchModeStop) != 0) {
            // Handle semaphore wait error
            // e.g., log the error or exit the thread
            perror("sem_wait");
            return NULL;
        }
        MGR_INFO("<================ SW DONE (%s Mode Now) ================\n\n\n",
            (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_CLIENT) ? "Client" : "AP");

        usleep(100 * 1000);
    }

    return NULL;
}

static int
_WifiMgr_Posix_Init(void)
{
    int nRet = WIFIMGR_ECODE_NOT_INIT;

    /* ====  Semaphore initialization ==== */
    nRet = sem_init(&semConnectStart, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semConnectStart sem_init() fail!\r\n");
        sem_destroy(&semConnectStart);
        nRet = WIFIMGR_ECODE_SEM_INIT;
        return nRet;
    }

    nRet = sem_init(&semConnectStop, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semConnectStop sem_init() fail!\r\n");
        sem_destroy(&semConnectStop);
        nRet = WIFIMGR_ECODE_SEM_INIT;
        return nRet;
    }

#if WIFIMGR_EXCUTE_WPS
    nRet = sem_init(&semWPSStart, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semWPSStart sem_init() fail!\r\n");
        sem_destroy(&semWPSStart);
        nRet = WIFIMGR_ECODE_SEM_INIT;
        return nRet;
    }

    nRet = sem_init(&semWPSStop, 0, 0);
    if (nRet == -1) {
        MGR_ERROR("semWPSStop sem_init() fail!\r\n");
        sem_destroy(&semWPSStop);
        nRet = WIFIMGR_ECODE_SEM_INIT;
        return nRet;
    }
#endif

    /* ====  Mutually Exclusive initialization ==== */
    nRet = pthread_mutex_init(&mutexALWiFi, NULL);
    if (nRet != 0) {
        MGR_ERROR("mutexALWiFi pthread_mutex_init() fail!\r\n");
        pthread_mutex_destroy(&mutexALWiFi);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        return nRet;
    }

    nRet = pthread_mutex_init(&mutexScan, NULL);
    if (nRet != 0) {
        MGR_ERROR("mutexScan pthread_mutex_init() fail!\r\n");
        pthread_mutex_destroy(&mutexScan);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        return nRet;
    }

    nRet = pthread_mutex_init(&mutexConnect, NULL);
        if (nRet != 0) {
            MGR_ERROR("mutexConnect pthread_mutex_init() fail!\r\n");
            pthread_mutex_destroy(&mutexConnect);
            nRet = WIFIMGR_ECODE_MUTEX_INIT;
            return nRet;
    }

    nRet = pthread_mutex_init(&mutexDisconnect, NULL);
    if (nRet != 0) {
        MGR_ERROR("mutexDisconnect pthread_mutex_init() fail!\r\n");
        pthread_mutex_destroy(&mutexDisconnect);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        return nRet;
    }
#if WIFIMGR_EXCUTE_WPS
    nRet = pthread_mutex_init(&mutexALWiFiWPS, NULL);
    if (nRet != 0) {
        MGR_ERROR("mutexALWiFiWPS pthread_mutex_init() fail!\r\n");
        pthread_mutex_destroy(&mutexALWiFiWPS);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        return nRet;
    }
#endif

    return WIFIMGR_ECODE_OK;
}

static void
_WifiMgr_Proc_Init(void)
{
    int nRet = WIFIMGR_ECODE_NOT_INIT;
    pthread_t          task;
    pthread_attr_t     attr;

    MGR_INFO("==== Create thread ==== \n");

    /* create STA mode tasklet thread */
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
        nRet = pthread_create(&task, &attr, _WifiMgr_Sta_Thread, NULL);
        if (nRet)
            MGR_ERROR(" create [STA mode] thread fail! 0x%08X \n", nRet );
    }

    /* create WifiMgr main process tasklet thread */
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
        nRet = pthread_create(&task, &attr, _WifiMgr_Main_Process_Thread, NULL);
        if (nRet)
            MGR_ERROR(" create [WifiMgr main process] thread fail! 0x%08X \n", nRet );
    }

    /* create Wifi mode switch tasklet thread */
    if (gWifiMgrVar.IS_WifiMgr_Switch_Mode_Thread == false) {/* Create task only first time */
            /* Not daemon thread, do not kill it. */
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
            nRet = pthread_create(&task, &attr, _WifiMgr_Sta_HostAP_Switch_Thread, NULL);
            if (nRet)
                MGR_ERROR(" create [Wifi mode switch] thread fail! 0x%08X \n", nRet );
    }
}

/* ============================================ */



static int
_WifiMgr_Enable_PowerSave(void)
{

    return wifi_enable_powersave();
}


static int
_WifiMgr_Disable_PowerSave(void)
{

    return wifi_disable_powersave();
}


static inline void
_WifiMgr_P2P_Entry_Info(char* p2p_name)
{
    if (p2p_name){
        // p2p
        snprintf(gWifiMgrVar.P2P_name, WIFI_SSID_MAXLEN, p2p_name);
    }
}


/* ================================================= */




/* ================================================= */
/*                                                   */
/* API Functions                                     */
/*                                                   */
/* ================================================= */


int
WifiMgr_Get_Connect_State(int *conn_state, int *e_code)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        *conn_state = WIFIMGR_CONNSTATE_STOP;
        *e_code = WIFIMGR_ECODE_NOT_INIT;
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexConnect);
    *conn_state = wifi_conn_state;
    *e_code = wifi_conn_ecode;
    pthread_mutex_unlock(&mutexConnect);

    return nRet;
}


/* Determine WifiMgr working states */
int
WifiMgr_Get_Sta_Work_State(WIFIMGR_WORKSTATE_E *work_state)
{
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_AVAIL, NULL)) {
        pthread_mutex_lock(&mutexALWiFi);
        if (gWifiMgrVar.WIFI_Init_Ready)
            *work_state = wifi_work_state;
        else
            *work_state = WIFIMGR_WORKSTATE_FAILED;
        pthread_mutex_unlock(&mutexALWiFi);
    }

    return 0;
}


int
WifiMgr_Get_Sta_Avalible(void)
{
    int rc;

    /* LWIP link detection  by SW/HW */
    rc = ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_AVAIL, NULL);
    return rc;
}

int
WifiMgr_Get_Sta_Connect_Complete(void)
{
    int rc;
/*
      Confirm 3 conditions of connection :
        1. SSID len > 0
        2. SW stack ready (DHCP OK)
        3. Already hook IP into netif
*/
    rc = ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL);
    return rc;
}


int
WifiMgr_Get_MAC_Address(uint8_t cMac[])
{
    if (cMac == NULL) {
        // Handle error: return an appropriate error code or take necessary action
        MGR_ERROR("Given MAC was null\n");
        return -1;
    }

    uint8_t *mac = WifiMgr_Get_WIFI_MAC();
    if (mac == NULL) {
        // Handle error: return an appropriate error code or take necessary action
        MGR_ERROR("WifiMgr_Get_WIFI_MAC() returned null\n");
        return -1;
    }

    size_t len = ((&mac[NETIF_MAX_HWADDR_LEN - 1] - &mac[0])/sizeof(mac[0])) + 1;

    // Check if the array size is sufficient
    if (cMac != NULL) {
        if (len >= 6) {
            for (int i = 0; i < len; i++) {
                cMac[i] = mac[i];
            }
        } else {
            // Handle error: return an appropriate error code or take necessary action
            MGR_ERROR("Given MAC array size is insufficient\n");
            return -1;
        }
    }

    MGR_INFO("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        cMac[0], cMac[1], cMac[2], cMac[3], cMac[4], cMac[5]);

    return 0;
}


int
WifiMgr_Get_WIFI_Mode(void)
{
    return gWifiMgrVar.WIFI_Mode;
}

uint8_t*
WifiMgr_Get_WIFI_MAC(void)
{
    return LwIP_GetMAC(&xnetif[0]);
}

uint8_t*
WifiMgr_Get_WIFI_IP(void)
{
    return LwIP_GetIP(&xnetif[0]);
}

uint8_t*
WifiMgr_Get_WIFI_GW(void)
{
    return LwIP_GetGW(&xnetif[0]);
}

uint8_t*
WifiMgr_Get_WIFI_Mask(void)
{
    return LwIP_GetMASK(&xnetif[0]);
}

int
WifiMgr_Get_Scan_AP_Info(WIFI_MGR_SCANAP_LIST* pList)
{
    int nApCount;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        MGR_ERROR("!WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexScan);

    /* Avoid scaning trigger from different threads */
    if (gWifiMgrVar.Start_Scan) {
        pthread_mutex_unlock(&mutexScan);
        return -1;
    }

    nApCount = _WifiMgr_Sta_Scan_Process(&gScanApInfo);
    if (nApCount > WIFI_SCAN_LIST_NUMBER) {
        nApCount = WIFI_SCAN_LIST_NUMBER;
    }

    memcpy(pList, gWifiMgrApList, sizeof(WIFI_MGR_SCANAP_LIST) * nApCount);

    pthread_mutex_unlock(&mutexScan);

    MGR_INFO("Get %d SSIDs \n", nApCount);

    return nApCount;
}


int
WifiMgr_Get_HostAP_Ready(void)
{
    MGR_INFO("WifiMgr_Get_HostAP_Ready: %d \n", wifi_is_ready_to_transceive(RTW_AP_INTERFACE));
    return wifi_is_ready_to_transceive(RTW_AP_INTERFACE);
}


int
WifiMgr_Get_HostAP_Device_Number(void)
{
    int stacount = 0;
    int client_number;
    struct {
        int    count;
        rtw_mac_t mac_list[3];
    } client_info;
    if (gWifiMgrVar.WIFI_Mode == WIFIMGR_MODE_CLIENT){
        return 0;
    }
	client_info.count = 3;
    wifi_get_associated_client_list(&client_info, sizeof(client_info));

#if 1
    //MGR_INFO("\n\rWifiMgr_Get_WIFI_Mode:");
    //MGR_INFO("\n\r==============================");

    if(client_info.count == 0){
       //MGR_INFO("\n\rClient Num: 0\n\r");
   	}

    else
    {
        MGR_INFO("\n\rClient Num: %d", client_info.count);

        for( client_number=0; client_number < client_info.count; client_number++ ) {
			printf("\n\rClient [%d]:", client_number);
			printf("\n\r\tMAC => "MAC_FMT"",MAC_ARG(client_info.mac_list[client_number].octet));
		}
        printf("\n\r");
    }
#endif

    return client_info.count;
}


int
WifiMgr_Sta_Reset_MAC(const char *mac)
{
    WIFIMGR_ECODE_E ret = WIFIMGR_ECODE_FAIL;
    char set_mac[18];

    /* Use the same IP */
    //ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_STOP_DHCP, NULL);

    /* Reset IP */
    ret = WifiMgr_Sta_Disconnect();

    if (ret == 0) {
        /* WIFI driver set off */
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_EXIT, NULL);
        usleep(100*1000);

        /* LWIP set down and clean */
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_DISABLE, NULL);
        usleep(100*1000);

        /* Set new MAC */
        strlcpy(set_mac, mac, strlen(mac) + 1);
        MGR_INFO("SET MAC(%s)\n", set_mac);
        ret = wifi_set_mac_address(set_mac);

        /* LWIP set up and build */
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL);

        /* WIFI driver resume */
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESUME, (void *)RTW_MODE_STA);
    }
    WifiMgr_Sta_Connect(gWifiMgrVar.Ssid, gWifiMgrVar.Password, WifiMgr_Secu_RTL_To_ITE(gWifiMgrVar.SecurityMode));

    return ret;
}


int
WifiMgr_Sta_Connect(const char* ssid, const char* password, const char* secumode)
{
    int nRet = WIFIMGR_ECODE_OK;
    rtw_security_t encrypt;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        MGR_ERROR("WifiMgr init not ready\n");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP || WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_P2P){
        MGR_ERROR("Need client mode to connect\n");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (gWifiMgrVar.Cancel_Connect) {
        return WIFIMGR_ECODE_CONNECT_ERROR;
    }

    if (ssid == NULL || password == NULL || secumode == NULL) {
        MGR_ERROR("Invalid input parameters\n");
        return WIFIMGR_ECODE_INVALID_PARAM;
    }

    encrypt = WifiMgr_Secu_ITE_To_RTL(secumode);

    _WifiMgr_Sta_Entry_Info(ssid, password, encrypt);

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
    int State, Ecode;
    int nRet = WIFIMGR_ECODE_OK;
    int sRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

	pthread_mutex_lock(&mutexDisconnect);

    WifiMgr_Sta_Cancel_Connect();

	MGR_INFO("==== WifiMgr_Sta_Disconnect ====\n");

    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL) > 0) {
        nRet = wifi_disconnect();

        usleep(1000 * 100);

        memset(&setting, 0, sizeof(ITPEthernetSetting));
        setting.dhcp = 0;
        setting.autoip = 0;

        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);
    } else {
        MGR_INFO("Connection was already disconnected, don't repeat\n");
        goto _END;
    }

    sRet = WifiMgr_Get_Connect_State(&State, &Ecode);
    MGR_INFO("==== Conn State = %d, Ecode = %d\n", State, Ecode);
    if (sRet == WIFIMGR_ECODE_OK && State != WIFIMGR_CONNSTATE_CONNECTING) {
        int i = 0;
#if 1
        do {
            MGR_INFO("WifiMgr_Sta_Disconnect wait connect-finish %d \n", i);
        i++;
        sleep(1);
        } while (gStaConnectProcess == 1 && i < 20);
#endif
        MGR_INFO("==== WifiMgr_Sta_Disconnect %d %d end ====\n", gStaConnectProcess, i);
    }

_END:
    usleep(100 * 1000);
    WifiMgr_Sta_Not_Cancel_Connect();
    /* Set Conn state and Err code */
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_STOP);
    _WifiMgr_Sta_Set_Ecode(WIFIMGR_ECODE_SET_DISCONNECT);
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);

    if (gWifiMgrSetting.wifiCallback) {
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT);
    }

    pthread_mutex_unlock(&mutexDisconnect);

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

	MGR_INFO("WifiMgr_Sta_Sleep_Disconnect \n");
    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_STOP);
    _WifiMgr_Sta_Set_Ecode(WIFIMGR_ECODE_SET_DISCONNECT);
    usleep(100 * 1000);

    return WIFIMGR_ECODE_OK;
}


void
WifiMgr_Sta_Switch(int status)
{
	gWifiMgrVar.Client_On_Off = status;
}


void
WifiMgr_Sta_Cancel_Connect(void)
{
    BOOLEAN_SET_TRUE(gWifiMgrVar.Cancel_Connect);
}


void
WifiMgr_Sta_Not_Cancel_Connect(void)
{
    BOOLEAN_SET_FALSE(gWifiMgrVar.Cancel_Connect);
}


void
WifiMgr_HostAP_Set_Hidden(void)
{
    BOOLEAN_SET_TRUE(gWifiMgrVar.SoftAP_Hidden);
}

rtw_security_t
WifiMgr_Secu_ITE_To_RTL(const char* ite_security_enum)
{
    uint32_t rtl_security_enum;
    char *end;

    if (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP)
        return RTW_SECURITY_UNKNOWN;

    /* Translate ITE WIFI security enum to RTL WIFI security enum*/
    switch (strtol(ite_security_enum, &end, 10)) {
        case 0:
        rtl_security_enum = RTW_SECURITY_OPEN;
            break;

        case 1:
        rtl_security_enum = RTW_SECURITY_WEP_PSK;
            break;

        case 2:
        rtl_security_enum = RTW_SECURITY_WPA_TKIP_PSK;
            break;

        case 3:
            rtl_security_enum = RTW_SECURITY_WPA_AES_PSK;
            break;

        case 4:
        rtl_security_enum = RTW_SECURITY_WPA2_AES_PSK;
            break;

        case 5:
        rtl_security_enum = RTW_SECURITY_WPA2_TKIP_PSK;
            break;

        case 6:
        rtl_security_enum = RTW_SECURITY_WPA2_MIXED_PSK;
            break;

        case 7:
        rtl_security_enum = RTW_SECURITY_WPA_WPA2_MIXED_PSK;
            break;

        case 8:
            rtl_security_enum = RTW_SECURITY_WPS_OPEN;
            break;

        case 9:
        rtl_security_enum = RTW_SECURITY_WPS_SECURE;
            break;

        case 10:
            rtl_security_enum = RTW_SECURITY_UNKNOWN;
            break;

        case 11:
        rtl_security_enum = RTW_SECURITY_WPA_TKIP_8021X;
            break;

        case 12:
        rtl_security_enum = RTW_SECURITY_WPA_AES_8021X;
            break;

        case 13:
        rtl_security_enum = RTW_SECURITY_WPA2_AES_8021X;
            break;

        case 14:
        rtl_security_enum = RTW_SECURITY_WPA2_TKIP_8021X;
            break;

        case 15:
        rtl_security_enum = RTW_SECURITY_WPA_WPA2_MIXED_8021X;
            break;

        case 16:
            rtl_security_enum = RTW_SECURITY_WPA3_AES_PSK;
            break;

        default:
        rtl_security_enum = RTW_SECURITY_UNKNOWN;
    }


    MGR_INFO("WifiMgr_Secu_ITE_To_RTL: ITE(%s) -> RTL(0x%x)\n", ite_security_enum, rtl_security_enum);

    return rtl_security_enum;
}


char*
WifiMgr_Secu_RTL_To_ITE(uint64_t rtl_security_enum)
{
    static char ite_security_enum[3];
    int len = sizeof(ite_security_enum);

    /* Translate RTL WIFI security enum to ITE WIFI security enum*/
    switch (rtl_security_enum) {
        case RTW_SECURITY_OPEN:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_OPEN);
            break;

        case RTW_SECURITY_WEP_PSK:;
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WEP_PSK);
            break;

        case RTW_SECURITY_WPA_TKIP_PSK:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA_TKIP_PSK);
            break;

        case RTW_SECURITY_WPA_TKIP_8021X:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA_TKIP_8021X);
            break;

        case RTW_SECURITY_WPA_AES_PSK:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA_AES_PSK);
            break;

        case RTW_SECURITY_WPA_AES_8021X:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA_AES_8021X);
            break;

        case RTW_SECURITY_WPA2_AES_PSK:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA2_AES_PSK);
            break;

        case RTW_SECURITY_WPA2_AES_8021X:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA2_AES_8021X);
            break;

        case RTW_SECURITY_WPA2_TKIP_PSK:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA2_TKIP_PSK);
            break;

        case RTW_SECURITY_WPA2_TKIP_8021X:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA2_TKIP_8021X);
            break;

        case RTW_SECURITY_WPA2_MIXED_PSK:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA2_MIXED_PSK);
            break;

        case RTW_SECURITY_WPA_WPA2_MIXED_PSK:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA_WPA2_MIXED);
            break;

        case RTW_SECURITY_WPA_WPA2_MIXED_8021X:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA_WPA2_MIXED_8021X);
            break;

        case RTW_SECURITY_WPA3_AES_PSK:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_WPA3_AES_PSK);
            break;

        default:
            snprintf(ite_security_enum, len, "%s", ITE_WIFI_SEC_UNKNOWN);
    }

    MGR_INFO("WifiMgr_Secu_RTL_To_ITE: RTL(0x%llX) -> ITE(%s)\n", rtl_security_enum, ite_security_enum);

    return ite_security_enum;
}

int
WifiMgr_Set_APMode_5G(int nOnOff)
{
    if (nOnOff==0){
        gSoftAP_5G = 0;
    } else if (nOnOff == 1){
        gSoftAP_5G = 5;
    }

    return WIFIMGR_ECODE_OK;
}


int
WifiMgr_Init(WIFIMGR_MODE_E init_mode, int mp_mode, WIFI_MGR_SETTING wifiSetting)
{
    int nRet = WIFIMGR_ECODE_NOT_INIT;

    // Validate input parameters
    if (init_mode < WIFIMGR_MODE_CLIENT || init_mode > WIFIMGR_MODE_MAX) {
        return WIFIMGR_ECODE_INVALID_PARAM;
    }

    if (mp_mode < 0) {
        return WIFIMGR_ECODE_INVALID_PARAM;
    }

    // Check if the WiFi device is ready
    if (init_mode == WIFIMGR_MODE_CLIENT) {
        int wifiReady = ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_DEVICE_READY, NULL);
        if (wifiReady == 0) {
            return WIFIMGR_ECODE_DEVICE_NOT_READY;
        }
    }

    // Wait for WiFiMgr to finish
    while (gWifiMgrVar.WIFI_Terminate) {
        MGR_INFO("WifiMgr not finished yet\n");
        usleep(100 * 1000);
    }

    // Initialize variables
    wifi_conn_ecode         = WIFIMGR_ECODE_SET_DISCONNECT;
    gWifiMgrVar.MP_Mode     = mp_mode;
    gWifiMgrVar.WIFI_Mode   = init_mode;

    // Copy wifiSetting parameters
    memcpy(&gWifiMgrSetting.setting, &wifiSetting.setting, sizeof(ITPEthernetSetting));
    gWifiMgrSetting.wifiCallback = wifiSetting.wifiCallback;

    MGR_INFO("Start init WifiMgr for %s mode\n", (init_mode == WIFIMGR_MODE_CLIENT) ? "Client" : "AP");

    if (gWifiMgrVar.WIFI_Terminate == true) {
        BOOLEAN_SET_FALSE(gWifiMgrVar.WIFI_Terminate);
    }

    // Initialize based on init_mode
    if (init_mode == WIFIMGR_MODE_CLIENT) {
        MGR_INFO("Pre-setting Client mode SSID/PW: %s/%s\n", wifiSetting.ssid, wifiSetting.password);
        _WifiMgr_Sta_Entry_Info(wifiSetting.ssid, wifiSetting.password, wifiSetting.secumode);
    } else if (init_mode == WIFIMGR_MODE_SOFTAP) {
        gSoftAP_5G = mp_mode;
        MGR_INFO("Pre-setting AP mode SSID/PW: %s/%s\n", wifiSetting.ap_ssid, wifiSetting.ap_password);
        _WifiMgr_HostAP_Entry_Info(wifiSetting.ap_ssid, wifiSetting.ap_password);
        _WifiMgr_HostAP_Init();
    } else if (init_mode == WIFIMGR_MODE_P2P) {
#if(defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)
        gSoftAP_5G = mp_mode;
        MGR_INFO("Pre-setting P2P mode P2P %s ,SSID/PW: %s/%s\n", wifiSetting.p2p_name, wifiSetting.ap_ssid, wifiSetting.ap_password);
        _WifiMgr_P2P_Entry_Info(wifiSetting.p2p_name);
        _WifiMgr_HostAP_Entry_Info(wifiSetting.ap_ssid, wifiSetting.ap_password);
        MGR_INFO("WifiMgr_P2P_GO_Start\n");
        WifiMgr_P2P_GO_Start();
#endif
    } else if (init_mode == WIFIMGR_MODE_MAX) {
        _WifiMgr_HostAP_Entry_Info(wifiSetting.ap_ssid, wifiSetting.ap_password);
        _WifiMgr_HostAP_Init();

        sleep(1);
        _WifiMgr_Sta_Entry_Info(wifiSetting.ssid, wifiSetting.password, wifiSetting.secumode);
    }

    /* ==== Init  Sema and Mutex ==== */
    nRet = _WifiMgr_Posix_Init();
    if (nRet != WIFIMGR_ECODE_OK) {
        goto err_end;
    }

    /* ====  Create necessary thread ==== */
    _WifiMgr_Proc_Init();

    _WifiMgr_Sta_Set_Connect_State(WIFIMGR_CONNSTATE_STOP);
    BOOLEAN_SET_FALSE(gWifiMgrVar.Pre_Scan);
    BOOLEAN_SET_FALSE(gWifiMgrVar.Cancel_WPS);
    BOOLEAN_SET_FALSE(gWifiMgrVar.Cancel_Connect);
    BOOLEAN_SET_FALSE(gWifiMgrVar.WIFI_Terminate);
    BOOLEAN_SET_TRUE(gWifiMgrVar.WIFI_Init_Ready);

    return nRet;

err_end:
    MGR_ERROR("Part of Posix init was failed, EC: %d", nRet);
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

    while (gWifiMgrVar.Start_Scan) {
        if (_WifiMgr_Negative_Condition() > 0) {
            break;
        } else {
            usleep(100 * 1000);
        }
    }

    /* Set some necessary flags in terminate scenes */
    BOOLEAN_SET_FALSE(gWifiMgrVar.Start_Scan);
    BOOLEAN_SET_TRUE(gWifiMgrVar.WIFI_Terminate);

#if WIFIMGR_EXCUTE_WPS
    sem_post(&semWPSStart);
#endif

    switch (WifiMgr_Get_WIFI_Mode()) {
        case WIFIMGR_MODE_CLIENT:
            _WifiMgr_Sta_Terminate();
            break;

        case WIFIMGR_MODE_SOFTAP:
            _WifiMgr_HostAP_Terminate();
            break;

        case WIFIMGR_MODE_P2P:
            break;

        default:
            MGR_ERROR("Unknown mode is not supported for termination\n");
    }

    {
        int val;
        sem_getvalue(&semSwitchModeStop, &val);
        if (val > 0) {
            sem_wait(&semSwitchModeStop);
        }
    }

    usleep(100 * 1000);

    /* Wait [_WifiMgr_Sta_Thread] terminate thread completely */
    sem_wait(&semConnectStop);

    /* Wait [_WifiMgr_Main_Process_Thread] terminate thread completely */
    do {
        MGR_TRACE("[%d/20] WifiMgr_Main_Process_Thread[%d]\n", nWait, gWifiMgrVar.IS_WifiMgr_Main_Process_Thread);
        usleep(100 * 1000);
        nWait++;
        if (nWait > 20) {
            break;
        }
    } while (gWifiMgrVar.IS_WifiMgr_Main_Process_Thread);

    /* Destroy mutex and semaphores */
    if (pthread_mutex_destroy(&mutexScan) != 0) {
        MGR_ERROR("Failed to destroy mutexScan\n");
    }

    if (pthread_mutex_destroy(&mutexALWiFi) != 0) {
        MGR_ERROR("Failed to destroy mutexALWiFi\n");
    }

    if (pthread_mutex_destroy(&mutexConnect) != 0) {
        MGR_ERROR("Failed to destroy mutexConnect\n");
    }

    if (pthread_mutex_destroy(&mutexDisconnect) != 0) {
        MGR_ERROR("Failed to destroy mutexDisconnect\n");
    }

#if WIFIMGR_EXCUTE_WPS
    if (pthread_mutex_destroy(&mutexALWiFiWPS) != 0) {
        MGR_ERROR("Failed to destroy mutexALWiFiWPS\n");
    }

    if (sem_destroy(&semWPSStop) != 0) {
        MGR_ERROR("Failed to destroy semWPSStop\n");
    }

    if (sem_destroy(&semWPSStart) != 0) {
        MGR_ERROR("Failed to destroy semWPSStart\n");
    }
#endif

    if (sem_destroy(&semConnectStop) != 0) {
        MGR_ERROR("Failed to destroy semConnectStop\n");
    } else {
        int val;
        sem_getvalue(&semConnectStop, &val);
        if (val > 0) {
            MGR_ERROR("semConnectStop still exists (%d)\n", val);
        } else {
            MGR_ERROR("semConnectStop destroyed successfully (%d)\n", val);
        }
    }

    /* Initialize these flags */
    BOOLEAN_SET_FALSE(gWifiMgrVar.WIFI_Init_Ready);
    BOOLEAN_SET_FALSE(gWifiMgrVar.WIFI_Terminate); //It must consident all task were terminate(e.g. _WifiMgr_Sta_Thread)
    BOOLEAN_SET_FALSE(gWifiMgrVar.SoftAP_Hidden);
    BOOLEAN_SET_FALSE(gWifiMgrVar.SoftAP_Init_Ready);

    MGR_TRACE("<==== Terminate end\n");
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);

    return nRet;
}


/* STA and HostAP are both open */
int
WifiMgr_Concurrent_First_Start(void)
{
    int rc;
    MGR_TRACE("\n");

	if (wifi_off() != 0) {
        rc = -1;
        MGR_ERROR("WIFI off get error\n");
        return rc;
    }
    vTaskDelay(20);
    if (wifi_on(RTW_MODE_STA_AP) != RTW_SUCCESS) {
        rc = -1;
        MGR_ERROR("WIFI on get error\n");
        return rc;
    }

    return 0;
}

int
WifiMgr_HostAP_First_Start(void)
{
    int rc;
    MGR_TRACE("\n");

    if (wifi_off() != 0) {
        rc = -1;
        MGR_ERROR("WIFI off get error\n");
        return rc;
    }
    sleep(1);
    if (wifi_on(RTW_MODE_AP) != RTW_SUCCESS) {
        rc = -1;
        MGR_ERROR("WIFI on get error\n");
        return rc;
    }

    return 0;
}


int
WifiMgr_Sta_HostAP_Switch(WIFI_MGR_SETTING wifiSetting)
{
    //gettimeofday(&gSwitchStart, NULL);

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
    return WIFIMGR_ECODE_OK;
}

#if(defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)
#define FORMAT_IPADDR(x) ((unsigned char *)&x)[3], ((unsigned char *)&x)[2], ((unsigned char *)&x)[1], ((unsigned char *)&x)[0]
extern unsigned char g8821_p2p_state;
static int gFrom_mode = 0;
static int gTo_mode = 0;
static uint32_t gP2PDeviceMapping[255];
static uint8_t gDeviceAddres[255][16];
// fix warnings
int cmd_p2p_deivce_connected(unsigned char *dev_addr);
void cmd_p2p_info(int argc, char **argv);
void cmd_wifi_p2p_start(int argc, char **argv);
void cmd_wifi_p2p_auto_go_start(int argc, char **argv);
void cmd_wifi_p2p_stop(int argc, char **argv);





enum RTK_P2P_STATE{
    P2P_STATE_NONE = 0,
    P2P_STATE_IDLE = 1,
    P2P_STATE_LISTEN = 2,
    P2P_STATE_SCAN = 3,
    P2P_STATE_FIND_PHASE_LISTEN  = 4,
    P2P_STATE_FIND_PHASE_SEARCH = 5,
    P2P_STATE_TX_PROVISION_DIS_REQ = 6,
    P2P_STATE_RX_PROVISION_DIS_RSP = 7,
    P2P_STATE_RX_PROVISION_DIS_REQ = 8,
    P2P_STATE_GONEGO_ING  = 9,
    P2P_STATE_GONEGO_OK = 10,
    P2P_STATE_GONEGO_FAIL = 11,
    P2P_STATE_RECV_INVITE_REQ_MATCH  = 12,
    P2P_STATE_PROVISIONING_ING = 13,
    P2P_STATE_PROVISIONING_DONE  = 14,
    P2P_STATE_TX_INVITE_REQ = 15,
    P2P_STATE_RX_INVITE_RESP = 16,
    P2P_STATE_RECV_INVITE_REQ_DISMATCH  = 17,
    P2P_STATE_RECV_INVITE_REQ_GO = 18,
    P2P_STATE_RECV_INVITE_REQ_JOIN  = 19,
    P2P_STATE_FORMATION_COMPLETE = 20,
    P2P_STATE_CONNECTED = 21,
    P2P_STATE_DISCONNECT  = 22,
    P2P_STATE_FOUND = 23,
    P2P_STATE_WSC_EXCHAGE_START = 24,
};

static void*
_WifiMgr_P2P_Switch_Process(void* arg)
{
    int nTemp;

    if (gFrom_mode == WIFIMGR_MODE_P2P){
        gWifiMgrVar.WIFI_Mode = WifiMgr_Get_WIFI_Mode();
        if (gFrom_mode != gWifiMgrVar.WIFI_Mode) {
            MGR_ERROR("from_mode(%d) != gWifiMgrVar.WIFI_Mode(%d) \n",gFrom_mode,gWifiMgrVar.WIFI_Mode);
            return NULL;
        }
        MGR_INFO("================ WifiMgr_P2P_Switch START (%d Mode Now) ================>\n",gWifiMgrVar.WIFI_Mode);

        WifiMgr_Terminate();
        WifiMgr_P2P_Stop();
        sleep(1);
////
#if (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)

#else
        printf("_WifiMgr_Switch_Mode_PwrSeq_Down  \n");
        _WifiMgr_Switch_Mode_PwrSeq_Down();


        printf("ITP_DEVICE_SDIO ITP_IOCTL_INIT \n ");
        
        /* ==== Install APP/Driver/IO ==== */
        ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);

#endif        
//////
        switch (gTo_mode) {
            case WIFIMGR_MODE_SOFTAP :
                /* === P2P  -> AP === */
                MGR_TRACE("Terminate P2P done, prepare to HostAP...\n");
                ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESUME, (void *)RTW_MODE_AP);
                usleep(200 * 1000);
                gSoftAP_5G = 5;
                if (gSoftAP_5G==5)
                    nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 5, gWifiSetting);
                else
                    nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
                break;

            case WIFIMGR_MODE_CLIENT:
                /* === P2P -> STA === */
                MGR_TRACE("Terminate P2P done, prepare to Client...\n");
                ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESUME, (void *)RTW_MODE_STA);
                usleep(200 * 1000);
                nTemp = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
                break;

            case WIFIMGR_MODE_MAX:
                MGR_INFO("Not support concurrent mode\n");
                break;

            default:
                MGR_ERROR("Unknow mode\n");
        }
    } else if ( (gFrom_mode == WIFIMGR_MODE_CLIENT || gFrom_mode == WIFIMGR_MODE_SOFTAP)) {
        if (gTo_mode != WIFIMGR_MODE_P2P) {
            MGR_ERROR("gTo_mode(%d) != WIFIMGR_MODE_P2P(%d) \n",gTo_mode,WIFIMGR_MODE_P2P);
            return NULL;
        }
        MGR_INFO("================ WifiMgr_P2P_Switch START (%d Mode Now) ================>\n",gWifiMgrVar.WIFI_Mode);

        WifiMgr_Terminate();
        sleep(1);
#if (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)

#else        
        _WifiMgr_Switch_Mode_PwrSeq_Down();
        ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);
#endif
        MGR_INFO("WifiMgr_P2P_GO_Start %s %s %s\n",gWifiMgrVar.P2P_name,gWifiMgrVar.ApSsid,gWifiMgrVar.ApPassword);
        WifiMgr_P2P_GO_Start();

    } else {
        MGR_ERROR("WifiMgr_P2P_Switch not support \n");
        return NULL;
    }

    return NULL;
}

unsigned int WifiMgr_P2P_SetPhoneIP_Is_P2PConnected(unsigned char *dev_addr)
{
    int nTemp;
    int i;

    nTemp = cmd_p2p_deivce_connected(dev_addr);
    for (i=0 ; i<=5 ; i++){
        gDeviceAddres[0][i]=dev_addr[i];
    }

    printf("WifiMgr_P2P_Get_Connect_Status hw %02X-%02X-%02X-%02X-%02X-%02X, p2p device %d \n",  dev_addr[0], dev_addr[1], dev_addr[2], dev_addr[3], dev_addr[4], dev_addr[5],nTemp);

    return nTemp;
}

unsigned int WifiMgr_P2P_SetDevice_Mapping(uint32_t addr, uint32_t p2pDevice)
{
    int i,j;
    unsigned char nIp[4];
    memcpy(nIp,&addr,sizeof(uint32_t));
    printf("WifiMgr_P2P_SetDevice_Mapping 0x%x, 0x%x, 0x%x, 0x%x (%u) \n",nIp[0],nIp[1],nIp[2],nIp[3],p2pDevice);
    i = (int)nIp[3];
    j = 0;
    printf("index %d \n",i);
    if (i < 255 && i>0){
      gP2PDeviceMapping[i]= p2pDevice;
      if (!gDeviceAddres[i][j]){
          for (j=0 ; j<=5 ; j++){
              gDeviceAddres[i][j]=gDeviceAddres[0][j];
          }
      }
    } else {
        MGR_ERROR("WifiMgr_P2P_SetDevice_Mapping index not support \n");
        return 1;
    }

    return 0;
}

unsigned int WifiMgr_P2P_GetDevice_Mapping(uint32_t addr)
{
    int i;
    int j;
    unsigned char nIp[4];
    int nTemp;
    memcpy(nIp,&addr,sizeof(uint32_t));
    printf("WifiMgr_P2P_GetDevice_Mapping 0x%x, 0x%x, 0x%x, 0x%x  \n",nIp[0],nIp[1],nIp[2],nIp[3]);
    i = (int)nIp[3];
    printf("index %d \n",i);
/////////////////
//    printf("------------------------ debug +\n");
//    for(j=2;j<=3 ;j++){
//        cmd_p2p_deivce_connected(gDeviceAddres[j]);
//    }
//    printf("------------------------ debug -\n");
///////////////



    if (i < 255 && i>0){
        //
#if 1
        nTemp = cmd_p2p_deivce_connected(gDeviceAddres[i]);
        return nTemp;
#else
        return gP2PDeviceMapping[i];
#endif
    } else {
        MGR_ERROR("WifiMgr_P2P_SetDevice_Mapping index not support \n");
        return 0;
    }

}

uint8_t WifiMgr_P2P_Get_Status(void)
{

    // Get p2p status
    cmd_p2p_info(0,0);



    return g8821_p2p_state;
}

bool WifiMgr_P2P_IsConnected(void)
{
    // Get p2p status
    cmd_p2p_info(0,0);

    if(g8821_p2p_state == P2P_STATE_CONNECTED)
        return true;

    return false;
}

bool WifiMgr_P2P_IsDisconnected(void)
{
    // Get p2p status
    cmd_p2p_info(0,0);

    if(g8821_p2p_state == P2P_STATE_NONE || g8821_p2p_state == P2P_STATE_DISCONNECT)
        return true;

    return false;
}

void WifiMgr_P2P_SetPhoneIP(uint32_t addr)
{
#if 0
    uint32_t nIp = 0x0;

    nIp = htonl(addr);
    printf("[%s] IPaddr(0x%x): %u.%u.%u.%u\n", __FUNCTION__, addr, FORMAT_IPADDR(nIp));
#endif

    PhoneIP = addr;
}

uint32_t WifiMgr_P2P_GetPhoneIP(void)
{
    return PhoneIP;
}

uint32_t WifiMgr_P2P_GetIteIP(void)
{
    uint32_t IteIP = 0x0;
    ip_addr_t * ip_r = NULL;

    ip_r = (ip_addr_t *) WifiMgr_Get_WIFI_IP();
    IteIP = ip_r->addr;

    return IteIP;
}

uint32_t WifiMgr_P2P_GC_Start(void)
{

    return 0;
}

uint32_t WifiMgr_P2P_GO_Start(void)
{
    //char* argv[] = {"ite_p2p","iteap","12345678"};
#if 1
    char* argv[] = {gWifiMgrVar.P2P_name,gWifiMgrVar.ApSsid,gWifiMgrVar.ApPassword};

    if (strlen(gWifiMgrVar.ApPassword)<8){
        MGR_ERROR("WifiMgr_P2P_GO_Start password %s len %d< 8)\n",gWifiMgrVar.ApPassword, strlen(gWifiMgrVar.ApPassword));
        return -1;
    }
    if (strlen(gWifiMgrVar.ApSsid)<1){
        MGR_ERROR("WifiMgr_P2P_GO_Start P2P_name %s len %d< 1)\n",gWifiMgrVar.ApSsid, strlen(gWifiMgrVar.ApSsid));
        return -1;
    }
    if (strlen(gWifiMgrVar.P2P_name)<1){
        MGR_ERROR("WifiMgr_P2P_GO_Start ApSsid %s len %d< 1)\n", gWifiMgrVar.P2P_name,strlen(gWifiMgrVar.P2P_name));
        return -1;
    }
#endif

    printf("cmd_wifi_p2p_start [+] \n");
    cmd_wifi_p2p_start(0,0);
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, &xnetif[0]);
    printf("cmd_wifi_p2p_start [-] \n");
    sleep(1);
    cmd_wifi_p2p_auto_go_start(ITH_COUNT_OF(argv),argv);
    printf("cmd_wifi_p2p_auto_go_start ....... \n");
    sleep(1);
    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);

    memset(gP2PDeviceMapping,0 , sizeof(gP2PDeviceMapping));
    memset(gDeviceAddres,0,sizeof(gDeviceAddres));

    return 0;
}


uint32_t WifiMgr_P2P_Stop(void)
{
    cmd_wifi_p2p_stop(0,0);

    return 0;
}


int WifiMgr_P2P_Switch(WIFIMGR_MODE_E from_mode, WIFIMGR_MODE_E to_mode, WIFI_MGR_SETTING wifiSetting)
{
    int nRet;
    pthread_t       task;
    pthread_attr_t  attr;
    int nTemp;

    if (!gWifiMgrVar.WIFI_Init_Ready)
        return -1;

    gFrom_mode = from_mode;
    gTo_mode = to_mode;
#if 1
    snprintf(gWifiSetting.p2p_name, WIFI_SSID_MAXLEN, wifiSetting.p2p_name);
    snprintf(gWifiSetting.ap_ssid, WIFI_SSID_MAXLEN, wifiSetting.ap_ssid);
    snprintf(gWifiSetting.ap_password, WIFI_PASSWORD_MAXLEN, wifiSetting.ap_password);
    snprintf(gWifiSetting.ssid, WIFI_SSID_MAXLEN, wifiSetting.ssid);
    snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN, wifiSetting.password);

    snprintf(gWifiMgrVar.P2P_name, WIFI_SSID_MAXLEN, wifiSetting.p2p_name);
    snprintf(gWifiMgrVar.ApSsid, WIFI_SSID_MAXLEN, wifiSetting.ap_ssid);
    snprintf(gWifiMgrVar.ApPassword, WIFI_PASSWORD_MAXLEN, wifiSetting.ap_password);
    snprintf(gWifiMgrVar.Ssid, WIFI_SSID_MAXLEN, wifiSetting.ssid);
    snprintf(gWifiMgrVar.Password, WIFI_PASSWORD_MAXLEN, wifiSetting.password);

    gWifiSetting.setting.dhcp = 1;
#endif

    MGR_INFO("WifiMgr_P2P_Switch %s %s %s\n",wifiSetting.p2p_name,wifiSetting.ap_ssid,wifiSetting.ap_password);
    MGR_INFO("WifiMgr_P2P_Switch %s %s %s\n",gWifiSetting.p2p_name,gWifiSetting.ap_ssid,gWifiSetting.ap_password);
    MGR_INFO("WifiMgr_P2P_Switch ssid %s password %s \n",gWifiSetting.ssid,gWifiSetting.password);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
    nRet = pthread_create(&task, &attr, _WifiMgr_P2P_Switch_Process, NULL);
    if (nRet) {
        MGR_ERROR(" create [_WifiMgr_P2P_Switch_Process] thread fail! 0x%08X \n", nRet );
        return WIFIMGR_ECODE_FAIL;
    }

    return WIFIMGR_ECODE_OK;
}

#endif


#endif
