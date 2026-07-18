/****************************************************************************
**
**  Name:          btapp_cfg.c
**
**  Description:   Contains btapp configuration file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

tBTAPP_CFG btapp_cfg;
UINT16 btapp_trace_layer = 0;
UINT8      btapp_user_set_log;
UINT8      btapp_user_set_no_lpo = 0;

static UINT8 btapp_default_addr[BD_ADDR_LEN]={0x43, 0x01, 0x30, 0xCC, 0xEE, 0xFF};
extern void bta_sys_set_trace_level(UINT8 level);
extern void bta_sys_event_set_trace_level(UINT8 level);
extern void btu_hci_set_trace_level(UINT8 trace_level);
extern UINT8 BTM_SetTraceLevel (UINT8 new_level);
extern UINT8 L2CA_SetTraceLevel (UINT8 new_level);
extern UINT8 PORT_SetTraceLevel (UINT8 new_level);
extern UINT8 GAP_SetTraceLevel (UINT8 new_level);
extern UINT8 SMP_SetTraceLevel (UINT8 new_level);
extern UINT8 SDP_SetTraceLevel (UINT8 new_level);
extern UINT8 GATT_SetTraceLevel (UINT8 new_level);
extern UINT8 PORT_SetTraceLevel (UINT8 new_level);
extern UINT8 AVRC_SetTraceLevel(UINT8 new_level);
extern UINT8 A2D_SetTraceLevel (UINT8 new_level);

void btapp_cfg_set_default_addr(BD_ADDR addr)
{
    if(addr != NULL)
    {
        bdcpy(btapp_default_addr, addr);
    }
    else
    {
        APPL_TRACE_ERROR0("btapp_cfg_set_addr error param!!!");
    }
}

void btapp_cfg_set_no_lpo(UINT8 enabled)
{
    btapp_user_set_no_lpo = enabled;
}

void btapp_cfg_trace_enable(UINT8 enabled)
{
    btapp_user_set_log = enabled;
    btapp_cfg.stack_trace_enable = enabled;
    btapp_trace_layer = 0;

    //Only allow APP layer and BTA,HCI layer logs to output
    btapp_trace_layer = BTAPP_TRACE_APP_MASK | BTAPP_TRACE_BTA_MASK | BTAPP_TRACE_HCI_MASK;
    btapp_cfg_trace_layer(btapp_trace_layer);
}

void btapp_cfg_trace_layer(UINT16 trace_layer_mask)
{
    bta_sys_set_trace_level((trace_layer_mask & BTAPP_TRACE_APP_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    bta_sys_event_set_trace_level((trace_layer_mask & BTAPP_TRACE_BTA_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    btu_hci_set_trace_level((trace_layer_mask & BTAPP_TRACE_HCI_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    BTM_SetTraceLevel((trace_layer_mask& BTAPP_TRACE_BTM_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    L2CA_SetTraceLevel((trace_layer_mask & BTAPP_TRACE_L2CAP_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    PORT_SetTraceLevel((trace_layer_mask & BTAPP_TRACE_RFCOMM_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    GAP_SetTraceLevel((trace_layer_mask & BTAPP_TRACE_GAP_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    SMP_SetTraceLevel((trace_layer_mask & BTAPP_TRACE_SMP_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    SDP_SetTraceLevel((trace_layer_mask & BTAPP_TRACE_SDP_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    GATT_SetTraceLevel((trace_layer_mask & BTAPP_TRACE_GATT_MASK) ? BT_TRACE_LEVEL_DEBUG : BT_TRACE_LEVEL_NONE);
    //PORT_SetTraceLevel(BT_TRACE_LEVEL_DEBUG);
    //AVRC_SetTraceLevel(BT_TRACE_LEVEL_DEBUG);
    //A2D_SetTraceLevel(BT_TRACE_LEVEL_DEBUG);
    //AVDT_SetTraceLevel(BT_TRACE_LEVEL_DEBUG);
}

void btapp_cfg_init(void)
{
    memset(&btapp_cfg, 0, sizeof(btapp_cfg));


    if(btapp_user_set_log)
        btapp_cfg.stack_trace_enable = 1;

    APPL_TRACE_API1("btapp_cfg_init: tBTAPP_CFG size:%d ", sizeof(btapp_cfg));

    strcpy(btapp_cfg.cfg_dev_name, BT_LOCAL_NAME);
    btapp_cfg.num_inq_devices = 0;

#if (BLE_INCLUDED == TRUE) && (BTM_BLE_PRIVACY_SPT == TRUE)
    btapp_cfg.privacy_enabled = TRUE;
#endif

    /* set to use no MITM by default */
    btapp_cfg.sp_loc_io_caps = BTA_IO_CAP_NONE;
    btapp_cfg.sp_auth_req = BTAPP_AUTH_REQ_NO;
    btapp_cfg.sp_auto_reply = TRUE;
#if (BLE_INCLUDED == TRUE) && (SMP_INCLUDED == TRUE)
    btapp_cfg.ble_auth_req = BTAPP_LE_AUTH_REQ_BOND_MITM;   /* require bonding but no MITM */
#endif

#if( defined BTA_HS_INCLUDED ) && ( BTA_HS_INCLUDED == TRUE )
    /* HS configuration */
    btapp_cfg.supported_services |=  BTA_HSP_HS_SERVICE_MASK | BTA_HFP_HS_SERVICE_MASK;
    btapp_cfg.hs_security = BTAPP_HS_SECURITY;
    btapp_cfg.hs_features = BTAPP_HS_FEATURES;
    btapp_cfg.hs_slc_auto_answer = TRUE;
    btapp_cfg.hs_outgoing_clcc = TRUE;
    strcpy(btapp_cfg.hshs_service_name, BTAPP_HSHS_SERVICE_NAME);
    strcpy(btapp_cfg.hfhs_service_name, BTAPP_HFHS_SERVICE_NAME);
#endif

#if( defined BTA_AG_INCLUDED ) && ( BTA_AG_INCLUDED == TRUE )
    /* ag configuration */
    btapp_cfg.supported_services |=  BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK ;
    btapp_cfg.ag_security = BTAPP_AG_SECURITY;
    btapp_cfg.ag_features = BTAPP_AG_FEATURES | BTA_AG_FEAT_NOSCO;
    memcpy(btapp_cfg.hsag_service_name, "BRCM HS", strlen("BRCM HS") + 1);
    memcpy(btapp_cfg.hfag_service_name, "BRCM HF", strlen("BRCM HF") + 1);
    btapp_cfg.ag_instances = 2;
#if (BTM_WBS_INCLUDED == TRUE )
    btapp_cfg.ag_sco_codec = BTM_SCO_CODEC_MSBC;//BTA_AG_CODEC_CVSD;
#endif

#endif

#if (defined BTA_AVK_INCLUDED) && (BTA_AVK_INCLUDED == TRUE)
    btapp_cfg.avk_included       = TRUE;
    btapp_cfg.avk_vdp_support    = FALSE;
    btapp_cfg.avk_sbc_decoder    = TRUE;
    btapp_cfg.avk_file_dump      = FALSE;
    btapp_cfg.avk_use_btc        = FALSE;
    btapp_cfg.avk_btc_i2s_rate   = 3;        /* 44.1k */
    btapp_cfg.avk_features       = (BTA_AVK_FEAT_RCCT | \
                                    BTA_AVK_FEAT_PROTECT |  \
                                    BTA_AVK_FEAT_BROWSE  |  \
                                    BTA_AVK_FEAT_VENDOR  |  \
                                    BTA_AVK_FEAT_DELAY_RPT | \
                                    BTA_AVK_FEAT_METADATA \
                                  );
    btapp_cfg.avk_security       = BTAPP_AVK_SECURITY;
#endif

#if( defined BTA_DG_INCLUDED) && (BTA_DG_INCLUDED == TRUE)
    btapp_cfg.supported_services |= BTA_SPP_SERVICE_MASK;
    btapp_cfg.sppdg_security      = BTA_SEC_NONE;
    btapp_cfg.spp_senddata_mode   = FALSE;
    btapp_cfg.spp_loopback_mode   = FALSE;
    memcpy(btapp_cfg.sppdg_service_name, "BRCM SPP", strlen("BRCM SPP") + 1);
#endif

#if( defined BTA_HD_INCLUDED) && (BTA_HD_INCLUDED == TRUE)
    btapp_cfg.hd_security         = BTA_SEC_NONE;
    memcpy(btapp_cfg.hd_service_name, "BRCM HID DEV", strlen("BRCM HID DEV") + 1);
#endif

#if( defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
    btapp_cfg.ble_included         = TRUE;
    btapp_cfg.ble_profile_included = TRUE;

    btapp_cfg.ble_scan_int         = 72;        //0.625ms per uint
    btapp_cfg.ble_scan_win         = 18;        //0.625ms per uint
    btapp_cfg.ble_scan_type        = 1;         //0 - passive scan; 1 - active scan

    btapp_cfg.ble_loc_io_caps      = BTA_IO_CAP_NONE;
    btapp_cfg.ble_max_key_size     = 16;        //Link key length
    btapp_cfg.ble_min_key_size     = 16;
    btapp_cfg.ble_auth_req         = BTA_LE_AUTH_REQ_SC_MITM_BOND;
    btapp_cfg.ble_accept_auth_enable = 1;
    btapp_cfg.ble_init_key         = BTM_LE_SEC_DEFAULT_KEY;
    btapp_cfg.ble_resp_key         = BTM_LE_SEC_DEFAULT_KEY;

#endif

#if ( defined BTA_AV_INCLUDED ) && (BTA_AV_INCLUDED == TRUE)
    /* AV configuration */
    btapp_cfg.av_included = TRUE;
#if 0
    btapp_cfg.av_features =(BTA_AV_FEAT_RCTG     | BTA_AV_FEAT_RCCT   | \
                            BTA_AV_FEAT_VENDOR | BTA_AV_FEAT_METADATA | \
                            BTA_AV_FEAT_BROWSE | BTA_AV_FEAT_PROTECT);
#else
    btapp_cfg.av_features = (BTA_AV_FEAT_VENDOR | BTA_AV_FEAT_PROTECT | BTA_AV_FEAT_NO_SCO_SSPD);
#endif
    btapp_cfg.av_security = (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT);
    btapp_cfg.av_line_speed_kbps = 229; //BTUI_AV_LINE_SPEED_KBPS;
    btapp_cfg.av_line_speed_busy = 166; //BTUI_AV_LINE_SPEED_BUSY;
    btapp_cfg.av_line_speed_swampd = 128 ;//BTUI_AV_LINE_SPEED_SWAMPD;
    btapp_cfg.av_line_speed_count = 5; //BTUI_AV_LINE_SPEED_COUNT;
    btapp_cfg.av_sampling_freq   = A2D_SBC_IE_SAMP_FREQ_44;
    btapp_cfg.av_channel_mode    = 1; //BTUI_AV_CHANNEL_MODE
    btapp_cfg.av_num_blocks      = A2D_SBC_IE_BLOCKS_16;
    btapp_cfg.av_num_subbands    = A2D_SBC_IE_SUBBAND_8;
    btapp_cfg.av_allocation_mode = A2D_SBC_IE_ALLOC_MD_L;
    btapp_cfg.av_max_bitpool     = A2D_SBC_IE_MAX_BITPOOL;
#if (BTUI_AV_M12_SUPORT == TRUE)
    btapp_cfg.av_mp3_ch_mode     = A2D_M12_IE_CH_MD_JOINT;
    btapp_cfg.av_mp3_samp_freq   = A2D_M12_IE_SAMP_FREQ_32;
    btapp_cfg.av_mp3_bitrate     = A2D_M12_IE_BITRATE_11;
#endif
    strcpy(btapp_cfg.av_audio_service_name, "Advanced audio source");
    strcpy(btapp_cfg.av_video_service_name, "Advanced video source");
    //btapp_cfg.av_services_used = (BTA_VDP_SERVICE_MASK | BTA_A2DP_SERVICE_MASK | BTA_AVRCP_SERVICE_MASK);
    btapp_cfg.av_services_used = (BTA_A2DP_SERVICE_MASK);
    btapp_cfg.av_vdp_support= FALSE;
    btapp_cfg.av_num_audio = 2;
    btapp_cfg.av_min_trace  = 0;
    btapp_cfg.av_use_player = TRUE; //if set FALSE,read .wav & .md from File
    btapp_cfg.av_suspd_rcfg = TRUE;
    btapp_cfg.av_repeat_list = TRUE;
    btapp_cfg.use_avrc = FALSE; //TODO. need fix it.
    btapp_cfg.av_reconn_rej = TRUE;
#endif

    memcpy(btapp_cfg.set_local_addr, btapp_default_addr, sizeof(btapp_default_addr));

    if(btapp_user_set_no_lpo != 0)
    {
        btapp_cfg.bt_controller_without_lpo = 1;
    }

    APPL_TRACE_API1("Local Addr:%s", utl_bd_addr_to_string(btapp_cfg.set_local_addr));
}

tBTA_STATUS btapp_cfg_set_io_caps(tBTA_IO_CAP io_cap)
{
    tBTA_STATUS st = BTA_SUCCESS;

    if(io_cap > BTA_IO_CAP_KBDISP)
    {
        st = BTA_FAILURE;
    }

    btapp_cfg.ble_loc_io_caps = io_cap;

    return st;
}

tBTA_STATUS btapp_cfg_set_auth_req(tBTA_AUTH_REQ auth_req)
{
    tBTA_STATUS st = BTA_SUCCESS;

    btapp_cfg.ble_auth_req = auth_req;

    bta_dm_co_ble_set_auth_req(auth_req);

    return st;
}

tBTA_STATUS btapp_cfg_set_accept_auth_enable(UINT8 accept_auth_enable)
{
    tBTA_STATUS st = BTA_SUCCESS;

    btapp_cfg.ble_accept_auth_enable = accept_auth_enable;
    bta_dm_co_ble_set_accept_auth_enable(accept_auth_enable);

    return st;
}

tBTA_STATUS btapp_cfg_set_init_key(UINT8 init_key)
{
    tBTA_STATUS st = BTA_SUCCESS;

    btapp_cfg.ble_init_key = init_key;

    return st;
}

tBTA_STATUS btapp_cfg_set_rsp_key(UINT8 rsp_key)
{
    tBTA_STATUS st = BTA_SUCCESS;

    btapp_cfg.ble_resp_key = rsp_key;

    return st;
}

tBTA_STATUS btapp_cfg_set_max_key_size(UINT8 key_size)
{
    tBTA_STATUS st = BTA_SUCCESS;

    if(key_size >=  BTM_BLE_MIN_KEY_SIZE && key_size <= BTM_BLE_MAX_KEY_SIZE)
    {
        btapp_cfg.ble_max_key_size = key_size;
    }
    else
    {
        st = BTA_FAILURE;
    }

    return st;
}

tBTA_STATUS btapp_cfg_set_min_key_size(UINT8 key_size)
{
    tBTA_STATUS st = BTA_SUCCESS;

    if(key_size >=  BTM_BLE_MIN_KEY_SIZE && key_size <= BTM_BLE_MAX_KEY_SIZE)
    {
        btapp_cfg.ble_min_key_size = key_size;
    }
    else
    {
        st = BTA_FAILURE;
    }

    return st;
}

