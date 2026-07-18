/*
 * Copyright (c) 2001,2002 Florian Schulze.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * pktif.c - This file is part of lwIP pktif
 *
 ****************************************************************************
 *
 * This file is derived from an example in lwIP with the following license:
 *
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "lwip/opt.h"

/* get the windows definitions of the following 4 functions out of the way */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "usbhcc/api/api_nwdriver.h"
#include "netif/etharp.h"
#include "ite/itp_usbh_rndis.h"


/* include the port-dependent configuration */
#include "lwipcfg_openrtos.h"

/* Define those to better describe your network interface.
   For now, we use 'e0', 'e1', 'e2' and so on */
#define IFNAME0               'r'
#define IFNAME1               '0'

static void low_level_init(struct netif *netif);
static void rndisif_input(struct netif *netif, void *packet, int packet_len);
static struct pbuf * rndis_create_pbuf(struct netif *netif, void *packet, int packet_len);
static err_t rndisif_linkoutput(struct netif *netif, struct pbuf *p);

err_t
rndisif_netifinitfn(struct netif *netif)
{
  netif->name[0] = IFNAME0;
  netif->name[1] = (char)(IFNAME1 + 0);
  netif->linkoutput = rndisif_linkoutput;

#if LWIP_ARP
  netif->output = etharp_output;
#else /* LWIP_ARP */
  netif->output = NULL; /* not used for PPPoE */
#endif /* LWIP_ARP */
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif_set_hostname(netif, "lwip");
#endif /* LWIP_NETIF_HOSTNAME */

  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);

  /* sets link up or down based on current status */
  low_level_init(netif);

  return ERR_OK;
}

static void
low_level_init(struct netif *netif)
{
    struct netif_state_rndis* state=(struct netif_state_rndis*)netif->state;
    int rc;
    uint8_t * pmac;

    netif->mtu=state->nwdriver->nwd_prop.nwp_mtu_size;
    state->nwdriver->p_nwd_fn->p_nwfn_get_hw_addr(state->nwdriver, &pmac);
    memcpy(netif->hwaddr, pmac, 6);
    ((struct netif_state_rndis*)netif->state)->input=rndisif_input;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif_set_link_up(netif);
    //tcpip_timeout(0, (sys_timeout_handler)netif_set_link_up, netif);
    //tcpip_callback((tcpip_callback_fn)netif_set_link_up, netif)

    ithPrintf("[i] %s:mac: %02X%02X%02X%02X%02X%02X\n", __FUNCTION__, netif->hwaddr[0],netif->hwaddr[1],netif->hwaddr[2],netif->hwaddr[3],netif->hwaddr[4],netif->hwaddr[5]);
}

static err_t
rndisif_linkoutput(struct netif *netif, struct pbuf *p)
{
    struct netif_state_rndis* state=(struct netif_state_rndis*)netif->state;

    struct pbuf *q;
    unsigned char *buffer;
    unsigned char *ptr;
    struct eth_hdr *ethhdr;
    u16_t tot_len = p->tot_len - ETH_PAD_SIZE;

itp_rndis_printf("[i] ++%s@%d, netif:%08X, state:%08X, tx_buffer_get:%08X\n", __FUNCTION__, __LINE__, netif, state, state->tx_buffer_get);
    buffer = state->tx_buffer_get(netif);
    if (buffer==NULL) {
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
        snmp_inc_ifoutdiscards(netif);
ithPrintf("[X] %s@%d: ERR_BUF\n", __FUNCTION__, __LINE__);
        return ERR_BUF;
    }

    if (p->tot_len > state->nwdriver->nwd_prop.nwp_mtu_size) {
        LINK_STATS_INC(link.lenerr);
        LINK_STATS_INC(link.drop);
        snmp_inc_ifoutdiscards(netif);
ithPrintf("[X] %s@%d: ERR_BUF\n", __FUNCTION__, __LINE__);
        return ERR_BUF;
    }

    ptr = buffer;
    for(q = p; q != NULL; q = q->next) {
        LWIP_DEBUGF(NETIF_DEBUG, ("netif: send ptr %p q->payload %p q->len %i q->next %p\n", ptr, q->payload, (int)q->len, q->next));
        if (q == p) {
            memcpy(ptr, &((char*)q->payload)[ETH_PAD_SIZE], q->len - ETH_PAD_SIZE);
            ptr += q->len - ETH_PAD_SIZE;
        } else {
            memcpy(ptr, q->payload, q->len);
            ptr += q->len;
        }
    }

    state->tx_buffer_commit(netif, buffer, tot_len);

    LINK_STATS_INC(link.xmit);
    snmp_add_ifoutoctets(netif, tot_len);
    ethhdr = (struct eth_hdr *)p->payload;
    if ((ethhdr->dest.addr[0] & 1) != 0) {
        /* broadcast or multicast packet*/
        snmp_inc_ifoutnucastpkts(netif);
    } else {
        /* unicast packet */
        snmp_inc_ifoutucastpkts(netif);
    }

itp_rndis_printf("[i] --%s@%d\n", __FUNCTION__, __LINE__);
    return ERR_OK;
}

static void
rndisif_input(struct netif *netif, void *packet, int packet_len)
{
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  if (packet_len <= 0) {
    return;
  }

  /* move received packet into a new pbuf */
  if (!(p = rndis_create_pbuf(netif, packet, packet_len))) {
ithPrintf("[X] %s@%d\n", __FUNCTION__, __LINE__);
    return;
  }

  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = (struct eth_hdr *)p->payload;
  switch (htons(ethhdr->type)) {
  /* IP or ARP packet? */
  case ETHTYPE_IP:
  case ETHTYPE_ARP:
//itp_rndis_printf("[i] %s@%d:IP/ARP:%08X:%d\n", __FUNCTION__, __LINE__, packet, packet_len);
//itp_rndis_hexdump("[i] IP/ARP\n", packet, packet_len);
    /* full packet send to tcpip_thread to process */
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("rndisif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
    break;

  default:
itp_rndis_printf("[X] %s@%d:??:%08X:%d\n", __FUNCTION__, __LINE__, packet, packet_len);
itp_rndis_hexdump("[X] ??\n", packet, (packet_len>16)?16:packet_len);
    LINK_STATS_INC(link.proterr);
    LINK_STATS_INC(link.drop);
    pbuf_free(p);
    p = NULL;
    break;
  }
}

static struct pbuf *
rndis_create_pbuf(struct netif *netif, void *packet, int packet_len)
{
  struct pbuf *p, *q;
  int start;
  int length = packet_len;
  struct eth_addr *dest = (struct eth_addr*)packet;
  struct eth_addr *src = dest + 1;
  int unicast;

  /* MAC filter: only let my MAC or non-unicast through */
  unicast = ((dest->addr[0] & 0x01) == 0);
  if (((memcmp(dest, &netif->hwaddr, ETHARP_HWADDR_LEN)) && unicast) ||
      /* and don't let feedback packets through (limitation in winpcap?) */
      (!memcmp(src, netif->hwaddr, ETHARP_HWADDR_LEN))) {
    /* don't update counters here! */
    ithPrintf("[X] %s: mac\n", __FUNCTION__);
    itp_rndis_hexdump("", packet, 14);
    return NULL;
  }

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, (u16_t)length + ETH_PAD_SIZE, PBUF_POOL);
  LWIP_DEBUGF(NETIF_DEBUG, ("netif: recv length %i p->tot_len %i\n", length, (int)p->tot_len));
  if (!p) {
    ithPrintf("[X] %s: pbuf_alloc(%i)\n", __FUNCTION__, length);
    itp_rndis_hexdump("", packet, 14);
  }

  if (p != NULL) {
    //ithPrintf("[!] netif: recv length %i p->tot_len %i\n", length, (int)p->tot_len);
    //printf("[!] << %i:%i\n", (int)p->tot_len, length);
    //itp_rndis_hexdump("", packet, 14);
    /* We iterate over the pbuf chain until we have read the entire
       packet into the pbuf. */
    start=0;
    for (q = p; q != NULL; q = q->next) {
      u16_t copy_len = q->len;
      /* Read enough bytes to fill this pbuf in the chain. The
         available data in the pbuf is given by the q->len
         variable. */
      /* read data into(q->payload, q->len); */
      LWIP_DEBUGF(NETIF_DEBUG, ("netif: recv start %i length %i q->payload %p q->len %i q->next %p\n", start, length, q->payload, (int)q->len, q->next));
      if (q == p) {
        LWIP_ASSERT("q->len >= ETH_PAD_SIZE", q->len >= ETH_PAD_SIZE);
        copy_len -= ETH_PAD_SIZE;
        memcpy(&((char*)q->payload)[ETH_PAD_SIZE], &((char*)packet)[start], copy_len);
      } else {
        memcpy(q->payload, &((char*)packet)[start], copy_len);
      }
      start += copy_len;
      length -= copy_len;
      if (length <= 0) {
        break;
      }
    }
    LINK_STATS_INC(link.recv);
    snmp_add_ifinoctets(netif, p->tot_len);
    if (unicast) {
      snmp_inc_ifinucastpkts(netif);
    } else {
      snmp_inc_ifinnucastpkts(netif);
    }
  } else {
    /* drop packet(); */
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
  }

  return p;
}


