/****************************************************************************
**
**  Name:          btapp_hs.c
**
**  Description:   contains  headset application
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "btapp_nv.h"

#if( defined BTA_AG_INCLUDED ) && ( BTA_AG_INCLUDED == TRUE )
#include "btapp_ag.h"
#endif



#if(( defined BTA_HS_INCLUDED ) && ( BTA_HS_INCLUDED == TRUE ))

#include "bta_platform.h"
#include "bte_glue.h"

#define BTAPP_HS_DISC_SLC_IN_CALL

tBTAPP_HS_DB btapp_hs_db;

static void btapp_hs_cback (tBTA_HS_EVT event, tBTA_HS *p_data);

tBTAPP_HSC_CB btapp_hsc_cb;

BOOLEAN btapp_hs_restart_ak;

struct node{
	UINT8 *data;
	UINT16 dataSize;
	UINT16 hash;
	struct node *next;
};
typedef struct node Node;

extern Node *gpHashNodeStart;

const char * btapp_hs_event_name[] =
{
    /* these events are handled by the state machine */
    "BTA_HS_ENABLE_EVT",
    "BTA_HS_DISABLE_EVT",
    "BTA_HS_REGISTER_EVT",
    "BTA_HS_DEREGISTER_EVT",
    "BTA_HS_OPEN_EVT",
    "BTA_HS_CLOSE_EVT",
    "BTA_HS_CONN_EVT",
    "BTA_HS_CONN_LOSS_EVT",
    "BTA_HS_AUDIO_OPEN_REQ_EVT",
    "BTA_HS_AUDIO_OPEN_EVT",
    "BTA_HS_AUDIO_CLOSE_EVT",
    "BTA_HS_CIND_EVT",
    "BTA_HS_CIEV_EVT",
    "BTA_HS_RING_EVT",
    "BTA_HS_CLIP_EVT",
    "BTA_HS_BSIR_EVT",
    "BTA_HS_BVRA_EVT",
    "BTA_HS_CCWA_EVT",
    "BTA_HS_CHLD_EVT",
    "BTA_HS_VGM_EVT",
    "BTA_HS_VGS_EVT",
    "BTA_HS_BINP_EVT",
    "BTA_HS_BTRH_EVT",
    "BTA_HS_CNUM_EVT",
    "BTA_HS_COPS_EVT",
    "BTA_HS_CMEE_EVT",
    "BTA_HS_CLCC_EVT",
    "BTA_HS_UNAT_EVT",
    "BTA_HS_OK_EVT",
    "BTA_HS_ERROR_EVT",
    "BTA_HF_BCS_EVT",
    "BTA_HF_BIND_EVT",
    "BTA_HF_BUSY_EVT"
};

const char * btapp_hs_cmd_name[] =
{
    "BTA_HS_SPK_CMD",
    "BTA_HS_MIC_CMD",
    "BTA_HS_CKPD_CMD",
    "BTA_HS_A_CMD",
    "BTA_HS_BINP_CMD",
    "BTA_HS_BVRA_CMD",
    "BTA_HS_BLDN_CMD",
    "BTA_HS_CHLD_CMD",
    "BTA_HS_CHUP_CMD",
    "BTA_HS_CIND_CMD",
    "BTA_HS_CNUM_CMD",
    "BTA_HS_D_CMD",
    "BTA_HS_NREC_CMD",
    "BTA_HS_VTS_CMD",
    "BTA_HS_BTRH_CMD",
    "BTA_HS_COPS_CMD",
    "BTA_HS_CMEE_CMD",
    "BTA_HS_CLCC_CMD",
    "BTA_HS_BCC_CMD",
    "BTA_HS_BCS_CMD",
    "BTA_HS_BAC_CMD",
    "BTA_HS_BIA_CMD",
    "BTA_HS_BIND_CMD",
    "BTA_HS_BIEV_CMD",
    "BTA_HS_UNAT_CMD"
};

const char *btapp_hs_service_ind_name[] =
{
    "NO SERVICE",
    "SERV(CS) AVAIL",
    "SERV(VOIP) AVAIL",
    "SERV(CS,VOIP) AVAIL"
};

const char *btapp_hs_call_ind_name[] =
{
    "NO CALL",
    "ACTIVE CALL"
};

const char *btapp_hs_callsetup_ind_name[] =
{
    "CALLSETUP DONE",
    "INCOMING CALL",
    "OUTGOING CALL",
    "ALERTING REMOTE"
};

const char *btapp_hs_callheld_ind_name[] =
{
    "NONE ON-HOLD",
    "ACTIVE+HOLD",
    "ALL ON-HOLD"
};

const char *btapp_hs_roam_ind_name[] =
{
    "HOME NETWORK",
    "ROAMING"
};

const char *btapp_hs_bearer_ind_name[] =
{
    "BEARER WLAN",
    "BEARER BLUETOOTH",
    "BEARER WIRED",
    "BEARER 2G_3G",
    "BEARER WIMAX",
    "BEARER RESERVED1",
    "BEARER RESERVED2",
    "BEARER RESERVED3"
};




static pthread_t test_task;
static UINT8 test_task_enable = 0;
static UINT8 ans_aleard = 0 ;
//bool gCalling = false, gEnding = false, gSleepModeBT = false;
bool gSleepModeBT = false;
int gHFstate = 0;

char gCallNum[64] = "", gCallName[64] = "";

static void ag_audio_open_delay_cb(void* arg)
{
    test_task_enable = 1;
    usleep(2*1000*1000); //delay 2s
    UINT16 ag_handle = btapp_ag_open_handle();
    APPL_TRACE_EVENT1("open AG audio %d", ag_handle);
    BTA_AgAudioOpen(ag_handle);
    test_task_enable = 0;
    pthread_exit(NULL);
}

/*******************************************************************************
**
** Function         btapp_hs_init
**
** Description      Initializes mono headset
**
** Returns          void
*******************************************************************************/
void btapp_hs_init(void)
{
    tBTA_HS_SETTINGS hs_setting;
    char * p_service_names[2];
    UINT8 btapp_hs_id;
    tBTA_SERVICE_MASK   services ;

    /* initialize mic/spk volume if no pre-stored value */
    if (btapp_hs_db.mic_vol == 0)
        btapp_hs_db.mic_vol = 5;
    if (btapp_hs_db.spk_vol == 0)
        btapp_hs_db.spk_vol = 5;

    services = ( btapp_cfg.supported_services & (BTA_HFP_HS_SERVICE_MASK | BTA_HSP_HS_SERVICE_MASK) );

    hs_setting.ecnr_enabled = (btapp_cfg.hs_features & BTA_HS_FEAT_ECNR) ? TRUE : FALSE;
    hs_setting.mic_vol = btapp_hs_db.mic_vol;
    hs_setting.spk_vol = btapp_hs_db.spk_vol;

    /* service names received by the peer device during service discovery */
    p_service_names[0] = btapp_cfg.hshs_service_name;
    p_service_names[1] = btapp_cfg.hfhs_service_name;

    BTA_HsEnable((tBTA_HS_CBACK *)btapp_hs_cback);

    /* Register service for all available connections */
    for(btapp_hs_id = 1; btapp_hs_id <= BTAPP_HS_MAX_NUM_CONN; btapp_hs_id++)
    {
        BTA_HsRegister(services, (tBTA_SEC) btapp_cfg.hs_security ,
            (tBTA_HS_FEAT) btapp_cfg.hs_features, &hs_setting,
            p_service_names, BTAPP_HS_APP_ID);
    }

    btapp_hsc_cb.enabled = TRUE;
}

/*******************************************************************************
**
** Function         btapp_hs_disable
**
** Description      Disable HS.
**
** Returns          void
*******************************************************************************/
void btapp_hs_disable(void)
{
    UINT8 handle;
    /* Register service for all available connections */
    for(handle = 1; handle <= BTAPP_HS_MAX_NUM_CONN; handle++)
    {
        BTA_HsDeregister(handle);
    }

    BTA_HsDisable();
}

/*******************************************************************************
**
** Function         btapp_hs_open
**
** Description      Establishes mono headset connections
**
** Returns          0 if failure, handle otherwise
*******************************************************************************/
UINT16 btapp_hs_open(void)
{
    /* All HSP and HFP connection we store as HFP. This do not matter
      since if device supports only HSP, we will be trying for HSP,
      inside BTA HS */
    BD_ADDR bdaddr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    UINT16 handle = 0;
    UINT8 i, null_bdaddr[BD_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    tBTA_SERVICE_MASK   services ;

    services = ( btapp_cfg.supported_services & (BTA_HFP_HS_SERVICE_MASK | BTA_HSP_HS_SERVICE_MASK) );

    /* check BD in DB */
    if (!btapp_get_device_record(btapp_hs_db.las_cnt_bda) )
    {
        APPL_TRACE_EVENT6("BD Addr not exist in DB %02x:%02x:%02x:%02x:%02x:%02x",
                 btapp_hs_db.las_cnt_bda[0], btapp_hs_db.las_cnt_bda[1],
                 btapp_hs_db.las_cnt_bda[2], btapp_hs_db.las_cnt_bda[3],
                 btapp_hs_db.las_cnt_bda[4], btapp_hs_db.las_cnt_bda[5]);
        return 0;
    }

    if(memcmp(btapp_hs_db.las_cnt_bda, null_bdaddr, BD_ADDR_LEN))
    {
        bdcpy(bdaddr, btapp_hs_db.las_cnt_bda);

        APPL_TRACE_EVENT6("Last Connected to BD Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 btapp_hs_db.las_cnt_bda[0], btapp_hs_db.las_cnt_bda[1],
                 btapp_hs_db.las_cnt_bda[2], btapp_hs_db.las_cnt_bda[3],
                 btapp_hs_db.las_cnt_bda[4], btapp_hs_db.las_cnt_bda[5]);
    }

    for(i = 0; i < BTAPP_HS_MAX_NUM_CONN; i++)
    {
        if(memcmp(bdaddr,null_bdaddr, BD_ADDR_LEN))
        {
            APPL_TRACE_EVENT6("Connecting to BD Addr %02x:%02x:%02x:%02x:%02x:%02x",
                       bdaddr[0],bdaddr[1],bdaddr[2],
                       bdaddr[3],bdaddr[4],bdaddr[5]);
            bdcpy(btapp_hsc_cb.conn_cb[i].connected_bd_addr, bdaddr);
            BTA_HsOpen((UINT16)(i+1), bdaddr, btapp_cfg.hs_security, services);
            handle = (UINT16)(i+1);
        }
    }
    return handle;
}

/*******************************************************************************
**
** Function         btapp_hs_open_handle
**
** Description      Establishes mono headset connection per handle
**
** Returns          TRUE or FALSE for success or failure
*******************************************************************************/
BOOLEAN btapp_hs_open_handle(UINT16 handle)
{
    /* All HSP and HFP connection we store as HFP. This do not matter
      since if device supports only HSP, we will be trying for HSP,
      inside BTA HS */
    BD_ADDR bdaddr;
    tBTA_SERVICE_MASK   services;

    services = ( btapp_cfg.supported_services & (BTA_HFP_HS_SERVICE_MASK | BTA_HSP_HS_SERVICE_MASK) );

    /* Check for valid handle */
    if(handle && !btapp_hsc_cb.conn_cb[handle-1].connection_active)
        bdcpy(bdaddr, btapp_hsc_cb.conn_cb[handle-1].connected_bd_addr);
    else
    {
        APPL_TRACE_WARNING1("HS Open failed: Conn Handle %d already exists", handle);
        return FALSE;
    }

    APPL_TRACE_EVENT1("HS Open for handle %d", handle);
    BTA_HsOpen(handle, bdaddr, btapp_cfg.hs_security, services);
    return TRUE;
}

/*******************************************************************************
**
** Function         btapp_hs_close
**
** Description      Close mono headset connection
**
** Returns          TRUE or FALSE for success or failure
*******************************************************************************/
BOOLEAN btapp_hs_close(UINT16 handle)
{
    UINT16 cb_index = handle - 1;
    BOOLEAN status = FALSE;

    if(btapp_hsc_cb.conn_cb[cb_index].connection_active)
    {
        BTA_HsClose(handle);
        status = TRUE;
    }

    return status;
}

/*******************************************************************************
**
** Function         btapp_hs_answer_call
**
** Description      Handles when user presses answer call button on Insight
**                  handset GUI
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_hs_answer_call (void)
{
    UINT16 handle = btapp_hs_get_active_handle(BTAPP_HS_ST_RINGACT);
    UINT16 cb_index = handle - 1;

    if(handle)
    {
        if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
        {
            BTA_HsCommand(handle,BTA_HS_CKPD_CMD, NULL);
        }
        else
        {
            btapp_hs_send_command(handle, BTA_HS_A_CMD, NULL);
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*******************************************************************************
**
** Function         btapp_hs_reject_call
**
** Description      Handles when user presses reject call button on Insight
**                  handset GUI
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_hs_reject_call (void)
{
    UINT16 handle = btapp_hs_get_active_handle(BTAPP_HS_ST_IN_CALL);
    UINT16 cb_index = handle - 1;

    if(handle)
    {
        if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
        {
            BTA_HsCommand(handle,BTA_HS_CKPD_CMD, NULL);
        }
        else
        {
            btapp_hs_send_command(handle, BTA_HS_CHUP_CMD, NULL);
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }


}

/*******************************************************************************
**
** Function         btapp_hs_end_call
**
** Description      Handles when user end call.
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_hs_end_call (void)
{
    UINT16 handle = btapp_hs_get_active_handle(BTAPP_HS_ST_IN_CALL);
    UINT16 cb_index = handle - 1;
    BOOLEAN end_call = TRUE;

    if(handle)
    {
        if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
        {
            BTA_HsCommand(handle,BTA_HS_CKPD_CMD, NULL);
        }
        else
        {
            /* always end call instead of transfering audio */
            btapp_hs_send_command(handle, BTA_HS_CHUP_CMD, NULL);
        }
    }
    else
    {
        end_call = FALSE;
        APPL_TRACE_WARNING0("End Call Ingnored: No Active Call");
    }
    return end_call;
}

/*******************************************************************************
**
** Function         btapp_hs_audio_transfer
**
** Description      Transfer audio between headset and phone
**
**
** Returns          void
*******************************************************************************/
void btapp_hs_audio_transfer (void)
{
    UINT16 handle = btapp_hs_get_active_handle(BTAPP_HS_ST_IN_CALL);
    UINT16 cb_index = handle - 1;

    if(handle)
    {
        if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HFP_HS_SERVICE_ID)
        {
            if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_SCOOPEN))
            {
                BTA_HsAudioClose(handle);
            }
            else
            {
                BTA_HsAudioOpen(handle);
            }
        }
    }
    else
    {
        APPL_TRACE_WARNING0("Audio Transfer Ingnored: No Active Call");
    }
}

/*******************************************************************************
**
** Function         btapp_hs_volup
**
** Description      Increases spk volume
**
** Returns          void
*******************************************************************************/
void btapp_hs_volup(void)
{
    tBTA_HS_CMD_DATA data;
    UINT16 handle;
	
    /* Send volume up update to all active connections */
    for(handle = 1; handle <= BTAPP_HS_MAX_NUM_CONN; handle++)
    {
        UINT16 cb_index = handle - 1;

        if(btapp_hsc_cb.conn_cb[cb_index].connection_active)
        {
            if (++btapp_hs_db.spk_vol > 15)
              btapp_hs_db.spk_vol = 15;  //max gain

            data.num = btapp_hs_db.spk_vol;
            BTA_HsCommand(handle, BTA_HS_SPK_CMD, (tBTA_HS_CMD_DATA*)&data);

            if (++btapp_hs_db.mic_vol > 15)
              btapp_hs_db.mic_vol = 15;  //max gain

            data.num = btapp_hs_db.mic_vol;
            BTA_HsCommand(handle, BTA_HS_MIC_CMD, (tBTA_HS_CMD_DATA*)&data);

            btapp_nv_store_hs_db();
        }
    }
	APPL_TRACE_EVENT1("btapp_hs_volup %d",btapp_hs_db.spk_vol);
}

/*******************************************************************************
**
** Function         btapp_hs_voldown
**
** Description      Decreases spk volume
**
** Returns          void
*******************************************************************************/
void btapp_hs_voldown(void)
{
    tBTA_HS_CMD_DATA data;

    UINT16 handle;

    /* Send volume down update to all active connections */
    for(handle = 1; handle <= BTAPP_HS_MAX_NUM_CONN; handle++)
    {
        UINT16 cb_index = handle - 1;

        if(btapp_hsc_cb.conn_cb[cb_index].connection_active)
        {
            if ((int8_t)--btapp_hs_db.spk_vol < 0 )
                btapp_hs_db.spk_vol = 0;    //min gain

            data.num = btapp_hs_db.spk_vol;
            BTA_HsCommand(handle, BTA_HS_SPK_CMD, (tBTA_HS_CMD_DATA*)&data);

            if ((int8_t)--btapp_hs_db.mic_vol < 0 )
                btapp_hs_db.mic_vol = 0;     //min gain

            data.num = btapp_hs_db.mic_vol;
            BTA_HsCommand(handle, BTA_HS_MIC_CMD, (tBTA_HS_CMD_DATA*)&data);

            btapp_nv_store_hs_db();
        }
    }
}

#if (BTM_WBS_INCLUDED == TRUE )
/*******************************************************************************
**
** Function         btapp_hs_process_bcs_evt
**
** Description      Process +BCS command. Reply with AT+BSC.
**
** Returns          void
*******************************************************************************/
void btapp_hs_process_bcs_evt(UINT16 handle, UINT16 val)
{
    UINT16 cb_index = handle - 1;
    tBTA_HS_CMD_DATA data;
    tBTAPP_HS_CONN_CB *p_conn_cb = &btapp_hsc_cb.conn_cb[cb_index];

    if (p_conn_cb->connection_active)
    {
#if (BTM_WBS_INCLUDED == TRUE )
        p_conn_cb->reject_msbc = FALSE;     //if set TRUE,it'll reject the WBS mode when peer device negotiate codec mode.
#endif
        if (p_conn_cb->reject_msbc && (val != UUID_CODEC_CVSD))
        {
            /* Send AT+BAC="1" instead of sending AT+BCS=2 */
            btapp_hs_send_command (handle, BTA_HS_BAC_CMD, (tBTA_HS_CMD_DATA *)"1");
            p_conn_cb->neg_codec_type = 1;
        }
        else
        {
            data.num = val;
            p_conn_cb->neg_codec_type = val;
            BTA_HsCommand(handle, BTA_HS_BCS_CMD, &data);
        }
    }
}
#endif

/*******************************************************************************
**
** Function         btapp_hs_send_command
**
** Description      Send AT command. This function used for testing AT commands
**
** Returns          void
*******************************************************************************/
void btapp_hs_send_command(UINT16 handle, tBTA_HS_CMD cmd, tBTA_HS_CMD_DATA* p_data)
{

     BTA_HsCommand(handle, cmd, (tBTA_HS_CMD_DATA*)p_data);

}

/*******************************************************************************
**
** Function         btapp_hs_get_last_dialed
**
** Description      Finds the handle for the last dialed connection
**
** Returns          void
*******************************************************************************/
UINT16 btapp_hs_get_last_dialed(void)
{
    UINT16 cb_index = 0;

    while(cb_index < BTAPP_HS_MAX_NUM_CONN)
    {
        if(btapp_hsc_cb.conn_cb[cb_index].connection_active &&
            btapp_hsc_cb.conn_cb[cb_index].last_dialed)
            return(cb_index + 1);
        cb_index++;
    }

    APPL_TRACE_EVENT0("No last Dialed HFP connection found");
    return(0);
}

/*******************************************************************************
**
** Function         btapp_hs_set_last_dialed
**
** Description      Sets the last dialed connection
**
** Returns          void
*******************************************************************************/
void btapp_hs_set_last_dialed(UINT16 handle)
{

    UINT16 cb_index;

    APPL_TRACE_EVENT1("Conn Handle %d set as last dialed", handle);

    for(cb_index = 0; cb_index < BTAPP_HS_MAX_NUM_CONN; cb_index++)
    {
        if(cb_index == handle - 1)
            btapp_hsc_cb.conn_cb[cb_index].last_dialed = TRUE;
        else
            btapp_hsc_cb.conn_cb[cb_index].last_dialed = FALSE;
    }
}

/*******************************************************************************
**
** Function         btapp_hs_get_num_active_conn
**
** Description      Forms a bit mask for active connections and determines total
**                  number of active connections.
**
** Returns          Number of active connections
*******************************************************************************/
UINT8 btapp_hs_get_num_active_conn(UINT8 *conn_mask)
{
    UINT8 num_conn = 0;
    UINT16 cb_index;

    *conn_mask = 0;

    for(cb_index = 0; cb_index < BTAPP_HS_MAX_NUM_CONN; cb_index++)
    {
        if(btapp_hsc_cb.conn_cb[cb_index].connection_active)
        {
            *conn_mask |= (1 << cb_index);
            num_conn++;
        }
    }
    return (num_conn);
}

/*******************************************************************************
**
** Function         btapp_hs_get_handle_mask
**
** Description      Forms a bit mask of connections with an active status flag.
**                  For example, connections with incoming call status.
**
** Returns          Bit mask of connections with active status flag
*******************************************************************************/
UINT8 btapp_hs_get_handle_mask(UINT16 status)
{
    UINT16 cb_index;
    UINT8 result = 0;

    for(cb_index = 0; cb_index < BTAPP_HS_MAX_NUM_CONN; cb_index++)
    {
        result |= ((btapp_hsc_cb.conn_cb[cb_index].status & status) ? 1 : 0) << cb_index;
    }

    return (result);
}

/*******************************************************************************
**
** Function         btapp_hs_get_active_handle
**
** Description      Determines the connection handle with the active status flag(s)
**                  The first handle that matches is returned.
**
** Returns          Active connection handle
*******************************************************************************/
UINT16 btapp_hs_get_active_handle(UINT16 status_mask)
{
    UINT16 cb_index = 0;

    for(cb_index = 0; cb_index < BTAPP_HS_MAX_NUM_CONN; cb_index++)
    {
        if((btapp_hsc_cb.conn_cb[cb_index].connection_active) &&
           (btapp_hsc_cb.conn_cb[cb_index].status & status_mask)
          )
            return (cb_index+1);
    }

    /* If active handle not found, check for HSP service */
    for(cb_index = 0; cb_index < BTAPP_HS_MAX_NUM_CONN; cb_index++)
    {
        if(btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
            return (cb_index+1);
    }
    return (0);
}

/*******************************************************************************
**
** Function         btapp_hs_sync_spk_vol
**
** Description      Synchronizes spk volume across all connections through reports
**
** Returns          void
*******************************************************************************/
void btapp_hs_sync_spk_vol(UINT16 handle, UINT16 val)
{
    UINT16 cb_index;
    tBTA_HS_CMD_DATA data;

    data.num = val;

    for(cb_index = 0; cb_index < BTAPP_HS_MAX_NUM_CONN; cb_index++)
    {
        if(btapp_hsc_cb.conn_cb[cb_index].connection_active && (cb_index != handle - 1))
        {
            BTA_HsCommand((UINT16)(cb_index+1), BTA_HS_SPK_CMD, (tBTA_HS_CMD_DATA*)&data);
            btapp_nv_store_hs_db();
        }
    }
}

/*******************************************************************************
**
** Function         btapp_hs_sync_mic_vol
**
** Description      Synchronizes mic volume across all connections through reports
**
** Returns          void
*******************************************************************************/
void btapp_hs_sync_mic_vol(UINT16 handle, UINT16 val)
{
    UINT16 cb_index;
    tBTA_HS_CMD_DATA data;

    data.num = val;

    for(cb_index = 0; cb_index < BTAPP_HS_MAX_NUM_CONN; cb_index++)
    {
        if(btapp_hsc_cb.conn_cb[cb_index].connection_active && (cb_index != handle - 1))
        {
            BTA_HsCommand((UINT16)(cb_index+1), BTA_HS_MIC_CMD, (tBTA_HS_CMD_DATA*)&data);
            btapp_nv_store_hs_db();
        }
    }
}

/*******************************************************************************
**
** Function         btapp_hs_keypress
**
** Description      User press key
**
** Returns          void
*******************************************************************************/
void btapp_hs_keypress(void)
{
    tBTA_HS_CMD_DATA data;
    UINT8 conn_mask;
    UINT8 num_conn;
    UINT16 default_cb_index;

    num_conn = btapp_hs_get_num_active_conn(&conn_mask);

    /* If no connection exist, open first connection (handle = 1) */
    if(num_conn == 0)
    {
        (void)btapp_hs_open();
    }
    /* one or more connection available */
    else
    {
        UINT16 cb_index;

        /* scan for incoming calls to answer */
        for(cb_index = 0; cb_index < num_conn; cb_index++)
        {
            if (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_INCOMING)
            {
                BTA_HsCommand((UINT16)(cb_index+1), BTA_HS_A_CMD, NULL);
                return;
            }
        }
        /* scan for call hold scenarios */
        for(cb_index = 0; cb_index < num_conn; cb_index++)
        {
            if(btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind == BTAPP_HS_CALLHELD_ACTIVE_HELD)
            {
                data.num = 1; /* Release all active calls and accepts the held or waiting call */
                BTA_HsCommand((UINT16)(cb_index+1), BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data);
                return;
            }
        }

        /* scan for active calls to hangup */
        for(cb_index = 0; cb_index < num_conn; cb_index++)
        {
            if((btapp_hsc_cb.conn_cb[cb_index].curr_call_ind == BTAPP_HS_CALL_ACTIVE) ||
               (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_OUTGOING)
               )
            {
                BTA_HsCommand((UINT16)(cb_index+1), BTA_HS_CHUP_CMD, NULL);
                return;
            }
        }

        /* No call activity, send HSP button press or VREC/Redial on default connection */
        if(conn_mask & 1)
            default_cb_index = 0;
        else if(conn_mask & 2)
            default_cb_index = 1;

        if (btapp_hsc_cb.conn_cb[default_cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
        {
            BTA_HsCommand((UINT16)(default_cb_index+1), BTA_HS_CKPD_CMD, NULL);
        }
        else if (btapp_hsc_cb.conn_cb[default_cb_index].connected_hs_service_id == BTA_HFP_HS_SERVICE_ID)
        {

            if((btapp_hsc_cb.conn_cb[default_cb_index].curr_call_ind == BTAPP_HS_CALL_INACTIVE)
                    && (btapp_hsc_cb.conn_cb[default_cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_NONE))
            {
                /* if both devices support voice recognition then do it */
                if((btapp_hsc_cb.conn_cb[default_cb_index].peer_feature & BTA_HS_PEER_FEAT_VREC)
                    && (btapp_cfg.hs_features & BTA_HS_FEAT_VREC))
                {

                    BTA_HsCommand((UINT16)(default_cb_index+1), BTA_HS_BVRA_CMD, NULL);
                    /* TODO: play some beep sound */

                }
                /* do last number redial */
                else
                {
                    BTA_HsCommand((UINT16)(default_cb_index+1), BTA_HS_BLDN_CMD, NULL);
                }
            }
        }
    }
}

/*******************************************************************************
**
** Function         btapp_hs_open_slc_and_audio
**
** Description      Open Service Level Connection and then open SCO.
                    This function is create to simulate the situation that
                    the user presses the talk button when there is no SLC.
**
** Returns          void
*******************************************************************************/
void btapp_hs_open_slc_and_audio(void)
{
    UINT8 conn_mask;
    UINT8 num_conn;
    UINT16 handle;

    num_conn = btapp_hs_get_num_active_conn(&conn_mask);

    /* If no connection exist, open first connection (handle = 1) */
    if(num_conn == 0)
    {
        handle = btapp_hs_open();

        if(handle)
        {
            /* We will open SCO as soon as the first SLC opens up */
            btapp_hsc_cb.conn_cb[handle - 1].open_audio_asap = TRUE;
        }
    }
    /* one or more connection available */
    else
    {
        APPL_TRACE_EVENT0("This function works only when there is no SLC.\n");
    }
}

/*******************************************************************************
**
** Function         btapp_hs_active_call_hold
**
** Description      User press long key
**
** Returns          void
*******************************************************************************/
void btapp_hs_active_call_hold(UINT16 handle)
{
    tBTA_HS_CMD_DATA data;
    UINT16 cb_index = handle - 1;

    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle, BTA_HS_CKPD_CMD, NULL);
    }
    else
    {
        /* Put the call on hold*/
        data.num = 2;
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_accept_call_wait
**
** Description      User press long key
**
** Returns          void
*******************************************************************************/
void btapp_hs_accept_call_wait(UINT16 handle)
{
    tBTA_HS_CMD_DATA data;
    UINT16 cb_index = handle - 1;

    data.num = 2;
    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle, BTA_HS_CKPD_CMD, NULL);
    }
    else
    {
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data);
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_reject_call_wait
**
** Description      User press long key
**
** Returns          void
*******************************************************************************/
void btapp_hs_reject_call_wait(UINT16 handle)
{
    tBTA_HS_CMD_DATA data;
    UINT16 cb_index = handle - 1;

    data.num = 0;
    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle, BTA_HS_CKPD_CMD, NULL);
    }
    else
    {
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data);
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_release_held_calls
**
** Description      User press long key
**
** Returns          void
*******************************************************************************/
void btapp_hs_release_held_calls(UINT16 handle)
{
    tBTA_HS_CMD_DATA data;
    UINT16 cb_index = handle - 1;

    data.num = 0;
    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle,BTA_HS_CKPD_CMD, NULL);
    }
    else
    {
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data);
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_release_active_accept_held
**
** Description      User press long key
**
** Returns          void
*******************************************************************************/
void btapp_hs_release_active_accept_held(UINT16 handle)
{
    tBTA_HS_CMD_DATA data;
    UINT16 cb_index = handle - 1;

    data.num = 1;
    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle, BTA_HS_CKPD_CMD, NULL);
    }
    else
    {
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data);
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_join_calls
**
** Description      Join held call.
**
** Returns          void
*******************************************************************************/
void btapp_hs_join_calls(UINT16 handle)
{
    tBTA_HS_CMD_DATA data1, data2;
    UINT16 cb_index = handle - 1;
    data1.num = 2;
    data2.num = 3;
    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle, BTA_HS_CKPD_CMD, NULL);
    }
    else
    {
        /* According to Errata 2037, have to send AT+CHLD=2 before AT+CHLD=3 to join the conf call */
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data1);
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data2);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_explicit_call_transfer
**
** Description      Join held call.
**
** Returns          void
*******************************************************************************/
void btapp_hs_explicit_call_transfer(UINT16 handle)
{
    tBTA_HS_CMD_DATA data;
    UINT16 cb_index = handle - 1;

    data.num = 4;
    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle, BTA_HS_CKPD_CMD, NULL);
    }
    else
    {
        BTA_HsCommand(handle, BTA_HS_CHLD_CMD, (tBTA_HS_CMD_DATA*)&data);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_voice_dial
**
** Description      User press long key
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_hs_voice_dial(UINT16 handle)
{
    tBTA_HS_CMD_DATA data;
    UINT16 cb_index = handle - 1;
    BOOLEAN voice_dial_result = TRUE;

    data.num = 1;
    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        voice_dial_result = FALSE;
    }
    else if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HFP_HS_SERVICE_ID)
    {
        if ((btapp_hsc_cb.conn_cb[cb_index].curr_call_ind == BTAPP_HS_CALL_INACTIVE)
            && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_NONE)
            &&((btapp_hsc_cb.conn_cb[cb_index].peer_feature & BTA_HS_PEER_FEAT_VREC)
               && (btapp_cfg.hs_features & BTA_HS_FEAT_VREC)))
        {
            if(BTAPP_HS_GETSTATUS(handle,BTAPP_HS_ST_VOICEDIALACT))
            {
                /* turn off voice activation*/
                data.num = 0;
                BTA_HsCommand(handle, BTA_HS_BVRA_CMD, (tBTA_HS_CMD_DATA*)&data);
                BTAPP_HS_RESETSTATUS(handle,BTAPP_HS_ST_VOICEDIALACT);
            }
            else
            {
                /* turn on voice activation*/
                data.num = 1;
                BTA_HsCommand(handle, BTA_HS_BVRA_CMD, (tBTA_HS_CMD_DATA*)&data);
                BTAPP_HS_SETSTATUS(handle,BTAPP_HS_ST_VOICEDIALACT);
                /* TODO: play beep for init voice recording */
            }
        }
        else
        {
            voice_dial_result = FALSE;
        }
    }

    return voice_dial_result;
}

/*******************************************************************************
**
** Function         btapp_hs_last_num_dial
**
** Description      User press long key
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_hs_last_num_dial(UINT16 handle)
{
    UINT16 cb_index = handle - 1;
    BOOLEAN dialed = FALSE;

    if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HSP_HS_SERVICE_ID)
    {
        BTA_HsCommand(handle, BTA_HS_CKPD_CMD, NULL);
    }
    else if (btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id == BTA_HFP_HS_SERVICE_ID)
    {
        if(BTAPP_HS_CALLSETUP_NONE != btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind)
            BTA_HsCommand(handle, BTA_HS_CHUP_CMD, NULL); /* Terminate the call */
        else
        {
            dialed = TRUE;
            BTA_HsCommand(handle, BTA_HS_BLDN_CMD, NULL);
        }
    }
    return dialed;
}

/*******************************************************************************
**
** Function         btapp_hs_find_indicator_id
**
** Description      parses the indicator string and dinds the position of a field
**
** Returns          void
*******************************************************************************/
static UINT8 btapp_hs_find_indicator_id(char * ind, char * field)
{
    UINT16 string_len = strlen(ind);
    UINT8 i, id = 0;
    BOOLEAN skip = FALSE;

    for(i=0; i< string_len ; i++)
    {

        if(ind[i] == '"')
        {
            if(!skip)
            {
                id++;
                if(!strncmp(&ind[i+1], field, strlen(field)) && (ind[i+1+strlen(field)] == '"'))
                {

                    return id;
                }
                else
                {
                    /* skip the next " */
                    skip = TRUE;

                }
            }
            else
            {
                skip = FALSE;

            }

        }

    }
    return 0;

}

/*******************************************************************************
**
** Function         btapp_hs_decode_indicator_string
**
** Description      process the indicator string and sets the indicator ids
**
** Returns          void
*******************************************************************************/
static void btapp_hs_decode_indicator_string(UINT16 handle, char * ind)
{
    UINT16 cb_index = handle - 1;

    btapp_hsc_cb.conn_cb[cb_index].call_ind_id = btapp_hs_find_indicator_id(ind, "call");
    btapp_hsc_cb.conn_cb[cb_index].call_setup_ind_id = btapp_hs_find_indicator_id(ind, "callsetup");
    if(!btapp_hsc_cb.conn_cb[cb_index].call_setup_ind_id)
    {
        btapp_hsc_cb.conn_cb[cb_index].call_setup_ind_id = btapp_hs_find_indicator_id(ind, "call_setup");
    }
    btapp_hsc_cb.conn_cb[cb_index].service_ind_id = btapp_hs_find_indicator_id(ind, "service");
    btapp_hsc_cb.conn_cb[cb_index].battery_ind_id = btapp_hs_find_indicator_id(ind, "battchg");
    btapp_hsc_cb.conn_cb[cb_index].callheld_ind_id = btapp_hs_find_indicator_id(ind, "callheld");
    btapp_hsc_cb.conn_cb[cb_index].signal_strength_ind_id = btapp_hs_find_indicator_id(ind, "signal");
    btapp_hsc_cb.conn_cb[cb_index].roam_ind_id = btapp_hs_find_indicator_id(ind, "roam");
    btapp_hsc_cb.conn_cb[cb_index].bearer_ind_id = btapp_hs_find_indicator_id(ind, "bearer");
}

/*******************************************************************************
**
** Function         btapp_hs_set_indicator_status
**
** Description      sets the current indicator
**
** Returns          void
*******************************************************************************/
static void btapp_hs_set_initial_indicator_status(UINT16 handle, char * ind)
{
    UINT8 i, pos;
    UINT16 cb_index = handle - 1;

    /* Clear all indicators. Not all indicators will be initialized */
    btapp_hsc_cb.conn_cb[cb_index].curr_call_ind = 0;
    btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind = 0;
    btapp_hsc_cb.conn_cb[cb_index].curr_service_ind = 0;
    btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind = 0;
    btapp_hsc_cb.conn_cb[cb_index].curr_signal_strength_ind = 0;
    btapp_hsc_cb.conn_cb[cb_index].curr_roam_ind = 0;
    btapp_hsc_cb.conn_cb[cb_index].curr_battery_ind = 0;
    btapp_hsc_cb.conn_cb[cb_index].curr_bearer_ind = 0;

    /* skip any spaces in the front */
    while ( *ind == ' ' ) ind++;

    /* get "call" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].call_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_call_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "callsetup" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].call_setup_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "service" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].service_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_service_ind = ind[i] - '0';
            /* if there is no service play the designated tone */
            if(!btapp_hsc_cb.conn_cb[cb_index].curr_service_ind)
            {
                /*
                if(HS_CFG_BEEP_NO_NETWORK)
                {
                    UTL_BeepPlay(HS_CFG_BEEP_NO_NETWORK);
                }*/
            }
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "callheld" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].callheld_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "signal" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].signal_strength_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_signal_strength_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "roam" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].roam_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_roam_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "battchg" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].battery_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_battery_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "bearer" indicator*/
    pos = btapp_hsc_cb.conn_cb[cb_index].bearer_ind_id - 1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            btapp_hsc_cb.conn_cb[cb_index].curr_bearer_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    if(btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind != 0)
    {
        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_3WAY_HELD);
    }
    else if(btapp_hsc_cb.conn_cb[cb_index].curr_call_ind == BTAPP_HS_CALL_ACTIVE)
    {
        if(btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_INCOMING)
        {
            BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
        }
        else
        {
            BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE);
        }
    }
    else if(btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_INCOMING)
    {
        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_RINGACT);
    }
    else if((btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_OUTGOING) ||
            (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_ALERTING)
           )
    {
        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);
    }

    /* Dump indicators */
    APPL_TRACE_EVENT1("Call: %s ", btapp_hs_call_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_call_ind]);
    APPL_TRACE_EVENT1("Callsetup: Ind %s ", btapp_hs_callsetup_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind]);
    APPL_TRACE_EVENT1("Service: %s", btapp_hs_service_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_service_ind]);
    APPL_TRACE_EVENT1("Signal Strength: %d", btapp_hsc_cb.conn_cb[cb_index].curr_signal_strength_ind);
    APPL_TRACE_EVENT1("Roam: %s ", btapp_hs_roam_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_roam_ind]);
    APPL_TRACE_EVENT1("Battery: %d", btapp_hsc_cb.conn_cb[cb_index].curr_battery_ind);
    APPL_TRACE_EVENT1("Hold: %s ", btapp_hs_callheld_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind]);
    APPL_TRACE_EVENT1("Bearer: %s ", btapp_hs_bearer_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_bearer_ind]);
}

/*******************************************************************************
**
** Function         btapp_hs_set_unsolicited_indicator_status
**
** Description      sets the current indicator
**
** Returns          void
*******************************************************************************/
static void btapp_hs_set_unsolicited_indicator_status(UINT16 handle, char * ind)
{

    UINT16 cb_index = handle - 1;
    UINT8 indicator;
    UINT8 indicator_pos;
    UINT8 value_pos;
    UINT8 prev_call_ind = btapp_hsc_cb.conn_cb[cb_index].curr_call_ind;
    UINT8 prev_call_setup_ind = btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind;
    UINT8 prev_service_ind = btapp_hsc_cb.conn_cb[cb_index].curr_service_ind;
#ifdef BTAPP_HS_DISC_SLC_IN_CALL
    UINT16 hdl, unused_handle;
#endif

    indicator_pos = 0;
    if(ind[indicator_pos] == ' ')
        indicator_pos++;

    indicator = ind[indicator_pos]-'0';

    if(ind[indicator_pos+1] == ',')
    {
        value_pos = indicator_pos+2;
    }
    else
    {
        /* two digits */
        indicator = indicator*10 + ind[indicator_pos+1]-'0';
        value_pos = indicator_pos+3;


    }
    if(ind[value_pos] == ' ')
    {

        value_pos++;
    }

    /* extract the standard indicators */
    if(indicator == btapp_hsc_cb.conn_cb[cb_index].call_ind_id)
    {
        btapp_hsc_cb.conn_cb[cb_index].curr_call_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Call Ind: %s ", btapp_hs_call_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_call_ind]);

        //if (btapp_hsc_cb.conn_cb[cb_index].curr_call_ind ==1)
        //{
       //     APPL_TRACE_EVENT0("audio transfer to phone");
        //    btapp_hs_audio_transfer();
       // }

#if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE)
        /* Call ended */
        if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_STR_CONNECT))
        {
            if((prev_call_ind == 1) && (btapp_hsc_cb.conn_cb[cb_index].curr_call_ind == 0))
            {
                /* play beep for call end
                UTL_BeepPlay(HS_CFG_BEEP_CALL_END);*/

                /* Need to send AVRCP play to all suspended A2DP source for MPS*/
                if (btapp_hsc_cb.is_susp_due_to_hs == TRUE && BTAPP_AK_GETSTATUS(BTAPP_AK_ST_SUSPEND))
                {
                    APPL_TRACE_EVENT0("Send Play when receiving call = 0 for MPS support");
                    btapp_avk_send_rc_cmd(BTA_AVK_RC_PLAY);
                    btapp_hsc_cb.is_susp_due_to_hs = FALSE;
                }
            }
        }
        else
        {
            APPL_TRACE_ERROR0("A2DP is not connected");
        }
#endif /* #if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE) */

        btapp_hs_get_call_list(handle, FALSE);
    }
    else if(indicator == btapp_hsc_cb.conn_cb[cb_index].call_setup_ind_id)
    {

        btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Callsetup Ind: %s", btapp_hs_callsetup_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind]);
#if 0        
        /* Don't send CLCC on incoming call to prevent answer delay */
        if((btapp_hsc_cb.conn_cb[cb_index].call_setup_ind_id != BTAPP_HS_CALLSETUP_INCOMING) &&
           (btapp_hsc_cb.conn_cb[cb_index].call_setup_ind_id != BTAPP_HS_CALLSETUP_ALERTING) &&
           (btapp_cfg.hs_outgoing_clcc)
          )
        {
          btapp_hs_get_call_list(handle, FALSE);
        }
#else

         btapp_hs_get_call_list(handle, TRUE);
#endif
#if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE)
        if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_STR_CONNECT))
        {
            /* callsetup change from 0 to 1, incoming call setup */
            if ((prev_call_setup_ind == 0) && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == 1))
            {
                /* Need to send AVRCP pause to all streaming A2DP source for MPS*/
                if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_START))
                {
                    APPL_TRACE_EVENT0("Send Pause when receiving callsetup = 1 for MPS support");
                    btapp_avk_send_rc_cmd(BTA_AVK_RC_PAUSE);
                    btapp_hsc_cb.is_susp_due_to_hs = TRUE;
                }
                else
                {
                    APPL_TRACE_ERROR0("A2DP not in streaming state");
                }
            }
            /* callsetup change from 0 to 1, incoming setup */
            else if ((prev_call_setup_ind == 0) && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == 2))
            {
                /* Need to send AVRCP pause to all streaming A2DP source for MPS*/
                if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_START))
                {
                    APPL_TRACE_EVENT0("Send Pause when receiving callsetup = 2 for MPS support");
                    btapp_avk_send_rc_cmd(BTA_AVK_RC_PAUSE);
                    btapp_hsc_cb.is_susp_due_to_hs = TRUE;
                }
                else
                {
                    APPL_TRACE_ERROR0("A2DP not in streaming state");
                }
            }
            /* callsetup change from 1 or 2 to 0 and there is not active call */
            else if (((prev_call_setup_ind == 1) || (prev_call_setup_ind == 2)) && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == 0) &&
                     (btapp_hsc_cb.conn_cb[cb_index].curr_call_ind == 0))
            {
                /* Need to send AVRCP play to all suspended A2DP source for MPS */
                if (btapp_hsc_cb.is_susp_due_to_hs == TRUE && BTAPP_AK_GETSTATUS(BTAPP_AK_ST_SUSPEND))
                {
                    APPL_TRACE_EVENT0("Send Play when receiving callsetup = 0 for MPS support");
                    btapp_avk_send_rc_cmd(BTA_AVK_RC_PLAY);
                    btapp_hsc_cb.is_susp_due_to_hs = FALSE;
                }
            }
        }
        else
        {
            APPL_TRACE_ERROR0("A2DP is not connected");
        }
#endif /* #if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE) */
    }
    else if(indicator == btapp_hsc_cb.conn_cb[cb_index].callheld_ind_id)
    {

        btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Hold Ind: %s ", btapp_hs_callheld_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind]);
    }
    else if(indicator == btapp_hsc_cb.conn_cb[cb_index].service_ind_id)
    {

        btapp_hsc_cb.conn_cb[cb_index].curr_service_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Service Ind: %s", btapp_hs_service_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_service_ind]);

        /* if there is no service play the designated tone */
        if(!btapp_hsc_cb.conn_cb[cb_index].curr_service_ind)
        {
            /*if(HS_CFG_BEEP_NO_NETWORK)
            {
                UTL_BeepPlay(HS_CFG_BEEP_NO_NETWORK);
            }*/
        }
        /* if phone is back in service, cancel the no service beep */
        else if(btapp_hsc_cb.conn_cb[cb_index].curr_service_ind && !prev_service_ind)
        {
            /*
            if(HS_CFG_BEEP_NO_NETWORK)
            {
                UTL_BeepCancel(HS_CFG_BEEP_NO_NETWORK->beep_id);
            }
            UTL_BeepPlay(HS_CFG_BEEP_NETWORK_AVAIL);
            headset_ui_set_led_pattern(BTAPP_LED_PATTERN_NETWORK_AVAILABLE);*/
        }

    }
    else if(indicator == btapp_hsc_cb.conn_cb[cb_index].roam_ind_id)
    {
        btapp_hsc_cb.conn_cb[cb_index].curr_roam_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Roam Ind: %s ", btapp_hs_roam_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_roam_ind]);
        if(btapp_hsc_cb.conn_cb[cb_index].curr_roam_ind)
        {
            /* TODO: play beep for roaming */
        }
        else
        {
            /* TODO: play beep for in network status */
        }
    }
    else if(indicator == btapp_hsc_cb.conn_cb[cb_index].battery_ind_id)
    {
        btapp_hsc_cb.conn_cb[cb_index].curr_battery_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Battery Ind: %d ", btapp_hsc_cb.conn_cb[cb_index].curr_battery_ind);
    }
    else if(indicator == btapp_hsc_cb.conn_cb[cb_index].signal_strength_ind_id)
    {
        btapp_hsc_cb.conn_cb[cb_index].curr_signal_strength_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Signal Strength Ind: %d ", btapp_hsc_cb.conn_cb[cb_index].curr_signal_strength_ind);
    }
    else if(indicator == btapp_hsc_cb.conn_cb[cb_index].bearer_ind_id)
    {
        btapp_hsc_cb.conn_cb[cb_index].curr_bearer_ind = ind[value_pos] - '0';
        APPL_TRACE_EVENT1("Bearer Ind: %s ", btapp_hs_bearer_ind_name[btapp_hsc_cb.conn_cb[cb_index].curr_bearer_ind]);
    }

    if(btapp_hsc_cb.conn_cb[cb_index].curr_callheld_ind)
        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_3WAY_HELD);
    else
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_3WAY_HELD);

    /* set some status for UI */
    if(btapp_hsc_cb.conn_cb[cb_index].curr_call_ind == BTAPP_HS_CALL_ACTIVE)
    {
        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE);

        /* Some old HFP 0.96 phones do not use callsetup indication.
           If this is the case, manually disable ring bit */
        if((btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_NONE) &&
            BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_RINGACT))
        {
          BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_RINGACT);
        }

        /* some phones don't send callsetup done after remote party answer.
        Manually disable outgoing call bit */
        if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL))
        {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);
            if((btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_OUTGOING) ||
               (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_ALERTING)
              )
               btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind = BTAPP_HS_CALLSETUP_NONE;
        }
    }
    else
    {

        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE);

        /* Reset mute state after call hangup */
        if (btapp_hsc_cb.is_muted)
        {
            /* headset_ui_hs_unmute(UTL_BTN_SHORT_EVT);          */
        }

        /* Disable Callwait if set */
        if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_WAITCALL))
        {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
        }

        /* some phones don't send callsetup done if outgoing call is cancelled
        Manually disable outgoing call bit */
        if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL))
        {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);
        }
    }


    if((prev_call_setup_ind == 0)
        && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind ==BTAPP_HS_CALLSETUP_INCOMING))
    {
        /* Do not go to RING state if a call already exists */
        if(!BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE))
        {
             BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_RINGACT);
             btapp_hsc_cb.conn_cb[cb_index].call_state = BTAPP_HS_CALL_INC;
        }

          /* In band ringing started, mute speaker if feature is set */
        if(btapp_hsc_cb.mute_inband_ring && BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_SCOOPEN))
        {
           APPL_TRACE_EVENT0 ("Mute Inband Ringing");
           /* TODO: mute speaker */
        }
    }

    if((prev_call_setup_ind == BTAPP_HS_CALLSETUP_INCOMING)
        && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind !=BTAPP_HS_CALLSETUP_INCOMING))
    {

         BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_RINGACT);

        /* Disable Callwait if set */
        if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_WAITCALL))
        {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
        }

         /* In band ringing ended, un-mute speaker if feature is set */
         if(btapp_hsc_cb.mute_inband_ring)
         {
            APPL_TRACE_EVENT0("Un-Muting Speaker");
            /* TODO: mute speaker */
         }
    }

    if((btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_OUTGOING)
        ||(btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind == BTAPP_HS_CALLSETUP_ALERTING))
    {
        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);
        /* TODO: play beep for outgoing call */
        btapp_hs_set_last_dialed(handle);
    }
    else
    {
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);
    }

    if((prev_call_setup_ind == 0)
        && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind !=0)
       )
    {
#ifdef BTAPP_HS_DISC_SLC_IN_CALL
         /* Temporary close all other connections */
         /* TODO: Make this configurable in CGS */
         for(unused_handle = 1; unused_handle <= BTAPP_HS_MAX_NUM_CONN; unused_handle++)
         {
            if((unused_handle != handle) && (btapp_hsc_cb.conn_cb[unused_handle-1].connection_active))
            {
                APPL_TRACE_EVENT1("Closing all other connections: Handle %d", unused_handle);
                btapp_hs_close(unused_handle);
                BTAPP_HS_SETSTATUS(unused_handle, BTAPP_HS_ST_CONN_SUSPENDED);
            }
         }
#endif /* BTAPP_HS_DISC_SLC_IN_CALL */

    }

    if((btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind ==0)
         && (btapp_hsc_cb.conn_cb[cb_index].curr_call_ind==0)
       )
    {

#ifdef BTAPP_HS_DISC_SLC_IN_CALL
        for(hdl = 1; hdl <= BTAPP_HS_MAX_NUM_CONN; hdl++)
        {
            if(BTAPP_HS_GETSTATUS(hdl, BTAPP_HS_ST_CONN_SUSPENDED))
            {
                APPL_TRACE_EVENT1("Re-open Connection with handle %d", hdl);
                BTAPP_HS_RESETSTATUS(hdl, BTAPP_HS_ST_CONN_SUSPENDED);
                btapp_hs_open_handle(hdl);
            }
        }
#endif
    }
    /* for missed calls etc we need to restart if source do not restart */
    else if( (prev_call_setup_ind != 0) && (btapp_hsc_cb.conn_cb[cb_index].curr_call_setup_ind ==0)
         && (btapp_hsc_cb.conn_cb[cb_index].curr_call_ind==0)
           )
    {
#ifdef BTAPP_HS_DISC_SLC_IN_CALL
        for(hdl = 1; hdl <= BTAPP_HS_MAX_NUM_CONN; hdl++)
        {
            if(BTAPP_HS_GETSTATUS(hdl, BTAPP_HS_ST_CONN_SUSPENDED))
            {
                APPL_TRACE_EVENT1("Re-open Connection with handle %d", hdl);
                BTAPP_HS_RESETSTATUS(hdl, BTAPP_HS_ST_CONN_SUSPENDED);
                btapp_hs_open_handle(hdl);
            }
        }
#endif
    }
}

/*******************************************************************************
**
** Function         btapp_hs_get_call_list
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_hs_get_call_list(UINT16 handle, BOOLEAN force)
{
    UINT16 cb_index = handle - 1;

    if(btapp_hsc_cb.conn_cb[cb_index].clcc_supported || force)
    {
        memset(btapp_hsc_cb.conn_cb[cb_index].call_list_info, 0, BTAPP_HS_CL_BUF_SIZE * BTAPP_HS_MAX_CL_IDX);
        BTA_HsCommand(handle, BTA_HS_CLCC_CMD, NULL);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_parse_call_list
**
** Description
**
** Returns          void
*******************************************************************************/
static void btapp_hs_parse_call_list(UINT16 handle, char * ind)
{
    UINT16 cb_index = handle - 1;
    UINT8 indicator_pos = 0;
    UINT8 offset;
    UINT8 call_idx = 0;

    /* skip spaces */
    if (ind[indicator_pos] == ' ')
        indicator_pos++;

    for(offset = IDX; offset <= NUMBER; offset++)
    {
       if(offset == NUMBER)
       {
            UINT8 i = 0;

            if(ind[indicator_pos] == '"')
                indicator_pos++;

            while((ind[indicator_pos] != '"') &&(ind[indicator_pos] != '\0'))
               btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][offset+(i++)] = ind[indicator_pos++];

            btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][offset+i] = '\0';
       }
       else
       {
            if(offset == IDX)
                call_idx = ind[indicator_pos] - '0' - 1;
            if (call_idx < BTAPP_HS_MAX_CL_IDX)
                btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][offset] = ind[indicator_pos++] - '0';
       }

       /* skip commas */
       if (ind[indicator_pos] == ',')
           indicator_pos++;
    }
}

/*******************************************************************************
**
** Function         btapp_hs_process_call_list
**
** Description
**
** Returns          void
*******************************************************************************/
static void btapp_hs_process_call_list(UINT16 handle)
{

#define ACTIVE   0
#define HELD     1
#define DIALING  2
#define ALERTING 3
#define INCOMING 4
#define WAITING  5

    UINT8 call_idx;
    UINT8 num_calls = 0;
    BOOLEAN call_wait = FALSE;
    UINT16 cb_index = handle - 1;

#if (defined(BTA_AG_INCLUDED)&& BTA_AG_INCLUDED==TRUE)
    tBTA_AG_RES_DATA  ag_data;
#endif

    for(call_idx = 0; call_idx < BTAPP_HS_MAX_CL_IDX; call_idx++)
    {
        if(btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][IDX] > 0)
        {
            num_calls++;
            switch(btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][STATUS])
            {
                case ACTIVE:
                    APPL_TRACE_EVENT2("Active (%s) %s", btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][HSDIR] ? "INCOMING":"OUTGOING",
                                      &btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][NUMBER]);
					gHFstate = 3;
                    BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE);

#if (defined(BTA_AG_INCLUDED)&& BTA_AG_INCLUDED==TRUE)
                    BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_IN_CALL_CONN_RES, NULL);
#endif
                    break;

                case HELD:
                    APPL_TRACE_EVENT1("%s ON-HOLD", &btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][NUMBER]);
                    BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_3WAY_HELD);
                    break;

                case DIALING:
                    BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);
                case ALERTING:
                    APPL_TRACE_EVENT1("DIALING/ALERTING %s", &btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][NUMBER]);
                    BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_SCALLSETUP);

                    break;

                case INCOMING:
                    APPL_TRACE_EVENT1("Incoming Call from %s", &btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][NUMBER]);
                    BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_SCALLSETUP);
                    BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);

#if (defined(BTA_AG_INCLUDED)&& BTA_AG_INCLUDED==TRUE)
                    strncpy(ag_data.str, &btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][NUMBER], BTA_AG_AT_MAX_LEN);
                    BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_IN_CALL_RES, &ag_data);
#endif
					gHFstate = 1;
                    break;

                case WAITING:
                    APPL_TRACE_EVENT1("Call-Wait for %s", &btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][NUMBER]);
                    BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
                    call_wait = TRUE;
                    break;
            }

            APPL_TRACE_DEBUG1("Call Index %d", btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][IDX]);
            APPL_TRACE_DEBUG1("Dir %s", btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][HSDIR] ? "INCOMING":"OUTGOING");
            APPL_TRACE_DEBUG1("Mode %s", btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][MODE] ? "DATA":"VOICE");
            APPL_TRACE_DEBUG1("Multiple Party %s", btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][MPTY] ? "CONFERENCE":"NONE");
            APPL_TRACE_DEBUG1("Number %s", &btapp_hsc_cb.conn_cb[cb_index].call_list_info[call_idx][NUMBER]);
        }
    }

    /* Do state clean up */
    if(num_calls == 0)
    {
       APPL_TRACE_EVENT0("No Active or Held calls");
	   gHFstate = 2;
	   memset(gCallName, 0, 64);
       if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE))
       {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE);
       }
       else if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_RINGACT))
       {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_RINGACT);
       }
       else if(BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL))
       {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_OUTGOINGCALL);
       }

#if (defined(BTA_AG_INCLUDED)&& BTA_AG_INCLUDED==TRUE)
        BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_END_CALL_RES, NULL);
#endif
    }

    if(!call_wait && (BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_WAITCALL)))
    {
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
    }
}

/*******************************************************************************
**
** Function         btapp_hs_process_ok_evt
**
** Description
**
** Returns          void
*******************************************************************************/
static void btapp_hs_process_ok_evt(UINT16 handle, UINT8 cmd_event)
{
    UINT16 cb_index = handle - 1;

    if (cmd_event < BTA_HS_MAX_CMD)
    {
        APPL_TRACE_DEBUG1("OK Received for AT CMD Event %s", btapp_hs_cmd_name[cmd_event]);
    }
    else
    {
        APPL_TRACE_ERROR1("btapp_hs_process_ok_evt : undefined cmd_event %d", cmd_event);
        return;
    }

    switch(cmd_event)
    {
        case BTA_HS_CLCC_CMD:
            btapp_hs_process_call_list(handle);
            if(btapp_hsc_cb.conn_cb[cb_index].clcc_supported == FALSE)
                btapp_hsc_cb.conn_cb[cb_index].clcc_supported = TRUE;
            break;

        default:
            break;
    }
}

/*******************************************************************************
**
** Function         btapp_hs_process_error_evt
**
** Description
**
** Returns          void
*******************************************************************************/
static void btapp_hs_process_error_evt(UINT16 handle, UINT8 cmd_event, UINT8 err_code)
{
    UINT16 cb_index = handle - 1;

    if (cmd_event < BTA_HS_MAX_CMD)
    {
        if (err_code == 0)
        {
            APPL_TRACE_WARNING1("AT Cmd Event %s Not supported by peer", btapp_hs_cmd_name[cmd_event]);
        }
        else
        {
            APPL_TRACE_WARNING2("AT cmd error: cmd = %s, err_code = %d", btapp_hs_cmd_name[cmd_event], err_code);
        }
    }
    else
    {
        APPL_TRACE_ERROR1("btapp_hs_process_error_evt : undefined cmd_event %d", cmd_event);
        return;
    }

    if (cmd_event == BTA_HS_CLCC_CMD)
        btapp_hsc_cb.conn_cb[cb_index].clcc_supported = FALSE;
}

/*******************************************************************************
**
** Function         btapp_hs_set_ag_authorized
**
** Description      Bond with the device
**
**
** Returns          void
*******************************************************************************/
void btapp_hs_set_ag_authorized (tBTAPP_REM_DEVICE * p_device_rec)
{
    /* update BTA with this information.If a device is set as trusted, BTA will
    not ask application for authorization, if authorization is required for
    AG connections */

    p_device_rec->is_trusted = TRUE;
    p_device_rec->trusted_mask |= (BTA_HSP_HS_SERVICE_MASK |BTA_HFP_HS_SERVICE_MASK);
    btapp_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);
}

/*******************************************************************************
**
** Function         btapp_hs_save_device
**
** Description      mark the HS service mask, so this device can connect to the
**                  AG again
**
** Returns          void
*******************************************************************************/
void btapp_hs_save_device(BD_ADDR_PTR p_addr, tBTAPP_REM_DEVICE * p_device_rec,
                          tBTA_SERVICE_MASK service_mask)
{
    tBTAPP_REM_DEVICE device_rec;

    if(p_device_rec)
    {
        p_device_rec->services |= service_mask;
    }
    else
    {
        memset(&device_rec, 0, sizeof(device_rec));
        device_rec.trusted_mask  = 0;
        device_rec.name[0] = '\0';
        bdcpy(device_rec.bd_addr, p_addr);
        device_rec.is_trusted = TRUE;
        device_rec.services = service_mask;
        p_device_rec = &device_rec;
    }


    btapp_store_device(p_device_rec);
}


/*******************************************************************************
**
** Function         btapp_hs_cback
**
** Description      callback from BTA HS
**
** Returns          void
*******************************************************************************/
static void btapp_hs_cback (tBTA_HS_EVT event, tBTA_HS *p_data)
{
    UINT16 handle = 0, mmi_evt = 0;
    UINT16 cb_index = 0;


    if(p_data)
    {
        handle = p_data->hdr.handle;
        cb_index = handle -1;
    }

    APPL_TRACE_EVENT3("btapp_hs_cback event:%d %s for handle %d", event,
                       btapp_hs_event_name[event], handle);

    switch(event)
    {
    case BTA_HS_ENABLE_EVT:
        //It should put some necessary HS enable event handler code here
        btapp_hsc_cb.enabled = TRUE;
        break;

    case BTA_HS_DEREGISTER_EVT:
    case BTA_HS_DISABLE_EVT:

        btapp_hsc_cb.enabled = FALSE;
        break;

    case BTA_HS_REGISTER_EVT:

        break;

    case BTA_HS_OPEN_EVT:
        if(p_data->open.status == BTA_HS_SUCCESS)
        {
            bdcpy(btapp_hsc_cb.conn_cb[cb_index].connected_bd_addr, p_data->open.bd_addr);
            btapp_hsc_cb.conn_cb[cb_index].connection_active = TRUE;
            btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id = p_data->open.service;
            btapp_hsc_cb.conn_cb[cb_index].indicator_string_received = FALSE;

            btapp_hsc_cb.conn_cb[cb_index].status = BTAPP_HS_ST_CONNECT;
            /* save the device */
            bdcpy(btapp_hs_db.las_cnt_bda, p_data->open.bd_addr);
            btapp_nv_store_hs_db();
            btapp_hs_save_device(p_data->open.bd_addr, btapp_get_device_record(p_data->open.bd_addr), p_data->open.service);
            
#if( defined BTA_PBC_INCLUDED ) && (BTA_PBC_INCLUDED == TRUE)
            BTA_PbcOpen(p_data->open.bd_addr, 0);
#endif
        }

        break;

    case BTA_HS_CLOSE_EVT:

        btapp_hsc_cb.conn_cb[cb_index].connection_active = FALSE;
        btapp_hsc_cb.conn_cb[cb_index].open_audio_asap = FALSE;

        /* reserve suspend status flag */
        btapp_hsc_cb.conn_cb[cb_index].status &= BTAPP_HS_ST_CONN_SUSPENDED;
        btapp_hsc_cb.conn_cb[cb_index].status |= BTAPP_HS_ST_CONNECTABLE;

        break;

    case BTA_HS_CONN_EVT:

        btapp_hsc_cb.conn_cb[cb_index].peer_feature = p_data->conn.peer_features;

        /* disable ECNR on phone if we support it */
        if((btapp_hsc_cb.conn_cb[cb_index].peer_feature & BTA_HS_PEER_FEAT_ECNR) &&
            (btapp_cfg.hs_features & BTA_HS_FEAT_ECNR))
        {
            BTA_HsCommand(handle, BTA_HS_NREC_CMD, NULL);
        }
        /* Get Call List if AG and HF support this feature */
        if(btapp_cfg.hs_features & BTA_HS_FEAT_ECS)
            btapp_hs_get_call_list(handle, TRUE);
        APPL_TRACE_EVENT1("HF Initialization complete: peer_feature = 0x%x", p_data->conn.peer_features);

        {
#ifdef BTAPP_HS_DISC_SLC_IN_CALL
            UINT16 hdl;
            for(hdl = 1; hdl <= BTAPP_HS_MAX_NUM_CONN; hdl++)
            {
                if(BTAPP_HS_GETSTATUS(hdl, BTAPP_HS_ST_IN_CALL))
                {
                    UINT16 unused_handle;
                    for(unused_handle = 1; unused_handle <= BTAPP_HS_MAX_NUM_CONN; unused_handle++)
                    {
                        if((unused_handle != hdl) &&
                            (btapp_hsc_cb.conn_cb[unused_handle-1].connection_active))
                        {
                            APPL_TRACE_EVENT1("Closing connection: Handle %d", unused_handle);
                            btapp_hs_close(unused_handle);
                            BTAPP_HS_SETSTATUS(unused_handle, BTAPP_HS_ST_CONN_SUSPENDED);
                        }
                    }
                    break;
                }
            }
#endif
        }

        /* Auto answer if phone still ringing on this handle */
        if (BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_RINGACT) && btapp_cfg.hs_slc_auto_answer)
        {
           btapp_hs_send_command(handle, BTA_HS_A_CMD, NULL);
        }
        else if(btapp_hsc_cb.conn_cb[cb_index].open_audio_asap)
        {
            /* Open Audio as soon as SLC opens up. */
            BTA_HsAudioOpen(handle);
            btapp_hsc_cb.conn_cb[cb_index].open_audio_asap = FALSE;
        }
        break;

    case BTA_HS_CONN_LOSS_EVT:

        btapp_hsc_cb.conn_cb[cb_index].connection_active = FALSE;
        btapp_hsc_cb.conn_cb[cb_index].open_audio_asap = FALSE;
        btapp_hsc_cb.conn_cb[cb_index].status = BTAPP_HS_ST_CONNECTABLE;
        break;


    case BTA_HS_AUDIO_OPEN_EVT:

        /* conn_cb.call_state = BTAPP_HS_CALL_CONN; */
        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_SCOOPEN);
        if (BTA_HFP_HS_SERVICE_ID != btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id)
        {
#ifdef BTAPP_HS_DISC_SLC_IN_CALL
        UINT16 unused_handle;
         /* Temporary close all other connections */
         /* TODO: Make this configurable in CGS */
         for(unused_handle = 1; unused_handle <= BTAPP_HS_MAX_NUM_CONN; unused_handle++)
         {
            if(unused_handle != handle)
            {
                APPL_TRACE_EVENT1("Closing all other connections: Handle %d", unused_handle);
                btapp_hs_close(unused_handle);
                BTAPP_HS_SETSTATUS(unused_handle, BTAPP_HS_ST_CONN_SUSPENDED);
            }
         }
#endif /*BTAPP_HS_DISC_SLC_IN_CALL */
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_RINGACT);
        }
        else if(btapp_hsc_cb.mute_inband_ring && BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_RINGACT))
        {
            APPL_TRACE_EVENT0 ("Mute Inband Ringing");
            /* TODO: mute speaker */
        }
#if (BTM_WBS_INCLUDED == TRUE )
        APPL_TRACE_EVENT3 ("%s neg_codec_type:%d, handle:%d", __FUNCTION__, btapp_hsc_cb.conn_cb[handle-1].neg_codec_type, handle);
        if(btapp_hsc_cb.conn_cb[handle-1].neg_codec_type == BTM_SCO_CODEC_MSBC)
        {//WBS
            //It should set the hf codec for WBS mode setting
        }
        else if(btapp_hsc_cb.conn_cb[handle-1].neg_codec_type == BTM_SCO_CODEC_CVSD)
        {//NBS
           
        }
#endif

        //if(!test_task_enable)
        //{
        //    pthread_attr_t attr;
        //    pthread_attr_init(&attr);
        //    pthread_create(&test_task, &attr, test_task_main, NULL );
        //}
#if( defined BTA_AG_INCLUDED ) && (BTA_AG_INCLUDED == TRUE)
        if(!test_task_enable)
        {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&test_task, &attr, (void*)ag_audio_open_delay_cb, NULL );
        }
#endif
        break;

    case BTA_HS_AUDIO_CLOSE_EVT:

        btapp_hsc_cb.conn_cb[cb_index].call_state = BTAPP_HS_CALL_NONE;
        BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_SCOOPEN);

        /* Get call list and clean up state if necessary */
        btapp_hs_get_call_list(handle, FALSE);
#ifdef BTAPP_HS_DISC_SLC_IN_CALL
        if(BTA_HFP_HS_SERVICE_ID != btapp_hsc_cb.conn_cb[cb_index].connected_hs_service_id)
        {
            UINT16 suspended_hdl;
            for(suspended_hdl = 1; suspended_hdl <= BTAPP_HS_MAX_NUM_CONN; suspended_hdl++)
            {
                if(BTAPP_HS_GETSTATUS(suspended_hdl, BTAPP_HS_ST_CONN_SUSPENDED))
                {
                    APPL_TRACE_EVENT1("Re-open Connection with handle %d", suspended_hdl);
                    BTAPP_HS_RESETSTATUS(suspended_hdl, BTAPP_HS_ST_CONN_SUSPENDED);
                    btapp_hs_open_handle(suspended_hdl);
                }
            }
        }
#endif /*BTAPP_HS_DISC_SLC_IN_CALL */

#if( defined BTA_AG_INCLUDED ) && (BTA_AG_INCLUDED == TRUE)
        BTA_AgAudioClose(btapp_ag_open_handle());
#endif  

        break;

    case BTA_HS_AUDIO_OPEN_REQ_EVT:
        APPL_TRACE_EVENT1("btapp_hs_cback reject_sco:%d", btapp_hsc_cb.conn_cb[cb_index].reject_sco);
#if 1
        if (btapp_hsc_cb.conn_cb[cb_index].reject_sco)
            BTA_HsAudioOpenResp(p_data->hdr.handle, FALSE);     /* Reject SCO */
        else
            BTA_HsAudioOpenResp(p_data->hdr.handle, TRUE);      /* Accept SCO */
        break;
#else
        BTA_HsAudioOpenResp(p_data->hdr.handle, FALSE);     /* Reject SCO */
        break;
#endif
    case BTA_HS_CIND_EVT:

        if(btapp_hsc_cb.conn_cb[cb_index].indicator_string_received)
        {
            btapp_hs_set_initial_indicator_status(handle, p_data->val.str);
        }
        else
        {
            btapp_hsc_cb.conn_cb[cb_index].indicator_string_received = TRUE;
            btapp_hs_decode_indicator_string(handle, p_data->val.str);
        }
        break;

    case BTA_HS_CIEV_EVT:

        btapp_hs_set_unsolicited_indicator_status(handle, p_data->val.str);

        break;

    case BTA_HS_RING_EVT:
        /* Do not go to RING state if a call already exists */
        if(!BTAPP_HS_GETSTATUS(handle, BTAPP_HS_ST_CALLACTIVE) ||
            btapp_hsc_cb.ring_handle != 0)
        {
            btapp_hsc_cb.conn_cb[cb_index].call_state = BTAPP_HS_CALL_INC;
            BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_RINGACT);
        }
        break;

   case BTA_HS_BUSY_EVT:
       APPL_TRACE_EVENT0("Call busy received");
       break;

    case BTA_HS_CLIP_EVT:
        APPL_TRACE_EVENT1("BTA_HS_CLIP_EVT: %s",p_data->val.str);
		strcpy(gCallNum, p_data->val.str);
		UINT8 *trim = gCallNum;
		UINT16 i = 0, hash = 0;
		Node *pHashNodeCur = NULL;

		while (gCallNum[i] != '"')
			i++;
		strcpy(gCallNum, trim + i + 1);
		i = 0;
		while (gCallNum[i] != '"')
			i++;
		gCallNum[i] = '\0';

		for (i = 0; gCallNum[i] > 0; i++)
		{
			hash = gCallNum[i] + (hash << 6) + (hash << 16) - hash;
		}

		for (pHashNodeCur = gpHashNodeStart; pHashNodeCur; pHashNodeCur = pHashNodeCur->next)
		{
			if (pHashNodeCur->hash == hash)
			{
				memset(gCallName, 0, 64);
				memcpy(gCallName, pHashNodeCur->data, pHashNodeCur->dataSize);
			}
		}
		printf("incomming call from: %s\n", gCallName);
		gSleepModeBT = true;


        break;

    case BTA_HS_BSIR_EVT:

        if(p_data->val.num ==1)
        {
            BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_INBANDRINGACT);
        }
        else
        {

            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_INBANDRINGACT);
        }

        break;

    case BTA_HS_BVRA_EVT:

        if(p_data->val.num ==1)
        {
            BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_VOICEDIALACT);
        }
        else
        {
            BTAPP_HS_RESETSTATUS(handle, BTAPP_HS_ST_VOICEDIALACT);
        }
        break;


    case BTA_HS_CCWA_EVT:

        BTAPP_HS_SETSTATUS(handle, BTAPP_HS_ST_WAITCALL);
        btapp_hs_get_call_list(handle, FALSE);
        break;

    case BTA_HS_CHLD_EVT:

        break;

    case BTA_HS_VGM_EVT:

        btapp_hs_db.mic_vol = (UINT8)p_data->val.num;
        /* Synchronize volume with other connected devices */
        btapp_hs_sync_mic_vol(handle, p_data->val.num);

#if (defined(BTA_AG_INCLUDED)&& BTA_AG_INCLUDED==TRUE)
        btapp_ag_change_mic_volume(btapp_hs_db.mic_vol);
#endif
        break;

    case BTA_HS_VGS_EVT:

        btapp_hs_db.spk_vol = (UINT8)p_data->val.num;
        /* Synchronize volume with other connected devices */
        btapp_hs_sync_spk_vol(handle, p_data->val.num);

#if (defined(BTA_AG_INCLUDED)&& BTA_AG_INCLUDED==TRUE)
        btapp_ag_change_spk_volume(btapp_hs_db.spk_vol);
#endif
        break;

    case BTA_HS_CLCC_EVT:

        btapp_hs_parse_call_list(handle, p_data->val.str);
        break;

#if (BTM_WBS_INCLUDED == TRUE )
    case BTA_HS_BCS_EVT:
        APPL_TRACE_EVENT1 ("HS receives BCS evt %d", p_data->val.num);
        btapp_hs_process_bcs_evt(handle, p_data->val.num);
        break;
#endif

    case BTA_HS_OK_EVT:

        btapp_hs_process_ok_evt(handle, (UINT8)p_data->val.num);
        break;

    case BTA_HS_ERROR_EVT:
        btapp_hs_process_error_evt(handle, (UINT8)p_data->val.num, 0);
        break;
    case BTA_HS_CMEE_EVT:

        btapp_hs_process_error_evt(handle, (UINT8)p_data->val.num, (UINT8)(p_data->val.num >> 8));
        break;
#if (BTA_HFP_VERSION >= HFP_VERSION_1_7 && BTA_HFP_HF_IND_SUPPORTED == TRUE)
    case BTA_HS_BIND_EVT:
        APPL_TRACE_EVENT1 ("HS receives BIND %s", p_data->val.str);
        break;
#endif
    }
}

#endif

