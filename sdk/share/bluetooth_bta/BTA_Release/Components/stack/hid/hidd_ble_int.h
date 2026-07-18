/****************************************************************************/
/*                                                                          */
/*  Name:       hidd_int.h                                                  */
/*                                                                          */
/*  Function:   this file contains HID DEVICE internal definitions          */
/*                                                                          */
/*                                                                          */
/*  Copyright (c) 2002-2004, WIDCOMM Inc., All Rights Reserved.                  */
/*  WIDCOMM Bluetooth Core. Proprietary and confidential.                   */
/*                                                                          */
/****************************************************************************/

#ifndef HIDD_BLE_INT_H
#define HIDD_BLE_INT_H

#include "hidd_ble_api.h"
#include "gatt_api.h"

#define HID_LE_MAX_RPT_NUM              6
#define HID_LE_MAX_CHAR_NUM             (4 + HID_LE_MAX_RPT_NUM)
#define HID_LE_MAX_NUM_INC_SVR       2
#define HID_LE_MAX_ATTR_NUM          (3 * HID_LE_MAX_CHAR_NUM + HID_LE_MAX_NUM_INC_SVR + 1)
#define HID_LE_MAX_CHAR_VALUE_SIZE   (30 + HID_LE_CHAR_DEV_NAME_SIZE)


#ifndef HID_LE_ATTR_DB_SIZE
    #define HID_LE_ATTR_DB_SIZE   GATT_DB_MEM_SIZE(HID_LE_MAX_NUM_INC_SVR, HID_LE_MAX_CHAR_NUM, HID_LE_MAX_CHAR_VALUE_SIZE)
#endif


/* by default include the optional write permission */
#ifndef HIDD_LE_BOOT_KB_INPUT_PERM
#define HIDD_LE_BOOT_KB_INPUT_PERM  (GATT_PERM_READ)
#endif

#ifndef HIDD_LE_BOOT_KB_INPUT_PROP
#define HIDD_LE_BOOT_KB_INPUT_PROP  (GATT_CHAR_PROP_BIT_READ|GATT_CHAR_PROP_BIT_NOTIFY)
#endif

/* by default include the optional write permission */
#ifndef HIDD_LE_BOOT_MI_INPUT_PERM
#define HIDD_LE_BOOT_MI_INPUT_PERM  (GATT_PERM_READ)
#endif

#ifndef HIDD_LE_BOOT_MI_INPUT_PROP
#define HIDD_LE_BOOT_MI_INPUT_PROP  (GATT_CHAR_PROP_BIT_READ|GATT_CHAR_PROP_BIT_NOTIFY)
#endif

/* HID information characteristic */
#define HIDD_LE_INFO_LEN 4
#define HIDD_LE_RPT_REF_LEN 2
#define HIDD_LE_CLT_CFG_LEN 2

/* boot mode report value */
#define HIDD_BOOT_KB_RPT_LEN        8
#define HIDD_BOOT_MICE_RPT_LEN    8
typedef UINT8 HIDD_BOOT_KB_RPT[HIDD_BOOT_KB_RPT_LEN];            /* boot mode Keyboard report */
typedef UINT8 HIDD_BOOT_MICE_RPT[HIDD_BOOT_MICE_RPT_LEN];        /* boot mode mouse report */

#define HIDD_LE_PROTO_MODE_LEN          1

/* report characteristic value */
typedef struct
{
    UINT16   len;
    UINT8    *p_data;
}tHIDD_LE_RPT;

struct elem
{
    int data;
    struct elem *p_next;
}Node;

#define HIDD_LE_CP_LEN          1
#define HIDD_LE_SUSPEND         0x00
#define HIDD_LE_EXIT_SUSPEND    0x01

/* HID service database attribute value: it can be characteristic or descriptor, or include service */
typedef union
{
    tHIDD_LE_RPT_REF        rpt_ref;
    UINT16                  clt_cfg;
}tHIDD_LE_ATTR_VALUE;

/* HID service data base attribute */
typedef struct
{
    BOOLEAN                 in_use;
    UINT16                  handle;
    UINT16                  uuid;
    tHIDD_LE_ATTR_VALUE     attr_value;
}tHIDD_LE_ATTR;


#define HIDD_LE_GATT_IGNORE     0
#define HIDD_LE_GATT_RSP        1
#define HIDD_LE_GATT_PENDING    2
typedef UINT8 tHIDD_LE_ACT;

/* LE HID service control block */
typedef struct
{
    tGATT_IF                gatt_if;
    tHIDD_DEV_TYPE          dev_type;
    tHIDD_LE_ATTR           hidd_attr[HID_LE_MAX_ATTR_NUM];
    UINT16                  conn_id;

    tHID_DEV_DSCP_INFO      rpt_map;
    UINT16                  bcdHID;
    UINT8                   contry_code;
    UINT8                   flags;
    UINT16                  pending_hdl;
    UINT8                   w4_evt;
    UINT32                  trans_id;

    BD_ADDR         host_addr;   /* BD-Addr of the host device */
    BOOLEAN         host_known;  /* Mode */
    UINT8           dev_state;   /* Device state if in HOST-KNOWN mode */


    tHIDD_LE_CBACK      *callback; /* Application callbacks */
    BOOLEAN             reg_flag;
    UINT8               trace_level;

}tHIDD_LE_CB;


/* HID LE Globals
*/
#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
** Main Control Block
*******************************************************************************/
#if HID_DYNAMIC_MEMORY == FALSE
HID_API extern tHIDD_LE_CB  hidd_le_cb;
#else
HID_API extern tHIDD_LE_CB *hidd_le_cb_ptr;
#define hidd_le_cb (*hidd_le_cb_ptr)
#endif

extern tHID_STATUS hidd_le_api_ack_handshake(UINT8 res_code);
extern tHID_STATUS hidd_le_get_proto_rsp(tHIDD_LE_PROTO proto_mode);
extern tHID_STATUS hidd_le_api_write_data(UINT8 rep_type, UINT8 rid, UINT16 data_len, UINT16 offset, UINT8 *p_data);
extern tHID_STATUS hidd_le_disconnect(void);
extern tHID_STATUS hidd_le_connect(void);
extern void hidd_ble_dereg(void);
extern tHID_STATUS hidd_le_attr_db_init(tHIDD_LE_DEV_INFO *p_dev_info);


#ifdef __cplusplus
}
#endif

#endif
