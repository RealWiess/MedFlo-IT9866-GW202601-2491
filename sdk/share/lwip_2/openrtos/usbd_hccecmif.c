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

#if LWIP_ETHERNET

/* get the windows definitions of the following 4 functions out of the way */
#include <stdlib.h>
#include <inttypes.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <malloc.h>
#include <net/if.h>

#include "pktif.h"

#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"

#include "netif/etharp.h"
#include "pktdrv.h"
#if LWIP_AUTOIP
#include "lwip/autoip.h"
#endif
#if LWIP_IGMP
#include "lwip/igmp.h"
#endif

#include "ite/ith.h"
#include "usbhcc/api/api_usbd_cdcecm.h"
#include <semaphore.h>

/* include the port-dependent configuration */
#include "lwipcfg_openrtos.h"

/* Define those to better describe your network interface.
   For now, we use 'e0', 'e1', 'e2' and so on */
#define IFNAME0               'e'
#define IFNAME1               '0'

/* index of the network adapter to use for lwIP */
#ifndef PACKET_LIB_ADAPTER_NR
#define PACKET_LIB_ADAPTER_NR  0
#endif

/* Define PHY delay when "link up" */
#ifndef PHY_LINKUP_DELAY
#define PHY_LINKUP_DELAY       0
#endif

#if LWIP_IGMP
//static err_t usbd_ncm_igmp_mac_filter(struct netif *netif, ip_addr_t *addr, u8_t action);
#endif
/* link state notification macro */
#if NO_SYS
#define NOTIFY_LINKSTATE(netif, linkfunc) linkfunc(netif)
#else  /* NO_SYS*/
#if LWIP_TCPIP_TIMEOUT
#define NOTIFY_LINKSTATE(netif, linkfunc) tcpip_timeout(PHY_LINKUP_DELAY, (sys_timeout_handler)linkfunc, netif)
#else /* LWIP_TCPIP_TIMEOUT */
#define NOTIFY_LINKSTATE(netif, linkfunc) tcpip_callback((tcpip_callback_fn)linkfunc, netif)
#endif /* LWIP_TCPIP_TIMEOUT */
#endif /* NO_SYS*/
//#define AIRPLAY_AUDIO
#ifdef AIRPLAY_AUDIO
static unsigned short gPreSN;
#endif


struct usbd_cdcecm_if {
    sem_t sem_tx;
    struct netif *ecm_netif;
};
static struct usbd_cdcecm_if ifctxt = { 0 };

static volatile int adapter_link_info = LINKEVENT_DOWN;

static enum link_adapter_event
link_adapter(void)
{
    enum link_adapter_event link_event;
    t_usbd_cdcecm_ret rc;

    rc = usbd_cdcecm_is_network_connection_up();
    if(rc == USBD_CDCECM_SUCCESS)
    {
        link_event = LINKEVENT_UP;
    }
    else
    {
        link_event = LINKEVENT_DOWN;
    }

    if (link_event == adapter_link_info)
    {
        link_event = LINKEVENT_UNCHANGED;
    }
    else
    {
        adapter_link_info = link_event;
        printf(" link_event = %d \n", link_event);
    }
    return link_event;
}

/*-----------------------------------------------------------------------------------*/

static void
low_level_init(struct netif *netif)
{
  /* same as string 3 in descriptor */
  char adapter_mac_addr[ETHARP_HWADDR_LEN] = {0x00,0xA0,0x91,0x00,0x92,0x84};
  int rc;

  (void)sem_init(&ifctxt.sem_tx, 0, 1);
  ifctxt.ecm_netif = netif;

  /* change the MAC address to a unique value
     so that multiple ethernetifs are supported */
  //my_mac_addr[ETHARP_HWADDR_LEN - 1] += netif->num;
  /* Copy MAC addr */
  //memcpy(&netif->hwaddr, my_mac_addr, ETHARP_HWADDR_LEN);
  (void)memcpy(&netif->hwaddr, adapter_mac_addr, ETHARP_HWADDR_LEN);
  LWIP_DEBUGF(NETIF_DEBUG, ("pktif: eth_addr %02X%02X%02X%02X%02X%02X\n",netif->hwaddr[0],netif->hwaddr[1],netif->hwaddr[2],netif->hwaddr[3],netif->hwaddr[4],netif->hwaddr[5]));
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

int usbd_cdcecm_tx_cb(t_usbd_cdcecm_ntf ntf)
{
    if (ntf != USBD_CDCECM_NTF_TX)
    {
        return 1;
    }
    
    (void)sem_post(&ifctxt.sem_tx);
    return 0;
}

#define TX_BUF_SIZE     (1536U + 32U)
#define TX_BUF_NUM      3U

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  static uint8_t tx_buffer[TX_BUF_SIZE*TX_BUF_NUM] __attribute__((aligned(32)));
  static volatile uint32_t  tx_buf_index = 0U;

  t_usbd_cdcecm_ret rc;
  struct pbuf *q;
  unsigned char *buffer = tx_buffer+(TX_BUF_SIZE*tx_buf_index);
  unsigned char *ptr;
  struct eth_hdr *ethhdr;
  u16_t tot_len = p->tot_len - ETH_PAD_SIZE;

#if defined(LWIP_DEBUG) && LWIP_NETIF_TX_SINGLE_PBUF
  LWIP_ASSERT("p->next == NULL && p->len == p->tot_len", p->next == NULL && p->len == p->tot_len);
#endif

  /* initiate transfer(); */
  //if (p->tot_len >= sizeof(buffer)) {
  if (p->tot_len > TX_BUF_SIZE) {
    LINK_STATS_INC(link.lenerr);
    LINK_STATS_INC(link.drop);
    snmp_inc_ifoutdiscards(netif);
    return ERR_BUF;
  }
  ptr = buffer;
  for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    /* send data from(q->payload, q->len); */
    LWIP_DEBUGF(NETIF_DEBUG, ("netif: send ptr %p q->payload %p q->len %i q->next %p\n", ptr, q->payload, (int)q->len, q->next));
    if (q == p) {
      memcpy(ptr, &((char*)q->payload)[ETH_PAD_SIZE], q->len - ETH_PAD_SIZE);
      ptr += q->len - ETH_PAD_SIZE;
    } else {
      memcpy(ptr, q->payload, q->len);
      ptr += q->len;
    }
  }
#if 0
	  {
		  unsigned char *data = (unsigned char*)buffer;
		  int i;
		  
		  ithPrintf("\r\nTx<%04d> \n", tot_len);
 		  #if 0
		  for(i=0; i<tot_len; i++)
		  {
              if (!(i % 0x10))
              {
				  ithPrintf("\n");
              }
			  ithPrintf("%02x ", data[i]);
		  }
		  ithPrintf("\n\n");
		  #endif
	  }
#endif

  tx_buf_index = (tx_buf_index + 1U) % TX_BUF_NUM;
  (void)sem_wait(&ifctxt.sem_tx);
  /* signal that packet should be sent(); */
  //if (packet_send(netif->state, buffer, tot_len) < 0) {
  rc = usbd_cdcecm_send_frame((uint8_t * const)buffer, (const uint16_t)tot_len);
  if (rc != USBD_CDCECM_SUCCESS) {
    (void)ithPrintf("[usbd_ecmif] TX send fail! rc=%d \n", rc);
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    snmp_inc_ifoutdiscards(netif);
    return ERR_BUF;
  }
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
  return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct netif *netif, void *packet, int packet_len)
{
  struct pbuf *p, *q;
  int start;
  int length = packet_len;
  struct eth_addr *dest = (struct eth_addr*)packet;
  struct eth_addr *src = dest + 1;
  int unicast;

#if 0
    {
        unsigned char *rx_data = (unsigned char*)packet;
        int i;
        ithPrintf("\r\nRx[%04d] \n", packet_len);
        #if 0
        for(i=0; i<packet_len; i++)
        {
              if (!(i % 0x10))
              {
                  ithPrintf("\n");
              }
              ithPrintf("%02x ", rx_data[i]);
        }
        ithPrintf("\n\n");
        #endif
    }
#endif

  /* MAC filter: only let my MAC or non-unicast through */
  unicast = ((dest->addr[0] & 0x01) == 0);
  if (((memcmp(dest, &netif->hwaddr, ETHARP_HWADDR_LEN)) && unicast) ||
      /* and don't let feedback packets through (limitation in winpcap?) */
      (!memcmp(src, netif->hwaddr, ETHARP_HWADDR_LEN))) {
    /* don't update counters here! */
    return NULL;
  }

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, (u16_t)length + ETH_PAD_SIZE, PBUF_POOL);

  if (p != NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("netif: recv length %i p->tot_len %i\n", length, (int)p->tot_len));
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

/*-----------------------------------------------------------------------------------*/
/*
 * usbd_ecmif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
usbd_ecmif_input(struct netif *netif, void *packet, int packet_len)
{
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  if (packet_len <= 0) 
  {
    return;
  }
  /* move received packet into a new pbuf */
  p = low_level_input(netif, packet, packet_len);
  /* no packet could be read, silently ignore this */
  if (p == NULL) {
    return;
  }

  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = (struct eth_hdr *)p->payload;
  switch (htons(ethhdr->type)) {
  /* IP or ARP packet? */
  case ETHTYPE_IP:
  case ETHTYPE_ARP:
#if PPPOE_SUPPORT
  /* PPPoE packet? */
  case ETHTYPE_PPPOEDISC:
  case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
    /* full packet send to tcpip_thread to process */
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("usbd_ncmif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
    break;

  default:
    LINK_STATS_INC(link.proterr);
    LINK_STATS_INC(link.drop);
    pbuf_free(p);
    p = NULL;
    break;
  }
}

int usbd_cdcecm_rx_cb(t_usbd_cdcecm_ntf ntf)
{
    t_usbd_cdcecm_ret usbd_ecm_rc;
    uint8_t *rx_buf;
    uint32_t rlen;
    int rc = 0;

    if (ntf != USBD_CDCECM_NTF_RX)
    {
        rc = 1;
    }

    if (rc == 0)
    {
        usbd_ecm_rc = usbd_cdcecm_receive_frame(&rx_buf, &rlen);
        if (rlen > 0U)
        {
            //(void)ithPrintf("[usbd_ecm] RX(%d) \n", rlen);

            usbd_ecmif_input(ifctxt.ecm_netif, (void*)rx_buf, (int)rlen);
            (void)usbd_cdcecm_receive_frame_done();
            rc = 0;
        }
        else
        {
            (void)ithPrintf("[usbd_ecm] receive fail! rc:%d, rlen:%" PRIu32 "\n", rc, rlen);
            rc = 2;
        }
    }

    return rc;
}

/*-----------------------------------------------------------------------------------*/
/*
 * usbd_ecmif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
usbd_ecmif_init(struct netif *netif)
{
  static int usbd_ecmif_index;

  int local_index;
  SYS_ARCH_DECL_PROTECT(lev);
  SYS_ARCH_PROTECT(lev);
  local_index = usbd_ecmif_index++;
  SYS_ARCH_UNPROTECT(lev);

  netif->name[0] = IFNAME0;
  netif->name[1] = (char)(IFNAME1 + local_index);
  netif->linkoutput = low_level_output;
#if LWIP_ARP
  netif->output = etharp_output;
#else /* LWIP_ARP */
  netif->output = NULL; /* not used for PPPoE */
#endif /* LWIP_ARP */
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif_set_hostname(netif, "lwip");
#endif /* LWIP_NETIF_HOSTNAME */
#if LWIP_IGMP
  //netif_set_igmp_mac_filter(netif, (netif_igmp_mac_filter_fn)usbd_ncm_igmp_mac_filter);
#endif

  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);

  /* sets link up or down based on current status */
  low_level_init(netif);

  return ERR_OK;
}

void
usbd_ecmif_shutdown(struct netif *netif)
{
  (void)sem_destroy(&ifctxt.sem_tx);
  ifctxt.ecm_netif = NULL;
  adapter_link_info = LINKEVENT_DOWN;
}

void
usbd_ecmif_poll(struct netif *netif)
{
  enum link_adapter_event link_event;

  /* Process the link status change */
  link_event = link_adapter();
  switch (link_event) 
  {
    case LINKEVENT_UP: 
    {
      NOTIFY_LINKSTATE(netif,netif_set_link_up);
      break;
    }
    case LINKEVENT_DOWN: 
    {
      NOTIFY_LINKSTATE(netif,netif_set_link_down);
      break;
    }
  }
}



#endif /* LWIP_ETHERNET */
