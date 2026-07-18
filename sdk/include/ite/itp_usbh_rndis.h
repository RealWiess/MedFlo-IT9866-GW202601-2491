#ifndef __ITP_USBH_RNDIS_H__

#include "ite/itp.h"
#include "ith/ith_utility.h"
#include "lwip/netif.h"
#include "usbhcc/api/api_usbh_rndis.h"

//#define itp_rndis_printf(fmt...) do{ithPrintf(fmt);/*usleep(100*1000);*/}while(0)
#define itp_rndis_printf(fmt...) do{}while(0)

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ITP_IOCTL_USBHRNDIS_INIT=1,
    ITP_IOCTL_USBHRNDIS_DEINIT,
    ITP_IOCTL_USBHRNDIS_GETIFCONFIG,
};

struct s_itp_usbh_rndis_ifconfig {
    char name[2];
    ip_addr_t ip_addr;
    ip_addr_t netmask;
    ip_addr_t gw;
    u16_t mtu;
    u8_t hwaddr_len;
    u8_t hwaddr[NETIF_MAX_HWADDR_LEN];
    u8_t flags;
};

struct netif_state_rndis {
    struct netif netif;
    t_nwdriver* nwdriver;
    void* (*tx_buffer_get)(struct netif *netif);
    void  (*tx_buffer_commit)(struct netif *netif, void* buffer, int len);
    void  (*input)(struct netif *netif, void *packet, int packet_len);
};

extern const ITPDevice itpdev_usbh_rndis;
void itp_rndis_hexdump(char* msg, void *mem, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /*__ITP_USBH_RNDIS_H__*/