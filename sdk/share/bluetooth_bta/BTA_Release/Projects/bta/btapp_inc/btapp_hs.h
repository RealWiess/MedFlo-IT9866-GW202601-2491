/*****************************************************************************
**
**  Name:             btapp_hs.h
**
**  Description:     This file contains btapp hs internal interface
**                     definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "gki.h"

#ifndef BTAPP_HS_H
#define BTAPP_HS_H

#include "bta_hs_api.h"

/*  call state */
enum
{
    BTAPP_HS_CALL_NONE,
    BTAPP_HS_CALL_INC,
    BTAPP_HS_CALL_OUT,
    BTAPP_HS_CALL_CONN
};

/* call indicator values */
#define BTAPP_HS_CALL_INACTIVE        0   /* Phone call inactive */
#define BTAPP_HS_CALL_ACTIVE          1   /* Phone call active */

/* callsetup indicator values */
#define BTAPP_HS_CALLSETUP_NONE       0   /* Not currently in call set up */
#define BTAPP_HS_CALLSETUP_INCOMING   1   /* Incoming call process ongoing */
#define BTAPP_HS_CALLSETUP_OUTGOING   2   /* Outgoing call set up is ongoing */
#define BTAPP_HS_CALLSETUP_ALERTING   3   /* Remote party being alerted in an outgoing call */

/* callheld indicator values */
#define BTAPP_HS_CALLHELD_NONE         0   /* No calls held */
#define BTAPP_HS_CALLHELD_ACTIVE_HELD  1   /* AG has both an active and a held call */
#define BTAPP_HS_CALLHELD_HELD         2   /* AG has call on hold, no active call */

#define BTAPP_HS_ST_CONNECTABLE         0x0001
#define BTAPP_HS_ST_CONNECT             0x0002
#define BTAPP_HS_ST_SCALLSETUP          0x0004 /* if value set , outgoingcall and incomming call value are valid*/
#define BTAPP_HS_ST_OUTGOINGCALL        0x0008 /* 0 => incomming call*/
#define BTAPP_HS_ST_SCOOPEN             0x0010 /* 0 => sco close */
#define BTAPP_HS_ST_VOICEDIALACT        0x0020
#define BTAPP_HS_ST_RINGACT             0x0040
#define BTAPP_HS_ST_WAITCALL            0x0080
#define BTAPP_HS_ST_CALLACTIVE          0x0100
#define BTAPP_HS_ST_INBANDRINGACT       0x0200
#define BTAPP_HS_ST_NRECACT             0x0400
#define BTAPP_HS_ST_CLIPACT             0x0800
#define BTAPP_HS_ST_MUTE                0x1000
#define BTAPP_HS_ST_3WAY_HELD           0x2000
#define BTAPP_HS_ST_CONN_SUSPENDED      0x4000
#define BTAPP_HS_ST_IN_CALL             (BTAPP_HS_ST_OUTGOINGCALL | BTAPP_HS_ST_RINGACT | BTAPP_HS_ST_CALLACTIVE)
#define BTAPP_HS_ST_ALL                 0x1fff

#define BTAPP_HS_MAX_CL_IDX             3
#define BTAPP_HS_CL_BUF_SIZE            32

typedef UINT16 tBTAPP_HS_STATUS;

#define BTAPP_HS_SETSTATUS(i,m)       (btapp_hsc_cb.conn_cb[i-1].status |= (m))
#define BTAPP_HS_RESETSTATUS(i,m)     (btapp_hsc_cb.conn_cb[i-1].status &= (~(m)))
#define BTAPP_HS_GETSTATUS(i,m)       (btapp_hsc_cb.conn_cb[i-1].status & m)

/* App ID to identifythe sco_co settings, need to differenciate from AG app_id */
#define BTAPP_HS_APP_ID              BTAPP_DM_SCO_4_HS_APP_ID

#define BTAPP_HS_RING_TIMEOUT        6   /* Consider the call is gone if no ring is received till timeout */

typedef enum
{
   IDX,
   HSDIR,
   STATUS,
   MODE,
   MPTY,
   NUMBER
}tBTAPP_HS_CALL_LIST_OFFSET;

#define     BTAPP_HS_MAX_NUM_CONN    1
/* HSAPP main control block */
typedef struct
{

     BD_ADDR    connected_bd_addr;                       /* peer bdaddr */
     tBTA_SERVICE_ID connected_hs_service_id;            /* service id for HS connection */
     BOOLEAN connection_active;            /* TRUE if HS connection is active */
     BOOLEAN open_audio_asap;             /* If TRUE, open Audio as soon as SLC opens */
     UINT8 call_state;                  /* TRUE if HS connection is active */
     tBTA_HS_PEER_FEAT  peer_feature;

     BOOLEAN last_dialed;

     BOOLEAN  indicator_string_received;
     UINT8 call_ind_id;
     UINT8 call_setup_ind_id;
     UINT8 service_ind_id;
     UINT8 battery_ind_id;
     UINT8 signal_strength_ind_id;
     UINT8 callheld_ind_id;
     UINT8 roam_ind_id;
     UINT8 bearer_ind_id;

     UINT8 curr_call_ind;
     UINT8 curr_call_setup_ind;
     UINT8 curr_service_ind;
     UINT8 curr_battery_ind;
     UINT8 curr_signal_strength_ind;
     UINT8 curr_callheld_ind;
     UINT8 curr_roam_ind;
     UINT8 curr_bearer_ind;

     tBTAPP_HS_STATUS status;
     UINT8 call_list_info[BTAPP_HS_MAX_CL_IDX][BTAPP_HS_CL_BUF_SIZE];
     BOOLEAN clcc_supported;
     BOOLEAN reject_sco;                /* Reject incoming eSCO */
#if (BTM_WBS_INCLUDED == TRUE)
    BOOLEAN  reject_msbc;
    UINT8    neg_codec_type;
#endif
} tBTAPP_HS_CONN_CB;

/* control block (not needed to be stored in NVRAM) */
typedef struct
{
    tBTAPP_HS_CONN_CB    conn_cb[BTAPP_HS_MAX_NUM_CONN];
    BOOLEAN             enabled;
    BOOLEAN             is_muted;
    BOOLEAN             mute_inband_ring;
    UINT32              ring_handle;
    TIMER_LIST_ENT      ring_tle;
    UINT32              cw_handle;
    UINT32              call_op_handle;
    BOOLEAN             is_susp_due_to_hs;
} tBTAPP_HSC_CB;


extern void btapp_hs_init(void);
extern void btapp_hs_disable(void);
extern UINT16 btapp_hs_open(void);
extern BOOLEAN btapp_hs_open_handle(UINT16 handle);
extern BOOLEAN btapp_hs_close(UINT16 handle);
extern void btapp_hs_keypress(void);
extern void btapp_hs_open_slc_and_audio(void);
extern void btapp_hs_voldown(void);
extern void btapp_hs_volup(void);
extern void btapp_hs_active_call_hold(UINT16 handle);
extern void btapp_hs_accept_call_wait(UINT16 handle);
extern void btapp_hs_reject_call_wait(UINT16 handle);
extern void btapp_hs_release_held_calls(UINT16 handle);
extern void btapp_hs_release_active_accept_held(UINT16 handle);
extern void btapp_hs_explicit_call_transfer(UINT16 handle);
extern BOOLEAN btapp_hs_voice_dial(UINT16 handle);
extern BOOLEAN btapp_hs_last_num_dial(UINT16 handle);
extern void btapp_hs_join_calls(UINT16 handle);
extern void btapp_hs_send_command(UINT16 handle, tBTA_HS_CMD cmd, tBTA_HS_CMD_DATA* p_data);
extern void btapp_hs_get_call_list(UINT16 handle, BOOLEAN force);
extern void btapp_hs_set_ag_authorized (tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_hs_sync_mic_vol(UINT16 handle, UINT16 val);
extern void btapp_hs_sync_spk_vol(UINT16 handle, UINT16 val);

extern void btapp_hs_audio_transfer (void);
extern BOOLEAN btapp_hs_end_call (void);
extern BOOLEAN btapp_hs_reject_call (void);
extern BOOLEAN btapp_hs_answer_call (void);

extern void btapp_hs_save_device(BD_ADDR_PTR p_addr, tBTAPP_REM_DEVICE * p_device_rec,
                          tBTA_SERVICE_MASK service_mask);
extern UINT8 btapp_hs_get_num_active_conn(UINT8 *conn_mask);
extern UINT16 btapp_hs_get_last_dialed(void);
extern UINT8 btapp_hs_get_handle_mask(UINT16 status);
extern UINT16 btapp_hs_get_active_handle(UINT16 status_mask);

extern void btapp_bta_hs_evt(tBTA_HS_EVT event, tBTA_HS *p_data);
extern UINT32 btapp_hs_tran_hs_to_swrap_evt(tBTA_HS_EVT event);

/* central control block for HS/HF profile */
extern tBTAPP_HSC_CB btapp_hsc_cb;

#endif
