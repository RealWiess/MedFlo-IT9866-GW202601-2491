/****************************************************************************
**
**  Name:          btapp_gatt_ibeacon.h
**
**  Description:   Contains gatt iBeacon header.
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef __BTAPP_GATT_IBEACON_H__
#define __BTAPP_GATT_IBEACON_H__

#include "data_types.h"
#include "bd.h"
#include "bta_gatt_api.h"

/* max adv data length */
#define APP_BLE_IBEACON_MAX_ADV_DATA_LEN        25
/* BLE IBEACON Helper UUID Definitions */
#define APP_BLE_IBEACON_APP_IBEACON_UUID       0x9890
#define APP_BLE_IBEACON_APP_CLIENT_UUID        0x9891
#define APP_BLE_IBEACON_APP_SERV_UUID          0xC158
#define APP_BLE_IBEACON_APP_CHAR_UUID          0xC188

#define APP_BLE_IBEACON_APP_ATTR_NUM           1
#define APP_BLE_IBEACON_APP_NUM_OF_SERVER      1
#define APP_BLE_IBEACON_APP_NUM_OF_CLIENT      1
#define APP_BLE_IBEACON_CLIENT_INFO_DATA_LEN   100

#define BTAPP_IBEACON_MAX_CHAR_NUM             3
#define BTAPP_IBEACON_HANDLE_NUM               ((3*BTAPP_IBEACON_MAX_CHAR_NUM)+1)

#define BTAPP_IBEACON_PERM                     (BTA_GATT_PERM_READ)
#define BTAPP_IBEACON_PROP                     (BTA_GATT_CHAR_PROP_BIT_READ |BTA_GATT_CHAR_PROP_BIT_NOTIFY)
#define BTAPP_IBEACON_CFG_PERM                 (BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE)
#define BTAPP_IBEACON_VALUE_LEN                10
#define BTAPP_IBEACON_ADJ_REASON_DST           (1 << 3)
#define BTAPP_IBEACON_ADJ_REASON_TIME_CONE     (1 << 2)
#define BTAPP_IBEACON_ADJ_REASON_EXT_REF       (1 << 1)
#define BTAPP_IBEACON_ADJ_REASON_MANU          (1 << 0)
#define BTAPP_IBEACON_DEF_ADJUST_DEF           BTAPP_IBEACON_ADJ_REASON_MANU


#define BTAPP_IBEACON_INST_MAX                 1
#define BTAPP_IBEACON_PREP_MAX                 4

#define BTAPP_IBEACON_CONSOLE_INCLUDE 		   FALSE

typedef struct
{
    BD_ADDR     remote_bda;
    UINT16      client_cfg;
}tBTAPP_IBEACON_CFG_INST;

#define BTAPP_IBEACON_CFG_MAX         4

typedef struct
{
    UINT16                    service_id;
    UINT16                    ibeacon_id;      /* IBEACON charatceristic handle*/
    UINT16                    clt_cfg_id;      /* client configuration handle */
    tBTAPP_IBEACON_CFG_INST   cfg_inst[BTAPP_IBEACON_CFG_MAX];
    UINT8                     adjust_reason;
}tBTAPP_IBEACON_INTS;

typedef struct
{
    UINT16  conn_id;
    BOOLEAN in_use;
    BOOLEAN connected;
    BD_ADDR bda;
}tBTAPP_IBEACON_CLCB;

#define BTAPP_IBEACON_MAX_CL GATT_CL_MAX_LCB

typedef struct
{
    BOOLEAN                     enabled;
    BOOLEAN                     need_dereg;
    tBTA_GATTS_IF               server_if;
    UINT8                       inst_id;
    tBTAPP_IBEACON_INTS         srvc_inst[BTAPP_IBEACON_INST_MAX];    /* service instance */
    UINT16                      prep_q[BTAPP_IBEACON_PREP_MAX];
    UINT8                       prep_num;
    tBTAPP_IBEACON_CLCB         clcb;
}tBTAPP_IBEACON_PERIPHERAL_CB;

void btapp_ble_ibeacon_start_ibeacon(void);
void btapp_ble_ibeacon_start_adv(UINT8 inst_id);
void btapp_ble_ibeacon_stop_adv(void);
void btapp_ble_ibeacon_stop_ibeacon(void);

#endif
