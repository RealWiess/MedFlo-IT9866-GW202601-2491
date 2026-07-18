#ifndef _DIAL_H_
#define _DIAL_H_

#include <stdlib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "ite/itp.h"

typedef enum {
    ECM_DIAL_FAIL            = -1,
    ECM_DIAL_SUCCESS         =  0
} ECM_Callback_Event;

typedef enum {
    ECM_STATUS_BUSY          = -2,
    ECM_STATUS_ERROR         = -1,
    ECM_STATUS_OK            =  0
} ECM_Status_t;

typedef enum {
    ECM_NETWORK_LINK_UP      =  1,
    ECM_NETWORK_LINK_DOWN    =  0
} ECM_Network_t;

typedef enum {
    ERR_AT      =  -1,
    ERR_ATE1    =  -2,
    ERR_CPIN    =  -3,
    ERR_CSQ     =  -4,
    ERR_CGREG   =  -5,
    ERR_COPS    =  -6,
    ERR_CGDCONT =  -7,
    ERR_GTRNDIS =  -8
} ECM_Diak_Err_t;


#if defined(CFG_USB_ECM) || defined(CFG_USBH_CD_CDCECM)
#define ITP_DEVICE_ECM  ITP_DEVICE_ETHERNET
#else // defined(CFG_USB_ECM_EX) || defined(CFG_USBH_CD_CDCECM)
#define ITP_DEVICE_ECM  ITP_DEVICE_ECM_EX
#endif

#ifndef WIN32
#define ECM_INFO(fmt, args...)  printf("\n\r[ ECM ] " fmt, ##args)
#define ECM_ERROR(fmt, args...) printf("\n\r[ ECM ][ ERROR ][ L#%ld ] " fmt, __LINE__, ##args)
#define ECM_TRACE(fmt, args...) printf("\n\r[ ECM ][ %s ][ L#%ld ] " fmt, __FUNCTION__, __LINE__, ##args)
#else
#define ECM_INFO
#define ECM_ERROR
#define ECM_TRACE
#endif

#define ECM_TIME_MSEC_TO_MIN        (60 * 1000)
#define ECM_DHCP_RENEW_TIME         10      /* min */


int  ECM_Init(void);
void ECM_Switch_Netif_Start(const char* name);
void ECM_Switch_Netif_Done(const char* name);
struct netif*
     ECM_Get_Default_Netif(void);
char ECM_Get_Default_Netif_Name(void);
void Switch_Wifi_Netif_Easy(void);
void Switch_Ecm_Netif_Easy(void);
void Switch_Eth_Netif_Easy(void);

ECM_Network_t ECM_Get_Network_Available(void);
#endif
