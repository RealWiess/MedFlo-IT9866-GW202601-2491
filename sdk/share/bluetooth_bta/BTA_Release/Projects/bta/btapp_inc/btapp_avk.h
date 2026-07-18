/*****************************************************************************
**
**  Name:             btapp_avk.h
**
**  Description:     This file contains  internal interface
**                   definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_AVK_H
#define BTAPP_AVK_H

#include "gki.h"
#include "bta_avk_api.h"

#define BTAPP_AK_ST_NONE            0x0000
#define BTAPP_AK_ST_CONNECTABLE     0x0001
#define BTAPP_AK_ST_SIG_CONNECT     0x0002
#define BTAPP_AK_ST_STR_CONNECT     0x0004
#define BTAPP_AK_ST_RC_CONNECT      0x0008
#define BTAPP_AK_ST_START           0x0010
#define BTAPP_AK_ST_SUSPEND         0x0020
#define BTAPP_AK_ST_STOP            0x0040
#define BTAPP_AK_ST_MUTE            0x0080
#define BTAPP_AK_ST_PAUSETOGGLE     0x0100
typedef UINT16 tBTAPP_AK_ST;


#define BTAPP_MIN_ABS_VOLUME        0x00
#define BTAPP_MAX_ABS_VOLUME        0x7F

typedef UINT16 tBTAPP_AK_STATUS;

#define BTAPP_AVK_AFMT_SBC   0
#define BTAPP_AVK_AFMT_MP3   1

typedef UINT16          tBTAPP_AVK_EVT_MASK;
typedef struct
{
    tBTAPP_AVK_EVT_MASK  evt_mask;
    UINT8               label[AVRC_NUM_NOTIF_EVENTS];
} tBTAPP_AVK_REG_EVT;

/* AK */
typedef struct
{
    BOOLEAN             enabled;
    BOOLEAN             str_started;
    UINT8               label;
    UINT8               play_st;
    BOOLEAN             seq_started;
    UINT16              avk_seq_num;
    UINT32              drop_count;
#if (BTU_BTC_SNK_INCLUDED == TRUE)
    tBTA_DM_ROUTE_PATH  btc_route;          /* BTC audio route */
    tAUDIO_CODEC_TYPE   btc_codec_type;     /* codec type for BTC lite stack */
    tCODEC_INFO         btc_codec_info;     /* codec info for BTC lite stack */
    UINT16              rt_handle;          /* Audio Routing handle */
    tAUDIO_CODEC_TYPE   sup_codec_type;     /* Codec type supported by controller */
    UINT16              audio_path_table[MAX_AUDIO_ROUTE_SRC][MAX_AUDIO_ROUTE_MIX]; /* audio path supported by controller */
    BOOLEAN             audio_path_supported;   /* Whether the request codec and audio path is supported by the controller */
    BOOLEAN             reconfig_pending;   /* Indicate whether reconfig is pending */
    UINT8               reconfig_codec_info[AVDT_CODEC_SIZE];   /* Save reconfig codec info */
    tAUDIO_CODEC_TYPE   reconfig_codec_type;    /* Save reconfig codec type */
    BOOLEAN             close_pending;      /* Indicate whether UIPC close is pending */
    BOOLEAN             btc_start_pending;  /* Receive A2DP start when stack switching is going on */
#endif
    /* Current codec */
    tBTA_AVK_CODEC      avk_codec_audio;
    tBTA_AVK_CODEC      avk_codec_video;

    UINT8               cur_volume;
    tBTAPP_AK_STATUS    status;
    UINT16              dump_index;     /* Dump file starts from avk0000 */
    BOOLEAN             suspend;
#ifdef BTA_AVK_INCLUDED
    UINT8               rc_handle;
    BT_HDR              *p_metadata_rsp_buf;    /* Buffer for holding fragmented metadata responses */
#endif
    BOOLEAN             is_sink_seps_disabled;
    tBTAPP_AVK_REG_EVT   registered_events;
    UINT16              peer_ct_features;
    UINT16              peer_tg_features;
    TIMER_LIST_ENT      rc_timer_tle;
    UINT8               rc_try;
#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
    BOOLEAN             avk_cp_engaged;
#endif
} tBTAPP_AK_CB;

extern tBTAPP_AK_CB btapp_ak_cb;

#define BTAPP_AK_SETSTATUS(m)           (btapp_ak_cb.status |= (m))
#define BTAPP_AK_RESETSTATUS(m)         (btapp_ak_cb.status &= (~(m)))
#define BTAPP_AK_GETSTATUS(m)           (btapp_ak_cb.status & m)

extern void btapp_avk_init(void);
extern void btapp_avk_disable(void);
extern void btapp_avk_start(void);
extern void btapp_avk_stop (BOOLEAN suspend);
extern void btapp_avk_pause(void);
extern void btapp_avk_close(void);
extern void btapp_avk_dump_cb(void);
extern void btapp_avk_volume_up(void);
extern void btapp_avk_volume_down(void);
extern void btapp_avk_next_track(void);
extern void btapp_avk_previous_track(void);
extern void btapp_avk_vendor(void);
extern void btapp_avk_protect_req(tBTA_AVK_CHNL chnl);
extern void btapp_avk_device_connect(BD_ADDR bda, tBTA_AVK_CHNL chnl);
extern void btapp_avk_update_seps(BOOLEAN available);
extern void btapp_avk_delay_report(tBTA_AVK_CHNL chnl, UINT16 delay);
extern void btapp_avk_send_rc_cmd (tBTA_AVK_RC rc_op_id);
extern UINT8 btapp_avk_get_next_label (void);

#if( defined BTA_AVK_INCLUDED ) && (BTA_AVK_INCLUDED == TRUE)
extern void btapp_platform_avk_remote_cmd(tBTA_AVK_RC rc_id, tBTA_AVK_STATE key_state);
#endif

extern void btapp_avk_set_the_vol (UINT8 volume);
extern void btapp_platform_ak_init(void);
extern void btapp_avk_send_evt_to_mmi(tBTA_AVK_EVT event, tBTA_AVK *p_data);
extern char *btapp_avk_get_fname (UINT8 afmt);
#if (BTU_BTC_SNK_INCLUDED == TRUE)
extern void btapp_avk_switch_bb2btc (void);
extern void btapp_avk_switch_btc2bb (void);
extern void btapp_avk_update_btc_codec_info(tAUDIO_CODEC_TYPE c_type, UINT8 *p_info);
#endif

void btapp_avk_rc_get_info(UINT8 opcode);
void btapp_avk_rc_get_capabilities(tAVRC_CAP capability_id);
void btapp_avk_rc_set_abs_volume(UINT8 volume);
void btapp_avk_rc_get_element_attibutes(UINT64 id,
                                        UINT8 num_attributes,
                                        UINT32 *p_attributes);
void btapp_avk_rc_list_player_app_attr(void);
void btapp_avk_rc_get_cur_player_app_value(UINT8 num_player_app_attr_ids,
                                           UINT8 *p_player_app_atr_id);
void btapp_avk_rc_get_play_status(void);
void btapp_avk_rc_reg_notif(tAVRC_EVT event_id, UINT32 playback_interval);
void btapp_avk_rc_request_cont_rsp(tAVRC_PDU continue_pdu_id);
void btapp_avk_rc_abort_cont_rsp(tAVRC_PDU cont_abort_pdu_id);
void btapp_avk_rc_get_folder_items(tAVRC_SCOPE scope,
                                   UINT32 start_item,
                                   UINT32 end_item,
                                   UINT8 attr_count,
                                   UINT32 *p_attr_list);
void btapp_avk_rc_play(void);
void btapp_avk_rc_pause(void);
void btapp_avk_rc_stop(void);
void btapp_avk_rc_volume_up(void);
void btapp_avk_rc_volume_down(void);
void btapp_avk_rc_open(void);
void btapp_avk_rc_acp_open(void);
void btapp_avk_rc_close(void);
#if (AVRC_1_6_INCLUDED == TRUE)
void btapp_avk_rc_get_total_num_of_items(UINT8 scope);
#endif  /* #if (AVRC_1_6_INCLUDED == TRUE) */
#endif  /* BTAPP_AVK_H */
