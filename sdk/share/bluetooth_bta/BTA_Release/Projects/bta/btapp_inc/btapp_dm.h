/*****************************************************************************
**
**  Name:             btapp_dm.h
**
**  Description:     This file contains btapp internal interface definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_DM_H
#define BTAPP_DM_H

#include "gki.h"
#include "bta_rt_api.h"
#include "btapp_int.h"

#define BTAPP_AUTH_REQ_NO            0   /* always not required */
#define BTAPP_AUTH_REQ_YES           1   /* always required */
#define BTAPP_AUTH_REQ_GEN_BOND      2   /* security database + general bonding, dedicated bonding as not required */
#define BTAPP_AUTH_REQ_DEFAULT       3   /* security database, dedicated bonding as not required */
#define BTAPP_AUTH_REQ_GEN_BOND_DD   4   /* security database + general bonding, dedicated bonding as required */
#define BTAPP_AUTH_REQ_DEFAULT_DD    5   /* security database, dedicated bonding as required */

#define BTAPP_AUTH_REQ_DD_BIT        4   /* dedicated bonding bit */

#if BTM_OOB_INCLUDED == TRUE && BTM_BR_SC_INCLUDED == TRUE
#define BTAPP_OOB_PRESENT            1
#define BTAPP_OOB_PRESENT_192_256    3
#endif

/* AG/HS will invoke SCO callout, need to identify different profile by APP ID */
/* AG use app ID 1, and 2, assign app ID 3 to HS */
#define BTAPP_DM_SCO_4_HS_APP_ID     3   /* SCO over HCI app ID for HS */


#if (BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE)
#define BTAPP_LE_AUTH_REQ_NO_BOND_NO_MITM    SMP_AUTH_NO_BOND                   /* 0000 */
#define BTAPP_LE_AUTH_REQ_NO_BOND_MITM       SMP_AUTH_YN_BIT                    /* 0100 */
#define BTAPP_LE_AUTH_REQ_BOND_NO_MITM       SMP_AUTH_BOND                      /* 0001 */
#define BTAPP_LE_AUTH_REQ_BOND_MITM         (SMP_AUTH_YN_BIT | SMP_AUTH_BOND)   /* 0101 */
#endif

/** Bluetooth Out Of Band data for bonding */
typedef struct
{
  UINT8 le_bt_dev_addr[6]; /* LE Bluetooth Device Address */
  UINT8 c192[16];          /* Simple Pairing Hash C-192 */
  UINT8 r192[16];          /* Simple Pairing Randomizer R-192 */
  UINT8 c256[16];          /* Simple Pairing Hash C-256 */
  UINT8 r256[16];          /* Simple Pairing Randomizer R-256 */
  UINT8 sm_tk[16];         /* Security Manager TK Value */
  UINT8 le_sc_c[16];       /* LE Secure Connections Confirmation Value */
  UINT8 le_sc_r[16];       /* LE Secure Connections Random Value */
} btapp_dm_out_of_band_data_t;

/* this structure holds optional OOB data for remote device */
typedef struct
{
  BD_ADDR bdaddr;            /* peer bdaddr */
  btapp_dm_out_of_band_data_t oob_data;
} btapp_dm_oob_cb_t;

extern btapp_dm_oob_cb_t oob_cb;

extern void btapp_startup(void);
extern void btapp_dm_init(void);
extern void btapp_dm_post_reset(void);
extern void btapp_dm_pin_code_reply(BOOLEAN accept, UINT8 pin_len, UINT8 *p_pin);
extern void btapp_dm_confirm_reply(BOOLEAN accept);
extern void btapp_dm_passkey_cancel(void);
extern void btapp_dm_passkey_reply(tBTM_STATUS res, BD_ADDR bd_addr, UINT32 passkey);
extern void btapp_dm_rmt_oob_reply(BOOLEAN accept, BT_OCTET16 c, BT_OCTET16 r);
#if (BTM_BR_SC_INCLUDED == TRUE)
extern void btapp_dm_rmt_oob_ext_reply(BOOLEAN accept, BT_OCTET16 c_192, BT_OCTET16 r_192, BT_OCTET16 c_256, BT_OCTET16 r_256);
#endif
extern tBTA_STATUS btapp_dm_create_oob_bond(BD_ADDR bd_addr, int transport, btapp_dm_out_of_band_data_t* oob_data);
extern void btapp_dm_loc_oob(void);
extern void btapp_dm_ble_oob_req_evt(tBTA_DM_SP_RMT_OOB* req_oob_type);
extern void btapp_dm_ble_sc_oob_req_evt(tBTA_DM_SP_RMT_OOB* req_oob_type);
extern void btapp_dm_set_oob_for_le_io_req(const BD_ADDR bd_addr, tBTA_OOB_DATA* p_has_oob_data, tBTA_LE_AUTH_REQ* p_auth_req);
extern void btapp_dm_authorize_resp(tBTA_AUTH_RESP response);
extern void btapp_dm_disable_bt(void);
extern void btapp_dm_enable_test_mode(void);
extern void btapp_dm_disable_test_mode(void);
extern void btapp_dm_add_custom_uuid (tBT_UUID *p_uuid);
extern void btapp_dm_remove_custom_uuid (tBT_UUID *p_uuid);
extern void btapp_dm_set_visibility( BOOLEAN is_visible, BOOLEAN is_temp);
extern void btapp_dm_set_find_me( void);
extern void btapp_dm_set_pairability( BOOLEAN is_pairable, BOOLEAN is_temp);
extern void btapp_dm_set_connectability_paired_only(BOOLEAN conn_paired_only, BOOLEAN is_temp);
extern void btapp_dm_set_local_name(char *p_name);
extern void btapp_dm_get_local_name(char *p_name);
extern BOOLEAN btapp_dm_is_enabled();
extern void btapp_dm_set_local_name_le(char *p_name);
extern void btapp_dm_close_acl(BD_ADDR remote_bda, BOOLEAN remove_dev, tBTA_TRANSPORT transport);
extern BOOLEAN btapp_dm_set_trusted(tBTA_SERVICE_MASK trusted_mask, tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_dm_set_not_trusted(tBTAPP_REM_DEVICE * p_device_rec);
extern BOOLEAN btapp_dm_delete_device(BD_ADDR bd_addr);
extern BOOLEAN btapp_dm_remove_ble_device(BD_ADDR bd_addr);
extern void btapp_dm_discover_device(BD_ADDR bd_addr, BOOLEAN is_new, tBTA_TRANSPORT transport);
extern void btapp_dm_clear_inq_db(BD_ADDR bd_addr);
extern BOOLEAN btapp_dm_stored_device_unbond (void);
extern void btapp_dm_cancel_search(void);
extern void btapp_dm_search(tBTA_SERVICE_MASK services,tBTA_DM_INQ *p_data);
extern void btapp_dm_search_ext(tBTA_SERVICE_MASK_EXT *p_services,tBTA_DM_INQ *p_data, tBTA_DM_SEARCH_CBACK *cf);
extern BOOLEAN btapp_dm_add_device(void);
extern void btapp_dm_sec_add_device(tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_dm_bond(tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_dm_bond_cancel(tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_dm_rename_device(tBTAPP_REM_DEVICE * p_device_rec, UINT8 * p_text);
extern void btapp_add_devices(void);
extern tBTAPP_REM_DEVICE * btapp_dm_db_get_device_info(BD_ADDR bd_addr);
extern void btapp_dm_db_print_ble_inq(void);
extern UINT8 btapp_dm_db_get_ble_inq_cnt(void);
extern BOOLEAN btapp_dm_db_get_device_list( tBTA_SERVICE_MASK services, tBTAPP_REM_DEVICE * p_device,UINT8*  number_of_devices, BOOLEAN new_only);

extern UINT8 btapp_dm_db_get_ble_bond_dev_cnt(void);
extern BOOLEAN btapp_dm_db_get_ble_bond_dev_list( tBTA_BLE_BOND_DEV *p_device,  UINT8 *number_of_devices );

extern void btapp_dm_proc_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                                 tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig);
extern void btapp_dm_proc_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap, tBTA_AUTH_REQ auth_req);
extern void btapp_dm_proc_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade);
extern void btapp_dm_read_remote_device_name(BD_ADDR bd_addr, tBTA_TRANSPORT transport);

/* DI discovery functions */
extern void btapp_dm_di_discover(BD_ADDR bd_addr);
extern UINT16 btapp_dm_add_di_record(BOOLEAN is_primary);
extern tBTA_STATUS btapp_dm_get_di_remote_record(tBTA_DI_GET_RECORD *p_record, UINT8 index);
extern tBTA_STATUS btapp_dm_get_di_local_record(tBTA_DI_GET_RECORD *p_di_record, UINT32 handle);

/* MPS discovery functions */
extern void btapp_dm_mps_discover(BD_ADDR bd_addr);
extern void btapp_dm_update_mps_record(BOOLEAN expose_record);

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE)
extern void btapp_dm_switch_bb2mm(void);
extern void btapp_dm_switch_mm2bb(void);
#endif

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
extern void btapp_dm_switch_bb2btc(void);
extern void btapp_dm_switch_btc2bb(void);
#endif

#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    #define IS_ENCODER(x)   (x & (AUDIO_CODEC_SBC_ENC | AUDIO_CODEC_MP3_ENC | AUDIO_CODEC_AAC_ENC))
    #define IS_DECODER(x)   (x & (AUDIO_CODEC_SBC_DEC | AUDIO_CODEC_MP3_DEC | AUDIO_CODEC_AAC_DEC))

extern void btapp_dm_set_codec_config (UINT16 hndl, tAUDIO_CODEC_TYPE codec, tCODEC_INFO codec_info);
extern void btapp_dm_route_audio(UINT16 hndl,
                                 tBTA_DM_ROUTE_PATH src_path,
                                 tBTA_DM_ROUTE_PATH out_path,
                                 tAUDIO_ROUTE_SF src_sf,
                                 tAUDIO_ROUTE_SF codec_sf,
                                 tAUDIO_ROUTE_SF i2s_sf);
extern void btapp_dm_read_audio_route_info(UINT16 hndl);
extern void btapp_dm_mix_audio (UINT16 hndl,
                                tAUDIO_ROUTE_MIX    mix_src,
                                tAUDIO_ROUTE_SF     mix_src_sf,
                                tMIX_SCALE_CONFIG   mix_scale,
                                tCHIRP_CONFIG      *p_chirp_config);
extern BOOLEAN btapp_dm_fill_burst_data(void *p_data, UINT16 length);
extern UINT16 btapp_dm_get_burst_size();
extern void btapp_dm_flush_burstbuffer (void);
extern void btapp_dm_set_eq_mode (UINT16 hndl, tAUDIO_ROUTE_EQ audio_eq_mode, tEQ_GAIN_CONFIG *gain_cfg);
extern void btapp_dm_set_audio_scale (UINT16 hndl, tMIX_SCALE_CONFIG scale);
extern void btapp_dm_update_bitrate (UINT16 hndl, tAUDIO_CODEC_TYPE codec_type, UINT32 param);
extern BOOLEAN btapp_dm_is_bta2dp_routed (tAUDIO_ROUTE_OUT rt_out);
extern BOOLEAN btapp_dm_is_host_routed (tAUDIO_ROUTE_OUT rt_out);
extern BOOLEAN btapp_dm_is_codec_active (void);
#endif /* (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) */

#if (BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE)
extern void btapp_dm_proc_ble_io_req(BD_ADDR bd_addr,
                                     tBTA_IO_CAP *p_io_cap,
                                     tBTA_OOB_DATA *p_oob_data,
                                     tBTA_LE_AUTH_REQ *p_auth_req,
                                     UINT8 *p_max_key_size,
                                     tBTA_LE_KEY_TYPE *p_init_key,
                                     tBTA_LE_KEY_TYPE *p_resp_key);
extern void btapp_dm_ble_load_local_keys (UINT8 *p_key_mask, BT_OCTET16 er, tBTA_BLE_LOCAL_ID_KEYS *p_id_keys);
extern void btapp_dm_security_grant(BOOLEAN accept);
extern void btapp_dm_ble_passkey_reply(BD_ADDR bd_addr, BOOLEAN accept, UINT32 passkey);
#if SMP_LE_SC_INCLUDED == TRUE
extern void btapp_dm_ble_confirm_reply(BD_ADDR bd_addr, BOOLEAN accept);
#endif
extern void btapp_dm_set_le_scatternet( BOOLEAN enable);
extern void btapp_dm_set_le_visibility( UINT16 le_disc_mode, UINT16 le_conn_mode,BOOLEAN is_temp );
extern void btapp_dm_set_le_dynscan( BOOLEAN enable);
extern void btapp_dm_transport_bond(tBTAPP_REM_DEVICE * p_device_rec, tBTA_TRANSPORT transport);

#if BLE_BRCM_INCLUDED == TRUE
extern void btapp_dm_ble_rssi_monitor(BD_ADDR remote_bda, UINT8 alert_mask, INT8 low, INT8 range, INT8 hi);
#endif
#endif /* BLE_INCLUDED && SMP_INCLUDED */

extern void btapp_dm_handle_authenticate_complete_event(tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data);
extern void btapp_dm_handle_ble_key_evt(tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data);

/* Buffer for parsing/building avrc messages for btapp_av, btapp_rc, and btapp_avk */
#define BTAPP_DM_AVRC_BUF_SIZE  2048
extern UINT8 btapp_dm_avrc_buf[BTAPP_DM_AVRC_BUF_SIZE];

#endif
