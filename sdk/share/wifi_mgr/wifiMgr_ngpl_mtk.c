#if defined(CFG_NET_WIFI_SDIO_VND_MTK)
#include "wifiMgr.h"
#include "config.h"
#include "wifi_api_ex.h"
#include "wpa_supplicant_task.h"
#include "wifi_event_gen4m.h"
#include "wifi_netif.h"

static pthread_mutex_t                   mutexScan;
extern wifi_scan_list_item_t g_ap_list[CFG_MAX_NUM_BSS_LIST];
static WIFI_MGR_SETTING                  gWifiMgrSetting    = {0};

char secu_str[][30] = {
    "OPEN",
    "WEP_PSK",
    "WPA_TKIP_PSK",
    "WPA_AES_PSK",
    "WPA2_AES_PSK",
    "WPA2_TKIP_PSK",
    "WPA2_MIXED_PSK",
    "WPA_WPA2_MIXED",
    "WPS_OPEN",
    "WPS_SECURE",
    "EAP_AES",
    "WPA_TKIP_8021X",
    "WPA_AES_8021X",
    "WPA2_AES_8021X",
    "WPA2_TKIP_8021X",
    "WPA_WPA2_MIXED_8021X",
    "WPA3_AES_PSK",
    "SEC_UNKNOWN"
};

int32_t Mgr_wifi_event_handler(wifi_event_t event, uint8_t *payload, uint32_t length)
{
    switch (event) {
        case WIFI_EVENT_IOT_PORT_SECURE:
            {
                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);
                break;
            }

        case WIFI_EVENT_IOT_DISCONNECTED:
            {
                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT);
                break;

            }
        case WIFI_EVENT_IOT_P2P_PROVISION_DIS:
            {
                wifi_start_ap_wps_thread(*payload);
                break;
            }
        default:
            break;
    }

    return 1;
}

int32_t mtk_auth_encrypt_2_ite_wifi_sec(wifi_auth_mode_t auth_mode, wifi_encrypt_type_t encrypt_type)
{
    int ite_secu_mode = MTK_ITE_WIFI_SEC_UNKNOWN;

     switch (auth_mode) {
         case WIFI_AUTH_MODE_OPEN:
             {
                 ite_secu_mode = MTK_ITE_WIFI_SEC_OPEN;
                 break;
             }
         case WIFI_AUTH_MODE_SHARED:
             {
                 ite_secu_mode = MTK_ITE_WIFI_SEC_WEP_PSK;
                 break;
             }
         case WIFI_AUTH_MODE_WPA_PSK:
             {
                 if(encrypt_type == WIFI_ENCRYPT_TYPE_AES_ENABLED) {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_WPA_AES_PSK;
                 } else if (encrypt_type == WIFI_ENCRYPT_TYPE_TKIP_ENABLED) {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_WPA_TKIP_PSK;
                 } else {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_UNKNOWN;
                 }
                 break;
             }
         case WIFI_AUTH_MODE_WPA2_PSK:
             {
                 if(encrypt_type == WIFI_ENCRYPT_TYPE_AES_ENABLED) {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_WPA2_AES_PSK;
                 } else if (encrypt_type == WIFI_ENCRYPT_TYPE_TKIP_ENABLED) {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_WPA2_TKIP_PSK;
                 } else {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_UNKNOWN;
                 }
                 break;
             }
         case WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK:
             {
                 if(encrypt_type == WIFI_ENCRYPT_TYPE_AES_ENABLED) {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_PSK;
                 } else if (encrypt_type == WIFI_ENCRYPT_TYPE_TKIP_ENABLED) {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_PSK;
                 } else {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_UNKNOWN;
                 }
                 break;
             }
         case WIFI_AUTH_MODE_WPA3_PSK:
         case WIFI_AUTH_MODE_WPA2_PSK_WPA3_PSK:
             {
                 if(encrypt_type == WIFI_ENCRYPT_TYPE_AES_ENABLED) {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_WPA3_AES_PSK;
                 } else {
                     ite_secu_mode = MTK_ITE_WIFI_SEC_UNKNOWN;
                 }
                 break;
             }
         default:
             {
                 ite_secu_mode = MTK_ITE_WIFI_SEC_UNKNOWN;
             }
     }

     return ite_secu_mode;
}

int32_t ite_wifi_sec_2_mtk_auth_encrypt(USEC_T secmode, wifi_auth_mode_t *auth_mode, wifi_encrypt_type_t *encrypt_type)
{
    MGR_ERROR("secmode %d \n", secmode);
    switch (secmode) {
        case MTK_ITE_WIFI_SEC_OPEN:
            {
                *auth_mode = WIFI_AUTH_MODE_OPEN;
                *encrypt_type = WIFI_ENCRYPT_TYPE_WEP_DISABLED;
                break;
            }
        case MTK_ITE_WIFI_SEC_WEP_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_OPEN;
                *encrypt_type = WIFI_ENCRYPT_TYPE_WEP_ENABLED;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPA_TKIP_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_WPA_PSK;
                *encrypt_type = WIFI_ENCRYPT_TYPE_TKIP_ENABLED;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPA_AES_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_WPA_PSK;
                *encrypt_type = WIFI_ENCRYPT_TYPE_AES_ENABLED;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPA2_AES_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_WPA2_PSK;
                *encrypt_type = WIFI_ENCRYPT_TYPE_AES_ENABLED;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPA2_TKIP_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_WPA2_PSK;
                *encrypt_type = WIFI_ENCRYPT_TYPE_TKIP_ENABLED;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPA2_MIXED_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_WPA2_PSK;
                *encrypt_type = WIFI_ENCRYPT_TYPE_TKIP_AES_MIX;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK;
                *encrypt_type = WIFI_ENCRYPT_TYPE_TKIP_AES_MIX;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPA3_AES_PSK:
            {
                *auth_mode = WIFI_AUTH_MODE_WPA3_PSK;
                *encrypt_type = WIFI_ENCRYPT_TYPE_AES_ENABLED;
                break;
            }
        case MTK_ITE_WIFI_SEC_WPS_OPEN:
        case MTK_ITE_WIFI_SEC_WPS_SECURE:
        case MTK_ITE_WIFI_SEC_EAP_AES:
        case MTK_ITE_WIFI_SEC_WPA_TKIP_8021X:
        case MTK_ITE_WIFI_SEC_WPA_AES_8021X:
        case MTK_ITE_WIFI_SEC_WPA2_AES_8021X:
        case MTK_ITE_WIFI_SEC_WPA2_TKIP_8021X:
        case MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_8021X:
        default :
            {
                MGR_ERROR("sec %d not support\n", secmode);
                return -1;
            }
    }
    return 0;
}

USEC_T WifiMgr_Secu_ITE_To_MTK(const char* ite_security_enum)
{
    USEC_T mtk_security_enum;
    char *end;

    switch (strtol(ite_security_enum, &end, 10)) {
        case 0:
            mtk_security_enum = MTK_ITE_WIFI_SEC_OPEN;
            break;

        case 1:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WEP_PSK;
            break;

        case 2:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA_TKIP_PSK;
            break;

        case 3:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA_AES_PSK;
            break;

        case 4:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA2_AES_PSK;
            break;

        case 5:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA2_TKIP_PSK;
            break;

        case 6:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA2_MIXED_PSK;
            break;

        case 7:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_PSK;
            break;

        case 8:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPS_OPEN;
            break;

        case 9:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPS_SECURE;
            break;

        case 10:
            mtk_security_enum = MTK_ITE_WIFI_SEC_UNKNOWN;
            break;

        case 11:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA_TKIP_8021X;
            break;

        case 12:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA_AES_8021X;
            break;

        case 13:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA2_AES_8021X;
            break;

        case 14:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA2_TKIP_8021X;
            break;

        case 15:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_8021X;
            break;

        case 16:
            mtk_security_enum = MTK_ITE_WIFI_SEC_WPA3_AES_PSK;
            break;

        default:
            mtk_security_enum = MTK_ITE_WIFI_SEC_UNKNOWN;
    }


    MGR_INFO("WifiMgr_Secu_ITE_To_MTK: ITE(%s) -> MTK(0x%x)\n", ite_security_enum, mtk_security_enum);

    return mtk_security_enum;
}

void InitWpaSupplicant(WIFIMGR_MODE_E init_mode, int mp_mode, WIFI_MGR_SETTING wifiSetting)
{
    wifi_config_t config = {0};
    wifi_config_ext_t config_ext = {0};
    wifi_auth_mode_t auth_mode;
    wifi_encrypt_type_t encrypt_type;
    extern void mtk_freertos_api2supp_semphr_take(void);


    ite_wifi_sec_2_mtk_auth_encrypt(wifiSetting.secumode, &auth_mode, &encrypt_type);

    memset(&config, 0, sizeof(config));
    memset(&config_ext, 0, sizeof(config_ext));

    switch(init_mode) {
        case WIFIMGR_MODE_CLIENT:
            {
                config.opmode = WIFI_MODE_STA_ONLY;
            }
            break;
        case WIFIMGR_MODE_SOFTAP:
            {
                config.opmode = WIFI_MODE_AP_ONLY;
            }
            break;
        case WIFIMGR_MODE_P2P:
            {
                config.opmode = WIFI_MODE_P2P_ONLY;
            }
            break;
        case WIFIMGR_MODE_SOFTAP_CLIENT:
            {
                config.opmode = WIFI_MODE_REPEATER;
            }
            break;
        case WIFIMGR_MODE_P2P_GO_CLIENT:
            {
                config.opmode = WIFI_MODE_P2P_STA;
            }
            break;
        default:
            MGR_ERROR("Wifi Operation mode %d not support\n", init_mode);
            return;
    }

    memcpy(config.sta_config.ssid, wifiSetting.ssid,
               WIFI_MAX_LENGTH_OF_SSID);
    config.sta_config.ssid_length = strlen(wifiSetting.ssid);
    config.sta_config.bssid_present = 0;
    config.sta_config.channel = 1;
    memcpy(config.sta_config.password, wifiSetting.password,
               WIFI_LENGTH_PASSPHRASE);
    /* Does not include '\0' */
    config.sta_config.password_length = strlen(wifiSetting.password);
    config.sta_config.auth_mode = auth_mode;
    config.sta_config.encrypt_type = encrypt_type;

    config_ext.sta_wep_key_index = 0;
    config_ext.sta_listen_interval_present = 1;
    config_ext.sta_listen_interval = 0;
    config_ext.sta_power_save_mode_present = 1;
    config_ext.sta_power_save_mode = 0;

    memcpy(config.ap_config.ssid, wifiSetting.ap_ssid,
               WIFI_MAX_LENGTH_OF_SSID);
    config.ap_config.ssid_length = strlen(wifiSetting.ap_ssid);
    memcpy(config.ap_config.password, wifiSetting.ap_password,
               WIFI_LENGTH_PASSPHRASE);
    /* Does not include '\0' */
    config.ap_config.password_length = strlen(wifiSetting.ap_password);
    config.ap_config.auth_mode = auth_mode;
    config.ap_config.encrypt_type = encrypt_type;
    config.ap_config.channel = 36;
    config_ext.ap_wep_key_index_present = 1;
    config_ext.ap_wep_key_index = 0;
    config_ext.ap_dtim_interval_present = 1;
    config_ext.ap_dtim_interval = 1;

    config_ext.sta_auto_connect_present = 1;
    config_ext.sta_auto_connect = 1;

    wpa_supplicant_task_init(&config, &config_ext);
    mtk_freertos_api2supp_semphr_take();

    if (init_mode != WIFIMGR_MODE_CLIENT) {
        netif_set_link_up(&ap_netif);
        netif_set_up(&ap_netif);
    } else {
        netif_set_down(&ap_netif);
    }

    if (init_mode != WIFIMGR_MODE_CLIENT) {
        dhcps_init();
    }

    wifi_api_event_trigger(WIFI_PORT_STA,
            WIFI_EVENT_IOT_INIT_COMPLETE, NULL, 0);
}

int
WifiMgr_Init(WIFIMGR_MODE_E init_mode, int mp_mode, WIFI_MGR_SETTING wifiSetting)
{
    int nRet = WIFIMGR_ECODE_NOT_INIT;
    MGR_ERROR("%s %d init_mode %d mp_mode %d \n", __func__, __LINE__, init_mode, mp_mode);

    gWifiMgrSetting.wifiCallback = wifiSetting.wifiCallback;
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_PORT_SECURE, Mgr_wifi_event_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, Mgr_wifi_event_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_P2P_PROVISION_DIS, Mgr_wifi_event_handler);


    nRet = pthread_mutex_init(&mutexScan, NULL);
    if (nRet != 0) {
        MGR_ERROR("mutexScan pthread_mutex_init() fail!\r\n");
        pthread_mutex_destroy(&mutexScan);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        return nRet;
    }

    InitWpaSupplicant(init_mode, mp_mode, wifiSetting);

    wifi_connection_scan_init(g_ap_list, CFG_MAX_NUM_BSS_LIST);
    return nRet;
}

int
WifiMgr_Sta_Connect(const char* ssid, const char* password, const char* secumode)
{
    int nRet = WIFIMGR_ECODE_OK;
    USEC_T tmp_secumode;

    wifi_auth_mode_t auth_mode;
    wifi_encrypt_type_t encrypt_type;

    wifi_config_set_ssid(WIFI_PORT_STA, (char *)ssid, strlen(ssid));
    wifi_config_set_wpa_psk_key(WIFI_PORT_STA, (char *)password, strlen(password));

    tmp_secumode = WifiMgr_Secu_ITE_To_MTK(secumode);
    ite_wifi_sec_2_mtk_auth_encrypt(tmp_secumode, &auth_mode, &encrypt_type);
    wifi_config_set_security_mode(WIFI_PORT_STA, auth_mode, encrypt_type);

    wifi_config_reload_setting();

    return nRet;
}

int
WifiMgr_Sta_Disconnect(void)
{
    wifi_connection_disconnect_ap();
    return WIFIMGR_ECODE_OK;
}

int
WifiMgr_Get_Scan_AP_Info(WIFI_MGR_SCANAP_LIST* pList)
{
    int nApCount = 0;
    /*Todo*/
#if 0
    if (!gWifiMgrVar.WIFI_Init_Ready) {
        MGR_ERROR("!WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }
#endif


    pthread_mutex_lock(&mutexScan);
    while(g_ap_list[nApCount].is_valid && nApCount < CFG_MAX_NUM_BSS_LIST) {
        memcpy(pList[nApCount].apMacAddr, g_ap_list[nApCount].bssid, WIFI_MAC_ADDRESS_LENGTH);
        pList[nApCount].rfQualityRSSI = g_ap_list[nApCount].rssi;
        pList[nApCount].securityMode =  mtk_auth_encrypt_2_ite_wifi_sec(g_ap_list[nApCount].auth_mode, g_ap_list[nApCount].encrypt_type);
        pList[nApCount].channelId =  g_ap_list[nApCount].channel;
        memcpy(pList[nApCount].ssidName, g_ap_list[nApCount].ssid, g_ap_list[nApCount].ssid_length);
        pList[nApCount].ssidName[g_ap_list[nApCount].ssid_length] = '\0';
#if 0
        MGR_INFO("bssid:%02x:%02x:%02x:%02x:%02x:%02x ssid:%s rssi:%d channel:%d secumode %s(a:%d e:%d)\n",
                pList[nApCount].apMacAddr[0], pList[nApCount].apMacAddr[1], pList[nApCount].apMacAddr[2],
                pList[nApCount].apMacAddr[3], pList[nApCount].apMacAddr[4], pList[nApCount].apMacAddr[5],
                pList[nApCount].ssidName, pList[nApCount].rfQualityRSSI, pList[nApCount].channelId,
                secu_str[pList[nApCount].securityMode], g_ap_list[nApCount].auth_mode,  g_ap_list[nApCount].encrypt_type);
#endif
        nApCount += 1;
    }

    pthread_mutex_unlock(&mutexScan);

    return nApCount;
}

int
WifiMgr_Get_WIFI_Mode(void)
{
    uint8_t opmode;

    wifi_config_get_opmode(&opmode);
    switch (opmode) {
        case WIFI_MODE_STA_ONLY:
            {
                return WIFIMGR_MODE_CLIENT;
            }
        case WIFI_MODE_AP_ONLY:
            {
                return WIFIMGR_MODE_SOFTAP;
            }
        case WIFI_MODE_P2P_ONLY:
            {
                return WIFIMGR_MODE_P2P;
            }
        default:
            {
                return WIFIMGR_MODE_MAX;
            }
    }
    return WIFIMGR_MODE_MAX;
}

int
WifiMgr_Get_Sta_Connect_Complete(void)
{
    int rc;
    uint8_t link_status;
    rc = wifi_connection_get_link_status(&link_status);

    if (link_status == WIFI_STATUS_LINK_CONNECTED) {
        rc = 1;
    } else {
        rc = 0;
    }

    return rc;
}

int
WifiMgr_HostAP_First_Start(void)
{
    printf("%s() not supported\n", __func__);
    return WIFIMGR_ECODE_OK;
}


int
WifiMgr_Sta_HostAP_Switch(WIFI_MGR_SETTING wifiSetting)
{
    printf("%s() not supported\n", __func__);
    return WIFIMGR_ECODE_OK;
}

void
WifiMgr_Sta_Switch(int status)
{
    printf("%s() not supported\n", __func__);
}

int
WifiMgr_Get_MAC_Address(uint8_t cMac[])
{
    uint8_t opmode;

    wifi_config_get_opmode(&opmode);
    if (opmode == WIFI_MODE_STA_ONLY) {
        wifi_config_get_mac_address(WIFI_PORT_STA, cMac);
    } else {
        wifi_config_get_mac_address(WIFI_PORT_AP, cMac);
    }
    return WIFIMGR_ECODE_OK;
}

int
WifiMgr_Get_HostAP_Device_Number(void)
{
    uint8_t sta_number = 0;
    int ret = 0;
    wifi_sta_list_t list[WIFI_MAX_NUMBER_OF_STA];

    wifi_connection_get_sta_list(&sta_number, list);

    ret = sta_number;
    return ret;
}

uint32_t WifiMgr_Get_WIFI_IP(void)
{
    uint8_t opmode;
    uint8_t port;
    uint32_t ip;
    wifi_config_get_opmode(&opmode);
    port = (opmode == WIFI_MODE_STA_ONLY)? WIFI_PORT_STA: WIFI_PORT_AP;
    ip =  wifi_config_get_ip_address(port);

    return ip;
}
#endif
