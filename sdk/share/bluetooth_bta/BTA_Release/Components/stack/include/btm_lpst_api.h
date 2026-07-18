/*****************************************************************************
**
**  Name:          btm_lpst_api.h
**
**  Description:   This file contains the Bluetooth Manager (BTM) API function
**                 external definitions.
**
** Broadcom Proprietary and Confidential. (C) 2017 Broadcom. All rights reserved.
******************************************************************************/
#ifndef BTM_LPST_API_H
#define BTM_LPST_API_H

#include "btm_ble_api.h"
#include "gatt_api.h"
#include "port_api.h"

/* LPST bridge device role */
#define BTM_LPST_ROLE_NONE      HCI_ROLE_UNKNOWN
#define BTM_LPST_ROLE_PE        HCI_ROLE_MASTER
#define BTM_LPST_ROLE_SE        HCI_ROLE_SLAVE

#ifndef BTM_LPST_DEBUG_DUMP
#define BTM_LPST_DEBUG_DUMP     FALSE
#endif

/* SYNC start/complete timeout value in second */
#define BTM_LPST_SYNC_START_TOUT            4
/* shadow handle timeout value in second */
#define BTM_LPST_SHADOW_TOUT                1

#define BTM_LPST_OFFSET    (L2CAP_MIN_OFFSET + 1)
enum
{
    BTM_LPST_LCID_RFCOMM,
    BTM_LPST_LCID_AVDT,
    BTM_LPST_LCID_AVCT,
    BTM_LPST_MAX_LCID_NUM
};

/* profile LPST host data forwarding */
#define BTM_LPST_ACL_DATA_OPCODE    (GATT_OP_CODE_MAX + 1)  /* 32/0x20 */
/* 33/0x21 - not used */
#define BTM_LPST_SYNC_MSG_OPCODE    (GATT_OP_CODE_MAX + 3)  /* 34/0x22*/
#define BTM_LPST_DISABLED_OPCODE    (GATT_OP_CODE_MAX + 4)  /* 35/0x23 */
#define BTM_LPST_SYNC_CBLK_OPCODE   (GATT_OP_CODE_MAX + 5)  /* 36/0x24 */
#define BTM_LPST_ROLE_CFG_OPCODE    (GATT_OP_CODE_MAX + 6)  /* 37/0x25*/
#define BTM_LPST_SWITCH_OPCODE      (GATT_OP_CODE_MAX + 7)  /* 38/0x26*/
#define BTM_LPST_OPCODE_MAX         (BTM_LPST_SWITCH_OPCODE)

#define BTM_LPST_APP_OPCODE_BASE    (GATT_OP_CODE_MAX + 0x10)


/* Used only with BRCM_LPST_INCLUDED */
#define BTM_LPST_HEADER_LEN         ((UINT16)(L2CAP_MIN_OFFSET + 1))

/* the len for BTM_LPST_ROLE_CFG_OPCODE */
#define BTM_LPST_ROLE_CFG_LEN         (1)
/* the len for L2C_LPST_CB_CACHE_OK */
#define BTM_LPST_CACHE_OK_LEN         (2) /* l2cb.first_cid */
/* the len for L2C_LPST_CB_BR_INFO */
#define BTM_LPST_BR_INFO_LEN          (10) /* BD_ADDR (6) + connected_mask(4) */

#define BTM_LPST_SYNC_CBLK_HEADER_SIZE  3 /* 1/control block id + 2 byte len */
/* subcode for BTM_LPST_SYNC_CBLK_OPCODE (control block id) */
#define BTM_LPST_SYNC_CBLK_SUB_BTM      0x01
#define BTM_LPST_SYNC_CBLK_SUB_L2CAP    0x02
#define BTM_LPST_SYNC_CBLK_SUB_RFCOMM   0x03
#define BTM_LPST_SYNC_CBLK_SUB_AVDTP    0x04
#define BTM_LPST_SYNC_CBLK_SUB_AVCTP    0x05
#define BTM_LPST_SYNC_CBLK_SUB_BTA      0x06 /* DM/SYS */
#define BTM_LPST_SYNC_CBLK_SUB_AVK      0x07
#define BTM_LPST_SYNC_CBLK_SUB_HS       0x08
#define BTM_LPST_SYNC_CBLK_SUB_DG       0x09        /* DG */

#define BTM_LPST_SYNC_CBLK_BIT_BTM      0x0002
#define BTM_LPST_SYNC_CBLK_BIT_L2CAP    0x0004
#define BTM_LPST_SYNC_CBLK_BIT_RFCOMM   0x0008
#define BTM_LPST_SYNC_CBLK_BIT_AVDTP    0x0010
#define BTM_LPST_SYNC_CBLK_BIT_AVCTP    0x0020
#define BTM_LPST_SYNC_CBLK_BIT_BTA      0x0040
#define BTM_LPST_SYNC_CBLK_BIT_AVK      0x0080
#define BTM_LPST_SYNC_CBLK_BIT_HS       0x0100
#define BTM_LPST_SYNC_CBLK_BIT_DG       0x0200
#define BTM_LPST_SYNC_CBLK_PROFILES     0x0380

/* LPST disable reason */
#define BTM_LPST_ROLE_FAILED            1   /* BR link role. */
#define BTM_LPST_SHADOW_FAILED          2   /* no shadow HCI handle. */
#define BTM_LPST_SHADOW_DROP            3   /* shadow link drop (on SE). */

/* LPST switch state*/
#define BTM_LPST_SWITCH_ACTIVE              1
#define BTM_LPST_SWITCH_WAIT_4_CB_CMPL      2
#define BTM_LPST_SWITCH_NONE                0

/* LPST event callback event used in tBTM_LPST_INT_EVT_CBACK(bta) */
#define BTM_LPST_SWITCH_INIT_EVT        1
#define BTM_LPST_SWITCH_CMPL_EVT        2
#define BTM_LPST_ROLE_UPDATE_EVT        3
#define BTM_LPST_SWITCH_REQ_EVT         4
#define BTM_LPST_ROLE_EXCHANGE_EVT      5   /* ROLE_CFG exchanged right after LE link up */
#define BTM_LPST_DISABLED_EVT           6   /* LPST is disabled due to failure */
#define BTM_LPST_SYNC_ADDR_EVT          7   /* LE link up; tell bta to start control block sync */

/* Union of all tBTM_LPST_INT_EVT_CBACK data */
typedef struct
{
    BD_ADDR addr;
    BOOLEAN sent;
} tBTM_LPST_SYNC_ADDR;

typedef union
{
    UINT8               new_role;
    UINT8               fail_reason;
    tBTM_LPST_SYNC_ADDR sync_addr;
} tBTM_LPST_INT_EVT_DATA;
typedef BOOLEAN (tBTM_LPST_INT_EVT_CBACK) (UINT8 event, UINT8 status, tBTM_LPST_INT_EVT_DATA *p_data);

/* LPST event callback event used in tBTM_LPST_EVENT_CBACK(app) */
#define BTM_LPST_AVK_MEDIA_START_EVT        1
#define BTM_LPST_AVK_SYNC_READY_EVT         2
#define BTM_LPST_AVK_STREAM_MODE_EVT        3

typedef struct
{
    BD_ADDR addr;
    UINT16  handle;
} tBTM_LPST_SHADOW_ADDR;

typedef union
{
    UINT8               num_streams;
    tBTM_LPST_SHADOW_ADDR shadow;
} tBTM_LPST_EVENT_DATA;
typedef void (tBTM_LPST_EVENT_CBACK) (UINT8 event, tBTM_LPST_EVENT_DATA *p_data);

typedef BOOLEAN (tBTM_LPST_DATA_HANDLER) (UINT8 op_code, BT_HDR *p_data);
typedef void (tBTM_LPST_SYNC_HANDLER) (UINT8 sync_code, UINT8 *p, UINT16 len);
#ifdef __cplusplus
extern "C"
{
#endif

BTM_API extern void btm_lpst_register_data_handler(tBTM_LPST_DATA_HANDLER *p_handler);
BTM_API extern void btm_lpst_register_event_handler(tBTM_LPST_EVENT_CBACK *p_handler);
BTM_API extern BOOLEAN btm_lpst_send_generic_data_to_bridge (UINT8 op_code, BT_HDR *p_buf);

#if (defined BRCM_LPST_INCLUDED) && (BRCM_LPST_INCLUDED == TRUE)
BTAPI extern void btm_lpst_bridge_disable (void);
BTAPI extern UINT8 btm_lpst_get_local_device_role (void);
extern BOOLEAN btm_lpst_send_acl_traffic_to_bridge (UINT16 cid, BT_HDR *p_buf, UINT16 len, UINT16 offset);
extern BOOLEAN btm_lpst_bridge_process_data (UINT8 op_code, BT_HDR *p_buf);
extern void btm_lpst_config_bridge_role(BD_ADDR peer_bda, BOOLEAN connected);
extern BOOLEAN btm_lpst_skip_tx_cmd (BT_HDR *p_buf);
extern BOOLEAN btm_lpst_skip_tx_acl (UINT16 handle, BT_HDR *p_buf);
extern void btm_lpst_set_media_channel_id (UINT16 cid);
extern BOOLEAN btm_lpst_update_media_packet (UINT16 *handle, BT_HDR *p_buf);
extern BOOLEAN btm_lpst_skip_rx_acl (BT_HDR *p_buf);
extern void btm_lpst_check_link_role(BD_ADDR bda, UINT8 link_role);
extern BOOLEAN btm_lpst_is_bridge_bda (BD_ADDR peer_bda);
extern BOOLEAN btm_lpst_shadow_connected(BD_ADDR bda, UINT16 handle, UINT8 status);
extern void btm_lpst_trace_handles(void);
extern void btm_lpst_hci_disconnected(UINT16 handle);
extern void btm_lpst_ble_evt(UINT8 *p, UINT16 evt_len);

void btm_lpst_set_forward_channel (UINT16 cid, int channel_idx);
BTAPI extern void btm_lpst_set_cblk_required (UINT8 cblk_id);
BTAPI extern void btm_lpst_clear_cblk_required (UINT8 cblk_id);
extern void btm_lpst_check_wake_br_link (void);
BTAPI extern UINT16 btm_lpst_get_cblk_required (void);
extern UINT8 * btm_lpst_get_addr( void);
extern BOOLEAN btm_lpst_br_is_connected(void);
extern void btm_lpst_set_br_connected(BD_ADDR bda);

/* from btm_lpst.c */
BTAPI extern void btm_lpst_dump_hex (UINT8 *p, char *p_title, UINT16 len);
extern void btm_lpst_process_control_blocks (BT_HDR *p_buf);

/* from l2c_lpst.c */
extern void l2c_lpst_send_sync(UINT16 lcid);
extern void l2c_lpst_process_control_blocks (UINT8 *p, UINT16 cb_len);
extern void l2c_lpst_send_close (UINT16 close_lcid, UINT8 cb_id, UINT8 cb_sub_id);
extern void l2c_lpst_release_ccb (UINT16 lcid);
extern void l2c_lpst_process_close (BD_ADDR bd_addr);
extern void l2c_lpst_save_cache (UINT16 handle);
extern void l2c_lpst_add_cache_info (BT_HDR *p_buf);
extern UINT16 l2c_lpst_add_cache_ok (UINT8 *p);
extern void l2c_lpst_free_cache (void);
extern void l2c_lpst_process_cache_info (UINT8 *p, UINT16 len);
extern void l2c_lpst_restore_cache (UINT8 *p);
BTAPI extern BOOLEAN l2c_lpst_validate_switch_state(void);

/* from rfc_lpst.c */
extern void rfc_lpst_process_control_blocks (UINT8 *p, UINT16 cb_len);
BTAPI extern void rfc_lpst_send_sync (UINT16 handle);
BTAPI extern void rfc_lpst_set_mgt_cback(UINT16 port_handle, tPORT_CALLBACK *p_mgmt_cb);
extern void rfc_lpst_send_chnl_close (UINT16 lcid);
extern void rfc_lpst_send_port_close(UINT16 handle);

/* from avdt_lpst.c */
BTAPI extern void avdt_lpst_send_sync(UINT8 handle, BD_ADDR bd_addr);
BTAPI extern void avdt_lpst_process_control_blocks (UINT8 *p, UINT16 cb_len);
extern void avdt_lpst_clear_cb (void);
extern UINT8 avdt_lpst_get_scb_state (UINT16 lcid);
extern void avdt_lpst_resume_start(UINT8 lpst_role);
extern void avdt_lpst_check_stream_state(UINT16 lcid);
BTAPI extern BOOLEAN avdt_lpst_validate_switch_state(void);

/* from avct_lpst.c */
BTAPI extern void avct_lpst_send_sync(UINT8 handle, BOOLEAN for_latecomer);
extern void avct_lpst_process_control_blocks (UINT8 *p, UINT16 cb_len);
extern void avct_lpst_process_open(BD_ADDR bd_addr);

typedef void (tBTM_LPST_SYNC_CMPL_CBACK) (UINT8 status, void *p_ref);
extern BOOLEAN btm_lpst_sync_start (tBTM_LPST_SYNC_CMPL_CBACK *p_sync_cmpl_cback, void *p_ref);
BTAPI extern void btm_lpst_shadow (void);
extern void btm_lpst_sync_stop (void);
BTM_API extern void btm_lpst_register_sync_handler(tBTM_LPST_SYNC_HANDLER *p_handler);
extern BOOLEAN btm_lpst_validate_switch_state (void);
BTM_API extern BOOLEAN btm_lpst_is_switch_active (void);

extern void btm_lpst_init (void);
extern void btm_lpst_pm_reg (void);

BTM_API BOOLEAN btm_lpst_role_switch_start (void);
BTM_API extern void btm_lpst_register_int_evt_cback(tBTM_LPST_INT_EVT_CBACK *p_cback);
extern void btm_lpst_switch_tout(void);
extern void btm_lpst_shadow_tout(void);
extern void btm_lpst_shadow_fail(void);

extern BT_HDR * btm_lpst_prepare_acl_data_to_bridge (UINT16 cid, BT_HDR *p_buf, UINT16 len, UINT16 offset);
BTM_API extern void btm_lpst_set_rfcomm_mask (UINT8 scn);
extern void btm_lpst_check_rfcomm_mask (UINT8 scn);
extern void btm_lpst_congestion_cback(BOOLEAN congested);


#endif

#ifdef __cplusplus
}
#endif

#endif

