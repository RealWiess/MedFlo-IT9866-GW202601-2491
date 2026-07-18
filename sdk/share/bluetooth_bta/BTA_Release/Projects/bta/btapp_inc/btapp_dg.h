/*****************************************************************************
**
**  Name:             btapp_dg.h
**
**  Description:     This file contains btapp internal interface
**                   definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_DG_H
#define BTAPP_DG_H

#include "btapp_dg_iap2.h"

#define BTAPP_DG_ID_SPP_0          0
#define BTAPP_DG_ID_SPP_1          1
#define BTAPP_DG_ID_SPP_2          2
#define BTAPP_DG_ID_SPP_3          3

#define BTAPP_DG_NUM_SERVICES      4

#define BTAPP_DG_DATA_TEST_TOUT    2
#define BTAPP_DG_SENT_Q_MAX_CNTS   10

typedef struct
{
    BUFFER_Q        loopback_data_q;
    BUFFER_Q        sent_data_q;
    uint16_t          port_handle;
    uint16_t          api_handle;
    uint16_t          tx_sent;
    uint16_t          rx_sent;
    uint16_t          mtu;
    bool         is_open;
    bool         is_used;
    uint8_t           port_id;
    tBTA_SERVICE_ID service_id;
    uint32_t          time_taken;
    uint32_t          data_send;
    uint32_t          prev_data_send;
    uint32_t          data_recvd;
    uint32_t          prev_data_recvd;
    TIMER_LIST_ENT  data_test_tle;
    uint8_t           data_pattern;
} tBTAPP_DG_APP_CB;

/* typedef for BTAPP DG control block */
typedef struct
{
    tBTAPP_DG_APP_CB app_cb[BTAPP_DG_NUM_SERVICES];
    bool         flowed_off;
    uint16_t          flowed_port_handle;
	btapp_dg_parsedataCB_t parseDataCB;
} tBTAPP_DG_CB;

extern tBTAPP_DG_CB btapp_dg_cb;

extern void btapp_dg_init(void);
extern void btapp_dg_register_entry(uint8_t index);
extern void btapp_dg_deregister_entry(uint8_t index);
extern void btapp_dg_register_entry_nums(uint8_t nums);
extern void btapp_dg_deregister_all(void);
extern void btapp_dg_close_connection(uint8_t conn_index);
extern void btapp_dg_connect(void);
extern void btapp_dg_set_device_authorized (tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_dg_spp_send_data(uint8_t app_id, uint8_t* pbuf, uint16_t len);
extern void btapp_dg_spp_rcv_data(uint8_t app_id, uint8_t* pbuf, uint16_t len);
extern void btapp_dg_spp_test_send(void);
extern void btapp_dg_spp_send_raw(uint8_t test_cnt, uint16_t send_interval, uint8_t* buf, uint16_t len);
extern void btapp_dg_spp_send_tht(uint8_t app_id, uint16_t test_cnt);

#endif
