/*****************************************************************************
**
**  Name:           bta_lecoc_int.h
**
**  Description:    This is the private interface file for the BTA LE COC I/F
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_LECOC_INT_H
#define BTA_LECOC_INT_H

#include "bta_sys.h"
#include "bta_api.h"
#include "bta_lecoc_api.h"
#include "l2c_api.h"
/*****************************************************************************
**  Constants
*****************************************************************************/
enum
{
    /* these events are handled by the state machine */
    BTA_LECOC_API_ENABLE_EVT = BTA_SYS_EVT_START(BTA_ID_LECOC),
    BTA_LECOC_API_DISABLE_EVT,
    BTA_LECOC_API_CONNECT_EVT,
    BTA_LECOC_API_CLOSE_EVT,
    BTA_LECOC_API_WRITE_EVT,
    BTA_LECOC_API_START_SERVER_EVT,
    BTA_LECOC_API_STOP_SERVER_EVT,
    BTA_LECOC_API_FLOW_CONTROL_EVT,
    BTA_LECOC_CI_DATA_READY_EVT,

    BTA_LECOC_MAX_INT_EVT
};


enum
{
    BTA_LECOC_ST_NONE = 0,
    BTA_LECOC_ST_OPENING,
    BTA_LECOC_ST_OPEN,
    BTA_LECOC_ST_CLOSING
};
typedef UINT8  tBTA_LECOC_STATE;

typedef struct
{
    tBTA_LECOC_CBACK *p_cback;   /* the callback function */
    void                *p_ref;
    UINT16              le_psm;        /* the psm used for this server connection */
    UINT16              v_psm;          /* virtual PSM used on L2CAP interface */
    UINT16              rx_mtu;         /* RX MTU size */
    BOOLEAN             le_server;      /* if this is a server control black */
    tBTA_SEC            sec_mask;       /* service requirement for the server connectino */
    UINT8               sec_key_size;    /* minimum key size requirement */

}tBTA_LECOC_RCB;

/* L2CAP LE COC CCB control block */
typedef struct
{

    tBTA_LECOC_RCB      *p_rcb;             /* pointer to the RCB */
    BT_HDR              *p_tx_buf;          /* pending TX buffer */    
    BUFFER_Q            rx_queue;           /* Queue of buffers queued up for congestion  */    
    UINT32              tx_data_size;       /* tx data pending size */
    UINT32              tx_sent;            /* data size sent */
    UINT32              req_id;
    UINT16              tx_mtu_size;        /* tx MTU  */    
    UINT16              lcid;               /* l2cap CID */
    UINT16              handle;             /* BTA LE COC handle */    
    BOOLEAN             tx_buf_ready;       /* TX data ready */
    BOOLEAN             rx_cong;            /* receiving path is congested */
    BOOLEAN             tx_cong;            /* TRUE, if congested */    
    BD_ADDR             remote_bda;
    tBTA_LECOC_STATE    state;              /* the state of this control block */
} tBTA_LECOC_CCB;

/* data type for BTA_LECOC_API_CONNECT_EVT */
typedef struct
{
    BT_HDR          hdr;
    tBTA_SEC        sec_mask;
    UINT8           sec_key_size;
    UINT16          le_psm;
    UINT16          rx_mtu;
    BD_ADDR         peer_bd_addr;
    void            *p_ref;
    tBTA_LECOC_CBACK *p_cback;
}tBTA_LECOC_API_CONNECT;

/* data type for BTA_LECOC_API_START/STOP_SERVER_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_SEC            sec_mask;
    UINT8               sec_key_size;
    UINT16              local_psm;
    UINT16              rx_mtu;
    void                *p_ref;
    tBTA_LECOC_CBACK    *p_cback;
} tBTA_LECOC_API_SERVER;

/* data type for BTA_LECOC_API_CLOSE_EVT */
typedef struct
{
    BT_HDR          hdr;
    UINT16          handle;
} tBTA_LECOC_API_CLOSE;

/* data type for BTA_LECOC_API_FLOW_CONTROL_EVT */
typedef struct
{
    BT_HDR          hdr;
    BOOLEAN         flow_on;
} tBTA_LECOC_API_FLOW_CONTROL;

/* data type for BTA_LECOC_WRITE_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT32              req_id;
} tBTA_LECOC_API_WRITE;

typedef struct
{
    BT_HDR                      hdr;
    tBTA_LECOC_ENB_CBACK      *p_cback;
}tBTA_LECOC_API_ENABLE;

typedef struct
{
    BT_HDR                      hdr;
    UINT16                      len;
    tBTA_LECOC_STATUS           status;
}tBTA_LECOC_CI_DATA;

/* union of all data types */
typedef union
{
    /* GKI event buffer header */
    BT_HDR                      hdr;
    tBTA_LECOC_API_ENABLE       lecoc_enable;
    tBTA_LECOC_API_CONNECT      lecoc_connect;
    tBTA_LECOC_API_SERVER       lecoc_server;
    tBTA_LECOC_API_CLOSE        lecoc_close;
    tBTA_LECOC_API_WRITE        lecoc_write;
    tBTA_LECOC_API_FLOW_CONTROL lecoc_fc;
    tBTA_LECOC_CI_DATA          ci_data;
} tBTA_LECOC_MSG;

/* LE COC control block */
typedef struct
{
    BOOLEAN          enabled;
    tBTA_LECOC_CCB   lecoc_ccb[BTA_LECOC_MAX_CONN];    /* index is connection handle (index) */
    tBTA_LECOC_RCB   lecoc_rcb[BTA_LECOC_MAX_RCB];    /* index is connection handle (index) */
    tL2CAP_LE_APPL_INFO     lecoc_reg_info;                /* LE registration informatrion */
} tBTA_LECOC_CB;

/* LE COC control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_LECOC_CB bta_lecoc_cb;
#else
extern tBTA_LECOC_CB *bta_lecoc_cb_ptr;
#define bta_lecoc_cb (*bta_lecoc_cb_ptr)
#endif

extern BOOLEAN bta_lecoc_sm_execute(BT_HDR *p_msg);

extern void bta_lecoc_enable(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_disable(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_stop_server(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_start_server(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_write(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_close(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_connect(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_flow_control(tBTA_LECOC_MSG *p_data, tBTA_LECOC_CCB *p_ccb);
extern void bta_lecoc_ci_data_ready(tBTA_LECOC_MSG *p_msg, tBTA_LECOC_CCB *p_ccb);
#endif /* BTA_LECOC_INT_H */

