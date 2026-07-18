/****************************************************************************
**
**  Name:          btapp_dm.c
**
**  Description:   contains  device manager application
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "bta_platform.h"
#include "bte_glue.h"
#include "btapp_nv.h"
#if( defined BTA_PBC_INCLUDED ) && (BTA_PBC_INCLUDED == TRUE)
#include "btapp_pbc.h"
#endif
#if( defined BTA_AG_INCLUDED ) && (BTA_AG_INCLUDED == TRUE)
#include "btapp_ag.h"
#endif
#if( defined BTA_AV_INCLUDED ) && (BTA_AV_INCLUDED == TRUE)
#include "btapp_av.h"
#endif

#ifndef BTAPP_DM_FIND_ME_TIME
#define BTAPP_DM_FIND_ME_TIME       62  /* Tgap(104) more than 1 min */
#endif

#ifndef BTAPP_DM_LIMITED_RSSI_OFFSET
#define BTAPP_DM_LIMITED_RSSI_OFFSET    30
#endif

#ifndef BTAPP_DM_DI_DB_SIZE
#define BTAPP_DM_DI_DB_SIZE      1024
#endif

#define BTAPP_DEFAULT_PRODUCT_ID      0x1200      /* BCM1200_ (BTE SW) */
#define BTAPP_MAJ_VER_STR_OFFSET      16          /* 'BCM1200_xx_10.3.' */
#define BTAPP_MIN_VER_STR_OFFSET      19          /* 'BCM1200_xx_10.3.' */

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_dm_security_cback (tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data);
void btapp_add_devices(void);
static void btapp_discover_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data);
void btapp_search_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data);
void btapp_search_cb_ext(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data);

tBTAPP_CB btapp_cb;

/* newly found devices */
tBTAPP_INQ_DB btapp_inq_db;
/* device database */
tBTAPP_DEV_DB btapp_device_db;

const UINT8 *btapp_dm_device_type[4] = {(UINT8 *)"U", (UINT8 *)"B", (UINT8 *)"L", (UINT8 *)"D"};

/* Buffer for parsing/building avrc messages for btapp_av, btapp_rc, and btapp_avk */
UINT8 btapp_dm_avrc_buf[BTAPP_DM_AVRC_BUF_SIZE];

#define TARGET_BLE_SCAN_ROUNDS  9999
#define REPEAT_BLE_SCAN_PERIOD  3000
UINT32 ble_scan_rounds = 0;
UINT32 last_ble_scan_rounds = 0;

#if BLE_INCLUDED == TRUE
/* BLE local key information */
tBTAPP_BLE_INFO btapp_ble_local_key;
static const UINT8  base_uuid[LEN_UUID_128] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
/* need to expand when new service is added,(1<< index) must match tBTAPP_BLE_SERVICE_MASK definition */
static const UINT16 service_uuid_16[BTAPP_BLE_SERVICE_BIT_MAX] =
{
    UUID_SERVCLASS_GAP_SERVER,      /* 0 */
    UUID_SERVCLASS_GATT_SERVER,     /* 1 */
    UUID_SERVCLASS_IMMEDIATE_ALERT, /* 2 */
    UUID_SERVCLASS_LINKLOSS,        /* 3 */
    UUID_SERVCLASS_TX_POWER,        /* 4 */
    UUID_SERVCLASS_CURRENT_TIME ,   /* 5 */
    UUID_SERVCLASS_DST_CHG ,        /* 6 */
    UUID_SERVCLASS_REF_TIME_UPD,    /* 7 */
    UUID_SERVCLASS_NWA   ,          /* 8 */
    UUID_SERVCLASS_PHALERT,         /* 9 */
    UUID_SERVCLASS_GLUCOSE,         /* 10 */
    UUID_SERVCLASS_DEVICE_INFO,     /* 11 */
    UUID_SERVCLASS_LE_HID,          /* 12 */
    UUID_SERVCLASS_BATTERY,         /* 13 */
    UUID_SERVCLASS_SCAN_PARAM,      /* 14 */
    UUID_SERVCLASS_RSC,             /* 15 */
    UUID_SERVCLASS_CSC,             /* 16 */
    UUID_SERVCLASS_CP               /* 17 */
};
#endif

btapp_dm_oob_cb_t oob_cb;
btapp_dm_oob_cb_t loc_oob_cb;
extern SemaphoreHandle_t bt_stack_sync_sema;

static pthread_t rssi_task;
static void* rssi_task_main(void* arg);
static UINT8 read_rssi_task_enable = 0;
static BD_ADDR conn_bda;

static bool is_empty_128bit(uint8_t* data)
{
  static const uint8_t zero[16] = {0};
  return !memcmp(zero, data, sizeof(zero));
}

/*******************************************************************************
**
** Function         btapp_startup
**
** Description      Initializes bt application and waits for device to come up
**
** Returns          void
*******************************************************************************/
void btapp_startup(void)
{
    memset(&btapp_device_db,0x00,sizeof(btapp_device_db));

    /* read all parmeters stored in nvram */
    btapp_init_device_db();
}

/*******************************************************************************
**
** Function         btapp_dm_init
**
** Description      Initializes Device manger
**
** Returns          void
*******************************************************************************/
void btapp_dm_init(void)
{
    BTA_BrcmInit();

    /* enable bluetooth before calling other BTA API */
    BTA_EnableBluetooth(&btapp_dm_security_cback);
}

static void btapp_dm_set_privacy_cb(UINT8 status)
{
    APPL_TRACE_DEBUG1("btapp_dm_set_privacy_cb status = %d", status);
}

/*******************************************************************************
**
** Function         btapp_dm_init_continue
**
** Description      Finishes dm init after enabling bluetooth.
**
** Returns          void
*******************************************************************************/
static void btapp_dm_init_continue(void)
{
    /* Set pairability to be TRUE by default */
    btapp_device_db.pairability = TRUE;

#if BLE_INCLUDED == TRUE
    APPL_TRACE_DEBUG1("btapp_dm_init_continue privacy_enabled = %d", btapp_cfg.privacy_enabled);
    BTA_DmBleConfigLocalPrivacy(btapp_cfg.privacy_enabled, btapp_dm_set_privacy_cb);
#endif

    /* set local bluetooth name */
#if ((defined BTA_SET_DEVICE_NAME) && (BTA_SET_DEVICE_NAME == TRUE))
    uint8_t i = 0;
    uint8_t mac[6] = "";
    char name[32] = "EY_";
    char tmp[4] = "";
    //set display neme
    BTM_GetLocalDeviceAddr(mac);
    for (i = 0; i < 6; i++)
    {
	  snprintf(tmp, 4, "%02X", mac[i]);
      strlcat(name, tmp,sizeof(name));
    }
    printf("set display name: %s\n", name);
    btapp_dm_set_local_name(name);

    //strcat(name, "_LE");
    btapp_dm_set_local_name_le(name);
#endif

    /* add devices from nv data base to BTA */
    btapp_add_devices();

#if ((defined BR_INCLUDED) && (BR_INCLUDED == TRUE))
    /* set visibility and connectability */
    if (btapp_device_db.visibility)
        BTA_DmSetVisibility(BTA_DM_GENERAL_DISC, BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);
    else
        BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);

    /* set Pairability */
    if (btapp_device_db.pairability)
        BTA_DmSetVisibility( BTA_DM_IGNORE, BTA_DM_IGNORE, BTA_DM_PAIRABLE , BTA_DM_IGNORE);
    else
        BTA_DmSetVisibility( BTA_DM_IGNORE, BTA_DM_IGNORE, BTA_DM_NON_PAIRABLE  , BTA_DM_IGNORE);
#endif

#if BLE_INCLUDED == TRUE /* by default to enable LE scatternet */
    btapp_dm_set_le_scatternet(TRUE);

    btapp_dm_set_le_visibility(BTA_DM_BLE_NON_DISCOVERABLE, BTA_DM_NON_CONN, TRUE);
#endif
}

static void bta_dm_version_print(char* fmt, ...)
{
    char tmp_buf[256];
    va_list args;
    va_start( args, fmt );
    /* Add the log message to the logging buffer. */
    vsprintf( &tmp_buf[0], fmt, args );
    va_end( args );

    BRCM_PLATFORM_TRACE("%s"LINE_ENDING, tmp_buf);
}

void btapp_dm_post_reset(void)
{
    btapp_cfg_trace_enable(1);
    btapp_cfg_init();
    //Init BT components
    btapp_startup();
    btapp_dm_init();

    bte_version_print(bta_dm_version_print);

    if (bt_stack_sync_sema != NULL)
    {
       // gki_openrtos_set_semaphore(bt_stack_sync_sema);
		 gki_openrtos_set_semaphore(&bt_stack_sync_sema);
    }
}

/*******************************************************************************
**
** Function         btapp_dm_pin_code_reply
**
** Description      Process the passkey entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_pin_code_reply(BOOLEAN accept, UINT8 pin_len,
                             UINT8 *p_pin)

{
    BTA_DmPinReply( btapp_cb.peer_bdaddr,
                    accept,
                    pin_len,
                    (UINT8*)p_pin);
}

/*******************************************************************************
**
** Function         btapp_dm_confirm_reply
**
** Description      Process the confirm/reject entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_confirm_reply(BOOLEAN accept)
{
    BTA_DmConfirm( btapp_cb.peer_bdaddr, accept);
}

/*******************************************************************************
**
** Function         btapp_dm_passkey_cancel
**
** Description      Process the passkey cancel entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_passkey_cancel(void)
{
    BTA_DmPasskeyCancel(btapp_cb.peer_bdaddr);
}

/*******************************************************************************
**
** Function         btapp_dm_passkey_reply
**
** Description      Send passkey reply.
**
** Returns          void
*******************************************************************************/
void btapp_dm_passkey_reply(tBTM_STATUS res, BD_ADDR bd_addr, UINT32 passkey)
{
    BTA_DmPasskeyReqReply(res, bd_addr, passkey);
}

/*******************************************************************************
**
** Function         btapp_dm_proc_io_req
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_proc_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                          tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig)
{
    UINT8   dd_bit = (btapp_cfg.sp_auth_req & BTAPP_AUTH_REQ_DD_BIT);
    UINT8   yes_no_bit = BTA_AUTH_SP_YES & *p_auth_req;

    if (0 == bdcmp(btapp_cb.oob_bdaddr, bd_addr))
    {
#if BTM_OOB_INCLUDED == TRUE && BTM_BR_SC_INCLUDED == TRUE
        if (*p_oob_data != BTAPP_OOB_PRESENT_192_256)
        {
            *p_oob_data = BTAPP_OOB_PRESENT;
        }
#else
        *p_oob_data = TRUE;
#endif
    }
    else
    {
        *p_oob_data = FALSE;
    }

    APPL_TRACE_DEBUG3("btapp_dm_proc_io_req cfg:%d, auth_req:%d, oob:%d",
                      btapp_cfg.sp_auth_req, *p_auth_req, *p_oob_data);

    *p_io_cap = btapp_cfg.sp_loc_io_caps;

    switch (btapp_cfg.sp_auth_req)
    {
        case BTAPP_AUTH_REQ_NO:  /* 0:not required */
        case BTAPP_AUTH_REQ_YES: /* 1:required */
            *p_auth_req = btapp_cfg.sp_auth_req;
            break;
        case BTAPP_AUTH_REQ_GEN_BOND: /* 2:use default + general bonding DD=NO*/
        case BTAPP_AUTH_REQ_GEN_BOND_DD:/* 4:use default + general bonding DD=YES*/
            /* the new cswg discussion wants us to indicate the bonding bit */
            if (btapp_get_device_record(bd_addr))
            {
                if (btapp_cb.is_dd_bond)
                {
                    /* if initing/responding to a dedicated bonding, use dedicate bonding bit */
                    if (dd_bit)
                        *p_auth_req = BTA_AUTH_DD_BOND | BTA_AUTH_SP_YES;
                    else
                        *p_auth_req = BTA_AUTH_DD_BOND;
                }
                else
                {
                    *p_auth_req = BTA_AUTH_GEN_BOND | yes_no_bit; /* set the general bonding bit for stored device */
                }
            }
            break;
        default:/*and BTAPP_AUTH_REQ_DEFAULT 3:use default */
            if (btapp_cb.is_dd_bond)
            {
                /* if initing/responding to a dedicated bonding, use dedicate bonding bit */
                if (dd_bit)
                    *p_auth_req = BTA_AUTH_DD_BOND | BTA_AUTH_SP_YES;
                else
                    *p_auth_req = BTA_AUTH_DD_BOND;
            }
            break;
    }
    APPL_TRACE_DEBUG2("auth_req:0x%02x, io_caps:%d", *p_auth_req, *p_io_cap);
}

/*******************************************************************************
**
** Function         btapp_dm_proc_io_rsp
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_proc_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap, tBTA_AUTH_REQ auth_req)
{
    tBTAPP_REM_DEVICE    *p_device_rec;
    p_device_rec = btapp_get_device_record(bd_addr);
    if (p_device_rec)
    {
        p_device_rec->peer_io_cap = io_cap;
    }
    if (auth_req & BTA_AUTH_BONDS)
    {
        if (auth_req & BTA_AUTH_DD_BOND)
            btapp_cb.is_dd_bond = TRUE;
        /* store the next generator link key */
        bdcpy(btapp_cb.sp_bond_bdaddr, bd_addr);
        btapp_cb.sp_io_cap = io_cap;
        btapp_cb.sp_bond_bits = (auth_req & BTA_AUTH_BONDS);
    }
    APPL_TRACE_DEBUG2("btapp_dm_proc_io_rsp auth_req:%d, is_dd_bond:%d",
                      auth_req, btapp_cb.is_dd_bond);
}

/*******************************************************************************
**
** Function         btapp_dm_proc_lk_upgrade
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_proc_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade)
{
    if (btapp_cfg.sp_auth_req == BTAPP_AUTH_REQ_NO)
    {
        /* if hard coded to use no MITM, do not upgrade the link key */
        *p_upgrade = FALSE;
    }
}

/*******************************************************************************
**
** Function         btapp_dm_rmt_oob_reply
**
** Description      Process the hash C, randomizer r/reject entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_rmt_oob_reply(BOOLEAN accept, BT_OCTET16 c, BT_OCTET16 r)
{
    APPL_TRACE_API0("calling bta_dm_ci_rmt_oob");
    bta_dm_ci_rmt_oob(accept, btapp_cb.peer_bdaddr, c, r);
}

#if (BTM_BR_SC_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_dm_rmt_oob_ext_reply
**
** Description      Process {reject / {hash C, randomizer r} pairs for P-192, P-256}
**                  entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_rmt_oob_ext_reply(BOOLEAN accept, BT_OCTET16 c_192, BT_OCTET16 r_192,
                                BT_OCTET16 c_256, BT_OCTET16 r_256)
{
    APPL_TRACE_API0("calling bta_dm_ci_rmt_oob");
    bta_dm_ci_rmt_oob_ext(accept, btapp_cb.peer_bdaddr, c_192, r_192, c_256, r_256);
}
#endif

tBTA_STATUS btapp_dm_create_oob_bond(BD_ADDR bd_addr, int transport, btapp_dm_out_of_band_data_t* oob_data)
{
    tBTA_STATUS st = BTA_SUCCESS;

    if(btapp_cb.is_bonding == TRUE)
    {
        st = BTA_BUSY;
    }
    else
    {
        //memcpy(oob_cb.bdaddr, bd_addr, sizeof(bd_addr));
		memcpy(oob_cb.bdaddr, bd_addr, sizeof(UINT8)*BD_ADDR_LEN);
        memcpy(&oob_cb.oob_data, oob_data, sizeof(btapp_dm_out_of_band_data_t));

        BTA_DmBondByTransport(bd_addr, transport);
    }

    return st;
}

/*******************************************************************************
**
** Function         btapp_dm_loc_oob
**
** Description      Read OOB data from local LM
**
** Returns          void
*******************************************************************************/
void  btapp_dm_loc_oob(void)
{
#if (BTM_OOB_INCLUDED == TRUE)
    BTA_DmLocalOob();
#else
    APPL_TRACE_ERROR0("BTM_OOB_INCLUDED is FALSE!!(btapp_dm_loc_oob)");
#endif
}

void btapp_dm_proc_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r)
{
    if(valid)
    {
        memcpy(loc_oob_cb.oob_data.le_sc_c, c, sizeof(BT_OCTET16));
        memcpy(loc_oob_cb.oob_data.le_sc_r, r, sizeof(BT_OCTET16));
    }
}

void btapp_dm_proc_loc_oob_ext(BOOLEAN valid, BT_OCTET16 c_192, BT_OCTET16 r_192,
                                          BT_OCTET16 c_256, BT_OCTET16 r_256)
{
    if(valid)
    {
        memcpy(loc_oob_cb.oob_data.c192, c_192, sizeof(BT_OCTET16));
        memcpy(loc_oob_cb.oob_data.r192, r_192, sizeof(BT_OCTET16));
        memcpy(loc_oob_cb.oob_data.c256, c_256, sizeof(BT_OCTET16));
        memcpy(loc_oob_cb.oob_data.r256, r_256, sizeof(BT_OCTET16));
    }
}

void btapp_dm_set_oob_for_le_io_req(const BD_ADDR bd_addr, tBTA_OOB_DATA* p_has_oob_data, tBTA_LE_AUTH_REQ* p_auth_req)
{
    if (!is_empty_128bit(oob_cb.oob_data.le_sc_c) && !is_empty_128bit(oob_cb.oob_data.le_sc_r))
    {
        /* We have LE SC OOB data */

        /* make sure OOB data is for this particular device */
        if (bd_addr == oob_cb.bdaddr)
        {
            *p_auth_req = ((*p_auth_req) | BTM_LE_AUTH_REQ_SC_ONLY);
            *p_has_oob_data = true;
        }
        else
        {
            *p_has_oob_data = false;
            APPL_TRACE_WARNING1("%s: remote address didn't match OOB data address",
                                __func__);
        }
    }
    else if (!is_empty_128bit(oob_cb.oob_data.sm_tk))
    {
        /* We have security manager TK */

        /* make sure OOB data is for this particular device */
        if (bd_addr == oob_cb.bdaddr)
        {
            // When using OOB with TK, SC Secure Connections bit must be disabled.
            tBTA_LE_AUTH_REQ mask = ~BTM_LE_AUTH_REQ_SC_ONLY;
            *p_auth_req = ((*p_auth_req) & mask);

            *p_has_oob_data = true;
        }
        else
        {
            *p_has_oob_data = false;
            APPL_TRACE_WARNING1("%s: remote address didn't match OOB data address",
                                __func__);
        }
     }
     else
     {
        *p_has_oob_data = false;
     }

     APPL_TRACE_DEBUG2("%s *p_has_oob_data=%d", __func__, *p_has_oob_data);
}

/*******************************************************************************
**
** Function         btapp_dm_authorize_resp
**
** Description      Action function to process auth reply
**
** Returns          void
*******************************************************************************/
void btapp_dm_authorize_resp(tBTA_AUTH_RESP response)
{

    tBTAPP_REM_DEVICE * p_device_rec;
    tBTAPP_REM_DEVICE  device_rec;

    if (response == BTA_DM_AUTH_PERM)
    {
        if ((p_device_rec = btapp_get_device_record(btapp_cb.peer_bdaddr))!= NULL)
        {
            p_device_rec->is_trusted = TRUE;
            p_device_rec->trusted_mask |= (1<<btapp_cb.peer_service);
			 APPL_TRACE_EVENT0("btapp_dm_authorize_resp1");
            btapp_store_device(p_device_rec);
        }
        else
        {
            memset(&device_rec, 0, sizeof(device_rec));
            device_rec.trusted_mask  = (1<<btapp_cb.peer_service);
            strncpy(device_rec.name, btapp_cb.peer_name, BTAPP_DEV_NAME_LENGTH);
            bdcpy(device_rec.bd_addr, btapp_cb.peer_bdaddr);
            device_rec.is_trusted = TRUE;
			APPL_TRACE_EVENT0("btapp_dm_authorize_resp2");
            btapp_store_device(&device_rec);
        }

//        BTA_DmAuthorizeReply(btapp_cb.peer_bdaddr, btapp_cb.peer_service,
//                             BTA_DM_AUTH_PERM);
    }
    else if (response == BTA_DM_AUTH_TEMP)
    {
//        BTA_DmAuthorizeReply(btapp_cb.peer_bdaddr, btapp_cb.peer_service,
//                             BTA_DM_AUTH_TEMP);

    }
    else
    {
//        BTA_DmAuthorizeReply(btapp_cb.peer_bdaddr, btapp_cb.peer_service,
//                             BTA_DM_NOT_AUTH);
    }

}
/*******************************************************************************
**
** Function         btapp_dm_disable_bt
**
** Description      Disables Bluetooth.
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_disable_bt()
{
    BTA_DisableBluetooth();
}

/*******************************************************************************
**
** Function         btapp_dm_enable_test_mode
**
** Description      Eenable test mode.
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_enable_test_mode()
{
    BTA_EnableTestMode();
}

/*******************************************************************************
**
** Function         btapp_dm_add_custom_uuid
**
** Description      Add custom UUID in EIR
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_add_custom_uuid (tBT_UUID *p_uuid)
{
    BTA_DmEirAddUUID (p_uuid);
}

/*******************************************************************************
**
** Function         btapp_dm_remove_custom_uuid
**
** Description      Remove custom UUID in EIR
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_remove_custom_uuid (tBT_UUID *p_uuid)
{
    BTA_DmEirRemoveUUID (p_uuid);
}

/*******************************************************************************
**
** Function         btapp_dm_disable_test_mode
**
** Description      Disables test mode.
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_disable_test_mode()
{
    BTA_DisableTestMode();
}

#if BT_BRCM_VS_INCLUDED == TRUE && BLE_BRCM_INCLUDED == TRUE
/*******************************************************************************
**
** Function         btapp_dm_set_le_visibility
**
** Description      Sets LE scatternet capability
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_le_scatternet( BOOLEAN enable)
{
    BTA_DmBleScatternetEnable( enable);
}
/*******************************************************************************
**
** Function         btapp_dm_set_le_dynscan
**
** Description      Sets LE dynamic scan capability
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_le_dynscan( BOOLEAN enable)
{
    BTA_DmBleDynamicScanEnable (enable);
}
#endif
/*******************************************************************************
**
** Function         btapp_dm_set_pairability
**
** Description      Sets pairability
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_pairability( BOOLEAN is_pairable, BOOLEAN is_temp)
{

    BTA_DmSetVisibility( BTA_DM_IGNORE, BTA_DM_IGNORE, (BOOLEAN)((is_pairable) ? BTA_DM_PAIRABLE : BTA_DM_NON_PAIRABLE), BTA_DM_IGNORE);

    btapp_device_db.pairability = is_pairable;


    if (!is_temp)
    {
        /* update to nvram */

    }

}

/*******************************************************************************
**
** Function         btapp_dm_set_connectability_paired_only
**
** Description      Sets pairability of the paired device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_connectability_paired_only( BOOLEAN conn_paired_only, BOOLEAN is_temp)
{

    BTA_DmSetVisibility( BTA_DM_IGNORE, BTA_DM_IGNORE, BTA_DM_IGNORE,(BOOLEAN)((conn_paired_only) ? BTA_DM_CONN_PAIRED : BTA_DM_CONN_ALL) );

    btapp_device_db.conn_paired_only = conn_paired_only;

    if (!is_temp)
    {
        /* update to nvram */

    }
}

/*******************************************************************************
**
** Function         btapp_dm_timer_cback
**
** Description      Timer used to end find-me disconverable mode
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dm_timer_cback(void *p_tle)
{
    APPL_TRACE_EVENT1(" Find me mode ends. vis: %d", btapp_device_db.visibility);
    /* go back to the previous DISC mode */
    btapp_dm_set_visibility(btapp_device_db.visibility, TRUE);
}

/*******************************************************************************
**
** Function         btapp_dm_set_find_me
**
** Description      Sets visibilty
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_find_me( void)
{
    btapp_device_db.dev_tle.p_cback = btapp_dm_timer_cback;
    btapp_start_timer(&btapp_device_db.dev_tle, 0, BTAPP_DM_FIND_ME_TIME);

    BTA_DmSetVisibility( BTA_DM_LIMITED_DISC, BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);
}

/*******************************************************************************
**
** Function         btapp_dm_set_local_name
**
** Description      Sets local name
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_local_name(char *p_name)
{
    BTA_DmSetDeviceName(p_name);
    strncpy(btapp_device_db.local_device_name, p_name, BTAPP_DEV_NAME_LENGTH);
    /* update to nv memory */

}

void btapp_dm_get_local_name(char *p_name)
{
	strncpy(p_name, btapp_device_db.local_device_name, BTAPP_DEV_NAME_LENGTH);
}


BOOLEAN btapp_dm_is_enabled()
{
	return btapp_device_db.bt_enabled;
}
/*******************************************************************************
**
** Function         btapp_dm_set_local_name_le
**
** Description      Sets LE local name
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_local_name_le(char *p_name)
{
    BTA_DmSetDeviceNameLE(p_name);
}
/*******************************************************************************
**
** Function         btapp_dm_close_acl
**
** Description      remove ACL link
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_close_acl(BD_ADDR remote_bda, BOOLEAN remove_dev, tBTA_TRANSPORT transport)
{
    BTA_DmCloseACL(remote_bda, remove_dev, transport);

    btapp_delete_device(remote_bda);
}

/*******************************************************************************
**
** Function         btapp_dm_sec_add_device
**
** Description      Sets device as trusted
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_sec_add_device(tBTAPP_REM_DEVICE *p_device_rec)
{
#if BLE_INCLUDED == TRUE
    char                msg_str[64];
#endif
    /* update BTA with new settings */
    if (p_device_rec->link_key_present)
    {
        BTA_DmAddDevice(p_device_rec->bd_addr, p_device_rec->dev_class,
                        p_device_rec->link_key, p_device_rec->trusted_mask,
                        p_device_rec->is_trusted, p_device_rec->key_type,
                        p_device_rec->peer_io_cap);
    }
    else
    {
        BTA_DmAddDevice(p_device_rec->bd_addr, p_device_rec->dev_class, NULL,
                        p_device_rec->trusted_mask, p_device_rec->is_trusted,
                        p_device_rec->key_type, p_device_rec->peer_io_cap);
    }

#if BLE_INCLUDED == TRUE
    {
        APPL_TRACE_DEBUG1("from NV p_device_rec->device_type = %d", p_device_rec->device_type);
        if (p_device_rec->device_type == BT_DEVICE_TYPE_BLE ||
            p_device_rec->device_type == BT_DEVICE_TYPE_DUMO)
        {
            APPL_TRACE_DEBUG0("add an LE device");
            sprintf (msg_str, "  %02x:%02x:%02x:%02x:%02x:%02x \n",
                         p_device_rec->bd_addr[0], p_device_rec->bd_addr[1],
                         p_device_rec->bd_addr[2], p_device_rec->bd_addr[3],
                         p_device_rec->bd_addr[4], p_device_rec->bd_addr[5]);

            APPL_TRACE_DEBUG0(msg_str);

            BTA_DmAddBleDevice(p_device_rec->bd_addr, p_device_rec->addr_type, p_device_rec->auth_mode, p_device_rec->device_type);

            if (p_device_rec->key_mask & BTA_LE_KEY_PENC)
            {
                BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->penc_key, BTA_LE_KEY_PENC);
            }
            if (p_device_rec->key_mask & BTA_LE_KEY_PID)
            {
                BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->pid_key, BTA_LE_KEY_PID);
            }
            if (p_device_rec->key_mask & BTA_LE_KEY_PCSRK)
            {
                BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->pcsrk_key, BTA_LE_KEY_PCSRK);
            }
            if (p_device_rec->key_mask & BTA_LE_KEY_LCSRK)
            {
                BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->lcsrk_key, BTA_LE_KEY_LCSRK);
            }
            if (p_device_rec->key_mask & BTA_LE_KEY_LENC)
            {
                BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->lenc_key, BTA_LE_KEY_LENC);
            }
            if (p_device_rec->key_mask & BTA_LE_KEY_LID)
            {
                BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->lid_key, BTA_LE_KEY_LID);
            }

        }
    }

#endif
}


/*******************************************************************************
**
** Function         btapp_dm_set_trusted
**
** Description      Sets device as trusted
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_set_trusted(tBTA_SERVICE_MASK trusted_mask, tBTAPP_REM_DEVICE *p_device_rec)
{

    p_device_rec->trusted_mask |= trusted_mask;
    p_device_rec->is_trusted = TRUE;
	APPL_TRACE_EVENT0("btapp_dm_set_trusted");
    btapp_store_device(p_device_rec);
    /* update BTA with new settings */
    btapp_dm_sec_add_device(p_device_rec);

    return TRUE;


}

/*******************************************************************************
**
** Function         btapp_dm_set_not_trusted
**
** Description      Sets device as not trusted
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_not_trusted(tBTAPP_REM_DEVICE * p_device_rec)
{

    p_device_rec->is_trusted = FALSE;
	APPL_TRACE_EVENT0("btapp_dm_set_not_trusted");
    btapp_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);


}
/*******************************************************************************
**
** Function         btapp_dm_delete_device
**
** Description      Deletes a device from data base
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_delete_device(BD_ADDR bd_addr)
{
    if ((BTA_DmRemoveDevice (bd_addr, BT_TRANSPORT_BR_EDR)) == BTA_SUCCESS)
    {
        btapp_delete_device(bd_addr);
        return TRUE;
    }
    else
        return FALSE;

}

/*******************************************************************************
**
** Function         btapp_dm_remove_ble_device
**
** Description      Deletes a device from data base
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_remove_ble_device(BD_ADDR bd_addr)
{
    if ((BTA_DmRemoveDevice (bd_addr, BT_TRANSPORT_LE)) == BTA_SUCCESS)
    {
        btapp_delete_device(bd_addr);
        return TRUE;
    }
    else
        return FALSE;

}


/*******************************************************************************
**
** Function         btapp_dm_discover_device
**
** Description      Searches for services on designated device.
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_discover_device(BD_ADDR bd_addr, BOOLEAN is_new, tBTA_TRANSPORT transport)
{
    tBTA_SERVICE_MASK client_services = BTA_BLE_SERVICE_MASK;
    BOOLEAN sdp_search;
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    tBTA_SERVICE_MASK_EXT client_services_ext;
#endif

#if( defined BTA_DG_INCLUDED ) && (BTA_DG_INCLUDED == TRUE)
    /* we need to find only services for which we can be in client role */
    client_services = (btapp_cfg.supported_services & ~(BTA_SPP_SERVICE_MASK | BTA_DUN_SERVICE_MASK | BTA_FAX_SERVICE_MASK | BTA_LAP_SERVICE_MASK));
    APPL_TRACE_EVENT2(" btapp_dm_discover_device  client_services x%x %c",client_services, btapp_cfg.dg_client_service_id[0]);
    client_services |= 1 << (btapp_cfg.dg_client_service_id[0] - '0');
#endif

    if ( is_new )
        sdp_search = FALSE;
    else
        sdp_search = TRUE;

#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    memset (&client_services_ext, 0,sizeof(tBTA_SERVICE_MASK_EXT));
    client_services |= BTA_BLE_SERVICE_MASK;
    client_services_ext.srvc_mask = client_services;

    if (transport == BTA_TRANSPORT_UNKNOWN)
        BTA_DmDiscoverExt(bd_addr, &client_services_ext, btapp_discover_cb, sdp_search);
    else
        BTA_DmDiscoverByTransport(bd_addr, &client_services_ext, btapp_discover_cb, sdp_search, transport);
#else
        BTA_DmDiscover(bd_addr, client_services, btapp_discover_cb, sdp_search);
#endif
}

/*******************************************************************************
**
** Function         btapp_dm_clear_inq_db
**
** Description      Make the inquiry record of the device deleted.
**                  Force discovery later.
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_clear_inq_db(BD_ADDR bd_addr)
{
    BTM_ClearInqDb (bd_addr);
}

/*******************************************************************************
**
** Function         btapp_dm_stored_device_unbond
**
** Description      Unbond selected device
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_stored_device_unbond ()
{

    if (!btapp_cb.p_selected_rem_device->link_key_present||
        (BTA_DmRemoveDevice(btapp_cb.p_selected_rem_device->bd_addr, BT_TRANSPORT_LE) != BTA_SUCCESS))
    {
        btapp_cb.p_selected_rem_device->link_key_present = FALSE;
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
        btapp_cb.p_selected_rem_device->key_mask = 0;
#endif
		APPL_TRACE_EVENT0("btapp_dm_stored_device_unbond1");
        btapp_store_device(btapp_cb.p_selected_rem_device);
        return FALSE;
    }
    else
    {
        btapp_cb.p_selected_rem_device->link_key_present = FALSE;

#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
        btapp_cb.p_selected_rem_device->key_mask = 0;

        /* unbond here will remove the security device record, device type will be gone, add back
           ideally should only delete bonding, not security record */
        if (btapp_cb.p_selected_rem_device->device_type & BT_DEVICE_TYPE_BLE)
        {
            BTA_DmAddBleDevice( btapp_cb.p_selected_rem_device->bd_addr,
                                btapp_cb.p_selected_rem_device->addr_type,
                                btapp_cb.p_selected_rem_device->auth_mode,
                                btapp_cb.p_selected_rem_device->device_type);
        }
#endif
		APPL_TRACE_EVENT0("btapp_dm_stored_device_unbond2");
        btapp_store_device(btapp_cb.p_selected_rem_device);

#if( defined BTA_HH_INCLUDED ) && (BTA_HH_INCLUDED == TRUE)
        btapp_act_hid_remove_dev(btapp_cb.p_selected_rem_device->bd_addr);
#endif

        return TRUE;
    }

}

/*******************************************************************************
**
** Function         btapp_dm_cancel_search
**
** Description      Cancels search
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_cancel_search(void)
{
    BTA_DmSearchCancel();
}

/*****************************************************************************
**  Function        btapp_dm_extract_version
**
**  Description     This function extracts a version number from version string
**                  assumes format of 'BCM1200_xx_10.3.MM.mm' or
**                  'BCM1200_xx_10.3.MM.m' to work properly.
**
**  Returns         the number of hex bytes filled.
*****************************************************************************/
static UINT16 btapp_dm_extract_version (void)
{
    const unsigned char  *p_major_str;
    const unsigned char  *p_minor_str;
    UINT16      retval = 0;
    int         minor_count = 0;

    p_major_str = &bte_version_string[BTAPP_MAJ_VER_STR_OFFSET];
    p_minor_str = &bte_version_string[BTAPP_MIN_VER_STR_OFFSET];

    retval = ( ((UINT16)(*p_major_str - '0') << 12) + ((UINT16)(*(p_major_str + 1) - '0') << 8) );

    /* Take the first two minor number (should be only 1 or 2) */
    if (isdigit((int)*p_minor_str))
        minor_count++;

    if (isdigit((int)*(p_minor_str + 1)))
        minor_count++;

    if (minor_count == 1)
        retval += ((UINT16)*p_minor_str) - '0';
    else if (minor_count == 2)
    {
        retval += ( ((UINT16)(*p_minor_str - '0') << 4) + (UINT16)(*(p_minor_str + 1) - '0') );
    }

    return(retval);
}

void btapp_dm_remname_cmpl_cback(tBTA_DM_REMNAME *p_rem_name)
{
    if (p_rem_name != NULL)
    {
        APPL_TRACE_DEBUG2("Remote name status: %d, name refreshed: [%s]",
            p_rem_name->status, p_rem_name->remote_bd_name);
    }
}

/*******************************************************************************
**
** Function         btapp_dm_read_remote_device_name
**
** Description      Read remote device name by transport
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_read_remote_device_name(BD_ADDR bd_addr, tBTA_TRANSPORT transport)
{
    BTA_DmReadRemoteDeviceName (bd_addr, btapp_dm_remname_cmpl_cback, transport);
}

/*******************************************************************************
**
** Function         btapp_dm_di_discover
**
** Description      Start DI discover
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_di_discover(BD_ADDR bd_addr)
{
    btapp_cb.p_di_db = (tBTA_DISCOVERY_DB *)GKI_getbuf(BTAPP_DM_DI_DB_SIZE);

    BTA_DmDiDiscover(bd_addr, btapp_cb.p_di_db, BTAPP_DM_DI_DB_SIZE, btapp_discover_cb);

}

/*******************************************************************************
**
** Function         btapp_dm_add_di_record
**
** Description      Set local DI record
**
**
** Returns          void
*******************************************************************************/
UINT16 btapp_dm_add_di_record(BOOLEAN is_primary)
{
    tBTA_DI_RECORD      device_info;

    memset(&device_info, 0, sizeof(tBTA_DI_RECORD));

    device_info.vendor = (btapp_cfg.vendor_id != 0)? btapp_cfg.vendor_id :LMP_COMPID_WIDCOMM ;           /* 17 */
    device_info.vendor_id_source = (btapp_cfg.vendor_id_source != 0)? btapp_cfg.vendor_id_source : DI_VENDOR_ID_SOURCE_BTSIG;  /* from Bluetooth SIG */
    device_info.product = (btapp_cfg.product_id != 0)? btapp_cfg.product_id : BTAPP_DEFAULT_PRODUCT_ID;
    device_info.version = (btapp_cfg.di_version != 0)? btapp_cfg.di_version : btapp_dm_extract_version();
    device_info.primary_record = is_primary;

    return BTA_DmSetLocalDiRecord(&device_info, &btapp_cb.di_handle);

}

/*******************************************************************************
**
** Function         btapp_dm_get_di_local_record
**
** Description      Get local DI record
**
**
** Returns          void
*******************************************************************************/
tBTA_STATUS btapp_dm_get_di_local_record(tBTA_DI_GET_RECORD *p_di_record, UINT32 handle)
{
    UINT32 di_handle = handle;

    return BTA_DmGetLocalDiRecord(p_di_record, &di_handle);
}

/*******************************************************************************
**
** Function         btapp_dm_get_di_remote_record
**
** Description      Get remote DI record by index.
**
**
** Returns          void
*******************************************************************************/
tBTA_STATUS btapp_dm_get_di_remote_record(tBTA_DI_GET_RECORD *p_record, UINT8 index)
{
    tBTA_STATUS status = BTA_FAILURE;

    memset(p_record, 0 , sizeof(tBTA_DI_GET_RECORD));

    if (btapp_cb.p_di_db != NULL)
    {
        status = BTA_DmGetDiRecord(index, p_record, btapp_cb.p_di_db);
    }

    return status;
}

/*******************************************************************************
**
** Function         btapp_dm_mps_discover
**
** Description      Start DI discover
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_mps_discover(BD_ADDR bd_addr)
{
#if (defined(BTA_MPS_INCLUDED) && (BTA_MPS_INCLUDED == TRUE))
    BTA_DmMpsDiscover(bd_addr, btapp_discover_cb);
#endif
}

/*******************************************************************************
**
** Function         btapp_dm_update_mps_record
**
** Description      Sets/Clears local MPS SDP record
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_update_mps_record(BOOLEAN expose_record)
{
#if (defined(BTA_MPS_INCLUDED) && (BTA_MPS_INCLUDED == TRUE))
    BTA_DmSetMpsRecord(expose_record, &mps_parms, btapp_discover_cb);
#endif
}

#ifdef KEEP
/*******************************************************************************
**
** Function         btapp_dm_search
**
** Description      Searches for devices supporting the services specified
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_search(tBTA_SERVICE_MASK services,tBTA_DM_INQ *p_data)
{
    tBTA_DM_INQ inq_params;

    if (!p_data)
    {
        inq_params.mode = BTA_DM_GENERAL_INQUIRY;
        inq_params.duration = BTAPP_DEFAULT_INQ_DURATION;
        inq_params.max_resps = btapp_cfg.num_inq_devices;
        inq_params.filter_type = btapp_cfg.dm_inq_filt_type;
        inq_params.report_dup = TRUE;
        memcpy(&inq_params.filter_cond, &btapp_cfg.dm_inq_filt_cond, sizeof(tBTA_DM_INQ_COND));
    }
    else
    {
        memcpy(&inq_params, p_data, sizeof(tBTA_DM_INQ));

    }
    btapp_inq_db.rem_index = 0;
    memset(&btapp_inq_db, 0, sizeof(btapp_inq_db));
    btapp_cb.search_services = services;
    /* find nearby devices */
    BTA_DmSearch(&inq_params, services, btapp_search_cb);
}
#endif

/*******************************************************************************
**
** Function         btapp_dm_search_ext
**
** Description      Searches for devices supporting the services specified
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_search_ext(tBTA_SERVICE_MASK_EXT *p_services,tBTA_DM_INQ *p_data, tBTA_DM_SEARCH_CBACK *cf)
{
    tBTA_DM_INQ inq_params;

    if (!p_data)
    {
        inq_params.mode = BTA_DM_GENERAL_INQUIRY;
        inq_params.duration = BTAPP_DEFAULT_INQ_DURATION;
        inq_params.max_resps = btapp_cfg.num_inq_devices;
        inq_params.filter_type = btapp_cfg.dm_inq_filt_type;
        inq_params.report_dup = FALSE;
        memcpy(&inq_params.filter_cond, &btapp_cfg.dm_inq_filt_cond, sizeof(tBTA_DM_INQ_COND));
    }
    else
    {
        memcpy(&inq_params, p_data, sizeof(tBTA_DM_INQ));
    }

    btapp_inq_db.rem_index = 0;
    memset(&btapp_inq_db, 0, sizeof(btapp_inq_db));

    if (p_services)
    {
        btapp_cb.search_services = p_services->srvc_mask;
        btapp_cb.search_num_uuid = p_services->num_uuid;
    }
    else
    {
        btapp_cb.search_services = 0;
        btapp_cb.search_num_uuid = 0;
    }

    /* find nearby devices */
	if(cf != NULL)
    	BTA_DmSearchExt(&inq_params, p_services, cf);
	else
		BTA_DmSearchExt(&inq_params, p_services, btapp_search_cb_ext);
}

/*******************************************************************************
**
** Function         btapp_dm_add_device
**
** Description      Adds a new device to database
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_add_device(void)
{
	APPL_TRACE_EVENT0("btapp_dm_add_device");
    if (btapp_store_device(btapp_cb.p_selected_rem_device))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*******************************************************************************
**
** Function         btapp_dm_bond
**
** Description      Initiates bonding with selected device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_bond(tBTAPP_REM_DEVICE * p_device_rec)
{

    if (btapp_cb.is_dd_bond == FALSE)
    {
        btapp_cb.is_dd_bond = TRUE;
        bdcpy(btapp_cb.sp_bond_bdaddr, p_device_rec->bd_addr);
        btapp_cb.bond_transport = BTA_TRANSPORT_BR_EDR;
        BTA_DmBond (p_device_rec->bd_addr);
		APPL_TRACE_EVENT0("btapp_dm_bond");
        btapp_store_device(p_device_rec);
    }
}

/*******************************************************************************
**
** Function         btapp_dm_bond_cancel
**
** Description      Cancels bonding with selected device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_bond_cancel(tBTAPP_REM_DEVICE * p_device_rec)
{
    btapp_cb.is_dd_bond = FALSE;
    BTA_DmBondCancel (p_device_rec->bd_addr);
}

/*******************************************************************************
**
** Function         btapp_dm_rename_device
**
** Description      sets user friendly name for remote device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_rename_device(tBTAPP_REM_DEVICE * p_device_rec, UINT8 * p_text)
{
    strncpy(p_device_rec->short_name, (char const *)p_text, BTAPP_DEV_NAME_LENGTH);
    /* update to nv memory */
    btapp_store_device(p_device_rec);
}

/*******************************************************************************
**
** Function         btapp_dm_set_visibility
**
** Description      Sets visibilty
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_visibility( BOOLEAN is_visible, BOOLEAN is_temp)
{

    BTA_DmSetVisibility( (tBTA_DM_DISC) ((is_visible) ? BTA_DM_GENERAL_DISC : BTA_DM_NON_DISC),
                         BTA_DM_CONN, BTA_DM_IGNORE, BTA_DM_IGNORE);

    btapp_device_db.visibility = is_visible;

    if (!is_temp)
    {
        /* update to nvram */

    }
}

#if (BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_dm_transport_bond
**
** Description      Initiates bonding with selected device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_transport_bond(tBTAPP_REM_DEVICE * p_device_rec, tBTA_TRANSPORT transport)
{

    if (btapp_cb.is_dd_bond == FALSE)
    {
        btapp_cb.is_dd_bond = TRUE;
        bdcpy(btapp_cb.sp_bond_bdaddr, p_device_rec->bd_addr);
        btapp_cb.bond_transport = transport;
        BTA_DmBondByTransport (p_device_rec->bd_addr, transport);

        btapp_store_device(p_device_rec);
    }
}

/*******************************************************************************
**
** Function         btapp_dm_set_le_visibility
**
** Description      Sets LE visibilty
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_le_visibility( UINT16 le_disc_mode, UINT16 le_conn_mode, BOOLEAN is_temp )
{
    btapp_device_db.le_disc_mode = le_disc_mode;
    btapp_device_db.le_conn_mode = le_conn_mode;

    BTA_DmSetVisibility( le_disc_mode,
                         le_conn_mode,
                         BTA_DM_IGNORE, BTA_DM_IGNORE);

    if(!is_temp)
    {//save the setting into NVRAM

    }
}

/*******************************************************************************
**
** Function         btapp_dm_proc_ble_io_req
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_proc_ble_io_req(BD_ADDR bd_addr,
                              tBTA_IO_CAP *p_io_cap,
                              tBTA_OOB_DATA *p_oob_data,
                              tBTA_LE_AUTH_REQ *p_auth_req,
                              UINT8 *p_max_key_size,
                              tBTA_LE_KEY_TYPE *p_init_key,
                              tBTA_LE_KEY_TYPE  *p_resp_key)
{
    APPL_TRACE_DEBUG0("btapp_dm_proc_ble_io_req");
    UINT8 has_oob_data;

    *p_auth_req = btapp_cfg.ble_auth_req;
    *p_io_cap = btapp_cfg.ble_loc_io_caps;

    *p_max_key_size = btapp_cfg.ble_max_key_size;

    *p_init_key = btapp_cfg.ble_init_key;
    *p_resp_key = btapp_cfg.ble_resp_key;

    btapp_dm_set_oob_for_le_io_req(bd_addr, &has_oob_data, &btapp_cfg.ble_auth_req);
    *p_oob_data = has_oob_data;
}
#endif

/*******************************************************************************
**
** Function         btapp_dm_ble_enc_cbacks
**
** Description      btapp dm ble encyption callback
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_ble_enc_cbacks (BD_ADDR bd_addr, tBTA_TRANSPORT transport, tBTA_STATUS result)
{
    if (result != 0)
    {
        APPL_TRACE_DEBUG0("encryption failed.");
    }
}

void btapp_dm_handle_authenticate_complete_event(tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data)
{
    tBTAPP_REM_DEVICE * p_device_record;
    char msg_str[64];

    APPL_TRACE_EVENT2("btapp_dm BTA_DM_AUTH_CMPL_EVT success:%d, fail reason:0x%x", p_data->auth_cmpl.success, p_data->auth_cmpl.fail_reason);
    sprintf (msg_str, "peer bdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
             p_data->auth_cmpl.bd_addr[0], p_data->auth_cmpl.bd_addr[1],
             p_data->auth_cmpl.bd_addr[2], p_data->auth_cmpl.bd_addr[3],
             p_data->auth_cmpl.bd_addr[4], p_data->auth_cmpl.bd_addr[5]);

    if (bdcmp(btapp_cb.sp_bond_bdaddr, p_data->auth_cmpl.bd_addr)==0)
    {
        /* peer indicated bonding during SP pairing process.
         * Allocate a device record to store the link key if pairing is successful */
        /*if (btapp_cb.sp_bond_bits)
            alloc = TRUE; */
        btapp_cb.sp_bond_bits = 0;
        btapp_cb.is_dd_bond = FALSE;
    }

	APPL_TRACE_EVENT1("btapp_dm_handle_authenticate_complete_event name %s",p_data->auth_cmpl.bd_name);
    if (p_data->auth_cmpl.key_present)
    {
        sprintf (msg_str, "link_key %s \n", btapp_utl_keys_to_str(p_data->auth_cmpl.key));

        APPL_TRACE_EVENT0(msg_str);

        p_device_record = btapp_get_device_record(p_data->auth_cmpl.bd_addr);

        if (!p_device_record)
        {
            APPL_TRACE_EVENT0("allocating a device record for the newly bonded device!");
            p_device_record = btapp_alloc_device_record(p_data->auth_cmpl.bd_addr);
            if (p_device_record)
                p_device_record->peer_io_cap = btapp_cb.sp_io_cap;
        }

        /* update data base with new link key */
        if (p_device_record)
        {
            p_device_record->peer_io_cap = btapp_cb.sp_io_cap;
            memcpy(p_device_record->link_key, p_data->auth_cmpl.key, LINK_KEY_LEN);
            p_device_record->link_key_present = TRUE;
            p_device_record->key_type = p_data->auth_cmpl.key_type;
			memcpy(p_device_record->name, p_data->auth_cmpl.bd_name, BTAPP_DEV_NAME_LENGTH+1);
			APPL_TRACE_EVENT0("btapp_dm_handle_authenticate_complete_event 1");
            btapp_store_device( p_device_record);
        }

        BTA_DmAddDevice(p_device_record->bd_addr, \
            p_device_record->dev_class, \
            p_device_record->link_key, \
            p_device_record->trusted_mask,\
            TRUE, \
            p_device_record->key_type,
            p_device_record->peer_io_cap);
    }

#if (defined(BLE_INCLUDED) && (BTA_GATT_INCLUDED == TRUE))
    if (!p_data->auth_cmpl.success)
    {
        APPL_TRACE_ERROR0("Authenticate with peer failed !!");
    }
    else
    {
        p_device_record = btapp_get_device_record(p_data->auth_cmpl.bd_addr);
        if (p_device_record)
        {
            p_device_record->addr_type = p_data->auth_cmpl.addr_type;
            p_device_record->device_type = p_data->auth_cmpl.dev_type;
            p_device_record->bond_with_peer  = TRUE;
			memcpy(p_device_record->name, p_data->auth_cmpl.bd_name, BTAPP_DEV_NAME_LENGTH+1);
			APPL_TRACE_EVENT0("btapp_dm_handle_authenticate_complete_event 2");
            btapp_store_device( p_device_record);
        }
        else
        {
            APPL_TRACE_ERROR0("Can't find the BLE device record!!");
        }
    }
#endif
}

void btapp_dm_handle_ble_key_evt(tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data)
{
    tBTAPP_REM_DEVICE * p_device_record;

    APPL_TRACE_EVENT1("BTA_DM_BLE_KEY_EVT bd_addr:%s ", utl_bd_addr_to_string(p_data->ble_key.bd_addr));
    p_device_record = btapp_get_device_record(p_data->ble_key.bd_addr);
    if (!p_device_record)
    {
        p_device_record = btapp_alloc_device_record(p_data->ble_key.bd_addr);
    }
    if (p_device_record)
    {
        p_device_record->link_key_present = TRUE;
        p_device_record->key_mask |= p_data->ble_key.key_type;
        p_device_record->device_type |= BT_DEVICE_TYPE_BLE;
        switch (p_data->ble_key.key_type)
        {
            case BTA_LE_KEY_PENC:
                memcpy (&p_device_record->penc_key, &p_data->ble_key.p_key_value->penc_key, sizeof(tBTA_LE_PENC_KEYS));
                APPL_TRACE_EVENT1("BTA_LE_KEY_PENC : %s ", btapp_utl_keys_to_str(p_device_record->penc_key.ltk));
                break;
            case BTA_LE_KEY_PID:
                memcpy (&p_device_record->pid_key, &p_data->ble_key.p_key_value->pid_key, sizeof(tBTA_LE_PID_KEYS));
                APPL_TRACE_EVENT1("BTA_LE_KEY_PID : %s ", btapp_utl_keys_to_str(p_device_record->pid_key.irk));
                break;
            case BTA_LE_KEY_PCSRK:
                memcpy (&p_device_record->pcsrk_key, &p_data->ble_key.p_key_value->pcsrk_key, sizeof(tBTA_LE_PCSRK_KEYS));
                APPL_TRACE_EVENT1("BTA_LE_KEY_PCSRK : %s ", btapp_utl_keys_to_str(p_device_record->pcsrk_key.csrk));
                break;
            case BTA_LE_KEY_LENC:
                memcpy (&p_device_record->lenc_key, &p_data->ble_key.p_key_value->lenc_key, sizeof(tBTA_LE_LENC_KEYS));
                APPL_TRACE_EVENT1("BTA_LE_KEY_LENC : %s ", btapp_utl_keys_to_str(p_device_record->lenc_key.ltk));
                break;
            case BTA_LE_KEY_LCSRK:
                memcpy (&p_device_record->lcsrk_key, &p_data->ble_key.p_key_value->lcsrk_key, sizeof(tBTA_LE_LCSRK_KEYS));
                APPL_TRACE_EVENT1("BTA_LE_KEY_LCSRK : %s ", btapp_utl_keys_to_str(p_device_record->lcsrk_key.csrk));
                break;
            case BTA_LE_KEY_LID:
                /* need to save LID key here, currently use the same ID key, if privacy is disabled, no ID is needed */
                APPL_TRACE_EVENT0("BTA_LE_KEY_LID");
                break;
            default:
                APPL_TRACE_EVENT1("BTAPP: BLE Key Failure: unknown BLE key type (0x%02x)", p_data->ble_key.key_type);
                break;
        }
        APPL_TRACE_EVENT1("BTAPP: Saving Dedicated BLE Key (type 0x%02x)", p_data->ble_key.key_type);
        btapp_store_device (p_device_record);
     }
}

/*******************************************************************************
**
** Function         btapp_dm_security_cback
**
** Description      Security callback from bta
**
**
** Returns          void
*******************************************************************************/
static void btapp_dm_security_cback (tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data)
{
    tBTAPP_REM_DEVICE * p_device_record;

    char msg_str[64];

    APPL_TRACE_EVENT1("btapp_dm_security_cback  event %d ",event);

    switch (event)
    {
        case BTA_DM_ENABLE_EVT:
            bdcpy(btapp_cb.local_bd_addr, p_data->enable.bd_addr);
            

            //Need retrieve btapp_device_db value from NV section

            btapp_dm_init_continue ();
            btapp_device_db.bt_enabled = TRUE;
            btapp_dm_add_di_record(TRUE);

#if (defined(BTA_AVK_INCLUDED) && (BTA_AVK_INCLUDED == TRUE))
            btapp_avk_init();
#endif

#if (defined(BTA_HS_INCLUDED) && (BTA_HS_INCLUDED == TRUE))
            btapp_hs_init();
#endif

#if( defined BTA_DG_INCLUDED ) && (BTA_DG_INCLUDED == TRUE)
            btapp_dg_init();
#endif

#if( defined BTA_PAN_INCLUDED ) && (BTA_PAN_INCLUDED == TRUE)
            btapp_cfg.pan_included  = TRUE;
            btapp_cfg.panu_supported = TRUE;
            /*TBD: pangn_supported & pannap_supported for GU/NAP */
            //btapp_cfg.pangn_supported = TRUE;
            //btapp_cfg.pannap_supported = TRUE;
            btapp_pan_init();
#endif

#if( defined BTA_PBC_INCLUDED ) && (BTA_PBC_INCLUDED == TRUE)
            btapp_pbc_init();
#endif

#if( defined BTA_AG_INCLUDED ) && (BTA_AG_INCLUDED == TRUE)
            btapp_ag_enable();
#endif

#if( defined BTA_AV_INCLUDED ) && (BTA_AV_INCLUDED == TRUE)
            btapp_av_init();
#endif

            sprintf (msg_str, "local bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
                     p_data->enable.bd_addr[0], p_data->enable.bd_addr[1],
                     p_data->enable.bd_addr[2], p_data->enable.bd_addr[3],
                     p_data->enable.bd_addr[4], p_data->enable.bd_addr[5]);
            APPL_TRACE_EVENT0(msg_str);
            APPL_TRACE_EVENT1("auth_req:%d", btapp_cfg.sp_auth_req);

            return;

        case BTA_DM_DISABLE_EVT:
            btapp_device_db.bt_enabled = FALSE;

            if(bt_stack_sync_sema != NULL)
            {
                gki_openrtos_set_semaphore(&bt_stack_sync_sema);
            }
            break;

        case BTA_DM_SIG_STRENGTH_EVT:
            if (p_data->sig_strength.mask & BTA_SIG_STRENGTH_RSSI_MASK)
            {
                APPL_TRACE_EVENT1("rssi value %d", p_data->sig_strength.rssi_value);
            }

            if (p_data->sig_strength.mask & BTA_SIG_STRENGTH_LINK_QUALITY_MASK)
            {
                APPL_TRACE_EVENT1("link quality value %d", p_data->sig_strength.link_quality_value);
            }
            return;

        case BTA_DM_LINK_UP_EVT:
            APPL_TRACE_EVENT2("btapp_dm_security_cback: bda:%s link_type:%d", \
                              utl_bd_addr_to_string(p_data->link_up.bd_addr), p_data->link_up.link_type);

            p_device_record = btapp_get_device_record(p_data->link_up.bd_addr);
            if (!p_device_record)
            {
                APPL_TRACE_EVENT0("allocating a device record for the newly link_up device");
                p_device_record = btapp_alloc_device_record(p_data->link_up.bd_addr);
            }

            if(p_data->link_up.link_type == BT_TRANSPORT_BR_EDR)
            {

            }
            else if(p_data->link_up.link_type == BT_TRANSPORT_LE)
            {
                // GOOGLE: added check for pairability
                if (btapp_device_db.pairability && (p_device_record->bond_with_peer == FALSE))
                {
                    /*
                    BTA_DmSetEncryption(
                        p_data->link_up.bd_addr,
                        BT_TRANSPORT_LE,
                        btapp_dm_ble_enc_cbacks,
                        BTA_DM_BLE_SEC_NO_MITM);
                    */
                }

                btapp_dm_ble_rssi_monitor(p_data->link_up.bd_addr,BTA_BLE_RSSI_ALERT_LO_BIT,(INT8)-90,0,0);

                if(!read_rssi_task_enable)
                {
                  memcpy(conn_bda,p_data->link_up.bd_addr,sizeof(conn_bda));
                  pthread_attr_t attr;
                  pthread_attr_init(&attr);
                  pthread_create(&rssi_task, &attr, rssi_task_main, NULL );
                }
            }

            return;

        case BTA_DM_LINK_DOWN_EVT:
            sprintf (msg_str, "disconnected from bdaddr %02x:%02x:%02x:%02x:%02x:%02x reason %04x",
                     p_data->link_down.bd_addr[0], p_data->link_down.bd_addr[1],
                     p_data->link_down.bd_addr[2], p_data->link_down.bd_addr[3],
                     p_data->link_down.bd_addr[4], p_data->link_down.bd_addr[5],p_data->link_down.status);

            APPL_TRACE_EVENT0(msg_str);

            if (bdcmp(btapp_cb.sp_bond_bdaddr, p_data->link_down.bd_addr)==0)
            {
                /* bonding must have failed - clear the is_bond flag */
                btapp_cb.sp_bond_bits = 0;
                btapp_cb.is_dd_bond = FALSE;
            }

#if BLE_INCLUDED == TRUE
            if(p_data->link_down.link_type == BT_TRANSPORT_LE)
            {
                APPL_TRACE_EVENT0("reset Start LE ADV");
                /* restart LE adv */
                BTA_DmSetBleVisibility(TRUE);
            }
#endif
            return;

        case BTA_DM_PIN_REQ_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_PIN_REQ_EVT");

            break;

        case BTA_DM_SP_CFM_REQ_EVT: /* 9  Simple Pairing User Confirmation request. */
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_SP_CFM_REQ_EVT");
            bdcpy(btapp_cb.peer_bdaddr, p_data->cfm_req.bd_addr);
            p_device_record = btapp_get_device_record(p_data->cfm_req.bd_addr);
            if(p_device_record != NULL)
            {
                memcpy(p_device_record->dev_class, p_data->cfm_req.dev_class, DEV_CLASS_LEN);

            }
            btapp_dm_confirm_reply(TRUE);
            break;

        case BTA_DM_SP_KEY_NOTIF_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_SP_KEY_NOTIF_EVT");

            break;

        case BTA_DM_SP_KEY_REQ_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_SP_KEY_REQ_EVT");

            break;

        case BTA_DM_SP_RMT_OOB_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_SP_RMT_OOB_EVT");

            break;

#if (BTM_BR_SC_INCLUDED == TRUE)
        case BTA_DM_SP_RMT_OOB_EXT_EVT:

            break;
#endif

        case BTA_DM_SP_KEYPRESS_EVT: /* key press notification */
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_SP_KEYPRESS_EVT");

            break;

        case BTA_DM_AUTH_CMPL_EVT:
            btapp_dm_handle_authenticate_complete_event(event, p_data);
            break;

        case BTA_DM_AUTHORIZE_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_AUTHORIZE_EVT");
            BTA_DmAuthorizeReply(p_data->authorize.bd_addr, p_data->authorize.service, BTA_DM_AUTH_PERM);
            break;

        case BTA_DM_BUSY_LEVEL_EVT:
            APPL_TRACE_EVENT1("btapp_dm_security_cback BTA_DM_BUSY_LEVEL_EVT %d",p_data->busy_level.level);
#if ((defined BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE))
//            btapp_codec_update_busy_level(p_data->busy_level.level);
#endif
            break;

        case BTA_DM_BOND_CANCEL_CMPL_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_BOND_CANCEL_CMPL_EVT");

            break;
#if BLE_INCLUDED == TRUE
        case BTA_DM_BLE_SEC_REQ_EVT:
            /* always grant access */
            APPL_TRACE_EVENT1("btapp_dm_security_cback BTA_DM_BLE_SEC_REQ_EVT, req_addr:%s ", utl_bd_addr_to_string(p_data->ble_req.bd_addr));
            bdcpy(btapp_cb.peer_bdaddr, p_data->ble_req.bd_addr);
            btapp_dm_security_grant(TRUE);
            break;

        case BTA_DM_BLE_PASSKEY_NOTIF_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_BLE_PASSKEY_NOTIF_EVT");

            break;

        case BTA_DM_BLE_PASSKEY_REQ_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_BLE_PASSKEY_REQ_EVT");

            break;

#if SMP_INCLUDED == TRUE && SMP_LE_SC_INCLUDED == TRUE
        case BTA_DM_BLE_NC_REQ_EVT:
            APPL_TRACE_EVENT0("btapp_dm_security_cback BTA_DM_BLE_NC_REQ_EVT");

            break;
#endif

        case BTA_DM_BLE_KEY_EVT:
            btapp_dm_handle_ble_key_evt(event, p_data);
            break;

        case BTA_DM_BLE_LOCAL_IR_EVT:
            btapp_ble_local_key.keys_mask |= BTA_BLE_LOCAL_KEY_TYPE_ID;
            memcpy(&btapp_ble_local_key.id_keys,&p_data->ble_id_keys, sizeof(tBTA_BLE_LOCAL_ID_KEYS));
            btapp_nv_store_ble_local_keys();
            break;

        case BTA_DM_BLE_LOCAL_ER_EVT:
            btapp_ble_local_key.keys_mask |= BTA_BLE_LOCAL_KEY_TYPE_ER;
            memcpy(&btapp_ble_local_key.er, &p_data->ble_er, sizeof(BT_OCTET16));
            btapp_nv_store_ble_local_keys();
            break;

        case BTA_DM_BLE_OOB_REQ_EVT:
            break;
#endif
        default:
            break;
    }
}

/*******************************************************************************
**
** Function         btapp_add_devices
**
** Description      called during startup to add the devices which are
**                  stored in NVRAM
**
** Returns          void
*******************************************************************************/
void btapp_add_devices(void)
{
    UINT8 i;
    tBTAPP_REM_DEVICE * p_device_rec;

    /* Update BTA with peer device information
    stored in NVRAM  */
    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (!btapp_device_db.device[i].in_use)
            continue;

        p_device_rec = &btapp_device_db.device[i];

#if BLE_INCLUDED == TRUE
        {

            APPL_TRACE_DEBUG1("from NV p_device_rec->device_type = %d", p_device_rec->device_type);

            if (p_device_rec->device_type == BT_DEVICE_TYPE_BLE ||
                p_device_rec->device_type == BT_DEVICE_TYPE_DUMO)
            {
                APPL_TRACE_DEBUG0("add a LE device");
                BTA_DmAddBleDevice(p_device_rec->bd_addr, p_device_rec->addr_type, p_device_rec->auth_mode, p_device_rec->device_type);

                if (p_device_rec->key_mask & BTA_LE_KEY_PENC)
                {
                    BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->penc_key, BTA_LE_KEY_PENC);
                }
                if (p_device_rec->key_mask & BTA_LE_KEY_PID)
                {
                    BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->pid_key, BTA_LE_KEY_PID);
                }
                if (p_device_rec->key_mask & BTA_LE_KEY_PCSRK)
                {
                    BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->pcsrk_key, BTA_LE_KEY_PCSRK);
                }
                if (p_device_rec->key_mask & BTA_LE_KEY_LCSRK)
                {
                    BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->lcsrk_key, BTA_LE_KEY_LCSRK);
                }
                if (p_device_rec->key_mask & BTA_LE_KEY_LENC)
                {
                    BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->lenc_key, BTA_LE_KEY_LENC);
                }
                if (p_device_rec->key_mask & BTA_LE_KEY_LID)
                {
                    BTA_DmAddBleKey(p_device_rec->bd_addr, (tBTA_LE_KEY_VALUE *)&p_device_rec->lid_key, BTA_LE_KEY_LID);
                }

            }
            if (p_device_rec->device_type == BT_DEVICE_TYPE_DUMO ||
                p_device_rec->device_type == BT_DEVICE_TYPE_BREDR);
        }
#endif
        {
            if (btapp_device_db.device[i].link_key_present)
            {
                BTA_DmAddDevice(btapp_device_db.device[i].bd_addr, btapp_device_db.device[i].dev_class,
                                btapp_device_db.device[i].link_key,
                                btapp_device_db.device[i].trusted_mask, btapp_device_db.device[i].is_trusted,
                                btapp_device_db.device[i].key_type, btapp_device_db.device[i].peer_io_cap);
            }
            else if (btapp_device_db.device[i].is_trusted)
            {
                BTA_DmAddDevice(btapp_device_db.device[i].bd_addr, btapp_device_db.device[i].dev_class,
                                NULL, btapp_device_db.device[i].trusted_mask,
                                btapp_device_db.device[i].is_trusted, btapp_device_db.device[i].key_type,
                                btapp_device_db.device[i].peer_io_cap);
            }
        }
    }
}

/*******************************************************************************
**
** Function         btapp_dm_sort_inq_db
**
** Description      checks if the given inq_res is in the inq_db
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_sort_inq_db(UINT8 index)
{
    tBTAPP_REM_DEVICE    inq_rec;
    tBTAPP_REM_DEVICE    *p_inq_rec;
    UINT8               i;
    BOOLEAN             copy = FALSE;
    int                 rssi_tgt;

/*
    APPL_TRACE_EVENT2("btapp_dm_sort_inq_db:%d, rssi:%d",
                      index, btapp_inq_db.remote_device[index].rssi);
*/

    if (index == 0 ||
        btapp_inq_db.remote_device[index].rssi == BTA_DM_INQ_RES_IGNORE_RSSI)
        return;

    memcpy(&inq_rec, &btapp_inq_db.remote_device[index], sizeof(tBTAPP_REM_DEVICE));
    rssi_tgt = inq_rec.rssi + inq_rec.rssi_offset;
    i=index-1;
    while (i>=0)
    {
        p_inq_rec = &btapp_inq_db.remote_device[i];
        if (p_inq_rec->rssi == BTA_DM_INQ_RES_IGNORE_RSSI ||
            (p_inq_rec->rssi + p_inq_rec->rssi_offset) < rssi_tgt)
        {
//            APPL_TRACE_EVENT2("moving:%d to :%d", i, i+1);
            memcpy(&btapp_inq_db.remote_device[i+1], p_inq_rec, sizeof(tBTAPP_REM_DEVICE));
            copy = TRUE;
        }
        else
        {
            i++;
            break;
        }
        if (i==0)
            break;
        i--;
    }

    if (copy)
    {
//        APPL_TRACE_EVENT2("moving:%d to :%d", index, i);
        memcpy(&btapp_inq_db.remote_device[i], &inq_rec, sizeof(tBTAPP_REM_DEVICE));
    }
}

/*******************************************************************************
**
** Function         btapp_dm_chk_inq_db
**
** Description      checks if the given inq_res is in the inq_db
**
**
** Returns          TRUE, is already in the inq_db
*******************************************************************************/
tBTAPP_REM_DEVICE * btapp_dm_chk_inq_db(tBTA_DM_INQ_RES *p_res)
{
    tBTAPP_REM_DEVICE    *p_inq_rec;
    UINT8               i;

/*
    APPL_TRACE_EVENT2("btapp_dm_chk_inq_db:%d, rssi:%d",
                      btapp_inq_db.rem_index, p_res->rssi);
*/

    if (btapp_inq_db.rem_index)
    {
        for (i=0; i<btapp_inq_db.rem_index; i++)
        {
            p_inq_rec = &btapp_inq_db.remote_device[i];

            if (memcmp(p_inq_rec->bd_addr, p_res->bd_addr, BD_ADDR_LEN) == 0)
            {
                p_inq_rec->rssi = p_res->rssi;
                btapp_dm_sort_inq_db(i);
                return p_inq_rec;
            }
        }
    }
    return NULL;
}

#if BLE_INCLUDED == TRUE

/*******************************************************************************
**
** Function         btapp_ble_convert_uuid16_to_uuid128
**
** Description      Convert a 16 bits UUID to be an standard 128 bits one.
**
** Returns          TRUE if two uuid match; FALSE otherwise.
**
*******************************************************************************/
static void btapp_ble_convert_uuid16_to_uuid128(UINT8 uuid_128[LEN_UUID_128], UINT16 uuid_16)
{
    UINT8   *p = &uuid_128[LEN_UUID_128 - 4];

    memcpy (uuid_128, base_uuid, LEN_UUID_128);

    UINT16_TO_STREAM(p, uuid_16);
}

/*******************************************************************************
**
** Function         btapp_ble_uuid_compare
**
** Description      Compare two UUID to see if they are the same.
**
** Returns          TRUE if two uuid match; FALSE otherwise.
**
*******************************************************************************/
BOOLEAN btapp_ble_uuid_compare(tBT_UUID tar, tBT_UUID src)
{
    UINT8  su[LEN_UUID_128], tu[LEN_UUID_128];
    UINT8  *ps, *pt;

    /* any of the UUID is unspecified */
    if (src.len == 0 || tar.len == 0)
    {
        return TRUE;
    }

    /* If both are 16-bit, we can do a simple compare */
    if (src.len == 2 && tar.len == 2)
    {
        return src.uu.uuid16 == tar.uu.uuid16;
    }

    /* One or both of the UUIDs is 128-bit */
    if (src.len == LEN_UUID_16)
    {
        /* convert a 16 bits UUID to 128 bits value */
        btapp_ble_convert_uuid16_to_uuid128(su, src.uu.uuid16);
        ps = su;
    }
    else
        ps = src.uu.uuid128;

    if (tar.len == LEN_UUID_16)
    {
        /* convert a 16 bits UUID to 128 bits value */
        btapp_ble_convert_uuid16_to_uuid128(tu, tar.uu.uuid16);
        pt = tu;
    }
    else
        pt = tar.uu.uuid128;

    return(memcmp(ps, pt, LEN_UUID_128) == 0);
}

tBTAPP_BLE_SERVICE_MASK btapp_convert_uuid_to_ble_srvc_mask(tBT_UUID uuid)
{
    tBT_UUID    service_uuid = {2, {0}};
    UINT8 i;

    for (i = 0; i < BTAPP_BLE_SERVICE_BIT_MAX; i ++)
    {
        service_uuid.uu.uuid16 = service_uuid_16[i];
        /* find a service */
        if (btapp_ble_uuid_compare(service_uuid, uuid))
        {
            return (tBTAPP_BLE_SERVICE_MASK)(1 << i);
        }
    }
    /* known service */
    APPL_TRACE_ERROR0("unknown GATT service");
    return 0;
}
#endif

/*******************************************************************************
**
** Function         btapp_search_cb
**
** Description      callback to notify the completion of device search
**
**
** Returns          void
*******************************************************************************/
void btapp_search_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{

    tBTAPP_REM_DEVICE    *p_device_rec, *p_inquiry_rec;
    char msg_str[128];
    UINT8               *p_eir_remote_name = NULL;
    UINT8               remote_name_len;
    tBTA_SERVICE_MASK   services = 0;
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    tBTAPP_BLE_SERVICE_MASK  ble_service;
#endif

//    APPL_TRACE_DEBUG1("btapp_search_cb event=%d",event);

    if (event == BTA_DM_DISC_RES_EVT)
    {
        APPL_TRACE_DEBUG0("Rcv BTA_DM_DISC_RES_EVT");
        if (!btapp_cb.search_services ||  (btapp_cb.search_services & p_data->disc_res.services))
        {
            p_inquiry_rec = btapp_get_inquiry_record(p_data->disc_res.bd_addr);

            if (p_inquiry_rec)
            {
                p_inquiry_rec->in_use = TRUE;
                p_inquiry_rec->services |= p_data->disc_res.services;
                memcpy ((void *)&p_inquiry_rec->bd_addr,
                        (const void *)p_data->disc_res.bd_addr, BD_ADDR_LEN);
                if (strlen((const char *)p_data->disc_res.bd_name))
                    strncpy ((char *)p_inquiry_rec->name,
                             (char *)p_data->disc_res.bd_name, BTAPP_DEV_NAME_LENGTH);
            }

            p_device_rec = btapp_get_device_record(p_data->disc_res.bd_addr);
            if (p_device_rec)
            {
                p_device_rec->services |= p_data->disc_res.services;

                if (strlen((const char*)p_data->disc_res.bd_name))
                {
                    strncpy ((char *)p_device_rec->name,
                             (char *)p_data->disc_res.bd_name, BTAPP_DEV_NAME_LENGTH);
                }
                if (p_inquiry_rec)
                    memcpy(p_inquiry_rec, p_device_rec, sizeof(tBTAPP_REM_DEVICE));

                btapp_store_device(p_device_rec);
            }
        }

    }
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    else if (event == BTA_DM_DISC_BLE_RES_EVT)
    {
        APPL_TRACE_EVENT0("Rcv BTA_DM_DISC_BLE_RES_EVT");

        ble_service = btapp_convert_uuid_to_ble_srvc_mask(p_data->disc_ble_res.service);

        APPL_TRACE_DEBUG1("receive BTA_DM_DISC_BLE_RES_EVT: ble_service_mask = %08x", ble_service);
        p_inquiry_rec = btapp_get_inquiry_record(p_data->disc_res.bd_addr);

        if (p_inquiry_rec)
        {
            p_inquiry_rec->in_use = TRUE;
            p_inquiry_rec->ble_services |= ble_service;
            memcpy ((void *)&p_inquiry_rec->bd_addr,
                    (const void *)p_data->disc_res.bd_addr, BD_ADDR_LEN);
            if (strlen((const char *)p_data->disc_res.bd_name))
                strncpy ((char *)p_inquiry_rec->name,
                         (char *)p_data->disc_res.bd_name, BTAPP_DEV_NAME_LENGTH);
        }

        p_device_rec = btapp_get_device_record(p_data->disc_res.bd_addr);

        if (p_device_rec)
        {
            if (p_inquiry_rec)
                p_device_rec->ble_services |= p_inquiry_rec->ble_services;
            else
                p_device_rec->ble_services |= ble_service;

            if (strlen((const char*)p_data->disc_res.bd_name))
            {
                strncpy ((char *)p_device_rec->name,
                         (char *)p_data->disc_res.bd_name, BTAPP_DEV_NAME_LENGTH);
            }
            if (p_inquiry_rec)
                memcpy(p_inquiry_rec, p_device_rec, sizeof(tBTAPP_REM_DEVICE));

            btapp_store_device(p_device_rec);
        }

    }
#endif
    else if (event == BTA_DM_DISC_CMPL_EVT)
    {
        ble_scan_rounds --;
        APPL_TRACE_DEBUG1("Rcv BTA_DM_DISC_CMPL_EVT round:%d", TARGET_BLE_SCAN_ROUNDS - ble_scan_rounds);
    }
    else if (event == BTA_DM_INQ_RES_EVT)
    {
        if ((p_inquiry_rec = btapp_dm_chk_inq_db(&p_data->inq_res)) != NULL)
        {
#if BLE_INCLUDED == TRUE
            if (btapp_cfg.ble_included &&
                p_inquiry_rec->device_type == p_data->inq_res.device_type)
                /* already in the inq_db and device type is the same*/
#endif
                ;
        }

        if (p_inquiry_rec == NULL )
        {
            if (btapp_inq_db.rem_index < BTAPP_NUM_INQ_DEVICE)
            {
                p_inquiry_rec = &btapp_inq_db.remote_device[btapp_inq_db.rem_index];

                p_inquiry_rec->name[0] = 0;
                p_inquiry_rec->services = 0;
                p_inquiry_rec->rssi_offset = 0;
                p_inquiry_rec->in_use = TRUE;
                btapp_inq_db.rem_index++;

                memcpy((void *)&p_inquiry_rec->bd_addr,
                       (const void *)p_data->inq_res.bd_addr, BD_ADDR_LEN);
            }
            else
                return;
        }

#if BLE_INCLUDED == TRUE
        if (btapp_cfg.ble_included)
        {
            sprintf (msg_str, "[%s]%02x:%02x:%02x:%02x:%02x:%02x rssi:%d",
                     btapp_dm_device_type[p_data->inq_res.device_type],
                     p_data->inq_res.bd_addr[0], p_data->inq_res.bd_addr[1],
                     p_data->inq_res.bd_addr[2], p_data->inq_res.bd_addr[3],
                     p_data->inq_res.bd_addr[4], p_data->inq_res.bd_addr[5], p_data->inq_res.rssi);
            APPL_TRACE_EVENT0(msg_str);
        }
        else
#endif
        {
            sprintf (msg_str, "%02x:%02x:%02x:%02x:%02x:%02x ",
                     p_data->inq_res.bd_addr[0], p_data->inq_res.bd_addr[1],
                     p_data->inq_res.bd_addr[2], p_data->inq_res.bd_addr[3],
                     p_data->inq_res.bd_addr[4], p_data->inq_res.bd_addr[5]);
            APPL_TRACE_EVENT0(msg_str);
        }

        p_inquiry_rec->rssi = p_data->inq_res.rssi;

#if BLE_INCLUDED == TRUE
        p_inquiry_rec->device_type  = p_data->inq_res.device_type;
        //APPL_TRACE_DEBUG1("device_type in inq_db=0x%x ", p_inquiry_rec->device_type);
        if (btapp_cfg.ble_included)
        {
            p_inquiry_rec->addr_type    = p_data->inq_res.ble_addr_type;

            if (p_data->inq_res.inq_result_type == BTM_INQ_RESULT_BLE)
            {
                //APPL_TRACE_DEBUG0("inq_result_type == BTM_INQ_RESULT_BLE");
                if (p_data->inq_res.p_eir != NULL)
                {
                    p_eir_remote_name = BTM_CheckAdvData(p_data->inq_res.p_eir,
                                                         BTM_BLE_AD_TYPE_NAME_CMPL,
                                                         &remote_name_len);
                    if (p_eir_remote_name == NULL)
                    {
                        p_eir_remote_name = BTM_CheckAdvData(p_data->inq_res.p_eir,
                                                             BTM_BLE_AD_TYPE_NAME_SHORT,
                                                             &remote_name_len);
                    }

                    if (p_eir_remote_name)
                    {
                        if ( remote_name_len > BTAPP_DEV_NAME_LENGTH )
                            remote_name_len = BTAPP_DEV_NAME_LENGTH;

                        memcpy(p_inquiry_rec->name, p_eir_remote_name, remote_name_len);
                        p_inquiry_rec->name[remote_name_len] = 0;
                        //APPL_TRACE_DEBUG0("Remote name found");
                    }
                    else
                    {
                        memset(p_inquiry_rec->name, 0, BTAPP_DEV_NAME_LENGTH);
                        //APPL_TRACE_DEBUG0("Remote name not found");
                    }
                }
            }
        }

        if (!btapp_cfg.ble_included ||
            p_data->inq_res.inq_result_type == BTM_INQ_RESULT_BR)
#endif
        {
            APPL_TRACE_DEBUG0("inq_result_type == BTM_INQ_RESULT_BR");
            memcpy( p_inquiry_rec->dev_class,
                    p_data->inq_res.dev_class, sizeof(DEV_CLASS));

            if (p_data->inq_res.is_limited)
                p_inquiry_rec->rssi_offset = BTAPP_DM_LIMITED_RSSI_OFFSET;

            if ( p_data->inq_res.p_eir)
            {
                p_eir_remote_name = BTA_CheckEirData( p_data->inq_res.p_eir,
                                                      BTM_EIR_COMPLETE_LOCAL_NAME_TYPE,
                                                      &remote_name_len );
                if ( !p_eir_remote_name )
                {
                    p_eir_remote_name = BTA_CheckEirData( p_data->inq_res.p_eir,
                                                          BTM_EIR_SHORTENED_LOCAL_NAME_TYPE,
                                                          &remote_name_len );
                }

                if ( p_eir_remote_name )
                {
                    if ( remote_name_len > BTAPP_DEV_NAME_LENGTH )
                        remote_name_len = BTAPP_DEV_NAME_LENGTH;

                    memcpy( p_inquiry_rec->name,
                            p_eir_remote_name, remote_name_len );
                    p_inquiry_rec->name[remote_name_len] = 0;
                }

                BTA_GetEirService( p_data->inq_res.p_eir, &services);
                APPL_TRACE_EVENT1("EIR BTA services = %08X", services);
            }
        }

        p_device_rec = btapp_get_device_record(p_data->inq_res.bd_addr);
        /* a saved device */
        if (p_device_rec)
        {
            if ((p_inquiry_rec->name[0])
                ||(p_eir_remote_name))
            {
                APPL_TRACE_EVENT1("1 EIR/LE remote name = <%s>", p_inquiry_rec->name);

                /* if EIR indicates that this device's name is empty */
                if (p_inquiry_rec->name[0] == 0)
                {
                    memcpy((void *)p_inquiry_rec->name,
                           (const void *)msg_str, BTAPP_DEV_NAME_LENGTH);
                }
                strncpy( p_device_rec->name,
                         p_inquiry_rec->name, BTAPP_DEV_NAME_LENGTH );

                if (memcmp(p_inquiry_rec->name, msg_str, BTAPP_DEV_NAME_LENGTH))
                    p_data->inq_res.remt_name_not_required = TRUE;
            }
            else if (p_device_rec->name[0])
            {
                APPL_TRACE_EVENT1("stored remote name = %s",p_device_rec->name);
                strncpy( p_inquiry_rec->name,
                         p_device_rec->name, BTAPP_DEV_NAME_LENGTH );
                /*if we know the name of the device, tell BTA not to get it */
                p_data->inq_res.remt_name_not_required = TRUE;
            }
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
            /* update this information every time */
            p_device_rec->device_type   = p_inquiry_rec->device_type;
            p_device_rec->addr_type     = p_inquiry_rec->addr_type;
#endif
            memcpy(p_inquiry_rec, p_device_rec, sizeof(tBTAPP_REM_DEVICE));

        }
        /* not a saved device and have EIR/ADV name*/
        else if ( p_inquiry_rec->name[0])
        {
            APPL_TRACE_EVENT1("2 EIR/LE remote name = %s", p_inquiry_rec->name);

            if (memcmp(p_inquiry_rec->name, msg_str, BTAPP_DEV_NAME_LENGTH))
                p_data->inq_res.remt_name_not_required = TRUE;
        }
        else /* non-saved device, and does not have EIR/ADV name */
        {
            strcpy((void *)p_inquiry_rec->name, "null");

            /* if EIR indicates that this device's name is empty */
            if (p_eir_remote_name)
                p_data->inq_res.remt_name_not_required = TRUE;
        }
        p_inquiry_rec->services |= services;

        btapp_dm_sort_inq_db((UINT8)(btapp_inq_db.rem_index - 1));
    }
    else if (event == BTA_DM_INQ_CMPL_EVT)
    {
        APPL_TRACE_DEBUG0("Rcv BTA_DM_INQ_CMPL_EVT");
    }
    else if(event  == BTA_DM_SEARCH_CANCEL_CMPL_EVT)
    {
        APPL_TRACE_DEBUG0("Rcv BTA_DM_SEARCH_CANCEL_CMPL_EVT");
    }
}

/*******************************************************************************
**
** Function         btapp_search_cb_ext
**
** Description
**
**
** Returns          void
*******************************************************************************/
#if 1
void btapp_search_cb_ext(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    char msg_str[256];
    char scanned_name[BTAPP_DEV_NAME_LENGTH];
	static int dev_idx = 0;

    APPL_TRACE_DEBUG1("btapp_search_cb_ext event=%d",event);
    tBTAPP_REM_DEVICE    *p_device_rec, *p_inquiry_rec;
    UINT8   *p_eir_remote_name = NULL;
    UINT8   remote_name_len;
    tBTA_SERVICE_MASK   services = 0;
    UINT16 idx = 0;
    if (event == BTA_DM_INQ_RES_EVT)
    {
        APPL_TRACE_DEBUG0("222Rcv BTA_DM_INQ_RES_EVT");

        idx = sprintf (&msg_str[idx], "[%s]%02x:%02x:%02x:%02x:%02x:%02x rssi:%d ",
                btapp_dm_device_type[p_data->inq_res.device_type],
                p_data->inq_res.bd_addr[0], p_data->inq_res.bd_addr[1],
                p_data->inq_res.bd_addr[2], p_data->inq_res.bd_addr[3],
                p_data->inq_res.bd_addr[4], p_data->inq_res.bd_addr[5], 
                p_data->inq_res.rssi);

        if ( p_data->inq_res.p_eir )
        {
            BTA_GetEirService( p_data->inq_res.p_eir, &services);
            idx += sprintf(&msg_str[idx], "EIR BTA services=%08X ", services);
			//APPL_TRACE_DEBUG1("btapp_cb.search_services=%08X ",btapp_cb.search_services);//0x00000040
            //if (!btapp_cb.search_services ||  (btapp_cb.search_services & services))
            {
                p_eir_remote_name = BTA_CheckEirData( p_data->inq_res.p_eir,
                                                      BTM_EIR_COMPLETE_LOCAL_NAME_TYPE,
                                                      &remote_name_len );
                if ( !p_eir_remote_name )
                {
                    p_eir_remote_name = BTA_CheckEirData( p_data->inq_res.p_eir,
                                                          BTM_EIR_SHORTENED_LOCAL_NAME_TYPE,
                                                          &remote_name_len );
                }

                if ( p_eir_remote_name )
                {
                    if ( remote_name_len > BTAPP_DEV_NAME_LENGTH )
                        remote_name_len = BTAPP_DEV_NAME_LENGTH;

                    memcpy( scanned_name, p_eir_remote_name, remote_name_len );
                    scanned_name[remote_name_len] = 0;
					
					memcpy( bb_device_list[dev_idx].bd_addr, p_data->inq_res.bd_addr, 6 );
					memcpy( bb_device_list[dev_idx].bd_name, &scanned_name, remote_name_len+1 );
					dev_idx++;	
                }
                idx += sprintf(&msg_str[idx], "Name=%s", scanned_name);
            }
        }

        msg_str[idx] = '\0';
        APPL_TRACE_DEBUG1("%s", msg_str);
    }

    else if(event == BTA_DM_DISC_CMPL_EVT)
    {
    	int total_cnt = dev_idx;
		int i = 0;
    	dev_idx = 0;
    	bb_test_count = total_cnt;
		for(i = 0; i < total_cnt; i++)
		{
			APPL_TRACE_DEBUG1("%s", bb_device_list[i].bd_name);
			printf("%02x:%02x:%02x:%02x:%02x:%02x\n", bb_device_list[i].bd_addr[0]
															,bb_device_list[i].bd_addr[1]
															,bb_device_list[i].bd_addr[2]
															,bb_device_list[i].bd_addr[3]
															,bb_device_list[i].bd_addr[4]
															,bb_device_list[i].bd_addr[5]);
		}
		
        APPL_TRACE_DEBUG1("scan finished, total %d device\r\n",total_cnt);
    }
    else if (event == BTA_DM_INQ_CMPL_EVT  ||
             event == BTA_DM_SEARCH_CANCEL_CMPL_EVT)
    {

    }

	
}
#endif
/*******************************************************************************
**
** Function         btapp_discover_cb
**
** Description      callback to notify the completion of device service discovery
**
**
** Returns          void
*******************************************************************************/
static void btapp_discover_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    tBTAPP_REM_DEVICE * p_device_rec, *p_inquiry_rec;
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    tBTAPP_BLE_SERVICE_MASK  ble_service = 0;
#endif
#if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE)
    char temp_str[64];
    UINT8 *p_temp;
#endif

    APPL_TRACE_DEBUG1("discover callback %d",event);

    if (event == BTA_DM_DISC_RES_EVT)
    {
        p_inquiry_rec = btapp_get_inquiry_record(p_data->disc_res.bd_addr);
        if (p_inquiry_rec)
        {
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
            p_inquiry_rec->ble_services |= ble_service;
            ble_service = p_inquiry_rec->ble_services;
#endif
            p_inquiry_rec->in_use = TRUE;
            memcpy ((void *)&p_inquiry_rec->bd_addr,
                    (const void *)p_data->disc_res.bd_addr, BD_ADDR_LEN);
            if (strlen((const char *)p_data->disc_res.bd_name))
                strncpy ((char *)p_inquiry_rec->name,
                         (char *)p_data->disc_res.bd_name, BTAPP_DEV_NAME_LENGTH);
        }
        p_device_rec = btapp_get_device_record(p_data->disc_res.bd_addr);
        if (p_device_rec)
        {
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
            ble_service = p_device_rec->ble_services;
#endif
            /* At least 1 service found */
            if ((p_data->disc_res.services & ~BTA_RES_SERVICE_MASK))
            {
                p_device_rec->services |= p_data->disc_res.services;
                btapp_store_device(p_device_rec);
            }
        }

    }
    /* DI discovery has completed */
    else if (event == BTA_DM_DI_DISC_CMPL_EVT)
    {

    }

#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE
    else if (event == BTA_DM_DISC_BLE_RES_EVT)
    {
        ble_service = btapp_convert_uuid_to_ble_srvc_mask(p_data->disc_ble_res.service);

        APPL_TRACE_DEBUG3("receive BTA_DM_DISC_BLE_RES_EVT: ble_service_mask = %08x uuid len= %d uuid = 0x%04x",
            ble_service,
            p_data->disc_ble_res.service.len, p_data->disc_ble_res.service.uu.uuid16);

        p_inquiry_rec = btapp_get_inquiry_record(p_data->disc_res.bd_addr);

        if (p_inquiry_rec)
        {
            p_inquiry_rec->in_use = TRUE;
            p_inquiry_rec->ble_services |= ble_service;
            ble_service = p_inquiry_rec->ble_services;
            memcpy ((void *)&p_inquiry_rec->bd_addr,
                    (const void *)p_data->disc_res.bd_addr, BD_ADDR_LEN);
            if (strlen((const char *)p_data->disc_res.bd_name))
                strncpy ((char *)p_inquiry_rec->name,
                         (char *)p_data->disc_res.bd_name, BTAPP_DEV_NAME_LENGTH);
        }

        p_device_rec = btapp_get_device_record(p_data->disc_res.bd_addr);
        if (p_device_rec)
        {
            p_device_rec->ble_services |= ble_service;

            if (strlen((const char *)p_data->disc_res.bd_name))
            {
                strncpy ((char *)p_device_rec->name,
                         (char *)p_data->disc_res.bd_name, BTAPP_DEV_NAME_LENGTH);
            }
            ble_service = p_device_rec->ble_services;

            btapp_store_device(p_device_rec);
        }
    }
#endif

#if (defined(BTA_MPS_INCLUDED) && BTA_MPS_INCLUDED == TRUE)
    /* MPS discovery has completed */
    else if (event == BTA_DM_MPS_DISC_CMPL_EVT)
    {
        APPL_TRACE_EVENT2("BTA_DM_MPS_DISC_CMPL_EVT status:%d, version:0x%04x",
                          p_data->mps_res.result, p_data->mps_res.profile_version);

        p_temp = (UINT8 *)&p_data->mps_res.mpsd_mask;
        sprintf (temp_str, "  MPSD Mask 0x%02x%02x%02x%02x%02x%02x%02x%02x  (MSB <-- LSB)",
                         p_temp[7], p_temp[6],
                         p_temp[5], p_temp[4],
                         p_temp[3], p_temp[2],
                         p_temp[1], p_temp[0]);
        APPL_TRACE_EVENT1("%s", temp_str);


        p_temp = (UINT8 *)&p_data->mps_res.mpmd_mask;
        sprintf (temp_str, "  MPMD Mask 0x%02x%02x%02x%02x%02x%02x%02x%02x  (MSB <-- LSB)",
                         p_temp[7], p_temp[6],
                         p_temp[5], p_temp[4],
                         p_temp[3], p_temp[2],
                         p_temp[1], p_temp[0]);
        APPL_TRACE_EVENT1("%s", temp_str);


        APPL_TRACE_EVENT1("  PPD  Mask 0x%04x              (MSB <-- LSB)",
                          p_data->mps_res.ppd_mask);

        if ((p_event_msg = (tBTAPP_BTA_MSG *)GKI_getbuf(sizeof(tBTAPP_BTA_MSG))) != NULL)
        {
            p_event_msg->hdr.event = BTAPP_MMI_DISCV_CMP;
            p_event_msg->hdr.layer_specific = event;
            GKI_send_msg(BTAPPL_TASK, TASK_MBOX_0, p_event_msg);
        }
    }
    else if (event == BTA_DM_MPS_SET_RECORD_CMPL_EVT)
    {
        APPL_TRACE_EVENT1("BTA_DM_MPS_SET_RECORD_CMPL_EVT result:%d", p_data->mps_sdp_res);

        if ((p_event_msg = (tBTAPP_BTA_MSG *)GKI_getbuf(sizeof(tBTAPP_BTA_MSG))) != NULL)
        {
            p_event_msg->hdr.event = BTAPP_MMI_DISCV_CMP;
            p_event_msg->hdr.layer_specific = event;
            GKI_send_msg(BTAPPL_TASK, TASK_MBOX_0, p_event_msg);
        }
    }
#endif  /* BTA_MPS_INCLUDED */
}

/*******************************************************************************

 $Function:        btapp_dm_db_get_device_info

 $Description:        gets the device record of a stored device.

 $Returns:        NULL if device not found. Pointer to a device structure if found. This data should
                 be copied if wanted to be used somewhere else.
                 If the device is not stored in Flash, the is_new flag is set => this means it is
                 a newly found device ( from an inquiry or discovery result ).

 $Arguments:        DB_ADDR of the device wanted.


*******************************************************************************/
tBTAPP_REM_DEVICE *btapp_dm_db_get_device_info(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (btapp_device_db.device[i].in_use
            && !memcmp(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return &btapp_device_db.device[i];
        }
    }

    /* we didn't find our device, look into the inquiry db */
    for (i=0; i<BTAPP_NUM_INQ_DEVICE; i++)
    {
        if (btapp_inq_db.remote_device[i].in_use
            && !memcmp(btapp_inq_db.remote_device[i].bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return &btapp_inq_db.remote_device[i];
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         btapp_dm_db_print_ble_inq
**
** Description      print ble inquired devices info
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_db_print_ble_inq(void)
{
    UINT8 i;
    UINT8 exist = FALSE;

    for(i = 0; i < BTAPP_NUM_INQ_DEVICE; i ++)
    {
        if(btapp_inq_db.remote_device[i].in_use && (btapp_inq_db.remote_device[i].device_type == BT_DEVICE_TYPE_BLE))
        {
            exist = TRUE;
            APPL_TRACE_DEBUG3("[%d]address %s, name:  %s", i, utl_bd_addr_to_string(btapp_inq_db.remote_device[i].bd_addr), btapp_inq_db.remote_device[i].name);
        }
    }

    if(exist == FALSE)
    {
        APPL_TRACE_ERROR0("Don't exist any inquiried ble devices!!!");
    }
}

/*******************************************************************************
**
** Function         btapp_dm_db_get_ble_inq_cnt
**
** Description      get the inquiried ble devices' counts.
**
**
** Returns          return inquiried ble counts
*******************************************************************************/
UINT8 btapp_dm_db_get_ble_inq_cnt(void)
{
    UINT8 i;
    UINT8 inq_cnt = 0;

    for(i = 0; i < BTAPP_NUM_INQ_DEVICE; i ++)
    {
        if(btapp_inq_db.remote_device[i].in_use && (btapp_inq_db.remote_device[i].device_type == BT_DEVICE_TYPE_BLE))
        {
            inq_cnt ++;
        }
    }

    return inq_cnt;
}

/*******************************************************************************
**
** Function         btapp_dm_db_get_device_list
**
** Description      gets the devices which mmets the input conditions
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_db_get_device_list(    tBTA_SERVICE_MASK services,
                                        tBTAPP_REM_DEVICE * p_device,
                                        UINT8*    number_of_devices,
                                        BOOLEAN new_only){
    UINT8 i;

    *number_of_devices = 0;
    if ( services == 0 )
        services = BTA_ALL_SERVICE_MASK;

    APPL_TRACE_DEBUG0("btapp_get_device_list");
    APPL_TRACE_DEBUG1("btapp_get_device_list - searched services = %x", services );

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (new_only == FALSE )
        {
            /* first, get the stored devices - only if not new_only asked */
            if (    btapp_device_db.device[i].in_use &&
                    (( btapp_device_db.device[i].services & services)||(services==BTA_ALL_SERVICE_MASK))  )
            {
                memcpy(&p_device[*number_of_devices], &btapp_device_db.device[i], sizeof(tBTAPP_REM_DEVICE));
                (*number_of_devices)++;
            }
        }

        /* then, get the new devices */
        APPL_TRACE_DEBUG1("btapp_get_device_list - device services = %x",btapp_inq_db.remote_device[i].services);

        if ( (btapp_inq_db.remote_device[i].in_use) && ( (btapp_inq_db.remote_device[i].services & services)||(services==BTA_ALL_SERVICE_MASK)) )
        {
            if ( &p_device[*number_of_devices] == NULL  )
            {
                APPL_TRACE_DEBUG0("pp_device[*number_of_devices] is NULL!");
            }
            else if ( &(btapp_inq_db.remote_device[i]) == NULL  )
            {
                APPL_TRACE_DEBUG0("&(btapp_inq_db[i]) is NULL!");
            }
            else
                memcpy(&p_device[*number_of_devices], &(btapp_inq_db.remote_device[i]), sizeof(tBTAPP_REM_DEVICE));

            (*number_of_devices) ++;
        }
    }

    APPL_TRACE_DEBUG1("%i devices into inq db",(*number_of_devices));

    if (*number_of_devices == 0 )
        return BTAPP_FAIL;

    return BTAPP_SUCCESS;
}

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)

/*******************************************************************************
**
** Function         btapp_dm_db_get_ble_bond_dev_cnt
**
** Description      get the bonded ble devices' counts.
**
**
** Returns          return bonded ble counts
*******************************************************************************/
UINT8 btapp_dm_db_get_ble_bond_dev_cnt(void)
{
    UINT8 i;
    UINT8 bond_cnt = 0;

    for(i = 0; i < BTAPP_NUM_REM_DEVICE; i++)
    {
        if(btapp_device_db.device[i].in_use && (btapp_device_db.device[i].bond_with_peer))
        {
            bond_cnt ++;
        }
    }

    return bond_cnt;
}

/*******************************************************************************
**
** Function         btapp_dm_db_get_device_list
**
** Description      gets the devices which mmets the input conditions
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_db_get_ble_bond_dev_list( tBTA_BLE_BOND_DEV *p_device,  UINT8 *number_of_devices )
{
    UINT8 i;

    if((NULL == p_device) || (NULL == number_of_devices))
    {
         return FALSE;
    }

    *number_of_devices = 0;

    APPL_TRACE_DEBUG0("btapp_dm_db_get_ble_bond_dev_list");

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if(btapp_device_db.device[i].in_use && (btapp_device_db.device[i].bond_with_peer))
        {
            UINT8 bond_index = *number_of_devices;

            memcpy(p_device[bond_index].bd_addr, btapp_device_db.device[i].bd_addr, BD_ADDR_LEN);
            p_device[bond_index].bond_key.key_mask = btapp_device_db.device[i].key_mask;
            memcpy(&p_device[bond_index].bond_key.penc_key, &btapp_device_db.device[i].penc_key, sizeof(tBTA_LE_PENC_KEYS));
            memcpy(&p_device[bond_index].bond_key.pcsrk_key, &btapp_device_db.device[i].pcsrk_key, sizeof(tBTA_LE_PCSRK_KEYS));
            memcpy(&p_device[bond_index].bond_key.pid_key, &btapp_device_db.device[i].pid_key, sizeof(tBTA_LE_PID_KEYS));

            (*number_of_devices)++;
        }

        /* then, get the new devices */
        //APPL_TRACE_DEBUG1("btapp_get_device_list - device services = %x",btapp_inq_db.remote_device[i].services);
    }

    APPL_TRACE_DEBUG1("%i devices into BONDED db",(*number_of_devices));

    if (*number_of_devices == 0 )
        return BTAPP_FAIL;

    return BTAPP_SUCCESS;
}

/*******************************************************************************
**
** Function         btapp_dm_ble_load_local_keys
**
** Description      load BLE local keys if available.
**
** Returns          TRUE if there is active codec
*******************************************************************************/
void btapp_dm_ble_load_local_keys (tBTA_DM_BLE_LOCAL_KEY_MASK *p_key_mask, BT_OCTET16 er,
                                   tBTA_BLE_LOCAL_ID_KEYS *p_id_keys){
    *p_key_mask = 0;

    if (btapp_ble_local_key.keys_mask & BTA_BLE_LOCAL_KEY_TYPE_ER)
    {
        *p_key_mask |= BTA_BLE_LOCAL_KEY_TYPE_ER;
        memcpy(er, btapp_ble_local_key.er, BT_OCTET16_LEN);
    }

    if (btapp_ble_local_key.keys_mask & BTA_BLE_LOCAL_KEY_TYPE_ID)
    {
        *p_key_mask |= BTA_BLE_LOCAL_KEY_TYPE_ID;
        memcpy(p_id_keys, &btapp_ble_local_key.id_keys, sizeof(tBTA_BLE_LOCAL_ID_KEYS));
    }
}

/*******************************************************************************
**
** Function         btapp_dm_security_grant
**
** Description      Action function to process security grant
**
** Returns          void
*******************************************************************************/
void btapp_dm_security_grant(BOOLEAN accept)
{

    tBTAPP_REM_DEVICE * p_device_rec;
    tBTAPP_REM_DEVICE   device_rec;

    if (accept)
    {
        if ((p_device_rec = btapp_get_device_record(btapp_cb.peer_bdaddr))!= NULL)
        {
            /* store LE device information in NV */
            btapp_store_device(p_device_rec);
        }
        else
        {
            memset(&device_rec, 0, sizeof(device_rec));
            strncpy(device_rec.name, btapp_cb.peer_name, BTAPP_DEV_NAME_LENGTH);
            bdcpy(device_rec.bd_addr, btapp_cb.peer_bdaddr);
            btapp_store_device(&device_rec);
        }

        BTA_DmBleSecurityGrant(btapp_cb.peer_bdaddr, BTA_DM_SEC_GRANTED);
    }
    else
    {
        BTA_DmBleSecurityGrant(btapp_cb.peer_bdaddr, BTA_DM_SEC_PAIR_NOT_SPT);
    }
}

/*******************************************************************************
**
** Function         btapp_dm_ble_passkey_reply
**
** Description      Send BLE passkey reply.
**
** Returns          void
*******************************************************************************/
void btapp_dm_ble_passkey_reply(BD_ADDR bd_addr, BOOLEAN accept, UINT32 passkey)
{
    BTA_DmBlePasskeyReply(bd_addr, accept, passkey);
}

#if SMP_INCLUDED == TRUE && SMP_LE_SC_INCLUDED == TRUE
/*******************************************************************************
**
** Function         btapp_dm_ble_confirm_reply
**
** Description      Send BLE user confirmation reply.
**
** Returns          void
*******************************************************************************/
void btapp_dm_ble_confirm_reply(BD_ADDR bd_addr, BOOLEAN accept)
{
    BTA_DmBleConfirmReply(bd_addr, accept);
}
#endif

#if BLE_BRCM_INCLUDED == TRUE
/*******************************************************************************
**
** Function         btapp_dm_ble_rssi_notify_cback
**
** Description      RSSI monitor cross threshold notification callback,
**
** Returns          void
*******************************************************************************/
void btapp_dm_ble_rssi_notify_cback(BD_ADDR bd_addr, UINT8 alert_evt, INT8 rssi)
{
    char buf[256];
    UINT8   xx = 0;

    if (alert_evt == BTA_BLE_RSSI_ALERT_HI)
    {
        xx += sprintf(&buf[0],"hi alert");

        APPL_TRACE_EVENT0("hi alert");
    }
    else if (alert_evt == BTA_BLE_RSSI_ALERT_LO)
    {
        xx += sprintf(&buf[0],"low alert");
        APPL_TRACE_EVENT0("low alert");
    }
    else if (alert_evt == BTA_BLE_RSSI_ALERT_RANGE)
    {
        xx += sprintf(&buf[0],"In Range alert");
        APPL_TRACE_EVENT0("In Range alert");
    }
    sprintf(&buf[xx],"event:%x RSSI reading: %i", alert_evt ,rssi);
    APPL_TRACE_EVENT1("%s", buf);
}

/*******************************************************************************
**
** Function         btapp_dm_ble_rssi_monitor
**
** Description      monitor a link RSSI between the remote bda and local device
**
** Returns          void
*******************************************************************************/
void btapp_dm_ble_rssi_monitor(BD_ADDR remote_bda, UINT8 alert_mask, INT8 low, INT8 range, INT8 hi)
{
    BTA_DmBleMonitorLinkRSSI(remote_bda, alert_mask, low, range, hi, btapp_dm_ble_rssi_notify_cback);
}


static void read_rssi_cb(tBTM_VSC_CMPL *p1)
{
    UINT8 param_buf[20];
    UINT8 i ,idx = 0;
    UINT8 rssi =0; //it should use the signed type to define the rssi value, but shell_printf can;t print signed value,
                   //if define the signed value, it'll print 0 when the value is negative value.
                   //Because shell_printf can't format print signed value, so need to convert manually.
    UINT8 minus = 0;
    UINT8* p_tmp = p1->p_param_buf;

    if(p1->param_len == 4)
    {
        if(p1->p_param_buf[0] == 0x02)
        {
            APPL_TRACE_EVENT0("HCI LE RSSI Error State!\r\n");
        }
        else if(p1->p_param_buf[0] == 0x00)
        {
            if(p1->p_param_buf[3] < 128)
            {
                rssi = p1->p_param_buf[3];
                minus = 0;
            }
            else if(p1->p_param_buf[3] == 128)
            {
                rssi = 0;
                minus = 0;
            }
            else
            {
                rssi = (255 - p1->p_param_buf[3]) + 1;
                minus = 1;
            }

            APPL_TRACE_EVENT2("HCI LE RSSI Value: %s%d \r\n", ((minus == 1) ? "-": ""), rssi);
        }
        else
        {
            APPL_TRACE_EVENT0("HCI LE RSSI Error Status!\r\n");
        }
    }
#if 0
    for(i = 0; i < p1->param_len; i ++)
    {
        idx += sprintf((char*)&param_buf[idx], "0x%02x ", p1->p_param_buf[i]);
    }

    APPL_TRACE_EVENT1("%s\r\n", param_buf);
#endif
}

static void* rssi_task_main(void* arg)
{

    read_rssi_task_enable = 1;

    while(read_rssi_task_enable)
    {
       if (!btu_hci_read_raw_rssi( conn_bda, BT_TRANSPORT_LE, (void*)read_rssi_cb))
       {
           read_rssi_task_enable = 0;
           break;
       }
       sleep(2);
    }

    printf("unknown BD_ADDR, exit rssi_task\r\n");
    pthread_exit(NULL);
    return NULL;
}
#endif
#endif

