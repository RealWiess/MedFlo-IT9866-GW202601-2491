/****************************************************************************
**
**  Name:          btapp_gattc.c
**
**  Description:   Contains btapp functions for GATT client
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "string.h"
#include "bta_platform.h"
#include "bte_glue.h"
#include "buildcfg.h"
#include "btapp_nv.h"
#include "btapp_utility.h"



#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
#if (defined BTA_GATT_INCLUDED) && (BTA_GATT_INCLUDED == TRUE)
#include "bta_gatt_api.h"

#define BTAPP_GATTC_APP_UUID            0x8888
#define BTAPP_GATTC_SERVICE_SEARCH_MAX  8

/* GATT client app UUID */
static tBT_UUID    btapp_gattc_app_uuid = {2, {BTAPP_GATTC_APP_UUID}};

/* BTAPP FT client main control block */
tBTAPP_GATTC_CB btapp_gattc_cb;
/* device database */
tBTAPP_GATT_CACHE_DB btapp_gatt_cache_db;

static UINT16 btapp_gattc_service_uuid[BTAPP_GATTC_SERVICE_SEARCH_MAX] =
{
    UUID_SERVCLASS_GATT_SERVER,
    UUID_SERVCLASS_LINKLOSS,
    UUID_SERVCLASS_TX_POWER,
    UUID_SERVCLASS_IMMEDIATE_ALERT,
    UUID_SERVCLASS_GLUCOSE,
    UUID_SERVCLASS_RSC,
    UUID_SERVCLASS_CSC,
    UUID_SERVCLASS_CP
};

void btapp_gattc_cback(tBTA_GATTC_EVT event, tBTA_GATTC *p_data);

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_swrap_notify_gattc(tBTA_GATTC_EVT event, void *p_data)
{

}

/*******************************************************************************
**
** Function         btapp_gatt_is_gattc_type
**
** Description      check the target bd_addr if is the gatt client during recive the conn_cback event.
**
**
** Returns          TRUE: the target connected or disconnected device is gatt client device. FALSE: normal.
**
** Note             Because the gatt l2cap connection state change callback will notify all registerd conn_cb
**                  to indicate the device connection state changed. The same actions also do in the Andriod stack,
**                  so we need filter the connection state event in the related gatt server callback by device address
**                  to avoid make stack confused.
**
*******************************************************************************/
BOOLEAN btapp_gatt_is_gattc_type(BD_ADDR bd_addr)
{
    BOOLEAN ret = FALSE;
    UINT8 i;

    if(!bdcmp(btapp_gattc_cb.connecting_bda, bd_addr))
    {
        ret = TRUE;
    }

    for(i = 0; i < BTAPP_GATTC_MAX_CL; i ++)
    {
        if(btapp_gattc_cb.clcb[i].in_use && (!bdcmp(btapp_gattc_cb.clcb[i].bda, bd_addr)) )
        {
            ret = TRUE;
            break;
        }
    }

    return ret;
}

/*******************************************************************************
**
** Function         btapp_gattc_init
**
** Description      Initializes GATT Client application
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_init(void)
{
    if(!btapp_gattc_cb.registered)
    {
        memcpy(&btapp_gattc_cb.app_id, &btapp_gattc_app_uuid, sizeof(tBT_UUID));
        BTA_GATTC_AppRegister(&btapp_gattc_app_uuid, btapp_gattc_cback);
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_init has inited!!!"LINE_ENDING);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_deinit
**
** Description      deregister GATT Client application
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_deinit(void)
{
    if(btapp_gattc_cb.registered)
    {
        BTA_GATTC_AppDeregister(btapp_gattc_cb.client_if, btapp_gattc_cback);
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_deinit has deinited!!!"LINE_ENDING);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_open
**
** Description      Open a GATT connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_open(BD_ADDR remote_bda, BOOLEAN is_direct)
{
    if(bdcmp(remote_bda, bd_addr_null) && btapp_gattc_cb.client_if != 0)
    {
        memcpy(btapp_gattc_cb.connecting_bda, remote_bda, BD_ADDR_LEN);
        /* initiate a connection to a peer device */

        BRCM_PLATFORM_TRACE("btapp_gattc_open bda:%s"LINE_ENDING, btapp_utl_bda_to_str(remote_bda));

        BTA_GATTC_Open(btapp_gattc_cb.client_if, remote_bda, is_direct, BTA_TRANSPORT_LE, btapp_gattc_cback);
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_open can't open bda:%s, client_if:%d"LINE_ENDING,
                        btapp_utl_bda_to_str(remote_bda), btapp_gattc_cb.client_if);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_cancel_open
**
** Description      Open a GATT connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_cancel_open(tBTA_GATTC_IF cif, BD_ADDR remote_bda, BOOLEAN is_direct)
{
    BTA_GATTC_CancelOpen(cif, remote_bda, is_direct, btapp_gattc_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_close
**
** Description      Close a GATT connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_close( BD_ADDR remote_bda)
{
    tBTAPP_GATTCC_CLCB *p_clcb;

    if(bdcmp(remote_bda, bd_addr_null) && btapp_gattc_cb.client_if != 0)
    {
        if ((p_clcb = btapp_gattc_find_clcb(btapp_gattc_cb.client_if, remote_bda, FALSE))==NULL)
        {
            BRCM_PLATFORM_TRACE("Link already dropped addr:%s, cif:%d"LINE_ENDING,
                            btapp_utl_bda_to_str(remote_bda), btapp_gattc_cb.client_if);
            return;
        }
        /* remove device from background connection if it was in the list */
        BTA_GATTC_CancelOpen(btapp_gattc_cb.client_if,
                            remote_bda,
                            FALSE,
                            btapp_gattc_cback);

        /* disconnect */
        BTA_GATTC_Close(p_clcb->conn_id, btapp_gattc_cback);

    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_close can't close bda:%s, client_if:%d"LINE_ENDING,
                        btapp_utl_bda_to_str(remote_bda), btapp_gattc_cb.client_if);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_cfg_mtu
**
** Description      configure MTU on a GATT connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_cfg_mtu(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 mtu)
{
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, FALSE))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    BTA_GATTC_ConfigureMTU(p_clcb->conn_id, mtu, btapp_gattc_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_read_char
**
** Description      Read a characteristic value.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_read_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                           UINT16 char_uuid, UINT8 char_inst, UINT16 descr, UINT8 auth_req,
                           BOOLEAN is_primary, UINT8 transport)
{
    tBTA_GATTC_CHAR_ID          char_id;
    tBTA_GATTC_CHAR_DESCR_ID    descr_id;
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }

    if (descr == 0)
    {
        char_id.srvc_id.id.uuid.len = LEN_UUID_16;
        char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        char_id.srvc_id.id.inst_id = srvc_inst;
        char_id.srvc_id.is_primary = is_primary;

        char_id.char_id.uuid.len = LEN_UUID_16;
        char_id.char_id.uuid.uu.uuid16 = char_uuid;
        char_id.char_id.inst_id = char_inst;
        BTA_GATTC_ReadCharacteristic(p_clcb->conn_id, &char_id, auth_req, btapp_gattc_cback);
    }
    else
    {
        descr_id.char_id.srvc_id.id.uuid.len = LEN_UUID_16;
        descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        descr_id.char_id.srvc_id.id.inst_id = srvc_inst;
        descr_id.char_id.srvc_id.is_primary = is_primary;

        descr_id.char_id.char_id.uuid.len = LEN_UUID_16;
        descr_id.char_id.char_id.uuid.uu.uuid16 = char_uuid;
        descr_id.char_id.char_id.inst_id = char_inst;

        descr_id.descr_id.inst_id   = 0;
        descr_id.descr_id.uuid.len = 2;
        descr_id.descr_id.uuid.uu.uuid16 = descr;
        BTA_GATTC_ReadCharDescr(p_clcb->conn_id, &descr_id, auth_req, btapp_gattc_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_write_char
**
** Description      write a characteristic value.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_write_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                            UINT16 char_uuid, UINT8 char_inst, UINT8 write_type,
                            UINT16 len, UINT8* p_value, UINT8 auth_req, BOOLEAN is_primary, UINT8 transport)
{
    tBTA_GATTC_CHAR_ID          char_id;
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }

    char_id.srvc_id.id.uuid.len = LEN_UUID_16;
    char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
    char_id.srvc_id.id.inst_id = srvc_inst;
    char_id.srvc_id.is_primary = is_primary;

    char_id.char_id.uuid.len = LEN_UUID_16;
    char_id.char_id.uuid.uu.uuid16 = char_uuid;
    char_id.char_id.inst_id = char_inst;

    BTA_GATTC_WriteCharValue (p_clcb->conn_id, &char_id, write_type, len, p_value, auth_req, btapp_gattc_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_prep_write_char
**
** Description      prepare write a characteristic value.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_prep_write_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid,
                                 UINT8 srvc_inst, UINT16 char_uuid, UINT8 char_inst, UINT16 offset,
                                 UINT16 len, UINT8* p_value, UINT8 auth_req, BOOLEAN is_primary,
                                 UINT8 transport)

{
    tBTA_GATTC_CHAR_ID          char_id;
    tBTAPP_GATTCC_CLCB            *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    char_id.srvc_id.id.uuid.len = LEN_UUID_16;
    char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
    char_id.srvc_id.id.inst_id = srvc_inst;
    char_id.srvc_id.is_primary = is_primary;

    char_id.char_id.uuid.len = LEN_UUID_16;
    char_id.char_id.uuid.uu.uuid16 = char_uuid;
    char_id.char_id.inst_id = char_inst;
    BTA_GATTC_PrepareWrite (p_clcb->conn_id, &char_id, offset, len, p_value, auth_req, btapp_gattc_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_exec_write_char
**
** Description      execute write a characteristic value.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_exec_write_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, BOOLEAN is_execute, UINT8 transport)

{
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    BTA_GATTC_ExecuteWrite(p_clcb->conn_id, is_execute, btapp_gattc_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_write_descr
**
** Description      write a characteristic descriptor.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_write_descr(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                             UINT16 char_uuid, UINT8 char_inst, UINT16 descr,
                             UINT8 write_type, tBTA_GATT_UNFMT *p_value,
                             UINT8 auth_req, BOOLEAN is_primary, UINT8 transport)
{
    tBTA_GATTC_CHAR_DESCR_ID    descr_id;
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }

    descr_id.char_id.srvc_id.id.uuid.len = LEN_UUID_16;
    descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
    descr_id.char_id.srvc_id.id.inst_id = srvc_inst;
    descr_id.char_id.srvc_id.is_primary = is_primary;

    descr_id.char_id.char_id.uuid.len = LEN_UUID_16;
    descr_id.char_id.char_id.uuid.uu.uuid16 = char_uuid;
    descr_id.char_id.char_id.inst_id = char_inst;

    descr_id.descr_id.inst_id   = 0;
    descr_id.descr_id.uuid.len = 2;
    descr_id.descr_id.uuid.uu.uuid16 = descr;

    BTA_GATTC_WriteCharDescr (p_clcb->conn_id, &descr_id, write_type, p_value, auth_req, btapp_gattc_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_send_handle_value_confirm
**
** Description      send handle value confirmation
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_register_4_notif(tBTA_GATTC_IF cif, BD_ADDR bda, UINT16 srvc_uuid, UINT8 srvc_inst_id,
                                  UINT16 char_uuid, UINT8 char_inst_id, BOOLEAN is_primary)
{
    tBTA_GATTC_CHAR_ID    char_id = {0};

    if (srvc_uuid != 0 && char_uuid != 0)
    {
        memset(&char_id, 0, sizeof(tBTA_GATTC_CHAR_ID));

        char_id.srvc_id.id.uuid.len = LEN_UUID_16;
        char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        char_id.srvc_id.id.inst_id = srvc_inst_id;
        char_id.srvc_id.is_primary = is_primary;

        char_id.char_id.uuid.len = LEN_UUID_16;
        char_id.char_id.uuid.uu.uuid16 = char_uuid;
        char_id.char_id.inst_id = char_inst_id;

        if (BTA_GATTC_RegisterForNotifications (cif, bda, &char_id))
        {
            BRCM_PLATFORM_TRACE("registration failed"LINE_ENDING);
        }
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_deregister_4_notif
**
** Description      send handle value confirmation
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_deregister_4_notif(tBTA_GATTC_IF cif, BD_ADDR bda, UINT16 srvc_uuid,
                                    UINT8 srvc_inst_id, UINT16 char_uuid, UINT8 char_inst_id,
                                    BOOLEAN is_primary)
{
    tBTA_GATTC_CHAR_ID    char_id = {0};

    if (srvc_uuid != 0 && char_uuid != 0)
    {
        memset(&char_id, 0, sizeof(tBTA_GATTC_CHAR_ID));

        char_id.srvc_id.id.uuid.len = LEN_UUID_16;
        char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        char_id.srvc_id.id.inst_id = srvc_inst_id;
        char_id.srvc_id.is_primary = is_primary;

        char_id.char_id.uuid.len = LEN_UUID_16;
        char_id.char_id.uuid.uu.uuid16 = char_uuid;
        char_id.char_id.inst_id = char_inst_id;

        if (BTA_GATTC_DeregisterForNotifications (cif, bda, &char_id))
        {
            BRCM_PLATFORM_TRACE("Deregistration failed"LINE_ENDING);
        }
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_deregister_4_notif failed: srvc_uuid = 0x%04x, char_uuid = 0x%04x"LINE_ENDING,
                       srvc_uuid, char_uuid);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_send_handle_value_confirm
**
** Description      send handle value confirmation
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_send_handle_value_confirm(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT8 transport)
{
    tBTAPP_GATTCC_CLCB *p_clcb;

    BRCM_PLATFORM_TRACE("btapp_gattc_send_handle_value_confirm"LINE_ENDING);
    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    BTA_GATTC_SendIndConfirm (p_clcb->conn_id, &btapp_gattc_cb.indicate_char_id, btapp_gattc_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_discover
**
** Description      discover a service on a server.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_discover(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 transport)
{
    tBT_UUID uuid;
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    if (srvc_uuid != 0)
    {
        uuid.len = 2;
        uuid.uu.uuid16 = srvc_uuid;
        BTA_GATTC_ServiceSearchRequest(p_clcb->conn_id, &uuid, btapp_gattc_cback);
    }
    else
    {
        BTA_GATTC_ServiceSearchRequest(p_clcb->conn_id, NULL, btapp_gattc_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_print_uuid128_to_string
**
** Description      print uuid128 as string format.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_print_uuid128_to_string(tBT_UUID uuid) {

    UINT8   i = 1, x = 0;
    UINT8   p_buf[40] = {0};

    x += sprintf((char*)&p_buf[x], "0x");
    if (uuid.len == 2)
    {
         sprintf((char*)&p_buf[x], "%04x", uuid.uu.uuid16);
    }
    if (uuid.len == 16)
    {
        while (i <= 16)
        {
            x += sprintf((char*)&p_buf[x], "%02x", uuid.uu.uuid128[16-i]);
            i ++;
        }
    }

    BRCM_PLATFORM_TRACE("      UUID = %s "LINE_ENDING,p_buf);
}

/*******************************************************************************
**
** Function         btapp_gattc_find_first_char
**
** Description      find first characteristic in the service.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_find_first_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda,
                                 UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 char_uuid,
                                 BOOLEAN is_primary, UINT8 transport)
{
    tBTA_GATT_SRVC_ID   srvc_id;
    tBTA_GATTC_CHAR_ID  char_result;
    tBT_UUID            *p_uuid = NULL;
    tBT_UUID            uuid;
    UINT8               prop;
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    srvc_id.id.inst_id = srvc_inst;
    srvc_id.id.uuid.len = 2;
    srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
    srvc_id.is_primary = is_primary;

    if (char_uuid != 0)
    {
        uuid.len = 2;
        uuid.uu.uuid16 = char_uuid;
        p_uuid = &uuid;
    }

    if (BTA_GATTC_GetFirstChar (p_clcb->conn_id, &srvc_id, p_uuid, &char_result, &prop) != BTA_GATT_OK)
    {
        BRCM_PLATFORM_TRACE("Read first characteristic failed"LINE_ENDING);
    }
    else
    {
        BRCM_PLATFORM_TRACE("First Char uuid [0x%04x] instance [%d]"LINE_ENDING,
                        char_result.char_id.uuid.uu.uuid16, char_result.char_id.inst_id);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_find_next_char
**
** Description      find next characteristic in the service.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_find_next_char(tBTA_GATTC_IF cif, BD_ADDR remote_bda, UINT16 srvc_uuid, UINT8 srvc_inst,
                                UINT16 start_uuid, UINT8 start_inst, UINT16 char_uuid,
                                BOOLEAN is_primary, UINT8 transport)
{
    tBT_UUID        *p_uuid = NULL;
    tBT_UUID        uuid;
    tBTA_GATTC_CHAR_ID  start_char, char_result;
    UINT8               prop;
    tBTAPP_GATTCC_CLCB *p_clcb;

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }

    start_char.srvc_id.id.inst_id = srvc_inst;
    start_char.srvc_id.id.uuid.len = 2;
    start_char.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
    start_char.srvc_id.is_primary = is_primary;

    start_char.char_id.inst_id = start_inst;
    start_char.char_id.uuid.len = 2;
    start_char.char_id.uuid.uu.uuid16 = start_uuid;


    if (char_uuid != 0)
    {
        uuid.len = 2;
        uuid.uu.uuid16 = char_uuid;
        p_uuid = &uuid;
    }

    if (BTA_GATTC_GetNextChar (p_clcb->conn_id, &start_char, p_uuid, &char_result, &prop) != BTA_GATT_OK)
    {
        BRCM_PLATFORM_TRACE("Read first characteristic failed"LINE_ENDING);
    }
    else
    {
        BRCM_PLATFORM_TRACE("Next Char uuid [0x%04x] instance [%d]"LINE_ENDING,
                          char_result.char_id.uuid.uu.uuid16, char_result.char_id.inst_id);
    }

}

/*******************************************************************************
**
** Function         btapp_gattc_find_first_descr
**
** Description      find first characteristic in the service.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_find_first_descr(tBTA_GATTC_IF cif, BD_ADDR remote_bda,
                                 UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 char_uuid,
                                 UINT8 char_inst, UINT16 descr_cond, BOOLEAN is_primary, UINT8 transport)
{
    tBTA_GATTC_CHAR_ID          char_id;
    tBTA_GATTC_CHAR_DESCR_ID    descr_result;
    tBT_UUID                    descr_uuid;
    tBTAPP_GATTCC_CLCB *p_clcb;
    tBTAPP_BLE_GATTC_DESCR_FIND        ble_descr_find;

    memset(&ble_descr_find, 0, sizeof(tBTAPP_BLE_GATTC_DESCR_FIND));

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        ble_descr_find.status = BTA_GATT_ERROR;
    }
    else
    {
        char_id.srvc_id.is_primary = is_primary;
        char_id.srvc_id.id.uuid.len = 2;
        char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        char_id.srvc_id.id.inst_id = srvc_inst;

        char_id.char_id.uuid.len = 2;
        char_id.char_id.uuid.uu.uuid16 = char_uuid;
        char_id.char_id.inst_id = char_inst;

        memset(&descr_uuid, 0, sizeof(tBT_UUID));
        if (descr_cond != 0)
        {
            descr_uuid.len = 2;
            descr_uuid.uu.uuid16 = descr_cond;
        }

        if ((ble_descr_find.status = BTA_GATTC_GetFirstCharDescr(p_clcb->conn_id,
                                                               &char_id,
                                                               &descr_uuid,
                                                               &descr_result))
                                  == BTA_GATT_OK)
        {
            memcpy(&ble_descr_find.char_id, &descr_result.char_id, sizeof(tBTA_GATTC_CHAR_ID));
            memcpy(&ble_descr_find.descr_uuid, &descr_result.descr_id.uuid, sizeof(tBT_UUID));
            BRCM_PLATFORM_TRACE("Find first descriptor UUID: 0x%04x"LINE_ENDING, descr_result.descr_id.uuid.uu.uuid16);
        }
        else
        {
            BRCM_PLATFORM_TRACE("No descriptor found"LINE_ENDING);
        }
    }

    btapp_swrap_notify_gattc(BTAPP_GATTC_FIND_FIRST_DESCR_EVT, &ble_descr_find);
}

/*******************************************************************************
**
** Function         btapp_gattc_find_next_descr
**
** Description      find next characteristic descriptor in the characteristic.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_find_next_descr(tBTA_GATTC_IF cif, BD_ADDR remote_bda,
                                 UINT16 srvc_uuid, UINT8 srvc_inst, UINT16 char_uuid,
                                 UINT8 char_inst, UINT16 start_descr_uuid,
                                 UINT16 descr_cond, UINT8 descr_inst, BOOLEAN is_primary, UINT8 transport)
{
    tBTA_GATTC_CHAR_DESCR_ID    start_descr_id;
    tBTA_GATTC_CHAR_DESCR_ID    descr_result;
    tBT_UUID                    descr_uuid;
    tBTAPP_GATTCC_CLCB             *p_clcb;
    tBTAPP_BLE_GATTC_DESCR_FIND        ble_descr_find;

    memset(&ble_descr_find, 0, sizeof(tBTAPP_BLE_GATTC_DESCR_FIND));
    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        ble_descr_find.status = BTA_GATT_ERROR;
    }
    else
    {
        memset (&start_descr_id, 0, sizeof(tBTA_GATTC_CHAR_DESCR_ID));
        memset (&descr_result, 0, sizeof(tBTA_GATTC_CHAR_DESCR_ID));

        start_descr_id.char_id.srvc_id.is_primary = is_primary;
        start_descr_id.char_id.srvc_id.id.uuid.len = 2;
        start_descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        start_descr_id.char_id.srvc_id.id.inst_id = srvc_inst;

        start_descr_id.char_id.char_id.uuid.len = 2;
        start_descr_id.char_id.char_id.uuid.uu.uuid16 = char_uuid;
        start_descr_id.char_id.char_id.inst_id = char_inst;

        if(start_descr_uuid != 0)
        {
            start_descr_id.descr_id.inst_id = descr_inst;
            start_descr_id.descr_id.uuid.len = 2;
            start_descr_id.descr_id.uuid.uu.uuid16 = start_descr_uuid;
        }

        memset(&descr_uuid, 0, sizeof(tBT_UUID));

        if (descr_cond != 0)
        {
            descr_uuid.len = 2;
            descr_uuid.uu.uuid16 = descr_cond;
        }

        if ((ble_descr_find.status = BTA_GATTC_GetNextCharDescr(p_clcb->conn_id,
                                                        &start_descr_id,
                                                        &descr_uuid,
                                                        &descr_result)) == BTA_GATT_OK)
        {
            memcpy(&ble_descr_find.char_id, &descr_result.char_id, sizeof(tBTA_GATTC_CHAR_ID));
            memcpy(&ble_descr_find.descr_uuid, &descr_result.descr_id.uuid, sizeof(tBT_UUID));

            BRCM_PLATFORM_TRACE("Find next descriptor UUID: 0x%04x"LINE_ENDING,
                            descr_result.descr_id.uuid.uu.uuid16);
        }
        else
        {
            BRCM_PLATFORM_TRACE("No descriptor found"LINE_ENDING);
        }
    }

    btapp_swrap_notify_gattc(BTAPP_GATTC_FIND_NEXT_DESCR_EVT, &ble_descr_find);
}

/*******************************************************************************
**
** Function         btapp_gattc_find_first_incl_srvc
**
** Description      find first included service in the service.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_find_first_incl_srvc(tBTA_GATTC_IF cif, BD_ADDR remote_bda,
                                      UINT16 srvc_uuid, UINT8 srvc_inst,
                                      UINT16 incl_srvc_uuid, BOOLEAN is_primary, UINT8 transport)
{
    tBTA_GATT_SRVC_ID  srvc_id;
    tBT_UUID            *p_uuid = NULL;
    tBT_UUID            uuid;
    tBTA_GATTC_INCL_SVC_ID  result;
    tBTAPP_GATTCC_CLCB *p_clcb;
    tBTAPP_BLE_GATTC_FIND    ble_find;

    memset(&ble_find, 0, sizeof(tBTAPP_BLE_GATTC_FIND));

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        ble_find.status = BTA_GATT_ERROR;
    }
    else
    {

        srvc_id.id.inst_id = srvc_inst;
        srvc_id.id.uuid.len = 2;
        srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        srvc_id.is_primary = is_primary;

        if (incl_srvc_uuid != 0)
        {
            uuid.len = 2;
            uuid.uu.uuid16 = incl_srvc_uuid;
            p_uuid = &uuid;
        }

        if ((ble_find.status = BTA_GATTC_GetFirstIncludedService (p_clcb->conn_id,
                                                                  &srvc_id,
                                                                  p_uuid,
                                                                  &result)) != BTA_GATT_OK)
        {
            BRCM_PLATFORM_TRACE("Read first included service failed"LINE_ENDING);
        }
        else
        {
            memcpy(&ble_find.srvc_id, &result.srvc_id.id, sizeof(tBTA_GATT_SRVC_ID));
            memcpy(&ble_find.find_id, &result.incl_svc_id.id, sizeof(tBTA_GATT_SRVC_ID));

            BRCM_PLATFORM_TRACE("First Included Service uuid [0x%04x] instance [%d]"LINE_ENDING,
                               result.incl_svc_id.id.uuid.uu.uuid16, result.incl_svc_id.id.inst_id);
        }
    }

    btapp_swrap_notify_gattc(BTAPP_GATTC_FIND_FIRST_INCL_SRVC_EVT, &ble_find);
}

/*******************************************************************************
**
** Function         btapp_gattc_find_next_incl_srvc
**
** Description      find next included service in the service.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_find_next_incl_srvc(tBTA_GATTC_IF cif, BD_ADDR remote_bda,
                                     UINT16 srvc_uuid, UINT8 srvc_inst, BOOLEAN is_primary,
                                     UINT16 start_uuid, UINT8 start_inst, UINT16 incl_srvc_uuid, UINT8 transport)
{
    tBT_UUID    *p_uuid = NULL;
    tBT_UUID    uuid;
    tBTA_GATTC_INCL_SVC_ID  start_id, result;
    tBTAPP_GATTCC_CLCB *p_clcb;
    tBTAPP_BLE_GATTC_FIND    ble_find;

    memset(&ble_find, 0, sizeof(tBTAPP_BLE_GATTC_FIND));

    if ((p_clcb = btapp_gattc_find_clcb(cif, remote_bda, transport))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        ble_find.status = BTA_GATT_ERROR;
    }
    else
    {
        start_id.srvc_id.id.inst_id = srvc_inst;
        start_id.srvc_id.id.uuid.len = 2;
        start_id.srvc_id.id.uuid.uu.uuid16 = srvc_uuid;
        start_id.srvc_id.is_primary = is_primary;

        start_id.incl_svc_id.id.inst_id = start_inst;
        start_id.incl_svc_id.id.uuid.len = 2;
        start_id.incl_svc_id.id.uuid.uu.uuid16 = start_uuid;


        if (incl_srvc_uuid != 0)
        {
            uuid.len = 2;
            uuid.uu.uuid16 = incl_srvc_uuid;
            p_uuid = &uuid;
        }

        if ((ble_find.status = BTA_GATTC_GetNextIncludedService (p_clcb->conn_id, &start_id, p_uuid, &result))
                            != BTA_GATT_OK)
        {
            BRCM_PLATFORM_TRACE("Read Next Inlcuded Service failed"LINE_ENDING);
        }
        else
        {
            memcpy(&ble_find.srvc_id, &result.srvc_id, sizeof(tBTA_GATT_SRVC_ID));
            memcpy(&ble_find.find_id, &result.incl_svc_id, sizeof(tBTA_GATT_SRVC_ID));

            BRCM_PLATFORM_TRACE("Next Inlcuded Service uuid [0x%04x] instance [%d]"LINE_ENDING,
                              result.incl_svc_id.id.uuid.uu.uuid16, result.incl_svc_id.id.inst_id);
        }
    }

    btapp_swrap_notify_gattc(BTAPP_GATTC_FIND_NEXT_INCL_SRVC_EVT, &ble_find);
}

/*******************************************************************************
**
** Function         btapp_gattc_find_clcb_by_conn
**
** Description      The function searches all LCB with macthing bd address
**
** Returns          total number of clcb found.
**
*******************************************************************************/
tBTAPP_GATTCC_CLCB *btapp_gattc_find_clcb_by_conn(tBTA_GATTC_IF client_if, UINT16 conn_id, UINT8 use_br_edr)
{
    UINT8 i_clcb;
    tBTAPP_GATTCC_CLCB    *p_clcb = NULL;
    tBTA_TRANSPORT  transport = BTA_TRANSPORT_LE;

    if (use_br_edr)
        transport = BTA_TRANSPORT_BR_EDR;

    for (i_clcb = 0, p_clcb= btapp_gattc_cb.clcb; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected &&
            (p_clcb->client_if == client_if) && p_clcb->conn_id == conn_id &&
            p_clcb->transport == transport)
        {
            BRCM_PLATFORM_TRACE("btapp_gattc_find_clcb cif=%d conn_id=%d"LINE_ENDING, p_clcb->client_if, p_clcb->conn_id);
            return p_clcb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         btapp_gattc_find_clcb_by_bd_addr
**
** Description      The function searches all LCB with macthing bd address
**
** Returns          total number of clcb found.
**
*******************************************************************************/
tBTAPP_GATTCC_CLCB *btapp_gattc_find_clcb(tBTA_GATTC_IF client_if, BD_ADDR bda, UINT8 use_br_edr)
{
    UINT8 i_clcb;
    tBTAPP_GATTCC_CLCB    *p_clcb = NULL;
    tBTA_TRANSPORT  transport = BTA_TRANSPORT_LE;

    if (use_br_edr)
        transport = BTA_TRANSPORT_BR_EDR;

    for (i_clcb = 0, p_clcb= btapp_gattc_cb.clcb; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected &&
            (p_clcb->client_if == client_if) && !memcmp(p_clcb->bda, bda, BD_ADDR_LEN) &&
            p_clcb->transport == transport)
        {
            BRCM_PLATFORM_TRACE("btapp_gattc_find_clcb cif=%d conn_id=%d"LINE_ENDING, p_clcb->client_if, p_clcb->conn_id);
            return p_clcb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         btapp_gattc_clcb_alloc
**
** Description      The function allocates a   connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
tBTAPP_GATTCC_CLCB *btapp_gattc_clcb_alloc (UINT16 conn_id,tBTA_GATTC_IF client_if,  BD_ADDR bda,
                                           tBTA_TRANSPORT transport)
{
    UINT8         i_clcb = 0;
    tBTAPP_GATTCC_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btapp_gattc_cb.clcb; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (!p_clcb->in_use)
        {
            p_clcb->in_use      = TRUE;
            p_clcb->conn_id     = conn_id;
            p_clcb->connected   = TRUE;
            p_clcb->client_if   = client_if;
            p_clcb->transport   = transport;
            memcpy (p_clcb->bda, bda, BD_ADDR_LEN);
            return p_clcb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         btapp_gattc_clcb_dealloc
**
** Description      The function deallocates a connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
BOOLEAN btapp_gattc_clcb_dealloc (UINT16 conn_id)
{
    UINT8         i_clcb = 0;
    tBTAPP_GATTCC_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btapp_gattc_cb.clcb; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && p_clcb->conn_id == conn_id )
        {
            memset(p_clcb, 0, sizeof(tBTAPP_GATTCC_CLCB));
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
**
** Function         btapp_gattc_enc_cbacks
**
** Description      The function is encryption complete callback.
**
** Parameters       bd_addr: peer device address
**                  transport: BT transport type
**                  result: status result
**
** Returns          NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
void btapp_gattc_enc_cbacks (BD_ADDR bd_addr, tBTA_TRANSPORT transport, tBTA_STATUS result)
{
    if (result != 0)
    {
        BRCM_PLATFORM_TRACE("encryption failed."LINE_ENDING);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_cback
**
** Description      GATTC UI Callback function.  Handles all GATTC events.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_cback(tBTA_GATTC_EVT event, tBTA_GATTC *p_data)
{
    tBTAPP_REM_DEVICE * p_device_record;

    BRCM_PLATFORM_TRACE("btapp_gattc_cback event:%d"LINE_ENDING, event);

    switch (event)
    {
        case BTA_GATTC_REG_EVT:
            BRCM_PLATFORM_TRACE("Rcv BTA_GATTC_REG_EVT cif = %d status=%d"LINE_ENDING, p_data->reg_oper.client_if,
                              p_data->reg_oper.status );
            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                btapp_gattc_cb.registered = TRUE;
            }
            btapp_gattc_cb.client_if = p_data->reg_oper.client_if;
            break;

        case BTA_GATTC_DEREG_EVT:
            BRCM_PLATFORM_TRACE("Rcv BTA_GATTC_DEREG_EVT cif = %d status=%d"LINE_ENDING,
                            p_data->reg_oper.client_if,
                            p_data->reg_oper.status );
            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                btapp_gattc_cb.registered = FALSE;
            }
            btapp_gattc_cb.client_if = 0;
            break;

        case BTA_GATTC_OPEN_EVT:
            BRCM_PLATFORM_TRACE("btapp GATTC connection opened, ID: %d"LINE_ENDING, p_data->open.conn_id);
            p_device_record = btapp_get_device_record(p_data->open.remote_bda);
            if (p_device_record &&
                (p_device_record->link_key_present || ((p_device_record->key_mask & BTA_LE_KEY_PENC) != 0)))
            {
                BTA_DmSetEncryption(p_data->open.remote_bda, p_data->open.transport, btapp_gattc_enc_cbacks, BTA_DM_BLE_SEC_ENCRYPT);
            }

            if (p_data->open.status == 0)
            {
                BRCM_PLATFORM_TRACE ("btapp_gatts_cback: open with MTU size:%d"LINE_ENDING, p_data->open.mtu);
                if (btapp_gattc_clcb_alloc(p_data->open.conn_id, p_data->open.client_if,  p_data->open.remote_bda, p_data->open.transport)== NULL)
                {
                    BRCM_PLATFORM_TRACE ("btapp_gatts_cback: no_resource"LINE_ENDING);
                    return;
                }
            }
            else
            {
                BRCM_PLATFORM_TRACE ("BTA_GATTC_OPEN_EVT failure"LINE_ENDING);
            }
            break;

        case BTA_GATTC_CLOSE_EVT:
            BRCM_PLATFORM_TRACE("btapp GATTC connection close, BDA: %s "LINE_ENDING,
                           btapp_utl_bda_to_str(p_data->close.remote_bda));
            btapp_gattc_clcb_dealloc( p_data->close.conn_id);
            break;

        case BTA_GATTC_READ_CHAR_EVT:
            BRCM_PLATFORM_TRACE("btapp GATTC read complete: status = 0x%02x"LINE_ENDING, p_data->read.status);
            if (p_data->read.status == 0)
            {
                BRCM_PLATFORM_TRACE("data len = %d"LINE_ENDING,  p_data->read.p_value->unformat.len);
            }
            break;

        case BTA_GATTC_SEARCH_RES_EVT:
            BRCM_PLATFORM_TRACE("btapp Disocvery Result: uuid = [0x%04x] inst_id = %d is_primaray= %d"LINE_ENDING,
                              p_data->srvc_res.service_uuid.id.uuid.uu.uuid16,
                              p_data->srvc_res.service_uuid.id.inst_id,
                              p_data->srvc_res.service_uuid.is_primary);
            break;

        case BTA_GATTC_SEARCH_CMPL_EVT:
            BRCM_PLATFORM_TRACE("btapp GATTC Discover Completed. status = %d"LINE_ENDING, p_data->search_cmpl.status);
            break;

        case BTA_GATTC_NOTIF_EVT:
            BRCM_PLATFORM_TRACE("%s recveived: service uuid: 0x%04x, char uuid : 0x%04x"LINE_ENDING,
                              (p_data->notify.is_notify) ? "Notif" :"Indiaction",
                              p_data->notify.char_id.srvc_id.id.uuid.uu.uuid16,
                              p_data->notify.char_id.char_id.uuid.uu.uuid16);

            memcpy(&btapp_gattc_cb.indicate_char_id, &p_data->notify.char_id, sizeof(tBTA_GATTC_CHAR_ID));

            break;

        case BTA_GATTC_PREP_WRITE_EVT:
            BRCM_PLATFORM_TRACE("btapp GATTC Prepare write completed. status = %d"LINE_ENDING, p_data->write.status);
            break;

        case BTA_GATTC_EXEC_EVT:
            BRCM_PLATFORM_TRACE("btapp GATTC Execute write completed. status = %d"LINE_ENDING, p_data->exec_cmpl.status);
            break;

        case BTA_GATTC_SRVC_CHG_EVT:
            BRCM_PLATFORM_TRACE("receive service change notification"LINE_ENDING);
            break;

        case BTA_GATTC_CFG_MTU_EVT:
            BRCM_PLATFORM_TRACE("receive BTA_GATTC_CFG_MTU_EVT event"LINE_ENDING);
            break;

        default:
            break;
    }
}

/*******************************************************************************
**
**           GATTC server cache NV management functions
***
*******************************************************************************/
/*******************************************************************************
**
** Function         btapp_gattc_reset_server_cache
**
** Description      remove the old cache value for the designated server.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_reset_server_cache(UINT8 idx)
{
    UINT8   i = 0, start_idx = 0;

    while (i < BTAPP_GATT_MAX_ATTR)
    {
        if (btapp_gatt_cache_db.attr_cache[i].in_use &&
            btapp_gatt_cache_db.attr_cache[i].device_idx == idx)
        {
            memset(&btapp_gatt_cache_db.attr_cache[i], 0, sizeof(tBTAPP_GATT_CACHE));

            if (start_idx == 0)
                start_idx = i + 1;
        }
        else
        {
            if (start_idx != 0 || !btapp_gatt_cache_db.attr_cache[i].in_use)
            {
                break;
            }
        }
        i ++;
    }
    /* move all attr forward */
    while (i < BTAPP_GATT_MAX_ATTR)
    {
        if (btapp_gatt_cache_db.attr_cache[i].in_use)
        {
            memcpy(&btapp_gatt_cache_db.attr_cache[start_idx - 1],
                   &btapp_gatt_cache_db.attr_cache[i],
                   sizeof(tBTAPP_GATT_CACHE));
            i ++;
            start_idx ++;
        }
        else
            break;

    }
    btapp_nv_store_gattc_db();
}

/*******************************************************************************
**
** Function         btapp_gattc_proc_cache_open
**
** Description      process cache open
**
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_gattc_proc_cache_open(BD_ADDR addr, BOOLEAN to_save)
{
    UINT8               idx = btapp_get_device_record_idx(addr);
    tBTAPP_REM_DEVICE    *p_device_record;

    if (idx == 0xff)
    {
        /* if a non-bond device, do not do anything */
        if ((p_device_record = btapp_get_device_record(addr)) == NULL)
            return TRUE;

        idx = btapp_get_device_record_idx(addr);
    }

    if (to_save)
    {
        btapp_gattc_reset_server_cache(idx);
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         btapp_gattc_proc_cache_save
**
** Description      process cache save callout
**
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_gattc_proc_cache_save(BD_ADDR addr, UINT16 num_attr, tBTA_GATTC_NV_ATTR *p_attr_list,
                                    UINT16 attr_index)
{
    UINT8   i = 0, j = 0;
    UINT8   idx = btapp_get_device_record_idx(addr);

    /* if a non-bond device, do not save */
    if (idx == BTAPP_NUM_REM_DEVICE)
        return TRUE;

    /* always assume cache is attaching to the end of the NV memory which is not been used */
    while (btapp_gatt_cache_db.attr_cache[i].in_use)
        i ++;

    for (; j < num_attr && i < BTAPP_GATT_MAX_ATTR ; j ++, i ++, p_attr_list ++)
    {
        btapp_gatt_cache_db.attr_cache[i].in_use = TRUE;
        btapp_gatt_cache_db.attr_cache[i].device_idx = idx;
        memcpy(&btapp_gatt_cache_db.attr_cache[i].attr, p_attr_list, sizeof(tBTA_GATTC_NV_ATTR));
    }

    btapp_nv_store_gattc_db();

    if (j != num_attr)
    {
        BRCM_PLATFORM_TRACE("No resources to save cache"LINE_ENDING);
        return FALSE;
    }
    else
        return TRUE;
}

/*******************************************************************************
**
** Function         btapp_gattc_proc_cache_load
**
** Description      process cache load callout
**
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_gattc_proc_cache_load(BD_ADDR server_bda, UINT16 max_attr,
                                    UINT16 *p_num_attr, tBTA_GATTC_NV_ATTR *p_attr, UINT16 start_idx)
{
    UINT8   idx = btapp_get_device_record_idx(server_bda), i;
    UINT16  num_attr = 0, offset = 0 ;
    tBTA_GATTC_NV_ATTR  *p_cur = p_attr;

    *p_num_attr = 0;

    if (idx == 0xff)
    {
        return FALSE;
    }

    for (i = 0; i < BTAPP_GATT_MAX_ATTR && num_attr < max_attr; i ++)
    {
        if (!btapp_gatt_cache_db.attr_cache[i].in_use)
            break;

        if (btapp_gatt_cache_db.attr_cache[i].device_idx == idx)
        {
            if (offset >= start_idx )
            {
                num_attr ++;
                memcpy(p_cur, &btapp_gatt_cache_db.attr_cache[i].attr, sizeof(tBTA_GATTC_NV_ATTR));
                p_cur ++;
            }
            offset ++;
        }
    }
    *p_num_attr = num_attr;

    return TRUE;

}

/*******************************************************************************
**
** Function         btapp_gattc_proc_cache_load
**
** Description      process cache load callout
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_clear_nv_cache(BD_ADDR server_bda)
{
    UINT8   idx = btapp_get_device_record_idx(server_bda), i;

    if (idx == 0xff)
    {
        return;
    }

    for (i = 0; i < BTAPP_GATT_MAX_ATTR; i ++)
    {
        if (btapp_gatt_cache_db.attr_cache[i].in_use &&
            btapp_gatt_cache_db.attr_cache[i].device_idx == idx)
        {
            memset(&btapp_gatt_cache_db.attr_cache[i], 0, sizeof(tBTAPP_GATT_CACHE));
            return;
        }
    }

}
#endif
#endif

