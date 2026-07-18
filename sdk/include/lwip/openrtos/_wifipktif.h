# ifdef CFG_NET_LWIP_2

#include "lwip_2/openrtos/_wifipktif.h"

# else
#ifndef __OPENRTOS_WIFIPKTIF_H__
#define __OPENRTOS_WIFIPKTIF_H__
#include "lwip/netif.h"

#define MAX_ETH_DRV_SG 32
#define MAX_ETH_MSG 1540
struct eth_drv_sg {
    unsigned int	buf;
    unsigned int 	len;
};

/*Extern SDIO WIFI driver function*/
extern unsigned char rltk_wlan_running(unsigned char idx);
extern int netif_get_idx(struct netif *pnetif);
extern void rltk_wlan_recv(int idx, struct eth_drv_sg *sg_list, int sg_len);
extern int rltk_wlan_send(int idx, struct eth_drv_sg *sg_list, int sg_len, int total_len);

void ethernetif_recv(struct netif *netif, int total_len);

#endif

# endif