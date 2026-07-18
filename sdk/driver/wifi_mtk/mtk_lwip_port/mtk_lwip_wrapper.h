#ifndef _MTK_LWIP_WRAPPER_H_
#define _MTK_LWIP_WRAPPER_H_

#ifdef CFG_NET_LWIP

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#define IP4_ADDR_ANY IP_ADDR_ANY

#define ip4_addr_isany_val(addr1)   ((addr1).addr == IPADDR_ANY)
#define ip4_addr_netcmp ip_addr_netcmp
#define ip4_addr_t ip_addr_t
#define PBUF_RAW_TX PBUF_RAW

#endif
#endif
