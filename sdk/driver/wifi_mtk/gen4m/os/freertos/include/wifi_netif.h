#ifndef __WIFI_NETIF_H__
#define __WIFI_NETIF_H__

#include "lwip/netif.h"

typedef struct {
    ip_addr_t ip;
    ip_addr_t netmask;
    ip_addr_t gw;
} ip_info_t;


#define LWIP_STA_MODE (0)
#define LWIP_AP_MODE (1)
#define LWIP_SNIFFER_MODE (2)

#define REQ_IP_MODE_STATIC 0
#define REQ_IP_MODE_DHCP 1

#define IFNAME00 's'
#define IFNAME01 't'
#define IFNAME10 'a'
#define IFNAME11 'p'
#define IFNAME20 'l'
#define IFNAME21 'o'

#ifndef AP_DEFAULT_IPADDR
#define AP_DEFAULT_IPADDR   ("192.168.1.1")
#endif
#ifndef AP_DEFAULT_MASK
#define AP_DEFAULT_MASK     ("255.255.255.0")
#endif
#ifndef AP_DEFAULT_GW
#define AP_DEFAULT_GW       ("192.168.1.254")
#endif
#ifndef PRIMARY_DNS
#define PRIMARY_DNS         ("8.8.8.8")
#endif
#ifndef SECONDARY_DNS
#define SECONDARY_DNS       ("8.8.4.4")
#endif
#ifndef IP_POOL_START
#define IP_POOL_START       ("192.168.1.2")
#endif
#ifndef IP_POOL_END
#define IP_POOL_END         ("192.168.1.20")
#endif
/* for dhcpd */
enum ENUM_STA_IP_MODE {
	STA_IP_MODE_STATIC   = 0,
	STA_IP_MODE_DHCP     = 1
};

enum ENUM_NETIF_TYPE {
	NETIF_TYPE_LOOPBACK = 0,
	NETIF_TYPE_AP       = 1,
	NETIF_TYPE_STA      = 2
};

int lwip_get_netif_name(enum ENUM_NETIF_TYPE netif_type, char *name);
struct netif *netif_find_by_type(enum ENUM_NETIF_TYPE netif_type);

extern struct netif sta_netif;
extern struct netif ap_netif;
extern uint8_t opmode;
void lwip_set_ipmode(uint8_t ipMode);
int8_t lwip_get_ipmode(void);
int lwip_net_ready(void);
int lwip_tcpip_init(void);
void lwip_tcpip_init_post(void);
int lwip_tcpip_deinit(void);
int8_t tcpip_stack_init(int mode, ip_info_t *ip_info, uint8_t *mac);
err_t wlan_if_init(struct netif *netif);
err_t mtk_l2_packet_send(void *packet, uint32_t len, struct netif *netif);
void register_eapol_rx_socket(int eapol_rx_socket);
void unregister_eapol_rx_socket(void);
void register_eapol_rx_socket_dual_intf
(int eapol_rx_socket, int eapol_rx_socket_second);
void unregister_eapol_rx_socket_dual_intf
(int eapol_rx_socket, int eapol_rx_socket_second);
#endif
