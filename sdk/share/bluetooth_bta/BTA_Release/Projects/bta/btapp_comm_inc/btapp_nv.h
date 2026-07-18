/****************************************************************************
**
**  Name:          btapp_nv.h
**
**  Description:   Contains btapp nvram abstraction header file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_NV_H
#define BTAPP_NV_H

#include "btapp_int.h"

void btapp_init_device_db (void);
BOOLEAN btapp_store_device( tBTAPP_REM_DEVICE * p_rem_device);
tBTAPP_REM_DEVICE * btapp_get_device_record(BD_ADDR bd_addr);
UINT8 btapp_get_device_record_idx(BD_ADDR bd_addr);
tBTAPP_REM_DEVICE * btapp_alloc_device_record(BD_ADDR bd_addr);
tBTAPP_REM_DEVICE * btapp_get_inquiry_record(BD_ADDR bd_addr);

void btapp_nv_store_device_db(void);
void btapp_nv_init_device_db(void);

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
void btapp_nv_init_ble_info(void);
void btapp_nv_store_ble_local_keys(void);

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)
void btapp_nv_init_gattc_db(void);
void btapp_nv_store_gattc_db(void);
void btapp_nv_init_gatts_hndl_range_db(void);
void btapp_nv_store_gatts_hndl_range_db(void);
void btapp_nv_init_gatts_srv_chg_db(void);
void btapp_nv_store_gatts_srv_chg_db(void);
#endif
#endif

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
void btapp_nv_init_hs_db(void);
void btapp_nv_store_hs_db(void);
#endif

void btapp_delete_device(BD_ADDR bd_addr);

#endif
