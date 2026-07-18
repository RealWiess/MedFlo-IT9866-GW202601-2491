/****************************************************************************
**
**  Name:          btapp_avk.c
**
**  Description:   Contains bluetooth application functions for advanced audio
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "btapp_nv.h"
#if (defined(BTA_AV_INCLUDED)&& BTA_AV_INCLUDED==TRUE)
#include "btapp_av.h"
#endif

#if( defined BTA_AVK_INCLUDED ) && ( BTA_AVK_INCLUDED == TRUE )
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "bta_platform.h"
#include "bte_glue.h"

#define BTAPP_AVK_RC_CONSOLE_INCLUDE FALSE

#if ( (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE) && \
      (defined BRCM_LPST_INCLUDED) && (BRCM_LPST_INCLUDED == TRUE))
#include "btm_lpst_api.h"

#define BTAPP_AVK_APP_EVENT     (BTM_LPST_APP_OPCODE_BASE + 0x01)
#define BTAPP_AVK_APP_SUBCODE_OPEN       0x01
#define BTAPP_AVK_APP_SUBCODE_START      0x02
#define BTAPP_AVK_APP_SUBCODE_STOP       0x03
#define BTAPP_AVK_APP_SUBCODE_CLOSE      0x04
static BOOLEAN btapp_avk_lpst_cmd_handler(UINT8 op_code, BT_HDR *p_buf);
static void btapp_avk_lpst_event_handler (UINT8 event, tBTM_LPST_EVENT_DATA *p_data);
static BOOLEAN btapp_avk_lpst_forward_event(UINT8 op_code, BD_ADDR bda);

BD_ADDR dummy_bda = {0};
#endif

#define BTAPP_AVK_RC_RETRY      2  /* Number of retry opening RC if RC fail to open */

/* BTAPP AK main control block */
tBTAPP_AK_CB btapp_ak_cb;
UINT8 vol_tran_label=0;
int g_BTConnectStatus = 0;
bool gTrackChange_Title = false;
bool gTrackChange_Artist = false;
UINT8 gTrackTitle[512];
UINT8 gTrackArtist[512];
int gPlayStatus = 2;//paused


static void btapp_avk_save_device (BD_ADDR_PTR p_addr, tBTAPP_REM_DEVICE * p_device_rec);
#if (BTU_BTC_SNK_INCLUDED == TRUE)
static void btapp_avk_stack_sw_cback (tBTA_DM_SWITCH_EVT evt, tBTA_STATUS status);
static void btapp_avk_route_audio(tBTA_AVK_EVT evt);
static void btapp_avk_audio_route_cback (tBTA_RT_EVT event, tBTA_RT *p_data);
#endif

static void btapp_avk_rc_meta_msg_handler(tBTA_AVK_EVT event, tBTA_AVK *p_data);
static char *btapp_avk_evt_to_str(tBTA_AVK_EVT evt_code);
static void btapp_avk_open_rc_timer_cback(void *p_tle);

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
#if ((defined BTAPP_AVK_RC_CONSOLE_INCLUDE) && (BTAPP_AVK_RC_CONSOLE_INCLUDE == TRUE))

static void btapp_avk_rc_menu(void)
{
    btapp_console_puts("0. Exit ");
    btapp_console_puts("1. AVK Play ");
    btapp_console_puts("2. AVK Stop ");
    btapp_console_puts("3. AVK Pause ");
    btapp_console_puts("4. AVK Vol-Up ");
    btapp_console_puts("5. AVK Vol-Down ");
    btapp_console_puts("    ");
}

static void btapp_avk_rc_handler(tBTAPP_CONSOLE_MSG* p_console_msg)
{
    switch(p_console_msg->data[0])
    {
        case MENU_ITEM_0:
            if( btapp_console_menu_db[CONSOLE_DM_ID] != NULL)
            {
                btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
                btapp_console_cb.console_state = CONSOLE_IDLE;
                btapp_console_menu_db[CONSOLE_DM_ID]();
            }
            break;
        case MENU_ITEM_1:
            btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
            btapp_console_cb.console_state = CONSOLE_AVKRC_PLAY;
            btapp_avk_send_rc_cmd(BTA_AVK_RC_PLAY);
            break;

        case MENU_ITEM_2:
            btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
            btapp_console_cb.console_state = CONSOLE_AVKRC_STOP;
            btapp_avk_send_rc_cmd(BTA_AVK_RC_STOP);
            break;

        case MENU_ITEM_3:
            btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
            btapp_console_cb.console_state = CONSOLE_AVKRC_PAUSE;
            btapp_avk_send_rc_cmd(BTA_AVK_RC_PAUSE);
            break;

        case MENU_ITEM_4:
            btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
            btapp_console_cb.console_state = CONSOLE_AVKRC_VOL_UP;
            btapp_avk_rc_volume_up();
            break;

        case MENU_ITEM_5:
            btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
            btapp_console_cb.console_state = CONSOLE_AVKRC_VOL_DOWN;
            btapp_avk_rc_volume_down();
            break;

        default:
            APPL_TRACE_ERROR0("Command or menu not support!!!");
            break;
    }
}

void btapp_avk_rc_console_init(void)
{
    btapp_console_register(btapp_avk_rc_menu, btapp_avk_rc_handler, CONSOLE_AVKRC_ID);
}
#endif

/*******************************************************************************
**
** Function         btapp_avk_cback
**
** Description      AK callback from BTA
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_cback(tBTA_AVK_EVT event, tBTA_AVK *p_data)
{
    FLOW_SPEC   flow_spec;
    char        *company_str;
    int         rc_timeout_val;


    APPL_TRACE_DEBUG1("avk callback %s", btapp_avk_evt_to_str(event));

    switch (event)
    {
    case BTA_AVK_ENABLE_EVT:
        btapp_ak_cb.enabled = TRUE;
        btapp_ak_cb.rc_try = BTAPP_AVK_RC_RETRY;

        BTA_AvkRegister(BTA_AVK_CHNL_AUDIO, btapp_cfg.avk_service_name, 0);

        btapp_avk_rc_acp_open();            //open one AVCT connection for peer device as ACP role when AVK enable event is posted.

        if (btapp_cfg.avk_vdp_support)
        {
            BTA_AvkRegister(BTA_AVK_CHNL_VIDEO, btapp_cfg.avk_service_name, 0);
        }

        break;

    case BTA_AVK_REGISTER_EVT:
        APPL_TRACE_DEBUG3("  REGISTER_EVT: chnl = %d, app_id = %d, status = %d",
            p_data->registr.chnl, p_data->registr.app_id, p_data->registr.status);
        break;
    case BTA_AVK_UPDATE_SEPS_EVT:
        if (p_data->update_seps.status)
        {
            APPL_TRACE_DEBUG0("Fail to update SEPs, close the current active AVK connection first");

        }
        else
        {
            APPL_TRACE_DEBUG0("Update SEPs successfully");
            if (p_data->update_seps.available == TRUE)
            {
                btapp_ak_cb.is_sink_seps_disabled = FALSE;
                APPL_TRACE_DEBUG0("AVK Sink SEPs Enabled");
            }
            else
            {   btapp_ak_cb.is_sink_seps_disabled = TRUE;
                APPL_TRACE_DEBUG0("AVK Sink SEPs Disabled");
            }
        }
        break;

    case BTA_AVK_SIG_OPEN_EVT:
        APPL_TRACE_DEBUG1("  status:%d", p_data->sig_open.status);
        if (p_data->sig_open.status == BTA_AVK_SUCCESS)
        {
            APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x",
                    p_data->sig_open.bd_addr[0], p_data->sig_open.bd_addr[1],
                    p_data->sig_open.bd_addr[2], p_data->sig_open.bd_addr[3],
                    p_data->sig_open.bd_addr[4], p_data->sig_open.bd_addr[5]);
            APPL_TRACE_DEBUG1(" chnl %x", p_data->sig_open.chnl);
            BTAPP_AK_SETSTATUS(BTAPP_AK_ST_SIG_CONNECT);
			
        }
        break;

    case BTA_AVK_SIG_CLOSE_EVT:
        APPL_TRACE_DEBUG1("  status:%d", p_data->sig_open.status);

        if (p_data->sig_close.reason == BTA_AVK_CLOSE_CONN_LOSS)
        {
            APPL_TRACE_DEBUG0("AVK is closed due to connection timeout");
        }

        if (p_data->sig_close.status == BTA_AVK_SUCCESS)
        {
            APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x",
                    p_data->sig_open.bd_addr[0], p_data->sig_open.bd_addr[1],
                    p_data->sig_open.bd_addr[2], p_data->sig_open.bd_addr[3],
                    p_data->sig_open.bd_addr[4], p_data->sig_open.bd_addr[5]);
            APPL_TRACE_DEBUG1(" chnl %x", p_data->sig_close.chnl);
            BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_SIG_CONNECT);
            btapp_ak_cb.status = BTAPP_AK_ST_CONNECTABLE;
			
        }
        break;

    case BTA_AVK_OPEN_EVT:
        APPL_TRACE_DEBUG1("  status:%d", p_data->open.status);
        APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x",
                          p_data->open.bd_addr[0], p_data->open.bd_addr[1],
                          p_data->open.bd_addr[2], p_data->open.bd_addr[3],
                          p_data->open.bd_addr[4], p_data->open.bd_addr[5]);

		
		if (p_data->open.status == BTA_AVK_SUCCESS)
        {
            /* We don't know what will be the seq_num of the first packet. */
            btapp_ak_cb.seq_started = FALSE;

            /* save the device in nvram. */
            bdcpy(btapp_avk_db.last_cnt_src, p_data->open.bd_addr);
            btapp_avk_save_device((UINT8*)p_data->open.bd_addr, btapp_get_device_record(p_data->open.bd_addr));

			/* get status , keep BLE status */
            tBTA_DM_DISC disc_mode;
            tBTA_DM_CONN conn_mode;
            BTA_DmGetVisibility(&disc_mode,  &conn_mode, NULL, NULL);
            disc_mode &=0xFF00;
            conn_mode &=0xFF00;
			
			/* disable inquiry scan */
			printf("111111 disc_mode 0x%x  conn_mode 0x%x\n", disc_mode, conn_mode);
            disc_mode |= BTA_DM_NON_DISC;
            conn_mode |= BTA_DM_CONN;
            BTA_DmSetVisibility(disc_mode, conn_mode, BTA_DM_IGNORE, BTA_DM_IGNORE);

            /* set qos */
            memset(&flow_spec, 0, sizeof(flow_spec));
            flow_spec.service_type = BEST_EFFORT;
            flow_spec.token_rate = 22900;
            BTM_SetQoS(p_data->open.bd_addr, &flow_spec, NULL);
            BTAPP_AK_SETSTATUS(BTAPP_AK_ST_STR_CONNECT);

#if (BTU_BTC_SNK_INCLUDED == TRUE)
            /* Using BTC lite stack */
            if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
                (btapp_ak_cb.audio_path_supported == TRUE))
            {
                APPL_TRACE_DEBUG1("Start Audio Routing route 0x%02x", btapp_ak_cb.btc_route);
                btapp_avk_switch_bb2btc ();
            }
#endif
			g_BTConnectStatus = 1;
        }
        break;

    case BTA_AVK_CLOSE_EVT:
        btapp_ak_cb.str_started = FALSE;

        /* restore visibility settings */
        if(btapp_device_db.visibility)
        {
            BTA_DmSetVisibility(BTA_DM_GENERAL_DISC, BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);
        }
        else
        {
            BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);
        }
        BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_STR_CONNECT);

#if (BTU_BTC_SNK_INCLUDED == TRUE)
        if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
            (btapp_ak_cb.audio_path_supported == TRUE) &&
            btapp_cb.is_switched)
        {
            btapp_ak_cb.close_pending = TRUE;
            btapp_avk_route_audio (event);
        }
#endif
        APPL_TRACE_DEBUG0("Open one AVCT connection for peer device as ACP role");
        btapp_avk_rc_acp_open();
		g_BTConnectStatus = 0;
        break;

    case BTA_AVK_START_EVT:
        APPL_TRACE_DEBUG1("  status:%d", p_data->start.status);
        if (p_data->start.status == BTA_AVK_SUCCESS)
        {
            btapp_ak_cb.str_started = TRUE;
			APPL_TRACE_DEBUG0("TAPP_AK_SETSTATUS(BTAPP_AK_ST_START) BTA_AVK_START_EVT");

            BTAPP_AK_SETSTATUS(BTAPP_AK_ST_START);
            BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_STOP);
            BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_SUSPEND);

#if (BTU_BTC_SNK_INCLUDED == TRUE)
            /* Using BTC lite stack */
            APPL_TRACE_DEBUG2("btc_route %x audio_path_supported %d", btapp_ak_cb.btc_route, btapp_ak_cb.audio_path_supported);
            if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
                (btapp_ak_cb.audio_path_supported == TRUE))
            {
                btapp_dm_set_codec_config (btapp_ak_cb.rt_handle, btapp_ak_cb.btc_codec_type, btapp_ak_cb.btc_codec_info);
                btapp_avk_route_audio (event);
            }
#endif

#if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE)
            if( btapp_hs_get_active_handle(BTAPP_HS_ST_IN_CALL))
            {
                APPL_TRACE_DEBUG0("Send Pause due to active call for MPS support");
                btapp_avk_send_rc_cmd(BTA_AVK_RC_PAUSE);
            }
#endif /* #if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE) */

#if (defined(BTA_AV_INCLUDED)&& BTA_AV_INCLUDED==TRUE)
            btapp_av_start_stream();
#endif
        }

        break;

    case BTA_AVK_STOP_EVT:
        btapp_ak_cb.str_started = FALSE;
		APPL_TRACE_DEBUG0("BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_START) BTA_AVK_STOP_EVT");
        BTAPP_AK_SETSTATUS(BTAPP_AK_ST_STOP);
        BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_START);
        BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_SUSPEND);

#if (defined(BTA_AV_INCLUDED)&& BTA_AV_INCLUDED==TRUE)
            btapp_av_stop_stream();
#endif
        break;

    case BTA_AVK_PROTECT_REQ_EVT:
        APPL_TRACE_DEBUG1("  PROTECT_REQ::data len:%d", p_data->protect_req.len);

        /* send protect response with same data for testing purposes */
        BTA_AvkProtectRsp(p_data->protect_req.chnl, BTA_AVK_ERR_NONE, p_data->protect_req.p_data, p_data->protect_req.len);
        break;

    case BTA_AVK_PROTECT_RSP_EVT:
        APPL_TRACE_DEBUG2("  PROTECT_RSP:: err_code:%d len:%d", p_data->protect_rsp.err_code, p_data->protect_rsp.len);
        break;

    case BTA_AVK_RC_OPEN_EVT:
        APPL_TRACE_DEBUG4("RC_OPEN status %d rc_handle:%x, peer features=0x%x avk_status:0x%02x",
                          p_data->rc_open.status, p_data->rc_open.rc_handle,
                          p_data->rc_open.peer_features, btapp_ak_cb.status);

        if (p_data->rc_open.status == BTA_AVK_SUCCESS)
        {
            if (p_data->rc_open.peer_features & BTA_AVK_FEAT_RCCT)
            {
                APPL_TRACE_DEBUG2("   Peer supports CT role (ver=0x%04x, CT features=0x%04x)", p_data->rc_open.peer_ct.version, p_data->rc_open.peer_ct.features);
                btapp_ak_cb.peer_ct_features = p_data->rc_open.peer_ct.features;
            }

            if (p_data->rc_open.peer_features & BTA_AVK_FEAT_RCTG)
            {
                APPL_TRACE_DEBUG2("   Peer supports TG role (ver=0x%04x, TG features=0x%04x)", p_data->rc_open.peer_tg.version, p_data->rc_open.peer_tg.features);
                btapp_ak_cb.peer_tg_features = p_data->rc_open.peer_tg.features;
            }
            BTAPP_AK_SETSTATUS(BTAPP_AK_ST_RC_CONNECT);
                btapp_ak_cb.rc_handle = p_data->rc_open.rc_handle;

            btapp_stop_timer(&btapp_ak_cb.rc_timer_tle);

            btapp_avk_rc_get_info(AVRC_OP_UNIT_INFO);
            btapp_avk_rc_get_info(AVRC_OP_SUB_INFO);
            btapp_avk_rc_get_capabilities(AVRC_CAP_COMPANY_ID);
            btapp_avk_rc_get_capabilities(AVRC_CAP_EVENTS_SUPPORTED);
        }

        /* If AVRCP fail to open and AVDTP is state after signaling is opened and retry is not 0 */
        /* Try open AVRCP from app after a random timeout */
        else if (p_data->rc_open.status == BTA_AVK_FAIL_AVRCP_CTRL &&
                 btapp_ak_cb.status > BTAPP_AK_ST_CONNECTABLE)

        {
            if (btapp_ak_cb.rc_try)
            {
                /* Use a random number generater to get timeout between 1 - 2 seconds */
                rc_timeout_val = 2;
                APPL_TRACE_DEBUG2("Retrying AVRCP open in %d secs (retries remaining %d)",
                                  rc_timeout_val, btapp_ak_cb.rc_try);
                btapp_ak_cb.rc_handle = BTA_AVK_RC_HANDLE_NONE;
                btapp_ak_cb.rc_try--;
                btapp_ak_cb.rc_timer_tle.p_cback = (TIMER_CBACK *)&btapp_avk_open_rc_timer_cback;
                btapp_start_timer(&btapp_ak_cb.rc_timer_tle, 0, rc_timeout_val);
            }
            else
            {
                btapp_ak_cb.rc_try = BTAPP_AVK_RC_RETRY;
            }
        }
        else
        {
            APPL_TRACE_DEBUG1("RC Open Failed %d", p_data->rc_open.status);
        }
        break;

    case BTA_AVK_RC_PEER_FEAT_EVT:
        APPL_TRACE_DEBUG2("RC_PEER_FEAT  rc_handle:%x, peer roles=0x%x", p_data->rc_peer_feat.rc_handle, p_data->rc_peer_feat.roles);
        if (p_data->rc_peer_feat.roles & BTA_AVK_FEAT_RCCT)
            APPL_TRACE_DEBUG2("   Peer supports CT role (ver=0x%04x, CT features=0x%04x)", p_data->rc_peer_feat.ct.version, p_data->rc_peer_feat.ct.features);

        if (p_data->rc_peer_feat.roles & BTA_AVK_FEAT_RCTG)
            APPL_TRACE_DEBUG2("   Peer supports TG role (ver=0x%04x, TG features=0x%04x)", p_data->rc_peer_feat.tg.version, p_data->rc_peer_feat.tg.features);
        break;

    case BTA_AVK_RC_CLOSE_EVT:
#if (defined(BTA_AVK_MIN_DEBUG_TRACES) && BTA_AVK_MIN_DEBUG_TRACES == TRUE)
        APPL_TRACE_ERROR0("RC_CLOSE");
#else
        APPL_TRACE_DEBUG0("RC_CLOSE");
#endif
        BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_RC_CONNECT);
        btapp_ak_cb.rc_try = BTAPP_AVK_RC_RETRY;
        APPL_TRACE_DEBUG0("Open one AVCT connection for peer device as ACP role");
        btapp_avk_rc_acp_open();
        break;

    case BTA_AVK_REMOTE_CMD_EVT:
        APPL_TRACE_DEBUG2("  rc_cmd_id:%d key_state:%d", p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state);
        btapp_platform_avk_remote_cmd(p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state);
        break;

    case BTA_AVK_REMOTE_RSP_EVT:
        APPL_TRACE_DEBUG2("  rc_id:%d key_state:%d", p_data->remote_rsp.rc_id, p_data->remote_rsp.key_state);
        APPL_TRACE_DEBUG2("  rsp_code:%d label:%d", p_data->remote_rsp.rsp_code, p_data->remote_rsp.label);
        /* parse key press command response, MSFT does not support key release */
        if (p_data->remote_rsp.key_state == BTA_AVK_STATE_PRESS && p_data->remote_rsp.rsp_code == AVRC_RSP_NOT_IMPL)
        {
            btapp_platform_avk_remote_cmd(p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state);
        }
        break;

    case BTA_AVK_VENDOR_CMD_EVT:
        if (p_data->vendor_rsp.company_id == AVRC_CO_BROADCOM)
            company_str = "BRCM";
        else
            company_str = "Unknown Company";

        APPL_TRACE_DEBUG2("  code:%d label:%d",  p_data->vendor_cmd.code, p_data->vendor_cmd.label);
        APPL_TRACE_DEBUG3("  company_id:%s [0x%x] len:%d",company_str, p_data->vendor_cmd.company_id, p_data->vendor_cmd.len);

        /* send vendor response with same data for testing purposes */
        BTA_AvkVendorRsp(btapp_ak_cb.rc_handle, p_data->vendor_cmd.label, BTA_AVK_RSP_ACCEPT,
                        p_data->vendor_cmd.p_data, (UINT8)p_data->vendor_cmd.len, AVRC_CO_METADATA);
        break;

    case BTA_AVK_VENDOR_RSP_EVT:
        if (p_data->vendor_rsp.company_id == AVRC_CO_BROADCOM)
            company_str = "BRCM";
        else
            company_str = "Unknown Company";

        APPL_TRACE_DEBUG2("  code:%d label:%d",  p_data->vendor_rsp.code, p_data->vendor_rsp.label);
        APPL_TRACE_DEBUG3("  company_id:%s [0x%x]  len:%d",company_str, p_data->vendor_rsp.company_id, p_data->vendor_rsp.len);
        break;

    case BTA_AVK_META_MSG_EVT:
        btapp_avk_rc_meta_msg_handler(event, p_data);
        break;

    case BTA_AVK_BROWSE_MSG_EVT:
        /* The layer above BTA is required to free browsing packet,
         * since some implementations need to hold on the buffers */
        if (p_data->meta_msg.p_msg->browse.p_browse_pkt)
        {
            GKI_freebuf (p_data->meta_msg.p_msg->browse.p_browse_pkt);
        }
        break;

    case BTA_AVK_RECONFIG_EVT:
        APPL_TRACE_EVENT1("  recfg:%d",p_data->reconfig.status);
        break;

    case BTA_AVK_SUSPEND_EVT:
        APPL_TRACE_EVENT1("  suspend:%d",p_data->suspend.status);
		APPL_TRACE_DEBUG0("BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_START) BTA_AVK_SUSPEND_EVT");
        btapp_ak_cb.str_started = FALSE;
        BTAPP_AK_SETSTATUS(BTAPP_AK_ST_SUSPEND);
        BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_STOP);
        BTAPP_AK_RESETSTATUS(BTAPP_AK_ST_START);

#if (defined(BTA_AV_INCLUDED)&& BTA_AV_INCLUDED==TRUE)
        btapp_av_stop_stream();
#endif

#if (BTU_BTC_SNK_INCLUDED == TRUE)
        if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
            (btapp_ak_cb.audio_path_supported == TRUE))
        {
            btapp_avk_route_audio (event);
        }

        if (btapp_ak_cb.reconfig_pending == TRUE)
        {
            APPL_TRACE_DEBUG0("BTA_AVK_SUSPEND_EVT done");
            btapp_ak_cb.reconfig_pending = FALSE;
            /* skip the length, the media type and the codec type byte */
            btapp_avk_update_btc_codec_info(btapp_ak_cb.reconfig_codec_type,
                                            &(btapp_ak_cb.reconfig_codec_info[3]));
        }
#endif
        break;

#if (BTU_BTC_SNK_INCLUDED == TRUE)
    case BTA_AVK_RECONFIG_PENDING:
        APPL_TRACE_EVENT0("BTA_AVK_RECONFIG_PENDING");
        btapp_ak_cb.reconfig_pending = TRUE;
        break;

#endif

    default:
        break;
    }
}

/*******************************************************************************
**
** Function         btapp_avk_proc_set_abs_volume
**
** Description      composes the AVRC_PDU_SET_ABSOLUTE_VOLUME response
**
** Returns          the GKI message that contains the response
**
*******************************************************************************/
static BT_HDR * btapp_avk_proc_set_abs_volume(tAVRC_SET_VOLUME_CMD *p_cmd,
                                             tAVRC_SET_VOLUME_RSP *p_rsp,
                                             tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    APPL_TRACE_DEBUG0("btapp_avk_proc_set_abs_volume");

    /* Change TG's absolute volume */
    if (p_cmd->volume >= BTAPP_MIN_ABS_VOLUME && p_cmd->volume <= BTAPP_MAX_ABS_VOLUME)
    {
        /* change waveout volume */
        btapp_avk_set_the_vol(p_cmd->volume);
        p_rsp->volume = p_cmd->volume;
    }
    else
    {
        APPL_TRACE_DEBUG0("Reach max/min volume");
        p_rsp->volume = btapp_ak_cb.cur_volume;
    }

    *p_code = BTA_AV_RSP_ACCEPT;
    AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);

    return p_rsp_pkt;

}

/*******************************************************************************
**
** Function         btapp_avk_proc_reg_notif
**
** Description      composes the AVRC_PDU_REGISTER_NOTIFICATION response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_avk_proc_reg_notif(tBTA_AVK_META_MSG *p_msg, tAVRC_REG_NOTIF_CMD *p_cmd,
                                        tAVRC_REG_NOTIF_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT16  evt_mask = 1, index;
    UINT8   rc_handle = p_msg->rc_handle;

    APPL_TRACE_DEBUG1("btapp_avk_proc_reg_notif 0x%04x",
                       btapp_ak_cb.registered_events.evt_mask);
    *p_code = BTA_AVK_RSP_INTERIM;
    index = p_cmd->event_id - 1;
    evt_mask <<= index;

    /* Register event to the BTAPP AVK control block  */
    btapp_ak_cb.registered_events.evt_mask |= evt_mask;
    btapp_ak_cb.registered_events.label[index] = p_msg->label;
    vol_tran_label = (p_msg->label & 0x0F);

    APPL_TRACE_DEBUG1("evt_mask: 0x%04x", btapp_ak_cb.registered_events.evt_mask);

    p_rsp->event_id = p_cmd->event_id;

    switch(p_cmd->event_id)
    {
    case AVRC_EVT_VOLUME_CHANGE:        /* 0x0d */
        p_rsp->param.volume = btapp_ak_cb.cur_volume;
        break;

    default:
        APPL_TRACE_DEBUG1("Unknown event 0x%x", p_cmd->event_id);
        *p_code = BTA_AV_RSP_NOT_IMPL;
    }

    if (*p_code == BTA_AV_RSP_INTERIM)
        AVRC_BldResponse (rc_handle, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);

    return p_rsp_pkt;

}

/*******************************************************************************
**
** Function         btapp_avk_validate_pdu
**
** Description      make sure the requested pdu id is valid with the current enabled features.
**
** Returns          AVRC_STS_NO_ERROR, if no error
**
*******************************************************************************/
static tAVRC_STS btapp_avk_validate_pdu(UINT8 pdu_id)
{
    tAVRC_STS       sts = AVRC_STS_BAD_CMD;
    tBTA_AVK_FEAT   avk_features = btapp_cfg.avk_features;

    APPL_TRACE_DEBUG1("btapp_rc_validate_pdu avk_features: x%x", avk_features);

    if (avk_features & BTA_AVK_FEAT_METADATA)
    {
        switch (pdu_id)
        {
        case AVRC_PDU_REGISTER_NOTIFICATION:       /* 0x31 */
            sts = AVRC_STS_NO_ERROR;
            break;
        }
    }

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    if (avk_features & BTA_AVK_FEAT_RCTG)
    {
        /* TG role only supports set abs volume */
        switch (pdu_id)
        {
        case AVRC_PDU_SET_ABSOLUTE_VOLUME:         /* 0x50 */
            sts = AVRC_STS_NO_ERROR;
            break;
        }
    }
#endif

    return sts;
}

/*******************************************************************************
**
** Function         btapp_avk_rc_meta_msg_handler
**
** Description      Handler for incoming avrc metadata messages
**
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_meta_msg_handler(tBTA_AVK_EVT event, tBTA_AVK *p_data)
{
    tBTA_AVK_META_MSG *p_meta = (tBTA_AVK_META_MSG *)(&p_data->meta_msg);
    tAVRC_RESPONSE  response;
    tAVRC_COMMAND   command;
    UINT8 pkt_type, target_pdu;
    tAVRC_STS       sts = AVRC_STS_BAD_CMD;
    BT_HDR          *p_rsp_pkt = NULL;
    tBTA_AV_CODE    rsp_code = BTA_AV_RSP_REJ;
    UINT8           i;
    UINT8           temp_attr_value[32];
    tAVRC_GET_ELEM_ATTRS_RSP * p_elem_attr;


    /* Check for browsing meta message */
    if (p_meta->p_msg->hdr.opcode == AVRC_OP_BROWSE)
    {
        /* Check if browsing command or response */
        if (p_meta->p_msg->browse.hdr.ctype == AVCT_RSP)
        {
            APPL_TRACE_DEBUG0("btapp_avk_rc_meta_msg_handler received browsing response");

            /* Metadata response */
            if (AVRC_ParsResponse(p_meta->p_msg, &response, btapp_dm_avrc_buf, BTAPP_DM_AVRC_BUF_SIZE) == AVRC_STS_NO_ERROR)
            {
                /* Check for browsing PDUs */
                switch (response.pdu)
                {
                case AVRC_PDU_GET_FOLDER_ITEMS:
                    APPL_TRACE_DEBUG1("AVRC_PDU_GET_FOLDER_ITEMS rsp status=0x%x", response.get_items.status);
                    break;

                case AVRC_PDU_CHANGE_PATH:
                    APPL_TRACE_DEBUG1("AVRC_PDU_CHANGE_PATH rsp status=0x%x", response.chg_path.status);
                    break;

                case AVRC_PDU_GET_TOTAL_NUM_OF_ITEMS:
                    APPL_TRACE_DEBUG3("AVRC_PDU_GET_TOTAL_NUM_OF_ITEMS rsp status=0x%x UID_counter=%d num_of_item=%d",
                                       response.get_num_of_items.status, response.get_num_of_items.uid_counter,
                                       response.get_num_of_items.num_items);
                    break;

                default:
                    APPL_TRACE_DEBUG1("Unhandled browsing pdu: 0x%x", response.pdu);

                }
            }
            else
            {
                APPL_TRACE_DEBUG0("Problem parsing browsing response");
            }
        }
        else if (p_meta->p_msg->browse.hdr.ctype == AVCT_CMD)
        {
            APPL_TRACE_DEBUG0("btapp_avk_rc_meta_msg_handler received browsing command");
        }
        else
        {
            APPL_TRACE_DEBUG0("btapp_avk_rc_meta_msg_handler received browsing reject response");
        }

    }
    /* Check if vendor dependent command or response */
    else if (p_meta->p_msg->hdr.ctype >= BTA_AVK_RSP_NOT_IMPL)
    {
        target_pdu = p_meta->p_msg->vendor.p_vendor_data[0];
        pkt_type = p_meta->p_msg->vendor.p_vendor_data[1];

        APPL_TRACE_DEBUG0("btapp_avk_rc_meta_msg_handler received metadata response");


        /* Metadata response */
        AVRC_ParsResponse(p_meta->p_msg, &response, btapp_dm_avrc_buf, BTAPP_DM_AVRC_BUF_SIZE);

        switch (response.pdu)
        {
        case AVRC_PDU_GET_CAPABILITIES:
            APPL_TRACE_DEBUG1("AVRC_PDU_GET_CAPABILITIES rsp status=0x%x", response.get_caps.status);
            APPL_TRACE_DEBUG2("count:%d, capability_id:%02x",response.get_caps.count, response.get_caps.capability_id);
            UINT8 * ptr = (UINT8*)&response.get_caps.param;
            for (i=0; i<response.get_caps.count; i++)
            {
                APPL_TRACE_DEBUG2("[%d]=%02x",i, *ptr);
                if( *ptr == AVRC_EVT_PLAY_STATUS_CHANGE || *ptr ==AVRC_EVT_TRACK_CHANGE || *ptr ==AVRC_EVT_PLAY_POS_CHANGED)
                    btapp_avk_rc_reg_notif(*ptr, 1);
                ptr++;
            }
            break;

        case AVRC_PDU_LIST_PLAYER_APP_ATTR:
            APPL_TRACE_DEBUG1("AVRC_PDU_LIST_PLAYER_APP_ATTR rsp status=0x%x", response.list_app_attr.status);
            break;

        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:
            APPL_TRACE_DEBUG1("AVRC_PDU_GET_CUR_PLAYER_APP_VALUE rsp status=0x%x", response.get_cur_app_val.status);
            break;

        case AVRC_PDU_GET_ELEMENT_ATTR:
            p_elem_attr = &response.get_elem_attrs;
            APPL_TRACE_DEBUG2("AVRC_PDU_GET_ELEMENT_ATTR rsp attr:%d status=0x%x", p_elem_attr->num_attr, response.get_elem_attrs.status);
			memset(gTrackTitle,0,sizeof(gTrackTitle));
			memset(gTrackArtist,0,sizeof(gTrackArtist));
			for(i=0;i<p_elem_attr->num_attr;i++)
            {
                if( p_elem_attr->p_attrs[i].attr_id == AVRC_MEDIA_ATTR_ID_TITLE)
                {
                	memcpy(gTrackTitle,p_elem_attr->p_attrs[i].name.p_str,p_elem_attr->p_attrs[i].name.str_len);
					gTrackChange_Title = true;
                    APPL_TRACE_DEBUG1("Title:%s", gTrackTitle)
					//APPL_TRACE_DEBUG1("Title:%s", p_elem_attr->p_attrs[i].name.p_str)
                }
				if( p_elem_attr->p_attrs[i].attr_id == AVRC_MEDIA_ATTR_ID_ARTIST)
                {
                	memcpy(gTrackArtist,p_elem_attr->p_attrs[i].name.p_str,p_elem_attr->p_attrs[i].name.str_len);
					gTrackChange_Artist = true;
                    APPL_TRACE_DEBUG1("Artist:%s", gTrackArtist)
						 //APPL_TRACE_DEBUG1("Artist:%s", p_elem_attr->p_attrs[i].name.p_str)
                }
				/*if( p_elem_attr->p_attrs[i].attr_id == AVRC_MEDIA_ATTR_ID_ALBUM)
                {
                    APPL_TRACE_DEBUG1("Album:%s", p_elem_attr->p_attrs[i].name.p_str)
                }
                if( p_elem_attr->p_attrs[i].attr_id == AVRC_MEDIA_ATTR_ID_PLAYING_TIME)
                {
                    APPL_TRACE_DEBUG1("Time:%sms", p_elem_attr->p_attrs[i].name.p_str)
                }*/
				
            }

            break;

        case AVRC_PDU_GET_PLAY_STATUS:
            APPL_TRACE_DEBUG2("AVRC_PDU_GET_PLAY_STATUS rsp status=0x%x, play_status=0x%x", response.get_play_status.status,
                response.get_play_status.play_status);
            break;

        case AVRC_PDU_REGISTER_NOTIFICATION:
            APPL_TRACE_DEBUG1("AVRC_PDU_REGISTER_NOTIFICATION rsp status=0x%x", response.reg_notif.status);
            switch(response.reg_notif.event_id)
            {
                /* 0x01 */
                case AVRC_EVT_PLAY_STATUS_CHANGE:
                    switch (response.reg_notif.param.play_status)
                    {
                        case AVRC_PLAYSTATE_PLAYING:
							gPlayStatus = 1;//play
                            APPL_TRACE_DEBUG0("Play Status Playing");
                            break;
                        case AVRC_PLAYSTATE_STOPPED:
                        case AVRC_PLAYSTATE_PAUSED:
							gPlayStatus = 2;//pause
                            APPL_TRACE_DEBUG0("Play Status Stopped");
                            break;
                        default:
                            APPL_TRACE_DEBUG1("Play Status : %02x", response.reg_notif.param.play_status);
                        break;
                    }
                    if (p_meta->p_msg->hdr.ctype == BTA_AVK_RSP_CHANGED)
                        btapp_avk_rc_reg_notif(AVRC_EVT_PLAY_STATUS_CHANGE, 0);
                    break;
                /* 0x02 */
                case AVRC_EVT_TRACK_CHANGE:
                    APPL_TRACE_DEBUG1("Track changed :0x%d", response.reg_notif.param.track);
                    if (p_meta->p_msg->hdr.ctype == BTA_AVK_RSP_CHANGED)
                    {
                        tAVRC_COMMAND_ATTR attr;
                        attr.p_uint32 = malloc(sizeof(tAVRC_MEDIA_ATTR_ID)*AVRC_MAX_NUM_MEDIA_ATTR_ID);
                        attr.p_uint32[0] = AVRC_MEDIA_ATTR_ID_TITLE;        /* Title   */
                        attr.p_uint32[1] = AVRC_MEDIA_ATTR_ID_ARTIST;       /* Artist  */
                        attr.p_uint32[2] = AVRC_MEDIA_ATTR_ID_ALBUM;        /* Album   */
                        attr.p_uint32[3] = AVRC_MEDIA_ATTR_ID_TRACK_NUM;    /* Track number */
                        attr.p_uint32[4] = AVRC_MEDIA_ATTR_ID_NUM_TRACKS;   /* Number of Track */
                        attr.p_uint32[5] = AVRC_MEDIA_ATTR_ID_GENRE;        /* Genre */
                        attr.p_uint32[6] = AVRC_MEDIA_ATTR_ID_PLAYING_TIME; /* Playing Time */
                        btapp_avk_rc_get_element_attibutes(0, 7, attr.p_uint32);
                        free(attr.p_uint32);

                        btapp_avk_rc_reg_notif(AVRC_EVT_TRACK_CHANGE, 0);
                    }
                    break;
                /* 0x03 0x04 */
                case AVRC_EVT_TRACK_REACHED_END:
                case AVRC_EVT_TRACK_REACHED_START:
                    APPL_TRACE_DEBUG1("REACHED START/END :0x%x", response.reg_notif.param.track);
                    break;
                /* 0x05 */
                case AVRC_EVT_PLAY_POS_CHANGED:
                    APPL_TRACE_DEBUG1("play_pos changed :%d", response.reg_notif.param.play_pos);
                    if (p_meta->p_msg->hdr.ctype == BTA_AVK_RSP_CHANGED)
                        btapp_avk_rc_reg_notif(AVRC_EVT_PLAY_POS_CHANGED, 2);
                    break;
                case AVRC_EVT_VOLUME_CHANGE:
                    APPL_TRACE_DEBUG1("Volume changed :0x%x", response.reg_notif.param.volume);
                    if (p_meta->p_msg->hdr.ctype == BTA_AVK_RSP_CHANGED)
                        btapp_avk_rc_reg_notif(AVRC_EVT_VOLUME_CHANGE, 0);
                    break;
                default:
                    APPL_TRACE_DEBUG1("unhandled event: Event ID: 0x%x", response.reg_notif.event_id);
                    break;
            }
            break;

        case AVRC_PDU_REQUEST_CONTINUATION_RSP:
            APPL_TRACE_DEBUG1("AVRC_PDU_REQUEST_CONTINUATION_RSP rsp status=0x%x", response.continu.status);
            break;

        case AVRC_PDU_ABORT_CONTINUATION_RSP:
            APPL_TRACE_DEBUG1("AVRC_PDU_ABORT_CONTINUATION_RSP rsp status=0x%x", response.abort.status);
            break;

        default:
            APPL_TRACE_DEBUG1("Unhandled pdu: 0x%x", response.pdu);
        }
    }
    else
    {
        /* vendor specific command */
        sts = AVRC_ParsCommand(p_meta->p_msg, &command, btapp_dm_avrc_buf, BTAPP_DM_AVRC_BUF_SIZE);
        response.pdu = command.pdu;
        if (sts == AVRC_STS_NO_ERROR)
        {
            sts = btapp_avk_validate_pdu(response.pdu);
        }

        if (sts == AVRC_STS_NO_ERROR)
        {
            response.rsp.status = AVRC_STS_NO_ERROR;
            switch (command.pdu)
            {
            case AVRC_PDU_SET_ABSOLUTE_VOLUME:
                p_rsp_pkt = btapp_avk_proc_set_abs_volume(&command.volume, &response.volume, &rsp_code);
                break;

            case AVRC_PDU_REGISTER_NOTIFICATION:
                p_rsp_pkt = btapp_avk_proc_reg_notif(p_meta, &command.reg_notif, &response.reg_notif, &rsp_code);
                break;
            default:
                APPL_TRACE_DEBUG1("unknown pdu: 0x%x", command.pdu);
                break;
            }
        }
        else
        {
            APPL_TRACE_DEBUG2("* * * PARSING ERROR : %d opcode:x%x * * *", sts, p_meta->p_msg->hdr.opcode);
            /* compose reject message */
            response.rsp.status = sts;
            response.rsp.opcode = p_meta->p_msg->hdr.opcode;
            if (response.rsp.opcode == AVRC_OP_VENDOR && sts == AVRC_STS_BAD_CMD)
                response.pdu = AVRC_PDU_GENERAL_REJECT;
            AVRC_BldResponse (0, &response, &p_rsp_pkt);
        }

        if (rsp_code == BTA_AV_RSP_NOT_IMPL)
        {
            if (p_rsp_pkt)
            {
                GKI_freebuf (p_rsp_pkt);
                p_rsp_pkt = NULL;
            }
#if (AVCT_BROWSE_INCLUDED == TRUE)
            if (p_meta->p_msg->hdr.opcode == AVRC_OP_BROWSE)
            {
                /* use general reject */
                response.rsp.status = AVRC_STS_INTERNAL_ERR;
            }
            else
#endif
            {
                /* must be vendor command */
                BTA_AvkVendorRsp(p_data->meta_msg.rc_handle, p_data->meta_msg.label, BTA_AV_RSP_NOT_IMPL,
                                 p_meta->p_msg->vendor.p_vendor_data, (UINT8)p_meta->p_msg->vendor.vendor_len,
                                 p_meta->p_msg->vendor.company_id);
                return;
            }
        }

        if (p_rsp_pkt)
        {
            BTA_AvkMetaRsp(p_data->meta_msg.rc_handle, p_data->meta_msg.label, rsp_code, p_rsp_pkt);
        }
    }

    if (btapp_ak_cb.p_metadata_rsp_buf)
    {
        GKI_freebuf(btapp_ak_cb.p_metadata_rsp_buf);
        btapp_ak_cb.p_metadata_rsp_buf = NULL;
    }
}

/*******************************************************************************
**
** Function         btapp_avk_init
**
** Description      Initializes AK application
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_init(void)
{
    if (btapp_cfg.avk_included)
    {
        memset(&btapp_ak_cb, 0, sizeof(tBTAPP_AK_CB));

#if (BTU_BTC_SNK_INCLUDED == TRUE)
        if (btapp_cfg.avk_use_btc == TRUE)
        {
            /* Register audio routing module */
            BTA_RtRegister (btapp_avk_audio_route_cback);
        }
        else
        {
            APPL_TRACE_DEBUG1("btapp_avk_init: Enabling AVK, features = %d", btapp_cfg.avk_features);
            BTA_AvkEnable(btapp_cfg.avk_security, btapp_cfg.avk_features, btapp_avk_cback);
        }
#else
        APPL_TRACE_DEBUG1("btapp_avk_init: Enabling AVK, features = %d", btapp_cfg.avk_features);
        BTA_AvkEnable(btapp_cfg.avk_security, btapp_cfg.avk_features, btapp_avk_cback);
#endif

#if (BRCM_LPST_INCLUDED == TRUE)
        btm_lpst_register_data_handler(btapp_avk_lpst_cmd_handler);
		btm_lpst_register_event_handler(btapp_avk_lpst_event_handler);
#endif

#if ((defined BTAPP_AVK_RC_CONSOLE_INCLUDE) && (BTAPP_AVK_RC_CONSOLE_INCLUDE == TRUE))
    btapp_avk_rc_console_init();
#endif
    }
}

/*******************************************************************************
**
** Function         btapp_avk_disable
**
** Description      Disables AVK application
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_disable(void)
{
    if (btapp_cfg.avk_included)
    {
        if (btapp_ak_cb.enabled)
        {
            APPL_TRACE_DEBUG0("btapp_avk_disable");
            BTA_AvkDisable ();
            btapp_ak_cb.enabled = FALSE;

#if (BTU_BTC_SNK_INCLUDED == TRUE)
            if (btapp_ak_cb.rt_handle)
            {
                /* Deregister audio routing module */
                BTA_RtDeregister (btapp_ak_cb.rt_handle);
                btapp_ak_cb.rt_handle = 0;
            }
#endif
        }
        else
        {
            APPL_TRACE_ERROR0("btapp_avk_disable: AVK is already disabled.");
        }
    }
}

/*******************************************************************************
**
** Function         btapp_avk_start
**
** Description      Starts streaming
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_start(void)
{
    BTA_AvkStart();
}

/*******************************************************************************
**
** Function         btapp_avk_stop
**
** Description      Stops streaming
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_stop (BOOLEAN suspend)
{
    BTA_AvkStop(suspend);
}

/*******************************************************************************
**
** Function         btapp_avk_close
**
** Description      Closes Connection
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_close(void)
{
#if (BRCM_LPST_INCLUDED == TRUE)
    BOOLEAN res;
    res = btapp_avk_lpst_forward_event (BTAPP_AVK_APP_SUBCODE_CLOSE, dummy_bda);
    if (res == FALSE)
#endif
        BTA_AvkClose();
}

/*******************************************************************************
**
** Function         btapp_avk_dump_cb
**
** Description      dump control blocks
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_dump_cb(void)
{
#if (BRCM_LPST_INCLUDED == TRUE)
    bta_avk_lpst_dump_control_blocks();
#endif
}

/*******************************************************************************
**
** Function         btapp_avk_vendor
**
** Description      Sending vendor specific command.
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_vendor(void)
{
    UINT8 p_data[] = "vendor cmd test";

    btapp_ak_cb.label++;
    BTA_AvkVendorCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label, BTA_AVK_CMD_CTRL, (UINT8 *) p_data,
                        (UINT8) strlen((char *)p_data), AVRC_CO_METADATA);
}

/*******************************************************************************
**
** Function         btapp_avk_protect_req
**
** Description      Sending vendor specific command.
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_protect_req(tBTA_AVK_CHNL chnl)
{
    UINT8   p_data[] = "protect req test";

    BTA_AvkProtectReq(chnl, (UINT8 *) p_data, (UINT16) strlen((char *)p_data));
}

/*******************************************************************************
**
** Function         btapp_avk_device_connect
**
** Description      Connects to the audio source
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_device_connect(BD_ADDR bda, tBTA_AVK_CHNL chnl)
{
    BOOLEAN res;
    APPL_TRACE_DEBUG0("btapp_avk_device_connect");

    if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_SIG_CONNECT))
    {
        return;
    }

#if (BRCM_LPST_INCLUDED == TRUE)
    res = btapp_avk_lpst_forward_event (BTAPP_AVK_APP_SUBCODE_OPEN, bda);
    if (res== FALSE)
#endif
        BTA_AvkOpen(bda ,chnl, btapp_cfg.avk_security);
}

/*******************************************************************************
**
** Function         btapp_avk_update_seps
**
** Description      Change all the sink SEPs to available or unavailable
**
** Parameter        available: True Set all SEPs to available
**                             FALSE Set all SEPs to unavailable
**
** Returns          void
*******************************************************************************/
void btapp_avk_update_seps(BOOLEAN available)
{

    BTA_AvkUpdateStreamEndPoints(available);
}

/*******************************************************************************
**
** Function         btapp_avk_delay_report
**
** Description      Sending delay report command.
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_delay_report(tBTA_AVK_CHNL chnl, UINT16 delay)
{
    BTA_AvkDelayReport(chnl, delay);
}

/*******************************************************************************
**
** Function:    btapp_avk_send_rc_cmd
**
** Description: Send RC Command.
**
** Returns:     void
**
*******************************************************************************/
void btapp_avk_send_rc_cmd (tBTA_AVK_RC rc_op_id)
{
	printf("in btapp_avk_send_rc_cmd\n");
    if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_RC_CONNECT))
    {	
        BTA_AvkRemoteCmd(btapp_ak_cb.rc_handle, btapp_avk_get_next_label(), rc_op_id, BTA_AVK_STATE_PRESS);
        GKI_delay(200);
        BTA_AvkRemoteCmd(btapp_ak_cb.rc_handle, btapp_avk_get_next_label(), rc_op_id, AVRC_STATE_RELEASE);
    }
    else
    {	
        APPL_TRACE_ERROR0("btapp_avk_send_rc_cmd : RC not connected");
    }
}

/*******************************************************************************
**
** Function:    btapp_avk_get_next_label
**
** Description: Get next label
**
** Returns:     void
**
*******************************************************************************/
UINT8 btapp_avk_get_next_label (void)
{
    /* label is 4 bit value */
    btapp_ak_cb.label = (btapp_ak_cb.label + 1) & 0x0F;

    return btapp_ak_cb.label;
}

/*******************************************************************************
**
** Function         btapp_avk_save_device
**
** Description
**
**
** Returns          void
*******************************************************************************/
static void btapp_avk_save_device (BD_ADDR_PTR p_addr, tBTAPP_REM_DEVICE * p_device_rec)
{
    tBTAPP_REM_DEVICE device_rec;
    tBTA_SERVICE_MASK avk_service_mask = (BTA_VDP_SERVICE_MASK | BTA_A2DP_SERVICE_MASK);

    if(p_device_rec)
    {
        p_device_rec->services |= avk_service_mask;
    }
    else
    {
        memset(&device_rec, 0, sizeof(device_rec));
        device_rec.trusted_mask  = 0;
        device_rec.name[0] = '\0';
        bdcpy(device_rec.bd_addr, p_addr);
        device_rec.is_trusted = TRUE;
        device_rec.services = avk_service_mask;
        p_device_rec = &device_rec;
    }

    btapp_store_device(p_device_rec);
}

#if (BTU_BTC_SNK_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_avk_switch_bb2btc
**
** Description      Switches from Baseband to BTC
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_switch_bb2btc(void)
{
    if (!btapp_cb.is_switched)
    {
        btapp_cb.switch_to = BTA_DM_SW_BB_TO_BTC;
        BTA_DmSwitchStack(BTA_DM_SW_BB_TO_BTC, btapp_avk_stack_sw_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_switch_btc2bb
**
** Description      Switches from BTC to Baseband
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_switch_btc2bb(void)
{
    btapp_cb.switch_to = BTA_DM_SW_BTC_TO_BB;
    BTA_DmSwitchStack(BTA_DM_SW_BTC_TO_BB, btapp_avk_stack_sw_cback);
}

/*******************************************************************************
**
** Function         btapp_avk_stack_sw_cback
**
** Description      Callback indicating result of stack switch
**
**
** Returns          void
*******************************************************************************/
static void btapp_avk_stack_sw_cback(tBTA_DM_SWITCH_EVT evt, tBTA_STATUS status)
{
    APPL_TRACE_DEBUG2 ("AVK SWITCH STACK evt %d, status %d", evt, status);

    if (status == BTA_SUCCESS)
    {
        if (btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BTC_TO_BB))
        {
            btapp_cb.is_switched = FALSE;
            APPL_TRACE_DEBUG0("AVK SWITCHED to BB");
#if (BTU_BTC_SNK_INCLUDED == TRUE)
            /* Stack switch is done, if there is pending setconfig, go ahead and proceed */
            if (btapp_ak_cb.close_pending == TRUE)
            {
                btapp_ak_cb.close_pending = FALSE;
                bta_avk_ci_setconfig(BTA_AVK_CHNL_AUDIO, A2D_SUCCESS, AVDT_ASC_CODEC);
            }
#endif
        }
        else if (!btapp_cb.is_switched && (btapp_cb.switch_to == BTA_DM_SW_BB_TO_BTC))
        {
            btapp_cb.is_switched = TRUE;
            APPL_TRACE_DEBUG0("AVK SWITCHED to BTC");
            /* Codec configuration */
            btapp_dm_set_codec_config (btapp_ak_cb.rt_handle, btapp_ak_cb.btc_codec_type, btapp_ak_cb.btc_codec_info);

            /* Stack switch is done, if there is pending start, go ahead and proceed */
            if (btapp_ak_cb.btc_start_pending == TRUE)
            {
                bta_avk_ci_audio_btc_start(BTA_AVK_CHNL_AUDIO);
                btapp_ak_cb.btc_start_pending = FALSE;
            }

        }
        else
        {
            APPL_TRACE_ERROR0("AVK STACK SWITCH : unknown case");
        }
    }
    else
    {
        APPL_TRACE_ERROR0("AVK STACK SWITCH FAILED!");
    }
}

/*******************************************************************************
**
** Function         btapp_avk_update_btc_codec_info
**
** Description      Update AVK codec information for BTC lite stack.
**
**
** Returns          void
*******************************************************************************/
void btapp_avk_update_btc_codec_info(tAUDIO_CODEC_TYPE c_type, UINT8 *p_info)
{
    UINT16  mp3_bitrate;

    btapp_ak_cb.btc_codec_type = c_type;
    APPL_TRACE_DEBUG1 ("btapp_avk_update_btc_codec_info, codec_type 0x%04x", c_type);

    if (c_type == AUDIO_CODEC_SBC_DEC)
    {
        /* Sample frequency configuration */
        switch((*p_info) & A2D_SBC_IE_SAMP_FREQ_MSK)
        {

            case A2D_SBC_IE_SAMP_FREQ_16:
                btapp_ak_cb.btc_codec_info.sbc.sampling_freq = CODEC_INFO_SBC_SF_16K;
                break;
            case A2D_SBC_IE_SAMP_FREQ_32:
                btapp_ak_cb.btc_codec_info.sbc.sampling_freq = CODEC_INFO_SBC_SF_32K;
                break;
            case A2D_SBC_IE_SAMP_FREQ_44:
                btapp_ak_cb.btc_codec_info.sbc.sampling_freq = CODEC_INFO_SBC_SF_44K;
                break;
            case A2D_SBC_IE_SAMP_FREQ_48:
                btapp_ak_cb.btc_codec_info.sbc.sampling_freq = CODEC_INFO_SBC_SF_48K;
                break;
            default:
                APPL_TRACE_ERROR0 ("btapp_avk_update_btc_codec_info: ERROR, SBC wrong sample freq");
                break;
        }

        /*  Chanel mode configuration */
        switch((*p_info) & A2D_SBC_IE_CH_MD_MSK)
        {

            case A2D_SBC_IE_CH_MD_MONO:
                btapp_ak_cb.btc_codec_info.sbc.channel_mode = CODEC_INFO_SBC_CH_MONO;
                break;
            case A2D_SBC_IE_CH_MD_DUAL:
                btapp_ak_cb.btc_codec_info.sbc.channel_mode = CODEC_INFO_SBC_CH_DUAL;
                break;
            case A2D_SBC_IE_CH_MD_STEREO:
                btapp_ak_cb.btc_codec_info.sbc.channel_mode = CODEC_INFO_SBC_CH_STEREO;
                break;
            case A2D_SBC_IE_CH_MD_JOINT:
                btapp_ak_cb.btc_codec_info.sbc.channel_mode = CODEC_INFO_SBC_CH_JS;
                break;
            default:
                APPL_TRACE_ERROR0 ("btapp_avk_update_btc_codec_info: ERROR, SBC wrong ChannelMode");
                break;
        }

        p_info++;
        /* block length */
        switch((*p_info) & A2D_SBC_IE_BLOCKS_MSK)
        {

            case A2D_SBC_IE_BLOCKS_4:
                btapp_ak_cb.btc_codec_info.sbc.block_length = CODEC_INFO_SBC_BLOCK_4;
                break;
            case A2D_SBC_IE_BLOCKS_8:
                btapp_ak_cb.btc_codec_info.sbc.block_length = CODEC_INFO_SBC_BLOCK_8;
                break;
            case A2D_SBC_IE_BLOCKS_12:
                btapp_ak_cb.btc_codec_info.sbc.block_length = CODEC_INFO_SBC_BLOCK_12;
                break;
            case A2D_SBC_IE_BLOCKS_16:
                btapp_ak_cb.btc_codec_info.sbc.block_length = CODEC_INFO_SBC_BLOCK_16;
                break;
            default:
                APPL_TRACE_ERROR0 ("btapp_avk_update_btc_codec_info: ERROR, SBC wrong block len");
                break;
        }

        /* subband */
        switch((*p_info) & A2D_SBC_IE_SUBBAND_MSK)
        {
            case A2D_SBC_IE_SUBBAND_4:
                btapp_ak_cb.btc_codec_info.sbc.num_subbands = CODEC_INFO_SBC_SUBBAND_4;
                break;
            case A2D_SBC_IE_SUBBAND_8:
                btapp_ak_cb.btc_codec_info.sbc.num_subbands = CODEC_INFO_SBC_SUBBAND_8;
                break;
            default:
                APPL_TRACE_ERROR0 ("btapp_avk_update_btc_codec_info: ERROR, SBC wrong subband");
                break;
        }

        /* Allocation method */
        switch((*p_info) & A2D_SBC_IE_ALLOC_MD_MSK)
        {
            case A2D_SBC_IE_ALLOC_MD_S:
                btapp_ak_cb.btc_codec_info.sbc.alloc_method = CODEC_INFO_SBC_ALLOC_SNR;
                break;
            case A2D_SBC_IE_ALLOC_MD_L:
                btapp_ak_cb.btc_codec_info.sbc.alloc_method = CODEC_INFO_SBC_ALLOC_LOUDNESS;
                break;
            default:
                APPL_TRACE_ERROR0 ("btapp_avk_update_btc_codec_info: ERROR, SBC wrong Alloc Method");
                break;
        }

        /* Skip Min bitpool size */
        ++p_info;

        /* Max bitpool size */
        btapp_ak_cb.btc_codec_info.sbc.bitpool_size = *++p_info;

    }
    else if (c_type == AUDIO_CODEC_MP3_DEC)
    {
        /*  Chanel mode configuration */
        switch((*p_info) & A2D_M12_IE_CH_MD_MSK)
        {
            case A2D_M12_IE_CH_MD_MONO:
                btapp_ak_cb.btc_codec_info.mp3.ch_mode   = CODEC_INFO_MP3_MODE_SINGLE;
                break;
            case A2D_M12_IE_CH_MD_DUAL:
                btapp_ak_cb.btc_codec_info.mp3.ch_mode   =  CODEC_INFO_MP3_MODE_DUAL;
                break;
            case A2D_M12_IE_CH_MD_STEREO:
                btapp_ak_cb.btc_codec_info.mp3.ch_mode   =  CODEC_INFO_MP3_MODE_STEREO;
                break;
            case A2D_M12_IE_CH_MD_JOINT:
                btapp_ak_cb.btc_codec_info.mp3.ch_mode   =  CODEC_INFO_MP3_MODE_JS;
                break;
            default:
                APPL_TRACE_ERROR0 ("btapp_avk_update_btc_codec_info: ERROR, MP3 wrong ChannelMode");
                break;
        }

        p_info++;
        /* Sample frequency configuration */
        switch((*p_info) & A2D_M12_IE_SAMP_FREQ_MSK)
        {
            case A2D_M12_IE_SAMP_FREQ_44:
                btapp_ak_cb.btc_codec_info.mp3.sampling_freq = CODEC_INFO_MP3_SF_44K;
                break;
            case A2D_M12_IE_SAMP_FREQ_48:
                btapp_ak_cb.btc_codec_info.mp3.sampling_freq = CODEC_INFO_MP3_SF_48K;
                break;
            case A2D_M12_IE_SAMP_FREQ_32:
                btapp_ak_cb.btc_codec_info.mp3.sampling_freq = CODEC_INFO_MP3_SF_32K;
                break;
            default:
                APPL_TRACE_ERROR0 ("btapp_avk_update_btc_codec_info: ERROR, MP3 wrong sample freq");
                break;
        }

        p_info++;
        BE_STREAM_TO_UINT16(mp3_bitrate, p_info);
        mp3_bitrate &= A2D_M12_IE_BITRATE_MSK;
        switch (mp3_bitrate)
        {
            case A2D_M12_IE_BITRATE_0:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_FREE;
                break;
            case A2D_M12_IE_BITRATE_1:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_32K;
                break;
            case A2D_M12_IE_BITRATE_2:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_40K;
                break;
            case A2D_M12_IE_BITRATE_3:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_48K;
                break;
            case A2D_M12_IE_BITRATE_4:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_56K;
                break;
            case A2D_M12_IE_BITRATE_5:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_64K;
                break;
            case A2D_M12_IE_BITRATE_6:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_80K;
                break;
            case A2D_M12_IE_BITRATE_7:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_96K;
                break;
            case A2D_M12_IE_BITRATE_8:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_112K;
                break;
            case A2D_M12_IE_BITRATE_9:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_128K;
                break;
            case A2D_M12_IE_BITRATE_10:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_160K;
                break;
            case A2D_M12_IE_BITRATE_11:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_192K;
                break;
            case A2D_M12_IE_BITRATE_12:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_224K;
                break;
            case A2D_M12_IE_BITRATE_13:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_256K;
                break;
            case A2D_M12_IE_BITRATE_14:
                btapp_ak_cb.btc_codec_info.mp3.bitrate_index = CODEC_INFO_MP3_BR_IDX_320K;
                break;
            default:
                APPL_TRACE_ERROR1 ("btapp_avk_update_btc_codec_info: ERROR, MP3 wrong BR(0x%02X)", mp3_bitrate);
                break;
        }
    }
}

/*******************************************************************************
**
** Function         btapp_avk_route_audio
**
** Description      Send audio routing command
**
**
** Returns          void
*******************************************************************************/
static void btapp_avk_route_audio(tBTA_AVK_EVT evt)
{
    tBTA_DM_ROUTE_PATH  src_path, out_path;
    tAUDIO_ROUTE_SF src_sf, out_sf, i2s_sf;
    BOOLEAN     unknown_evt = FALSE;

    if (evt == BTA_AVK_START_EVT)
    {
        src_path = BTA_DM_ROUTE_BTSNK;
        out_path = btapp_ak_cb.btc_route;

        if (btapp_ak_cb.btc_codec_type == AUDIO_CODEC_SBC_DEC)
        {
            switch (btapp_ak_cb.btc_codec_info.sbc.sampling_freq)
            {
                case CODEC_INFO_SBC_SF_16K: src_sf = AUDIO_ROUTE_SF_16K;    break;
                case CODEC_INFO_SBC_SF_32K: src_sf = AUDIO_ROUTE_SF_32K;    break;
                case CODEC_INFO_SBC_SF_44K: src_sf = AUDIO_ROUTE_SF_44_1K;  break;
                case CODEC_INFO_SBC_SF_48K: src_sf = AUDIO_ROUTE_SF_48K;    break;
                default:
                    src_sf = AUDIO_ROUTE_SF_NA;
                    break;
            }
        }
        else if (btapp_ak_cb.btc_codec_type == AUDIO_CODEC_MP3_DEC)
        {
            switch (btapp_ak_cb.btc_codec_info.mp3.sampling_freq)
            {
                case CODEC_INFO_MP3_SF_32K: src_sf = AUDIO_ROUTE_SF_32K;    break;
                case CODEC_INFO_MP3_SF_44K: src_sf = AUDIO_ROUTE_SF_44_1K;  break;
                case CODEC_INFO_MP3_SF_48K: src_sf = AUDIO_ROUTE_SF_48K;    break;
                default:
                    src_sf = AUDIO_ROUTE_SF_NA;
                    break;
            }
        }
        else
        {
            src_sf = AUDIO_ROUTE_SF_NA;
        }

        out_sf = btapp_cfg.avk_btc_i2s_rate;
        i2s_sf = btapp_cfg.avk_btc_i2s_rate;

    }
    else if (evt == BTA_AVK_SUSPEND_EVT)
    {
        src_path = BTA_DM_ROUTE_BTSNK;
        out_path = BTA_DM_ROUTE_NONE;
        src_sf = AUDIO_ROUTE_SF_NA;
        out_sf = AUDIO_ROUTE_SF_NA;
        i2s_sf = AUDIO_ROUTE_SF_NA;
    }
    else if (evt == BTA_AVK_CLOSE_EVT)
    {
        src_path = BTA_DM_ROUTE_NONE;
        out_path = BTA_DM_ROUTE_NONE;
        src_sf = AUDIO_ROUTE_SF_NA;
        out_sf = AUDIO_ROUTE_SF_NA;
        i2s_sf = AUDIO_ROUTE_SF_NA;
    }
    else
        unknown_evt = TRUE;

    if (!unknown_evt)
    {
        btapp_dm_route_audio (btapp_ak_cb.rt_handle, src_path, out_path, src_sf, out_sf, i2s_sf);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_audio_route_cback
**
** Description      Callback for AVK to BTA RT module
**
**
** Returns          void
**
*******************************************************************************/
static void btapp_avk_audio_route_cback (tBTA_RT_EVT event, tBTA_RT *p_data)
{
    tAUDIO_ROUTE_OUT route_out;
    UINT8 i, j;

    APPL_TRACE_DEBUG3 ("btapp_avk_audio_route_cback : opcode:%d, status;%d, param:%d",
                       event, p_data->resp.status, p_data->resp.param);

    switch (event)
    {
        case BTA_RT_REGISTER_EVT:
            if (p_data->registr.status == BTA_SUCCESS)
                btapp_ak_cb.rt_handle = p_data->registr.rt_handle;

            APPL_TRACE_DEBUG1("btapp_avk_init: Enabling AVK, features = %d", btapp_cfg.avk_features);
            BTA_AvkEnable(btapp_cfg.avk_security, btapp_cfg.avk_features, btapp_avk_cback);

            break;

        case BTA_RT_SET_AUDIO_CODEC_RESP:
            APPL_TRACE_EVENT0 ("BTA_RT_SET_AUDIO_CODEC_RESP");
            btapp_app_send_mmi_evt (BTAPP_MMI_AVK_CODEC_RESP, p_data->resp.status);
            break;

        case BTA_RT_SET_AUDIO_ROUTE_RESP:
            route_out = (tAUDIO_ROUTE_OUT)(p_data->resp.param & 0x0000FFFF);
            APPL_TRACE_EVENT1 ("BTA_RT_SET_AUDIO_ROUTE_RESP, route_out = 0x%04x", route_out);

            btapp_app_send_mmi_evt (BTAPP_MMI_AVK_ROUTE_RESP, p_data->resp.status);
            break;

        case BTA_RT_READ_AUDIO_INFO:
            APPL_TRACE_EVENT2 ("BTA_RT_READ_AUDIO_INFO, status %d codec_info = 0x%04x",
                                p_data->audio_info.status, p_data->audio_info.codec_info);
            btapp_ak_cb.sup_codec_type = p_data->audio_info.codec_info;
            for ( i = 0; i < MAX_AUDIO_ROUTE_SRC; i++ )
            {
                for ( j = 0; j < MAX_AUDIO_ROUTE_MIX; j++ )
                {
                    btapp_ak_cb.audio_path_table[i][j] = p_data->audio_info.audio_path_table[i][j];
                }
            }

            break;

        default:
            APPL_TRACE_ERROR1 ("Unknown event(%d)", event);
            break;

    }
}
#endif

void btapp_avk_rc_get_info(UINT8 opcode)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;

    cmd.cmd.pdu    = AVRC_PDU_GET_PLAY_STATUS;
    cmd.cmd.opcode = opcode;
    cmd.cmd.status = AVRC_STS_NO_ERROR;

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_STATUS, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_get_capabilities
**
** Description      Get the capabilities supported by peer device.
**
** Parameters:      capability_id: type of capability supported
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_get_capabilities(tAVRC_CAP capability_id){
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;

    cmd.get_caps.pdu = AVRC_PDU_GET_CAPABILITIES;
    cmd.get_caps.capability_id = capability_id;
    cmd.get_caps.status = AVRC_STS_NO_ERROR;

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label, BTA_AVK_CMD_STATUS, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_set_abs_volume
**
** Description      Set abs volume to peer device.
**
** Parameters:      volume: abs volume
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_set_abs_volume(UINT8 volume)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;

    cmd.volume.status = AVRC_STS_NO_ERROR;
    cmd.volume.pdu    = AVRC_PDU_SET_ABSOLUTE_VOLUME;
    cmd.volume.volume = volume;

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
	printf("btapp_avk_rc_set_abs_volume\n");
    if (p_pkt)
    {	
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_avk_get_next_label(), BTA_AVK_CMD_CTRL, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_list_player_app_attr
**
** Description      Request the target device to provide target supported player
**                  application setting attributes.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_list_player_app_attr(void)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;

    cmd.list_app_attr.pdu = AVRC_PDU_LIST_PLAYER_APP_ATTR;
    cmd.list_app_attr.status = AVRC_STS_NO_ERROR;
    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_STATUS, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_get_cur_player_app_value
**
** Description      Request the target device to provide the current set values
**                  on the target for the provided player application setting
**                  attribute list.
**
** Parameters:      -num_player_app_attr_ids: size of the list of player ids.
**                  -p_player_app_atr_id: list of player application ids.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_get_cur_player_app_value(UINT8 num_player_app_attr_ids,
                                           UINT8 *p_player_app_atr_id)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;
    UINT8 i = 0;

    /* Validate number of attributes */
    if (num_player_app_attr_ids > AVRC_MAX_ELEM_ATTR_SIZE)
    {
        APPL_TRACE_ERROR2 ("btapp_avk_rc_get_cur_player_app_value: num_player_app_attr_ids (%i) exceeds maximum (%i)", num_player_app_attr_ids, AVRC_MAX_APP_ATTR_SIZE);
        return;
    }

    cmd.get_cur_app_val.pdu = AVRC_PDU_GET_CUR_PLAYER_APP_VALUE;
    cmd.get_cur_app_val.status = AVRC_STS_NO_ERROR;
    cmd.get_cur_app_val.num_attr = num_player_app_attr_ids;

    while( i < num_player_app_attr_ids)
    {
        cmd.get_cur_app_val.attrs[i] = p_player_app_atr_id[i];
        i++;
    }

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_STATUS, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_get_element_attibutes
**
** Description      Request the TG (target) to provide the attributes of the
**                  element specified.
**
** Parameters:      -id: unique identifier to identify an element on target.
**                   Support just 0x00 (current track playing)
**                  -num_attributes: size of the attribute's id list.
**                  -p_attributes: attribute id list.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_get_element_attibutes(UINT64 id,
                                        UINT8 num_attributes,
                                        tAVRC_MEDIA_ATTR_ID *p_attributes)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;
    UINT8 i = 0;

    /* Validate number of attributes */
    if (num_attributes > AVRC_MAX_ELEM_ATTR_SIZE)
    {
        APPL_TRACE_ERROR2 ("btapp_avk_rc_get_element_attibutes: num_attributes (%i) exceeds maximum (%i)", num_attributes, AVRC_MAX_ELEM_ATTR_SIZE);
        return;
    }

    cmd.get_elem_attrs.pdu = AVRC_PDU_GET_ELEMENT_ATTR;
    cmd.get_elem_attrs.num_attr = num_attributes;
    while (i < num_attributes)
    {
        cmd.get_elem_attrs.attrs[i] = p_attributes[i];
        i++;
    }

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label, BTA_AVK_CMD_STATUS, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_get_play_status
**
** Description      Request status of the currently playing media at target.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_get_play_status(void)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;

    cmd.get_play_status.pdu = AVRC_PDU_GET_PLAY_STATUS;
    cmd.get_play_status.status = AVRC_STS_NO_ERROR;
    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_STATUS, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_reg_notif
**
** Description      Request a register at the target to receive notifications
**                  asynchronously based on specific events occurring.
**
** Parameters:      -event_id: event for which is requested notification.
**                  -playback_interval: just considered together with event
**                   AVRC_EVT_PLAY_POS_CHANGED - Interval in seconds of change
**                   in the playback position to receive a notification.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_reg_notif(tAVRC_EVT event_id, UINT32 playback_interval)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND cmd;

    cmd.reg_notif.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
    cmd.reg_notif.status = AVRC_STS_NO_ERROR;
    cmd.reg_notif.event_id = event_id;
    cmd.reg_notif.param = playback_interval;


    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_NOTIF, p_pkt);
    }
}

void btapp_avk_rc_reg_notif_rsp(tAVRC_EVT event_id, tBTA_AVK_CODE rsp_code)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_RESPONSE rsp;

    rsp.reg_notif.status = AVRC_STS_NO_ERROR;
    rsp.reg_notif.pdu    = AVRC_PDU_REGISTER_NOTIFICATION;
    rsp.reg_notif.opcode = AVRC_OP_VENDOR;
    rsp.reg_notif.event_id = event_id;
    rsp.reg_notif.param.volume = btapp_ak_cb.cur_volume;

    AVRC_BldResponse(btapp_ak_cb.rc_handle, &rsp, &p_pkt);

    if(p_pkt)
    {
        BTA_AvkMetaRsp(btapp_ak_cb.rc_handle, vol_tran_label, rsp_code, p_pkt);
        vol_tran_label = (vol_tran_label + 1) & 0x0F;
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_request_cont_rsp
**
** Description      Request continuing response packets for the sent PDU not
**                  yet completed.
**
** Parameters:      -continue_pdu_id: target PDU_ID for continue command.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_request_cont_rsp(tAVRC_PDU continue_pdu_id)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_NEXT_CMD cmd;

    cmd.pdu = AVRC_PDU_REQUEST_CONTINUATION_RSP;
    cmd.status = AVRC_STS_NO_ERROR;
    cmd.target_pdu = continue_pdu_id;

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_CTRL, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_abort_cont_rsp
**
** Description      Request abort continuing response.
**
** Parameters:      -cont_abort_pdu_id: target PDU_ID for abort continue command.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_abort_cont_rsp(tAVRC_PDU cont_abort_pdu_id)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_NEXT_CMD cmd;

    cmd.pdu = AVRC_PDU_ABORT_CONTINUATION_RSP;
    cmd.status = AVRC_STS_NO_ERROR;
    cmd.target_pdu = cont_abort_pdu_id;

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_CTRL, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_get_folder_items
**
** Description      Retrieve a listing of contents of a folder.
**
** Parameters:      -scope: see avrc spec 6.10.1: List of media player,
**                   file system items, search result or play list items.
**                  -start_item: offset within the list of items.
**                  -end_item: offset within the list of items that should be
**                   the list item returned.
**                  -attr_count:
**                      [0x00] - all attributes are requested (no attr. list).
**                      [0x01-0xFE] - the list contains this number off attr.
**                      [0xff] - no attributes requested (no attr. list).
**                  -p_attr_list: attribute list
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_get_folder_items(tAVRC_SCOPE scope,
                                   UINT32 start_item,
                                   UINT32 end_item,
                                   UINT8 attr_count,
                                   UINT32 *p_attr_list)
{
    BT_HDR *p_pkt = NULL;
    tAVRC_GET_ITEMS_CMD cmd;

    cmd.pdu = AVRC_PDU_GET_FOLDER_ITEMS;
    cmd.status = AVRC_STS_NO_ERROR;
    cmd.scope = scope;

    cmd.start_item = start_item;
    cmd.end_item = end_item;
    cmd.attr_count = attr_count;
    cmd.p_attr_list = p_attr_list;

    AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_STATUS, p_pkt);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_open
**
** Description      RC volume up
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_volume_up(void)
{
    btapp_ak_cb.cur_volume += 5;

    if(btapp_ak_cb.cur_volume > BTAPP_MAX_ABS_VOLUME)
        btapp_ak_cb.cur_volume = BTAPP_MAX_ABS_VOLUME;
	APPL_TRACE_ERROR0("btapp_avk_rc_volume_up!!!");

    btapp_avk_rc_reg_notif_rsp(AVRC_EVT_VOLUME_CHANGE, BTA_AVK_RSP_CHANGED);
}

/*******************************************************************************
**
** Function         btapp_avk_rc_open
**
** Description      RC volume down
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_volume_down(void)
{
    if(btapp_ak_cb.cur_volume < 5)
    {
       btapp_ak_cb.cur_volume = BTAPP_MIN_ABS_VOLUME;
    }
    else
    {
        btapp_ak_cb.cur_volume -= 5;
    }
	APPL_TRACE_ERROR0("btapp_avk_rc_volume_down!!!");

    btapp_avk_rc_reg_notif_rsp(AVRC_EVT_VOLUME_CHANGE, BTA_AVK_RSP_CHANGED);
}

/*******************************************************************************
**
** Function         btapp_avk_rc_open
**
** Description      Open AVRCP connection
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_open(void)
{
    if (!BTAPP_AK_GETSTATUS(BTAPP_AK_ST_RC_CONNECT))
    {
        APPL_TRACE_DEBUG0("btapp_avk_rc_open");
        BTA_AvkOpenRc();
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_acp_open
**
** Description      Open AVRCP connection as acp
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_acp_open(void)
{
    if (!BTAPP_AK_GETSTATUS(BTAPP_AK_ST_RC_CONNECT))
    {
        APPL_TRACE_DEBUG0("btapp_avk_rc_open_acp");
        BTA_AvkOpenRcAcp();
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_close
**
** Description      Close AVRCP connection
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_rc_close(void)
{
    if (BTAPP_AK_GETSTATUS(BTAPP_AK_ST_RC_CONNECT))
    {
        APPL_TRACE_DEBUG1("btapp_avk_rc_close rc_handle %d", btapp_ak_cb.rc_handle);
        BTA_AvkCloseRc(btapp_ak_cb.rc_handle);
    }
}

/*******************************************************************************
**
** Function         btapp_avk_rc_get_total_num_of_items
**
** Description      Retrieve the number of items in a folder
**
** Parameters       scope - Media player List   0x00
**                          Virtual Filesystem  0x01
**                          Search              0x02
**                          Now Playing         0x03
**
**
** Returns          void
*******************************************************************************/
#if (AVRC_1_6_INCLUDED == TRUE)
void btapp_avk_rc_get_total_num_of_items(UINT8 scope)
{

    BT_HDR *p_pkt = NULL;
    tAVRC_COMMAND command;
    APPL_TRACE_DEBUG1("btapp_avk_rc_get_total_num_of_items scope %d", scope);
    command.get_num_of_items.pdu = AVRC_PDU_GET_TOTAL_NUM_OF_ITEMS;
    command.get_num_of_items.scope = scope;
    command.get_num_of_items.status = AVRC_STS_NO_ERROR;
    AVRC_BldCommand((tAVRC_COMMAND *)&command, &p_pkt);
    if (p_pkt)
    {
        BTA_AvkMetaCmd(btapp_ak_cb.rc_handle, btapp_ak_cb.label++, BTA_AVK_CMD_STATUS, p_pkt);
    }

}
#endif /* #if (AVRC_1_6_INCLUDED == TRUE) */

/*******************************************************************************
**
** Function         btapp_avk_evt_to_str
**
** Description      This function convert the event code to string
**
** Returns          char *
**
*******************************************************************************/
static char *btapp_avk_evt_to_str(tBTA_AVK_EVT evt_code)
{
    switch(evt_code)
    {
    case BTA_AVK_ENABLE_EVT:        return "BTA_AVK_ENABLE_EVT";
    case BTA_AVK_REGISTER_EVT:      return "BTA_AVK_REGISTER_EVT";
    case BTA_AVK_SIG_OPEN_EVT:      return "BTA_AVK_SIG_OPEN_EVT";
    case BTA_AVK_SIG_CLOSE_EVT:     return "BTA_AVK_SIG_CLOSE_EVT";
    case BTA_AVK_OPEN_EVT:          return "BTA_AVK_OPEN_EVT";
    case BTA_AVK_CLOSE_EVT:         return "BTA_AVK_CLOSE_EVT";
    case BTA_AVK_START_EVT:         return "BTA_AVK_START_EVT";
    case BTA_AVK_STOP_EVT:          return "BTA_AVK_STOP_EVT";
    case BTA_AVK_PROTECT_REQ_EVT:   return "BTA_AVK_PROTECT_REQ_EVT";
    case BTA_AVK_PROTECT_RSP_EVT:   return "BTA_AVK_PROTECT_RSP_EVT";
    case BTA_AVK_RC_OPEN_EVT:       return "BTA_AVK_RC_OPEN_EVT";
    case BTA_AVK_RC_CLOSE_EVT:      return "BTA_AVK_RC_CLOSE_EVT";
    case BTA_AVK_REMOTE_CMD_EVT:    return "BTA_AVK_REMOTE_CMD_EVT";
    case BTA_AVK_REMOTE_RSP_EVT:    return "BTA_AVK_REMOTE_RSP_EVT";
    case BTA_AVK_VENDOR_CMD_EVT:    return "BTA_AVK_VENDOR_CMD_EVT";
    case BTA_AVK_VENDOR_RSP_EVT:    return "BTA_AVK_VENDOR_RSP_EVT";
    case BTA_AVK_RECONFIG_EVT:      return "BTA_AVK_RECONFIG_EVT";
    case BTA_AVK_SUSPEND_EVT:       return "BTA_AVK_SUSPEND_EVT";
    case BTA_AVK_UPDATE_SEPS_EVT:   return "BTA_AVK_UPDATE_SEPS_EVT";
    case BTA_AVK_META_MSG_EVT:      return "BTA_AVK_META_MSG_EVT";
    case BTA_AVK_RC_CMD_TOUT_EVT:   return "BTA_AVK_RC_CMD_TOUT_EVT";
    case BTA_AVK_RC_PEER_FEAT_EVT:  return "BTA_AVK_RC_PEER_FEAT_EVT";
    case BTA_AVK_BROWSE_MSG_EVT:    return "BTA_AVK_BROWSE_MSG_EVT";
#if (BTU_BTC_SNK_INCLUDED == TRUE)
    case BTA_AVK_RECONFIG_PENDING:  return "BTA_AVK_RECONFIG_PENDING";
#endif
    default:                        return "unknown";
    }
}

/*******************************************************************************
**
** Function         btapp_avk_open_rc_timer_cback
**
** Description      This function is called after rc timer expires and try
**                  to open AVRCP if it is not opened
**
** Returns          void
**
*******************************************************************************/
static void btapp_avk_open_rc_timer_cback(void *p_tle)
{
    APPL_TRACE_DEBUG1("btapp_avk_open_rc_timer_cback rc_handle=%d", btapp_ak_cb.rc_handle);

    /* Check whether AVRCP is already opened */
    if (btapp_ak_cb.rc_handle == BTA_AVK_RC_HANDLE_NONE)
        BTA_AvkOpenRc();
}


#if (BRCM_LPST_INCLUDED == TRUE)

static BOOLEAN btapp_avk_lpst_cmd_handler(UINT8 op_code, BT_HDR *p_buf)
{
    UINT8 subcode;
    UINT8 *p =  (UINT8 *)(p_buf + 1) + p_buf->offset;
    BD_ADDR bda;

    APPL_TRACE_DEBUG1("btapp_avk_lpst_cmd_handler op_code=%d", op_code);

    if (op_code == BTAPP_AVK_APP_EVENT)
    {
        STREAM_TO_UINT8(subcode, p);
        switch (subcode)
        {
        case BTAPP_AVK_APP_SUBCODE_OPEN:
            STREAM_TO_BDADDR (bda, p);
            BTA_AvkOpen (bda, BTA_AVK_CHNL_AUDIO, btapp_cfg.avk_security);
            break;

        case BTAPP_AVK_APP_SUBCODE_CLOSE:
            BTA_AvkClose ();
            break;

        case BTAPP_AVK_APP_SUBCODE_START:
            break;
        default:
            break;
        }

        return TRUE;
    }

    return FALSE;
}


/*******************************************************************************
**
** Function         btapp_avk_lpst_event_handler
**
** Description      This function is called for LPST events
**
** Returns          void
**
*******************************************************************************/
static void btapp_avk_lpst_event_handler (UINT8 event, tBTM_LPST_EVENT_DATA *p_data)
{
    APPL_TRACE_DEBUG1("btapp_avk_lpst_event_handler event=%d", event);
    switch (event)
    {
    case BTM_LPST_AVK_MEDIA_START_EVT:
        APPL_TRACE_DEBUG0("AVK_MEDIA_START");
        break;

    case BTM_LPST_AVK_SYNC_READY_EVT:
        APPL_TRACE_DEBUG0("AVK_SYNC_READY");
        break;

    case BTM_LPST_AVK_STREAM_MODE_EVT:
        APPL_TRACE_DEBUG1("AVK_STREAM_MODE: %d", p_data->num_streams);
        break;

    default:
        APPL_TRACE_DEBUG0("LPST unknown event");
        break;
    }
}

/*******************************************************************************
**
** Function         btapp_avk_lpst_forward_event
**
** Description      This function forward an event to the give device
**
** Returns          TRUE if sent
**
*******************************************************************************/
static BOOLEAN btapp_avk_lpst_forward_event(UINT8 sub_code, BD_ADDR bda)
{
    UINT16 len = sizeof(BT_HDR) + (L2CAP_MIN_OFFSET + 1) + 1/* subcode */ + BD_ADDR_LEN;
    BT_HDR *p_buf;
    UINT8   *pp;
    UINT8 local_role = btm_lpst_get_local_device_role();
    BOOLEAN res = FALSE;

    APPL_TRACE_DEBUG2("btapp_avk_lpst_forward_event sub_code=%d local_role:%d",
        sub_code, local_role);
    if (local_role == BTM_LPST_ROLE_SE)
    {
        p_buf = (BT_HDR *) GKI_getbuf(len);
        if (!p_buf)
            return FALSE;
        p_buf->offset = L2CAP_MIN_OFFSET + 1;
        p_buf->len = 1 + BD_ADDR_LEN;
        pp = (UINT8 *)(p_buf + 1) + p_buf->offset;
        UINT8_TO_STREAM(pp, sub_code);
        BDADDR_TO_STREAM (pp, bda);

        res = btm_lpst_send_generic_data_to_bridge(BTAPP_AVK_APP_EVENT, p_buf);
        if (!res)
            GKI_freebuf(p_buf);
#if (BRCM_LPST_INCLUDED == TRUE)
    }
    return res;
#endif
}
#endif

#endif
