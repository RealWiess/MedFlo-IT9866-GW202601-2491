/*****************************************************************************
**
**  Name:             btapp_gatts.h
**
**  Description:     This file contains btapp server interface
**                   definition.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_GATTS_H
#define BTAPP_GATTS_H

#include "bta_gatt_api.h"

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)

#define BTAPP_GATTS_MAX_SERV                1 //in this case, we just create 1 server  with 12 services
#define BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV    12 //3
#define BTAPP_GATTS_MAX_CCCD_IN_ONE_SERV    12 //

#define BTAPP_GATTS_SIMPLE_CHAR_SIZE        512

typedef struct
{
    UINT16  conn_id;
    BOOLEAN in_use;
    BOOLEAN connected;
    BD_ADDR bda;
    tBTA_GATTS_IF server_if;
    tBTA_TRANSPORT  transport;
    BOOLEAN     congested;
    UINT32      loop;
}tBTAPP_GATTS_CLCB;

typedef struct
{
    tBT_UUID*               p_parent_uuid;
    UINT16                  attr_id;
    UINT16                  cccd_cfg;
}tBTAPP_GATTS_CHAR_CFG_INST;

typedef struct
{
    tBT_UUID*               p_char_uuid;
    UINT16                  attr_id;
    UINT8                   attr_val[BTAPP_GATTS_SIMPLE_CHAR_SIZE];
}tBTAPP_GATTS_CHAR_INST;

typedef struct
{
    UINT8                   created;
    UINT16                  service_id;
    tBT_UUID*               p_serv_uuid;
    tBTAPP_GATTS_CHAR_INST  char_inst[BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV];
    tBTAPP_GATTS_CHAR_CFG_INST char_cfg[BTAPP_GATTS_MAX_CCCD_IN_ONE_SERV];
}tBTAPP_GATTS_SERV_INST;

typedef struct
{
    BOOLEAN                 in_use;
    tBTA_GATTS_IF           server_if;
    BD_ADDR                 bda;
}tBTAPP_GATTS_BG_CONN;

#define BTAPP_GATTS_BG_CONN_MAX      10

typedef struct
{
    BOOLEAN                 registered;
    tBTA_GATTS_IF           server_if;
    tBT_UUID                app_id;
    BOOLEAN                 need_dereg;
    tBTAPP_GATTS_BG_CONN    bg_conn_list[BTAPP_GATTS_BG_CONN_MAX];
    tBTAPP_GATTS_SERV_INST  serv_inst[BTAPP_GATTS_MAX_SERV];
    tBTAPP_GATTS_CLCB       clcb[GATT_CL_MAX_LCB];
} tBTAPP_GATTS_CB;

typedef struct
{
	UINT16	 len;
	UINT8    resp_val[GATT_MAX_ATTR_LEN];
}tBTAPP_GATTS_CB_RET;

typedef tBTAPP_GATTS_CB_RET* (*GATTS_READ_PROCESS_CB)();
typedef void (*GATTS_WRITE_PROCESS_CB)(UINT8*);



extern tBTAPP_GATTS_CB btapp_gatts_cb;

extern void btapp_gatts_start(void);
extern void btapp_gatts_stop(void);
extern void btapp_gatts_adv_start(void);
extern void btapp_gatts_adv_stop(void);

extern void btapp_gatts_register(tBT_UUID *p_app_uuid);
extern void btapp_gatts_deregister(tBTA_GATTS_IF sif);

extern void btapp_gatts_open(tBTA_GATTS_IF sif, BD_ADDR remote_bda, BOOLEAN is_direct, UINT8 use_br_edr);
extern void btapp_gatts_cancel_open(tBTA_GATTS_IF sif, BD_ADDR remote_bda, BOOLEAN is_direct);

extern void btapp_gatts_close (tBTA_GATTS_IF sif, BD_ADDR remote_bda, UINT8 use_br_edr );
extern void btapp_gatts_notify(tBTA_GATTS_IF sif, BD_ADDR remote_bda,
                        UINT16 attr_handle, UINT16 len, UINT8* p_value, UINT8 use_br_edr);
extern void btapp_gatts_send_2_peer(UINT8* p_data, UINT8 len);
extern void btapp_gatts_notify_loop (tBTA_GATTS_IF sif, BD_ADDR remote_bda, UINT8 use_br_edr, UINT32 loop);
extern tBTAPP_GATTS_CLCB *btapp_gatts_find_clcb( tBTA_GATTS_IF sif, BD_ADDR bda, UINT8 use_br_edr);
extern tBTAPP_GATTS_CLCB *btapp_gatts_clcb_alloc (UINT16 conn_id, tBTA_GATTS_IF sif, BD_ADDR bda,  tBTA_TRANSPORT  transport);
extern BOOLEAN btapp_gatts_clcb_dealloc (UINT16 conn_id);

extern void btapp_gatts_server_load_bonded(tBTA_GATTS_IF server_if, UINT32 service_mask);
extern void btapp_gatts_server_unload_bond(tBTA_GATTS_IF server_if);

extern void btapp_gatts_add_service_uuid(tBT_UUID *p_app_uuid, int idx);
extern void btapp_gatts_register_read_cb_function(int idx, GATTS_READ_PROCESS_CB pFun);
extern void btapp_gatts_register_write_cb_function(int idx, GATTS_WRITE_PROCESS_CB pFun);




/* NV cache management */


#endif

#endif /* BTAPP_GATTS_H */
