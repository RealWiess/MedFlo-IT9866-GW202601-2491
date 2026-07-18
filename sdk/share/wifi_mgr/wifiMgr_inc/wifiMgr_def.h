#ifndef WIFIMGR_DEF_H
#define WIFIMGR_DEF_H
#include "wifiMgr_inc/wifiMgr_vnd.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "../dhcps/dhcps.h"
#if defined(_WIFIMGR_API_RTK_)
#include "wifi_conf.h"
#include "wifi_constants.h"
#endif
#include "ite/itp.h"
#include "ite/ite_wifi.h"

#define WIFI_SSID_MAXLEN                 32
#define WIFI_PASSWORD_MAXLEN             256
#define WIFI_SECUMODE_MAXLEN             3
#define WIFI_SCAN_LIST_NUMBER            64
#define WIFI_STACK_SIZE                  80000
#define WIFIMGR_CHECK_WIFI_MSEC          1000
#define WIFIMGR_RECONNTECT_TIME          3
#define WIFIMGR_REOFFER_DHCP_TIME        3

/* Set 0 to Disable */
#define WIFIMGR_EXCUTE_WPS               0
#define WIFIMGR_RECONNTECT_INFINITE      0  //Set(0): Allow 'WIFIMGR_RECONNTECT_TIME'. Set(1):Alway retry to connect
#define WIFIMGR_REMOVE_ALL_SAME_SSID     0  //Set(0): Remove same MAC Addr but have same SSID. Set(1): Remove same SSID no matter with different  MAC.
#define WIFIMGR_SHOW_SCAN_LIST           0

#define WIFI_CONNECT_COUNT               (30 * 10)
#define WIFI_CONNECT_DHCP_COUNT          (20 * 10)
#define WIFIMGR_RECONNECT_MSEC           (60 * WIFIMGR_CHECK_WIFI_MSEC)
#define WIFIMGR_TEMPDISCONN_MSEC         (40 * WIFIMGR_CHECK_WIFI_MSEC)
#define WIFIMGR_DHCP_RENEW_MSEC          (8  * 60 * WIFIMGR_CHECK_WIFI_MSEC)


//ITE WIFI Definition of Encrypt
#define ITE_WIFI_SEC_OPEN                "0"
#define ITE_WIFI_SEC_WEP_PSK             "1"    //WEP
#define ITE_WIFI_SEC_WPA_TKIP_PSK        "2"    //WPA TKIP+PSK
#define ITE_WIFI_SEC_WPA_AES_PSK         "3"    //WPA AES+PSK
#define ITE_WIFI_SEC_WPA2_AES_PSK        "4"    //WPA2 AES+PSK
#define ITE_WIFI_SEC_WPA2_TKIP_PSK       "5"    //WPA2 TKIP+PSK
#define ITE_WIFI_SEC_WPA2_MIXED_PSK      "6"    //WPA2 AES/TKIP PSK
#define ITE_WIFI_SEC_WPA_WPA2_MIXED      "7"    //WPA/WPA2 AES
#define ITE_WIFI_SEC_WPS_OPEN            "8"    //WPS OPEN
#define ITE_WIFI_SEC_WPS_SECURE          "9"    //WPS SECURE
#define ITE_WIFI_SEC_EAP_AES             "10"   //EAP WPA2 AES
#define ITE_WIFI_SEC_WPA_TKIP_8021X      "11"   //WPA 8021X Security with TKIP
#define ITE_WIFI_SEC_WPA_AES_8021X       "12"   //WPA 8021X Security with AES
#define ITE_WIFI_SEC_WPA2_AES_8021X      "13"   //WPA2 8021X Security with AES
#define ITE_WIFI_SEC_WPA2_TKIP_8021X     "14"   //WPA2 8021X Security with TKIP
#define ITE_WIFI_SEC_WPA_WPA2_MIXED_8021X "15"  //WPA/WPA2 8021X Security
#define ITE_WIFI_SEC_WPA3_AES_PSK        "16"   //WPA3-AES with AES security
#define ITE_WIFI_SEC_UNKNOWN             "-1"

#define BOOLEAN_SET_TRUE(BOOL)  (bool)(BOOL = true)
#define BOOLEAN_SET_FALSE(BOOL) (bool)(BOOL = false)

#ifndef WIN32
#define MGR_INFO(fmt, args...)  printf("\n\r[ WIFIMGR ] " fmt, ##args)
#define MGR_ERROR(fmt, args...) printf("\n\r[ WIFIMGR ][ ERROR ][ L#%d ] " fmt, __LINE__, ##args)
#define MGR_TRACE(fmt, args...) printf("\n\r[ WIFIMGR ][ %s ][ L#%d ] " fmt, __FUNCTION__, __LINE__, ##args)
#else
#define MGR_INFO
#define MGR_ERROR
#define MGR_TRACE
#endif

#if defined(_WIFIMGR_API_RTK_)
typedef rtw_security_t  USEC_T;
#elif defined(_WIFIMGR_API_BCM_)
typedef uint32_t        USEC_T;
#elif defined(_WIFIMGR_API_ATBM_)
typedef uint32_t        USEC_T;
#elif defined(_WIFIMGR_API_MTK_)
typedef uint32_t        USEC_T;
#else
typedef char            USEC_T;
#endif


typedef enum tagWIFI_TRYAP_PHASE_E
{
    WIFI_TRYAP_PHASE_NONE = 0,
    WIFI_TRYAP_PHASE_SAME_SSID,
    WIFI_TRYAP_PHASE_EMPTY_SSID,
    WIFI_TRYAP_PHASE_FINISH,
    WIFI_TRYAP_PHASE_MAX,
} WIFI_TRYAP_PHASE_E;

typedef enum tagWIFIMGR_ECODE_E
{
    WIFIMGR_ECODE_FAIL = -1,
    WIFIMGR_ECODE_OK = 0,
    WIFIMGR_ECODE_NOT_INIT,
    WIFIMGR_ECODE_DEVICE_NOT_READY,
    WIFIMGR_ECODE_ALLOC_MEM,
    WIFIMGR_ECODE_SEM_INIT,
    WIFIMGR_ECODE_MUTEX_INIT, /* 5 */
    WIFIMGR_ECODE_NO_LED,
    WIFIMGR_ECODE_NO_WIFI_DEVICE,
    WIFIMGR_ECODE_NO_INI_FILE,
    WIFIMGR_ECODE_NO_SSID,
    WIFIMGR_ECODE_CONNECT_ERROR, /* 10 */
    WIFIMGR_ECODE_DHCP_ERROR,
    WIFIMGR_ECODE_SCAN_ERROR,
    WIFIMGR_ECODE_OPEN_FILE,
    WIFIMGR_ECODE_UNKNOWN_SECURITY,
    WIFIMGR_ECODE_SET_DISCONNECT, /* 15 */
    WIFIMGR_ECODE_INVALID_PARAM,
    WIFIMGR_ECODE_MAX,
} WIFIMGR_ECODE_E;

typedef enum tagWIFIMGR_WORKSTATE_E
{
    WIFIMGR_WORKSTATE_NONINIT = -4,
    WIFIMGR_WORKSTATE_TIMEOUT = -3,
    WIFIMGR_WORKSTATE_CANCEL = -2,
    WIFIMGR_WORKSTATE_FAILED = -1,
    WIFIMGR_WORKSTATE_SUCCESS = 0,
    WIFIMGR_WORKSTATE_STANDBY = 1,
    WIFIMGR_WORKSTATE_WORKING,
    WIFIMGR_WORKSTATE_DONE,
    WIFIMGR_WORKSTATE_RETRY,
    WIFIMGR_WORKSTATE_MAX /* Last Enum */
} WIFIMGR_WORKSTATE_E;

typedef enum tagWIFIMGR_CONNSTATE_E
{
    WIFIMGR_CONNSTATE_STOP = 0,         /* Connection all stop in the way for something happened */
    WIFIMGR_CONNSTATE_DONE = 1,         /* Connection all done and have no error */
    /* Set parameter phase */
    WIFIMGR_CONNSTATE_SETTING,          /* 2 */
    WIFIMGR_CONNSTATE_SET_FAILED,
    WIFIMGR_CONNSTATE_SET_DONE,
    /* Scan phase */
    WIFIMGR_CONNSTATE_SCANNING,         /* 5 */
    WIFIMGR_CONNSTATE_SCAN_FAILED,      /* Connection have an error after scan list is no match */
    WIFIMGR_CONNSTATE_SCAN_DONE,
    /* Connect phase(before DHCP) */
    WIFIMGR_CONNSTATE_CONNECTING,       /* 8 */
    WIFIMGR_CONNSTATE_CONNECT_FAILED,
    WIFIMGR_CONNSTATE_CONNECT_DONE,     /* Connection all complete */
    /* DHCP phase */
    WIFIMGR_CONNSTATE_DHCP_PROCESSING,  /* 11 */
    WIFIMGR_CONNSTATE_DHCP_FAILED,
    WIFIMGR_CONNSTATE_DHCP_DONE,
    /* Retry scene */
    WIFIMGR_CONNSTATE_RETRY_SCAN,       /* 14 */
    WIFIMGR_CONNSTATE_RETRY_CONNECT,

    WIFIMGR_CONNSTATE_MAX
} WIFIMGR_CONNSTATE_E;

typedef enum tagWIFIMGR_STATE_CALLBACK_E
{
    WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH = 0,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_RECONNECTION,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_CANCEL,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_SAVE_INFO,
    WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_CLEAN_INFO,
    WIFIMGR_STATE_CALLBACK_SWITCH_CLIENT_SOFTAP_FINISH,
    WIFIMGR_STATE_CALLBACK_MAX,
} WIFIMGR_STATE_CALLBACK_E;


typedef enum tagWIFIMGR_MODE_E
{
    WIFIMGR_MODE_CLIENT = 0,
    WIFIMGR_MODE_SOFTAP,
    WIFIMGR_MODE_P2P,
    WIFIMGR_MODE_SOFTAP_CLIENT,
    WIFIMGR_MODE_P2P_GO_CLIENT,
    WIFIMGR_MODE_MAX,
} WIFIMGR_MODE_E;

typedef enum tagWIFIMGR_SWTICH_ON_OFF_E
{
    WIFIMGR_SWITCH_OFF = 0,
    WIFIMGR_SWITCH_ON,
} WIFIMGR_SWTICH_ON_OFF_E;


typedef struct WIFI_MGR_SETTING_TAG
{
    int             (*wifiCallback)(int nCondition);
    char            ssid[WIFI_SSID_MAXLEN+1];
    char            password[WIFI_PASSWORD_MAXLEN];
    char            ap_ssid[WIFI_SSID_MAXLEN+1];
    char            ap_password[WIFI_PASSWORD_MAXLEN];
    char            p2p_name[WIFI_SSID_MAXLEN+1];
    USEC_T          secumode;
    ITPEthernetSetting setting;
}WIFI_MGR_SETTING;

typedef struct WIFI_MGR_SCANAP_LIST_TAG
{
    u8_t            name[16];
#if defined(_WIFIMGR_API_RTK_)
    u8_t            apMacAddr[6];
    s16_t           rfQualityRSSI; //RSSI(dBm)
    rtw_security_t  securityMode;
#else
    u8_t            apMacAddr[6+2];
    signed char     rfQualityRSSI; //RSSI(dBm)
    int             securityMode;  /*Sec. Mode*/
#endif
    int             channelId;
    u8_t            ssidName[WIFI_SSID_MAXLEN+1];
    int             operationMode;
    int             securityOn;
    u8_t            rfQualityQuant; //Percent : 0~100
    u8_t            reserved[2];
    int             bitrate;
} WIFI_MGR_SCANAP_LIST;

typedef struct WIFI_MGR_VAR_TAG
{
    /* WIFI Connect Info */
    char            Ssid[WIFI_SSID_MAXLEN+1];
    char            Password[WIFI_PASSWORD_MAXLEN];
    USEC_T          SecurityMode;
    char            Mac_String[32];

    /* WIFI HostAP Mode Info */
    char            ApSsid[WIFI_SSID_MAXLEN+1];
    char            ApPassword[WIFI_PASSWORD_MAXLEN];

    /* WIFI P2P Mode Info */
    char            P2P_name[WIFI_SSID_MAXLEN+1];

    WIFIMGR_MODE_E   WIFI_Mode;
    WIFI_MGR_SETTING WifiMgrSetting;

    u32_t           Client_On_Off;
    int             MP_Mode;

    /* WIFI Flags */
    bool            Need_Set;
    bool            Pre_Scan;
    bool            Start_Scan;
    bool            WIFI_Init_Ready;
    bool            SoftAP_Hidden;
    bool            SoftAP_Init_Ready;
    bool            WIFI_Terminate;
    bool            WPA_Terminate;
    bool            Cancel_Connect;
    bool            Cancel_WPS;
    bool            No_SSID;
    bool            No_Config_File;
    bool            Is_First_Connect;   // first connect when the system start up
    bool            Is_WIFI_Available;
    bool            Is_Temp_Disconnect; // is temporary disconnect
    bool            IS_WifiMgr_Sta_Thread;
    bool            IS_WifiMgr_Switch_Mode_Thread;
    bool            IS_WifiMgr_Main_Process_Thread;
}WIFI_MGR_VAR;

#if defined(_WIFIMGR_API_MTK_)
typedef enum {
    MTK_ITE_WIFI_SEC_OPEN,
    MTK_ITE_WIFI_SEC_WEP_PSK,
    MTK_ITE_WIFI_SEC_WPA_TKIP_PSK,
    MTK_ITE_WIFI_SEC_WPA_AES_PSK,
    MTK_ITE_WIFI_SEC_WPA2_AES_PSK,
    MTK_ITE_WIFI_SEC_WPA2_TKIP_PSK,
    MTK_ITE_WIFI_SEC_WPA2_MIXED_PSK,
    MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_PSK,
    MTK_ITE_WIFI_SEC_WPS_OPEN,
    MTK_ITE_WIFI_SEC_WPS_SECURE,
    MTK_ITE_WIFI_SEC_EAP_AES,
    MTK_ITE_WIFI_SEC_WPA_TKIP_8021X,
    MTK_ITE_WIFI_SEC_WPA_AES_8021X,
    MTK_ITE_WIFI_SEC_WPA2_AES_8021X,
    MTK_ITE_WIFI_SEC_WPA2_TKIP_8021X,
    MTK_ITE_WIFI_SEC_WPA_WPA2_MIXED_8021X,
    MTK_ITE_WIFI_SEC_WPA3_AES_PSK,
    MTK_ITE_WIFI_SEC_UNKNOWN,
} mtk_ite_wifi_secmode_t;
#endif
#endif
