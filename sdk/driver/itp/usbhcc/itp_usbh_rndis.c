#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_rndis.h"
#include "usbhcc/oal/oal_event.h"
#include "usbhcc/oal/oal_mutex.h"
#include "ite/itp_usbh_rndis.h"

#define HEXDUMP_COLS 16
void itp_rndis_hexdump(char* msg, void *mem, unsigned int len)
{
#if (1)
    char buffer[256];
    char* p=buffer;
    int n;
    unsigned int i, j;

    ithPrintf(msg);
    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
    {
            /* print offset */
            if(i % HEXDUMP_COLS == 0)
            {
                    n=sprintf(p, "0x%06X: ", i);
                    p+=n;
            }

            /* print hex data */
            if(i < len)
            {
                    n=sprintf(p, "%02X ", 0xFF & ((char*)mem)[i]);
                    p+=n;
            }
            else /* end of block, just aligning for ASCII dump */
            {
                    n=sprintf(p, "   ");
                    p+=n;
            }

            /* print ASCII dump */
            if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
            {
                    for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                    {
                            if(j >= len) /* end of block, not really printing */
                            {
                                    //putchar(' ');
                                    *p++=' ';
                            }
                            else if(isprint(((char*)mem)[j])) /* printable char */
                            {
                                    //putchar(0xFF & ((char*)mem)[j]);
                                    *p++=(((char*)mem)[j]);
                            }
                            else /* other char */
                            {
                                    //putchar('.');
                                    *p++='.';
                            }
                    }
                    //putchar('\n');
                    *p++='\n';
                    *p++=0;
                    ithPrintf(buffer);
                    p=buffer;
            }
    }
    if (p!=buffer) {
        *p++='\n';
        *p++=0;
        ithPrintf(buffer);
    }
    ithPrintf("\n");
#endif
}

#define BIT(n)  (1u<<n)
#define EVENT_CONNECT       BIT(13)
#define EVENT_DISCONNECT    BIT(14)
#define EVENT_EXIT          BIT(15)
#define EVENT_UPDATE        BIT(0)
#define EVENT_COMMIT        BIT(1)
#define EVENT_ALL           65535u

enum{
    THREADSTATE_EXIT=-1,
    THREADSTATE_UNKNOWN=0,
    THREADSTATE_IDLE,
    THREADSTATE_RUNNING,
};

/* Queue item structure */
struct q_item
{
    struct q_item* next;
    void* buf;
    uint16_t len;
};


struct netif_state_rndisex {
    struct netif netif;
    t_nwdriver* nwdriver;
    void* (*tx_buffer_get)(struct netif *netif);
    void  (*tx_buffer_commit)(struct netif *netif, void* buffer, int len);
    void  (*input)(struct netif *netif, void *packet, int packet_len);

    /* only visible to itp_usbh_rndis.c */

    struct dhcp dhcp;
    struct timeval timevalFine0;
    struct timeval timevalCoarse0;
    //timer_t dhcp_timerid;


    // rndis thread
    pthread_t rndis_thread;
    int rndis_thread_state;
    oal_event_t rndis_event;

    // rx
    pthread_t rx_thread;
    int rx_thread_state;
    oal_event_t rx_buf_event;
    int rx_buf_size;
    int rx_buf_count;

    // tx
    pthread_t tx_thread;
    int tx_thread_state;
    oal_event_t tx_buf_event;
    unsigned char* rx_buf_base;
    unsigned char* tx_buf_base;
    struct q_item* tx_q_free;
    struct q_item* tx_q_busy;
    struct q_item* tx_q_wait;
    struct q_item  tx_q_items[15];
    int tx_buf_count;
    int tx_buf_size;

    // dhcp
    int do_dhcp_timer;
};

err_t rndisif_netifinitfn(struct netif *netif);
static struct netif_state_rndisex* netif_state=NULL;
static pthread_mutex_t gmutex = PTHREAD_MUTEX_INITIALIZER;

static void put_q(struct netif_state_rndisex* state, struct q_item** q, struct q_item* item)
{
    if (!q || !item) {
        return;
    }

    pthread_mutex_lock(&gmutex);

    item->next=*q;
    *q=item;

    pthread_mutex_unlock(&gmutex);
}

static void put_q_tail(struct netif_state_rndisex* state, struct q_item** q, struct q_item* item)
{
    struct q_item* p;

    if (!q || !item) {
        return;
    }

    pthread_mutex_lock(&gmutex);

    item->next=NULL;

    if (!*q)
    {
        *q = item;
    }
    else
    {
        p=*q;
        while (p->next != NULL)
        {
            p = p->next;
        }

        p->next=item;
    }

    pthread_mutex_unlock(&gmutex);
}

static int is_q_empty(struct netif_state_rndisex* state, struct q_item* q)
{
    int yes;

    pthread_mutex_lock(&gmutex);
    yes=!!(q==NULL);
    pthread_mutex_unlock(&gmutex);

    return yes;
}
/*
static int is_tx_q_full(struct netif_state_rndisex* state)
{
    int yes;

    pthread_mutex_lock(&gmutex);
    yes=!!((state->tx_q_free!=NULL) && (state->tx_q_r==state->tx_q_w));
    pthread_mutex_unlock(&gmutex);

    return yes;
}
*/
static struct q_item* get_q(struct netif_state_rndisex* state, struct q_item** q)
{
    struct q_item* p=NULL;

    if (!q || !*q) return NULL;
    pthread_mutex_lock(&gmutex);
    p=*q;
    *q=p->next;
    p->next=NULL;
    pthread_mutex_unlock(&gmutex);

    return p;
}

static void dhcp_ticker(timer_t timerid, int arg)
{
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)arg;
    struct timeval timeval;

    gettimeofday(&timeval, NULL);
    //if (state->netif.flags & NETIF_FLAG_UP) {
        if (itpTimevalDiff(&state->timevalCoarse0, &timeval) >= DHCP_COARSE_TIMER_MSECS) {
            itp_rndis_printf("[i] %s: dhcp_coarse_tmr\n", __FUNCTION__);
            //state->timevalCoarse0=timeval;
            state->timevalCoarse0.tv_usec+=DHCP_COARSE_TIMER_MSECS*1000;
            if (state->timevalCoarse0.tv_usec>=1000000) {
                state->timevalCoarse0.tv_sec+=state->timevalCoarse0.tv_usec/1000000;
                state->timevalCoarse0.tv_usec%=1000000;
            }
            dhcp_coarse_tmr();
            state->timevalFine0=state->timevalCoarse0;
        }
    /*}
    else*/ {
        if (itpTimevalDiff(&state->timevalFine0, &timeval) >= DHCP_FINE_TIMER_MSECS) {
            itp_rndis_printf("[i] %s: dhcp_fine_tmr\n", __FUNCTION__);
            //state->timevalFine0=timeval;
            state->timevalFine0.tv_usec+=DHCP_FINE_TIMER_MSECS*1000;
            if (state->timevalFine0.tv_usec>=1000000) {
                state->timevalFine0.tv_sec+=state->timevalFine0.tv_usec/1000000;
                state->timevalFine0.tv_usec%=1000000;
            }
            dhcp_fine_tmr();
            state->timevalCoarse0=state->timevalFine0;
        }
    }
}

static int usbh_rndis_ntf( t_usbh_unit_id uid, t_usbh_ntf ntf )
{
    int rc;

    if (netif_state) {
        if (ntf == USBH_NTF_CONNECT)
        {
            rc=oal_event_set(&netif_state->rndis_event, EVENT_CONNECT, 0 );
        }
        else if ( ntf == USBH_NTF_DISCONNECT )
        {
            rc=oal_event_set(&netif_state->rndis_event, EVENT_DISCONNECT, 0 );
        }
    }

    return 0;
} /* usbh_rndis_connect_ntf */


static void* tx_buffer_get(struct netif *netif) {
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)netif->state;
    struct q_item* p;

    pthread_mutex_lock(&gmutex);
    p=state->tx_q_free;
    pthread_mutex_unlock(&gmutex);

    itp_rndis_printf("[%c] %s=%08X\n", p?'i':'X', __FUNCTION__, p?p->buf:NULL);
    if (!p) {
        return NULL;
    }

    return p->buf;
}

static void tx_buffer_commit(struct netif *netif, void* buffer, int len) {
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)netif->state;
    struct q_item* p;

    itp_rndis_printf("[i] %s=%08X:%d\n", __FUNCTION__, buffer, len);
    p=get_q(state, &state->tx_q_free);
    if (!p) {
        ithPrintf("[X] commit: p is NULL\n");
    }
    else if (p->buf != buffer) {
        ithPrintf("[X] commit: %08X!=%08X\n", buffer, p->buf);
        put_q(state, &state->tx_q_free, p);
    }
    p->len=len;
    put_q_tail(state, &state->tx_q_busy, p);
    oal_event_set(&state->tx_buf_event, EVENT_UPDATE, 0);
}

static void tx_buffer_done(struct netif *netif, t_nwdriver_ret rc) {
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)netif->state;
    struct q_item* p;

    p=get_q(state, &state->tx_q_wait);
    if (!p) {
        ithPrintf("[X] %s: no q!\n", __FUNCTION__);
    }
    put_q_tail(state, &state->tx_q_free, p);
    itp_rndis_printf("[i] %s=%08X:%d\n", __FUNCTION__, p->buf, rc);
}

static void* tx_thread_proc(void* arg) {
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)arg;
    //void* p;
    oal_event_flags_t flags;
    struct q_item* p;

    itp_rndis_printf("[i] tx_thread_proc++\n");
    while (1) {
        ithPrintf("[i] %s: wait EVENT_CONNECT\n", __FUNCTION__);
        flags=0;
        state->tx_thread_state=THREADSTATE_IDLE;
        if (oal_event_get(&state->tx_buf_event, EVENT_ALL, &flags, OAL_WAIT_FOREVER))
            continue;

        if ((flags&EVENT_EXIT)) {
            goto l_exit;
        }

        if (!(flags&EVENT_CONNECT)) {
            continue;
        }

        ithPrintf("[i] %s: EVENT_CONNECT\n", __FUNCTION__);
        while (1) {
            flags=0;
            state->tx_thread_state=THREADSTATE_RUNNING;
            oal_event_get(&state->tx_buf_event, EVENT_ALL, &flags, is_q_empty(state, state->tx_q_busy)?OAL_WAIT_FOREVER:1);

            if ((flags&EVENT_EXIT)) {
                goto l_exit;
            }

            if (flags&EVENT_DISCONNECT) {
                break;
            }

            while ((p=get_q(state, &state->tx_q_busy))!=NULL) {
                itp_rndis_printf("[i] %s@get_q=%08X:%d\n", __FUNCTION__, p->buf, p->len);
                //itp_rndis_hexdump("", p->buf, p->len);
                if (state->nwdriver->p_nwd_fn->p_nwfn_send(state->nwdriver, p->buf, p->len)) {
                    ithPrintf("[X] %s@put_q: requeue %08X:%d!\n", __FUNCTION__, p->buf, p->len);
                    put_q(state, &state->tx_q_busy, p);
                    break;
                }
                put_q_tail(state, &state->tx_q_wait, p);
            }
        }
    }

l_exit:
    itp_rndis_printf("[i] tx_thread_proc--\n");
    state->tx_thread_state=THREADSTATE_EXIT;
    pthread_exit(0);
}

static inline uint32_t psp_rd_le32 ( const uint8_t * const p )
{
  /* 436 S : MISRA-C:2004 17.1,17.4, MISRA-C:2012/AMD1/TC1 R.18.1: Declaration does not specify an array. */
  /*LDRA_INSPECTED 436 S*/
  uint32_t  tmp32_q1 = p[3];

  /*LDRA_INSPECTED 436 S*/
  uint32_t  tmp32_q2 = p[2];

  /*LDRA_INSPECTED 436 S*/
  uint32_t  tmp32_q3 = p[1];

  /*LDRA_INSPECTED 436 S*/
  uint32_t  tmp32_q4 = p[0];

  tmp32_q1 <<= 24;
  tmp32_q2 <<= 16;
  tmp32_q3 <<= 8;
  return tmp32_q1 | tmp32_q2 | tmp32_q3 | tmp32_q4;
} /* psp_rd_le32 */

typedef struct
{
  uint8_t  msg_type[4u];          /* Message Type */
  uint8_t  msg_len[4u];           /* Message Length */
  uint8_t  data_offset[4u];       /* Offset in bytes from the start of the DataOffset field of this message to the start of the data. */
  uint8_t  data_len[4u];          /* Number of bytes in the data content of this message. */
  uint8_t  oob_data_offset[4u];   /* Offset in bytes of the first out of band data record from the start of the DataOffset field */
  uint8_t  oob_data_len[4u];      /* Specifies in bytes the total length of the out of band data. */
  uint8_t  num_of_oob_data_elements[4u];
  uint8_t  per_pkt_info_offset[4u];
  uint8_t  per_pkt_info_len[4u];
  uint8_t  vc_hdl[4u];
  uint8_t  reserved[4u];
} t_rndis_pkt_msg;

static uint8_t assemble_buffer[32768];
static int assemble_buffer_h=0;
static int assemble_buffer_t=0;

static void* rx_thread_proc(void* arg) {
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)arg;
    uint8_t* p;
    uint16_t rlen;
    oal_event_flags_t flags;
    int rc;

    itp_rndis_printf("[i] rx_thread_proc++\n");
    while (1) {
        ithPrintf("[i] %s: wait EVENT_CONNECT\n", __FUNCTION__);
        flags=0;
        state->rx_thread_state=THREADSTATE_IDLE;
        if (oal_event_get(&state->rx_buf_event, EVENT_ALL, &flags, OAL_WAIT_FOREVER))
            continue;

        if ((flags&EVENT_EXIT)) {
            goto l_exit;
        }

        if (!(flags&EVENT_CONNECT)) {
            continue;
        }

        ithPrintf("[i] %s: EVENT_CONNECT\n", __FUNCTION__);
        while (1) {
            flags=0;
            state->rx_thread_state=THREADSTATE_RUNNING;
            if (oal_event_get(&state->rx_buf_event, EVENT_ALL, &flags, OAL_WAIT_FOREVER))
                continue;

            if ((flags&EVENT_EXIT)) {
                goto l_exit;
            }

            if (flags&EVENT_DISCONNECT) {
                break;
            }

            if (flags&EVENT_UPDATE) {
                while ((rc=state->nwdriver->p_nwd_fn->p_nwfn_receive(state->nwdriver, &p, &rlen))==0) {
                    unsigned char *p0, *p9;
                    t_rndis_pkt_msg* pkt;
                    if (rlen) {
                        //rlen+=44;
                        itp_rndis_printf("[i] %s@recv=%08X:%08X:%d\n", __FUNCTION__, p-44, p, rlen);
                        if (assemble_buffer_t!=0) {
                            itp_rndis_printf("[i] assemble buffer: using assemble buffer!\n");
                            itp_rndis_printf("[i] %s@recv=%08X:%08X:%d\n", __FUNCTION__, p-44, p, rlen);
                            if (assemble_buffer_t+rlen>sizeof(assemble_buffer)) {
                                ithPrintf("[X] assemble buffer: overflow %d>%d, reset assemble buffer\n!\n", sizeof(assemble_buffer), assemble_buffer_t+rlen);
                                p0=p9=0;
                                assemble_buffer_h=assemble_buffer_t=0;
                                rlen=0;
                            }
                            else {
                                memcpy(assemble_buffer+assemble_buffer_t, p-44, rlen);
                                p0=p9=assemble_buffer;
                                assemble_buffer_t+=rlen;
                                rlen=assemble_buffer_t;
                                pkt=(t_rndis_pkt_msg*) p9;
                            }
                        }
                        else {
                            p0=p9=p-44;
                            pkt=(t_rndis_pkt_msg*) p9;
                            itp_rndis_printf("[i] assemble buffer: using rndis buffer!\n");
                        }

                        while (p9-p0+44<=rlen) {
                            // packet validation
                            if ((psp_rd_le32(pkt->msg_type)!=0x00000001u)
                                //|| (psp_rd_le32(pkt->msg_len)>state->nwdriver->nwd_prop.nwp_mtu_size)
                                || (psp_rd_le32(pkt->data_offset)>pkt->data_offset-pkt->msg_type+state->nwdriver->nwd_prop.nwp_mtu_size)
                                || (psp_rd_le32(pkt->data_len)>psp_rd_le32(pkt->msg_len))
                                ) {

                                ithPrintf("[X] RNDIS_PACKET_MSG\n");
                                ithPrintf("    pkt        : %08X\n", pkt);
                                ithPrintf("    msg_type   : %08X\n", psp_rd_le32(pkt->msg_type));
                                ithPrintf("    msg_len    : %08X\n", psp_rd_le32(pkt->msg_len));
                                ithPrintf("    data_offset: %08X\n", psp_rd_le32(pkt->data_offset));
                                ithPrintf("    data_len   : %08X\n", psp_rd_le32(pkt->data_len));
                                ithPrintf("[X] assemble buffer: p9-p0=%d rlen=%d, discard data and reset!\n", p9-p0, rlen);

                                rlen=0;
                                assemble_buffer_h=0;
                                assemble_buffer_t=0;
                                break;
                            }
                            if (p9-p0+psp_rd_le32(pkt->msg_len)>rlen) {
                                break;
                            }
                            itp_rndis_printf("[i] %s@recv=%08X:%08X:%d:%d:%d:%d:%d\n", __FUNCTION__, p-44, p, rlen, p9-p0, psp_rd_le32(pkt->msg_len), psp_rd_le32(pkt->data_offset), psp_rd_le32(pkt->data_len));
                            state->input(&state->netif, p9+8+psp_rd_le32(pkt->data_offset), psp_rd_le32(pkt->data_len));
                            p9+=psp_rd_le32(pkt->msg_len);
                            pkt=(t_rndis_pkt_msg*) p9;
                            itp_rndis_printf("[i] %s@p9-p0=%d\n", __FUNCTION__, p9-p0);
                        }

                        if (rlen > p9-p0) {
                            itp_rndis_printf("[i] assemble buffer: moving %d bytes data!\n", rlen-(p9-p0));
                            memmove(assemble_buffer, p9, rlen-(p9-p0));
                            assemble_buffer_h=0;
                            assemble_buffer_t=rlen-(p9-p0);
                        }
                        else {
                            assemble_buffer_h=0;
                            assemble_buffer_t=0;
                        }
                    }
                    else {
                        ithPrintf("[X] p_nwfn_receive(%08X:%08X, %d)\n", p-44, p, rlen);
                    }
                    // just for sure
                    memset(p-state->nwdriver->nwd_prop.nwp_header_size, 0, state->nwdriver->nwd_prop.nwp_header_size);
                    state->nwdriver->p_nwd_fn->p_nwfn_add_buf(state->nwdriver, p);
                }
            }
        }
    }

l_exit:
    itp_rndis_printf("[i] rx_thread_proc--\n");
    state->rx_thread_state=THREADSTATE_EXIT;
    pthread_exit(0);
}


static void ntf_state(uint32_t param, uint8_t state, uint8_t from_isr) {

}


static void ntf_rx(uint32_t param, uint16_t len, uint8_t from_isr ) {
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)(void*)param;
    itp_rndis_printf("[i] %s=%d\n", __FUNCTION__, len);
    oal_event_set(&state->rx_buf_event, EVENT_UPDATE, 0);
}

static void ntf_tx(uint32_t param, t_nwdriver_ret ret, uint8_t from_isr ) {
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)(void*)param;
    tx_buffer_done(&state->netif, ret);
}

extern int g_rndis_rx_buffer_size;
static void* rndis_thread_proc(void* arg)
{
    struct netif_state_rndisex* state=(struct netif_state_rndisex*)arg;

    ip_addr_t ipaddr, netmask, gw;
    struct itimerspec timerspec;
    t_nwdriver_cb_dsc cb_dsc;
    oal_event_flags_t flags;
    uint8_t* p_buf;
    int i;
    int rc;

    itp_rndis_printf("[i] rndis_thread_proc++\n");

    while (1) {
        flags=0;
        state->rndis_thread_state=THREADSTATE_RUNNING;
        if (oal_event_get(&state->rndis_event, EVENT_ALL, &flags, (state->nwdriver)?DHCP_FINE_TIMER_MSECS/2:OAL_WAIT_FOREVER)) {
            if (state->nwdriver) {
                dhcp_ticker(0, (int)state);
            }
            continue;
        }

        if (flags&(EVENT_DISCONNECT|EVENT_EXIT)) {
            (void)ithPrintf("[i] %s: EVENT_DISCONNECT||EVENT_EXIT\n", __FUNCTION__);

            if (state->nwdriver) {
                // EVENT_DICONNECT
                (void)ithPrintf("[i] %s: notify rx and tx threads\n", __FUNCTION__);
                oal_event_set(&state->rx_buf_event, EVENT_DISCONNECT, 0);
                oal_event_set(&state->tx_buf_event, EVENT_DISCONNECT, 0);

                (void)ithPrintf("[i] %s: wait rx and tx threads idle\n", __FUNCTION__);
                while ((state->rx_thread_state!=THREADSTATE_IDLE) || (state->tx_thread_state!=THREADSTATE_IDLE)) {
                    (void)usleep(100*1000);
                }

                // dhcp
                (void)ithPrintf("[i] %s: stop dhcp\n", __FUNCTION__);
                dhcp_stop(&state->netif);
                //timer_delete(state->dhcp_timerid);

                // netif
                (void)ithPrintf("[i] %s: shutdown and remove netif\n", __FUNCTION__);
                netif_set_down(&state->netif);
                netif_remove(&state->netif);

                // nwdriver
                (void)ithPrintf("[i] %s: stop rndis nwdriver\n", __FUNCTION__);
                state->nwdriver->p_nwd_fn->p_nwfn_stop(state->nwdriver);
                state->nwdriver->p_nwd_fn->p_nwfn_delete(state->nwdriver);
                state->nwdriver=NULL;
            }
        }

        if (flags&EVENT_EXIT) {
            (void)ithPrintf("[i] %s: EVENT_EXIT\n", __FUNCTION__);
            break;
        }

        if (flags&EVENT_CONNECT) {
            (void)ithPrintf("[i] %s: EVENT_CONNECT\n", __FUNCTION__);
            if ((rc=usbh_rndis_nwdrv_init(0, &state->nwdriver))) {
                (void)ithPrintf("[X] usbh_rndis_nwdrv_init=%d\n", rc);
                continue;
            }

            (void)ithPrintf("[i] nwp_mtu_size   : %d\n", state->nwdriver->nwd_prop.nwp_mtu_size);
            (void)ithPrintf("[i] nwp_header_size: %d\n", state->nwdriver->nwd_prop.nwp_header_size);
            (void)ithPrintf("[i] p_nwp_buf      : %08x\n", state->nwdriver->nwd_prop.p_nwp_buf);
            (void)ithPrintf("[i] nwp_buf_size   : %d\n", state->nwdriver->nwd_prop.nwp_buf_size);

            p_buf=state->nwdriver->nwd_prop.p_nwp_buf;

            // tx buffers
            state->tx_buf_base=p_buf;
            state->tx_q_free=NULL;
            state->tx_q_busy=NULL;
            state->tx_q_wait=NULL;
            state->tx_buf_count=state->nwdriver->nwd_prop.nwp_rxbuf_count;
            state->tx_buf_size=(state->nwdriver->nwd_prop.nwp_header_size+state->nwdriver->nwd_prop.nwp_mtu_size+511)&~511;
            for ( i = 0 ; i < state->tx_buf_count; i++ ) {
(void)ithPrintf("[i] %d. %s@add_tx_buffer: %08X:%08X:%d\n", i, __FUNCTION__, p_buf, p_buf+state->nwdriver->nwd_prop.nwp_header_size, state->tx_buf_size);
                state->tx_q_items[i].buf=p_buf+state->nwdriver->nwd_prop.nwp_header_size;
                put_q_tail(state, &state->tx_q_free, &state->tx_q_items[i]);
                p_buf+=state->tx_buf_size;
            }

            // rx buffers
            state->rx_buf_base=p_buf;
            state->rx_buf_count=state->nwdriver->nwd_prop.nwp_rxbuf_count;
            state->rx_buf_size=g_rndis_rx_buffer_size=((state->nwdriver->nwd_prop.nwp_buf_size-state->tx_buf_count*state->tx_buf_size)/state->rx_buf_count)&~511;
            for ( i = 0, rc = NWDRIVER_SUCCESS ; ( i < state->rx_buf_count ) && ( rc == NWDRIVER_SUCCESS ) ; i++ )
            {
                rc=state->nwdriver->p_nwd_fn->p_nwfn_add_buf(state->nwdriver, p_buf+state->nwdriver->nwd_prop.nwp_header_size);
(void)ithPrintf("[i] %d. %s@add_rx_buffer: %08X:%08X:%d\n", i, __FUNCTION__, p_buf, p_buf+state->nwdriver->nwd_prop.nwp_header_size, state->rx_buf_size);
                p_buf+=state->rx_buf_size;
            }

            cb_dsc.p_nwcb_ntf_rx = ntf_rx;
            cb_dsc.p_nwcb_ntf_tx = ntf_tx;
            cb_dsc.p_nwcb_ntf_state = ntf_state;
            if ((state->nwdriver->p_nwd_fn->p_nwfn_start(state->nwdriver, &cb_dsc, (uint32_t)state))) {
(void)ithPrintf("[X] %s@%d\n", __FUNCTION__, __LINE__);
                state->nwdriver=NULL;
                continue;
            }

            // netif
            (void)ithPrintf("[i] %s: new rndis netif\n", __FUNCTION__);
            state->netif.flags=0;
            #if LWIP_NETIF_REMOVE_CALLBACK
            netif_set_remove_callback(&state->netif, NULL);
            #endif /* LWIP_NETIF_REMOVE_CALLBACK */
            #if LWIP_NETIF_STATUS_CALLBACK
            netif_set_status_callback(&state->netif, NULL);
            #endif /* LWIP_NETIF_STATUS_CALLBACK */
            ip_addr_set_zero(&ipaddr);
            ip_addr_set_zero(&netmask);
            ip_addr_set_zero(&gw);
            netif_set_default(netif_add(&state->netif, &ipaddr, &netmask, &gw, state, rndisif_netifinitfn, tcpip_input));
            netif_set_up(&state->netif);

            // EVENT_CONNECT
            (void)ithPrintf("[i] %s: notify rx and tx threads\n", __FUNCTION__);
            oal_event_set(&state->rx_buf_event, EVENT_CONNECT, 0);
            oal_event_set(&state->tx_buf_event, EVENT_CONNECT, 0);

            // dhcp
            (void)ithPrintf("[i] %s: start dhcp\n", __FUNCTION__);
            dhcp_set_struct(&state->netif, &state->dhcp);
            dhcp_start(&state->netif);
            gettimeofday(&state->timevalFine0, NULL);
            state->timevalCoarse0=state->timevalFine0;
            //timer_create(CLOCK_REALTIME, NULL, &state->dhcp_timerid);
            //timer_connect(state->dhcp_timerid, (VOIDFUNCPTR)dhcp_ticker, (int)state);
            //timerspec.it_value.tv_sec = timerspec.it_interval.tv_sec  = 0;
            //timerspec.it_value.tv_nsec = timerspec.it_interval.tv_nsec = DHCP_FINE_TIMER_MSECS * 1000000 / 2;
            //timer_settime(state->dhcp_timerid, 0, &timerspec, NULL);
        }
    }

    (void)ithPrintf("[i] rndis_thread_proc--\n");
    state->rndis_thread_state=THREADSTATE_EXIT;
    pthread_exit(0);
}

static int itp_usbh_rndis_init(t_usbh_unit_id uid)
{
    int rc;
    pthread_attr_t attr;
    struct sched_param param;

    (void)ithPrintf("[i] %s++\n", __FUNCTION__);
    if (netif_state!=NULL) {
        (void)ithPrintf("[!] %s called multiple times\n", __FUNCTION__);
        return 0;
    }

    netif_state=calloc(sizeof(*netif_state), 1);
    netif_state->tx_buffer_get=tx_buffer_get;
    netif_state->tx_buffer_commit=tx_buffer_commit;

    if ((rc=usbh_rndis_register_ntf(0, USBH_NTF_CONNECT, usbh_rndis_ntf))) {
        (void)ithPrintf("[X] usbh_rndis_register_ntf(0)=%d\n", rc);
        free(netif_state);
        netif_state=0;
        return -1;
    }

    oal_event_create(&netif_state->rndis_event);
    oal_event_create(&netif_state->rx_buf_event);
    oal_event_create(&netif_state->tx_buf_event);

    pthread_attr_init(&attr);
    //pthread_attr_getschedparam (&attr, &param);
    //param.sched_priority+=4;
    //pthread_attr_setschedparam(&attr, &param);
    //pthread_attr_setstacksize(&attr, 20*1024);
    pthread_create(&netif_state->rndis_thread, &attr, rndis_thread_proc, netif_state);

    pthread_attr_init(&attr);
    pthread_attr_getschedparam (&attr, &param);
    param.sched_priority+=4;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, 20*1024);
    pthread_create(&netif_state->rx_thread, &attr, rx_thread_proc, netif_state);

    pthread_attr_init(&attr);
    pthread_attr_getschedparam (&attr, &param);
    param.sched_priority+=4;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, 20*1024);
    pthread_create(&netif_state->tx_thread, &attr, tx_thread_proc, netif_state);

    (void)ithPrintf("[i] %s--\n", __FUNCTION__);
    return 0;
}

static int itp_usbh_rndis_deinit(t_usbh_unit_id uid)
{
    (void)ithPrintf("[i] %s++\n", __FUNCTION__);
    if (!netif_state) {
        return 0;
    }

    usbh_rndis_register_ntf(0, USBH_NTF_CONNECT, NULL);

    oal_event_set(&netif_state->rndis_event, EVENT_EXIT, 0);
    oal_event_set(&netif_state->rx_buf_event, EVENT_EXIT, 0);
    oal_event_set(&netif_state->tx_buf_event, EVENT_EXIT, 0);

    pthread_join(netif_state->rndis_thread, NULL);
    pthread_join(netif_state->rx_thread, NULL);
    pthread_join(netif_state->tx_thread, NULL);

    oal_event_delete(&netif_state->rndis_event);
    oal_event_delete(&netif_state->rx_buf_event);
    oal_event_delete(&netif_state->tx_buf_event);

    free(netif_state);
    netif_state=NULL;

    (void)ithPrintf("[i] %s--\n", __FUNCTION__);
    return 0;
}

static int itp_usbh_rndis_getifconfig(struct netif_state_rndisex* state, struct s_itp_usbh_rndis_ifconfig* config)
{

//(void)ithPrintf("[i] %s++\n", __FUNCTION__);
    if (!state || !state->nwdriver || !config) {
        return -1;
    }

    memset(config, 0, sizeof(*config));
    ip_addr_copy(config->ip_addr, state->netif.ip_addr);
    ip_addr_copy(config->netmask, state->netif.netmask);
    ip_addr_copy(config->gw, state->netif.gw);
    config->name[0]=state->netif.name[0];
    config->name[1]=state->netif.name[1];
    config->mtu=state->netif.mtu;
    config->hwaddr_len=state->netif.hwaddr_len;
    config->flags=state->netif.flags;
    memcpy(config->hwaddr, state->netif.hwaddr, NETIF_MAX_HWADDR_LEN);

//(void)ithPrintf("[i] %s--\n", __FUNCTION__);
    return 0;
}
static int itp_usbh_rndis_ioctl(int file, unsigned long request, void *ptr, void *info)
{
    int rc=-1;

//(void)ithPrintf("[i] %s(%d, %d, %08X, %08X)\n", __FUNCTION__, file, request, ptr, info);
    switch (request) {
        case ITP_IOCTL_USBHRNDIS_INIT:
            rc=itp_usbh_rndis_init((t_usbh_unit_id)(unsigned int)ptr);
            break;
        case ITP_IOCTL_USBHRNDIS_DEINIT:
            rc=itp_usbh_rndis_deinit((t_usbh_unit_id)(unsigned int)ptr);
            break;
        case ITP_IOCTL_USBHRNDIS_GETIFCONFIG:
            rc=itp_usbh_rndis_getifconfig(netif_state, (struct s_itp_usbh_rndis_ifconfig*) ptr);
            break;
        default:
            errno=(ITP_DEVICE_USBHRNDIS<<ITP_DEVICE_ERRNO_BIT) | 1;
            break;
    }
if (rc)
(void)ithPrintf("[X] %s(%d, %d, %08X, %08X)=%d\n", __FUNCTION__, file, request, ptr, info, rc);
    return rc;
}


const ITPDevice itpdev_usbh_rndis=
{
    ":usbhrndis",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    itp_usbh_rndis_ioctl,
    NULL
};
