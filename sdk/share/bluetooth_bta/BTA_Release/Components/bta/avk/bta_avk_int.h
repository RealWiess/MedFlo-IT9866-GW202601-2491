/*****************************************************************************
**
**  Name:           bta_avk_int.h
**
**  Description:    This is the private interface file for the BTA advanced
**                  audio/video.
**
**  Copyright (c) 2004-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_AVK_INT_H
#define BTA_AVK_INT_H

#include "bta_sys.h"
#include "bta_api.h"
#include "bta_avk_api.h"
#include "avdt_api.h"


/*****************************************************************************
**  Constants
*****************************************************************************/

/* state machine states */
enum
{
    BTA_AVK_INIT_SST,
    BTA_AVK_SIG_OPENING_SST,
    BTA_AVK_SIG_OPEN_SST,
    BTA_AVK_INCOMING_SST,
    BTA_AVK_OPEN_SST,
    BTA_AVK_CLOSING_SST
};

enum
{
    /* these events are handled by the AV main state machine */
    BTA_AVK_API_DISABLE_EVT = BTA_SYS_EVT_START(BTA_ID_AVK),
    BTA_AVK_API_REMOTE_CMD_EVT,
    BTA_AVK_API_VENDOR_CMD_EVT,
    BTA_AVK_API_VENDOR_RSP_EVT,
    BTA_AVK_API_META_MSG_EVT,
    BTA_AVK_API_AVRCP_OPEN_REQ_EVT,
    BTA_AVK_API_AVRCP_CLOSE_REQ_EVT,
    BTA_AVK_SDP_AVRC_DISC_EVT,
    BTA_AVK_AVRC_OPEN_EVT,
    BTA_AVK_AVRC_CLOSE_EVT,
    BTA_AVK_AVRC_OPEN_REQ_EVT,
    BTA_AVK_AVRC_CLOSE_REQ_EVT,
    BTA_AVK_AVRC_MSG_EVT,
    BTA_AVK_AVRC_OPEN_REQ_AS_ACP_EVT,
    BTA_AVK_AVRCP_TIMER_EVT,
    BTA_AVK_CONN_CHG_EVT,
    BTA_AVK_DEREG_COMP_EVT,

    /* these events are handled by the AV stream state machine */
    BTA_AVK_API_OPEN_EVT,
    BTA_AVK_API_CLOSE_EVT,
    BTA_AVK_API_START_EVT,
    BTA_AVK_API_STOP_EVT,
    BTA_AVK_API_PROTECT_REQ_EVT,
    BTA_AVK_API_PROTECT_RSP_EVT,
    BTA_AVK_API_DELAY_RPT_EVT,
    BTA_AVK_CI_SETCONFIG_OK_EVT,
    BTA_AVK_CI_SETCONFIG_FAIL_EVT,
    BTA_AVK_CI_CP_SCMS_EVT,
    BTA_AVK_SDP_DISC_OK_EVT,
    BTA_AVK_SDP_DISC_FAIL_EVT,
    BTA_AVK_STR_DISC_OK_EVT,
    BTA_AVK_STR_DISC_FAIL_EVT,
    BTA_AVK_STR_GETCAP_OK_EVT,
    BTA_AVK_STR_GETCAP_FAIL_EVT,
    BTA_AVK_STR_OPEN_OK_EVT,
    BTA_AVK_STR_OPEN_FAIL_EVT,
    BTA_AVK_STR_START_OK_EVT,
    BTA_AVK_STR_START_FAIL_EVT,
    BTA_AVK_STR_CLOSE_EVT,
    BTA_AVK_STR_CONFIG_IND_EVT,
    BTA_AVK_STR_SECURITY_IND_EVT,
    BTA_AVK_STR_SECURITY_CFM_EVT,
    BTA_AVK_STR_SUSPEND_CFM_EVT,
    BTA_AVK_STR_RECONFIG_CFM_EVT,
    BTA_AVK_STR_RECONFIG_IND_EVT,
    BTA_AVK_AVDT_CONNECT_EVT,
    BTA_AVK_AVDT_DELAY_RPT_EVT,
#if (BTU_BTC_SNK_INCLUDED == TRUE)
    BTA_AVK_STR_BTC_START_OK_EVT,
    BTA_AVK_CI_AUDIO_BTC_START_EVT,
#endif
    BTA_AVK_AVDT_DISCONNECT_EVT,

    /* these events are handled outside of the state machine */
    BTA_AVK_API_ENABLE_EVT,
    BTA_AVK_API_REGISTER_EVT,
    BTA_AVK_API_DEREGISTER_EVT,
    BTA_AVK_API_UPDATE_SEPS_EVT,
    BTA_AVK_SIG_CHG_EVT,
    BTA_AVK_AVDT_RPT_CONN_EVT
#if (BTU_BTC_SNK_INCLUDED == TRUE)
    ,BTA_AVK_SYNC_REQ_EVT           /* Sync request from DM */
    ,BTA_AVK_JITTER_EXP_EVT         /* Jitter timer expired */
#endif
};

/* events for AV control block state machine */
#define BTA_AVK_FIRST_SM_EVT     BTA_AVK_API_DISABLE_EVT
#define BTA_AVK_LAST_SM_EVT      BTA_AVK_DEREG_COMP_EVT

/* events for AV stream control block state machine */
#define BTA_AVK_FIRST_SSM_EVT    BTA_AVK_API_OPEN_EVT
#define BTA_AVK_LAST_SSM_EVT     BTA_AVK_AVDT_DISCONNECT_EVT

/* events that do not go through state machine */
#define BTA_AVK_FIRST_NSM_EVT    BTA_AVK_API_ENABLE_EVT

#if (BTU_BTC_SNK_INCLUDED == TRUE)
#define BTA_AVK_LAST_EVT             BTA_AVK_JITTER_EXP_EVT
#else
#define BTA_AVK_LAST_EVT             BTA_AVK_AVDT_RPT_CONN_EVT
#endif

/* maximum number of SEPS in stream discovery results */
#define BTA_AVK_NUM_SEPS         8

/* size of database for service discovery */
#define BTA_AVK_DISC_BUF_SIZE        1000

/* offset of media type in codec info byte array */
#define BTA_AVK_MEDIA_TYPE_IDX       1

/* maximum length of AVDTP security data */
#define BTA_AVK_SECURITY_MAX_LEN     400

/* check number of buffers queued at L2CAP when this amount of buffers are queued to L2CAP */
#define BTA_AVK_QUEUE_DATA_CHK_NUM   5


/*****************************************************************************
**  Data types
*****************************************************************************/
/* Definitions for stream control block handle */
typedef UINT8 tBTA_AVK_HNDL;
#define BTA_AVK_CHNL_TO_MASK(chnl)  ((UINT8)(1 << (chnl)))
#define BTA_AVK_VIDEO_MASK          2
#define BTA_AVK_AUDIO_MASK          1

/* Channel ID to indicate both audio and video channels (used for CLOSE, START, and STOP) */
#define BTA_AVK_CHNL_ALL            0xFF

/* function types for call-out functions */
typedef BOOLEAN (*tBTA_AVK_CO_INIT) (UINT8 *p_codec_type, UINT8 *p_codec_info,
                                   UINT8 *p_num_protect, UINT8 *p_protect_info, UINT8 index);

typedef void (*tBTA_AVK_CO_DISC_RES) (UINT8 num_seps, UINT8 num_snk, BD_ADDR addr);

typedef UINT8 (*tBTA_AVK_CO_GETCFG) (tBTA_AVK_CODEC codec_type,
                                     UINT8 *p_codec_info, UINT8 *p_sep_info_idx, UINT8 seid,
                                     UINT8 *p_num_protect, UINT8 *p_protect_info);
typedef void (*tBTA_AVK_CO_SETCFG) (tBTA_AVK_CODEC codec_type,
                                    UINT8 *p_codec_info, UINT8 seid, BD_ADDR addr,
                                    UINT8 num_protect, UINT8 *p_protect_info);
typedef void (*tBTA_AVK_CO_AUDIO_OPEN) (tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info, UINT16 mtu);
typedef void (*tBTA_AVK_CO_OPENCFG) (tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info, BD_ADDR addr,
                                    UINT8 num_protect, UINT8 *p_protect_info);
typedef void (*tBTA_AVK_CO_VIDEO_OPEN) (tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info, UINT8 avdt_handle, UINT16 mtu);
typedef void (*tBTA_AVK_CO_CLOSE) (void);
typedef void (*tBTA_AVK_CO_START) (tBTA_AVK_CODEC codec_type);
typedef void (*tBTA_AVK_CO_STOP) (void);
#if (BTU_BTC_SNK_INCLUDED == TRUE)
typedef void (*tBTA_AVK_CO_BTC_START) (void);
#endif
typedef void (*tBTA_AVK_CO_DELAY) (UINT8 err);
typedef void (*tBTA_AVK_CO_AUDIO_DATAPATH) (tBTA_AVK_CODEC codec_type, BT_HDR *p_pkt,
                                        UINT32 timestamp, UINT16 seq_num, UINT8 m_pt);
typedef void (*tBTA_AVK_CO_VIDEO_DATAPATH) (tBTA_AVK_CODEC codec_type, UINT8 *p_media, UINT32 media_len,
                                        UINT32 timestamp, UINT16 seq_num, UINT8 m_pt);

/* union of all event datatypes */
typedef union
{
    tBTA_AVK_CO_AUDIO_OPEN audio;
    tBTA_AVK_CO_VIDEO_OPEN video;
} tBTA_AVK_CO_OPEN;

typedef union
{
    tBTA_AVK_CO_AUDIO_DATAPATH audio;
    tBTA_AVK_CO_VIDEO_DATAPATH video;
} tBTA_AVK_CO_DATAPATH;


/* the call-out functions for one stream */
typedef struct
{
    tBTA_AVK_CO_INIT     init;
    tBTA_AVK_CO_DISC_RES disc_res;
    tBTA_AVK_CO_GETCFG   getcfg;
    tBTA_AVK_CO_SETCFG   setcfg;
    tBTA_AVK_CO_OPENCFG  opencfg;
    void                 *open;
    tBTA_AVK_CO_CLOSE    close;
    tBTA_AVK_CO_START    start;
    tBTA_AVK_CO_STOP     stop;
    void                 *data;
#if (BTU_BTC_SNK_INCLUDED == TRUE)
    tBTA_AVK_CO_BTC_START btc_start;
#endif
    tBTA_AVK_CO_DELAY    delay;

} tBTA_AVK_CO_FUNCTS;

/* data type for BTA_AVK_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_AVK_CBACK       *p_cback;
    tBTA_AVK_FEAT        features;
    tBTA_SEC            sec_mask;
} tBTA_AVK_API_ENABLE;

/* data type for BTA_AVK_API_REG_EVT */
typedef struct
{
    BT_HDR              hdr;
    char                p_service_name[BTA_SERVICE_NAME_LEN+1];
    UINT8               app_id;
} tBTA_AVK_API_REG;

/* data type for BTA_AVK_API_OPEN_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    tBTA_SEC            sec_mask;
} tBTA_AVK_API_OPEN;

/* data type for BTA_AVK_API_STOP_EVT */
typedef struct
{
    BT_HDR              hdr;
    BOOLEAN             suspend;
    BOOLEAN             flush;
} tBTA_AVK_API_STOP;

/* data type for BTA_AVK_API_UPDATE_SEPS_EVT */
typedef struct
{
    BT_HDR              hdr;
    BOOLEAN             available;
} tBTA_AVK_API_UPDATE_SEPS;

/* data type for BTA_AVK_API_DELAY_RPT_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT16              delay;
} tBTA_AVK_API_DELAY_RPT;

/* data type for BTA_AVK_API_PROTECT_REQ_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT8               *p_data;
    UINT16              len;
} tBTA_AVK_API_PROTECT_REQ;

/* data type for BTA_AVK_API_PROTECT_RSP_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT8               *p_data;
    UINT16              len;
    UINT8               error_code;
} tBTA_AVK_API_PROTECT_RSP;

/* data type for BTA_AVK_API_REMOTE_CMD_EVT */
typedef struct
{
    BT_HDR              hdr;
    tAVRC_MSG_PASS      msg;
    UINT8               label;
} tBTA_AVK_API_REMOTE_CMD;

/* data type for BTA_AVK_API_VENDOR_CMD_EVT and RSP */
typedef struct
{
    BT_HDR              hdr;
    tAVRC_MSG_VENDOR    msg;
    UINT8               label;
} tBTA_AVK_API_VENDOR;

/* data type for BTA_AV_API_META_MSG_EVT */
typedef struct
{
    BT_HDR              hdr;
    BOOLEAN             is_rsp;
    UINT8               label;
    tBTA_AVK_CODE       cmd_rsp_code;
    BT_HDR              *p_pkt;
} tBTA_AVK_API_META_MSG;

/* data type for BTA_AVK_API_AVRCP_CLOSE_REQ_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT8               handle;
} tBTA_AVK_API_AVRCP_CLOSE;

/* data type for BTA_AVK_API_RECONFIG_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT8               codec_info[AVDT_CODEC_SIZE];    /* codec configuration */
    UINT8               *p_protect_info;
    UINT8               num_protect;
    BOOLEAN             suspend;
    UINT8               sep_info_idx;
} tBTA_AVK_API_RCFG;

/* data type for BTA_AVK_CI_SETCONFIG_OK_EVT and BTA_AVK_CI_SETCONFIG_FAIL_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_AVK_HNDL        hndl;
    UINT8               err_code;
    UINT8               category;
} tBTA_AVK_CI_SETCONFIG;

/* type for SCMS Content Protection Call-In (BTA_AVK_CI_CP_SCMS_EVT) */
typedef struct
{
    BT_HDR              hdr;
    BOOLEAN             enable;
    UINT8               scms_hdr;
} tBTA_AVK_CI_CP_SCMS;

#if (BTU_BTC_SNK_INCLUDED == TRUE)
/* type for BTA_AVK_CI_AUDIO_BTC_START_EVT) */
typedef struct
{
    BT_HDR              hdr;
} tBTA_AVK_CI_AUDIO_BTC_START;
#endif

/* data type for all stream events from AVDTP */
typedef struct {
    BT_HDR              hdr;
    tAVDT_CFG           cfg;        /* configuration/capabilities parameters */
    tAVDT_CTRL          msg;        /* AVDTP callback message parameters */
    BD_ADDR             bd_addr;    /* bd address */
    UINT8               handle;
} tBTA_AVK_STR_MSG;

/* data type for BTA_AVK_AVRC_MSG_EVT */
typedef struct
{
    BT_HDR              hdr;
    tAVRC_MSG           msg;
    UINT8               handle;
    UINT8               label;
    UINT8               opcode;
} tBTA_AVK_RC_MSG;

/* data type for BTA_AVK_CONN_CHG_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             peer_addr;
    UINT8               handle;
} tBTA_AVK_RC_CONN_CHG;

/* data type for BTA_AVK_CONN_CHG_EVT */
typedef struct
{
    BT_HDR              hdr;
    BOOLEAN             is_up;
} tBTA_AVK_CONN_CHG;

/* data type for BTA_AVK_SDP_DISC_OK_EVT */
typedef struct
{
    BT_HDR              hdr;
    UINT16              avdt_version;   /* AVDTP protocol version */
} tBTA_AVK_SDP_RES;

/* type for SEP control block */
typedef struct
{
    UINT8               av_handle;      /* AVDTP handle */
    tBTA_AVK_CODEC       codec_type;     /* codec type */
    UINT16              codec_id;       /* codec id for codec_type=BTA_AVK_CODEC_VEND */
} tBTA_AVK_SEP;

/* initiator/acceptor role for adaption */
#define BTA_AVK_ROLE_OPEN_ACP       0x00        /* Connection Open Acceptor */
#define BTA_AVK_ROLE_OPEN_INT       0x01        /* Connection Open Initiator */

/* initiator/acceptor signaling roles */
#define BTA_AVK_ROLE_START_ACP       0x00
#define BTA_AVK_ROLE_START_INT       0x10    /* do not change this value */

/* union of all event datatypes */
typedef union
{
    BT_HDR                  hdr;
    tBTA_AVK_API_ENABLE      api_enable;
    tBTA_AVK_API_REG         api_reg;
    tBTA_AVK_API_OPEN        api_open;
    tBTA_AVK_API_STOP        api_stop;
    tBTA_AVK_API_UPDATE_SEPS api_update_seps;
    tBTA_AVK_API_DELAY_RPT   api_delay_rpt;
    tBTA_AVK_API_PROTECT_REQ api_protect_req;
    tBTA_AVK_API_PROTECT_RSP api_protect_rsp;
    tBTA_AVK_API_REMOTE_CMD  api_remote_cmd;
    tBTA_AVK_API_VENDOR      api_vendor;
    tBTA_AVK_API_META_MSG    api_meta_msg;
    tBTA_AVK_API_AVRCP_CLOSE api_avrcp_close;
    tBTA_AVK_API_RCFG        api_reconfig;
    tBTA_AVK_CI_SETCONFIG    ci_setconfig;
    tBTA_AVK_CI_CP_SCMS      ci_cp_scms;
    tBTA_AVK_STR_MSG         str_msg;
    tBTA_AVK_RC_MSG          rc_msg;
    tBTA_AVK_RC_CONN_CHG     rc_conn_chg;
    tBTA_AVK_CONN_CHG        conn_chg;
    tBTA_AVK_SDP_RES         sdp_res;
} tBTA_AVK_DATA;

/* type for AV stream control block */
typedef struct
{
    tAVDT_CFG           cfg;            /* local SEP configuration */
    UINT16              l2c_cid;        /* L2CAP channel ID */
    UINT16              stream_mtu;     /* MTU of stream */
    UINT16              avdt_version;   /* the avdt version of peer device */
    tBTA_SEC            sec_mask;       /* security mask */
    tBTA_AVK_CODEC      codec_type;     /* codec type */
    UINT16              codec_id;       /* codec id for codec_type=BTA_AVK_CODEC_VEND */
    UINT8               media_type;     /* Media type */
    tBTA_AVK_STATUS     open_status;    /* open failure status */
    tBTA_AVK_CHNL       chnl;           /* the channel: audio/video */
    UINT16              cur_psc_mask;   /* Protocol service capabilities mask for
                                         * current connection */
    UINT8               avdt_handle;    /* AVDTP handle */
    UINT8               hdi;            /* the index to SCB[] */
    UINT8               num_seps;       /* number of seps returned by stream discovery */
    UINT8               sep_info_idx;   /* current index into sep_info */
    UINT8               sep_idx;        /* current index into local seps[] */
    UINT8               state;          /* state machine state */
    UINT8               avdt_label;     /* AVDTP label */
    UINT8               app_id;         /* application id */
    UINT8               num_recfg;      /* number of reconfigure sent */
    UINT8               role;
    BOOLEAN             started;        /* TRUE if stream started */
    BOOLEAN             co_started;     /* TRUE if stream started
                                         * from call-out perspective */
    BOOLEAN             co_delay;       /* TRUE if co_delay is called */
    BOOLEAN             suspend_sup;    /* TRUE if Suspend stream is supported,
                                         * else FALSE if suspend command fails */
    BOOLEAN             recfg_ind;      /* TRUE if reconfigure attempt
                                         * happens in open state */
    UINT8               inuse_sep;      /* index of the sep for the
                                         * codec in use in src */
    tBTA_AVK_CODEC      inuse_codec;    /* Codec type which is already
                                         * used in audio SRC */
    BOOLEAN             is_api_close;   /* Whether the close is called by local
                                         * device through API or not */

    BOOLEAN             recfg_sup;      /* TRUE if the first attempt to reconfigure the
                                         * stream was successfull, else False if command
                                         * fails
                                         */
    tAVDT_SEP_INFO      sep_info[BTA_AVK_NUM_SEPS];      /* stream discovery results */
    tBTA_AVK_SEP        seps[BTA_AVK_MAX_SEPS];
    const tBTA_AVK_ACT   *p_act_tbl;     /* the action table for stream state machine */
    const tBTA_AVK_CO_FUNCTS *p_cos;     /* the associated callout functions */
    tSDP_DISCOVERY_DB   *p_disc_db;     /* pointer to discovery database */
    tAVDT_CTRL_CBACK    *p_dt_cback;    /* the callback function to receive events from
                                         * AVDT control channel */
    tAVDT_CFG           *p_cap;         /* buffer used for get capabilities */
    BD_ADDR             peer_addr;      /* peer BD address */
    UINT8               rcfg_idx;       /* reconfig requested index into sep_info */
    BOOLEAN             deregistring;   /* TRUE if deregistering */
} tBTA_AVK_SCB;

/* part of tBTA_AVK_SCB */
typedef struct
{
    tAVDT_CFG           cfg;            /* local SEP configuration */
    UINT16              l2c_cid;        /* L2CAP channel ID */
    UINT16              stream_mtu;     /* MTU of stream */
    UINT16              avdt_version;   /* the avdt version of peer device */
    tBTA_SEC            sec_mask;       /* security mask */
    tBTA_AVK_CODEC      codec_type;     /* codec type */
    UINT16              codec_id;       /* codec id for codec_type=BTA_AVK_CODEC_VEND */
    UINT8               media_type;     /* Media type */
    tBTA_AVK_STATUS     open_status;    /* open failure status */
    tBTA_AVK_CHNL       chnl;           /* the channel: audio/video */
    UINT16              cur_psc_mask;   /* Protocol service capabilities mask for
                                         * current connection */
    UINT8               avdt_handle;    /* AVDTP handle */
    UINT8               hdi;            /* the index to SCB[] */
    UINT8               num_seps;       /* number of seps returned by stream discovery */
    UINT8               sep_info_idx;   /* current index into sep_info */
    UINT8               sep_idx;        /* current index into local seps[] */
    UINT8               state;          /* state machine state */
    UINT8               avdt_label;     /* AVDTP label */
    UINT8               app_id;         /* application id */
    UINT8               num_recfg;      /* number of reconfigure sent */
    UINT8               role;
    BOOLEAN             started;        /* TRUE if stream started */
    BOOLEAN             co_started;     /* TRUE if stream started
                                         * from call-out perspective */
    BOOLEAN             co_delay;       /* TRUE if co_delay is called */
    BOOLEAN             suspend_sup;    /* TRUE if Suspend stream is supported,
                                         * else FALSE if suspend command fails */
    BOOLEAN             recfg_ind;      /* TRUE if reconfigure attempt
                                         * happens in open state */
    UINT8               inuse_sep;      /* index of the sep for the
                                         * codec in use in src */
    tBTA_AVK_CODEC      inuse_codec;    /* Codec type which is already
                                         * used in audio SRC */
} tBTA_AVK_LPST_SCB;

#define BTA_AVK_RC_ROLE_MASK     0x10
#define BTA_AVK_RC_ROLE_INT      0x00
#define BTA_AVK_RC_ROLE_ACP      0x10

#define BTA_AVK_RC_CONN_MASK     0x20

/* type for AV RCP control block */
/* index to this control block is the rc handle */
typedef struct
{
    UINT8   status;
    UINT8   handle;
} tBTA_AVK_RCB;

#if (BTU_BTC_SNK_INCLUDED == TRUE)
#define BTA_AVK_JITTER_NONE     0x00
#define BTA_AVK_JITTER_WAIT     0x01    /* Waiting for jitter done indication */
#define BTA_AVK_JITTER_SEND     0x02    /* Trying to send while waiting for done ind */
#define BTA_AVK_WAIT_SWITCH     0x04    /* Waiting for stack switch */
#endif

/* Definitions for tBTA_AVK_CB flags */
#define BTA_AVK_FLAG_DISABLING              0x01    /* BTA_AvkDisable called */
#define BTA_AVK_FLAG_DISC_CB_IN_USE         0x02    /* Discovery database p_disc_db is in use */
#define BTA_AVK_FLAG_RC_API_OPEN_PENDING    0x04    /* AVRCP API open is pending */
#define BTA_AVK_FLAG_RC_OPENED              0x08    /* AVRCP connection opened */

/* type for AV control block */
typedef struct
{
    tBTA_AVK_FEAT       peer_features;  /* peer features mask */
    tBTA_AVK_RC_INFO    peer_ct;        /* peer CT role info */
    tBTA_AVK_RC_INFO    peer_tg;        /* peer TG role info */
    BD_ADDR             peer_addr;      /* peer BD address */
    tBTA_SEC            sec_mask;       /* security mask */
    UINT8               flags;          /* See BTA_AVK_FLAG_* definitions */
    UINT8               state;          /* state machine state */
    UINT8               conn_rc;        /* handle mask of connected RCP channels */
    UINT8               conn_audio;     /* handle mask of connected audio channels */
    UINT8               reg_audio;      /* handle mask of registered audio channels */

    UINT8               audio_streams;  /* handle mask of streaming audio channels */
    UINT8               rc_handle;      /* Connected RC handle */
    UINT8               rc_acp_handle;  /* Accepting RC handle */
    UINT8               rc_int_handle;  /* Iniating RC handle */
    /* do not need to be sync'd */
    tBTA_AVK_SCB         *p_scb[BTA_AVK_NUM_STRS];    /* stream control block */
    tSDP_DISCOVERY_DB   *p_disc_db;     /* pointer to discovery database */
    tBTA_AVK_CBACK       *p_cback;       /* application callback function */
    TIMER_LIST_ENT      timer;          /* delay timer for AVRC and CLOSE */
    UINT32              sdp_a2d_handle; /* SDP record handle for audio src */
    tBTA_AVK_FEAT       features;       /* features mask */
#if (BRCM_LPST_INCLUDED == TRUE)
    UINT16              cb_required;    /* the cblk_requred when bridge is up */
#endif
    tBTA_AVK_HNDL       handle;         /* the SCB handle for SDP activity as INT */
    /* the following are for VDP_INCLUDED == TRUE */
    UINT32              sdp_vdp_handle; /* SDP record handle for video src */
    UINT8               conn_video;     /* handle mask of connected video channels */
    UINT8               reg_video;      /* handle mask of registered video channels */
    UINT8               video_streams;  /* handle mask of streaming video channels */

#if (BTU_BTC_SNK_INCLUDED == TRUE)
    UINT8               jitter_st;      /* TRUE if sent A2DP_SUSPEND or STOP,
                                         * reset if received JITTER_DONE_IND */
    UINT8               sync_status;    /* BTM lite stack status */
#endif
} tBTA_AVK_CB;

/* part of tBTA_AVK_CB */
typedef struct
{
    tBTA_AVK_FEAT       peer_features;  /* peer features mask */
    tBTA_AVK_RC_INFO    peer_ct;        /* peer CT role info */
    tBTA_AVK_RC_INFO    peer_tg;        /* peer TG role info */
    BD_ADDR             peer_addr;      /* peer BD address */
    tBTA_SEC            sec_mask;       /* security mask */
    UINT8               flags;          /* See BTA_AVK_FLAG_* definitions */
    UINT8               state;          /* state machine state */
    UINT8               conn_rc;        /* handle mask of connected RCP channels */
    UINT8               conn_audio;     /* handle mask of connected audio channels */
    UINT8               reg_audio;      /* handle mask of registered audio channels */

    UINT8               audio_streams;  /* handle mask of streaming audio channels */
    UINT8               rc_handle;      /* Connected RC handle */
    UINT8               rc_acp_handle;  /* Accepting RC handle */
    UINT8               rc_int_handle;  /* Iniating RC handle */
} tBTA_AVK_LPST_MCB;

/* type for stream state machine action functions */
typedef void (*tBTA_AVK_SACT)(tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);

/*****************************************************************************
**  Global data
*****************************************************************************/

/* control block declaration */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_AVK_CB bta_avk_cb;
#else
extern tBTA_AVK_CB *bta_avk_cb_ptr;
#define bta_avk_cb (*bta_avk_cb_ptr)
#endif

/* config struct */
extern tBTA_AVK_CFG *p_bta_avk_cfg;

/* rc id config struct */
extern UINT16 *p_bta_avk_rc_id;

extern const tBTA_AVK_SACT bta_avk_a2dp_action[];
extern const tBTA_AVK_CO_FUNCTS bta_avk_a2d_cos;
extern const tBTA_AVK_SACT bta_avk_vdp_action[];

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

/* main functions */
extern void bta_avk_api_deregister(tBTA_AVK_DATA *p_data);
extern void bta_avk_dup_audio_buf(tBTA_AVK_SCB *p_scb, BT_HDR *p_buf, UINT32 timestamp);
extern void bta_avk_sm_execute(tBTA_AVK_CB *p_cb, UINT16 event, tBTA_AVK_DATA *p_data);
extern void bta_avk_ssm_execute(tBTA_AVK_SCB *p_scb, UINT16 event, tBTA_AVK_DATA *p_data);
extern BOOLEAN bta_avk_hdl_event(BT_HDR *p_msg);
extern void bta_avk_reg_a2dp (tAVDT_CS *p_cs, char *p_service_name, void *p_data);
#if (defined(BTA_AVK_DEBUG) && BTA_AVK_DEBUG == TRUE)
extern char *bta_avk_evt_code(UINT16 evt_code);
#endif
extern void bta_avk_util_open_rc (UINT8 *p_rc_handle, UINT8 conn_role, BD_ADDR_PTR peer_addr);

/* sm action functions */
extern void bta_avk_disable (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_dereg_comp (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_conn_chg(tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_listen (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_disc_done (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_api_close (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_close (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_opened (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_closed (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_api_disc (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_disc (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_remote_cmd (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_vendor_cmd (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_vendor_rsp (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_meta_send (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_msg (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_open_as_acp (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_timer (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_free_meta (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_free_browse (tBTA_AVK_CB *p_cb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rc_peer_feat_evt(void);

/* ssm action functions */
extern void bta_avk_do_disc_a2d (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_cleanup (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_free_sdb (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_config_ind (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_config_ind_vdp (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_disconnect_req (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_security_req (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_security_rsp (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_setconfig_rsp (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_cp_scms (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_reconfig_rsp (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_snd_rc_listen(tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_str_opened (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_str_open_fail (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_security_ind (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_security_cfm (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_do_close (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_connect_req (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_sdp_failed (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_setconfig_rej (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_discover_req (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_sig_hdl_ap_close (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_do_start (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_str_stopped (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_reconfig (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_data_path (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_start_ok (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_start_failed (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_suspend_cfm (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_security_rej (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_open_rc (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_hdl_str_close (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_save_caps (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_rej_conn (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_delay_rpt (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_delay_cfm (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern BOOLEAN bta_avk_chk_start(tBTA_AVK_SCB *p_scb);
extern void bta_avk_stream_chg (tBTA_AVK_SCB * p_scb, BOOLEAN started);
extern void bta_avk_sig_opened (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_sig_closed (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_sig_open_fail (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
#if (BTU_BTC_SNK_INCLUDED == TRUE)
extern void bta_avk_btc_start_ok (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_ci_btc_start (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
#endif


/* ssm action functions - vdp specific */
extern void bta_avk_do_disc_vdp (tBTA_AVK_SCB *p_scb, tBTA_AVK_DATA *p_data);
extern void bta_avk_reg_vdp (tAVDT_CS *p_cs, char *p_service_name, void *p_data);

#if (BTU_BTC_SNK_INCLUDED == TRUE)
extern void bta_avk_sync_req_hdlr(UINT8 opcode, UINT8 param);
extern void bta_avk_sync_proc (tBTA_AVK_DATA *p_data);
extern void bta_avk_jitter_expired (tBTA_AVK_DATA *p_data);
extern void bta_avk_ipc_evt_hdlr(tBTM_STATUS status, BT_HDR *p_buf);
extern void bta_avk_ipc_send(tBTA_DUAL_STACK_MSG *dual_stack_msg);
#endif

/* data path and avdt control callback functions */
void bta_avk_audio_ctrl_cback(UINT8 handle, BD_ADDR bd_addr,
                              UINT8 event, tAVDT_CTRL *p_data);
void bta_avk_data_cback(UINT8 handle, BT_HDR *p_pkt,
                        UINT32 time_stamp, UINT8 m_pt);
void bta_avk_video_ctrl_cback(UINT8 handle, BD_ADDR bd_addr,
                              UINT8 event, tAVDT_CTRL *p_data);
void bta_avk_media_cback(UINT8 handle, UINT8 *p_media, UINT32 media_len,
                         UINT32 time_stamp, UINT16 seq_num, UINT8 m_pt, UINT8 marker);

/* Internal functions provided by bta_avk_lpst.c (for LPST support)
**********************************************
*/

#if (BRCM_LPST_INCLUDED == TRUE)
#define BTA_AVK_LPST_CB_MCB            0x01    // for tBTA_AVK_CB
#define BTA_AVK_LPST_CB_SCB            0x02    // for tBTA_AVK_SCB
#define BTA_AVK_LPST_CB_RC_INFO        0x04    // for AVRCP peer info

extern void bta_avk_lpst_send_sync (BOOLEAN is_bridge_up);
extern void bta_avk_lpst_send_rc_open(void);
extern void bta_avk_lpst_send_control_blocks (tBTA_AVK_SCB *p_scb, BOOLEAN for_rc_open);
extern void bta_avk_lpst_send_rc_info (void);
extern void bta_avk_lpst_process_control_blocks (UINT8 *p, UINT16 cb_len);
extern BOOLEAN bta_avk_lpst_validate_switch_state(void);
#endif

#endif /* BTA_AVK_INT_H */
