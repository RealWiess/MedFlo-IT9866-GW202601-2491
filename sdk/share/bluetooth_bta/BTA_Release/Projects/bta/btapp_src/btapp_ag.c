/****************************************************************************
**                                                                           
**  Name:          btapp_ag.c                                           
**                 
**  Description:   Contains application code for audio gateway
**                 
**                                                                           
**  Copyright (c) 2002-2013, Broadcom Corp., All Rights Reserved.              
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
******************************************************************************/
 
#include "bt_target.h"
#include "gki.h"

#if( defined BTA_AG_INCLUDED ) && ( BTA_AG_INCLUDED == TRUE )

#include "bta_api.h"
#include "bta_ag_api.h"
//#include "btui.h"
//#include "btui_int.h"
#include "btapp_ag.h"
#include "btapp_dm.h"
#include "bd.h"
#include "btapp_nv.h"
//#include "btui_app_swrap.h"
#include <stdio.h>
#include <string.h>

#ifndef BTUI_AG_DEBUG
#define BTUI_AG_DEBUG   TRUE
#endif

#ifndef BTUI_AG_DIAL_TYPE_DEFAULT
#define BTUI_AG_DIAL_TYPE_DEFAULT       129
#endif

#if BTUI_AG_DEBUG == TRUE
static char *btui_ag_evt_str(tBTA_AG_EVT event);
#endif

/* BTUI AG main control block */
tBTUI_AG_CB btui_ag_cb;

#ifndef BTA_AG_CIND_INFO
#define BTA_AG_CIND_INFO     "(\"call\",(0,1)),(\"callsetup\",(0-3)),(\"service\",(0-3)),(\"signal\",(0-6)),(\"roam\",(0,1)),(\"battchg\",(0-5)),(\"callheld\",(0-2)),(\"bearer\",(0-7))"
#endif

 void btapp_ag_sco_repeater(UINT8 en);
 extern BOOLEAN btapp_hs_answer_call (void);
 extern BOOLEAN btapp_hs_reject_call (void);


 /*******************************************************************************
**
** Function         btapp_ag_cback
**
** Description      AG callback from BTA
**                  
**
** Returns          void
*******************************************************************************/
void btapp_ag_cback(tBTA_AG_EVT event, tBTA_AG *p_data)
{
    //tBTUI_BTA_MSG    *p_msg;
    UINT16            handle = 0;
    UINT8             app_id = 0;
    tBTA_AG_RES_DATA  data;
    tBTUI_CALL_DATA* call_data = NULL;
    
    if (p_data)
    {
        handle = p_data->hdr.handle;
        app_id = p_data->hdr.app_id;
    }

#if BTUI_AG_DEBUG == TRUE
    APPL_TRACE_DEBUG2("BTUI ag callback: Event %d (%s)", event, btui_ag_evt_str(event));
#else
    APPL_TRACE_DEBUG1("BTUI ag callback: Event %d", event);
#endif

    switch (event)
    {
    case BTA_AG_ENABLE_EVT:
        APPL_TRACE_DEBUG0("enable AG repeater");
        btapp_ag_sco_repeater((UINT8)1);
        btui_ag_cb.enabled = TRUE;
        break;
    case BTA_AG_REGISTER_EVT:
        APPL_TRACE_DEBUG3 ("BTA_AG_REGISTER_EVT handle:%d, app_id:%d, status:%d", 
                handle, app_id, p_data->reg.hdr.status);
        
        if (p_data->reg.hdr.status == BTA_AG_SUCCESS)
        {
            btui_ag_cb.app[app_id].handle = handle;
        }
        break;

    case BTA_AG_DISABLE_EVT:
        btui_ag_cb.enabled = FALSE;
        break;

    case BTA_AG_OPEN_EVT:
        if (!p_data)
        {
            APPL_TRACE_ERROR0("BTA_AG_OPEN_EVT with NULL p_data");
            break;
        }
        
        APPL_TRACE_DEBUG5("AG RFCOMM opened with handle:%d app_id:%d service_id:%d status:%d role:%s", handle,
            app_id, p_data->open.service_id, p_data->open.hdr.status, (p_data->open.is_initiator == TRUE)?"INITIATOR":"ACCEPTOR");

        if (p_data->open.hdr.status == BTA_AG_SUCCESS)
        {
#if (defined BTA_HS_INCLUDED &&(BTA_HS_INCLUDED == TRUE))        
            //if((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
            //{
            //    p_msg->open.hdr.event = BTUI_MMI_HS_AG_ACTIVE;
            //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
            //}
#endif
            btui_ag_cb.app[app_id].is_open = TRUE;
            btui_ag_cb.app[app_id].bvra_active = FALSE; /* Voice Recognition starts off */
            btui_ag_cb.app[app_id].service = p_data->open.service_id;
            btui_ag_cb.app[app_id].p_dev = btapp_dm_db_get_device_info(p_data->open.bd_addr);
        }
        else
        {
            APPL_TRACE_WARNING1("AG RFCOMM failed to open with status %d", p_data->open.hdr.status);
        }
        
        btui_ag_cb.call_handle = handle;
        break;

    case BTA_AG_CLOSE_EVT:
        APPL_TRACE_DEBUG2("  handle:%d app_id:%d", handle, app_id);
        btui_ag_cb.app[app_id].is_open = FALSE;

#if (defined BTA_HS_INCLUDED &&(BTA_HS_INCLUDED == TRUE))        
        if (!btui_ag_cb.app[BTUI_AG_ID_1].is_open &&
            !btui_ag_cb.app[BTUI_AG_ID_2].is_open)
        {   /* no active AG connection, send event to HS to enable the service */
            //if((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
            //{
            //    p_msg->open.hdr.event = BTUI_MMI_HS_AG_INACT;
            //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
            //}
        }
#endif
        //if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        //{
        //    p_msg->close.hdr.event = BTUI_MMI_CONN_DOWN;
        //    p_msg->close.service = btui_ag_cb.app[app_id].service;
        //    GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
        //}
        break;

    case BTA_AG_CONN_EVT:
        if (!p_data)
        {
            APPL_TRACE_ERROR0("BTA_AG_CONN_EVT with NULL p_data");
            break;
        }

#if (BTM_WBS_INCLUDED == TRUE )
        APPL_TRACE_DEBUG4("  handle:%d app_id:%d features:0x%x codec:0x%x", 
            handle, app_id, p_data->conn.peer_feat, p_data->conn.peer_codec);

        btui_ag_cb.app[app_id].peer_feat = p_data->conn.peer_feat;
        btui_ag_cb.app[app_id].peer_codec = p_data->conn.peer_codec;

        btapp_ag_set_wbs_codec(app_id, (tBTA_AG_PEER_CODEC)btapp_cfg.ag_sco_codec);
#else
        APPL_TRACE_DEBUG3("  handle:%d app_id:%d features:0x%x", handle, app_id, p_data->conn.peer_feat);
        btui_ag_cb.app[app_id].peer_feat = p_data->conn.peer_feat;
#endif

        /* Need to send RING to the peer HS if we having incoming call. */
        if (btui_ag_cb.call_state == BTUI_AG_CALL_INC)
        {
            /* indicate incoming call */
            //call_data = btui_ag_get_call_data_in_call_setup();
            if(call_data)
            {
                sprintf(data.str, "\"%s\"", call_data->dial_num);
                data.num = call_data->dial_type;
            }
            else
            {
                APPL_TRACE_DEBUG0("No call in call setup to Answer");
                strcpy(data.str, "\"8584538400\"");
                data.num = BTUI_AG_DIAL_TYPE_DEFAULT;
            }

            BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_IN_CALL_RES, &data);
        }
        break;

    case BTA_AG_AUDIO_OPEN_EVT:
        APPL_TRACE_DEBUG2("  handle:%d app_id:%d", handle, app_id);
        btui_ag_cb.app[app_id].is_audio_open = TRUE;
        break;

    case BTA_AG_AUDIO_CLOSE_EVT:
        /* If voice recognition is enabled send the unsolicited disable to peer
           because this could have closed because of a SCO transfer */
            btapp_ag_disable_vr(app_id);

        APPL_TRACE_DEBUG2("aud_close:  handle:%d app_id:%d", handle, app_id);
        btui_ag_cb.app[app_id].is_audio_open = FALSE;
        break;
#if (BTM_WBS_INCLUDED == TRUE )
    case BTA_AG_WBS_EVT:
        APPL_TRACE_DEBUG2("BTA_AG_WBS_EVT Set codec status %d codec %d 1=CVSD 2=MSBC", p_data->val.hdr.status, p_data->val.num);
        break;

#endif

    case BTA_AG_AT_CIND_EVT:
        strcpy(data.str, BTA_AG_CIND_INFO);
        BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_CIND_RES, &data);
        break;

    case BTA_AG_AT_CHUP_EVT:
        APPL_TRACE_DEBUG0("BTA_AG_AT_CHUP_EVT");
        btapp_hs_reject_call();
        break;

    case BTA_AG_AT_A_EVT:
        APPL_TRACE_DEBUG0("BTA_AG_AT_A_EVT");
        btapp_hs_answer_call();
        break;

    default:
        //if (p_data)
        //    btui_platform_ag_event(event, p_data);
        break;
    }
#if 0
    if (event <= BTA_AG_AUDIO_CLOSE_EVT)
    btui_app_swrap_notify_ag(btapp_ag_tran_ag_to_swrap_evt(event), p_data);
#endif
}

#if 0
/*******************************************************************************
**
** Function:    btapp_ag_tran_ag_to_swrap_evt
**
** Description: Convert AG event to script wrapper ISE event
**
** Returns:     Script wrapper ISE event
**
*******************************************************************************/
UINT32 btapp_ag_tran_ag_to_swrap_evt(tBTA_AG_EVT event){
    UINT32 ise_evt = BTUI_APP_ISE_EVT_ANY;
    switch (event)
    {

        case BTA_AG_CLOSE_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_RFCOMM_CLOSE;
            break;
    
        case BTA_AG_CONN_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_CONN;
            break;
    
        case BTA_AG_OPEN_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_RFCOMM_OPEN;
            break;

        case BTA_AG_AUDIO_OPEN_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_AUDIO_OPEN;
            break;

        case BTA_AG_AUDIO_CLOSE_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_AUDIO_CLOSE;
            break;

        case BTA_AG_ENABLE_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_ENABLE;
            break;

        case BTA_AG_REGISTER_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_REGISTER;
            break;

        case BTA_AG_SPK_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_SPK;
            break;

        case BTA_AG_MIC_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_MIC;
            break;

        case BTA_AG_AT_A_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_AT_A;
            break;

        case BTA_AG_AT_CLCC_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_CLCC;
            break;

        case BTA_AG_AT_CHUP_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_CHUP;
            break;

        case BTA_AG_AT_BLDN_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BLDN;
            break;

        case BTA_AG_AT_BVRA_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BVRA;
            break;

        case BTA_AG_AT_D_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_AT_D;
            break;

        case BTA_AG_AT_NREC_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_NREC;
            break;

        case BTA_AG_AT_BINP_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BINP;
            break;

        case BTA_AG_AT_VTS_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_VTS;
            break;

        case BTA_AG_AT_BTRH_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BTRH;
            break;

        case BTA_AG_AT_COPS_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_COPS;
            break;

        case BTA_AG_AT_CNUM_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_CNUM;
            break;

        case BTA_AG_AT_CBC_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_CBC;
            break;

        case BTA_AG_AT_CHLD_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_CHLD;
            break;

        case BTA_AG_AT_CIND_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_CIND;
            break;

        case BTA_AG_AT_BAC_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BAC;
            break;

        case BTA_AG_AT_BCS_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BCS;
            break;
#if (BTA_HFP_VERSION >= HFP_VERSION_1_7 && BTA_HFP_HF_IND_SUPPORTED == TRUE)
        case BTA_AG_AT_BIND_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BIND;
            break;

        case BTA_AG_AT_BIEV_EVT:
            ise_evt = BTUI_APP_ISE_EVT_AG_BIEV;
            break;
#endif

        default:
            ise_evt = BTUI_APP_ISE_EVT_UNKNOWN;
            break;
  
    }

    return ise_evt;
}
#endif
/*******************************************************************************
**
** Function         btapp_ag_disable
**
** Description      Disable AG service                 
**
** Returns          void
*******************************************************************************/
void btapp_ag_disable(void)
{
    BTA_AgDeregister(btui_ag_cb.app[BTUI_AG_ID_1].handle);        
    BTA_AgDeregister(btui_ag_cb.app[BTUI_AG_ID_2].handle);        
    BTA_AgDisable();

    btui_ag_cb.enabled = FALSE;
}

/*******************************************************************************
**
** Function         btapp_ag_enable
**
** Description      Enable AG service
**                  
**
** Returns          void
*******************************************************************************/
tBTA_SERVICE_MASK btapp_ag_enable(void)
{

    tBTA_SERVICE_MASK   services;
    char * p_service_names[2];

    /* get the supported services, In real phones, both HFP and HSP services are always supported
       The configuration option is mainly for test purposes */
    services = ( btapp_cfg.supported_services & (BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK) );

    if (services)
    {
        /* Enable Audio gateway service.
           BTA_AG_PARSE -> AT commands are parsed by BTA, and application
           receives events */
        if (BTA_AgEnable(BTA_AG_PARSE, btapp_ag_cback) != BTA_SUCCESS)
        {
            /* API failed */
            return 0;
        }

        /* service names received by the peer device during service discovery */
        p_service_names[0] = btapp_cfg.hsag_service_name;
        p_service_names[1] = btapp_cfg.hfag_service_name;

        /* BTA supports 2 AG instances at the same time,
           There can be two simulataneous AG connections,
           for example simultaneous service level connection to a
           headset and a car kit */
        if (btapp_cfg.ag_instances > 0)
        {
            BTA_AgRegister(services, btapp_cfg.ag_security, (tBTA_AG_FEAT) btapp_cfg.ag_features,
                           p_service_names, BTUI_AG_ID_1);
        }

        if (btapp_cfg.ag_instances > 1)
        {
            BTA_AgRegister(services, btapp_cfg.ag_security, (tBTA_AG_FEAT) btapp_cfg.ag_features,
                           p_service_names, BTUI_AG_ID_2);
        }
    }

    return services;
}


/*******************************************************************************
**
** Function         btapp_ag_close_conn
**
** Description      Close the AG service connection specified by the app_id
**                  
**
** Returns          void
*******************************************************************************/
void btapp_ag_close_conn(UINT8 app_id)
{

    BTA_AgClose(btui_ag_cb.app[app_id].handle);

}

/*******************************************************************************
**
** Function         btapp_ag_change_spk_volume
**
** Description      Change speaker volume on the connection specified by the app_id
**                  
**
** Returns          void
*******************************************************************************/
void btapp_ag_change_spk_volume(UINT8 spk_vol)
{
    tBTA_AG_RES_DATA    data;
    UINT8 app_id = btapp_ag_open_handle();
    APPL_TRACE_DEBUG2("btapp_ag_change_spk_volume app_id:%d spk_vol:%d", app_id, spk_vol);

    if(spk_vol > 15)
        spk_vol = 15;

    btui_ag_cb.app[app_id].spk_vol = spk_vol;

    data.num = btui_ag_cb.app[app_id].spk_vol;
    BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_SPK_RES, &data);
}

/*******************************************************************************
**
** Function         btapp_ag_change_mic_volume
**
** Description      Change MIC volume on the connection specified by the app_id
**                  
**
** Returns          void
*******************************************************************************/
void btapp_ag_change_mic_volume(UINT8 mic_vol)
{
    tBTA_AG_RES_DATA    data;
    UINT8 app_id = btapp_ag_open_handle();
    APPL_TRACE_DEBUG2("btapp_ag_change_mic_volume app_id:%d mic_vol:%d", app_id, mic_vol);

    if(mic_vol > 15)
        mic_vol = 15;

    btui_ag_cb.app[app_id].mic_vol = mic_vol;

    data.num = btui_ag_cb.app[app_id].mic_vol;
    BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_MIC_RES, &data);
}


/*******************************************************************************
**
** Function         btapp_ag_set_audio_device_authorized
**
** Description      Bond with the device
**                  
**
** Returns          void
*******************************************************************************/
void btapp_ag_set_audio_device_authorized (tBTAPP_REM_DEVICE * p_device_rec)
{


    /* update BTA with this information.If a device is set as trusted, BTA will
    not ask application for authorization, if authorization is required for
    AG connections */

    p_device_rec->is_trusted = TRUE;
    p_device_rec->trusted_mask |= BTA_HSP_SERVICE_MASK |BTA_HFP_SERVICE_MASK;
    btapp_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);


}

/*******************************************************************************
**
** Function         btapp_ag_set_device_default_headset
**
** Description      Sets the device as default headset
**                  
**
** Returns          void
*******************************************************************************/
void btapp_ag_set_device_default_headset (tBTAPP_REM_DEVICE * p_device_rec )
{

    int i;

    /* if any other device was set as
    default change that.*/
    for(i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        btapp_device_db.device[i].is_default = FALSE;

    }

    /* phone can try a audio connection to default
    audio device during a incoming call if no audio
    connections are active at that time */
    p_device_rec->is_default = TRUE;

    btapp_store_device(p_device_rec);



}

/*******************************************************************************
**
** Function         btapp_ag_connect_device
**
** Description      Makes an HSP/HFP connection to the specified device
**                  
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_ag_connect_device(tBTAPP_REM_DEVICE * p_device_rec, BOOLEAN is_fm)
{

    UINT16 handle;

    if (btui_ag_cb.app[BTUI_AG_ID_1].is_open == FALSE)
    {
        handle = btui_ag_cb.app[BTUI_AG_ID_1].handle;
    }
    else if (btui_ag_cb.app[BTUI_AG_ID_2].is_open == FALSE)
    {
        handle = btui_ag_cb.app[BTUI_AG_ID_2].handle;
    }
    else
    {
        /* both handles are busy, cannot make a new connection now */
        return FALSE;
    }

    BTA_AgOpen(handle, p_device_rec->bd_addr, btapp_cfg.ag_security,
        btapp_cfg.supported_services & (BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK));

    return TRUE;



}
/*******************************************************************************
**
** Function         btapp_ag_find_active_vr
**
** Description      Searches for the handle with Voice Recognition active.
**                  BTUI_AG_NUM_APP is returned if none active..
**                  Note: Assumes only one active VR at a time.
**
** Returns          void
*******************************************************************************/
UINT8 btapp_ag_find_active_vr(void)
{
    UINT8 xx;
    
    /* Find an active VR if any */
    for (xx = 0; xx < BTUI_AG_NUM_APP; xx++)
    {
        /* Should only be one active SCO */
        if (btui_ag_cb.app[xx].bvra_active)
        {
            break;
        }
    }

    return xx;
}

/*******************************************************************************
**
** Function         btapp_ag_disable_vr
**
** Description      Disables the voice recognition if enabled by sending an
**                  unsolicited BVRA:0 to the peer HS.
**                  BTUI_AG_NUM_APP is used to turn off any active VR peer.
**                  Note: Assumes only one active VR at a time.
**
** Returns          void
*******************************************************************************/
void btapp_ag_disable_vr(UINT8 app_id)
{
    tBTA_AG_RES_DATA  data;
    BOOLEAN           is_active = FALSE;

    if (app_id < BTUI_AG_NUM_APP && btui_ag_cb.app[app_id].bvra_active)
    {
        is_active = TRUE;   /* instance is specified and active */
    }
    /* See if disabling unspecified peer */
    else if (app_id == BTUI_AG_NUM_APP)
    {
        if ((app_id = btapp_ag_find_active_vr()) < BTUI_AG_NUM_APP)
        {
            is_active = TRUE;   /* instance is specified and active */
        }
    }

    if (is_active)
    {
        APPL_TRACE_DEBUG1("btapp_ag_disable_vr: Sending BVRA:0 for handle %d",
                          btui_ag_cb.app[app_id].handle);
        btui_ag_cb.app[app_id].bvra_active = FALSE;
        data.state = FALSE;
        BTA_AgResult(btui_ag_cb.app[app_id].handle, BTA_AG_BVRA_RES, &data);
    }
}

#if (BTM_WBS_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_ag_set_wbs_codec
**
** Description      Set codec type
**                  
**
** Returns          void
*******************************************************************************/
void btapp_ag_set_wbs_codec(UINT8 app_id, tBTA_AG_PEER_CODEC wb_codec)
{
    if((btapp_cfg.ag_features & BTA_AG_FEAT_CODEC) && 
        (btui_ag_cb.app[app_id].peer_feat & BTA_AG_PEER_FEAT_CODEC))
    {
        /* peer_codec is a bitmask & sco_codec should have one bit set at most. */
        if (wb_codec & btui_ag_cb.app[app_id].peer_codec)
            btui_ag_cb.app[app_id].sco_codec = wb_codec;
        else
            btui_ag_cb.app[app_id].sco_codec = BTA_AG_CODEC_CVSD;

        BTA_AgSetCodec(btui_ag_cb.call_handle, btui_ag_cb.app[app_id].sco_codec);
    }
    else
        btui_ag_cb.app[app_id].sco_codec = BTA_AG_CODEC_NONE;
}
#endif

#if BTUI_AG_DEBUG == TRUE
static char *btui_ag_evt_str(tBTA_AG_EVT event)
{
    switch (event)
    {
    case BTA_AG_ENABLE_EVT:
        return "Enable Evt";
    case BTA_AG_REGISTER_EVT:
        return "Register Evt";
    case BTA_AG_OPEN_EVT:
        return "Open Evt";
    case BTA_AG_CLOSE_EVT:
        return "Close Evt";
    case BTA_AG_CONN_EVT:
        return "Connect Evt";
    case BTA_AG_AUDIO_OPEN_EVT:
        return "Audio Opened";
    case BTA_AG_AUDIO_CLOSE_EVT:
        return "Audio Closed";
    case BTA_AG_DISABLE_EVT:
        return "Disable Evt";
    case BTA_AG_AT_CIND_EVT:
        return "CIND Evt";
    default:
        return "Other";
    }
}
#endif

/*******************************************************************************
**
** Function         btui_ag_open_handle
**
** Description      Return handle of first open AG connection
**
**
** Returns          void
*******************************************************************************/
UINT16 btapp_ag_open_handle(void)
{
    if (btui_ag_cb.app[BTUI_AG_ID_1].is_open)
    {
        return btui_ag_cb.app[BTUI_AG_ID_1].handle;
    }
    else if (btui_ag_cb.app[BTUI_AG_ID_2].is_open)
    {
        return btui_ag_cb.app[BTUI_AG_ID_2].handle;
    }
    else
    {
        return 0;
    }
}

/* Vendor Specific Opcode for SCO Forwarding */
#ifndef HCI_BRCM_SCO_FORWARD_ENABLE
#define HCI_BRCM_SCO_FORWARD_ENABLE         (0x00CE | (0x3F << 10))
#endif

void btapp_ag_sco_repeater(UINT8 en)
{
    UINT8 enable = en;
    BTM_VendorSpecificCommand (HCI_BRCM_SCO_FORWARD_ENABLE, 1, &enable, NULL);
}

/*******************************************************************************
**
** Function         btui_act_find_av_devices
**
** Description      Action function to search for headset devices
**
**
** Returns          void
*******************************************************************************/
void btapp_find_av_devices(tBTA_DM_SEARCH_CBACK *cf)
{
    /* filter: Major service class is set on 21st bit */
    tBTA_DM_COD_COND  dev_filt = {
              {0x20,0x00, 0x00}, {0x20,0x00, 0x00}};

    tBTA_DM_INQ inq_params;
    tBTA_SERVICE_MASK_EXT   service;
    tBT_UUID    ia_uuid = {2, {UUID_SERVCLASS_AUDIO_SINK}};

    memset(&inq_params, 0 ,sizeof(inq_params));

    inq_params.mode = BTA_DM_GENERAL_INQUIRY;
    inq_params.duration = BTAPP_DEFAULT_INQ_DURATION;
    inq_params.max_resps = btapp_cfg.num_inq_devices;
    inq_params.filter_type = BTA_DM_INQ_DEV_CLASS; //btapp_cfg.dm_inq_filt_type;
    inq_params.report_dup = TRUE;
    memcpy(&inq_params.filter_cond, &dev_filt, sizeof(tBTA_DM_INQ_COND));

    service.srvc_mask = BTA_HFP_SERVICE_MASK;
    service.num_uuid = 1;
    service.p_uuid = &ia_uuid;

    btapp_dm_search_ext(&service, &inq_params, cf);
}

#endif
