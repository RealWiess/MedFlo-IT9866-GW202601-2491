/*****************************************************************************
**
**  Name:           bta_rc_int.h
**
**  Description:    This is the private interface file for the BTA_RC
**
**  Copyright (c) 2013 - 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_RC_INT_H
#define BTA_RC_INT_H

#include "bta_sys.h"
#include "bta_api.h"
#include "bta_rc_api.h"


/*****************************************************************************
**  Constants
*****************************************************************************/
/* maximum number of peer AVRCP connecitons */
#ifndef BTA_RC_MAX_CONN
#define BTA_RC_MAX_CONN         2           /* Maximum avrc connections supported. Default: one acceptor, one initiator */
#endif

#define BTA_RC_IDX_ACP          0           /* Entry in peer_cb table reserved for acceptor connection */

/* BTA_RC states */
enum
{
    BTA_RC_ST_UNUSED,
    BTA_RC_ST_LISTENING,        /* Acceptor, waiting for incoming connection */
    BTA_RC_ST_CONNECTING,       /* Initiator, waiting for connection to complete */
    BTA_RC_ST_CONNECTED         /* Acceptor or Initiator, avrc connection is up */
};

/* BTA_RC internal events */
enum
{
    BTA_RC_API_ENABLE_EVT = BTA_SYS_EVT_START(BTA_ID_RC),
    BTA_RC_API_DISABLE_EVT,
    BTA_RC_API_OPEN_EVT,
    BTA_RC_API_CLOSE_EVT,
    BTA_RC_API_SEND_EVT,

    BTA_RC_AVRC_OPEN_EVT,
    BTA_RC_AVRC_CLOSE_EVT,
    BTA_RC_AVRC_DATA_EVT,

    BTA_RC_EVT_MAX
};

/* Message types */
enum
{
    BTA_RC_MSG_TYPE_UNIT_INFO,
    BTA_RC_MSG_TYPE_SUBUNIT_INFO,
    BTA_RC_MSG_TYPE_PASS_THRU,
    BTA_RC_MSG_TYPE_VENDOR,
    BTA_RC_MSG_TYPE_METADATA
};
typedef UINT8 tBTA_RC_MSG_TYPE;

/* size of database for service discovery */
#define BTA_RC_DISC_BUF_SIZE                450

/* Flags and masks for layer_specific field of bta_rc messages */
#define BTA_RC_FLAG_IS_CMD  0x0100
#define BTA_RC_HANDLE_MASK  0x00FF

/* Flags for bta_rc_cb control block */
#define BTA_RC_CB_FLAG_DISABLE_PENDING      0x01    /* Disable notification pending until all connections are closed */
#define BTA_RC_CB_FLAG_DISC_CB_IN_USE       0x02    /* Discovery database in use */

/* Data for BTA_RC events */

/* Data for BTA_RC_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_SEC            sec_mask;
    tBTA_RC_FEAT        features;
    tBTA_RC_CBACK       *p_cback;
} tBTA_RC_API_ENABLE;

/* Data for BTA_RC_API_SEND_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT32              company_id;
    UINT32              code;
    UINT32              extra;
    tBTA_RC_MSG_TYPE    msg_type;
    UINT8               rc_handle;
    UINT8               label;
    UINT8               page;
    UINT8               subunit_type[AVRC_SUB_TYPE_LEN];
    BT_HDR              *p_metadata;
} tBTA_RC_API_SEND;

/* Data for BTA_RC_API_OPEN_EVT */
typedef struct
{
    BT_HDR  hdr;
    BD_ADDR bd_addr;
    tBTA_RC_FEAT role;
} tBTA_RC_API_OPEN;

typedef struct
{
    BT_HDR  hdr;
    BD_ADDR bd_addr;
    UINT8   avrc_handle;
} tBTA_RC_CONN_CHG;

/* Data type for BTA_RC_AVRC_DATA_EVT */
typedef struct
{
    BT_HDR              hdr;
    tAVRC_MSG           msg;
    UINT8               avrc_handle;
    UINT8               label;
    UINT8               opcode;
} tBTA_RC_AVRC_DATA;

typedef union
{
    BT_HDR              hdr;
    tBTA_RC_API_ENABLE  api_enable;
    tBTA_RC_API_OPEN    api_open;
    tBTA_RC_API_SEND    api_send;
    tBTA_RC_CONN_CHG    conn_chg;
    tBTA_RC_AVRC_DATA   avrc_data;
}  tBTA_RC_DATA;

/* BTA_RC peer control block */
typedef struct
{
    BD_ADDR bd_addr;
    UINT32  features;
    UINT8   avrc_handle;
    UINT8   state;
    
    /* SDP info from peer */
    tBTA_RC_PEER_FEAT peer_feat;
} tBTA_RC_PEER_CB;

/* BTA_RC control block */
typedef struct
{
    tBTA_RC_CBACK   *p_cback;                   /* BTA_RC event notification callback fcn */
    UINT32          sdp_ct_handle;              /* sdp handle for CT record */
    UINT32          sdp_tg_handle;              /* sdp handle for TG record */
    BD_ADDR         sdp_bda;                    /* bdaddr for which we are currently performing SDP */
    UINT8           flags;                      /* See BTA_RC_CB_FLAG_* definitions */

    /* AVRC CT control blocks */
    tBTA_RC_PEER_CB peer_cb[BTA_RC_MAX_CONN];

    /* SDP database */
    tSDP_DISCOVERY_DB *p_disc_db;
} tBTA_RC_CB;


/* Internal constant definition */
#define BTA_RC_SUBTYPE_SHIFT        3
#define BTA_RC_SUBPAGE_SHIFT        4 
#define BTA_RC_SUBID_IGNORE         0x07    /* Reserved subunit_id value for 'ignore' */
#define BTA_RC_SUBINIT_PAGE_MAX     0x07    /* Maximum value for SUBUNIT INFO page and extenstion code params */
#define BTA_RC_INVALID_EVT          0xFF

/*****************************************************************************
**  Global data
*****************************************************************************/

/* control block declaration */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_RC_CB bta_rc_cb;
#else
extern tBTA_RC_CB *bta_rc_cb_ptr;
#define bta_rc_cb (*bta_rc_cb_ptr)
#endif

/* Configuration structures */
extern tBTA_RC_CFG *p_bta_rc_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/
/* utility functions */

/* main event handler for BTA_RC  */
BOOLEAN bta_rc_hdl_event(BT_HDR *p_msg);

/* action functions */
BOOLEAN bta_rc_api_enable(tBTA_RC_DATA *p_data);    /* BTA_RC_API_ENABLE_EVT */
BOOLEAN bta_rc_api_disable(tBTA_RC_DATA *p_data);   /* BTA_RC_API_DISABLE_EVT */
BOOLEAN bta_rc_api_open(tBTA_RC_DATA *p_data);      /* BTA_RC_API_OPEN_EVT */
BOOLEAN bta_rc_api_close(tBTA_RC_DATA *p_data);     /* BTA_RC_API_CLOSE_EVT */
BOOLEAN bta_rc_api_send(tBTA_RC_DATA *p_data);      /* BTA_RC_API_SEND_EVT */
BOOLEAN bta_rc_open(tBTA_RC_DATA *p_data);          /* BTA_RC_OPEN_EVT */
BOOLEAN bta_rc_close(tBTA_RC_DATA *p_data);         /* BTA_RC_CLOSE_EVT */
BOOLEAN bta_rc_rcv(tBTA_RC_DATA *p_data);           /* BTA_RC_RCV_EVT */
BOOLEAN bta_rc_timeout(tBTA_RC_DATA *p_data);       /* BTA_RC_TIMEOUT_EVT */

#endif /* BTA_AV_INT_H */
