/*****************************************************************************
**
**  Name:           bta_dm_co.c
**
**  Description:    This file contains the device manager callout function
**                  implementation for Insight.
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

/* during first phase of porting,
 * this can be set to FALSE to reduce the number of files/functions included from platform dependent code */
#ifndef BTA_DM_CO_INCLUDE_BTAPP
#define BTA_DM_CO_INCLUDE_BTAPP      TRUE
#endif

#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
#include "btapp_int.h"
#include "btapp_dm.h"
#endif

static UINT8 ble_accept_auth_enable = BTM_BLE_ONLY_ACCEPT_SPECIFIED_SEC_AUTH_DISABLE;
static UINT8 ble_auth_req = 0;

/*******************************************************************************
**
** Function         bta_dm_co_ble_scan_param
**
** Description      This callout function is executed by DM to get ble scan paramters from configuration.
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req - TRUE, if MITM protection is required.
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_ble_scan_param(UINT16* ble_scan_int, UINT16* ble_scan_win, UINT8* ble_scan_type)
{
  *ble_scan_int  = btapp_cfg.ble_scan_int;
  *ble_scan_win  = btapp_cfg.ble_scan_win;
  *ble_scan_type = btapp_cfg.ble_scan_type;

}

/*******************************************************************************
**
** Function         bta_dm_co_io_req
**
** Description      This callout function is executed by DM to get IO capabilities
**                  of the local device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req - TRUE, if MITM protection is required.
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                      tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig)
{
    /* if OOB is not supported, this call-out function does not need to do anything
     * otherwise, look for the OOB data associated with the address and set *p_oob_data accordingly
     * If the answer can not be obtained right away,
     * set *p_oob_data to BTA_OOB_UNKNOWN and call bta_dm_ci_io_req() when the answer is available */

    /* *p_auth_req by default is FALSE for devices with NoInputNoOutput; TRUE for other devices. */

#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
    btapp_dm_proc_io_req(bd_addr, p_io_cap, p_oob_data, p_auth_req, is_orig);
#else
    *p_oob_data = BTA_OOB_NONE;
#endif
}

/*******************************************************************************
**
** Function         bta_dm_co_io_rsp
**
** Description      This callout function is executed by DM to report IO capabilities
**                  of the peer device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  io_cap - The remote Input/Output capabilities
**                  oob_data - TRUE, if OOB data is available for the peer device.
**                  auth_req - TRUE, if MITM protection is required.
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap,
                      tBTA_OOB_DATA oob_data, tBTA_AUTH_REQ auth_req)
{
#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
    btapp_dm_proc_io_rsp(bd_addr, io_cap, auth_req);
#endif
}

/*******************************************************************************
**
** Function         bta_dm_co_lk_upgrade
**
** Description      This callout function is executed by DM to check if the
**                  platform wants allow link key upgrade
**
** Parameters       bd_addr  - The peer device
**                  *p_upgrade - TRUE, if link key upgrade is desired.
**
** Returns          void.
**
*******************************************************************************/
void  bta_dm_co_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade )
{
#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
    btapp_dm_proc_lk_upgrade(bd_addr, p_upgrade);
#endif
}

#if (BTM_OOB_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_loc_oob
**
** Description      This callout function is executed by DM to report the OOB
**                  data of the local device for the Simple Pairing process
**
** Parameters       valid - TRUE, if the local OOB data is retrieved from LM
**                  c     - Simple Pairing Hash C
**                  r     - Simple Pairing Randomnizer R
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r)
{
#if (defined(NFC_INCLUDED) && (NFC_INCLUDED == TRUE))
    extern void btapp_cho_proc_loc_oob (BOOLEAN valid, BT_OCTET16 hash_c, BT_OCTET16 rand_r);
#endif

#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
    extern void btapp_dm_proc_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r);
    /* process the local oob data. */
    btapp_dm_proc_loc_oob(valid, c, r);
#endif

#if (defined(NFC_INCLUDED) && (NFC_INCLUDED == TRUE))
    btapp_cho_proc_loc_oob (valid, c, r);
#endif
}

#if (BTM_BR_SC_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_loc_oob_ext
**
** Description      This callout function is executed by DM to report the OOB
**                  extended data of the local device for the Simple Pairing process
**                  This function is called instead of bta_dm_co_loc_oob(...)
**                  if the local device supports Secure Connections (SC)
**
** Parameters       valid - TRUE, if the local OOB extended data is retrieved from LM
**                  c_192     - Simple Pairing Hash C derived from the P-192 public key
**                  r_192     - Simple Pairing Randomnizer R associated with the P-192 public key
**                  c_256     - Simple Pairing Hash C derived from the P-256 public key
**                  r_256     - Simple Pairing Randomnizer R associated with the P-256 public key
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_loc_oob_ext(BOOLEAN valid, BT_OCTET16 c_192, BT_OCTET16 r_192,
                                          BT_OCTET16 c_256, BT_OCTET16 r_256)
{
#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
    extern void btapp_dm_proc_loc_oob_ext(BOOLEAN valid, BT_OCTET16 c_192, BT_OCTET16 r_192, BT_OCTET16 c_256, BT_OCTET16 r_256);
    /* process the local oob data. */
    btapp_dm_proc_loc_oob_ext(valid, c_192, r_192, c_256, r_256);
#endif

}
#endif /* BTM_BR_SC_INCLUDED */

/*******************************************************************************
**
** Function         bta_dm_co_rmt_oob
**
** Description      This callout function is executed by DM to request the OOB
**                  data for the remote device for the Simple Pairing process
**                  Need to call bta_dm_ci_rmt_oob() in response
**
** Parameters       bd_addr  - The peer device
**
** Returns          TRUE    if the data is successfully retrieved,
**                  FALSE   otherwise.
**
** Note             This function is platform specific and does nothing in
**                  btapp_app.
**
*******************************************************************************/
BOOLEAN bta_dm_co_rmt_oob(BD_ADDR bd_addr)
{
    return TRUE;
}
#endif /* BTM_OOB_INCLUDED */

#if BTM_SCO_INCLUDED == TRUE
/*******************************************************************************
**
** Function         bta_dm_sco_co_init
**
** Description      This function can be used by the platform to initialize audio
**                  codec or for other initialization purposes before SCO connection
**                  is opened.
**
**                  Note: Input Data Path is configured in this callout function
**                  p_parms->[in/out]put_data_path
**                          BTM_DATA_PATH_PCM, BTM_DATA_PATH_HCI, BTM_DATA_PATH_TEST
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_init(tBTM_ENH_ESCO_PARAMS *p_parms, UINT8 app_id)
{
    if (p_parms->input_data_path == BTM_DATA_PATH_HCI)
    {
        APPL_TRACE_DEBUG1("bta_dm_sco_co_init <ENTER>: input_path HCI Transport (%u)",
                            p_parms->input_data_path);
    }
    else
    {
        APPL_TRACE_DEBUG2("bta_dm_sco_co_init <ENTER>: input_path PCM (%u), coding_format:%d",
                            p_parms->input_data_path, p_parms->rx_cf.coding_format);
    }

#if (BTM_SCO_HCI_INCLUDED == TRUE )
    //If openning sco over hci, use bta_hs_esco_params->input_data_path/output_data_path => BTM_DATA_PATH_HCI
#else
    p_parms->input_data_path = p_parms->output_data_path = BTM_DATA_PATH_PCM;
#endif

    /* set up SCO routing configuration if SCO over HCI app ID is used and run time
        configuration is set to SCO over HCI */
    /* HS invoke this call-out */

    if(p_parms->input_data_path == BTM_DATA_PATH_PCM)
    {
        if(p_parms->rx_cf.coding_format == BTM_CODING_FORMAT_CVSD)
        {
            btm_cb.sco_cb.codec_in_use = BTM_SCO_CODEC_MSBC;
            BTM_ConfigI2SPCM(BTM_SCO_CODEC_CVSD, HCI_BRCM_I2SPCM_IS_MASTER, HCI_BRCM_I2SPCM_SAMPLE_8K,\
                HCI_BRCM_I2SPCM_CLOCK_512K);
        }
#if (BTM_WBS_INCLUDED == TRUE )
        else if(p_parms->rx_cf.coding_format == BTM_SCO_CODEC_MSBC)
        {
            btm_cb.sco_cb.codec_in_use = BTM_SCO_CODEC_CVSD;
            BTM_ConfigI2SPCM(BTM_SCO_CODEC_MSBC, HCI_BRCM_I2SPCM_IS_MASTER, HCI_BRCM_I2SPCM_SAMPLE_16K,\
                HCI_BRCM_I2SPCM_CLOCK_512K);
        }
#endif
    }

    /* no codec is used for the SCO data */
    if (p_parms->rx_cf.coding_format != BTM_CODING_FORMAT_MSBC
        && p_parms->input_data_path == BTM_DATA_PATH_HCI)
    {
        /* initialize SCO codec */

    }

    if (p_parms->input_data_path == BTM_DATA_PATH_HCI)
    {
        APPL_TRACE_DEBUG1("bta_dm_sco_co_init <EXIT>: input_path HCI Transport (%u)",
                            p_parms->input_data_path);
    }
    else
    {
        APPL_TRACE_DEBUG1("bta_dm_sco_co_init <EXIT>: input_path PCM (%u)",
                            p_parms->input_data_path);
    }
}

#endif  /* BTM_SCO_INCLUDED */

#if (BTM_SCO_HCI_INCLUDED == TRUE ) && (BTM_SCO_INCLUDED == TRUE)

/*******************************************************************************
**
** Function         btapp_sco_codec_callback
**
** Description      Callback for btapp codec.
**
**
** Returns          void
**
*******************************************************************************/
static void btapp_sco_codec_callback(UINT16 event, UINT16 sco_handle)
{
    bta_dm_sco_ci_data_ready(event, sco_handle);
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_open
**
** Description      This function is executed when a SCO connection is open.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_open(UINT16 handle, UINT8 pkt_size, UINT16 event)
{
    tBTA_SCO_CODEC_CFG cfg;
    APPL_TRACE_DEBUG0("bta_dm_sco_co_open");

    if (HCI_BRCM_SCO_ROUTE_HCI == bta_sco_get_route())
    {
        APPL_TRACE_DEBUG3("bta_dm_sco_co_open handle:%d pkt_size:%d event:%d", handle, pkt_size, event);
        /* use dedicated SCO buffer pool for SCO TX data */
        cfg.pool_id = HCI_SCO_POOL_ID;
        cfg.p_cback = btapp_sco_codec_callback;
        cfg.pkt_size = pkt_size;
        cfg.cb_event = event;
        cfg.sco_handle = handle;
        /* open and start the codec */
        bta_sco_codec_open(&cfg);
    }
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_close
**
** Description      This function is called when a SCO connection is closed
**
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_close(void)
{
    APPL_TRACE_DEBUG0("bta_dm_sco_co_close()");

    bta_sco_codec_close();
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_in_data
**
** Description      This function is called to send incoming SCO data to application.
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_in_data(BT_HDR  *p_buf, tBTM_SCO_DATA_FLAG status)
{
    UINT8   *pp = (UINT8 *)(p_buf + 1) + p_buf->offset;
    UINT8   pkt_len = *(pp+2);

    AUDIO_SCO_In_Data((UINT8 *)(pp + 3), pkt_len);
    GKI_freebuf(p_buf);

    bta_sco_data_cback(pkt_len);
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_out_data
**
** Description      This function is called to send SCO data over HCI.
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_out_data(BT_HDR  **p_buf)
{
    bta_sco_codec_readbuf(p_buf);
}

#endif /* #if (BTM_SCO_HCI_INCLUDED == TRUE ) && (BTM_SCO_INCLUDED == TRUE)*/


#if (BLE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_ble_load_local_keys
**
** Description      This callout function is to load the local BLE ER if available
**                  on the device.
**
** Parameters       none
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_ble_load_local_keys (tBTA_DM_BLE_LOCAL_KEY_MASK *p_key_mask, BT_OCTET16 er,
                                    tBTA_BLE_LOCAL_ID_KEYS *p_id_keys)
{
#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
    btapp_dm_ble_load_local_keys(p_key_mask, er, p_id_keys);
#endif
}
#if (SMP_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_ble_io_req
**
** Description      This callout function is executed by DM to get BLE IO capabilities
**                  before SMP pairing gets going.
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req -  Auth request setting (Bonding and MITM required or not)
**                  *p_max_key_size - max key size local device supported.
**                  *p_init_key - initiator keys.
**                  *p_resp_key - responder keys.
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_ble_io_req(BD_ADDR bd_addr,  tBTA_IO_CAP *p_io_cap,
                         tBTA_OOB_DATA *p_oob_data,
                         tBTA_LE_AUTH_REQ *p_auth_req,
                         UINT8 *p_max_key_size,
                         tBTA_LE_KEY_TYPE *p_init_key,
                         tBTA_LE_KEY_TYPE  *p_resp_key)
{
    /* if OOB is not supported, this call-out function does not need to do anything
     * otherwise, look for the OOB data associated with the address and set *p_oob_data accordingly
     * If the answer can not be obtained right away,
     * set *p_oob_data to BTA_OOB_UNKNOWN and call bta_dm_ci_io_req() when the answer is available */

    /* *p_auth_req by default is FALSE for devices with NoInputNoOutput; TRUE for other devices. */

#if (BTA_DM_CO_INCLUDE_BTAPP == TRUE)
    btapp_dm_proc_ble_io_req(bd_addr, p_io_cap, p_oob_data, p_auth_req, p_max_key_size, p_init_key, p_resp_key);
#endif
}
#endif

void bta_dm_co_ble_set_accept_auth_enable(UINT8 enable)
{
#if (SMP_INCLUDED == TRUE)
    if (enable)
    {
        enable = BTM_BLE_ONLY_ACCEPT_SPECIFIED_SEC_AUTH_ENABLE;
        GAP_SetSecurityMode (BTM_SEC_MODE_SC);
    }
    else
    {
        GAP_SetSecurityMode (BTM_SEC_MODE_SP);
    }
    ble_accept_auth_enable = enable;
#endif
}

UINT8 bta_dm_co_ble_get_accept_auth_enable(void)
{
#if (SMP_INCLUDED == TRUE)
    return ble_accept_auth_enable;
#endif
    return 0;
}

void bta_dm_co_ble_set_auth_req(UINT8 ble_auth_req)
{
#if (SMP_INCLUDED == TRUE)
    ble_auth_req = ble_auth_req;
#endif
}

UINT8 bta_dm_co_ble_get_auth_req(void)
{
#if (SMP_INCLUDED == TRUE)
    return ble_auth_req;
#endif
    return 0;
}

#endif
