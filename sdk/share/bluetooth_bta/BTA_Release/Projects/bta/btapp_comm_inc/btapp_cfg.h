/****************************************************************************
**
**  Name:          btapp_cfg.h
**
**  Description:   Contains btapp configuration header file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_CFG_H
#define BTAPP_CFG_H

#include "btapp_int.h"

#define BTAPP_TRACE_APP_MASK        0x0001
#define BTAPP_TRACE_HCI_MASK        0x0002
#define BTAPP_TRACE_BTM_MASK        0x0004
#define BTAPP_TRACE_L2CAP_MASK      0x0008
#define BTAPP_TRACE_RFCOMM_MASK     0x0010
#define BTAPP_TRACE_GAP_MASK        0x0020
#define BTAPP_TRACE_HSP2_MASK       0x0040
#define BTAPP_TRACE_SPP_MASK        0x0080
#define BTAPP_TRACE_SMP_MASK        0x0100
#define BTAPP_TRACE_SDP_MASK        0x0200
#define BTAPP_TRACE_GATT_MASK       0x0400
#define BTAPP_TRACE_BTA_MASK        0x0800
#define BTAPP_TRACE_RESERVER2_MASK  0x1000
#define BTAPP_TRACE_RESERVER3_MASK  0x2000
#define BTAPP_TRACE_RESERVER4_MASK  0x4000
#define BTAPP_TRACE_RESERVER5_MASK  0x8000


extern tBTAPP_CFG btapp_cfg;
extern UINT16 btapp_trace_layer;

void btapp_cfg_init(void);
void btapp_cfg_trace_enable(UINT8 enabled);
void btapp_cfg_trace_layer(UINT16 trace_layer_mask);
void btapp_cfg_set_default_addr(BD_ADDR addr);
void btapp_cfg_set_no_lpo(UINT8 enabled);
tBTA_STATUS btapp_cfg_set_io_caps(tBTA_IO_CAP io_cap);
tBTA_STATUS btapp_cfg_set_auth_req(tBTA_AUTH_REQ auth_req);
tBTA_STATUS btapp_cfg_set_accept_auth_enable(UINT8 accept_auth_enable);
tBTA_STATUS btapp_cfg_set_init_key(UINT8 init_key);
tBTA_STATUS btapp_cfg_set_rsp_key(UINT8 rsp_key);
tBTA_STATUS btapp_cfg_set_max_key_size(UINT8 key_size);
tBTA_STATUS btapp_cfg_set_min_key_size(UINT8 key_size);

#endif
