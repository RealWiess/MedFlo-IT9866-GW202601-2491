/****************************************************************************
**
**  Name:          btapp_comm.h
**
**  Description:   Contains btapp common header file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_COMM_H
#define BTAPP_COMM_H

#include "bta_api.h"

/* Event groups for BTE_appl_task events */
#define BTAPP_EVT_GROUP_CONSOLE     0x1000
#define BTAPP_EVT_GROUP_BTA         0x2000
#define BTAPP_EVT_GROUP_AVK         0x3000
#define BTAPP_EVT_GROUP_BTU         0x4000
#define BTAPP_EVT_GROUP_MASK        0xF000

#define MENU_ITEM_0 '0'
#define MENU_ITEM_1 '1'
#define MENU_ITEM_2 '2'
#define MENU_ITEM_3 '3'
#define MENU_ITEM_4 '4'
#define MENU_ITEM_5 '5'
#define MENU_ITEM_6 '6'
#define MENU_ITEM_7 '7'
#define MENU_ITEM_8 '8'
#define MENU_ITEM_9 '9'

/* events from BTU */
enum
{
    BTAPP_BTU_START_TIMER    =   BTAPP_EVT_GROUP_BTU
};


/* events from CONSOLE */
enum
{
    BTAPP_CONSOLE_CMD      =     BTAPP_EVT_GROUP_CONSOLE
};

#define BTAPP_DATA_LEN          56
#define BTAPP_DEV_NAME_LENGTH   32
#define BTAPP_INST_INFO_LEN     200

/* data type for console message */
typedef struct
{
    BT_HDR    hdr;
    UINT8     data[BTAPP_DATA_LEN + 1];
} tBTAPP_CONSOLE_MSG;

#endif
