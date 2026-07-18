/*****************************************************************************
**
**  Name:             btapp_pan.h
**
**  Description:     This file contains btapp internal interface definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "gki.h"

#ifndef BTAPP_PAN_H
#define BTAPP_PAN_H

#define BTAPP_PAN_ID_PANU            0
#define BTAPP_PAN_ID_NAP             1
#define BTAPP_PAN_ID_GN              2

#define BTAPP_PAN_NUM_SERVICES       3

#define BTAPP_PAN_ST_NONE            0x0000
#define BTAPP_PAN_ST_CONNECTABLE     0x0001
#define BTAPP_PAN_ST_CONNECT         0x0002
#define BTAPP_PAN_ST_DISCONNECT      0x0004

typedef UINT16 tBTAPP_PAN_STATUS;

/* BTAPP pan control block */
typedef struct
{

     BT_HDR             * p_rx_buf;
     UINT16             conn_handle;
     BD_ADDR            peer_bdaddr;
     UINT16             rx_buf_len;
     UINT16             tx_sent;
     UINT16             rx_sent;
     BOOLEAN            is_open;
     UINT8              port_id;
     tBTA_SERVICE_ID service_id;
     BOOLEAN            ping_sent;


} tBTAPP_PAN_APP_CB;

/* typedef for BTAPP PAN control block */
typedef struct
{
     tBTAPP_PAN_APP_CB  app_cb[BTAPP_PAN_NUM_SERVICES];
     BOOLEAN            uart_data;
     UINT8              port_id;
     UINT16             active_handle;
     tBTAPP_PAN_STATUS   status;
} tBTAPP_PAN_CB;

extern tBTAPP_PAN_CB btapp_pan_cb;

#define BTAPP_PAN_SETSTATUS(m)           (btapp_pan_cb.status |= (m))
#define BTAPP_PAN_RESETSTATUS(m)         (btapp_pan_cb.status &= (~(m)))
#define BTAPP_PAN_GETSTATUS(m)           (btapp_pan_cb.status & m)

extern void btapp_pan_init(void);
/*TBD: void btapp_pan_close(UINT8 conn_index) for GU/NAP */
//extern void btapp_pan_close(UINT8 conn_index);
extern void btapp_pan_close(void);
extern void btapp_pan_open_conn(tBTA_PAN_ROLE peer_role);
extern void btapp_pan_set_device_authorized (tBTAPP_REM_DEVICE * p_device_rec);

#endif
