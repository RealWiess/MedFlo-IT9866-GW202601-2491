/**
 * @file
 * Ethernet common functions
 *
 * @defgroup ethernet Ethernet
 * @ingroup callbackstyle_api
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * Copyright (c) 2003-2004 Leon Woestenberg <leon.woestenberg@axon.tv>
 * Copyright (c) 2003-2004 Axon Digital Design B.V., The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 */

#include "lwip/opt.h"

#if LWIP_ARP || LWIP_ETHERNET

#include "netif/ethernet.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"

#include <string.h>

#include "netif/ppp/ppp_opts.h"
#if PPPOE_SUPPORT
#include "netif/ppp/pppoe.h"
#endif /* PPPOE_SUPPORT */

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

const struct eth_addr ethbroadcast = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const struct eth_addr ethzero = {{0, 0, 0, 0, 0, 0}};

#if ETHARP_SUPPORT_VLAN
static
void remove_vlan_header(struct pbuf *p)
{
    struct eth_hdr *eth_hdr = (struct eth_hdr *)p->payload;
    struct eth_vlan_hdr *vlan_hdr = (struct eth_vlan_hdr *)(eth_hdr + 1);

    u16_t move_len = p->len - (((u8_t*)vlan_hdr + sizeof(struct eth_vlan_hdr)) - (u8_t*)p->payload);

    memmove((u8_t*)vlan_hdr, (u8_t*)vlan_hdr + sizeof(struct eth_vlan_hdr), move_len);

    p->len -= sizeof(struct eth_vlan_hdr);
    p->tot_len -= sizeof(struct eth_vlan_hdr);

    eth_hdr->type = vlan_hdr->tpid;
}

#if !defined(ETHARP_VLAN_CHECK_FN)
static int
etharp_vlan_check_fn(struct eth_hdr *ethhdr, struct eth_vlan_hdr *vlan_hdr)
{
  u16_t vlan_id = VLAN_HDR_VID(vlan_hdr);

  /* Accept pbuf during ID 100~200 */
  if (vlan_id >= 100 && vlan_id <= 200) {
    return 1;
  }

  /* Check pbuf priority for QoS */
  #if LWIP_VLAN_PCP
  u8_t priority = VLAN_HRD_PRIO(vlan_hdr);
  if (priority > 5) {
    return 1;
  }
  #endif
}
#endif /* ETHARP_VLAN_CHECK_FN */

err_t
ethernet_add_vlan_tag(struct pbuf *p, u16_t vlan_id)
{
  struct eth_hdr *ethhdr;
  struct eth_vlan_hdr *vlan_hdr;

  if (pbuf_header(p, SIZEOF_VLAN_HDR) != 0) {
    return ERR_MEM;
  }

  ethhdr = (struct eth_hdr *)p->payload;
  vlan_hdr = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);

  MEMCPY(vlan_hdr, ethhdr->dest.addr, 2 * ETH_HWADDR_LEN);

  vlan_hdr->tpid = PP_HTONS(ETHTYPE_VLAN);
  vlan_hdr->prio_vid = PP_HTONS(vlan_id & 0xFFF);

  return ERR_OK;
}

err_t
ethernet_remove_vlan_tag(struct pbuf *p)
{
  struct eth_hdr *ethhdr;
  struct eth_vlan_hdr *vlan_hdr;

  ethhdr = (struct eth_hdr *)p->payload;
  vlan_hdr = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);

  MEMCPY(ethhdr->dest.addr, vlan_hdr, 2 * ETH_HWADDR_LEN);

  if (pbuf_header(p, -SIZEOF_VLAN_HDR) != 0) {
    return ERR_MEM;
  }

  return ERR_OK;
}


#if defined(LWIP_HOOK_VLAN_CHECK)
static int
lwip_vlan_check(struct netif *netif, struct eth_hdr *ethhdr, struct eth_vlan_hdr *vlan_hdr)
{
  u16_t vlan_id = VLAN_HDR_VID(vlan);

  if (vlan_id >= 1 && vlan_id <= 10) {
    return 1;
  }

  if (netif->num == 0 && vlan_id == 100) {
    return 1;
  }

  return 0;
}
#endif
#endif


/**
 * @ingroup lwip_nosys
 * Process received ethernet frames. Using this function instead of directly
 * calling ip_input and passing ARP frames through etharp in ethernetif_input,
 * the ARP cache is protected from concurrent access.<br>
 * Don't call directly, pass to netif_add() and call netif->input().
 *
 * @param p the received packet, p->payload pointing to the ethernet header
 * @param netif the network interface on which the packet was received
 *
 * @see LWIP_HOOK_UNKNOWN_ETH_PROTOCOL
 * @see ETHARP_SUPPORT_VLAN
 * @see LWIP_HOOK_VLAN_CHECK
 */
err_t
ethernet_input(struct pbuf *p, struct netif *netif)
{
  struct eth_hdr *ethhdr;
  u16_t type;
#if LWIP_ARP || ETHARP_SUPPORT_VLAN || LWIP_IPV6
  u16_t next_hdr_offset = SIZEOF_ETH_HDR;
#endif /* LWIP_ARP || ETHARP_SUPPORT_VLAN */

  LWIP_ASSERT_CORE_LOCKED();

  if (p->len <= SIZEOF_ETH_HDR) {
    /* a packet with only an ethernet header (or less) is not valid for us */
    ETHARP_STATS_INC(etharp.proterr);
    ETHARP_STATS_INC(etharp.drop);
    MIB2_STATS_NETIF_INC(netif, ifinerrors);
    goto free_and_return;
  }

  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = (struct eth_hdr *)p->payload;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
              ("ethernet_input: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\n",
               (u8_t)ethhdr->dest.addr[0], (u8_t)ethhdr->dest.addr[1], (u8_t)ethhdr->dest.addr[2],
               (u8_t)ethhdr->dest.addr[3], (u8_t)ethhdr->dest.addr[4], (u8_t)ethhdr->dest.addr[5],
               (u8_t)ethhdr->src.addr[0],  (u8_t)ethhdr->src.addr[1],  (u8_t)ethhdr->src.addr[2],
               (u8_t)ethhdr->src.addr[3],  (u8_t)ethhdr->src.addr[4],  (u8_t)ethhdr->src.addr[5],
               lwip_htons(ethhdr->type)));

  type = ethhdr->type;
#if ETHARP_SUPPORT_VLAN
  if (type == PP_HTONS(ETHTYPE_VLAN)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((char *)ethhdr) + SIZEOF_ETH_HDR);
    next_hdr_offset = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
    if (p->len <= SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) {
      /* a packet with only an ethernet/vlan header (or less) is not valid for us */
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      MIB2_STATS_NETIF_INC(netif, ifinerrors);
      goto free_and_return;
    }
#if defined(LWIP_HOOK_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) /* if not, allow all VLANs */
#ifdef LWIP_HOOK_VLAN_CHECK
    if (!LWIP_HOOK_VLAN_CHECK(netif, ethhdr, vlan)) {
#elif defined(ETHARP_VLAN_CHECK_FN)
    if (!ETHARP_VLAN_CHECK_FN(ethhdr, vlan)) {
#elif defined(ETHARP_VLAN_CHECK)
    if (VLAN_HDR_VID(vlan) != ETHARP_VLAN_CHECK) {
#endif
      /* silently ignore this packet: not for our VLAN */
      pbuf_free(p);
      return ERR_OK;
    }
#endif /* defined(LWIP_HOOK_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) */
    type = vlan->tpid; //packet real eth type
  }
#endif /* ETHARP_SUPPORT_VLAN */

#if LWIP_ARP_FILTER_NETIF
  netif = LWIP_ARP_FILTER_NETIF_FN(p, netif, lwip_htons(type));
#endif /* LWIP_ARP_FILTER_NETIF*/

  if (p->if_idx == NETIF_NO_INDEX) {
    p->if_idx = netif_get_index(netif);
  }

  if (ethhdr->dest.addr[0] & 1) {
    /* this might be a multicast or broadcast packet */
    if (ethhdr->dest.addr[0] == LL_IP4_MULTICAST_ADDR_0) {
#if LWIP_IPV4
      if ((ethhdr->dest.addr[1] == LL_IP4_MULTICAST_ADDR_1) &&
          (ethhdr->dest.addr[2] == LL_IP4_MULTICAST_ADDR_2)) {
        /* mark the pbuf as link-layer multicast */
        p->flags |= PBUF_FLAG_LLMCAST;
      }
#endif /* LWIP_IPV4 */
    }
#if LWIP_IPV6
    else if ((ethhdr->dest.addr[0] == LL_IP6_MULTICAST_ADDR_0) &&
             (ethhdr->dest.addr[1] == LL_IP6_MULTICAST_ADDR_1)) {
      /* mark the pbuf as link-layer multicast */
      p->flags |= PBUF_FLAG_LLMCAST;
    }
#endif /* LWIP_IPV6 */
    else if (eth_addr_cmp(&ethhdr->dest, &ethbroadcast)) {
      /* mark the pbuf as link-layer broadcast */
      p->flags |= PBUF_FLAG_LLBCAST;
    }
  }

  switch (type) {
#if LWIP_IPV4 && LWIP_ARP
    /* IP packet? */
    case PP_HTONS(ETHTYPE_IP):
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
      /* skip Ethernet header (min. size checked above) */
      if (pbuf_remove_header(p, next_hdr_offset)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
                    ("ethernet_input: IPv4 packet dropped, too short (%"U16_F"/%"U16_F")\n",
                     p->tot_len, next_hdr_offset));
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("Can't move over header in packet\n"));
        goto free_and_return;
      } else {
        /* pass to IP layer */
        ip4_input(p, netif);
      }
      break;

    case PP_HTONS(ETHTYPE_ARP):
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
      /* skip Ethernet header (min. size checked above) */
      if (pbuf_remove_header(p, next_hdr_offset)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
                    ("ethernet_input: ARP response packet dropped, too short (%"U16_F"/%"U16_F")\n",
                     p->tot_len, next_hdr_offset));
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("Can't move over header in packet\n"));
        ETHARP_STATS_INC(etharp.lenerr);
        ETHARP_STATS_INC(etharp.drop);
        goto free_and_return;
      } else {
        /* pass p to ARP module */
#if defined(LWIP_MAC_ETH_WIFI) || defined(LWIP_MAC_ECM_WIFI)
        if (netif != netif_default) {
          netif = netif_default;
        }
#endif
        etharp_input(p, netif);
      }
      break;
#endif /* LWIP_IPV4 && LWIP_ARP */
#if PPPOE_SUPPORT
    case PP_HTONS(ETHTYPE_PPPOEDISC): /* PPP Over Ethernet Discovery Stage */
      pppoe_disc_input(netif, p);
      break;

    case PP_HTONS(ETHTYPE_PPPOE): /* PPP Over Ethernet Session Stage */
      pppoe_data_input(netif, p);
      break;
#endif /* PPPOE_SUPPORT */

#if LWIP_IPV6
    case PP_HTONS(ETHTYPE_IPV6): /* IPv6 */
      /* skip Ethernet header */
      if ((p->len < next_hdr_offset) || pbuf_remove_header(p, next_hdr_offset)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
                    ("ethernet_input: IPv6 packet dropped, too short (%"U16_F"/%"U16_F")\n",
                     p->tot_len, next_hdr_offset));
        goto free_and_return;
      } else {
        /* pass to IPv6 layer */
        ip6_input(p, netif);
      }
      break;
#endif /* LWIP_IPV6 */

    default:
#ifdef LWIP_HOOK_UNKNOWN_ETH_PROTOCOL
      if (LWIP_HOOK_UNKNOWN_ETH_PROTOCOL(p, netif) == ERR_OK) {
        break;
      }
#endif
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      MIB2_STATS_NETIF_INC(netif, ifinunknownprotos);
      goto free_and_return;
  }

  /* This means the pbuf is freed or consumed,
     so the caller doesn't have to free it again */
  return ERR_OK;

free_and_return:
  pbuf_free(p);
  return ERR_OK;
}

#if (ETHMAC_SUPPORT_VLAN || ETHARP_SUPPORT_VLAN)

static inline
u8_t get_netif_priority(struct netif *netif)
{
  u8_t pcp = 0;

#if LWIP_VLAN_PCP
  if (netif->hints) {
      pcp = (u8_t)PCP_VAL_GET(netif->hints->tci);
  }
#else
  s32_t *tci_val = (s32_t*)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_VLAN);
  pcp = (u8_t)PCP_VAL_GET(*tci_val);
#endif

  return pcp;
}

struct netif *
find_best_netif(ip4_addr_t *dest_ip, const struct eth_addr *dest_mac) {
  struct netif *best_netif = NULL;
  u8_t best_priority = 0;

  for (u8_t i = 0; i < ARP_TABLE_SIZE; i++) {
    ip4_addr_t *table_ipaddr;
    struct netif *table_netif;
    struct eth_addr *table_ethaddr;

    if (etharp_get_entry(i, &table_ipaddr, &table_netif, &table_ethaddr) == 1) {
      if (ip4_addr_cmp(table_ipaddr, dest_ip) &&
          memcmp(table_ethaddr, dest_mac, sizeof(struct eth_addr)) == 0) {
          return table_netif;
      }
    }
  }

  struct netif *netif;
  NETIF_FOREACH(netif) {
    if (ip4_addr_netcmp(dest_ip, &netif->ip_addr, &netif->netmask)) {
      u8_t priority = get_netif_priority(netif);
      if (priority > best_priority) {
          best_netif = netif;
          best_priority = priority;
      }
    } else if (eth_addr_cmp(dest_mac, &ethbroadcast)) {
      u8_t priority = get_netif_priority(netif);
      if (priority > best_priority) {
          best_netif = netif;
          best_priority = priority;
      }
    }
  }

  if (best_netif == NULL) {
    best_netif = ip4_route(dest_ip);
  }

  return best_netif;
}

#if (ETHMAC_SUPPORT_VLAN || ETHARP_SUPPORT_VLAN)
#if 0
s32_t
my_hook_vlan_set(struct netif* netif, struct pbuf* p, const struct eth_addr* src, const struct eth_addr* dst, u16_t eth_type)
{
    u16_t prio = 0;
    u16_t vid = 0;

    /* Set VLAN ID based on network interface */
    switch(netif->num) {
        case 0:
            vid = 10;
            break;
        case 1:
            vid = netif_default->vlan_id;
            break;
        default:
            /* default VLAN ID */
            vid = 1;
    }

    /* Set priority based on Ethernet type */
    switch(eth_type) {
        case ETHTYPE_IP:
            /* Higher priority for IP traffic */
            prio = 5;
            break;
        case ETHTYPE_ARP:
            /* Highest priority for ARP */
            prio = 7;
            break;
        default:
            /* Default priority */
            prio = 0;
    }

    /*  Special case: If destination is a specific MAC address, don't use VLAN */
    static const struct eth_addr special_mac = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    if (eth_addr_cmp(dst, &special_mac)) {
        return -1; /*  Don't use VLAN tag */
    }

    /* Combine priority and VLAN ID
            Priority in the upper 3 bits, VLAN ID in the lower 12 bits*/
    return ((prio << 13) | vid);
}

#else
s32_t
my_hook_vlan_set(struct netif* netif, struct pbuf* p, const struct eth_addr* src, const struct eth_addr* dst, u16_t eth_type)
{
    struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
    struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
    ip4_addr_t dest_ip;
    u8_t pcp = 0;
    u16_t vid = 0;
    s32_t tci = 0;

#if LWIP_VLAN_PCP
    if (netif->hints) {
        pcp = (u8_t)PCP_VAL_GET(netif->hints->tci);
        vid = (u16_t)VID_VAL_GET(netif->hints->tci);
        tci = netif->hints->tci;
    }
#else
    s32_t *tci_val = (s32_t*)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_VLAN);
    pcp = (u8_t)PCP_VAL_GET(*tci_val);
    vid = (u16_t)VID_VAL_GET(*tci_val);
    tci = *tci_val;
#endif
    printf("[my_hook_vlan_set] PCP 0x%X - VID 0x%X - TCI 0x%X\n", pcp, vid, tci);

    /*  Special case: If destination is a specific MAC address, don't use VLAN */
    static const struct eth_addr special_mac = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    if (eth_addr_cmp(dst, &special_mac)) {
      LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: Special MAC detected, not using VLAN\n"));
      return -1; /* No VLAN tag */
    }

    LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: Set tag 0x%04x (pcp=%u, vid=%u) for netif %c%c\n",
                             tci, pcp, vid, netif->name[0], netif->name[1]));

    switch(eth_type) {
      case ETHTYPE_IP:
        printf("[my_hook_vlan_set] VID: 0x%X - PCP: 0x%X - TCI: 0x%X \n", vid, pcp, TCI_VAL_SET(pcp, vid));
        return tci;
      case ETHTYPE_ARP:
        /* Highest priority for ARP */
        return TCI_VAL_SET(7, vid);
      default:
        LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: Unsupported ethernet type: %04x\n", eth_type));
        return -1;
    }
}
#endif

s32_t
my_hook_vlan_set_2(struct netif* netif, struct pbuf* p, const struct eth_addr* src, const struct eth_addr* dst, u16_t eth_type, struct netif** out_best_netif)
{
    struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
    struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
    ip4_addr_t dest_ip;
    struct netif *best_netif = NULL;
    u8_t pcp = 0;
    u16_t vid = 0;
    s32_t tci = 0;

#if LWIP_VLAN_PCP
    if (netif->hints) {
      pcp = (u8_t)PCP_VAL_GET(netif->hints->tci);
      vid = (u16_t)VID_VAL_GET(netif->hints->tci);
      tci = netif->hints->tci;
    }
#else
    s32_t *tci_val = (s32_t*)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_VLAN);
    pcp = (u8_t)PCP_VAL_GET(*tci_val);
    vid = (u16_t)VID_VAL_GET(*tci_val);
    tci = *tci_val;
#endif

    switch(eth_type) {
        case ETHTYPE_IP:
            ip4_addr_copy(dest_ip, iphdr->dest);
            best_netif = find_best_netif(&dest_ip, dst);
            if (best_netif == NULL) {
                LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: No suitable netif found for IP packet\n"));
                return -1;
            }
            break;
        case ETHTYPE_ARP:
            {
                struct etharp_hdr *arphdr = (struct etharp_hdr *)((u8_t*)ethhdr);
                ip4_addr_t arp_src_ip, arp_target_ip;
                ip_addr_set_zero(&arp_src_ip);
                ip_addr_set_zero(&arp_target_ip);
                IPADDR_WORDALIGNED_COPY_FROM_IP4_ADDR_T(&arp_src_ip, &arphdr->sipaddr);
                IPADDR_WORDALIGNED_COPY_FROM_IP4_ADDR_T(&arp_target_ip, &arphdr->dipaddr);

                best_netif = find_best_netif(&arp_target_ip, NULL);
                if (best_netif == NULL) {
                    best_netif = netif_default;
                    LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: No specific netif found for ARP, using default\n"));
                }
                if (pcp < 7) {
                  pcp = 7; /* Highest priority for ARP */
                  tci = TCI_VAL_SET(pcp, vid); /* renew tci value */
                }
            }
            break;
        default:
            LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: Unsupported ethernet type: %04x\n", eth_type));
            *out_best_netif = NULL;
            return -1;
    }

    /*  Special case: If destination is a specific MAC address, don't use VLAN */
    static const struct eth_addr special_mac = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    if (eth_addr_cmp(dst, &special_mac)) {
        LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: Special MAC detected, not using VLAN\n"));
        *out_best_netif = best_netif;  /* stll return best_netif */
        return -1;
    }

    /* Combine priority and VLAN ID
            Priority in the upper 3 bits, VLAN ID in the lower 12 bits*/
    LWIP_DEBUGF(VLAN_DEBUG, ("VLAN: Set tag 0x%04x (pcp=%u, vid=%u) for netif %c%c\n",
                             tci, pcp, vid, netif->name[0], netif->name[1]));

    *out_best_netif = best_netif;
    return tci;
}
#endif
#endif

/**
 * @ingroup ethernet
 * Send an ethernet packet on the network using netif->linkoutput().
 * The ethernet header is filled in before sending.
 *
 * @see LWIP_HOOK_VLAN_SET
 *
 * @param netif the lwIP network interface on which to send the packet
 * @param p the packet to send. pbuf layer must be @ref PBUF_LINK.
 * @param src the source MAC address to be copied into the ethernet header
 * @param dst the destination MAC address to be copied into the ethernet header
 * @param eth_type ethernet type (@ref lwip_ieee_eth_type)
 * @return ERR_OK if the packet was sent, any other err_t on failure
 */
err_t
ethernet_output(struct netif * netif, struct pbuf * p,
                const struct eth_addr * src, const struct eth_addr * dst,
                u16_t eth_type) {
  struct eth_hdr *ethhdr;
  u16_t eth_type_be = lwip_htons(eth_type);

  /* Insert the VLAN tag for using SW VLAN */
#if ETHARP_SUPPORT_VLAN && (defined(LWIP_HOOK_VLAN_SET) || LWIP_VLAN_PCP)
  s32_t vlan_prio_vid;
#ifdef LWIP_HOOK_VLAN_SET
  vlan_prio_vid = LWIP_HOOK_VLAN_SET(netif, p, src, dst, eth_type);
#elif LWIP_VLAN_PCP
  vlan_prio_vid = -1;
  if (netif->hints && (netif->hints->tci >= 0)) {
    vlan_prio_vid = (u16_t)netif->hints->tci;
  }
#endif
  if (vlan_prio_vid >= 0) {
    struct eth_vlan_hdr *vlanhdr;

    LWIP_ASSERT("prio_vid must be <= 0xFFFF", vlan_prio_vid <= 0xFFFF);

    if (pbuf_add_header(p, SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) != 0) {
      goto pbuf_header_failed;
    }
    vlanhdr = (struct eth_vlan_hdr *)(((u8_t *)p->payload) + SIZEOF_ETH_HDR);
    vlanhdr->tpid     = eth_type_be;
    vlanhdr->prio_vid = lwip_htons((u16_t)vlan_prio_vid);

    eth_type_be = PP_HTONS(ETHTYPE_VLAN);
  } else
#elif ETHMAC_SUPPORT_VLAN && (defined(LWIP_HOOK_VLAN_SET_2) || LWIP_VLAN_PCP)
  s32_t vlan_prio_vid;
  struct netif *best_netif = NULL;
  vlan_prio_vid = LWIP_HOOK_VLAN_SET_2(netif, p, src, dst, eth_type, &best_netif);
#endif /* ETHARP_SUPPORT_VLAN && (defined(LWIP_HOOK_VLAN_SET) || LWIP_VLAN_PCP) */
  {
    if (pbuf_add_header(p, SIZEOF_ETH_HDR) != 0) {
      goto pbuf_header_failed;
    }
  }

  LWIP_ASSERT_CORE_LOCKED();

  ethhdr = (struct eth_hdr *)p->payload;
  ethhdr->type = eth_type_be;
  SMEMCPY(&ethhdr->dest, dst, ETH_HWADDR_LEN);
  SMEMCPY(&ethhdr->src,  src, ETH_HWADDR_LEN);

  LWIP_ASSERT("netif->hwaddr_len must be 6 for ethernet_output!",
              (netif->hwaddr_len == ETH_HWADDR_LEN));
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
              ("ethernet_output: sending packet %p\n", (void *)p));

  /* send the packet */
#if ETHMAC_SUPPORT_VLAN
  if (best_netif != NULL) {
    LWIP_DEBUGF(VLAN_DEBUG | LWIP_DBG_TRACE,
                ("ethernet_output: using best_netif(0x%X) %c%c for output\n",
                 best_netif, best_netif->name[0], best_netif->name[1]));
    return best_netif->linkoutput(best_netif, p);
  } else {
    LWIP_DEBUGF(VLAN_DEBUG | LWIP_DBG_TRACE,
                ("ethernet_output: using original netif %c%c for output\n",
                 netif->name[0], netif->name[1]));
    return netif->linkoutput(netif, p);
  }
#else
  return netif->linkoutput(netif, p);
#endif

pbuf_header_failed:
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
              ("ethernet_output: could not allocate room for header.\n"));
  LINK_STATS_INC(link.lenerr);
  return ERR_BUF;
}

#endif /* LWIP_ARP || LWIP_ETHERNET */
