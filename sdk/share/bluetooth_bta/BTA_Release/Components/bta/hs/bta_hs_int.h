/*****************************************************************************
**
**  Name:           bta_hs_int.h
**
**  Description:    This is the private interface file for the BTA mono headset
**                  .
**
**  Copyright (c) 2003-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_HS_INT_H
#define BTA_HS_INT_H

#include "bta_sys.h"
#include "bta_api.h"
#include "bta_hs_api.h"
#include "bta_hs_at.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/* Number of SCBs (HS service instances that can be registered) */
#ifndef BTA_HS_NUM_SCB
#define BTA_HS_NUM_SCB          2
#endif

/* RFCOMM MTU SIZE */
#define BTA_HS_MTU              255

/* Internal profile indexes */
#define BTA_HS_HSP              0       /* index for HSP */
#define BTA_HS_HFP              1       /* index for HFP */
#define BTA_HS_NUM_IDX          2       /* number of profile indexes */

/* profile role for connection */
#define BTA_HS_ACP              0       /* accepted connection */
#define BTA_HS_INT              1       /* initiating connection */

#if (BTA_HFP_VERSION >= HFP_VERSION_1_7 && BTA_HFP_HF_IND_SUPPORTED == TRUE)
/* Max number of peer HF indicator */
#define BTA_HS_MAX_NUM_PEER_HF_IND     20
#endif

/* feature mask that matches spec */
#if (BTM_WBS_INCLUDED == TRUE )
#define BTA_HS_FEAT_SPEC        (BTA_HS_FEAT_ECNR | BTA_HS_FEAT_3WAY | \
                                 BTA_HS_FEAT_CLIP | BTA_HS_FEAT_VREC | \
                                 BTA_HS_FEAT_RVOL | BTA_HS_FEAT_ECS  | \
                                 BTA_HS_FEAT_ECC  | BTA_HS_FEAT_CODEC| \
                                 BTA_HS_FEAT_HF_IND | BTA_HS_FEAT_ESCO | \
                                 BTA_HS_FEAT_VOIP)
#else
#define BTA_HS_FEAT_SPEC        (BTA_HS_FEAT_ECNR | BTA_HS_FEAT_3WAY | \
                                 BTA_HS_FEAT_CLIP | BTA_HS_FEAT_VREC | \
                                 BTA_HS_FEAT_RVOL | BTA_HS_FEAT_ECS  | \
                                 BTA_HS_FEAT_ECC  | BTA_HS_FEAT_HF_IND | \
                                 BTA_HS_FEAT_ESCO)
#endif

#define BTA_HS_SDP_FEAT_SPEC    (BTA_HS_FEAT_ECNR | BTA_HS_FEAT_3WAY | \
                                 BTA_HS_FEAT_CLIP | BTA_HS_FEAT_VREC | \
                                 BTA_HS_FEAT_RVOL)

#define BTA_HS_CMD_TIMEOUT_VALUE 5000 /* 5 seconds */

enum
{
    /* these events are handled by the state machine */
    BTA_HS_API_REGISTER_EVT = BTA_SYS_EVT_START(BTA_ID_HS),
    BTA_HS_API_DEREGISTER_EVT,
    BTA_HS_API_OPEN_EVT,
    BTA_HS_API_CLOSE_EVT,
    BTA_HS_API_AUDIO_OPEN_EVT,
    BTA_HS_API_AUDIO_CLOSE_EVT,
    BTA_HS_API_AUDIO_OPEN_RSP_EVT,
    BTA_HS_API_AUDIO_SET_PRIO_EVT,
    BTA_HS_API_CMD_EVT,
    BTA_HS_RFC_OPEN_EVT,
    BTA_HS_RFC_CLOSE_EVT,
    BTA_HS_RFC_CONN_LOSS_EVT,
    BTA_HS_RFC_SRV_CLOSE_EVT,
    BTA_HS_RFC_DATA_EVT,
    BTA_HS_SCO_OPEN_EVT,
    BTA_HS_SCO_CLOSE_EVT,
    BTA_HS_DISC_ACP_RES_EVT,
    BTA_HS_DISC_INT_RES_EVT,
    BTA_HS_DISC_OK_EVT,
    BTA_HS_DISC_FAIL_EVT,
    BTA_HS_BTM_AUDIO_OPEN_REQ_EVT,
    BTA_HS_CMD_TIMEOUT_EVT,
    BTA_HS_CI_SCO_DATA_EVT,

    /* these events are handled outside of the state machine */
    BTA_HS_API_ENABLE_EVT,
    BTA_HS_API_DISABLE_EVT
};

/* AT commands */
enum
{
    BTA_HS_CMD_VGS,
    BTA_HS_CMD_VGM,
    BTA_HS_CMD_CKPD,
    BTA_HS_CMD_A,
    BTA_HS_CMD_D,
    BTA_HS_CMD_CCWA,
    BTA_HS_CMD_CHLD,
    BTA_HS_CMD_CHUP,
    BTA_HS_CMD_CIND,
    BTA_HS_CMD_CLIP,
    BTA_HS_CMD_CMER,
    BTA_HS_CMD_VTS,
    BTA_HS_CMD_BINP,
    BTA_HS_CMD_BLDN,
    BTA_HS_CMD_BVRA,
    BTA_HS_CMD_BRSF,
    BTA_HS_CMD_NREC,
    BTA_HS_CMD_CNUM,
    BTA_HS_CMD_BTRH,
    BTA_HS_CMD_COPS,
    BTA_HS_CMD_CMEE,
    BTA_HS_CMD_CLCC,
    BTA_HS_CMD_BCC,
    BTA_HS_CMD_BCS,
    BTA_HS_CMD_BAC,
    BTA_HS_CMD_BIA,
    BTA_HS_CMD_BIND,
    BTA_HS_CMD_BIEV,
    BTA_HS_CMD_UNAT
};

/* Actions to perform after a SCO event */
enum
{
    BTA_HS_POST_SCO_NONE,       /* no action */
    BTA_HS_POST_SCO_CLOSE_RFC  /* close RFCOMM channel after SCO closes */
};


/* state machine states */
enum
{
    BTA_HS_INIT_ST,
    BTA_HS_OPENING_ST,
    BTA_HS_OPEN_ST,
    BTA_HS_CLOSING_ST
};

/*****************************************************************************
**  Data types
*****************************************************************************/

/* data type for BTA_HS_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_HS_CBACK      *p_cback;

} tBTA_HS_API_ENABLE;

/* data type for BTA_HS_API_REGISTER_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_SERVICE_MASK   services;
    tBTA_SEC            sec_mask;
    tBTA_HS_FEAT        features;
    tBTA_HS_SETTINGS    settings;
    char                p_name[2][BTA_SERVICE_NAME_LEN+1];
    UINT8               app_id;

} tBTA_HS_API_REGISTER;

/* data type for BTA_HS_API_OPEN_EVT */
typedef struct
{
    BT_HDR     hdr;
    BD_ADDR    bd_addr;
    tBTA_SEC   sec_mask;
    tBTA_SERVICE_MASK services;

} tBTA_HS_API_OPEN;

/* data type for BTA_HS_API_CMD_EVT */
typedef struct
{
    BT_HDR     hdr;
    tBTA_HS_CMD command;
    tBTA_HS_CMD_DATA data;

} tBTA_HS_API_CMD;

/* data type for BTA_HS_API_AUDIO_OPEN_RSP_EVT */
typedef struct
{
    BT_HDR     hdr;
    BOOLEAN accept;
} tBTA_HS_API_AUDIO_OPEN_RSP;

/* data type for BTA_HS_DISC_RESULT_EVT */
typedef struct
{
    BT_HDR          hdr;
    UINT16          status;
} tBTA_HS_DISC_RESULT;

/* data type for RFCOMM events */
typedef struct
{
    BT_HDR          hdr;
    UINT16          port_handle;
} tBTA_HS_RFC;


/* union of all event datatypes */
typedef union
{
    BT_HDR                  hdr;
    tBTA_HS_API_ENABLE      api_enable;
    tBTA_HS_API_REGISTER      api_register;
    tBTA_HS_API_OPEN        api_open;
    tBTA_HS_API_CMD         api_cmd;
    tBTA_HS_DISC_RESULT     disc_result;
    tBTA_HS_RFC             rfc;
    tBTA_HS_API_AUDIO_OPEN_RSP audio_conn_rsp;
} tBTA_HS_DATA;

#if (BTA_HFP_VERSION >= HFP_VERSION_1_7 && BTA_HFP_HF_IND_SUPPORTED == TRUE)
/* type for HF indicator */
typedef struct
{
    UINT32          ind_id;
    BOOLEAN         is_enable;
    UINT32          ind_val;
} tBTA_HS_PEER_HF_IND;
#endif

/* type for each service control block */
typedef struct
{
    UINT16              serv_handle[BTA_HS_NUM_IDX]; /* RFCOMM server handles */
    BD_ADDR             peer_addr;      /* peer bd address */
    tBTA_HS_AT_CB       at_cb;          /* AT command interpreter */
    tSDP_DISCOVERY_DB   *p_disc_db;     /* pointer to discovery database */
    tBTA_SERVICE_MASK   reg_services;   /* services specified in register API */
    tBTA_SERVICE_MASK   open_services;  /* services specificed in open API */
    BUFFER_Q bta_at_cmd_queue;              /* Q for sedning AT commands one after the other */
    TIMER_LIST_ENT bta_at_cmd_queue_timer;  /* timer to recover if peer do not respond to AT command */
    UINT8 bta_at_cmd_queue_depth;       /* depth of AT command Q*/
    UINT16              conn_handle;    /* RFCOMM handle of connected service */
    UINT16              sco_idx;            /* SCO handle */
    tBTA_SEC            serv_sec_mask;  /* server security mask */
    tBTA_SEC            cli_sec_mask;   /* client security mask */
    tBTA_HS_FEAT        features;       /* features registered by application */
    tBTA_HS_PEER_FEAT   peer_features;  /* peer device features */
    UINT16              peer_hf_version;/* Handsfree profile version of te peer */
#if (BTM_WBS_INCLUDED == TRUE )
    tBTM_SCO_CODEC_TYPE sco_codec;      /* codec type requested from the peer */
#endif
    BOOLEAN             in_use;         /* scb in use */
    BOOLEAN             dealloc;        /* TRUE if service shutting down */
    tBTA_HS_SETTINGS    settings;       /* current headset settings */
    BOOLEAN             svc_conn;       /* set to TRUE when service level connection up */
    BOOLEAN             sco_open;       /* set to TRUE when SCO connection is open */
    UINT8               state;          /* state machine state */
    UINT8               conn_service;   /* connected service */
    UINT8               peer_scn;       /* peer scn */
    UINT8               app_id;         /* application id */
    UINT8               role;           /* initiator/acceptor role */
    UINT8               slc_at_init_state; /* at-cmd state during slc establishment */
    UINT8               post_sco;          /* action after sco connection open/close */
    BOOLEAN             retry_with_sco_only; /* ind to try with SCO only if eSCO fails */
    BOOLEAN             using_enh_sco;  /* TRUE if ctlr supports enhanced eSCO cmds */
    UINT16              handle;         /* handle used by appl to identify connection */
#if (BTA_HFP_VERSION >= HFP_VERSION_1_7 && BTA_HFP_HF_IND_SUPPORTED == TRUE)
    tBTA_HS_PEER_HF_IND peer_hf_ind[BTA_HS_MAX_NUM_PEER_HF_IND]; /* Peer HF indicator status */
#endif
} tBTA_HS_SCB;

/* type for each profile */
typedef struct
{
    UINT32          sdp_handle;
    UINT8           scn;
} tBTA_HS_PROFILE;

/* type for HS control block */
typedef struct
{
    tBTA_HS_SCB          scb[BTA_HS_NUM_SCB];       /* service control blocks */
    tBTA_HS_PROFILE      profile[BTA_HS_NUM_IDX];   /* profile-specific data */
    tBTA_HS_CBACK        *p_cback;                  /* application callback */
    BOOLEAN              sco_param_updated;         /* TRUE if updated to non-default */
    tBTM_ENH_ESCO_PARAMS sco_params;                /* Enhanced ESCO parameters */
} tBTA_HS_CB;



/*****************************************************************************
**  Global data
*****************************************************************************/

/* control block declaration */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_HS_CB bta_hs_cb;
#else
extern tBTA_HS_CB *bta_hs_cb_ptr;
#define bta_hs_cb (*bta_hs_cb_ptr)
#endif

extern const UINT16 bta_hs_uuid[BTA_HS_NUM_IDX];
extern const UINT8 bta_hs_sec_id[BTA_HS_NUM_IDX];
extern const tBTA_SERVICE_ID bta_hs_svc_id[BTA_HS_NUM_IDX];
extern const tBTA_SERVICE_MASK bta_hs_svc_mask[BTA_HS_NUM_IDX];
extern const tBTA_HS_AT_RES * bta_hs_res_tbl[BTA_HS_NUM_IDX];

/* config struct */
extern tBTA_HS_CFG *p_bta_hs_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/
/* main functions */
extern void bta_hs_scb_dealloc(tBTA_HS_SCB *p_scb);
extern BOOLEAN bta_hs_hdl_event(BT_HDR *p_msg);
extern void bta_hs_sm_execute(tBTA_HS_SCB *p_scb, UINT16 event, tBTA_HS_DATA *p_data);
extern UINT8 bta_hs_service_to_idx(tBTA_SERVICE_MASK services);
extern UINT16 bta_hs_scb_to_idx(tBTA_HS_SCB *p_scb);
extern tBTA_HS_SCB *bta_hs_scb_by_idx(UINT16 idx);
extern UINT16 bta_hs_idx_by_bdaddr(BD_ADDR peer_addr);


extern void bta_hs_free_db(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);

extern void bta_hs_api_enable(tBTA_HS_DATA *p_data);
extern void bta_hs_api_disable(tBTA_HS_DATA *p_data);
extern void bta_hs_register(tBTA_HS_SCB * p_scb,tBTA_HS_DATA *p_data);
extern void bta_hs_deregister(tBTA_HS_SCB * p_scb, tBTA_HS_DATA *p_data);

extern void bta_hs_start_open(tBTA_HS_SCB * p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_start_close(tBTA_HS_SCB * p_scb, tBTA_HS_DATA *p_data);

extern BOOLEAN bta_hs_add_record(UINT16 service_uuid, char *p_service_name, UINT8 scn,
                                 tBTA_HS_FEAT features, UINT32 sdp_handle);
extern void bta_hs_create_records(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_del_records(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern BOOLEAN bta_hs_sdp_find_attr(tBTA_HS_SCB *p_scb, tBTA_SERVICE_MASK service);
extern void bta_hs_do_disc(tBTA_HS_SCB *p_scb, tBTA_SERVICE_MASK service);
extern void bta_hs_free_db(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);

/* RFCOMM functions */
extern void bta_hs_start_servers(tBTA_HS_SCB *p_scb, tBTA_SERVICE_MASK services);
extern void bta_hs_close_servers(tBTA_HS_SCB *p_scb, tBTA_SERVICE_MASK services);
extern void bta_hs_rfc_do_close(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_rfc_do_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_setup_port(tBTA_HS_SCB *p_scb, UINT16 handle);


extern void bta_hs_at_hsp_cback(tBTA_HS_SCB *p_scb, UINT16 cmd, char *p_arg);
extern void bta_hs_at_hfp_cback(tBTA_HS_SCB *p_scb, UINT16 cmd, char *p_arg);
extern void bta_hs_at_err_cback(tBTA_HS_SCB *p_scb, BOOLEAN unknown, char *p_arg);

/* AT commands */
extern void bta_hs_at_send_cmd(tBTA_HS_SCB *p_scb, UINT8 cmd, UINT8 arg_type, UINT8 arg_format,
                        const char * p_arg, INT16 int_arg );


extern void bta_hs_at_slc_hfp_conn(tBTA_HS_SCB *p_scb);
extern void bta_hs_at_slc_hsp_conn(tBTA_HS_SCB *p_scb);



/* Action functions */
extern void bta_hs_start_dereg(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_start_close(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_start_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_disc_int_res(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_disc_acp_res(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_disc_fail(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_rfc_fail(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_rfc_close(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_rfc_conn_loss(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_rfc_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_rfc_acp_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_rfc_data(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_listen(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_close(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_shutdown(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_conn_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_conn_close(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_post_sco_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_post_sco_close(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_svc_conn_open(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_cmd(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_cmd_timeout(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_audio_open_rsp(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_conn_rsp(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_sco_conn_req(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);
extern void bta_hs_ci_sco_data(tBTA_HS_SCB *p_scb, tBTA_HS_DATA *p_data);

/* Queued AT command */
extern void bta_at_cmd_queue_init(tBTA_HS_SCB *p_scb);
extern void bta_at_cmd_queue_free(tBTA_HS_SCB *p_scb);
extern void bta_at_cmd_queue_enqueue(tBTA_HS_SCB *p_scb, UINT8 cmd, char *buf, UINT16 len);
extern UINT8 bta_at_cmd_queue_handle_res(tBTA_HS_SCB *p_scb);
extern void bta_at_cmd_queue_send(tBTA_HS_SCB *p_scb);
extern void *bta_at_cmd_queue_dequeue(tBTA_HS_SCB *p_scb);
extern void bta_at_cmd_queue_timeout(tBTA_HS_SCB *p_scb);
extern void bta_at_cmd_queue_flush(tBTA_HS_SCB *p_scb);
extern void bta_at_cmd_flush_cmd_in_queue(tBTA_HS_SCB *p_scb, UINT8 cmd);

extern void bta_hs_set_esco_param(BOOLEAN set_reset, tBTM_ENH_ESCO_PARAMS *param);
extern void bta_hs_cback_open(tBTA_HS_SCB *p_scb, tBTA_HS_STATUS status);

#if (BRCM_LPST_INCLUDED == TRUE)
extern void bta_hs_lpst_sync (tBTA_HS_SCB *p_scb, BOOLEAN snapshot_sync);
extern BOOLEAN bta_hs_lpst_profile_sync(BOOLEAN snapshot_sync);
extern void bta_hs_lpst_process_control_blocks (UINT8 *p, UINT16 cb_len);
extern BOOLEAN bta_hs_lpst_validate_switch_state(void);
#endif

#endif /* BTA_HS_INT_H */



