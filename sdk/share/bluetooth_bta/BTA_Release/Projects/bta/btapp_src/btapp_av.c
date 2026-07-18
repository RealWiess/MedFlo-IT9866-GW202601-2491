/****************************************************************************
**
**  Name:          btapp_av.c
**
**  Description:   Contains application functions for advanced audio/video.
**
**
**  Copyright (c) 2004-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"
#include "gki.h"

#if ( defined BTA_AV_INCLUDED ) && ( BTA_AV_INCLUDED == TRUE )

#include "bta_api.h"
#include "bta_av_api.h"
#include "btapp.h"
#include "btapp_int.h"
#include "bd.h"
#include <stdio.h>
#include <string.h>
//#include "tgt_api.h"
//#include "btapp_av_audio.h"
#include "btapp_av.h"
#include "btapp_avk.h"
#include "btapp_dm.h"
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
#include "btapp_plb.h"
#endif
//#include <btapp_codec.h>
#include "btapp_nv.h"
#include "btapp_utility.h"



#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
#include "btapp_fm.h"
#endif

#ifndef BTAPP_AV_NO_SCAN
#define BTAPP_AV_NO_SCAN    TRUE
#endif

#if defined(BTA_AV_MIN_DEBUG_TRACES)
tSTACKDLL_TRACE_LEVEL btapp_av_min_trace_level = {0};
tSTACKDLL_TRACE_LEVEL btapp_av_max_trace_level = {BT_TRACE_LEVEL_DEBUG};
#endif


/* BTAPP AV main control block */
tBTAPP_AV_CB btapp_av_cb;
BOOLEAN g_UI_HS_connecting = FALSE;
char gConnectedHeadSet[64] = "";
UINT8 gConnectedHeadsetAddr[6] = {0};

extern void btapp_rc_init(void);
extern void btapp_platform_av_init(void);
extern void btapp_platform_av_remote_cmd(UINT8 rc_handle, tBTA_AV_RC rc_id, tBTA_AV_STATE key_state, BOOLEAN vk_only);

extern tBTA_AV_HNDL	g_av_handle;

BOOLEAN btapp_av_audio_get_rcfg(tBTA_AV_HNDL handle);
void btapp_av_audio_set_prev_start();
void btapp_av_audio_chk_start();
void btapp_audio_reconfig_done();
void  btapp_av_timer_init();
void btapp_audio_read_cfg();


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static char *btapp_av_evt_to_str(tBTA_AV_EVT evt_code);
/*******************************************************************************
**
** Function         btapp_av_chk_rc_open
**
** Description
**
** Returns          TRUE, if an AVRCP channel is open
*******************************************************************************/
BOOLEAN btapp_av_chk_rc_open(void)
{
    BOOLEAN is_open = FALSE;
    int     xx = 0;

    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        if (btapp_av_cb.audio[xx].rc_open)
        {
            APPL_TRACE_DEBUG1("btapp_av_chk_rc_open %d", xx);
            is_open = TRUE;
            break;
        }
    }

    /* rc only channel */
    if (btapp_av_cb.audio[BTA_AV_NUM_STRS].rc_open)
    {
        APPL_TRACE_DEBUG0("btapp_av_chk_rc_open rc_only");
        is_open = TRUE;
    }

    APPL_TRACE_DEBUG1("btapp_av_chk_rc_open is_open:%d", is_open);
    return is_open;
}

/*******************************************************************************
**
** Function         btapp_av_get_audio_ent
**
** Description      find the tBTAPP_AV_AUDIO_ENT with the given bit "index"
**
** Returns          tBTAPP_AV_AUDIO_ENT *
*******************************************************************************/
tBTAPP_AV_AUDIO_ENT * btapp_av_get_audio_ent(int index)
{
    int     xx = 0, count = 0;
    UINT8   audio;
    tBTAPP_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        audio = BTAPP_AV_HNDL_TO_MASK(btapp_av_cb.audio[xx].av_handle);
        if (btapp_av_cb.audio_open & audio)
        {
            /* this channel is open */
            count++;
            if (count == index)
            {
                p_ret = &btapp_av_cb.audio[xx];
                break;
            }
        }
    }
    return p_ret;
}

#if (BTA_FM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
/*******************************************************************************
*******************************************************************************/
UINT8 btapp_av_close_ui_device(UINT8 ui_module)
{
    int     xx = 0;
    UINT8   audio;
    UINT8   num_closing_dev = 0;

    btapp_av_stop_stream();

    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        audio = BTAPP_AV_HNDL_TO_MASK(btapp_av_cb.audio[xx].av_handle);
        if ((btapp_av_cb.audio_open & audio) && (btapp_av_cb.audio[xx].conn_ui == ui_module))
        {
            btapp_av_cb.remote_disconnect = TRUE;
            /* this channel is open */
            btapp_av_close(btapp_av_cb.audio[xx].av_handle);
            btapp_av_cb.audio[xx].conn_ui = UI_NONE_ID;
            num_closing_dev++;
        }
    }

    btapp_av_cb.av_ui_module = UI_NONE_ID;

    return num_closing_dev;

}
#endif
/*******************************************************************************
**
** Function         btapp_av_get_audio_ent_by_addr
**
** Description      find the tBTAPP_AV_AUDIO_ENT with the given peer address
**
** Returns          tBTAPP_AV_AUDIO_ENT *
*******************************************************************************/
tBTAPP_AV_AUDIO_ENT * btapp_av_get_audio_ent_by_addr(BD_ADDR addr)
{
    int     xx;
    tBTAPP_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        if (bdcmp(btapp_av_cb.audio[xx].peer_addr, addr) == 0)
        {
            p_ret = &btapp_av_cb.audio[xx];
            break;
        }
    }

    if (!p_ret)
    {
        /* check rc only channel */
        if (bdcmp(btapp_av_cb.audio[BTA_AV_NUM_STRS].peer_addr, addr) == 0)
            p_ret = &btapp_av_cb.audio[BTA_AV_NUM_STRS];
    }
    return p_ret;
}

/*******************************************************************************
**
** Function         btapp_av_get_audio_ent_by_hndl
**
** Description      find the tBTAPP_AV_AUDIO_ENT with the given BTA AV handle
**
** Returns          tBTAPP_AV_AUDIO_ENT *
*******************************************************************************/
tBTAPP_AV_AUDIO_ENT * btapp_av_get_audio_ent_by_hndl(tBTA_AV_HNDL handle)
{
    int     xx;
    tBTAPP_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        if (btapp_av_cb.audio[xx].av_handle == handle)
        {
            p_ret = &btapp_av_cb.audio[xx];
            break;
        }
    }
    return p_ret;
}

/*******************************************************************************
**
** Function         btapp_av_get_audio_ent_by_rc_hndl
**
** Description      find the tBTAPP_AV_AUDIO_ENT with the given BTA AV RC handle
**
** Returns          tBTAPP_AV_AUDIO_ENT *
*******************************************************************************/
tBTAPP_AV_AUDIO_ENT * btapp_av_get_audio_ent_by_rc_hndl(UINT8 handle)
{
    int     xx;
    tBTAPP_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        if (btapp_av_cb.audio[xx].rc_handle == handle && btapp_av_cb.audio[xx].rc_open)
        {
            p_ret = &btapp_av_cb.audio[xx];
            APPL_TRACE_DEBUG5("btapp_av_get_audio_ent_by_rc_hndl xx:%d, rc_handle:%d av_handle:x%x addr:%06x-%06x",
                xx, handle, p_ret->av_handle,
                (p_ret->peer_addr[0] << 16) + (p_ret->peer_addr[1] << 8) + p_ret->peer_addr[2],
                (p_ret->peer_addr[3] << 16) + (p_ret->peer_addr[4] << 8) + p_ret->peer_addr[5]);
            break;
        }
    }

    if (!p_ret)
    {
        /* check rc only channel */
        if (btapp_av_cb.audio[BTA_AV_NUM_STRS].rc_handle == handle && btapp_av_cb.audio[BTA_AV_NUM_STRS].rc_open)
        {
            p_ret = &btapp_av_cb.audio[BTA_AV_NUM_STRS];
            APPL_TRACE_DEBUG4("btapp_av_get_audio_ent_by_rc_hndl rc only, rc_handle:%d av_handle:x%x addr:%06x-%06x",
                handle, p_ret->av_handle,
                (p_ret->peer_addr[0] << 16) + (p_ret->peer_addr[1] << 8) + p_ret->peer_addr[2],
                (p_ret->peer_addr[3] << 16) + (p_ret->peer_addr[4] << 8) + p_ret->peer_addr[5]);
        }
    }
    return p_ret;
}

/*******************************************************************************
**
** Function         btapp_av_free_audio_ent
**
** Description      free the given tBTAPP_AV_AUDIO_ENT
**
** Returns          void
*******************************************************************************/
void btapp_av_free_audio_ent(tBTAPP_AV_AUDIO_ENT *p_ent)
{
    BD_ADDR tmp_bdaddr={0, 0, 0, 0, 0, 0};
    UINT8   mask;

    if (p_ent)
    {
        mask = BTAPP_AV_HNDL_TO_MASK(p_ent->av_handle);
        APPL_TRACE_DEBUG2("btapp_av_free_audio_ent rc_open:%d, rc_play:%d", p_ent->rc_open, p_ent->rc_play);
        APPL_TRACE_DEBUG5("mask:x%x aopen:x%x, vopen:x%x addr:%06x-%06x", mask,
            btapp_av_cb.audio_open, btapp_av_cb.video_open,
            (p_ent->peer_addr[0] << 16) + (p_ent->peer_addr[1] << 8) + p_ent->peer_addr[2],
            (p_ent->peer_addr[3] << 16) + (p_ent->peer_addr[4] << 8) + p_ent->peer_addr[5]);
        if ((mask & btapp_av_cb.audio_open) == 0 && (mask & btapp_av_cb.video_open) == 0)
        {
            APPL_TRACE_DEBUG2("freed rc_open:%d, av_handle:x%x", p_ent->rc_open, p_ent->av_handle);
            if (p_ent->rc_open && p_ent->av_handle)
            {
                /* A2DP closes to cause the free ent - mark this ent as rc_only */
                p_ent->rc_only = TRUE;
            }
            else
            {
                p_ent->rc_supt = BTAPP_AV_RC_SUPT_UNKNOWN;
                bdcpy(p_ent->peer_addr, tmp_bdaddr);
            }
			
            p_ent->rc_play = FALSE;
        }
        else
        {
            /* A2DP for VDP is still connected. mark rc_open FALSE */
            p_ent->rc_open = FALSE;
            p_ent->rc_play = FALSE;
        }
    }
}

/*******************************************************************************
**
** Function         btapp_av_get_unused_audio_hndl
**
** Description      find an unused BTA AV handle
**
** Returns          tBTA_AV_HNDL
*******************************************************************************/
tBTA_AV_HNDL btapp_av_get_unused_audio_hndl(tBTA_AV_HNDL hndl)
{
    tBTA_AV_HNDL handle = 0, temp;
    int     xx;
    UINT8   audio;

    APPL_TRACE_DEBUG2("btapp_av_get_unused_audio_hndl:x%x, hndl:x%x",
        btapp_av_cb.audio_open, hndl);
    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        temp = xx+1;
        audio = BTA_AV_HNDL_TO_MSK(temp);
        temp |= BTA_AV_CHNL_AUDIO;
        APPL_TRACE_DEBUG2("audio:x%x, temp:x%x", audio, temp);
        if ((btapp_av_cb.audio_open & audio) == 0 && temp != hndl)
        {
            handle = temp;
            break;
        }
    }
    return handle;
}

/*******************************************************************************
**
** Function         btapp_av_audio_open_count
**
** Description      make sure the audio_open_count is correct.
**                  Called after audio_open mask is changed
**
** Returns          void
*******************************************************************************/
static void btapp_av_audio_open_count(void)
{
    int     xx = 0, count = 0;
    UINT8   audio;
    UINT8   mask, edr_count = 0;

    /* this mask is the EDR headsets that supports only 2Mbps */
    mask    = btapp_av_cb.audio_2edr & (~btapp_av_cb.audio_3edr);
    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        audio = BTAPP_AV_HNDL_TO_MASK(btapp_av_cb.audio[xx].av_handle);
        if (btapp_av_cb.audio_open & audio)
        {
            /* this channel is open */
            count++;
            if (mask & audio)
                edr_count++;
        }
    }
    btapp_av_cb.audio_open_count = count;
    btapp_av_cb.audio_2edr_count = edr_count;
}

/*******************************************************************************
**
** Function         btapp_av_chk_a2dp_open
**
** Description      Given an AVRCP handle, check if any A2DP channel is open
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_av_chk_a2dp_open(tBTAPP_AV_AUDIO_ENT  *p_ent)
{
    BOOLEAN is_open = FALSE;

    if (p_ent && p_ent->av_handle)
    {
        is_open = TRUE;
    }
    return is_open;
}

/*******************************************************************************
**
** Function         btapp_av_rc_play
**
** Description      AV received AVRCP play command or the equivelent (like PlayItem)
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_av_rc_play(tBTAPP_AV_AUDIO_ENT  *p_ent, BOOLEAN avc)
{
    tBTA_AV_HNDL new_handle = p_ent->av_handle;
    UINT8   mask;
    BOOLEAN is_open = FALSE;
    BOOLEAN rcfg;

    APPL_TRACE_DEBUG3("btapp_av_rc_play rc_open:%d hdl rc:x%x, av:x%x", p_ent->rc_open, p_ent->rc_handle, p_ent->av_handle);
    if (p_ent->av_handle == 0)
        new_handle = btapp_av_get_unused_audio_hndl(0);
    if (!avc)
    {
        /* not AV/C command -> AVRCP 1.4 related to play a selected item
         * load the selected file */
        btapp_audio_read_cfg(NULL);
    }

    /* check if A2DP is already open */
    mask = BTAPP_AV_HNDL_TO_MASK(new_handle);
    if (btapp_av_cb.audio_open & mask)
    {
        is_open = TRUE;
        rcfg = btapp_av_audio_get_rcfg(new_handle);
        APPL_TRACE_DEBUG2("rc_play causing api_start handle:0x%x, rcfg:%d", new_handle, rcfg);
        if (rcfg)
        {
            APPL_TRACE_DEBUG0("BTAPP_RCFG_NEEDED" );
            btapp_av_audio_set_prev_start();
            bta_av_co_reconfig(new_handle);
        }
        else
            btapp_av_start_stream();
    }
    else
    {
        p_ent->rc_play = TRUE;

        if (btapp_av_cb.audio_open == 0)
        {
            /* Must read file first if av is not connected. */
            btapp_audio_read_cfg(NULL);
        }

        btapp_av_open(p_ent->peer_addr, new_handle);
    }

    return is_open;
}


#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_av_audio_start_count
**
** Description      Gte the number of audio-streams ongoing
**
** Returns          num audio streams started
*******************************************************************************/
static UINT8 btapp_av_audio_start_count(void)
{
    int     xx = 0, count = 0;

    for (xx=0; xx<BTA_AV_NUM_STRS; xx++)
    {
        if (btapp_av_cb.audio[xx].started)
        {
            count++;
        }
    }
    return count;
}
#endif

/*******************************************************************************
**
** Function         btapp_av_cback
**
** Description      AV callback from BTA
**
**
** Returns          void
*******************************************************************************/
void btapp_av_cback(tBTA_AV_EVT event, tBTA_AV *p_data)
{

    FLOW_SPEC     flow_spec;
    char dbg_msg[60];
    tBTA_SERVICE_ID service = BTA_A2DP_SERVICE_ID;
    int     xx;
    UINT8   mask;
    tBTAPP_AV_AUDIO_ENT  *p_ent;
    tBTAPP_AV_AUDIO_ENT  *p_ent2;
    tBTAPP_REM_DEVICE    *p_device_rec;
    UINT8 * p_name = NULL;
    BD_ADDR tmp_bdaddr={0, 0, 0, 0, 0, 0};
    tBTA_AV_HNDL new_handle;
    BOOLEAN rc_only;
    BOOLEAN vk_only;
    tAVRC_MSG       *p_rc_msg;

    APPL_TRACE_DEBUG1("av callback %s", btapp_av_evt_to_str(event));

    switch (event)
    {
    case BTA_AV_ENABLE_EVT:
        btapp_av_cb.enabled = TRUE;
        btapp_av_cb.num_count = 0;
        if (!(p_data->enable.features & BTA_AV_FEAT_MULTI_AV))
        {
            if (btapp_cfg.av_num_audio > 2)
                btapp_cfg.av_num_audio = 2;
        }
        APPL_TRACE_DEBUG2("AV enabled. num_audio:%d, features:x%x",
            btapp_cfg.av_num_audio, p_data->enable.features);
        for (xx=0; xx < btapp_cfg.av_num_audio; xx++)
        {
            BTA_AvRegister(BTA_AV_CHNL_AUDIO, btapp_cfg.av_audio_service_name, 0);
        }

        if (btapp_cfg.av_services_used & BTA_VDP_SERVICE_MASK)
        {
            BTA_AvRegister(BTA_AV_CHNL_VIDEO, btapp_cfg.av_video_service_name, 0);
        }
        break;

    case BTA_AV_REGISTER_EVT:
        sprintf(dbg_msg, "BTA AV %s registered %s, hndl: x%x",
            (p_data->registr.chnl == BTA_AV_CHNL_VIDEO)?"Video":"Audio",
            (p_data->registr.status == BTA_AV_SUCCESS)?"OK":"Failed",
            p_data->registr.hndl);
        APPL_TRACE_DEBUG0(dbg_msg);
        if (p_data->registr.status == BTA_AV_SUCCESS)
        {
            if (p_data->registr.chnl == BTA_AV_CHNL_VIDEO)
            {
                btapp_av_cb.video_hndl = p_data->registr.hndl;
            }
            else
            {
                p_ent = &btapp_av_cb.audio[btapp_av_cb.num_count];
                p_ent->av_handle = p_data->registr.hndl;
                p_ent->index = btapp_av_cb.num_count;
                p_ent->rc_open = FALSE;
                p_ent->rc_supt = BTAPP_AV_RC_SUPT_UNKNOWN;
                btapp_av_cb.num_count++;
                APPL_TRACE_DEBUG2("BTAPP idx: %d AV hndl: x%x", p_ent->index, p_ent->av_handle);
            }
        }
        break;

    case BTA_AV_UPDATE_SEPS_EVT:
        if (p_data->update_seps.status)
        {
            APPL_TRACE_DEBUG0("Fail to update SEPs, close the current active AV connection first");

        }
        else
        {
            APPL_TRACE_DEBUG0("Update SEPs successfully");
            if (p_data->update_seps.available == TRUE)
            {
                btapp_av_cb.is_source_seps_disabled = FALSE;
                //btapp_menu_av_devices ();
            }
            else
            {   btapp_av_cb.is_source_seps_disabled = TRUE;
                //btapp_menu_av_devices ();
            }


        }
        break;

    case BTA_AV_OPEN_EVT:

        APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x",
                          p_data->open.bd_addr[0], p_data->open.bd_addr[1],
                          p_data->open.bd_addr[2], p_data->open.bd_addr[3],
                          p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
        APPL_TRACE_DEBUG4("  Open channel: x%x, status:%d, starting: %d, edr x%x",
            p_data->open.chnl, p_data->open.status,
            p_data->open.starting, p_data->open.edr);

        xx = p_data->open.hndl;
        if (xx == btapp_av_cb.video_hndl)
        {
            service = BTA_VDP_SERVICE_ID;
            mask = 0;
            if (p_data->open.status == BTA_AV_SUCCESS)
                btapp_av_cb.video_open = BTAPP_AV_HNDL_TO_MASK(btapp_av_cb.video_hndl);
            else
                btapp_av_cb.video_open = 0;
        }
        else
        {
            p_ent = btapp_av_get_audio_ent_by_hndl(p_data->open.hndl);
            mask = BTAPP_AV_HNDL_TO_MASK(p_data->open.hndl);

            if (p_data->open.status == BTA_AV_SUCCESS)
            {
            	APPL_TRACE_DEBUG0("connect to headset done !!!!");
				//g_at_connecting = false;
				APPL_TRACE_DEBUG1("111 BTAPP_AK_GETSTATUS %d",BTAPP_AK_GETSTATUS(BTAPP_AK_ST_START));
                extern UINT8 btapp_avk_play_is_start(void);
				APPL_TRACE_DEBUG1("222 BTAPP_AK_GETSTATUS %d",BTAPP_AK_GETSTATUS(BTAPP_AK_ST_START));
                if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_START))
                {
                    btapp_av_start_stream();
                }

#if (defined BTA_AG_INCLUDED) && (BTA_AG_INCLUDED == TRUE)
                #include "btapp_ag.h"
                extern tBTUI_AG_CB btui_ag_cb;
                if(btui_ag_cb.enabled && !btapp_ag_open_handle())
                {
                    //try connect Headset-HS
                    tBTAPP_REM_DEVICE headset;
                    memcpy(headset.bd_addr, p_data->open.bd_addr, BD_ADDR_LEN);
                    btapp_ag_connect_device(&headset, 0);
                }
#endif

                btapp_av_cb.audio_open |= mask;
                if (p_data->open.edr)
                    btapp_av_cb.audio_2edr |= mask;
                if (p_data->open.edr & BTA_AV_EDR_3MBPS)
                    btapp_av_cb.audio_3edr |= mask;
                if (p_ent)
                {
                    rc_only = FALSE;
                    if (bdcmp(p_ent->peer_addr, p_data->open.bd_addr) != 0)
                    {
                        bdcpy(p_ent->peer_addr, p_data->open.bd_addr);
                        p_device_rec = btapp_get_device_record(p_ent->peer_addr);
                        if (p_device_rec)
                        {
                            if (strlen(p_device_rec->short_name) > 0)
                            {
                                p_name = p_device_rec->short_name;
                            }
                            else if (strlen(p_device_rec->name) > 0)
                            {
                                p_name = p_device_rec->name;
                            }
                        }
                        if (p_name)
                            strcpy(p_ent->short_name, p_name);
                        else
                            sprintf(p_ent->short_name, "%02x-%02x-%02x",
                                    p_ent->peer_addr[3], p_ent->peer_addr[4], p_ent->peer_addr[5]);

						memset(gConnectedHeadSet,0,64);
						memcpy(gConnectedHeadSet,p_ent->short_name,BTAPP_DEV_NAME_LENGTH+1);
						APPL_TRACE_DEBUG1(" name:%s", p_ent->short_name);

						memcpy(gConnectedHeadsetAddr, p_data->open.bd_addr, BD_ADDR_LEN);
						
					}
                    else
                    {
                        if (p_ent->rc_only)
                        {
                            p_ent->rc_only = FALSE;
                            rc_only = TRUE;
                            APPL_TRACE_DEBUG2("  rc_play:%d, rc_only:%d", p_ent->rc_play, rc_only);
                        }
                    }

                    p_ent2 = &btapp_av_cb.audio[BTA_AV_NUM_STRS];
                    if (p_ent2->rc_open && bdcmp(p_ent2->peer_addr, p_data->open.bd_addr) == 0)
                    {
                        p_ent->rc_open = TRUE;
                        p_ent->rc_play = p_ent2->rc_play;
                        p_ent->rc_supt = p_ent2->rc_supt;
                        APPL_TRACE_ERROR0("clearing RC only");
                        btapp_av_free_audio_ent (p_ent2);
                        p_ent2->rc_open = FALSE;
                    }
                    else if (!rc_only)
                    {
                        p_ent->rc_open = FALSE;
                        p_ent->rc_play = FALSE;
                        p_ent->rc_supt = BTAPP_AV_RC_SUPT_UNKNOWN;
                    }

                    APPL_TRACE_DEBUG2("  rc_play:%d, rc_only:%d", p_ent->rc_play, rc_only);
                    if (p_ent->rc_play)
                    {
                        p_ent->rc_play = FALSE;
                        if (p_ent->rc_open)
                        {
                            APPL_TRACE_DEBUG0("rc_cmd causing api_start");
                            btapp_av_start_stream();
                        }
                    }

                    btapp_av_audio_open_count();

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE )
                    if (btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BB_TO_MM))
                    {
                        if (btapp_av_cb.audio_open_count == 1)
                            btapp_av_send_sync_request(BTAPP_AV_AUDIO_SYNC_TO_LITE);
                        else
                            btapp_av_send_sync_request(BTAPP_AV_AUDIO_RESYNC);
                    }
#endif
                }
                else
                {
                    APPL_TRACE_ERROR1("BTAPP can not find CB for:x%x", p_data->open.hndl);
                }
            }
            else /* open failed */
            {
            	APPL_TRACE_DEBUG0("connect to headset faile !!!!");
                btapp_av_cb.audio_open &= ~mask;
                btapp_av_cb.audio_2edr  &= ~mask;
                btapp_av_cb.audio_3edr  &= ~mask;
                btapp_av_audio_open_count();
                /* check if this open is caused by AVRCP play command */
                p_ent = btapp_av_get_audio_ent_by_addr (p_data->open.bd_addr);
                if (p_ent && p_ent->rc_play)
                {
                    APPL_TRACE_DEBUG3("  rc_play: %d, audio:0x%x, video: 0x%x",
                        p_ent->rc_play, btapp_av_cb.audio_open, btapp_av_cb.video_open );
                    p_ent->rc_play = FALSE;
                    /* it is caused by AVRCP play command.
                     * if any stream is still open, start it now */
                    if (btapp_av_cb.audio_open || btapp_av_cb.video_open)
                        btapp_av_start_stream();
                }
            }

            if (btapp_av_cb.audio_open_count > 1)
            {
                /* notify SBC encoder to update the bit rate settings
                 * according to the number of audio/edr channels */
                //btapp_codec_update_busy_level(BTAPP_CODEC_UNKNOWN);
            }
            else if (btapp_av_cb.audio_open_count == 1 && p_ent)
            {
                btapp_av_cb.active_audio_idx = p_ent->index;
            }

        } /* audio */

        if (p_data->open.status == BTA_AV_SUCCESS)
        {
#if (BTAPP_AV_NO_SCAN == TRUE)

            /* disable inquiry/page scan when 2 audio is connected and neither are not EDR */
            if (btapp_av_cb.audio_open_count == 2 && btapp_av_cb.audio_2edr == 0 && btapp_av_cb.audio_3edr == 0)
            {
            	APPL_TRACE_DEBUG0("2 audio is connectted, disable inquiry/page scan");
                BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_NON_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);
            }
#endif
            /* set qos */
            memset(&flow_spec, 0, sizeof(flow_spec));
            flow_spec.service_type = BEST_EFFORT;
            flow_spec.token_rate = 22900;
            /*flow_spec.token_rate = 45000;*/
            BTM_SetQoS(p_data->open.bd_addr, &flow_spec, NULL);

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
            if (btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BB_TO_BTC))
            {
                if (IS_ENCODER(btapp_cb.req_codec))
                {
                    /* Only when encoder is used. */
                    btapp_av_update_bit_rate();
                }
            }
#endif /* (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) */

#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
            if (btapp_av_cb.av_ui_module == UI_FM_ID)
            {
                btapp_app_send_mmi_evt (BTAPP_MMI_FM_BT_OPEN, p_data->open.status);
                p_ent->conn_ui = UI_FM_ID;
            }
#endif
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
            else if (btapp_av_cb.av_ui_module == UI_TIVO_ID)
            {
                btapp_app_send_mmi_evt (BTAPP_MMI_FM_BT_OPEN, p_data->open.status);
                p_ent->conn_ui = UI_TIVO_ID;
            }
            else if (btapp_av_cb.av_ui_module == UI_PLB_ID)
            {
                btapp_app_send_mmi_evt (BTAPP_MMI_PLB_BT_OPEN, p_data->open.status);
                p_ent->conn_ui = UI_PLB_ID;
            }
#endif
			if(btapp_av_cb.pfn)
				btapp_av_cb.pfn(1);
        }
        APPL_TRACE_DEBUG4("  handle: %d, mask:0x%x, audio:0x%x, video: 0x%x",
            p_data->open.hndl, mask, btapp_av_cb.audio_open, btapp_av_cb.video_open );
#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
        if ((btapp_av_cb.av_ui_module != UI_FM_ID) || p_data->open.status != BTA_AV_SUCCESS ||
             btapp_av_cb.audio_open_count != 0)
#endif
        {

        }

		g_UI_HS_connecting = FALSE;
		
		APPL_TRACE_DEBUG1("g_UI_HS_connecting %d",g_UI_HS_connecting);

        break;

    case BTA_AV_PENDING_EVT:
        if (!btapp_cfg.av_reconn_rej)
        {
            /* a headset connects to the phone, but does not do anything.
             * if AVRCP channel is not open either, disconnect it */
            if (FALSE == btapp_av_chk_rc_open())
            {
                BTA_AvDisconnect(p_data->pend.bd_addr);
            }
        }
        else
        {
            /*
            if the implementation decides to connect to headset in this case, use this code
            */
            p_ent = btapp_av_get_audio_ent_by_addr(p_data->reject.bd_addr);
            if (p_ent)
            {
                new_handle = btapp_av_get_unused_audio_hndl(p_ent->av_handle);
                if (new_handle)
                {
                    if (btapp_av_cb.audio_open == 0)
                    {
                        /* Must read configuration first before opening the 1st connection. */
                        btapp_audio_read_cfg(NULL);
                    }

                    btapp_av_open(p_data->pend.bd_addr, new_handle);
                }
            }
        }
        break;

    case BTA_AV_REJECT_EVT:
        /* a headset connects to the phone with an AV control block that is already connected with another headset
         * connect back to the headset for better user experience.
         * The platform may need to add a little bit delay before initiating the connection back,
         * because the headset side may be in "disconnecting mode" */
        if (btapp_cfg.av_reconn_rej)
        {
            new_handle = btapp_av_get_unused_audio_hndl(p_data->reject.hndl);
            APPL_TRACE_DEBUG6("rejected bd_addr:%02x-%02x-%02x-%02x-%02x-%02x",
                              p_data->reject.bd_addr[0], p_data->reject.bd_addr[1],
                              p_data->reject.bd_addr[2], p_data->reject.bd_addr[3],
                              p_data->reject.bd_addr[4], p_data->reject.bd_addr[5]);
            APPL_TRACE_DEBUG2("new_handle:x%x, old_handle:x%x",
                new_handle, p_data->reject.hndl);	

			
			
            if (new_handle)
            {
                btapp_av_open(p_data->reject.bd_addr, new_handle);
            }


			
        }
        else
        {
        	APPL_TRACE_DEBUG0("BTA_AvDisconnect");
        	BTA_AvDisconnect(p_data->reject.bd_addr);
			
        }
        break;

    case BTA_AV_CLOSE_EVT:
        APPL_TRACE_DEBUG1("  Close channel: x%x", p_data->close.chnl);
        xx = p_data->close.hndl;
		
        if (xx == btapp_av_cb.video_hndl)
        {
            service = BTA_VDP_SERVICE_ID;
            btapp_av_cb.video_open = 0;
        }
        else
        {
            mask = BTAPP_AV_HNDL_TO_MASK(p_data->close.hndl);
            p_ent = btapp_av_get_audio_ent_by_hndl(p_data->close.hndl);
            btapp_av_cb.audio_open &= ~mask;
            btapp_av_cb.audio_2edr  &= ~mask;
            btapp_av_cb.audio_3edr  &= ~mask;
            btapp_av_cb.rcfg_chk_mask  &= ~mask;
            btapp_av_free_audio_ent (p_ent);
            btapp_av_audio_open_count();
#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE )
            if (btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BB_TO_MM))
            {
                if (btapp_av_cb.audio_open_count > 0)
                {
                    btapp_av_send_sync_request(BTAPP_AV_AUDIO_RESYNC);
                }
            }
#endif
            if (btapp_av_cb.audio_open_count == 0)
                btapp_av_cb.cur_play = 0;
			
			if(btapp_av_cb.pfn)
				btapp_av_cb.pfn(0);
            btapp_rc_change_play_status (AVRC_PLAYSTATE_STOPPED);
        }
#if (defined(BTA_AV_MIN_DEBUG_TRACES) && BTA_AV_MIN_DEBUG_TRACES == TRUE)
        if (btapp_cfg.av_min_trace && 0 == btapp_av_cb.audio_open_count)
        {
            appl_trace_level = BT_TRACE_LEVEL_DEBUG;
            bta_sys_set_trace_level(appl_trace_level);
            STACKDLL_SetTraceLevel(&btapp_av_max_trace_level, 0xFF);
        }
#endif

#if (BTA_FM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        /* connection drop when FM or PLB is playing */
        if (btapp_av_cb.av_ui_module != UI_NONE_ID)
        {
            if (!btapp_av_cb.remote_disconnect)
            {
                if ((btapp_av_cb.av_ui_module == UI_FM_ID) || (btapp_av_cb.av_ui_module == UI_TIVO_ID))
                    btapp_app_send_mmi_evt (BTAPP_MMI_FM_BT_DROP, btapp_av_cb.audio_open_count);
                else if (btapp_av_cb.av_ui_module == UI_PLB_ID)
                    btapp_app_send_mmi_evt (BTAPP_MMI_PLB_BT_DROP, btapp_av_cb.audio_open_count);
            }
        }
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        else if (btapp_plb_cb.closing > 0)
        {
            /* If playback is closing, av_ui_module is already None here. */
            btapp_app_send_mmi_evt (BTAPP_MMI_PLB_BT_DROP, btapp_av_cb.audio_open_count);
        }
#endif
#if (BTA_FM_INCLUDED == TRUE)
        else if (btapp_fm_cb.closing > 0)
        {
            /* If FM is closing, av_ui_module is already None here. */
            btapp_app_send_mmi_evt (BTAPP_MMI_FM_BT_DROP, btapp_av_cb.audio_open_count);
        }
#endif
        else
#endif


#if (BTA_FM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        if (btapp_av_cb.audio_open_count == 0)
        {
             btapp_av_cb.remote_disconnect = FALSE;
        }
#endif

#if (BTAPP_AV_NO_SCAN == TRUE)
        /* restore visibility settings when any channel is closed */
        if (btapp_device_db.visibility)
        {
            BTA_DmSetVisibility(BTA_DM_GENERAL_DISC, BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);
        }
        else
        {
            BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);
        }
#endif
        break;

    case BTA_AV_START_EVT:
        APPL_TRACE_DEBUG4("start handle:x%x, status:%d, suspending:%d, initiator:%d",
            p_data->start.hndl, p_data->start.status, p_data->start.suspending, p_data->start.initiator);
        if (p_data->start.status)
        {
            break;
        }

        if (p_data->start.suspending)
        {
            /* suspending on start -> SNK must be the INT.
             * mark this handle needs to check reconfig on suspend */
            btapp_av_cb.rcfg_chk_mask |= BTAPP_AV_HNDL_TO_MASK(p_data->start.hndl);
        }
#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        else if (p_data->start.hndl != btapp_av_cb.video_hndl)
        {
            p_ent = btapp_av_get_audio_ent_by_hndl(p_data->start.hndl);
            if (p_ent)
                p_ent->started = TRUE;
        }

#endif
#if (defined(BTA_AV_MIN_DEBUG_TRACES) && BTA_AV_MIN_DEBUG_TRACES == TRUE)
        if (btapp_cfg.av_min_trace)
        {
            appl_trace_level = btapp_cfg.av_min_trace;
            bta_sys_set_trace_level(appl_trace_level);
            btapp_av_min_trace_level.avp_trace_level = btapp_cfg.av_min_trace;
            STACKDLL_SetTraceLevel(&btapp_av_min_trace_level, 0);
        }
#endif
        btapp_rc_change_play_status (AVRC_PLAYSTATE_PLAYING);

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) && (BTA_FM_INCLUDED == TRUE)
        if (btapp_av_cb.av_ui_module == UI_FM_ID)
            btapp_app_send_mmi_evt (BTAPP_MMI_FM_BT_AV_START, p_data->start.status);
#endif

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        if (btapp_av_cb.av_ui_module == UI_PLB_ID)
            btapp_app_send_mmi_evt (BTAPP_MMI_PLB_AV_START, p_data->start.status);
#endif
     break;

    case BTA_AV_STOP_EVT:
#if (defined(BTA_AV_MIN_DEBUG_TRACES) && BTA_AV_MIN_DEBUG_TRACES == TRUE)
        if (btapp_cfg.av_min_trace)
        {
            appl_trace_level = BT_TRACE_LEVEL_DEBUG;
            bta_sys_set_trace_level(appl_trace_level);
            STACKDLL_SetTraceLevel(&btapp_av_max_trace_level, 0xFF);
        }
#endif
        /* If the original play_status is PAUSED, stay as PAUSED */
        if (btapp_av_cb.meta_info.play_status.play_status != AVRC_PLAYSTATE_PAUSED)
        {
            btapp_rc_change_play_status (AVRC_PLAYSTATE_STOPPED);
        }

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        if (p_data->suspend.hndl != btapp_av_cb.video_hndl)
        {
            p_ent = btapp_av_get_audio_ent_by_hndl(p_data->suspend.hndl);
            if (p_ent)
                p_ent->started = FALSE;
        }
        if (btapp_cb.is_switched)
        {
#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE)
            if ((btapp_cfg.av_auto_switch)
                &&( btapp_av_audio_get_rcfg(0) == BTAPP_RCFG_FALSE )
                &&( 0 == btapp_av_audio_start_count()))
            {
                if (btapp_cb.switch_to == BTA_DM_SW_BB_TO_MM)
                    btapp_dm_switch_mm2bb();
            }
#endif

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
            /* FM-Over-A2DP and network music(I2S->SBC->A2DP) are always switched to BTC */
            if ((btapp_cb.switch_to == BTA_DM_SW_BB_TO_BTC)
                    && (0 == btapp_av_audio_start_count()))
            {
#if (BTA_FM_INCLUDED == TRUE)
                /* Let FM handle the audio routing */
                if (btapp_av_cb.av_ui_module == UI_FM_ID)
                    btapp_app_send_mmi_evt (BTAPP_MMI_FM_BT_AV_STOP, p_data->open.status);
#endif

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
                /* Let FM handle the audio routing */
                if (btapp_av_cb.av_ui_module == UI_PLB_ID)
                    btapp_app_send_mmi_evt (BTAPP_MMI_PLB_AV_STOP, p_data->open.status);
#endif
            }
#endif
        }
#endif
        break;

    case BTA_AV_PROTECT_REQ_EVT:
        APPL_TRACE_DEBUG2("hdl:x%x  data len:%d", p_data->protect_req.hndl, p_data->protect_req.len);

        /* send protect response with same data for testing purposes */
        BTA_AvProtectRsp(p_data->protect_req.hndl, BTA_AV_ERR_NONE, p_data->protect_req.p_data, p_data->protect_req.len);
        break;

    case BTA_AV_PROTECT_RSP_EVT:
        APPL_TRACE_DEBUG2("  err_code:%d len:%d", p_data->protect_rsp.err_code, p_data->protect_rsp.len);
        break;

    case BTA_AV_RC_OPEN_EVT:
#if (defined(BTA_AV_MIN_DEBUG_TRACES) && BTA_AV_MIN_DEBUG_TRACES == TRUE)
        APPL_TRACE_ERROR4("RC_OPEN status:%d rc_handle:%d peer_features:%x, addr:%s", p_data->rc_open.status,
            p_data->rc_open.rc_handle, p_data->rc_open.peer_features,
            btapp_addr_str(p_data->rc_open.peer_addr));
#else
        APPL_TRACE_DEBUG4("RC_OPEN status:%d rc_handle:%d peer_features:%x, addr:%s", p_data->rc_open.status,
            p_data->rc_open.rc_handle, p_data->rc_open.peer_features,
            btapp_utl_bda_to_str(p_data->rc_open.peer_addr));
		APPL_TRACE_DEBUG1("p_data->rc_open.status %d",p_data->rc_open.status);//snadra
#endif

        p_ent = btapp_av_get_audio_ent_by_addr(p_data->rc_open.peer_addr);
        if (p_ent)
        {
            if (p_data->rc_open.status == BTA_AV_SUCCESS)
            {
                p_ent->rc_open = TRUE;
                p_ent->rc_supt = BTAPP_AV_RC_SUPT_YES;
                p_ent->rc_handle = p_data->rc_open.rc_handle;
                APPL_TRACE_DEBUG2("BTAPP idx: %d RC hndl: x%x", p_ent->index, p_ent->rc_handle);
            }
            else
            {
                btapp_av_free_audio_ent (p_ent);
            }
			
        }
        else
        {
            /* If the p_data->rc_open.bd_addr is not found in audio device, then add it to audio */
            if (FALSE == btapp_av_cb.audio[BTA_AV_NUM_STRS].rc_open)
            {
                APPL_TRACE_ERROR0("saving as RC only");
                p_ent = &btapp_av_cb.audio[BTA_AV_NUM_STRS];
                p_ent->index = BTA_AV_NUM_STRS;
                p_ent->rc_open = TRUE;
                p_ent->rc_supt = BTAPP_AV_RC_SUPT_YES;
                p_ent->rc_only = TRUE;
                p_ent->av_handle = 0;
                p_ent->rc_handle = p_data->rc_open.rc_handle;
                if (bdcmp(p_ent->peer_addr, p_data->rc_open.peer_addr) != 0)
                {
                    bdcpy(p_ent->peer_addr, p_data->rc_open.peer_addr);
                    p_device_rec = btapp_get_device_record(p_ent->peer_addr);
                    if (p_device_rec)
                    {
                        if (strlen(p_device_rec->short_name) > 0)
                        {
                            p_name = p_device_rec->short_name;
                        }
                        else if (strlen(p_device_rec->name) > 0)
                        {
                            p_name = p_device_rec->name;
                        }
                    }

                    if (p_name)
                        strcpy(p_ent->short_name, p_name);
                    else
                        sprintf(p_ent->short_name, "%02x-%02x-%02x",
                                p_ent->peer_addr[3], p_ent->peer_addr[4], p_ent->peer_addr[5]);
                }
            }
            else
                APPL_TRACE_ERROR1("BTAPP can not find CB for:%d", p_data->rc_open.rc_handle);
        }

        if (p_ent)
        {
            if (p_data->rc_open.peer_features & BTA_AV_FEAT_RCCT)
            {
                APPL_TRACE_DEBUG2("   Peer supports CT role (ver=0x%04x, CT features=0x%04x)", p_data->rc_open.ct.version, p_data->rc_open.ct.features);
                memcpy(&p_ent->peer_ct, &p_data->rc_open.ct, sizeof(tBTA_AV_RC_INFO));
            }
            if (p_data->rc_open.peer_features & BTA_AV_FEAT_RCTG)
            {
                APPL_TRACE_DEBUG2("   Peer supports TG role (ver=0x%04x, TG features=0x%04x)", p_data->rc_open.tg.version, p_data->rc_open.tg.features);
                memcpy(&p_ent->peer_tg, &p_data->rc_open.tg, sizeof(tBTA_AV_RC_INFO));
            }
            p_ent->peer_features = p_data->rc_open.peer_features;
        }
        btapp_rc_conn (p_ent);
        break;

    case BTA_AV_RC_CLOSE_EVT:
#if (defined(BTA_AV_MIN_DEBUG_TRACES) && BTA_AV_MIN_DEBUG_TRACES == TRUE)
        APPL_TRACE_ERROR1("RC_CLOSE, addr:%s", btapp_utl_bda_to_str(p_data->rc_close.peer_addr));
#else
        APPL_TRACE_DEBUG1("RC_CLOSE, addr:%s", btapp_utl_bda_to_str(p_data->rc_close.peer_addr));
#endif

        p_ent = btapp_av_get_audio_ent_by_addr(p_data->rc_close.peer_addr);
        if (!p_ent)
        {
            p_ent = btapp_av_get_audio_ent_by_rc_hndl (p_data->rc_close.rc_handle);
        }

        if (p_ent)
        {
            APPL_TRACE_DEBUG2("BTAPP idx: %d RC hndl: x%x", p_ent->index, p_ent->rc_handle);
            p_ent->rc_open = FALSE;
            p_ent->rc_only = FALSE;
            btapp_av_free_audio_ent (p_ent);
        }
        else
        {
            /* Check if there is AVRC only device in the audio list */
            p_ent = &btapp_av_cb.audio[BTA_AV_NUM_STRS];
            if (p_ent->rc_open)
            {
                APPL_TRACE_ERROR0("clearing RC only");
                btapp_av_free_audio_ent (p_ent);
            }
            else
                APPL_TRACE_ERROR1("BTAPP can not find CB for:%d", p_data->rc_close.rc_handle);
        }
        btapp_rc_conn (p_ent);

        if (btapp_av_chk_rc_open() == FALSE)
            btapp_rc_check_reset ();
        break;

    case BTA_AV_RC_FEAT_EVT:
        APPL_TRACE_DEBUG2("RC_FEAT rc_handle:%d peer_features:%x", p_data->rc_feat.rc_handle,
            p_data->rc_feat.peer_features);
        p_ent = btapp_av_get_audio_ent_by_rc_hndl (p_data->rc_feat.rc_handle);
        if (p_ent)
        {
            if (p_data->rc_feat.peer_features & BTA_AV_FEAT_RCCT)
            {
                APPL_TRACE_DEBUG2("   Peer supports CT role (ver=0x%04x, CT features=0x%04x)", p_data->rc_feat.ct.version, p_data->rc_feat.ct.features);
                memcpy(&p_ent->peer_ct, &p_data->rc_feat.ct, sizeof(tBTA_AV_RC_INFO));
            }
            if (p_data->rc_feat.peer_features & BTA_AV_FEAT_RCTG)
            {
                APPL_TRACE_DEBUG2("   Peer supports TG role (ver=0x%04x, TG features=0x%04x)", p_data->rc_feat.tg.version, p_data->rc_feat.tg.features);
                memcpy(&p_ent->peer_tg, &p_data->rc_feat.tg, sizeof(tBTA_AV_RC_INFO));
            }
            p_ent->peer_features = p_data->rc_feat.peer_features;

        }
        break;

    case BTA_AV_RC_CMD_TOUT_EVT:
        APPL_TRACE_ERROR2("BTA_AV_RC_CMD_TOUT_EVT: timeout waiting for response for command label 0x%02x (rc_handle=0x%02x)", p_data->rc_cmd_tout.label, p_data->rc_cmd_tout.rc_handle);
        break;

    case BTA_AV_REMOTE_CMD_EVT:
        APPL_TRACE_DEBUG5("  rc_hdl:x%x rc_id:0x%x key_state:%d, video_open:%x, audio_open:%x",
            p_data->remote_cmd.rc_handle, p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state,
            btapp_av_cb.video_open,btapp_av_cb.audio_open);

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        if (btapp_av_cb.av_ui_module != UI_TIVO_ID)  /* RC cmd will be handled by TIVO(FM) module. */
        {
#endif
        vk_only = FALSE;
        if ((p_data->remote_cmd.rc_id == AVRC_ID_PLAY) && (p_data->remote_cmd.key_state == BTA_AV_STATE_RELEASE))
        {
            p_ent = btapp_av_get_audio_ent_by_rc_hndl (p_data->remote_cmd.rc_handle);
            APPL_TRACE_DEBUG0("  rc play cmd only");
            if ( p_ent && p_ent->rc_handle == p_data->remote_cmd.rc_handle)
            {
                btapp_av_rc_play(p_ent, TRUE);
                vk_only = TRUE;
            }
        }

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        }
#endif

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        btapp_rc_pass_cmd(&p_data->remote_cmd);
#endif

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        if (btapp_av_cb.av_ui_module == UI_TIVO_ID)
        {
            /* Let each application module handle the rc cmd. */
            if (p_data->remote_cmd.key_state == BTA_AV_STATE_RELEASE)
                btapp_app_send_mmi_evt (BTAPP_MMI_FM_RC_CMD, p_data->remote_cmd.rc_id);
        }
        else
#endif
#if 0
        btapp_platform_av_remote_cmd(p_data->remote_cmd.rc_handle, p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state, vk_only);

        if (p_data->remote_cmd.rc_id == BTA_AV_VENDOR)
        {
            APPL_TRACE_DEBUG0(" Remote cmd for Metadata received.")
            btapp_rc_handler(event, p_data);
        }
#endif
        break;

    case BTA_AV_REMOTE_RSP_EVT:
        APPL_TRACE_DEBUG3("  rc_hdl:x%x rc_id:0x%x key_state:%d",
            p_data->remote_cmd.rc_handle, p_data->remote_rsp.rc_id, p_data->remote_rsp.key_state);
        APPL_TRACE_DEBUG2("  rsp_code:%d label:%d", p_data->remote_rsp.rsp_code, p_data->remote_rsp.label);
        break;

    case BTA_AV_VENDOR_CMD_EVT:
        APPL_TRACE_DEBUG3("  rc_hdl:x%x code:%d label:%d",
            p_data->vendor_cmd.rc_handle, p_data->vendor_cmd.code, p_data->vendor_cmd.label);
        APPL_TRACE_DEBUG2("  company_id:0x%x len:%d",p_data->vendor_cmd.company_id, p_data->vendor_cmd.len);

        /* send vendor response with same data for testing purposes */
        BTA_AvVendorRsp(p_data->vendor_cmd.rc_handle, p_data->vendor_cmd.label, BTA_AV_RSP_ACCEPT,
                        p_data->vendor_cmd.p_data, p_data->vendor_cmd.len, 0);
        break;

    case BTA_AV_VENDOR_RSP_EVT:
        APPL_TRACE_DEBUG3("  rc_hdl:x%x code:%d label:%d",
            p_data->vendor_cmd.rc_handle, p_data->vendor_rsp.code, p_data->vendor_rsp.label);
        APPL_TRACE_DEBUG2("  company_id:0x%x len:%d",p_data->vendor_rsp.company_id, p_data->vendor_rsp.len);
        break;

    case BTA_AV_META_MSG_EVT:
        btapp_rc_handler(event, p_data);
        p_rc_msg = p_data->meta_msg.p_msg;
        if (p_rc_msg->hdr.opcode == AVRC_OP_BROWSE && p_rc_msg->browse.p_browse_pkt)
        {
            GKI_freebuf (p_rc_msg->browse.p_browse_pkt);
            p_rc_msg->browse.p_browse_pkt = NULL;
        }
        break;

    case BTA_AV_RECONFIG_EVT:
        APPL_TRACE_EVENT2("  channel: x%x, recfg:%d",p_data->reconfig.chnl, p_data->reconfig.status);

        btapp_av_cb.num_rcfg_evt++;
        if (p_data->reconfig.status == BTA_AV_SUCCESS)
            btapp_av_cb.num_rcfg_ok++;

        APPL_TRACE_EVENT4("rcfg_evt:evt#%d,ok#%d,req#%d,audio_open_cnt#%d",btapp_av_cb.num_rcfg_evt,
                          btapp_av_cb.num_rcfg_ok, btapp_av_cb.num_rcfg_req, btapp_av_cb.audio_open_count);

        if ((btapp_av_cb.num_rcfg_evt == btapp_av_cb.audio_open_count) ||
            (btapp_av_cb.num_rcfg_evt == btapp_av_cb.num_rcfg_req))
        {
            if (btapp_av_cb.num_rcfg_ok > 0)
            {
                btapp_av_audio_chk_start();
                btapp_audio_reconfig_done();
#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE )
                if (btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BB_TO_MM))
                {
                    btapp_av_send_sync_request(BTAPP_AV_AUDIO_RECONFIG_DONE);
                }
#endif

#if (BTA_FM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
                /* Let each application(FM or PLB) handle the audio routing */
                if (btapp_av_cb.av_ui_module == UI_PLB_ID)
                    btapp_app_send_mmi_evt (BTAPP_MMI_PLB_AV_RECONFIG, p_data->reconfig.status);
                else if (btapp_av_cb.av_ui_module == UI_FM_ID)
                    btapp_app_send_mmi_evt (BTAPP_MMI_FM_BT_AV_RECONFIG, p_data->reconfig.status);
#endif
            }
            btapp_av_cb.num_rcfg_evt = 0;
            btapp_av_cb.num_rcfg_ok = 0;
            btapp_av_cb.num_rcfg_req = 0;
        }
        else
            APPL_TRACE_EVENT2("rcfg evt #%d, remaining:%d",btapp_av_cb.num_rcfg_evt,
                                (btapp_av_cb.audio_open_count - btapp_av_cb.num_rcfg_evt));
        break;

    case BTA_AV_SUSPEND_EVT:
        APPL_TRACE_EVENT2("  channel: x%x, suspend:%d",p_data->suspend.chnl, p_data->suspend.status);
        mask = BTAPP_AV_HNDL_TO_MASK(p_data->suspend.hndl);
        if (btapp_av_cb.rcfg_chk_mask & mask)
        {
            /* clear the mask */
            btapp_av_cb.rcfg_chk_mask &= ~mask;
            if ( btapp_av_audio_get_rcfg(p_data->suspend.hndl) == BTAPP_RCFG_NEEDED)
            {
                APPL_TRACE_DEBUG0("BTAPP_RCFG_NEEDED" );
                btapp_av_audio_set_prev_start();
                bta_av_co_reconfig(p_data->suspend.hndl);
            }
        }

        btapp_rc_change_play_status (AVRC_PLAYSTATE_PAUSED);

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        if (p_data->suspend.hndl != btapp_av_cb.video_hndl)
        {
            p_ent = btapp_av_get_audio_ent_by_hndl(p_data->suspend.hndl);
            if (p_ent)
                p_ent->started = FALSE;
        }
#endif

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE)
        if (btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BB_TO_MM))
        {
            if ((btapp_cfg.av_auto_switch)
                &&( btapp_av_audio_get_rcfg(0) == BTAPP_RCFG_FALSE )
                &&( 0 == btapp_av_audio_start_count()))
            {
                btapp_dm_switch_mm2bb();
            }
        }
#endif
        break;

    default:
        break;
    }
}


/*******************************************************************************
**
** Function         btapp_av_reset_co
**
** Description      reset the AV call-out control blocks (for testing purposes)
**
**
** Returns          void
*******************************************************************************/
void btapp_av_reset_co(void)
{
    extern void bta_av_co_init(void);
    bta_av_co_init();
}


/*******************************************************************************
**
** Function         btapp_av_init
**
** Description      Initializes AV
**
**
** Returns          void
*******************************************************************************/
void btapp_av_init(void)
{
    if (btapp_cfg.av_included)
    {
        memset(&btapp_av_cb, 0, sizeof(tBTAPP_AV_CB));

        if ((btapp_cfg.av_features & (BTA_AV_FEAT_RCTG|BTA_AV_FEAT_RCCT)) == 0)
        {
            /* metadata & vendor specific commands are not valid features, if AVRCP CT/TG roles are not enabled */
            btapp_cfg.av_features &= ~(BTA_AV_FEAT_VENDOR|BTA_AV_FEAT_METADATA);
        }
        if ((btapp_cfg.av_features & BTA_AV_FEAT_METADATA) == 0)
        {
            /* advanced control is a superset of metadata */
            btapp_cfg.av_features &= ~(BTA_AV_FEAT_ADV_CTRL);
        }
        if ((btapp_cfg.av_features & BTA_AV_FEAT_ADV_CTRL) == 0)
        {
            /* browsing channel is a superset of advanced control */
            btapp_cfg.av_features &= ~(BTA_AV_FEAT_BROWSE);
        }
        BTA_AvEnable(btapp_cfg.av_security,
                     btapp_cfg.av_features, btapp_av_cback);
#if (defined(BTA_AV_MIN_DEBUG_TRACES) && BTA_AV_MIN_DEBUG_TRACES == TRUE)
        btapp_av_min_trace_level.avp_trace_level = BT_TRACE_LEVEL_ERROR;
#endif

        btapp_platform_av_init();
        extern void bta_av_co_init(void);
        bta_av_co_init();

        btapp_rc_init();
		APPL_TRACE_DEBUG0("===btapp_audio_read_cfg btapp_audio_read_cfg btapp_audio_read_cfg===" );
        btapp_audio_read_cfg(NULL);
    }
}

/*******************************************************************************
**
** Function         btapp_av_disable
**
** Description      Disables AV
**
**
** Returns          void
*******************************************************************************/
void btapp_av_disable(void)
{
    BTA_AvDisable ();
    btapp_av_cb.enabled = FALSE;
}

/*******************************************************************************
**
** Function         btapp_av_update_seps
**
** Description      Change all the source SEPs to available or unavailable
**
** Parameter        available: True Set all SEPs to available
**                             FALSE Set all SEPs to unavailable
**
** Returns          void
*******************************************************************************/
void btapp_av_update_seps(BOOLEAN available)
{

    BTA_AvUpdateStreamEndPoints(available);
}

/*******************************************************************************
**
** Function         btapp_av_increase_vol
**
** Description      increase volume
**
**
** Returns          void
*******************************************************************************/
void btapp_av_increase_vol(void)
{
    tBTAPP_AV_AUDIO_ENT *p_ent = &btapp_av_cb.audio[btapp_av_cb.active_audio_idx];
    UINT8 rc_handle;

    if (p_ent && p_ent->rc_open)
    {
        rc_handle = p_ent->rc_handle;
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_UP, BTA_AV_STATE_PRESS);
        btapp_rc_next_label(p_ent);
        GKI_delay(200);
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_UP, AVRC_STATE_RELEASE);
        btapp_rc_next_label(p_ent);
    }
    else
    {
        APPL_TRACE_ERROR1("BTAPP can not find CB for active_audio_idx:%d", btapp_av_cb.active_audio_idx);
    }

}
/*******************************************************************************
**
** Function         btapp_av_decrease_vol
**
** Description      decrease volume
**
**
** Returns          void
*******************************************************************************/
void btapp_av_decrease_vol(void)
{
    tBTAPP_AV_AUDIO_ENT *p_ent = &btapp_av_cb.audio[btapp_av_cb.active_audio_idx];
    UINT8 rc_handle;

    if (p_ent && p_ent->rc_open)
    {
        rc_handle = p_ent->rc_handle;
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_DOWN, BTA_AV_STATE_PRESS);
        btapp_rc_next_label(p_ent);
        GKI_delay(200);
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_DOWN, AVRC_STATE_RELEASE);
        btapp_rc_next_label(p_ent);
    }
    else
    {
        APPL_TRACE_ERROR1("BTAPP can not find CB for active_audio_idx:%d", btapp_av_cb.active_audio_idx);
    }
}

/*******************************************************************************
**
** Function         btapp_av_open_rc
**
** Description      open AVRC channel
**
**
** Returns          void
*******************************************************************************/
void btapp_av_open_rc(void)
{
    tBTAPP_AV_AUDIO_ENT *p_ent = &btapp_av_cb.audio[btapp_av_cb.active_audio_idx];

    if (p_ent && !p_ent->rc_open)
    {
        BTA_AvOpenRc(p_ent->av_handle);
    }
    else
    {
        APPL_TRACE_ERROR1("BTAPP can not find CB for active_audio_idx:%d", btapp_av_cb.active_audio_idx);
    }
}

/*******************************************************************************
**
** Function         btapp_av_close_rc
**
** Description      close AVRC channel
**
**
** Returns          void
*******************************************************************************/
void btapp_av_close_rc(void)
{
    tBTAPP_AV_AUDIO_ENT *p_ent = &btapp_av_cb.audio[btapp_av_cb.active_audio_idx];

    if (p_ent && p_ent->rc_open)
    {
        BTA_AvCloseRc(p_ent->rc_handle);
    }
    else
    {
        APPL_TRACE_ERROR1("BTAPP can not find CB for active_audio_idx:%d", btapp_av_cb.active_audio_idx);
    }
}

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_av_get_bit_rate
**
** Description
**
** Returns
**
*******************************************************************************/
UINT16 btapp_av_get_bit_rate (UINT8 busy_level)
{
    UINT16 bit_rate = btapp_cfg.av_line_speed_kbps;
    UINT8  num_bits;
    UINT8   mask, count;

    if (busy_level > 6)
        bit_rate = btapp_cfg.av_line_speed_swampd;
    else
    {
        /* the bit rate is reduced for non-EDR headsets during multi-audio streaming
         * If at least one of the headsets supports EDR, it's OK to use the regular bit rate */

        /* initialize the busy level to the number of ACL connections */
        count = busy_level;
        if (btapp_av_cb.audio_open_count >= 2)
        {
            /* this mask is for the non-EDR headsets */
            mask    = btapp_av_cb.audio_open & (~btapp_av_cb.audio_2edr);
            num_bits = A2D_BitsSet(mask);
            if(num_bits == A2D_SET_ONE_BIT)
                count+=3;
            if(num_bits == A2D_SET_MULTL_BIT)
                count+=6;

            count += btapp_av_cb.audio_2edr_count;

            APPL_TRACE_DEBUG2("count:%d/%d", count, btapp_cfg.av_line_speed_count);

            if (count == btapp_cfg.av_line_speed_count)
            {
                bit_rate = (btapp_cfg.av_line_speed_kbps >> 1) + (btapp_cfg.av_line_speed_busy >> 1);
            }
            else if (count > btapp_cfg.av_line_speed_count)
            {
                bit_rate = btapp_cfg.av_line_speed_busy;
            }
        }
    }

    APPL_TRACE_DEBUG5("btapp_av_get_bit_rate: busy_level:%d bit_rate=%d ao=%d, a2e=%d, a3e=%d",
                       busy_level, bit_rate, btapp_av_cb.audio_open, btapp_av_cb.audio_2edr, btapp_av_cb.audio_3edr);

    return (bit_rate);
}
/*******************************************************************************
**
** Function         btapp_av_get_sbc_bitpool
**
** Description
**
** Returns
**
*******************************************************************************/
UINT8 btapp_av_get_sbc_bitpool(UINT16 bit_rate, tA2D_SBC_CIE cie)
{
    UINT16 sampling_freq;
    UINT8  num_channels;
    UINT8  block_length;
    UINT8  num_subbands;
    UINT32 frame_length, temp, bitpool;

    if ( cie.ch_mode == A2D_SBC_IE_CH_MD_MONO )
        num_channels = 1;
    else
        num_channels = 2;

    if ( cie.samp_freq == A2D_SBC_IE_SAMP_FREQ_16)
        sampling_freq = 16000;
    else if ( cie.samp_freq == A2D_SBC_IE_SAMP_FREQ_32)
        sampling_freq = 32000;
    else if ( cie.samp_freq == A2D_SBC_IE_SAMP_FREQ_44)
        sampling_freq = 44100;
    else if ( cie.samp_freq == A2D_SBC_IE_SAMP_FREQ_48)
        sampling_freq = 48000;


    if ( cie.block_len == A2D_SBC_IE_BLOCKS_4)
        block_length = 4;
    else if ( cie.block_len == A2D_SBC_IE_BLOCKS_8)
        block_length = 8;
    else if ( cie.block_len == A2D_SBC_IE_BLOCKS_12)
        block_length = 12;
    else if ( cie.block_len == A2D_SBC_IE_BLOCKS_16)
        block_length = 16;

    if ( cie.num_subbands == A2D_SBC_IE_SUBBAND_4)
        num_subbands = 4;
    else if ( cie.num_subbands == A2D_SBC_IE_SUBBAND_8)
        num_subbands = 8;

    frame_length = ((UINT32)bit_rate * 1000 * num_subbands * block_length) / (8 * sampling_freq);
    temp = frame_length - 4 - (4*num_subbands*num_channels)/8;
    if (( cie.ch_mode == A2D_SBC_IE_CH_MD_MONO )
     ||( cie.ch_mode == A2D_SBC_IE_CH_MD_DUAL ))
    {
        bitpool = (temp*8 - 7 + block_length*num_channels - 1)/(block_length*num_channels);
    }
    else if ( cie.ch_mode == A2D_SBC_IE_CH_MD_STEREO )
    {
        bitpool = (temp*8 - 7 + block_length - 1)/block_length;
    }
    else /* A2D_SBC_IE_CH_MD_JOINT */
    {
        bitpool = (temp*8 - 7 - num_subbands + block_length - 1)/block_length;
    }

    if ( bitpool > cie.max_bitpool )
        bitpool = cie.max_bitpool;

    if ( bitpool < cie.min_bitpool )
        bitpool = cie.min_bitpool;

    APPL_TRACE_DEBUG2("btapp_av_get_sbc_bitpool: bit_rate=%d, bitpool=%d",
                       bit_rate, bitpool);

    return ((UINT8)bitpool);
}

/*******************************************************************************
**
** Function         btapp_av_update_bit_rate
**
** Description
**
** Returns
**
*******************************************************************************/
void btapp_av_update_bit_rate(void)
{
    UINT16 bit_rate;
    tAUDIO_CODEC_TYPE codec_type;
    UINT32 param;
    tA2D_SBC_CIE    cie_sbc;
#if (A2D_M12_INCLUDED == TRUE)
    tA2D_M12_CIE    cie_m12;
#endif
#if (A2D_M24_INCLUDED == TRUE)
    tA2D_M24_CIE    cie_m24;
#endif
    UINT8           index, cie_index;
    UINT16  rt_hndl;

    if (btapp_av_cb.audio_open_count == 0)
        return;

    bit_rate   = btapp_av_get_bit_rate (btapp_codec_get_busy_level());
    if (bit_rate == btapp_av_cb.bit_rate)
        return;

    btapp_av_cb.bit_rate = bit_rate;

    /* parse codec infor */
    switch(btapp_av_cb.audio_cfg[BTA_AV_CODEC_TYPE_IDX])
    {
    case BTA_AV_CODEC_SBC:
        if (A2D_ParsSbcInfo(&cie_sbc, btapp_av_cb.audio_cfg, FALSE) != 0)
        {
            APPL_TRACE_ERROR0("Parsing SBC codec_info failed");
            return;
        }

        codec_type = AUDIO_CODEC_SBC_ENC;
        param      = btapp_av_get_sbc_bitpool(bit_rate, cie_sbc);
        break;

#if (A2D_M12_INCLUDED == TRUE)
    case BTA_AV_CODEC_M12:
        if (A2D_ParsM12Info(&cie_m12, btapp_av_cb.audio_cfg, FALSE) != 0)
        {
            APPL_TRACE_ERROR0("Parsing M12 codec_info failed");
            return;
        }

        /* 32, 40, 48, 56, 64 */
        if ( bit_rate < 80 )
            index = (bit_rate - 32)/8 + 1;
        /* 80, 96, 112, 128 */
        else if ( bit_rate < 160 )
            index = (bit_rate - 80)/16 + 6;
        /* 160, 192, 224, 256 */
        else if ( bit_rate < 320 )
            index = (bit_rate - 160)/32 + 10;
        /* 320 */
        else if ( bit_rate >= 320 )
            index = 14;

        /* index should not go above the specified value from the application. */
        for (cie_index = 14; cie_index > 0; cie_index--)
        {
            if ((cie_m12.bitrate >> cie_index) & 0x0001)
                break;
        }

        if (index > cie_index)
            index = cie_index;

        codec_type = AUDIO_CODEC_MP3_ENC;
        param = index;
#endif
        break;

#if (A2D_M24_INCLUDED == TRUE)
    case BTA_AV_CODEC_M24:
        if (A2D_ParsM24Info(&cie_m24, btapp_av_cb.audio_cfg, FALSE) != 0)
        {
            APPL_TRACE_ERROR0("Parsing M24 codec_info failed");
            return;
        }

        bit_rate *= 1000;

        /* bit_rate should not go above the specified value from the application. */
        if (bit_rate > cie_m24.bitrate)
            bit_rate = cie_m24.bitrate;

        codec_type = AUDIO_CODEC_AAC_ENC;
        param = bit_rate;
        break;
    }
#endif
    if (btapp_cb.audio_route_src == AUDIO_ROUTE_SRC_FMRX)
        rt_hndl = btapp_fm_cb.parms.rt_handle;
    else
        rt_hndl = btapp_plb_cb.rt_handle;

    BTA_RtSetCodecBitRate (rt_hndl, codec_type, param );

}

/*******************************************************************************
**
** Function         btapp_av_convert_cie_sbc_codec_config
**
** Description      Convert CIE into tCODEC_INFO structure.
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_av_convert_cie_sbc_codec_config(tA2D_SBC_CIE cie_sbc, tCODEC_INFO *p_codec_info)
{
    UINT16          bit_rate;

    if(!p_codec_info)
    {
        APPL_TRACE_ERROR0("p_codec_info uninitialized.");
        return FALSE;
    }

    switch (cie_sbc.samp_freq)
    {
        case A2D_SBC_IE_SAMP_FREQ_16:
            p_codec_info->sbc.sampling_freq = CODEC_INFO_SBC_SF_16K;
            break;
        case A2D_SBC_IE_SAMP_FREQ_32:
            p_codec_info->sbc.sampling_freq = CODEC_INFO_SBC_SF_32K;
            break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            p_codec_info->sbc.sampling_freq = CODEC_INFO_SBC_SF_44K;
            break;
        case A2D_SBC_IE_SAMP_FREQ_48:
            p_codec_info->sbc.sampling_freq = CODEC_INFO_SBC_SF_48K;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC sampling frequency(0x%x)", cie_sbc.samp_freq);
            return FALSE;
            break;
    }

    switch (cie_sbc.ch_mode)
    {
        case A2D_SBC_IE_CH_MD_MONO:
            p_codec_info->sbc.channel_mode = CODEC_INFO_SBC_CH_MONO;
            break;
        case A2D_SBC_IE_CH_MD_DUAL:
            p_codec_info->sbc.channel_mode = CODEC_INFO_SBC_CH_DUAL;
            break;
        case A2D_SBC_IE_CH_MD_STEREO:
            p_codec_info->sbc.channel_mode = CODEC_INFO_SBC_CH_STEREO;
            break;
        case A2D_SBC_IE_CH_MD_JOINT:
            p_codec_info->sbc.channel_mode = CODEC_INFO_SBC_CH_JS;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC channel mode(0x%x)", cie_sbc.ch_mode);
            return FALSE;
            break;
    }

    switch (cie_sbc.block_len)
    {
        case A2D_SBC_IE_BLOCKS_4:
            p_codec_info->sbc.block_length = CODEC_INFO_SBC_BLOCK_4;
            break;
        case A2D_SBC_IE_BLOCKS_8:
            p_codec_info->sbc.block_length = CODEC_INFO_SBC_BLOCK_8;
            break;
        case A2D_SBC_IE_BLOCKS_12:
            p_codec_info->sbc.block_length = CODEC_INFO_SBC_BLOCK_12;
            break;
        case A2D_SBC_IE_BLOCKS_16:
            p_codec_info->sbc.block_length = CODEC_INFO_SBC_BLOCK_16;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC block length(0x%x)", cie_sbc.block_len);
            return FALSE;
            break;
    }

    switch (cie_sbc.num_subbands)
    {
        case A2D_SBC_IE_SUBBAND_4:
            p_codec_info->sbc.num_subbands = CODEC_INFO_SBC_SUBBAND_4;
            break;
        case A2D_SBC_IE_SUBBAND_8:
            p_codec_info->sbc.num_subbands = CODEC_INFO_SBC_SUBBAND_8;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC sub bands(0x%x)", cie_sbc.num_subbands);
            return FALSE;
            break;
    }

    switch (cie_sbc.alloc_mthd)
    {
        case A2D_SBC_IE_ALLOC_MD_S:
            p_codec_info->sbc.alloc_method = CODEC_INFO_SBC_ALLOC_SNR;
            break;
        case A2D_SBC_IE_ALLOC_MD_L:
            p_codec_info->sbc.alloc_method = CODEC_INFO_SBC_ALLOC_LOUDNESS;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC alloc method(0x%x)", cie_sbc.alloc_mthd);
            return FALSE;
            break;
    }

    bit_rate = btapp_av_get_bit_rate (btapp_codec_get_busy_level());
    p_codec_info->sbc.bitpool_size = btapp_av_get_sbc_bitpool(bit_rate, cie_sbc);

    return TRUE;

}

/*******************************************************************************
**
** Function         btapp_av_convert_cie_mp3_codec_config
**
** Description      Convert MP3 CIE into tCODEC_INFO structure.
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_av_convert_cie_mp3_codec_config(tA2D_M12_CIE cie_mp3, tCODEC_INFO *p_codec_info)
{

    if(!p_codec_info)
    {
        APPL_TRACE_ERROR0("p_codec_info uninitialized.");
        return FALSE;
    }

    /* cie_mp3.layer : BTC lite stack only support LAYER3. */
    if (cie_mp3.layer != A2D_M12_IE_LAYER3)
    {
        APPL_TRACE_ERROR1("Invalid MP3 Layer (0x%02X)", cie_mp3.layer);
        return FALSE;
    }

    /* cie_mp3.crc : CRC field is not used in BTC lite stack. */

    switch (cie_mp3.ch_mode)
    {
        case A2D_M12_IE_CH_MD_MONO:
            p_codec_info->mp3.ch_mode = CODEC_INFO_MP3_MODE_SINGLE;
            break;
        case A2D_M12_IE_CH_MD_DUAL:
            p_codec_info->mp3.ch_mode = CODEC_INFO_MP3_MODE_DUAL;
            break;
        case A2D_M12_IE_CH_MD_STEREO:
            p_codec_info->mp3.ch_mode = CODEC_INFO_MP3_MODE_STEREO;
            break;
        case A2D_M12_IE_CH_MD_JOINT:
            p_codec_info->mp3.ch_mode = CODEC_INFO_MP3_MODE_JS;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid MP3 channel mode(0x%02X)", cie_mp3.ch_mode);
            return FALSE;
            break;
    }

    switch (cie_mp3.samp_freq)
    {
        case A2D_M12_IE_SAMP_FREQ_44:
            p_codec_info->mp3.sampling_freq = CODEC_INFO_MP3_SF_44K;
            break;
        case A2D_M12_IE_SAMP_FREQ_48:
            p_codec_info->mp3.sampling_freq = CODEC_INFO_MP3_SF_48K;
            break;
        case A2D_M12_IE_SAMP_FREQ_32:
            p_codec_info->mp3.sampling_freq = CODEC_INFO_MP3_SF_32K;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid MP3 sampling freq (0x%02X)", cie_mp3.samp_freq);
            return FALSE;
            break;
    }

    switch (cie_mp3.bitrate)
    {
        case A2D_M12_IE_BITRATE_0:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_FREE;
            break;
        case A2D_M12_IE_BITRATE_1:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_32K;
            break;
        case A2D_M12_IE_BITRATE_2:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_40K;
            break;
        case A2D_M12_IE_BITRATE_3:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_48K;
            break;
        case A2D_M12_IE_BITRATE_4:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_56K;
            break;
        case A2D_M12_IE_BITRATE_5:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_64K;
            break;
        case A2D_M12_IE_BITRATE_6:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_80K;
            break;
        case A2D_M12_IE_BITRATE_7:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_96K;
            break;
        case A2D_M12_IE_BITRATE_8:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_112K;
            break;
        case A2D_M12_IE_BITRATE_9:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_128K;
            break;
        case A2D_M12_IE_BITRATE_10:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_160K;
            break;
        case A2D_M12_IE_BITRATE_11:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_192K;
            break;
        case A2D_M12_IE_BITRATE_12:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_224K;
            break;
        case A2D_M12_IE_BITRATE_13:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_256K;
            break;
        case A2D_M12_IE_BITRATE_14:
            p_codec_info->mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_320K;
            break;
        default:
            APPL_TRACE_ERROR1("Invalid MP3 Bitrate Index (0x%02X)", cie_mp3.bitrate);
            return FALSE;
            break;
    }

    return TRUE;

}

/*******************************************************************************
**
** Function         btapp_av_convert_codec_config_sbc_cie
**
** Description      Convert tCODEC_INFO into SBC CIE structure.
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_av_convert_codec_config_sbc_cie(tCODEC_INFO *p_codec_info, tA2D_SBC_CIE *cie_sbc)
{
    if((!p_codec_info) || (!cie_sbc))
    {
        APPL_TRACE_ERROR0("btapp_av_convert_codec_config_sbc_cie: bad parameters");
        return FALSE;
    }

    switch (p_codec_info->sbc.sampling_freq)
    {
        case CODEC_INFO_SBC_SF_16K:
            cie_sbc->samp_freq  = A2D_SBC_IE_SAMP_FREQ_16;  break;
        case CODEC_INFO_SBC_SF_32K:
            cie_sbc->samp_freq  = A2D_SBC_IE_SAMP_FREQ_32;  break;
        case CODEC_INFO_SBC_SF_44K:
            cie_sbc->samp_freq  = A2D_SBC_IE_SAMP_FREQ_44;  break;
        case CODEC_INFO_SBC_SF_48K:
            cie_sbc->samp_freq  = A2D_SBC_IE_SAMP_FREQ_48;  break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC sampling frequency(0x%x)", p_codec_info->sbc.sampling_freq);
            return FALSE;
            break;
    }

    switch (p_codec_info->sbc.channel_mode)
    {
        case CODEC_INFO_SBC_CH_MONO:
            cie_sbc->ch_mode    = A2D_SBC_IE_CH_MD_MONO;    break;
        case CODEC_INFO_SBC_CH_DUAL:
            cie_sbc->ch_mode    = A2D_SBC_IE_CH_MD_DUAL;    break;
        case CODEC_INFO_SBC_CH_STEREO:
            cie_sbc->ch_mode    = A2D_SBC_IE_CH_MD_STEREO;  break;
        case CODEC_INFO_SBC_CH_JS:
            cie_sbc->ch_mode    = A2D_SBC_IE_CH_MD_JOINT;   break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC channel mode(0x%x)", p_codec_info->sbc.channel_mode);
            return FALSE;
            break;
    }

    switch (p_codec_info->sbc.block_length)
    {
        case CODEC_INFO_SBC_BLOCK_4:
            cie_sbc->block_len  = A2D_SBC_IE_BLOCKS_4;      break;
        case CODEC_INFO_SBC_BLOCK_8:
            cie_sbc->block_len  = A2D_SBC_IE_BLOCKS_8;      break;
        case CODEC_INFO_SBC_BLOCK_12:
            cie_sbc->block_len  = A2D_SBC_IE_BLOCKS_12;     break;
        case CODEC_INFO_SBC_BLOCK_16:
            cie_sbc->block_len  = A2D_SBC_IE_BLOCKS_16;     break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC block length(0x%x)", p_codec_info->sbc.block_length);
            return FALSE;
            break;
    }

    switch (p_codec_info->sbc.num_subbands)
    {
        case CODEC_INFO_SBC_SUBBAND_4:
            cie_sbc->num_subbands   = A2D_SBC_IE_SUBBAND_4; break;
        case CODEC_INFO_SBC_SUBBAND_8:
            cie_sbc->num_subbands   = A2D_SBC_IE_SUBBAND_8; break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC sub bands(0x%x)", p_codec_info->sbc.num_subbands);
            return FALSE;
            break;
    }

    switch (p_codec_info->sbc.alloc_method)
    {
        case CODEC_INFO_SBC_ALLOC_SNR:
            cie_sbc->alloc_mthd = A2D_SBC_IE_ALLOC_MD_S;    break;
        case CODEC_INFO_SBC_ALLOC_LOUDNESS:
            cie_sbc->alloc_mthd = A2D_SBC_IE_ALLOC_MD_L;    break;
        default:
            APPL_TRACE_ERROR1("Invalid SBC alloc method(0x%x)", p_codec_info->sbc.alloc_method);
            return FALSE;
            break;
    }

    /* TODO by mschoi: Not sure how to calculate max & min bitpool */
    cie_sbc->max_bitpool    = p_codec_info->sbc.bitpool_size;
    cie_sbc->min_bitpool    = A2D_SBC_IE_MIN_BITPOOL;

    return TRUE;

}

/*******************************************************************************
**
** Function         btapp_av_convert_codec_config_mp3_cie
**
** Description      Convert tCODEC_INFO into MP3 CIE structure.
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_av_convert_codec_config_mp3_cie(tCODEC_INFO *p_codec_info, tA2D_M12_CIE *cie_mp3)
{
    if((!p_codec_info) || (!cie_mp3))
    {
        APPL_TRACE_ERROR0("btapp_av_convert_codec_config_mp3_cie: bad parameters");
        return FALSE;
    }

    cie_mp3->layer = A2D_M12_IE_LAYER3;
    cie_mp3->crc = TRUE;

    switch (p_codec_info->mp3.ch_mode)
    {
        case CODEC_INFO_MP3_MODE_SINGLE:
            cie_mp3->ch_mode = A2D_M12_IE_CH_MD_MONO;       break;
        case CODEC_INFO_MP3_MODE_DUAL:
            cie_mp3->ch_mode = A2D_M12_IE_CH_MD_DUAL;       break;
        case CODEC_INFO_MP3_MODE_STEREO:
            cie_mp3->ch_mode = A2D_M12_IE_CH_MD_STEREO;     break;
        case CODEC_INFO_MP3_MODE_JS:
            cie_mp3->ch_mode = A2D_M12_IE_CH_MD_JOINT;      break;
        default:
            APPL_TRACE_ERROR1("Invalid MP3 Channel Mode(0x%02X)", p_codec_info->mp3.ch_mode);
            return FALSE;
            break;
    }

    switch (p_codec_info->mp3.sampling_freq)
    {
        case CODEC_INFO_MP3_SF_44K:
            cie_mp3->samp_freq = A2D_M12_IE_SAMP_FREQ_44;   break;
        case CODEC_INFO_MP3_SF_48K:
            cie_mp3->samp_freq = A2D_M12_IE_SAMP_FREQ_48;   break;
        case CODEC_INFO_MP3_SF_32K:
            cie_mp3->samp_freq = A2D_M12_IE_SAMP_FREQ_32;   break;
        default:
            APPL_TRACE_ERROR1("Invalid MP3 sampling freq (0x%02X)", p_codec_info->mp3.sampling_freq);
            return FALSE;
            break;
    }

    switch (p_codec_info->mp3.bitrate_index)
    {
        case CODEC_INFO_MP3_BR_IDX_FREE:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_0;        break;
        case CODEC_INFO_MP3_BR_IDX_32K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_1;        break;
        case CODEC_INFO_MP3_BR_IDX_40K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_2;        break;
        case CODEC_INFO_MP3_BR_IDX_48K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_3;        break;
        case CODEC_INFO_MP3_BR_IDX_56K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_4;        break;
        case CODEC_INFO_MP3_BR_IDX_64K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_5;        break;
        case CODEC_INFO_MP3_BR_IDX_80K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_6;        break;
        case CODEC_INFO_MP3_BR_IDX_96K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_7;        break;
        case CODEC_INFO_MP3_BR_IDX_112K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_8;        break;
        case CODEC_INFO_MP3_BR_IDX_128K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_9;        break;
        case CODEC_INFO_MP3_BR_IDX_160K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_10;       break;
        case CODEC_INFO_MP3_BR_IDX_192K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_11;       break;
        case CODEC_INFO_MP3_BR_IDX_224K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_12;       break;
        case CODEC_INFO_MP3_BR_IDX_256K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_13;       break;
        case CODEC_INFO_MP3_BR_IDX_320K:
            cie_mp3->bitrate = A2D_M12_IE_BITRATE_14;       break;
        default:
            APPL_TRACE_ERROR1("Invalid MP3 Bitrate Index (0x%02X)", p_codec_info->mp3.bitrate_index);
            return FALSE;
            break;
    }

    /* TODO by mschoi: don't know how to set mpf and vbr. */
    cie_mp3->mpf = 0;
    cie_mp3->vbr = FALSE;

    return TRUE;

}


#endif /* (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) */

/*******************************************************************************
**
** Function         btapp_av_close_rc_only
**
** Description      close AVRC channel
**
**
** Returns          void
*******************************************************************************/
void btapp_av_close_rc_only(void)
{
    tBTAPP_AV_AUDIO_ENT *p_ent = &btapp_av_cb.audio[BTA_AV_NUM_STRS];

    if (p_ent && p_ent->rc_open)
    {
        BTA_AvCloseRc(p_ent->rc_handle);
    }
    else
    {
        APPL_TRACE_ERROR0("RC only connection is not open:%d");
    }
}

/*******************************************************************************
**
** Function         btapp_av_start_stream
**
** Description      starts the stream
**
**
** Returns          void
*******************************************************************************/
void btapp_av_start_stream(void)
{
    APPL_TRACE_API0("btapp_av_start_stream");
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    tBTAPP_AV_AUDIO_ENT *p_ent = &btapp_av_cb.audio[btapp_av_cb.active_audio_idx];
    static BOOLEAN btc_played = FALSE;

    if (btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BB_TO_BTC))
        btc_played = TRUE;
#endif

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    if (!btapp_cb.is_switched)
    {
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
        /* Streaming should be started by the other app(FM, PLB) after BTC switch. */
        if (((btapp_av_cb.av_ui_module == UI_FM_ID) && (btapp_cfg.fm_embedded_lite)) ||
                (btapp_av_cb.av_ui_module == UI_PLB_ID))
        {
            APPL_TRACE_ERROR0("btapp_av_start_stream call before BTC stack switched.");
            return;
        }
#endif

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE)
        if (btapp_cfg.av_auto_switch)
        {
            btapp_cb.is_starting_stream = TRUE;
            btapp_dm_switch_bb2mm();
            return;
        }
#endif
    }
#endif /* ((BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)) */

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    /* If av streaming was done through BTC stack previously,   */
    /* and now back to normal av streaming.                     */
    if (btc_played && (btapp_av_cb.av_ui_module == UI_NONE_ID))
    {
        btc_played = FALSE;

        /* To reset the codec for the current file. */
        /* The current file config might be different than that of BTC streaming.   */
        /* If so, streaming will be started after reconfig is done.                 */
        /* Otherwise, start streaming right away.                                   */
        if (btapp_av_audio_codec_reconfig())
            return;
    }
    else if ((btapp_av_cb.av_ui_module == UI_TIVO_ID) && (p_ent->conn_ui != UI_TIVO_ID))
    {
        /* We may opened a regular audio file. Need to switch to TIVO file. */
        if (btapp_av_audio_codec_reconfig())
            return;
    }
#endif

    btapp_rc_change_play_status (AVRC_PLAYSTATE_PLAYING);
    btapp_rc_send_notification(AVRC_EVT_PLAY_POS_CHANGED);

    BTA_AvStart();

}

/*******************************************************************************
**
** Function         btapp_av_stop_stream
**
** Description      stops the stream
**
**
** Returns          void
*******************************************************************************/
void btapp_av_stop_stream(void)
{
    APPL_TRACE_API0("btapp_av_stop_stream");
    BTA_AvStop(btapp_cfg.av_suspd_rcfg);

    btapp_rc_change_play_status (AVRC_PLAYSTATE_STOPPED);
    btapp_rc_send_notification(AVRC_EVT_PLAY_POS_CHANGED);

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    btapp_av_cb.bit_rate = 0;
#endif
}
/*******************************************************************************
**
** Function         btapp_av_close
**
** Description      close connection
**
**
** Returns          void
*******************************************************************************/
void btapp_av_close(tBTA_AV_HNDL handle)
{

    BTA_AvClose(handle);



}


/*******************************************************************************
**
** Function         btapp_av_set_device_authorized
**
** Description      set device as authorized for AV connections
**
**
** Returns          void
*******************************************************************************/
void btapp_av_set_device_authorized (tBTAPP_REM_DEVICE * p_device_rec)
{


    /* update BTA with this information.If a device is set as trusted, BTA will
    not ask application for authorization, if authorization is required for
    AV connections */

    p_device_rec->is_trusted = TRUE;
    p_device_rec->trusted_mask |= (BTA_VDP_SERVICE_MASK | BTA_A2DP_SERVICE_MASK | BTA_AVRCP_SERVICE_MASK);
	APPL_TRACE_EVENT0("btapp_av_set_device_authorized");
    btapp_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);



}
/*******************************************************************************
**
** Function         btapp_av_open
**
** Description      open av connection
**
**
** Returns          void
*******************************************************************************/
void btapp_av_open(BD_ADDR bd_addr, tBTA_AV_HNDL handle)
{
    BOOLEAN use_rc = btapp_cfg.use_avrc;

    if (btapp_av_cb.video_hndl == handle)
        use_rc = FALSE;

    BTA_AvOpen(bd_addr, handle, use_rc, btapp_cfg.av_security);


}

/*******************************************************************************
**
** Function         btapp_av_get_open_count
**
** Description      given the open channel mask, find the mask and handle to
**                  open request
**
** Returns          number of handles to open
*******************************************************************************/
int btapp_av_get_open_count(tBTA_AV_CHNL chnl, tBTA_AV_HNDL *p_hndl)
{
    int     xx = 0, count = 0;
    UINT8   audio;

    if (chnl == BTA_AV_CHNL_AUDIO)
    {
        for (xx=0; xx<btapp_av_cb.num_count; xx++)
        {
            audio = BTAPP_AV_HNDL_TO_MASK(btapp_av_cb.audio[xx].av_handle);
            if ((btapp_av_cb.audio_open & audio) == 0)
            {
                /* this channel is not open yet */
                count++;
                *p_hndl = btapp_av_cb.audio[xx].av_handle;
                break;
            }
        }
    }
    else if (chnl == BTA_AV_CHNL_VIDEO)
    {
        if (btapp_av_cb.video_open == 0)
        {
            count++;
            *p_hndl = btapp_av_cb.video_hndl;
        }
    }
    return count;
}

/*******************************************************************************
**
** Function         btapp_av_connetced_dev
**
** Description      check to see the audio device of the bd address is connected
**                  or not.
**
** Returns          TRUE: is connected.
**                  FALSE: not connected.
*******************************************************************************/
BOOLEAN btapp_av_connetced_dev(BD_ADDR bda)
{
    int     xx = 0;
    UINT8   audio;
    BOOLEAN is_connected = FALSE;

    for (xx=0; xx<btapp_av_cb.num_count; xx++)
    {
        audio = BTAPP_AV_HNDL_TO_MASK(btapp_av_cb.audio[xx].av_handle);
        if ((btapp_av_cb.audio_open & audio) != 0)
        {
            /* bda is connected */
            if ( bdcmp(bda, btapp_av_cb.audio[xx].peer_addr) == 0)
            {
                is_connected = TRUE;
                break;
            }
        }
    }

    return is_connected;
}

void btapp_platform_av_init(void)
{
    /* initialize volume settings */
    btapp_av_cb.label        = 0;
    btapp_av_cb.active_ch    = BTA_AV_CHNL_AUDIO;
    btapp_av_cb.audio_open   = 0;
    btapp_av_cb.audio_2edr   = 0;
    btapp_av_cb.audio_3edr   = 0;
    btapp_av_cb.video_open   = 0;
    btapp_av_cb.play_count   = 0;
    btapp_av_cb.if_platform_av_init  = TRUE;
	btapp_av_cb.pfn = NULL;
    btapp_av_timer_init();
}

/*******************************************************************************
**
** Function         btapp_av_evt_to_str
**
** Description      This function convert the event code to string
**
** Returns          char *
**
*******************************************************************************/
static char *btapp_av_evt_to_str(tBTA_AV_EVT evt_code)
{
    switch(evt_code)
    {
    case BTA_AV_ENABLE_EVT:        return "BTA_AV_ENABLE_EVT";
    case BTA_AV_REGISTER_EVT:      return "BTA_AV_REGISTER_EVT";
    case BTA_AV_OPEN_EVT:          return "BTA_AV_OPEN_EVT";
    case BTA_AV_CLOSE_EVT:         return "BTA_AV_CLOSE_EVT";
    case BTA_AV_START_EVT:         return "BTA_AV_START_EVT";
    case BTA_AV_STOP_EVT:          return "BTA_AV_STOP_EVT";
    case BTA_AV_PROTECT_REQ_EVT:   return "BTA_AV_PROTECT_REQ_EVT";
    case BTA_AV_PROTECT_RSP_EVT:   return "BTA_AV_PROTECT_RSP_EVT";
    case BTA_AV_RC_OPEN_EVT:       return "BTA_AV_RC_OPEN_EVT";
    case BTA_AV_RC_CLOSE_EVT:      return "BTA_AV_RC_CLOSE_EVT";
    case BTA_AV_REMOTE_CMD_EVT:    return "BTA_AV_REMOTE_CMD_EVT";
    case BTA_AV_REMOTE_RSP_EVT:    return "BTA_AV_REMOTE_RSP_EVT";
    case BTA_AV_VENDOR_CMD_EVT:    return "BTA_AV_VENDOR_CMD_EVT";
    case BTA_AV_VENDOR_RSP_EVT:    return "BTA_AV_VENDOR_RSP_EVT";
    case BTA_AV_RECONFIG_EVT:      return "BTA_AV_RECONFIG_EVT";
    case BTA_AV_SUSPEND_EVT:       return "BTA_AV_SUSPEND_EVT";
    case BTA_AV_PENDING_EVT:       return "BTA_AV_PENDING_EVT";
    case BTA_AV_META_MSG_EVT:      return "BTA_AV_META_MSG_EVT";
    case BTA_AV_REJECT_EVT:        return "BTA_AV_REJECT_EVT";
    case BTA_AV_RC_FEAT_EVT:       return "BTA_AV_RC_FEAT_EVT";
    case BTA_AV_UPDATE_SEPS_EVT:   return "BTA_AV_UPDATE_SEPS_EVT";
    case BTA_AV_RC_CMD_TOUT_EVT:   return "BTA_AV_RC_CMD_TOUT_EVT";
    default:                       return "unknown";
    }
}

//register callback function which be called when received BTA_AV_OPEN_EVT or BTA_AV_CLOSE_EVT
void btapp_av_register_cbfunction(void* pfunction)
{
	AVCB function = (AVCB)pfunction;
	btapp_av_cb.pfn = function;

}


BOOLEAN btapp_av_check_record(BD_ADDR bda)
{
	if(btapp_get_device_record(bda))
		return TRUE;
	else
		return FALSE;

}

#endif
