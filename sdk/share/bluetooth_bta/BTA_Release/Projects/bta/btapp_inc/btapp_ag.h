/*****************************************************************************
**
**  Name:             btapp_ag.h
**
**  Description:     This file contains btui internal interface
**                   definition
**
**  Copyright (c) 2003-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "gki.h"

#ifndef BTAPP_AG_H
#define BTAPP_AG_H
#include "bta_ag_api.h"
#include "btapp_int.h"
/* phone call-related events */
enum
{
    BTUI_AG_HF_DIAL_EVT,
    BTUI_AG_HF_HANGUP_EVT,
    BTUI_AG_HF_ANS_EVT,
    BTUI_AG_HF_ANS_HELD_EVT,
    BTUI_AG_HF_CANCEL_EVT,
    BTUI_AG_IN_CALL_EVT,
    BTUI_AG_IN_CALL_CONN_PH_EVT,
    BTUI_AG_IN_CALL_CONN_EVT,
    BTUI_AG_OUT_CALL_ORIG_EVT,
    BTUI_AG_OUT_CALL_ORIG_PH_EVT,
    BTUI_AG_OUT_CALL_CONN_EVT,
    BTUI_AG_OUT_CALL_CONN_PH_EVT,
    BTUI_AG_END_CALL_EVT,
    BTUI_AG_CALL_WAIT_EVT,
    BTUI_AG_CALL_CANCEL_EVT,
    BTUI_AG_CALL_BUSY_EVT,
    BTUI_AG_HS_CKPD_EVT
};

#define BTUI_AG_IND_CALL    0
#define BTUI_AG_IND_SETUP   2
#define BTUI_AG_IND_SVC     4
#define BTUI_AG_IND_SIG     6
#define BTUI_AG_IND_ROAM    8
#define BTUI_AG_IND_BATT    10
#define BTUI_AG_IND_HELD    12
#define BTUI_AG_IND_BEAR    14
#define BTUI_AG_IND_LEN     16

#define BTUI_AG_ID_1        0
#define BTUI_AG_ID_2        1

#define BTUI_AG_NUM_APP     2

/* SCO owner */
#define BTUI_SCO_NONE       0
#define BTUI_SCO_AG         1
#define BTUI_SCO_FM         2

/* phone call state */
enum
{
    BTUI_AG_CALL_NONE,
    BTUI_AG_CALL_INC,
    BTUI_AG_CALL_OUT,
    BTUI_AG_CALL_ACTIVE,
    BTUI_AG_CALL_HELD
};

#if (BTA_HFP_VERSION >= HFP_VERSION_1_7 && BTA_HFP_HF_IND_SUPPORTED == TRUE)
typedef struct
{
    UINT8   ind_id;
    UINT16  ind_min_val;
    UINT16  ind_max_val;


} tBTUI_AG_IND_TABLE;
#endif

typedef struct
{
    tBTA_SERVICE_ID service;
    UINT16          handle;
    BOOLEAN         is_open;
    BOOLEAN         is_audio_open;
    UINT8           spk_vol;
    UINT8           mic_vol;
    BOOLEAN         bvra_active;
    tBTA_AG_PEER_FEAT   peer_feat;
#if (BTM_WBS_INCLUDED == TRUE )
    tBTA_AG_PEER_CODEC  peer_codec;
    tBTA_AG_PEER_CODEC  sco_codec;
#endif
    tBTAPP_REM_DEVICE    *p_dev;     /* BT device */
} tBTUI_AG_APP;

typedef struct
{
    BOOLEAN         enabled;
    tBTUI_AG_APP    app[BTUI_AG_NUM_APP];
    char            ind[BTUI_AG_IND_LEN];
    UINT16          call_handle;
    UINT8           call_state;
    BOOLEAN         inband_ring;
    BOOLEAN         voip_dial;      /* Use VOIP dial type if TRUE */
    UINT8           btrh_state;
    BOOLEAN         btrh_active;
    BOOLEAN         cnum_disabled;
    UINT8           btrh_cmd;
    BOOLEAN         btrh_no_sco;
    UINT8           atd_err_code;
    UINT16          atd_err_timer;
    TIMER_LIST_ENT  atd_err_tle;
#if (defined BTA_FM_INCLUDED && BTA_FM_INCLUDED == TRUE)
/* add for FM */
    BOOLEAN         play_fm;
    BOOLEAN         fm_disconnect;
    BOOLEAN         fm_connect;
    UINT16          cur_handle;
    UINT8           sco_owner; /* 0, none; 1: AG, 2: FM */
#endif
#if (BTA_HFP_VERSION >= HFP_VERSION_1_7 && BTA_HFP_HF_IND_SUPPORTED == TRUE)
    BOOLEAN         is_hf_ind_disabled;
#endif


} tBTUI_AG_CB;

extern tBTUI_AG_CB btui_ag_cb;

extern tBTA_SERVICE_MASK btapp_ag_enable(void);
extern void btapp_ag_disable(void);
extern void btapp_ag_close_conn(UINT8 app_id);
extern void btapp_ag_change_spk_volume(UINT8 spk_vol);
extern void btapp_ag_change_mic_volume(UINT8 mic_vol);
extern void btapp_ag_set_audio_device_authorized (tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_ag_set_device_default_headset (tBTAPP_REM_DEVICE * p_device_rec );
extern BOOLEAN btapp_ag_connect_device(tBTAPP_REM_DEVICE * p_device_rec, BOOLEAN is_fm );
extern void btui_platform_ag_event(tBTA_AG_EVT event, tBTA_AG *p_data);
extern void btui_platform_ag_init(void);
extern void btapp_ag_disable_vr(UINT8 app_id);
extern UINT8 btapp_ag_find_active_vr(void);
extern UINT32 btapp_ag_tran_ag_to_swrap_evt(tBTA_AG_EVT event);
void btapp_find_av_devices(tBTA_DM_SEARCH_CBACK *cf);

#if (defined BTA_FM_INCLUDED && BTA_FM_INCLUDED == TRUE)
extern void btui_view_audio_devices (void);
#endif

#if (BTM_WBS_INCLUDED == TRUE )
extern void btapp_ag_set_wbs_codec(UINT8 app_id, tBTA_AG_PEER_CODEC wb_codec);
#endif

extern UINT16 btapp_ag_open_handle(void);


#define BTUI_MAX_DIAL_DIGITS            128     /* extended for VoIP calls */
#define BTUI_MAX_CALLS                  3

#define BTUI_AG_DIAL_TYPE_DEFAULT       129
#define BTUI_AG_DIAL_TYPE_INTER         145     /* International */
#define BTUI_AG_DIAL_TYPE_VOIP          255

typedef struct
{
    BOOLEAN         is_in_use;
    char            dial_num[BTUI_MAX_DIAL_DIGITS+1]; /* For dial cmd, copy the number HF sends,
                                                       * for other cases use dial_num_to_use */
    UINT8           dial_type;
    UINT8           call_index;
    UINT8           call_state;
    UINT8           call_dir;
    char            dial_num_to_use[BTUI_MAX_DIAL_DIGITS+1];    /* for CS call(default) */
    char            dial_num_to_voip[BTUI_MAX_DIAL_DIGITS+1];   /* for VoIP call */
} tBTUI_CALL_DATA;

#endif
