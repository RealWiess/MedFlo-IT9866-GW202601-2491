/*****************************************************************************
**
**  Name:           bta_pan_co.c
**
**  Description:    This file contains the data gateway callout function
**                  implementation for Insight.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include "stdlib.h"
#include "bta_platform.h"
#include "bte_glue.h"

#if (defined BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)

#define IPPROTO_ICMP        1       /* control message protocol */
#define IPPROTO_TCP         6       /* tcp */
#define IPPROTO_UDP         17      /* user datagram protocol */

#define PROTOCOL_ID_IPV4            0x0800
#define PROTOCOL_ID_ARP             0x0806
#define SCRU_PROTOCOL_ID_802_1P     0x8100


#define BTAPP_PAN_PING_LENGTH 450

#define BTAPP_SRC_IP_ADDR "192.168.0.1"
#define BTAPP_DST_IP_ADDR "192.168.0.1"

#define BTAPP_SRC_IP_HEX  0xc0a80001
#define BTAPP_DST_IP_HEX  0xc0a80001

extern void btapp_pan_ping_peer(UINT16 handle, BD_ADDR p_src_bda, BD_ADDR p_dst_bda);
static UINT16 calc_checksum (UINT8 *p_data, UINT16 len);
static UINT32 scru_ascii_2_ip_addr (char *p_ascii);
static BOOLEAN btapp_pan_check_send_ping_rsp (UINT16 handle, BT_HDR *p_data, BD_ADDR p_src_bda, BD_ADDR p_dst_bda);

static UINT16   btapp_pan_ip_id = 1;

static char *   p_src_ip = BTAPP_SRC_IP_ADDR;
static char *   p_dst_ip = BTAPP_DST_IP_ADDR;

BT_HDR *  p_data_to_send = NULL;

typedef struct
{
    UINT8   vers_hdr_len;
    UINT8   ip_tos;         /* type of service */
    UINT16  ip_len;         /* total length */
    UINT16  ip_id;          /* identification */
    UINT16  ip_off;         /* fragment offset field */
    UINT8   ip_ttl;         /* time to live */
    UINT8   ip_p;           /* protocol */
    UINT16  ip_sum;         /* checksum */
    UINT32  ip_src, ip_dst; /* source and dest address */
} tIP_STRUCT;

typedef struct
{
    UINT8   ic_type;
    UINT8   ic_code;
    UINT16  ic_cksum;
    UINT16  ic1_id;
    UINT16  ic1_seq;
} tICMP_STRUCT;

typedef struct
{
    UINT8       HardwareType[2];
    UINT8       ProtocolType[2];
    UINT8       HrdAddrLength;
    UINT8       PrtAddrLength;
    UINT8       OpCode[2];
    UINT8       SenderHrdAddr[6];
    UINT8       SenderPrtAddr[4];
    UINT8       TargetHrdAddr[6];
    UINT8       TargetPrtAddr[4];
} tARP_STRUCT;

/*******************************************************************************
**
** Function         bta_pan_co_init
**
** Description
**
**
** Returns          Data flow mask.
**
*******************************************************************************/
UINT8 bta_pan_co_init(UINT8 *q_level)
{

    APPL_TRACE_DEBUG0("bta_pan_co_init");

    /* set the q_level to 30 buffers */
    *q_level = 30;

    return (BTA_PAN_RX_PULL | BTA_PAN_TX_PULL);
}

/*******************************************************************************
**
** Function         bta_pan_co_open
**
** Description
**
**
**
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_open(UINT16 handle, UINT8 app_id, tBTA_PAN_ROLE local_role, tBTA_PAN_ROLE peer_role, BD_ADDR peer_addr)
{

    APPL_TRACE_EVENT4("bta_pan_co_open (handle %d, app_id %d, lrole %d, prole %d)", handle, app_id, local_role, peer_role);
    btapp_pan_cb.app_cb[app_id].conn_handle = handle;
    btapp_pan_cb.app_cb[app_id].service_id = local_role;
    btapp_pan_cb.app_cb[app_id].is_open = TRUE;
    bdcpy(btapp_pan_cb.app_cb[app_id].peer_bdaddr, peer_addr);
    btapp_pan_cb.app_cb[app_id].ping_sent = FALSE;

    bta_pan_ci_rx_ready(handle);

}

/*******************************************************************************
**
** Function         bta_pan_co_close
**
** Description      This function is called by PAN when a connection to a
**                  peer is closed.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_close(UINT16 handle, UINT8 app_id)
{
    if (btapp_pan_cb.app_cb[app_id].is_open == TRUE)
    {
        btapp_pan_cb.app_cb[app_id].is_open = FALSE;
        APPL_TRACE_EVENT2("bta_pan_co_close (handle %d, app_id %d)", handle, app_id);
    }
    else
    {
        APPL_TRACE_ERROR2("bta_pan_co_close (NOT OPEN--) (handle %d, app_id %d)", handle, app_id);
    }
}

/*******************************************************************************
**
** Function         bta_pan_co_tx_path
**
** Description      This function is called by PAN to transfer data on the
**                  TX path; that is, data being sent from BTA to the phone.
**                  This function is used when the TX data path is configured
**                  to use the pull interface.  The implementation of this
**                  function will typically call Bluetooth stack functions
**                  PORT_Read() or PORT_ReadData() to read data from RFCOMM
**                  and then a platform-specific function to send data that
**                  data to the phone.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_tx_path(UINT16 handle, UINT8 app_id)
{
    BT_HDR          *p_buf;
    UINT8           i;
    BD_ADDR            src;
    BD_ADDR            dst;
    UINT16            protocol;
    BOOLEAN            ext;
    BOOLEAN         forward;

    APPL_TRACE_DEBUG0("bta_pan_co_tx_path");


    for(i=0; i<BTAPP_PAN_NUM_SERVICES; i++)
    {
        if(btapp_pan_cb.app_cb[i].conn_handle == handle)
            break;

    }

    if(i == BTAPP_PAN_NUM_SERVICES)
        return;


    do
    {
        /* parse through message from peer and see if its ping or ping response */

        /* read next data buffer from pan */
        if ((p_buf = bta_pan_ci_readbuf(handle, src, dst, &protocol,
                                 &ext, &forward)) == NULL)
        {
            ;/* no more data to read */
        }
        else
        {
            btapp_pan_check_send_ping_rsp(handle, p_buf, src, dst);

        }

    } while (p_buf != NULL);

}

/*******************************************************************************
**
** Function         bta_pan_co_rx_path
**
** Description
**
**
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_rx_path(UINT16 handle, UINT8 app_id)
{


    UINT8           i;

    APPL_TRACE_DEBUG0("bta_pan_co_rx_path");

    for(i=0; i<BTAPP_PAN_NUM_SERVICES; i++)
    {
        if(btapp_pan_cb.app_cb[i].conn_handle == handle)
            break;

    }

    if(i == BTAPP_PAN_NUM_SERVICES)
        return;

    if(!btapp_pan_cb.app_cb[app_id].ping_sent)
    {
        btapp_pan_ping_peer(handle,  btapp_pan_cb.app_cb[app_id].peer_bdaddr, btapp_cb.local_bd_addr);
        btapp_pan_cb.app_cb[app_id].ping_sent = TRUE;
    }
    else
    {

        if(p_data_to_send)
        {

            PAN_WriteBuf (handle, btapp_pan_cb.app_cb[app_id].peer_bdaddr, btapp_cb.local_bd_addr, PROTOCOL_ID_IPV4, p_data_to_send, FALSE);
            p_data_to_send = NULL;
        }

    }

}

/*******************************************************************************
**
** Function         bta_pan_co_tx_write
**
** Description      This function is called by PAN to send data to the phone
**                  when the TX path is configured to use a push interface.
**                  The implementation of this function must copy the data to
**                  the phone's memory.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_tx_write(UINT16 handle, UINT8 app_id, BD_ADDR src, BD_ADDR dst, UINT16 protocol, UINT8 *p_data,
                                UINT16 len, BOOLEAN ext, BOOLEAN forward)
{
     APPL_TRACE_DEBUG0("bta_pan_co_tx_write");

}

/*******************************************************************************
**
** Function         bta_pan_co_tx_writebuf
**
** Description      This function is called by PAN to send data to the phone
**                  when the TX path is configured to use a push interface with
**                  zero copy.  The phone must free the buffer using function
**                  GKI_freebuf() when it is through processing the buffer.
**
**
** Returns          TRUE if flow enabled
**
*******************************************************************************/
void  bta_pan_co_tx_writebuf(UINT16 handle, UINT8 app_id, BD_ADDR src, BD_ADDR dst, UINT16 protocol, BT_HDR *p_buf,
                                   BOOLEAN ext, BOOLEAN forward)
{

    APPL_TRACE_DEBUG0("bta_pan_co_tx_writebuf");


}

/*******************************************************************************
**
** Function         bta_pan_co_rx_flow
**
** Description      This function is called by PAN to enable or disable
**                  data flow on the RX path when it is configured to use
**                  a push interface.  If data flow is disabled the phone must
**                  not call bta_pan_ci_rx_write() or bta_pan_ci_rx_writebuf()
**                  until data flow is enabled again.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_rx_flow(UINT16 handle, UINT8 app_id, BOOLEAN enable)
{

    APPL_TRACE_DEBUG0("bta_pan_co_rx_flow");

}

/*******************************************************************************
**
** Function         bta_pan_co_filt_ind
**
** Description      protocol filter indication from peer device
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_pfilt_ind(UINT16 handle, BOOLEAN indication, tBTA_PAN_STATUS result,
                                    UINT16 len, UINT8 *p_filters)
{


}

/*******************************************************************************
**
** Function         bta_pan_co_mfilt_ind
**
** Description      multicast filter indication from peer device
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_mfilt_ind(UINT16 handle, BOOLEAN indication, tBTA_PAN_STATUS result,
                                    UINT16 len, UINT8 *p_filters)
{


}

/*******************************************************************************
**
** Function         btapp_pan_ping_peer
**
** Description      sends a ping to peer device
**
** Returns          void
**
*******************************************************************************/
extern void btapp_pan_ping_peer(UINT16 handle, BD_ADDR p_src_bda, BD_ADDR p_dst_bda)
{

    BT_HDR * p_buf;
    UINT8  *p, *p_start, *p_csum;
    UINT16          xx, csum;
    static UINT16   ping_id = 0x1000, ping_seq = 0x1000;
    UINT32          src_ip, dst_ip;

    src_ip = scru_ascii_2_ip_addr (p_src_ip);
    dst_ip = scru_ascii_2_ip_addr (p_dst_ip);


    if((p_buf = (BT_HDR *) GKI_getpoolbuf(PAN_POOL_ID)) != NULL)
    {

        p_buf->offset = PAN_MINIMUM_OFFSET;
        p = p_start = (UINT8 *)(p_buf + 1) + p_buf->offset;

        /* First, the IP header */
        UINT8_TO_BE_STREAM  (p, 0x45);              /* Protocol/len */
        UINT8_TO_BE_STREAM  (p, 0x00);              /* type of service */
        UINT16_TO_BE_STREAM (p, BTAPP_PAN_PING_LENGTH + 8 + 20);      /* Total length */
        UINT16_TO_BE_STREAM (p, btapp_pan_ip_id++);       /* ID */
        UINT16_TO_BE_STREAM (p, 0x00);              /* Offset */
        UINT8_TO_BE_STREAM  (p, 0x10);              /* Time to live */
        UINT8_TO_BE_STREAM  (p, IPPROTO_ICMP);      /* Protocol (ICMP) */

        p_csum = p;
        UINT16_TO_BE_STREAM (p, 0x0000);            /* Checksum holder */
        UINT32_TO_BE_STREAM (p, src_ip);            /* Source IP */
        UINT32_TO_BE_STREAM (p, dst_ip);            /* Destination IP */

        csum = calc_checksum (p_start, 20);
        UINT16_TO_BE_STREAM (p_csum, csum);         /* Put in checksum */

        /* Now the ICMP header */
        UINT8_TO_BE_STREAM  (p, 0x08);              /* ICMP type - ping */
        UINT8_TO_BE_STREAM  (p, 0x00);              /* ICMP code */
        p_csum = p;
        UINT16_TO_BE_STREAM (p, 0x0000);            /* Checksum holder */
        UINT16_TO_BE_STREAM (p, ping_id);           /* ID */
        UINT16_TO_BE_STREAM (p, ping_seq);          /* Seq */

        /* Ping data */
        for (xx = 0; xx < BTAPP_PAN_PING_LENGTH; xx++)
            *p++ = (UINT8) xx;

        csum = calc_checksum (p_start + 20, (UINT16)(BTAPP_PAN_PING_LENGTH + 8));
        UINT16_TO_BE_STREAM (p_csum, csum);         /* Checksum */

        p_buf->len = (UINT16) (BTAPP_PAN_PING_LENGTH + 28);

        PAN_WriteBuf (handle,  p_dst_bda,  p_src_bda, PROTOCOL_ID_IPV4, p_buf, FALSE);

    }
}

/*******************************************************************************
** Function              check_send_ping_rsp
**
** Description           This function checks if the received packet was
**                       a Ping, and replies if yes.
**
*******************************************************************************/
static BOOLEAN btapp_pan_check_send_ping_rsp (UINT16 handle, BT_HDR *p_data, BD_ADDR p_src_bda, BD_ADDR p_dst_bda)
{
    UINT8           *p, *p1;
    UINT16          xx;
    tIP_STRUCT      *p_ip;
    tICMP_STRUCT    *p_icmp;
    UINT32          ipaddr;

    /* Check if a ping */
    p = (UINT8 *)(p_data + 1) + p_data->offset;

    p_ip   = (tIP_STRUCT *)  p;
    p_icmp = (tICMP_STRUCT *)(p + 20);

    /* If this is an ping request, reply to it */
    if ((p_ip->ip_p == IPPROTO_ICMP) && (p_icmp->ic_type == 8) && (p_data->len >= 28))
    {
        /* Ensure we have space */
        if (p_data->offset < PAN_MINIMUM_OFFSET)
        {
            UINT16 diff = PAN_MINIMUM_OFFSET - p_data->offset;
            p = p + p_data->len - 1;
            for (xx = 0; xx < p_data->len; xx++, p--)
                p[diff] = *p;

            p_data->offset = PAN_MINIMUM_OFFSET;
            p = (UINT8 *)(p_data + 1) + p_data->offset;
            p_ip   = (tIP_STRUCT *) p;
            p_icmp = (tICMP_STRUCT *)(p + 20);
        }

        p1 = p + 16;
        BE_STREAM_TO_UINT32 (ipaddr, p1);
        if (BTAPP_SRC_IP_HEX != ipaddr)
        {
            GKI_freebuf (p_data);
            return (TRUE);
        }

        /* Switch source and destination addresses */
        ipaddr       = p_ip->ip_src;
        p_ip->ip_src = p_ip->ip_dst;
        p_ip->ip_dst = ipaddr;
        p_ip->ip_sum = 0x0000;       /* place holder for IP header checksum */

        /* calculate the header checksum */
        xx = calc_checksum (p, 20);
        UINT16_TO_BE_FIELD (&p_ip->ip_sum, xx);
               UINT16_TO_FIELD (&p_ip->ip_sum, xx);

        p_icmp->ic_type = 0;        /* Echo reply */
        p_icmp->ic_cksum = 0;

        /* calculate the header and data checksum */
        xx = calc_checksum (p + 20, (UINT16)(p_data->len - 20));
        UINT16_TO_BE_FIELD (&p_icmp->ic_cksum, xx);
                UINT16_TO_FIELD (&p_icmp->ic_cksum, xx);

        p_data_to_send = p_data;

        bta_pan_ci_rx_ready(handle);

        return (TRUE);
    }

    /* free buffer if its not a ping */
    GKI_freebuf (p_data);

    return (FALSE);
}

/*******************************************************************************
** Function              calc_checksum
**
** Description           This function calculate check sum
**
*******************************************************************************/
static UINT16 calc_checksum (UINT8 *p_data, UINT16 len)
{
    int     xx, sum = 0;
    UINT16  *p16 = (UINT16 *)p_data;
    UINT16  len16 = len / 2, answer, aa;

    for (xx = 0; xx < len16; xx++)
        sum += *p16++;

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                 /* add carry */
    answer = ~sum;                      /* truncate to 16 bits */

    /* Flip the order of the checksum */
    aa = ((answer >> 8) & 0xff) | ((answer << 8) & 0xFF00);

    return (aa);
}

/*******************************************************************************
** Function              scru_ascii_2_ip_addr
**
** Description           This function converts ip addr from script to UINT32
**
**
*******************************************************************************/
static UINT32 scru_ascii_2_ip_addr (char *p_ascii)
{
    int     x;
    UINT8   c;
    char    *p_next;
    UINT32  ip;

    if ((!p_ascii) || (*p_ascii == '\0'))
        return 0;

    ip = 0;
    for (x = 0; x < 4; x++)
    {
        if (x)
            ip <<= 8;

        c = (UINT8)strtoul(p_ascii, &p_next, 0);
        p_ascii = p_next + 1;   /* skip dot */

        ip |= c;
    }

    return ip;
}

#endif

