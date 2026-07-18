/*****************************************************************************
**
**  Name:             btapp_gattc.h
**
**  Description:     This file contains btapp gattc interface
**				     definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_GATTC_H
#define BTAPP_GATTC_H

#include "bta_gatt_api.h"

#define BTAPP_GATTC_FIRST_EVT                   (BTA_GATTC_EVT_MAX + 1)
#define BTAPP_GATTC_FIND_FIRST_INCL_SRVC_EVT    (BTAPP_GATTC_FIRST_EVT+1)
#define BTAPP_GATTC_FIND_NEXT_INCL_SRVC_EVT     (BTAPP_GATTC_FIRST_EVT+2)
#define BTAPP_GATTC_FIND_FIRST_DESCR_EVT        (BTAPP_GATTC_FIRST_EVT+3)
#define BTAPP_GATTC_FIND_NEXT_DESCR_EVT         (BTAPP_GATTC_FIRST_EVT+4)

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)
typedef struct
{
    UINT16  conn_id;
    BOOLEAN in_use;
    BOOLEAN connected;
    BD_ADDR bda;
    tBTA_GATTC_IF client_if;
    UINT8       transport;
}tBTAPP_GATTCC_CLCB;

#define BTAPP_GATTC_MAX_CL GATT_CL_MAX_LCB

typedef struct
{
    BOOLEAN                 registered;
    tBT_UUID                app_id;
    tBTA_GATTC_IF           client_if;
    tBTA_GATTC_CHAR_ID      indicate_char_id;
    BD_ADDR                 connecting_bda;
#if BTAPP_BRCM_CS_INCLUDED
    BOOLEAN                 enabled;
#endif
    tBTAPP_GATTCC_CLCB        clcb[BTAPP_GATTC_MAX_CL];
} tBTAPP_GATTC_CB;

extern tBTAPP_GATTC_CB btapp_gattc_cb;

extern void btapp_gattc_init(void);
extern void btapp_gattc_deinit(void);

extern BOOLEAN btapp_gatt_is_gattc_type(BD_ADDR bd_addr);
extern void btapp_gattc_open(BD_ADDR remote_bda, BOOLEAN is_direct);
extern void btapp_gattc_cancel_open(tBTA_GATTC_IF cif, BD_ADDR remote_bda, BOOLEAN is_direct);

extern void btapp_gattc_close (BD_ADDR remote_bda);
extern void btapp_gattc_cfg_mtu(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 mtu);

extern void btapp_gattc_read_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                           UINT16 char_uuid, UINT8 char_inst, UINT16 descr_uuid, UINT8 auth_req, BOOLEAN is_primary, UINT8 transport);
extern void btapp_gattc_discover(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 transport);
extern void btapp_gattc_find_first_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 char_uuid, BOOLEAN is_primary, UINT8 transport);
extern void btapp_gattc_find_next_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 start_uuid, UINT8 start_id, UINT16 char_uuid, BOOLEAN is_primary, UINT8 transport);
extern void btapp_gattc_find_first_incl_srvc(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 char_uuid, BOOLEAN is_primary, UINT8 transport);
extern void btapp_gattc_find_next_incl_srvc(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst, BOOLEAN is_primary, UINT16 start_uuid, UINT8 start_id, UINT16 char_uuid, UINT8 transport);
extern void btapp_gattc_find_first_descr(tBTA_GATTC_IF cif, BD_ADDR remote_bda,
                                 UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 char_uuid,
                                 UINT8 char_inst, UINT16 descr_cond, BOOLEAN is_primary, UINT8 transport);
extern void btapp_gattc_find_next_descr(tBTA_GATTC_IF cif, BD_ADDR remote_bda,
                                 UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 char_uuid,
                                 UINT8 char_inst, UINT16 start_descr_uuid,
                                 UINT16 descr_cond, UINT8 descr_inst, BOOLEAN is_primary, UINT8 transport);

extern void btapp_gattc_write_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                           UINT16 char_uuid, UINT8 char_inst, UINT8 write_type, UINT16 len, UINT8 *p_data, UINT8 auth_req, BOOLEAN is_primary, UINT8 transport);
extern void btapp_gattc_write_descr(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                           UINT16 char_uuid, UINT8 char_inst, UINT16 descr, UINT8 write_type, tBTA_GATT_UNFMT *p_value, UINT8 auth_req, BOOLEAN is_primary, UINT8 transport);
extern void btapp_gattc_send_handle_value_confirm(tBTA_GATTC_IF cif, BD_ADDR remote_bdae, UINT8 transport);
extern void btapp_gattc_register_4_notif(tBTA_GATTC_IF cif, BD_ADDR bda, UINT16 srvc_uuid, UINT8 srvc_inst_id,
                                  UINT16 char_uuid, UINT8 char_inst_id, BOOLEAN is_primary);
extern void btapp_gattc_deregister_4_notif(tBTA_GATTC_IF cif, BD_ADDR bda, UINT16 srvc_uuid, UINT8 inst_id, UINT16 char_uuid, UINT8 char_inst_id, BOOLEAN is_primary);
extern void btapp_gattc_exec_write_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, BOOLEAN is_execute, UINT8 transport);
extern void btapp_gattc_prep_write_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                           UINT16 char_uuid, UINT8 char_inst, UINT16 offset, UINT16 len, UINT8* p_value, UINT8 auth_req, BOOLEAN is_primary, UINT8 transport);

/* NV cache management */
extern BOOLEAN btapp_gattc_proc_cache_load(BD_ADDR server_bda,UINT16 max_attr, UINT16 *p_num_attr, tBTA_GATTC_NV_ATTR *p_attr, UINT16 start_idx);
extern BOOLEAN btapp_gattc_proc_cache_save(BD_ADDR addr, UINT16 num_attr, tBTA_GATTC_NV_ATTR *p_attr_list, UINT16 attr_index);
extern BOOLEAN btapp_gattc_proc_cache_open(BD_ADDR addr, BOOLEAN to_save);
extern void btapp_gattc_clear_nv_cache(BD_ADDR server_bda);

/* clcb */
extern tBTAPP_GATTCC_CLCB *btapp_gattc_find_clcb( tBTA_GATTC_IF client_if, BD_ADDR bda, UINT8 transport);
extern tBTAPP_GATTCC_CLCB *btapp_gattc_clcb_alloc (UINT16 conn_id,tBTA_GATTC_IF client_if, BD_ADDR bda, tBTA_TRANSPORT transport);
extern BOOLEAN btapp_gatts_clcb_dealloc (UINT16 conn_id);

/* BRCM customer specific feature testing */
extern void btapp_observer_disable(void);
extern void btapp_observer_enable(void);

#endif

#endif /* BTAPP_GATTC_H */
