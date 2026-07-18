/*
 * Copyright (C) 2018-2020 AICSemi Ltd.
 *
 * All Rights Reserved
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "lwip/sockets.h"
#include "lwip/netifapi.h"
#include "lwip/ip_addr.h"
#include "lwip/dhcp.h"
#include "sys/socket.h"

#include "fhost_tx.h"
#include "fhost_rx.h"
#include "fhost_cntrl.h"
#include "tx_swdesc.h"
#include "co_list.h"
#include "rtos_al.h"
#include "net_al.h"
#include "log.h"
#include "wifi.h"
#ifdef CONFIG_USB_SUPPORT
#include "usb_port.h"
#endif

#ifndef ETH_ALEN
#define ETH_ALEN (6)
#endif

#define NX_NB_L2_FILTER 2

struct l2_filter_tag
{
    struct netif *net_if;
    int sock;
    struct fhost_cntrl_link *link;
    uint16_t ethertype;
};

static struct l2_filter_tag l2_filter[NX_NB_L2_FILTER] = {0};
static rtos_semaphore l2_semaphore;
static rtos_mutex     l2_mutex;

#define ERR_BUF (-1)
#define ERR_OK (0)

struct net_tx_buf_tag
{
    /// Chained list element
    struct co_list_hdr hdr;
    TCPIP_PACKET_INFO_T pkt_hdr;
    uint8_t buf[1600];
};

#define NET_TXBUF_CNT 40//28
/// List element for the free TX buf
struct co_list net_tx_buf_free_list;
static rtos_mutex net_tx_buf_mutex;
static rtos_semaphore net_tx_buf_sema;
#if defined PLATFORM_ITE_FREERTOS
static rtos_semaphore deliver_sema = NULL;
static uint8_t *deliver_buf = NULL;
static uint32_t deliver_len = 0;
static bool net_inited = false;

struct ecos_send_desc_tag {
    struct co_list_hdr hdr;
    unsigned long key;
};
#define ECOS_TX_DONE_IN_DELIVER 0//1
#define ECOS_SEND_DESC_CNT 80
static rtos_mutex ecos_deliver_trigger_mutex = NULL;
static rtos_mutex ecos_send_desc_free_mutex = NULL;
static struct co_list ecos_send_desc_free_list;
static rtos_mutex ecos_send_desc_post_mutex = NULL;
static struct co_list ecos_send_desc_post_list;
static struct ecos_send_desc_tag ecos_send_desc_pool[ECOS_SEND_DESC_CNT];

#if defined (PLATFORM_ITE_FREERTOS)
struct ecos_send_desc_tag *ITEAIC8800_freertos_send_desc_free_list_dequeue(void);
void ITEAIC8800_freertos_send_desc_free_list_enqueue(struct ecos_send_desc_tag *desc);
uint32_t ITEAIC8800_freertos_send_desc_free_list_cnt(void);

struct ecos_send_desc_tag *ITEAIC8800_freertos_send_desc_post_list_dequeue(void);
void ITEAIC8800_freertos_send_desc_post_list_enqueue(struct ecos_send_desc_tag *desc);
uint32_t ITEAIC8800_freertos_send_desc_post_list_cnt(void);
#endif
#endif

/*
 * FUNCTIONS
 ****************************************************************************************
 */

 #if 0
/// Fake function used to detected too small link encapsulation header length
void p_buf_link_encapsulation_hlen_too_small(void);

/// Declaration of the LwIP checksum computation function
//u16_t lwip_standard_chksum(const void *dataptr, int len);

/**
 ****************************************************************************************
 * @brief Callback used by the networking stack to push a buffer for transmission by the
 * WiFi interface.
 *
 * @param[in] net_if Pointer to the network interface on which the TX is done
 * @param[in] p_buf  Pointer to the buffer to transmit
 *
 * @return ERR_OK upon successful pushing of the buffer, ERR_BUF otherwise
 ****************************************************************************************
 */
static err_t net_if_output(struct netif *net_if, struct pbuf *p_buf)
{
    err_t status = ERR_BUF;

    // Increase the ref count so that the buffer is not freed by the networking stack
    // until it is actually sent over the WiFi interface
    pbuf_ref(p_buf);

    // Push the buffer and verify the status
    if (netif_is_up(net_if) && fhost_tx_start(net_if->state, p_buf, NULL, NULL) == 0)
    {
        status = ERR_OK;
    }
    else
    {
        // Failed to push message to TX task, call pbuf_free only to decrease ref count
        pbuf_free(p_buf);
    }

    return (status);
}
#endif
static char netif_num = 0;
/**
 ****************************************************************************************
 * @brief Callback used by the networking stack to setup the network interface.
 * This function should be passed as a parameter to netifapi_netif_add().
 *
 * @param[in] net_if Pointer to the network interface to setup
 * @param[in] p_buf  Pointer to the buffer to transmit
 *
 * @return ERR_OK upon successful setup of the interface, other status otherwise
 ****************************************************************************************
 */
err_t net_if_init(struct netif *net_if)
{
    err_t status = ERR_OK;
    struct fhost_vif_tag *vif;

    net_if->state = &fhost_env.vif[0];
    /* Initialize interface hostname */
    //net_if->hostname = "AicWlan";

    net_if->name[ 0 ] = 'w';
    net_if->name[ 1 ] = 'l';

    vif = (struct fhost_vif_tag *)net_if->state;

    #if 0
    net_if->output = etharp_output;
    net_if->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;
    net_if->hwaddr_len = ETHARP_HWADDR_LEN;
    net_if->mtu = LLC_ETHER_MTU;
    net_if->linkoutput = net_if_output;
    #endif
    memcpy(&vif->mac_addr, get_mac_address(), 6);
    memcpy(net_if->hwaddr, &vif->mac_addr, 6);

    return status;
}

int net_if_add(net_if_t *net_if,
               const uint32_t *ipaddr,
               const uint32_t *netmask,
               const uint32_t *gw,
               struct fhost_vif_tag *vif)
{
    err_t status;

    #if 0
    status = netifapi_netif_add(net_if,
                               (const ip4_addr_t *)ipaddr,
                               (const ip4_addr_t *)netmask,
                               (const ip4_addr_t *)gw,
                               vif,
                               net_if_init,
                               tcpip_input);
    #endif
    net_if->num  = netif_num++;

    return (status == ERR_OK ? 0 : -1);
}
uint16_t net_ip_chksum(const void *dataptr, int len)
{
    // Simply call the LwIP function
    //return lwip_standard_chksum(dataptr, len);
}

const uint8_t *net_if_get_mac_addr(net_if_t *net_if)
{
    return (uint8_t *)net_if->hwaddr;
}

net_if_t *net_if_find_from_name(const char *name)
{
    return &fhost_env.vif[0].net_if;  // TODO:
}

net_if_t *net_if_find_from_wifi_idx(unsigned int idx)
{
    if ((idx >= NX_VIRT_DEV_MAX) || (fhost_env.vif[idx].mac_vif == NULL))
        return NULL;


    if (fhost_env.vif[idx].mac_vif->type != VIF_UNKNOWN)
    {
        return &fhost_env.vif[idx].net_if;
    }

    return NULL;
}

int net_if_get_name(net_if_t *net_if, char *buf, int len)
{
    if (len > 0)
        buf[0] = net_if->name[0];
    if (len > 1)
        buf[1] = net_if->name[1];
    if (len > 2)
        buf[2] = net_if->num + '0';
    if ( len > 3)
        buf[3] = '\0';

    return 3;
}

int net_if_get_wifi_idx(net_if_t *net_if)
{
    struct fhost_vif_tag *vif;
    int idx;

    if (!net_if)
        return -1;

    vif = (struct fhost_vif_tag *)net_if;
    idx = CO_GET_INDEX(vif, fhost_env.vif);

    /* sanity check */
    if (&fhost_env.vif[idx].net_if == net_if)
        return idx;

    return -1;
}

void net_if_up(net_if_t *net_if)
{
    //netifapi_netif_set_up(net_if);
}

void net_if_down(net_if_t *net_if)
{
    //netifapi_netif_set_down(net_if);
}

void net_if_set_default(net_if_t *net_if)
{
    //netifapi_netif_set_default(net_if);
}

void net_if_set_ip(net_if_t *net_if, uint32_t ip, uint32_t mask, uint32_t gw)
{
    if (!net_if)
        return;
//    netif_set_addr(net_if, (const ip4_addr_t *)&ip, (const ip4_addr_t *)&mask,
//                   (const ip4_addr_t *)&gw);
}

int net_if_get_ip(net_if_t *net_if, uint32_t *ip, uint32_t *mask, uint32_t *gw)
{
    if (!net_if)
        return -1;
#if 0
    if (ip)
        *ip = netif_ip4_addr(net_if)->addr;
    if (mask)
        *mask = netif_ip4_netmask(net_if)->addr;
    if (gw)
        *gw = netif_ip4_gw(net_if)->addr;
#endif
    return 0;
}

int net_if_input(net_buf_rx_t *buf, net_if_t *net_if, void *addr, uint16_t len, net_buf_free_fn free_fn)
{
    struct pbuf* p;
#if 0
    buf->custom_free_function = (pbuf_free_custom_fn)free_fn;
    p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, buf, addr, len);
    ASSERT_ERR(p != NULL);

    if (net_if->input(p, net_if))
    {
        free_fn(buf);
        return -1;
    }
#endif
    return 0;
}

struct fhost_vif_tag *net_if_vif_info(net_if_t *net_if)
{
    return ((struct fhost_vif_tag *)net_if->state);
}

net_buf_tx_t *net_buf_tx_alloc(const uint8_t *payload, uint32_t length)
{
    unsigned int reserved_len = offsetof(struct hostdesc, cfm_cb);

    net_buf_tx_t *buf = rtos_malloc(CO_ALIGN4_HI(sizeof(net_buf_tx_t)) + CO_ALIGN4_HI(length + reserved_len));
    if(!buf) {
        return NULL;
    }
    memset(buf, 0, (sizeof(net_buf_tx_t)));
    uint8_t *payload_buf = (uint8_t *)buf + CO_ALIGN4_HI(sizeof(net_buf_tx_t));
    memcpy((payload_buf + reserved_len), payload, length);
    buf->data_ptr = (payload_buf + reserved_len);
    buf->data_len = length;
    buf->pkt_type = 0xFF;

    //AIC_LOG_PRINTF("%s tcpip %x _buf %x %x\n", __func__, buf, payload_buf, buf->data_ptr);
    //AIC_LOG_PRINTF("nbta:%p/%p\n", buf, buf->data_ptr);

    return buf;
}

void net_buf_tx_info(net_buf_tx_t *buf, uint16_t *tot_len, uint8_t *seg_cnt)
{
    #if 0
    uint8_t  idx;
    uint16_t length = buf->tot_len;

    *tot_len = length;

    idx = 0;
    while (length && buf)
    {
        // Sanity check - the payload shall be in shared RAM
        //ASSERT_ERR(!TST_SHRAM_PTR(buf->payload));

        length -= buf->len;
        idx++;
        // Get info of extra segments if any
        buf = buf->next;
    }

    *seg_cnt = idx;
    if (length != 0)
    {
        // The complete buffer must be included in all the segments
        ASSERT_ERR(0);
    }
    #endif
    *tot_len = buf->data_len;
    *seg_cnt = 1;
}

void net_buf_tx_free(net_buf_tx_t *buf)
{
    if (!buf) {
        return ;
    }
    //AIC_LOG_PRINTF("%s tcpip %x %x %x\n", __func__, buf, buf->data_ptr, buf->pkt_type);
    if (0xFF == buf->pkt_type) {
        //AIC_LOG_PRINTF("nbtf:%p/%p\n", buf, buf->data_ptr);
        rtos_free(buf);
        buf = NULL;
    } else {
        //buf->data_ptr -= (offsetof(struct hostdesc, cfm_cb));
        //buf->data_ptr -= sizeof(struct co_list_hdr);
        uint8_t *list_hdr = (uint8_t *)buf;
        list_hdr -= sizeof(struct co_list_hdr);
        //AIC_LOG_PRINTF("%s buf %x, net_buf %x\n", __func__, list_hdr, buf->data_ptr);

        rtos_mutex_lock(net_tx_buf_mutex, -1);
        co_list_push_back(&net_tx_buf_free_list, (struct co_list_hdr *)(list_hdr));
        rtos_mutex_unlock(net_tx_buf_mutex);
        rtos_semaphore_signal(net_tx_buf_sema, 0);
    }
}

uint32_t net_buf_tx_cnt(void) {
    rtos_mutex_lock(net_tx_buf_mutex, -1);
    uint32_t cnt = co_list_cnt(&net_tx_buf_free_list);
    rtos_mutex_unlock(net_tx_buf_mutex);
    return cnt;
}
static int aic_data_tx(const TCPIP_PACKET_INFO_T *skb)
{
    unsigned char type;

    unsigned int len;
    //TCPIP_NETID_T net_id = skb->net_id;

    TCPIP_PACKET_INFO_T *sent_pkt;

    struct fhost_vif_tag *fhost_vif;

    fhost_vif = &fhost_env.vif[0];

    while (skb) {
        unsigned int reserved_len = offsetof(struct hostdesc, cfm_cb);
        #if 0
        sent_pkt = rtos_malloc(sizeof(TCPIP_PACKET_INFO_T));
        if (!sent_pkt) {
            AIC_LOG_PRINTF("sent_pkt malloc fail\n");
            rtos_task_suspend(1);
            continue;
        }
        uint8_t *tx_buf = rtos_malloc(skb->data_len + reserved_len);
        if (!tx_buf) {
            AIC_LOG_PRINTF("Tx buf malloc fail\n");
            rtos_free(sent_pkt);
            rtos_task_suspend(1);
            retry_cnt++;
            if (retry_cnt >= 5) {
                retry_cnt = 0;
                return 0;
            }
            continue;
        }
        retry_cnt = 0;
        #endif
        #if 0
        __retry:
        rtos_mutex_lock(net_tx_buf_mutex, -1);
        struct net_tx_buf_tag *tx_buf = (struct net_tx_buf_tag *)co_list_pop_front(&net_tx_buf_free_list);
        rtos_mutex_unlock(net_tx_buf_mutex);
        if (!tx_buf) {
            rtos_task_suspend(1);
            retry_cnt++;
            if (retry_cnt >= 5) {
                retry_cnt = 0;
                AIC_LOG_PRINTF("%s get buffer fail\n", __func__);
                return 0;
            }
            goto __retry;
        }
        #else
        int ret = rtos_semaphore_wait(net_tx_buf_sema, 100);
        struct net_tx_buf_tag *tx_buf = NULL;
        if (ret == 0) {
            rtos_mutex_lock(net_tx_buf_mutex, -1);
            tx_buf = (struct net_tx_buf_tag *)co_list_pop_front(&net_tx_buf_free_list);
            rtos_mutex_unlock(net_tx_buf_mutex);
        }
        if (!tx_buf) {
            AIC_LOG_PRINTF("%s get buffer fail, ret=%d\n", __func__, ret);
            return 0;
        }
        #endif
        memcpy((tx_buf->buf + reserved_len), skb->data_ptr, skb->data_len);
        sent_pkt = (TCPIP_PACKET_INFO_T *)&(tx_buf->pkt_hdr);
        sent_pkt->data_ptr = tx_buf->buf + reserved_len;
        sent_pkt->data_len = skb->data_len;
        sent_pkt->net_id   = skb->net_id;
        sent_pkt->pkt_type = skb->pkt_type;
        //AIC_LOG_PRINTF("%s pkt %x %x %x, %x\n", __func__, sent_pkt, tx_buf->buf, sent_pkt->data_ptr, sent_pkt->pkt_type);

        fhost_tx_start(&fhost_vif->net_if, sent_pkt, NULL, NULL);

        skb = skb->frag_next;
    }

    //aic_dbg("#*Ao\n");

    return 0;
}

int net_init(void)
{
    #if defined PLATFORM_ITE_FREERTOS
    if (net_inited) {
        return 0;
    }
    #endif

    if (rtos_semaphore_create(&l2_semaphore, "l2_semaphore", 1, 0))
    {
        ASSERT_ERR(0);
    }

        if (rtos_mutex_create(&l2_mutex, "l2_mutex"))
        {
            ASSERT_ERR(0);
        }
    // Initial free tx buf
    co_list_init(&net_tx_buf_free_list);
    if (rtos_semaphore_create(&net_tx_buf_sema, "net_tx_buf_sema", NET_TXBUF_CNT, 0)) {
        ASSERT_ERR(0);
    }
    uint8_t i;
    for(i = 0; i < NET_TXBUF_CNT; i++)
    {
        struct net_tx_buf_tag *net_tx_buffer = rtos_malloc(sizeof(struct net_tx_buf_tag));
        if(net_tx_buffer) {
            //AIC_LOG_PRINTF("%s net_buf %d %x\n", __func__, i, (net_tx_buffer));
            co_list_push_back(&net_tx_buf_free_list, (struct co_list_hdr *)(net_tx_buffer));
            rtos_semaphore_signal(net_tx_buf_sema, 0);
        }
    }
    AIC_LOG_PRINTF("net_tx_buf_sema initial count:%d\n", rtos_semaphore_get_count(net_tx_buf_sema));
    if (rtos_mutex_create(&net_tx_buf_mutex, "net_tx_buf_mutex"))
    {
        ASSERT_ERR(0);
    }

    #if defined PLATFORM_ITE_FREERTOS
    net_inited = true;
    #endif

    return 0;
}

extern struct rwnx_hw *cntrl_rwnx_hw;
int rx_eth_data_process(unsigned char *pdata,
                     unsigned short len)
{
#if defined PLATFORM_ITE_FREERTOS
    #ifdef CONFIG_RX_TRIGGER_TS
    uint32_t start_time = 0;
    uint32_t end_time = 0;
    static volatile uint8_t count = 0;
    #endif

    #ifdef CONFIG_RX_TRIGGER_TS
    count++;
    start_time = cyg_time_get_ms();
    aic_dbg("rts:%u/%u\n", start_time, count);
    #endif

    #ifdef CONFIG_FHOST_RX_ASYNC
    if (fhost_rx_async_post_cnt(true) > 0) {
        //ITEAIC8800_freertos_deliver_trigger();
        int fhost_vif_idx = 0;
       	ethernetif_recv(&fhost_env.vif[fhost_vif_idx].net_if, len); 
    }
    #else
    rtos_mutex_lock(ecos_deliver_trigger_mutex, -1);
    ITEAIC8800_freertos_deliver_buf_set(pdata, len);
    ITEAIC8800_freertos_deliver_trigger();
    rtos_mutex_unlock(ecos_deliver_trigger_mutex);
    ITEAIC8800_freertos_deliver_wait_done(-1);
    
    #endif

    #ifdef CONFIG_RX_TRIGGER_TS
    end_time = cyg_time_get_ms();
    aic_dbg("rte:%u/%u\n", end_time, count);
    #endif
    return 0;
#endif
}

static void net_l2_send_cfm(uint32_t frame_id, bool acknowledged, void *arg)
{
    if (arg)
        *((bool *)arg) = acknowledged;
    rtos_semaphore_signal(l2_semaphore, false);
}

int net_l2_send(net_if_t *net_if, const uint8_t *data, int data_len, uint16_t ethertype,
                const uint8_t *dst_addr, bool *ack)
{
    int res;

    net_buf_tx_t *net_buf;

    if (net_if == NULL || data == NULL /* || data_len >= net_if->mtu || !netif_is_up(net_if) */)
        return -1;

    net_buf = net_buf_tx_alloc((data - sizeof(struct mac_eth_hdr)), (data_len + sizeof(struct mac_eth_hdr)));
    if (net_buf == NULL)
        return 0;

    if (dst_addr)
    {
        // Need to add ethernet header as fhost_tx_start is called directly
        struct mac_eth_hdr* ethhdr;
        ethhdr = (struct mac_eth_hdr*)net_buf->data_ptr;
        ethhdr->type = htons(ethertype);
        memcpy(&ethhdr->da, dst_addr, 6);
        memcpy(&ethhdr->sa, net_if->hwaddr, 6);
    }

    // Ensure no other thread will program a L2 transmission while this one is waiting
    // for its confirmation
    rtos_mutex_lock(l2_mutex, -1);

    //AIC_LOG_PRINTF("%s pkt %x %x, t:%x\n", __func__, net_buf, net_buf->data_ptr, net_buf->pkt_type);
    // In order to implement this function as blocking until the completion of the frame
    // transmission, directly call fhost_tx_start with a confirmation callback.
    res = fhost_tx_start(net_if->state, net_buf, net_l2_send_cfm, ack);

    // Wait for the transmission completion
    rtos_semaphore_wait(l2_semaphore, -1);

    // Now new L2 transmissions are possible
    rtos_mutex_unlock(l2_mutex);

    return res;
}

int net_l2_socket_create(net_if_t *net_if, uint16_t ethertype)
{
    struct l2_filter_tag *filter = NULL;
    int i;
    struct fhost_cntrl_link *link;

    /* First find free filter and check that socket for this ethertype/net_if couple
       doesn't already exists */
    for (i = 0; i < NX_NB_L2_FILTER; i++)
    {
        if ((l2_filter[i].net_if == net_if) &&
            (l2_filter[i].ethertype == ethertype))
        {
            return -1;
        }
        else if ((filter == NULL) && (l2_filter[i].net_if == NULL))
        {
            filter = &l2_filter[i];
        }
    }

    if (!filter)
        return -1;
    // Open link with cntrl task to send cfgrwnx commands and retrieve events
    link = fhost_cntrl_cfgrwnx_link_open();
    if (link == NULL)
        return -1;

    filter->link = link;
    filter->sock = link->sock_recv;
    if (filter->sock == -1)
        return -1;

    filter->net_if = net_if;
    filter->ethertype = ethertype;

    return filter->sock;
}

int net_l2_socket_delete(int sock)
{
    int i;
    for (i = 0; i < NX_NB_L2_FILTER; i++)
    {
        if ((l2_filter[i].net_if != NULL) &&
            (l2_filter[i].sock == sock))
        {
            l2_filter[i].net_if = NULL;
            fhost_cntrl_cfgrwnx_link_close(l2_filter[i].link);
            l2_filter[i].sock = -1;
            return 0;
        }
    }

    return -1;
}

err_t net_eth_receive(unsigned char *pdata, unsigned short len, struct netif *netif)
{
    struct l2_filter_tag *filter = NULL;
    struct mac_eth_hdr* ethhdr = pdata;
    uint16_t ethertype = ntohs(ethhdr->type);
    int i;

    for (i = 0; i < NX_NB_L2_FILTER; i++)
    {
        if ((l2_filter[i].net_if == netif) &&
            (l2_filter[i].ethertype == ethertype))
        {
            filter = &l2_filter[i];
            break;
        }
    }

    if (!filter)
        return -1;

    if (send(filter->link->sock_send, pdata, len, 0) < 0)
    {
        AIC_LOG_PRINTF("Err: %s len %d\n", __func__, len);
        return -1;
    }
    return ERR_OK;
}

static int net_dhcp_started = 0;
int net_dhcp_start(int net_id)
{
    int ret = 0;

    return ret;
}

void net_dhcp_stop(net_if_t *net_if)
{
    #if 0//LWIP_IPV4 && LWIP_DHCP
    netifapi_dhcp_stop(net_if);
    net_dhcp_started = 0;
    #endif //LWIP_IPV4 && LWIP_DHCP
}

int net_dhcp_start_status(void)
{
    return net_dhcp_started;
}

int net_dhcp_release(net_if_t *net_if)
{
    #if 0//LWIP_IPV4 && LWIP_DHCP
    if (netifapi_dhcp_release(net_if) ==  ERR_OK)
        return 0;
    #endif //LWIP_IPV4 && LWIP_DHCP
    return -1;
}

int net_dhcp_address_obtained(net_if_t *net_if)
{
    #if 0//LWIP_IPV4 && LWIP_DHCP
    if (dhcp_supplied_address(net_if))
        return 0;
    #endif //LWIP_IPV4 && LWIP_DHCP
    return -1;
}

int net_set_dns(uint32_t dns_server)
{
    #if 0//LWIP_DNS
    ip_addr_t ip;
    ip_addr_set_ip4_u32(&ip, dns_server);
    dns_setserver(0, &ip);
    return 0;
    #else
    return -1;
    #endif
}

int net_get_dns(uint32_t *dns_server)
{
    #if 0//LWIP_DNS
    const ip_addr_t *ip;

    if (dns_server == NULL)
        return -1;

    ip = dns_getserver(0);
    *dns_server = ip_addr_get_ip4_u32(ip);
    return 0;
    #else
    return -1;
    #endif
}


char* aic_inet_ntoa(struct in_addr addr)
{
  return inet_ntoa(addr);
}

extern int ITEAIC8800_usb_wifi_init(void);
static char stack[4096];
static rtos_task_handle rtos_handle = NULL;

extern int aic_wifi_init(int mode, int chip_id, void *param);

void ITEAIC8800_sdio_wifi_init(void) 
{
    static int retry_count = 0;

    ITEAIC8800_freertos_deliver_init();
    ITEAIC8800_freertos_send_init();

    struct aic_sta_cfg cfg = {0};

	AIC_WIFI_MODE mode = WIFI_MODE_STA;
	aic_wifi_set_mode(mode);
	aic_wifi_init(mode, 0, &cfg);
	aic_dbg("ITEAIC8800_sdio_wifi_init done\n");

	vTaskDelete(NULL);
}

void ITEAIC8800_freertos_init(void) 
{
    #ifdef CONFIG_USB_SUPPORT
    rtos_task_create(ITEAIC8800_usb_wifi_init, "ITEAIC8800_usb_wifi_init", 1,
                         sizeof(stack), NULL, USB_RX_PRIORITY,
                         &rtos_handle);
    #elif CONFIG_SDIO_SUPPORT
    rtos_task_create(ITEAIC8800_sdio_wifi_init, "ITEAIC8800_sdio_wifi_init", 1,
                         sizeof(stack)*2, NULL, sdio_datrx_priority,
                         &rtos_handle);
    #endif
	aic_dbg("ITEAIC8800_freertos_init done\n");
}

// command: if up

bool send_flow_cntl_tx_buf = false;
bool send_flow_cntl_tx_urb = false;
bool send_flow_cntl_tx_desc = false;
int ITEAIC8800_freertos_can_send(void)
{
    int ret = 1;
    uint32_t cnt;
    //TRACE_IN();
    #ifdef CONFIG_USB_SUPPORT
    cnt = aicwf_usb_tx_buf_cnt(g_aic_usb_dev);
    if (cnt <= NET_TXBUF_CNT) {
        send_flow_cntl_tx_urb = true;
        if (cnt == 0)
            printf("urb cnt is 0\n");
        ret = 0;
        goto exit;
    } else if (cnt < NET_TXBUF_CNT + 10) {
        if (send_flow_cntl_tx_urb) {
            ret = 0;
            goto exit;
        } else {
            ret = 1;
        }
    } else {
        send_flow_cntl_tx_urb = false;
        ret = 1;
    }
    #endif
    #if ECOS_TX_DONE_IN_DELIVER
    cnt = ITEAIC8800_freertos_send_desc_free_list_cnt();
    if (cnt <= 3) {
        send_flow_cntl_tx_desc = true;
        if (cnt == 0)
            printf("send_desc cnt is 0\n");
        ret = 0;
        goto exit;
    } else if (cnt < 15) {
        if (send_flow_cntl_tx_desc) {
            ret = 0;
            goto exit;
        } else {
            ret = 1;
        }
    } else {
        send_flow_cntl_tx_desc = false;
        ret = 1;
    }
    #endif
    cnt = fhost_tx_free_list_cnt(false);
    if (cnt <= NET_TXBUF_CNT) {
        if (cnt == 0)
            printf("tx desc cnt is 0\n");
        ret = 0;
        goto exit;
    }
    
    cnt = net_buf_tx_cnt();

    if (cnt <= 3) {
        send_flow_cntl_tx_buf = true;
        if (cnt == 0)
            printf("tx buf cnt is 0\n");
        ret = 0;
    } else if (cnt < 15) {
        if (send_flow_cntl_tx_buf) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        send_flow_cntl_tx_buf = false;
        ret = 1;
    }
    //TRACE_OUT();

exit:
    return ret;
}

typedef struct {
    u_long cmd;
    struct ifreq * data;
} ifreq_priv_driver;

struct eth_drv_sg {
    unsigned int	buf;
    unsigned int 	len;
};
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif

/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	u_char	ether_dhost[ETHER_ADDR_LEN];
	u_char	ether_shost[ETHER_ADDR_LEN];
	u_short	ether_type;
} __attribute__ ((aligned(1), packed));

int SPAPP_AIC8800_ioctl(struct ifreq *rq, int cmd)
{
    int ret               = 0;
    uint32_t org_len      = 0;
    struct fhost_vif_tag *fhost_vif;
    fhost_vif = &fhost_env.vif[0];

    /*
    struct iwreq *wrqin   = (struct iwreq *)rq;
    PRIV_IOCTL_INPUT_STRUCT rt_wrq, *wrq = &rt_wrq;
    wrq->u.data.pointer = wrqin->u.data.pointer;
    wrq->u.data.length  = wrqin->u.data.length;
    org_len             = wrq->u.data.length;
    */

    switch (cmd) {
		/*
        case ETH_DRV_GET_IF_STATS_UD:
        case ETH_DRV_GET_IF_STATS:
        {
            struct ifreq *ifr = (struct ifreq *)rq;
            ifreq_priv_driver * data_priv = (ifreq_priv_driver *)ifr->ifr_data;
            if (data_priv) { 
                struct iwreq * iwr = (struct iwreq *)data_priv->data;
                TCPIP_PACKET_INFO_T *sent_pkt = NULL;
            
                sent_pkt = net_buf_tx_alloc((uint8_t *)iwr->u.data.pointer, iwr->u.data.length);
                if (sent_pkt) {
                    fhost_tx_start(&fhost_vif->net_if, sent_pkt, NULL, NULL);
                } else {
                    printf("tx buf alloc failed\n");
                }
            }
            break;
        }
        
        case SIOCGIFHWADDR:
            aic_dbg("IOCTL:SIOCGIFHWADDR\n");
            sprintf((char *)wrq->u.data.pointer, "%02x:%02x:%02x:%02x:%02x:%02x",
                        usbwifi_reg_ctx[wifi_type].usb_wifi_get_mac_accord_index(net_dev, 0),
                        usbwifi_reg_ctx[wifi_type].usb_wifi_get_mac_accord_index(net_dev, 1),
                        usbwifi_reg_ctx[wifi_type].usb_wifi_get_mac_accord_index(net_dev, 2),
                        usbwifi_reg_ctx[wifi_type].usb_wifi_get_mac_accord_index(net_dev, 3),
                        usbwifi_reg_ctx[wifi_type].usb_wifi_get_mac_accord_index(net_dev, 4),
                        usbwifi_reg_ctx[wifi_type].usb_wifi_get_mac_accord_index(net_dev, 5));

            wrq->u.data.length += ETH_ALEN * 2 + 5;
            ret = 0;
            break;
        
        case SIOCGIFBSSID:
        {
            aic_dbg("IOCTL:SIOCGIFBSSID\n");
            #if 0
            ret = usbwifi_sta_ioctl_cmd_get_apaddr(sc, wrq);
            #else
            aic_dbg("err SIOCGIFBSSID\n");
            #endif
            break;
        }
        case SIOCGIFSTATE:
        {
            aic_dbg("IOCTL:SIOCGIFSTATE\n");
            #if 0
            ret = usbwifi_sta_ioctl_cmd_get_state(sc, wrq);
            #else
            aic_dbg("err SIOCGIFSTATE\n");
            #endif
            break;
        }
       
        case RTPRIV_IOCTL_SET:
        {
            aic_dbg("IOCTL:RTPRIV_IOCTL_SET\n");
            char *substr       = NULL;
            char *string_cmd   = (char *)wrq->u.data.pointer;
            char *strcmd_value = NULL;

            if ((substr = strstr(string_cmd, "AuthMode=")) != NULL) {
                strcmd_value = string_cmd + os_strlen("AuthMode=");
                if (!os_strcmp(strcmd_value, "WPA2PSK")) {
                    os_memcpy(gxapp_ap_info_g.ENCRPTY, "wpa2", 4);
                } else if (!os_strcmp(strcmd_value, "WPAPSK")) {
                    os_memcpy(gxapp_ap_info_g.ENCRPTY, "wpa1", 4);
                } else if (!os_strcmp(strcmd_value, "SHARED")) {
                    os_memcpy(gxapp_ap_info_g.ENCRPTY, "wep", 3);
                } else if (!os_strcmp(strcmd_value, "OPEN")) {
                    os_memcpy(gxapp_ap_info_g.ENCRPTY, "free", 4);
                } else {
                    diag_printf("iwpriv command unknow!!!\n");
                    ret = -1;
                    break;
                }
                ret = 0;
                break;
            }

            if ((substr = strstr(string_cmd, "BSSID=")) != NULL) {
                strcmd_value = string_cmd + os_strlen("BSSID=");
                sprintf(gxapp_ap_info_g.BSSID, "%s", strcmd_value);

                #if 0
                usbwifi_disconnect_ap(sc);
                if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "free")) {
                    ret = usbwifi_wpa_connect_ap(sc, NULL, gxapp_ap_info_g.BSSID, NULL, AUTHMODE_OPEN);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wep")) {
                    ret = usbwifi_wpa_connect_ap(sc, NULL, gxapp_ap_info_g.BSSID, gxapp_ap_info_g.PSK, AUTHMODE_SHARED);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wpa1")) {
                    ret = usbwifi_wpa_connect_ap(sc, NULL, gxapp_ap_info_g.BSSID, gxapp_ap_info_g.PSK, AUTHMODE_WPA);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wpa2")) {
                    ret = usbwifi_wpa_connect_ap(sc, NULL, gxapp_ap_info_g.BSSID, gxapp_ap_info_g.PSK, AUTHMODE_WPA2);
                } else {
                    diag_printf("iwpriv command unknow!!!\n");
                    ret = -1;
                    break;
                }
                os_memset(&gxapp_ap_info_g, 0, sizeof(gxapp_ap_info_g));
                #else
                diag_printf("iwpriv command unknow!!!\n");
                ret = -1;
                #endif
                break;
            }

            if ((substr = strstr(string_cmd, "SSID=")) != NULL) {
                strcmd_value = string_cmd + os_strlen("SSID=");
                sprintf(gxapp_ap_info_g.SSID, "%s", strcmd_value);

                #if 0
                usbwifi_disconnect_ap(sc);
                if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "free")) {
                    ret = usbwifi_wpa_connect_ap(sc, gxapp_ap_info_g.SSID, NULL, NULL, AUTHMODE_OPEN);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wep")) {
                    ret = usbwifi_wpa_connect_ap(sc, gxapp_ap_info_g.SSID, NULL, gxapp_ap_info_g.PSK, AUTHMODE_SHARED);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wpa1")) {
                    ret = usbwifi_wpa_connect_ap(sc, gxapp_ap_info_g.SSID, NULL, gxapp_ap_info_g.PSK, AUTHMODE_WPA);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wpa2")) {
                    ret = usbwifi_wpa_connect_ap(sc, gxapp_ap_info_g.SSID, NULL, gxapp_ap_info_g.PSK, AUTHMODE_WPA2);
                } else {
                    diag_printf("iwpriv command unknow!!!\n");
                    ret = -1;
                    break;
                }
                os_memset(&gxapp_ap_info_g, 0, sizeof(gxapp_ap_info_g));
                #else
                wlan_disconnect_sta(0);
                if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "free")) {
                    ret = wlan_start_sta(gxapp_ap_info_g.SSID, "", -1);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wep")) {
                    ret = wlan_start_sta(gxapp_ap_info_g.SSID, gxapp_ap_info_g.PSK, -1);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wpa1")) {
                    ret = wlan_start_sta(gxapp_ap_info_g.SSID, gxapp_ap_info_g.PSK, -1);
                } else if (!os_strcmp(gxapp_ap_info_g.ENCRPTY, "wpa2")) {
                    ret = wlan_start_sta(gxapp_ap_info_g.SSID, gxapp_ap_info_g.PSK, -1);
                } else {
                    diag_printf("iwpriv command unknow!!!\n");
                    ret = -1;
                    break;
                }
                #endif
                break;
            }

            if ((substr = strstr(string_cmd, "WPAPSK=")) != NULL) {
                strcmd_value = string_cmd + os_strlen("WPAPSK=");
                sprintf(gxapp_ap_info_g.PSK, "%s", strcmd_value);
                ret = 0;
                break;
            }

            if ((substr = strstr(string_cmd, "Key1=")) != NULL) {
                strcmd_value = string_cmd + os_strlen("Key1=");
                sprintf(gxapp_ap_info_g.PSK, "%s", strcmd_value);
                ret = 0;
                break;
            }

            if ((substr = strstr(string_cmd, "Disconnect")) != NULL) {
                #if 0
                ret = usbwifi_disconnect_ap(sc);
                #else
                ret  = wlan_disconnect_sta(0);
                #endif
                break;
            }

            if((substr = strstr(string_cmd, "SiteSurvey")) != NULL){
                #if 0
                cyg_semaphore_init(&scan_complete_wait_sema, 0);
                if(wrq->u.data.length > os_strlen("SiteSurvey")){
                    int strcmd_offset = os_strlen("SiteSurvey") + 1;
                    ret = usbwifi_sta_ioctl_cmd_set_scan(sc, string_cmd + strcmd_offset, wrq->u.data.length - strcmd_offset);
                }else
                    ret = usbwifi_sta_ioctl_cmd_set_scan(sc, NULL, 0);
                // wait time 3s
                cyg_semaphore_timed_wait(&scan_complete_wait_sema, cyg_current_time() + 500);
                cyg_semaphore_destroy(&scan_complete_wait_sema);
                break;
                #endif
                #if 1
                aic_wifi_scan();
                #endif
            }

            ret = -1;
            break;
        }
        case RTPRIV_IOCTL_GSITESURVEY:
        {
            aic_dbg("IOCTL:RTPRIV_IOCTL_GSITESURVEY\n");
            #if 0
            char result[256];
#if 0
            cyg_semaphore_init(&scan_complete_wait_sema, 0);
            usbwifi_sta_ioctl(sc, "set_scan", result);

            // wait time 3s
            cyg_semaphore_timed_wait(&scan_complete_wait_sema, cyg_current_time() + 500);
            cyg_semaphore_destroy(&scan_complete_wait_sema);
#endif
            usbwifi_sta_ioctl(sc, "get_range", result);

            ret = gxapp_usbwifi_sta_ioctl_cmd_get_scan(sc, wrq);
            #else
            aic_wifi_get_scan_result();
            #endif
            break;
        } */
        default:
            printf("IOCTL::unknown IOCTL's cmd = 0x%08x\n", cmd);
            ret = -1;
            break;
    }

    if (ret == 0) {
        //if (wrq->u.data.length != org_len)
        //    wrqin->u.data.length = wrq->u.data.length;
    }

    return ret;
}

void ITEAIC8800_freertos_send(struct eth_drv_sg *sg_list, int sg_len, int total_len)
{
    //aic_dbg("%s %p %d %d\n", __func__, sg_list, sg_len, total_len);
    #ifdef CONFIG_ECOS_SEND_TS
    uint32_t start_time = 0;
    uint32_t end_time = 0;
    static volatile uint8_t send_count = 0;
    #endif

    #ifdef CONFIG_ECOS_SEND_TS
    send_count++;
    #ifdef PLATFORM_ITE_FREERTOS
    start_time = rtos_now(false);
    #endif
    #endif

    uint32_t offset = 0;

    //aic_dbg("es:%d\n", len);
    //aic_dbg("es:%d %d %d\n", len, id, priority);
    //aic_dbg("es:%d %s %d\n", len, info.name, info.cur_pri);
    #ifdef CONFIG_ECOS_SEND_TS
    aic_dbg("esin:%u/%u/%u\n", start_time, send_count, priority);
    #endif

    TCPIP_PACKET_INFO_T *sent_pkt;
    struct fhost_vif_tag *fhost_vif;
    fhost_vif = &fhost_env.vif[0];

    uint32_t reserved_len = offsetof(struct hostdesc, cfm_cb);
    int ret = rtos_semaphore_wait(net_tx_buf_sema, 100);
    struct net_tx_buf_tag *tx_buf = NULL;
    if (ret == 0) {
        rtos_mutex_lock(net_tx_buf_mutex, -1);
        //printf("fl.f: %p\n", net_tx_buf_free_list.first);
        //printf("fl.fn: %p\n", net_tx_buf_free_list.first->next);
        tx_buf = (struct net_tx_buf_tag *)co_list_pop_front(&net_tx_buf_free_list);
        rtos_mutex_unlock(net_tx_buf_mutex);
    }
    if (!tx_buf) {
        aic_dbg("%s get buf fail, ret = %d\n", __func__, ret);
        return;
    }

    uint8_t *buf = tx_buf->buf + reserved_len;

    if (sg_list && (sg_len > 0)) {
        while (sg_len > 0) {
            //aic_dbg("sg:%d\n", sg_list->len);
            memcpy((void *)(buf + offset), (void *)sg_list->buf, sg_list->len);
            offset += sg_list->len;
            sg_list++;
            sg_len--;
        }
    }

    #ifdef CONFIG_ECOS_SEND_TCP_PARSE
    struct ether_header *ehdr = (struct ether_header *)buf;
    if (ntohs(ehdr->ether_type) == ETHERTYPE_IP) {
        struct ip *ihdr = (struct ip *)(buf + sizeof(struct ether_header));
        if (ihdr->ip_p == IPPROTO_TCP) {
            struct tcphdr *thdr = (struct tcphdr *)(buf + sizeof(struct ether_header) + ihdr->ip_hl * 4);
            //aic_dbg("tx:%u %u %u\n", ntohl(thdr->th_seq), ntohl(thdr->th_ack), ntohs(thdr->th_win));
            aic_dbg("tx:%u %u\n", ntohl(thdr->th_seq), ntohl(thdr->th_ack));
        }
    }
    #endif

    sent_pkt = (TCPIP_PACKET_INFO_T *)&(tx_buf->pkt_hdr);
    sent_pkt->data_ptr = tx_buf->buf + reserved_len;
    sent_pkt->data_len = offset;
    sent_pkt->net_id   = 0;
    sent_pkt->pkt_type = 0;
    //printf("es:%u, tid:%u\n", send_count, cyg_thread_get_id(cyg_thread_self()));
    fhost_tx_start(&fhost_vif->net_if, sent_pkt, NULL, NULL);
#if ECOS_TX_DONE_IN_DELIVER
    struct ecos_send_desc_tag *desc = ITEAIC8800_freertos_send_desc_free_list_dequeue();
    //printf("es fl: %d\n", ITEAIC8800_freertos_send_desc_free_list_cnt());
    if (desc == NULL) {
        aic_dbg("Err: no free ecos_send_desc\n");
        return;
    }
    desc->key = key;
    ITEAIC8800_freertos_send_desc_post_list_enqueue(desc);
    //printf("es pl: %d\n", ITEAIC8800_freertos_send_desc_post_list_cnt());

    ITEAIC8800_freertos_deliver_trigger();
#endif
    #ifdef CONFIG_ECOS_SEND_TS
    #ifdef PLATFORM_ITE_FREERTOS
    end_time = rtos_now(false);
    #endif
    aic_dbg("esout:%u/%u\n", end_time, send_count);
    #endif
}

static void ITEAIC8800_freertos_recv(struct eth_drv_sg *sg_list, int sg_len)
{    
    //TRACE_IN();

    //uint32_t cur_time = rtos_now(false);
    //if (sg_list[0].len != sizeof(struct ether_header)) {
    //    aic_dbg("Packet miss match, sg_list[0].len(%d) != sizeof(struct ether_header)(%d)\n", sg_list[0].len, sizeof(struct ether_header));
    //}
    ///aic_dbg("sg_len %d, deliver_len %d\n", sg_len, deliver_len);
    //memcpy((unsigned char*)sg_list[0].buf, deliver_buf, sizeof(struct ether_header));
    //sg_list++;
    //sg_len--;

    unsigned char *source = deliver_buf;// + sizeof(struct ether_header);
    int srcLen = deliver_len;// - sizeof(struct ether_header);
    while (sg_len > 0 && srcLen > 0) {
        if (sg_list->buf) {
            //aic_dbg("receive sglist len=%d, srcLen=%d\n", sg_list->len, srcLen);
            memcpy((unsigned char*)sg_list->buf,source,sg_list->len);
        }

        source += sg_list->len;
        srcLen -= sg_list->len;
        ++sg_list;
        --sg_len;
    }
    if (srcLen != 0) {
        aic_dbg("Packet miss match, left data len(%d)\n", srcLen);
    }
    //printf("d: %d\n", rtos_now(false) - cur_time);
    //TRACE_OUT();
}

void ITEAIC8800_freertos_deliver(struct eth_drv_sg *sg_list, int sg_len)
{
    //TRACE_IN();

    #ifdef DCONFIG_ECOS_DELIVER_TS
    uint32_t start_time = 0;
    uint32_t end_time = 0;
    static volatile uint8_t deliver_count = 0;
    #endif

    #ifdef DCONFIG_ECOS_DELIVER_TS
    deliver_count++;
    #ifdef PLATFORM_ITE_FREERTOS
    start_time = rtos_now(false);
    #endif
    #endif

    /*
    cyg_handle_t handle = cyg_thread_self();
    cyg_uint16 id = cyg_thread_get_id(handle);
    cyg_thread_info info;
    cyg_bool_t result = cyg_thread_get_info(handle, id, &info);
    cyg_priority_t priority = cyg_thread_get_current_priority(handle);

    aic_dbg("%s %p\n", __func__, sc);
    aic_dbg("ed:%d\n", deliver_len);
    aic_dbg("ed:%d %d %d\n", deliver_len, id, priority);
    aic_dbg("ed:%d %s %d\n", deliver_len, info.name, info.cur_pri);
    aic_dbg("ed:%d/%d\n", deliver_len, priority);
    */
    #if ECOS_TX_DONE_IN_DELIVER
    struct ecos_send_desc_tag *desc = ITEAIC8800_freertos_send_desc_post_list_dequeue();
    //printf("dl pl: %d\n", ITEAIC8800_freertos_send_desc_post_list_cnt());
    if (desc) {
        unsigned long key = desc->key;
        ITEAIC8800_freertos_send_desc_free_list_enqueue(desc);
        //printf("dl fl: %d\n", ITEAIC8800_freertos_send_desc_free_list_cnt());
        if (ITEAIC8800_freertos_send_desc_post_list_cnt() > 0) {
            ITEAIC8800_freertos_deliver_trigger();
        }
    }
    #endif

    #ifdef CONFIG_FHOST_RX_ASYNC
    struct fhost_rx_async_desc_tag *async_desc = fhost_rx_async_post_dequeue();
    if (async_desc == NULL) {
        return;
    }
    deliver_buf = async_desc->data;
    deliver_len = async_desc->len;
    #endif

    #ifdef DCONFIG_ECOS_DELIVER_TS
    uint8_t deliver_buf_valid = (deliver_buf && deliver_len) ? 1 : 0;
    aic_dbg("edin:%u/%u/%u\n", start_time, deliver_count, deliver_buf_valid);
    #endif

    if (deliver_buf && deliver_len) {
        #ifdef CONFIG_ECOS_DELIVER_TCP_PARSE
        struct ether_header *ehdr = (struct ether_header *)deliver_buf;
        if (ntohs(ehdr->ether_type) == ETHERTYPE_IP) {
            struct ip *ihdr = (struct ip *)(deliver_buf + sizeof(struct ether_header));
            if (ihdr->ip_p == IPPROTO_TCP) {
                struct tcphdr *thdr = (struct tcphdr *)(deliver_buf + sizeof(struct ether_header) + ihdr->ip_hl * 4);
                //aic_dbg("rx:%u %u %u\n", ntohl(thdr->th_seq), ntohl(thdr->th_ack), ntohs(thdr->th_win));
                aic_dbg("rx:%u %u\n", ntohl(thdr->th_seq), ntohl(thdr->th_ack));
            }
        }
        #endif

        //uint32_t cur_time = rtos_now(false);
        ITEAIC8800_freertos_recv(sg_list, sg_len);
        //printf("delta: %d\n", rtos_now(false) - cur_time);
        deliver_buf = NULL;
        deliver_len = 0;
        #ifndef CONFIG_FHOST_RX_ASYNC
        ITEAIC8800_freertos_deliver_done();
        #endif
    }

    #ifdef CONFIG_FHOST_RX_ASYNC
    if (async_desc->frame_type == RX_ASYNC_RX_FRAME) {
        #ifdef CONFIG_USB_SUPPORT
        struct aicwf_usb_buf *frame = (struct aicwf_usb_buf *)async_desc->frame_ptr;
        aicwf_usb_rx_buf_put(g_aic_usb_dev, frame);
        aicwf_usb_rx_submit_all_urb(g_aic_usb_dev);
        #elif CONFIG_SDIO_SUPPORT
        /*
        struct sdio_buf_node_s *frame = (struct sdio_buf_node_s *)async_desc->frame_ptr;
        sdio_buf_free(frame);
        */
        #endif
    } else if (async_desc->frame_type == RX_ASYNC_REORDER_FRAME) {
        struct recv_msdu *frame = (struct recv_msdu *)async_desc->frame_ptr;
        reord_rxframe_free(frame);
    } else {
        aic_dbg("Err: unknown frame type to deliver\n");
    }
    fhost_rx_async_desc_free(async_desc);
	/*
    if (fhost_rx_async_post_cnt(true) > 0) {
        ITEAIC8800_freertos_deliver_trigger();
    } */
    #endif

    #ifdef DCONFIG_ECOS_DELIVER_TS
    #ifdef PLATFORM_ITE_FREERTOS
    end_time = rtos_now(false);
    #endif
    aic_dbg("edout:%u/%u\n", end_time, deliver_count);
    #endif
}


int ITEAIC8800_freertos_ioctl(unsigned long cmd,
                void *data,
                int len)
{
    printf("Control cmd: 0x%04X, len: %d\n", cmd, len);

    if (len)
        return 0;
    else
        return SPAPP_AIC8800_ioctl(data, cmd);
}

void ITEAIC8800_freertos_deliver_init(void)
{
    rtos_semaphore_create(&deliver_sema, "deliver_sema", 0x7FFFFFFF, 0);
    deliver_buf = NULL;
    deliver_len = 0;
}

void ITEAIC8800_freertos_deliver_buf_set(uint8_t *buf, uint32_t len)
{
    deliver_buf = buf;
    deliver_len = len;
}
/*
void ITEAIC8800_freertos_deliver_trigger(void)
{
    eth_drv_dsr(0, 0, (cyg_addrword_t)&aic8800_sc);
}
*/
int ITEAIC8800_freertos_deliver_wait_done(int timeout)
{
    return rtos_semaphore_wait(deliver_sema, timeout);
}

int ITEAIC8800_freertos_deliver_done(void)
{
    return rtos_semaphore_signal(deliver_sema, false);
}

void ITEAIC8800_freertos_send_init(void)
{
    int ret, i;

    ret = rtos_mutex_create(&ecos_deliver_trigger_mutex, "ecos_deliver_trigger_mutex");
    if (ret) {
        aic_dbg("Err: ecos_deliver_trigger_mutex create fail, ret= %d\n", ret);
    }

    ret = rtos_mutex_create(&ecos_send_desc_free_mutex, "ecos_send_desc_free_mutex");
    if (ret) {
        aic_dbg("Err: ecos_send_desc_free_mutex create fail, ret= %d\n", ret);
    }
    co_list_init(&ecos_send_desc_free_list);
    for (i = 0; i < ECOS_SEND_DESC_CNT; i++) {
        co_list_push_back(&ecos_send_desc_free_list, &ecos_send_desc_pool[i].hdr);
    }

    ret = rtos_mutex_create(&ecos_send_desc_post_mutex, "ecos_send_desc_post_mutex");
    if (ret) {
        aic_dbg("Err: ecos_send_desc_post_mutex create fail, ret= %d\n", ret);
    }
    co_list_init(&ecos_send_desc_post_list);
}

struct ecos_send_desc_tag *ITEAIC8800_freertos_send_desc_free_list_dequeue(void)
{
    struct ecos_send_desc_tag *desc;
    
    rtos_mutex_lock(ecos_send_desc_free_mutex, -1);
    desc = (struct ecos_send_desc_tag *)co_list_pop_front(&ecos_send_desc_free_list);
    rtos_mutex_unlock(ecos_send_desc_free_mutex);

    return desc;
}

void ITEAIC8800_freertos_send_desc_free_list_enqueue(struct ecos_send_desc_tag *desc)
{
    rtos_mutex_lock(ecos_send_desc_free_mutex, -1);
    co_list_push_back(&ecos_send_desc_free_list, &desc->hdr);
    rtos_mutex_unlock(ecos_send_desc_free_mutex);
}

uint32_t ITEAIC8800_freertos_send_desc_free_list_cnt(void)
{
    uint32_t cnt;

    rtos_mutex_lock(ecos_send_desc_free_mutex, -1);
    cnt = co_list_cnt(&ecos_send_desc_free_list);
    rtos_mutex_unlock(ecos_send_desc_free_mutex);

    return cnt;
}

struct ecos_send_desc_tag *ITEAIC8800_freertos_send_desc_post_list_dequeue(void)
{
    struct ecos_send_desc_tag *desc;
    
    rtos_mutex_lock(ecos_send_desc_post_mutex, -1);
    desc = (struct ecos_send_desc_tag *)co_list_pop_front(&ecos_send_desc_post_list);
    rtos_mutex_unlock(ecos_send_desc_post_mutex);

    return desc;
}

void ITEAIC8800_freertos_send_desc_post_list_enqueue(struct ecos_send_desc_tag *desc)
{
    rtos_mutex_lock(ecos_send_desc_post_mutex, -1);
    co_list_push_back(&ecos_send_desc_post_list, &desc->hdr);
    rtos_mutex_unlock(ecos_send_desc_post_mutex);
}

uint32_t ITEAIC8800_freertos_send_desc_post_list_cnt(void)
{
    uint32_t cnt;

    rtos_mutex_lock(ecos_send_desc_post_mutex, -1);
    cnt = co_list_cnt(&ecos_send_desc_post_list);
    rtos_mutex_unlock(ecos_send_desc_post_mutex);

    return cnt;
}

unsigned char ITEAIC8800_get_mac_accord_index(struct eth_drv_sc *sc, int index)
{
    char *mac_addr = (char *)get_mac_address();

    aic_dbg("%s %p %d %#x\n", __func__, sc, index, mac_addr[index]);

    return mac_addr[index];
}

void *ITEAIC8800_get_wireless_stats(struct eth_drv_sc *sc)
{
    aic_dbg("%s %p\n", __func__, sc);

    return NULL;
}

