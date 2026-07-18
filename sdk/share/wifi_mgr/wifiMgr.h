#ifndef WIFIMGR_H
#define WIFIMGR_H
#include "wifiMgr_inc/wifiMgr_def.h"
#ifndef _WIN32
/* ============= Extern APIs ============= */
extern void         vTaskDelay( const TickType_t xTicksToDelay );
extern TickType_t   xTaskGetTickCount(void);
#if defined (_WIFIMGR_API_RTK_)
extern uint8_t*     LwIP_GetMAC(struct netif *pnetif);
extern uint8_t*     LwIP_GetIP(struct netif *pnetif);
extern uint8_t*     LwIP_GetGW(struct netif *pnetif);
extern uint8_t*     LwIP_GetMASK(struct netif *pnetif);
#elif defined (_WIFIMGR_API_BCM_)
extern void         wifi_set_mode(int nMode);
extern void         wifi_switch(void);
#endif

/* ============= WifiMgr APIs  ============= */

/* Init WIFI mode */
int  WifiMgr_Init(WIFIMGR_MODE_E init_mode, int mp_mode, WIFI_MGR_SETTING wifiSetting);

/* Terminate WIFI mode */
int  WifiMgr_Terminate(void);

int  WifiMgr_Is_WPA_Terminating(void);

/* Get all of  "WIFI_MGR_SCANAP_LIST" and return AP list's conut */
int  WifiMgr_Get_Scan_AP_Info(WIFI_MGR_SCANAP_LIST* pList);

int  WifiMgr_Get_Connect_State(int* conn_state, int* e_code);

int  WifiMgr_Get_Sta_Avalible(void);

int  WifiMgr_Get_Sta_Connect_Complete(void);

int  WifiMgr_Get_Sta_Work_State(int* work_state);

int  WifiMgr_Get_MAC_Address(uint8_t cMac[]);

int  WifiMgr_Get_WIFI_Mode(void);

/* Get number of connecting device to AP */
int  WifiMgr_Get_HostAP_Device_Number(void);

/* Not ready(-1), Ready(0) */
int  WifiMgr_Get_HostAP_Ready(void);

int  WifiMgr_HostAP_First_Start(void);

#if defined (_WIFIMGR_API_RTK_)
uint8_t* WifiMgr_Get_WIFI_MAC(void);

uint8_t* WifiMgr_Get_WIFI_IP(void);

uint8_t* WifiMgr_Get_WIFI_GW(void);

uint8_t* WifiMgr_Get_WIFI_Mask(void);

int  WifiMgr_Concurrent_First_Start(void);

int  WifiMgr_Sta_Reset_MAC(const char *mac);

rtw_security_t WifiMgr_Secu_ITE_To_RTL(const char* ite_security_enum);

char* WifiMgr_Secu_RTL_To_ITE(uint64_t rtl_security_enum);
#elif defined (_WIFIMGR_API_BCM_)
uint32_t WifiMgr_Secu_ITE_To_MHD(const char* ite_security_enum);
char*    WifiMgr_Secu_MHD_To_ITE(uint32_t security);
#elif defined (_WIFIMGR_API_MTK_)
USEC_T WifiMgr_Secu_ITE_To_MTK(const char* ite_security_enum);
uint32_t WifiMgr_Get_WIFI_IP(void);
#endif


// 1: on , 0:off
int WifiMgr_Set_APMode_5G(int nOnOff);

#if defined(CFG_NET_WIFI_SDIO_VND_MHD)
uint32_t WifiMgr_Get_WIFI_IP(void);

uint32_t WifiMgr_Get_WIFI_GW(void);

uint32_t WifiMgr_Get_WIFI_Mask(void);

void WifiMgr_Sta_Wowl_Disable(void);

#endif

#if defined(CFG_NET_WIFI_SDIO_ATBM6031)
uint32_t WifiMgr_Get_WIFI_IP(void);

uint32_t WifiMgr_Get_WIFI_GW(void);

uint32_t WifiMgr_Get_WIFI_Mask(void);

#endif

int  WifiMgr_Sta_Connect(const char* ssid, const char* password, const char* secumode);

int  WifiMgr_Sta_Disconnect(void);

int  WifiMgr_Sta_Sleep_Disconnect(void);

/* Switch "Client to SoftAP" or "SoftAP to Client" */
int  WifiMgr_Sta_HostAP_Switch(WIFI_MGR_SETTING wifiSetting);

void WifiMgr_Sta_Cancel_Connect(void);

void WifiMgr_Sta_Not_Cancel_Connect(void);

/* Check WIFI status: WIFI is turned close(0), WIFI is turned open(1) */
void WifiMgr_Sta_Switch(int status);

int WifiMgr_Secu_8188E_To_ITE(int rtw_security_8188eu);

// disable auto reconnect
int WifiMgr_Disable_Auto_Reconnect(int nDisable);


#if defined (CFG_NET_WIFI_SDIO_NGPL_ATBM6031) || (defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_VND_AIC)|| (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)
/** Wifi P2P **/

// Check if P2P is connected
bool WifiMgr_P2P_IsConnected(void);

// Check if P2P is disconnected
bool WifiMgr_P2P_IsDisconnected(void);

// Set Phone IP of Miracast source side
void WifiMgr_P2P_SetPhoneIP(uint32_t addr);

// Get Phone IP of Miracast source side
uint32_t WifiMgr_P2P_GetPhoneIP(void);

// Get ITE IP of Miracast sink side
uint32_t WifiMgr_P2P_GetIteIP(void);

// As P2P GC
uint32_t WifiMgr_P2P_GC_Start(void);

// As P2P GO
uint32_t WifiMgr_P2P_GO_Start(void);

// Stop p2p connection
uint32_t WifiMgr_P2P_Stop(void);

// Get the status of P2P connection
uint8_t WifiMgr_P2P_Get_Status(void);

unsigned int WifiMgr_P2P_SetPhoneIP_Is_P2PConnected(unsigned char *dev_addr);

#endif

#if(defined CFG_NET_WIFI_SDIO_NGPL_8821CS) || (defined CFG_NET_WIFI_SDIO_NGPL_8733BS)
int WifiMgr_P2P_Switch(WIFIMGR_MODE_E from_mode, WIFIMGR_MODE_E to_mode, WIFI_MGR_SETTING wifiSetting);
#endif

#if(defined CFG_NET_WIFI_SDIO_VND_AIC)
int WifiMgr_P2P_Mirror_Enable();
#endif

#if defined (CFG_NET_WIFI_SDIO_NGPL_AP6256)
// 1: enable 0: disable , default disable
void WifiMgr_P2P_Enable(int nP2p);
#endif



/* Old API Name */
#define wifiMgr_get_connect_state           WifiMgr_Get_Connect_State
#define wifiMgr_get_scan_ap_info            WifiMgr_Get_Scan_AP_Info
#define wifiMgr_get_APMode_Ready            WifiMgr_Get_HostAP_Ready
#define wifiMgr_get_softap_device_number    WifiMgr_Get_HostAP_Device_Number
#define wifiMgr_clientMode_connect_ap       WifiMgr_Sta_Connect
#define wifiMgr_clientMode_disconnect       WifiMgr_Sta_Disconnect
#define wifiMgr_clientMode_sleep_disconnect WifiMgr_Sta_Sleep_Disconnect
#define WifiMgr_clientMode_switch           WifiMgr_Sta_Switch
#define wifiMgr_is_wifi_available           WifiMgr_Get_Sta_Work_State
#define WifiMgr_Sta_Is_Available            WifiMgr_Get_Sta_Work_State
#define wifiMgr_CancelConnect               WifiMgr_Sta_Cancel_Connect
#define wifiMgr_Not_CancelConnect           WifiMgr_Sta_Not_Cancel_Connect
#define wifiMgr_softap_set_hidden           WifiMgr_HostAP_Set_Hidden
#define wifiMgr_init                        WifiMgr_Init
#define wifiMgr_terminate                   WifiMgr_Terminate
#define WifiMgr_firstStartSoftAP_Mode       WifiMgr_HostAP_First_Start
#define WifiMgr_Switch_ClientSoftAP_Mode    WifiMgr_Sta_HostAP_Switch
#define WifiMgr_Is_Wpa_Wifi_Terminating     WifiMgr_Sta_WPA_Terminate_Status
#endif
#endif
