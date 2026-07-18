/****************************************************************************
**
**  Name:          btapp_gattc_profile.c
**
**  Description:   Contains btapp functions for GATT client profile.
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"
#include "btapp_nv.h"
#include "btapp_utility.h"


#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE

#define BTAPP_PROX_GATTC_PROFILE_APP_UUID      0x9999

static void btapp_gattc_profile_cback(tBTA_GATTC_EVT event,  tBTA_GATTC *p_data);

tBTAPP_GATTC_PROFILE_CB btapp_gattc_profile_cb = {0};

/* GATT client profile app UUID */
static tBT_UUID    btapp_gattc_profile_app_uuid = {2, {BTAPP_PROX_GATTC_PROFILE_APP_UUID}};

#define BTAPP_GATTC_SERVICE_SEARCH_MAX  8

static UINT16 btapp_gattc_profile_service_uuid[BTAPP_GATTC_SERVICE_SEARCH_MAX] =
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

/*******************************************************************************
**
** Function         btapp_gattc_profile_discover_service
**
** Description      search for services on remote device
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_discover_service(BD_ADDR remote_bda, UINT16 uuid)
{
    tBTAPP_GATTC_CLCB  *p_clcb = btapp_gattc_profile_find_clcb_by_bda(remote_bda);
    tBT_UUID   service_uuid = {2, {uuid}};

    if (p_clcb != NULL && p_clcb->conn_id != BTA_GATT_INVALID_CONN_ID)
    {
        BTA_GATTC_ServiceSearchRequest(p_clcb->conn_id, &service_uuid, btapp_gattc_profile_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_discover_all_service
**
** Description      search for services on remote device
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_discover_all_service(BD_ADDR remote_bda)
{
    tBTAPP_GATTC_CLCB  *p_clcb = btapp_gattc_profile_find_clcb_by_bda(remote_bda);

    if (p_clcb != NULL && p_clcb->conn_id != BTA_GATT_INVALID_CONN_ID)
    {
        BTA_GATTC_ServiceSearchRequest(p_clcb->conn_id, NULL, btapp_gattc_profile_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_clcb_alloc
**
** Description      The function allocates a   connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
tBTAPP_GATTC_CLCB *btapp_gattc_profile_clcb_alloc (UINT16 conn_id, BD_ADDR bda)
{
    UINT8         i_clcb = 0, i;
    tBTAPP_GATTC_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btapp_gattc_profile_cb.clcb; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (!p_clcb->in_use)
        {
            memset(p_clcb, 0, sizeof(tBTAPP_GATTC_CLCB));

            p_clcb->in_use      = TRUE;
            p_clcb->conn_id     = conn_id;
            p_clcb->connected   = TRUE;
            memcpy (p_clcb->bda, bda, BD_ADDR_LEN);

            for (i = 0; i < BTAPP_NUM_REM_DEVICE; i ++)
            {
                if (memcmp(btapp_device_db.device[i].bd_addr,
                           bda,
                           BD_ADDR_LEN) == 0)
                {
                    p_clcb->p_selected_rem_device = &btapp_device_db.device[i];
                    break;
                }
            }
            if (i == BTAPP_NUM_REM_DEVICE)
            {
                for (i = 0; i < BTAPP_NUM_INQ_DEVICE; i++)
                {
                    if (memcmp(btapp_inq_db.remote_device[i].bd_addr,
                               bda,
                               BD_ADDR_LEN) == 0)
                    {
                        p_clcb->p_selected_rem_device =  &btapp_inq_db.remote_device[i];
                        break;
                    }
                }
            }
            if (i == BTAPP_NUM_INQ_DEVICE)
            {
                BRCM_PLATFORM_TRACE("Unknown device connected"LINE_ENDING);
            }
            /* discover services interested by the app on peer device for the needed on GATT clients */
            p_clcb->service_mask = 0;
            p_clcb->search_idx = 0;

            btapp_gattc_profile_discover_service(bda,
                                                btapp_gattc_profile_service_uuid[p_clcb->search_idx]);
            //BTA_GATTC_ServiceSearchRequest(conn_id, NULL);

            return p_clcb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_dealloc
**
** Description      The function deallocates a connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
BOOLEAN btapp_gattc_profile_dealloc (UINT16 conn_id)
{
    UINT8         i_clcb = 0;
    tBTAPP_GATTC_CLCB    * p_clcb= btapp_gattc_profile_cb.clcb;

    for (; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && (p_clcb->conn_id == conn_id))
        {
            memset(p_clcb, 0, sizeof(tBTAPP_GATTC_CLCB));
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_find_clcb_by_bda
**
** Description      The function find the connection ID to a remote device
**
** Returns
**
*******************************************************************************/
tBTAPP_GATTC_CLCB * btapp_gattc_profile_find_clcb_by_bda(BD_ADDR remote_bda)
{
    UINT8               i_clcb = 0;
    tBTAPP_GATTC_CLCB    *p_clcb = btapp_gattc_profile_cb.clcb;

    for (; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected &&
            memcmp(p_clcb->bda, remote_bda, BD_ADDR_LEN) == 0)
        {
            return p_clcb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_find_clcb_by_conn_id
**
** Description      The function find the clcb by connection ID
**
** Returns
**
*******************************************************************************/
tBTAPP_GATTC_CLCB * btapp_gattc_profile_find_clcb_by_conn_id(UINT16 conn_id)
{
    UINT8               i_clcb = 0;
    tBTAPP_GATTC_CLCB    *p_clcb = btapp_gattc_profile_cb.clcb;

    for (; i_clcb < BTAPP_GATTC_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && conn_id == p_clcb->conn_id)
        {
            return p_clcb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_init
**
** Description      initialize GATTC client profile
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_init(void)
{
    if (!btapp_gattc_profile_cb.enabled)
    {
        memset(&btapp_gattc_profile_cb, 0, sizeof(tBTAPP_GATTC_PROFILE_CB));

        BTA_GATTC_AppRegister(&btapp_gattc_profile_app_uuid, btapp_gattc_profile_cback);
    }

    btapp_gattc_profile_cb.profiles ++;
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_disable
**
** Description      disable the GATT client profile in general.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_disable(void)
{
    btapp_gattc_profile_cb.profiles --;

    if (btapp_gattc_profile_cb.profiles == 0)
    {
        /* deregister the app */
        BTA_GATTC_AppDeregister(btapp_gattc_profile_cb.client_if, btapp_gattc_profile_cback);

        memset(&btapp_gattc_profile_cb, 0, sizeof(tBTAPP_GATTC_PROFILE_CB));
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_connect
**
** Description      GATT client initiate a connection.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_connect(BD_ADDR remote_bda, BOOLEAN is_direct)
{
    if(bdcmp(remote_bda, bd_addr_null))
    {
        memcpy(btapp_gattc_profile_cb.connecting_bda, remote_bda, BD_ADDR_LEN);
        /* initiate a connection to a peer device */

        BRCM_PLATFORM_TRACE("btapp_gattc_profile_connect bda:%s"LINE_ENDING, btapp_utl_bda_to_str(remote_bda));

        BTA_GATTC_Open(btapp_gattc_profile_cb.client_if, remote_bda, is_direct, BTA_TRANSPORT_LE, btapp_gattc_profile_cback);
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_profile_connect can't connect bda:%s"LINE_ENDING, btapp_utl_bda_to_str(remote_bda));
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_disconnect
**
** Description      disconnect the GATT client.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_disconnect(BD_ADDR remote_bda)
{
    tBTAPP_GATTC_CLCB  *p_clcb = btapp_gattc_profile_find_clcb_by_bda(remote_bda);

    /* remove device from background connection if it was in the list */
    BTA_GATTC_CancelOpen(btapp_gattc_profile_cb.client_if,
                        remote_bda,
                        FALSE,
                        btapp_gattc_profile_cback);
    if (p_clcb)
    {
        /* disconnect */
        BTA_GATTC_Close(p_clcb->conn_id, btapp_gattc_profile_cback);
    }
    else
    {
        BRCM_PLATFORM_TRACE("Link already dropped"LINE_ENDING);
    }
}

/*******************************************************************************
**
** Function         btapp_register_4_notification
**
** Description      register or deregister for notifications
**
**
** Returns          void
**
*******************************************************************************/
void btapp_register_4_notification(BD_ADDR bda, tBTA_GATTC_CHAR_ID *p_char_id,
                                   BOOLEAN is_register)
{
    tBTAPP_GATTC_CLCB  *p_clcb = btapp_gattc_profile_find_clcb_by_bda(bda);

    if (is_register)
        BTA_GATTC_RegisterForNotifications(btapp_gattc_profile_cb.client_if,
                                            p_clcb->bda,
                                            p_char_id);
    else
        BTA_GATTC_DeregisterForNotifications(btapp_gattc_profile_cb.client_if,
                                               p_clcb->bda,
                                               p_char_id);
}

/*******************************************************************************
**
** Function         btapp_gattc_send_confirm
**
** Description      Send confirmation for specified char id.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_send_confirm(UINT16 conn_id, tBTA_GATTC_CHAR_ID *p_char_id)
{
    BTA_GATTC_SendIndConfirm(conn_id, p_char_id, btapp_gattc_profile_cback);
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_remove_bg_dev
**
** Description      remove a device from backgroud connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_remove_bg_dev(BD_ADDR remote_bda)
{
    tBTAPP_REM_DEVICE * p_dev_rec;

    p_dev_rec = btapp_get_device_record(remote_bda);

    if (p_dev_rec && p_dev_rec->bg_conn)
    {
        BTA_GATTC_CancelOpen(btapp_gattc_profile_cb.client_if,
                             remote_bda,
                             FALSE,
                             btapp_gattc_profile_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_load_bonded
**
** Description      load  bonded device into background connection list
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_load_bonded(void)
{
    UINT8 i;

    /* enable background connection*/
    BTA_DmBleSetBgConnType(BTA_DM_BLE_CONN_AUTO, NULL);

    for (i = 0; i < BTAPP_NUM_REM_DEVICE; i ++)
    {
        if (btapp_device_db.device[i].ble_services & BTAPP_BLE_GATT_SERVICE_MASK &&
            (btapp_device_db.device[i].key_mask & BTA_LE_KEY_PENC))
        {
            BRCM_PLATFORM_TRACE("start backgournd connection for a saved device"LINE_ENDING);
            /* initiate a background connection to a peer device */
            btapp_gattc_profile_connect(btapp_device_db.device[i].bd_addr, FALSE);

            btapp_device_db.device[i].bg_conn = TRUE;
        }
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_proc_search_cmpl
**
** Description      service search completed
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_proc_search_cmpl(tBTA_GATTC_SEARCH_CMPL *p_res)
{
    tBTAPP_GATTC_CLCB *p_clcb = btapp_gattc_profile_find_clcb_by_conn_id(p_res->conn_id);

    BRCM_PLATFORM_TRACE("btapp_gattc_profile_proc_search_cmpl status = %d"LINE_ENDING, p_res->status);
    if (p_clcb == NULL)
    {
        BRCM_PLATFORM_TRACE("unknown conn_id = %d"LINE_ENDING, p_res->conn_id);
        return;
    }

    p_clcb->search_idx ++;

    if (p_clcb->search_idx < BTAPP_GATTC_SERVICE_SEARCH_MAX)
    {
        btapp_gattc_profile_discover_service(p_clcb->bda,
                                             btapp_gattc_profile_service_uuid[p_clcb->search_idx]);
    }
    else
    {
        p_clcb->search_idx = 0;
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_proc_search_res
**
** Description      process the service search result
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_proc_search_res(tBTA_GATTC_SRVC_RES *p_res)
{
    tBTAPP_GATTC_CLCB *p_clcb = btapp_gattc_profile_find_clcb_by_conn_id(p_res->conn_id);
//    tBT_UUID    char_uuid = {2, {0}};
//    tBTA_GATTC_CHAR_ID  char_result = {0};

    if (p_clcb != NULL)
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_profile_proc_search_res service = 0x%04x"LINE_ENDING,
                        p_res->service_uuid.id.uuid.uu.uuid16);

        /* always assume 16 bits UUID temp */
        switch (p_res->service_uuid.id.uuid.uu.uuid16)
        {
        case UUID_SERVCLASS_GATT_SERVER:
            p_clcb->service_mask |= BTAPP_BLE_GATT_SERVICE_MASK;
            break;

        default:
            BRCM_PLATFORM_TRACE("ignore service uuid 0x%04x for now"LINE_ENDING, p_res->service_uuid.id.uuid.uu.uuid16);
            break;
        }
    }
    else
    {
        APPL_TRACE_ERROR1("btapp_gattc_profile_proc_search_res unkown connection ID: %d", p_res->conn_id);
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_proc_read_res
**
** Description      read complete
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_proc_read_res(tBTA_GATTC_READ *p_res)
{
//    tBTAPP_GATTC_CLCB *p_clcb = btapp_gattc_profile_find_clcb_by_conn_id(p_res->conn_id);

    if (p_res->status != BTA_GATT_OK)
    {
        BRCM_PLATFORM_TRACE("GATT read failure code= %d"LINE_ENDING, p_res->status);
        return;
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_proc_write_res
**
** Description      write complete
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_proc_write_res(tBTA_GATTC_WRITE *p_res)
{
//    tBTAPP_GATTC_CLCB *p_clcb = btapp_gattc_profile_find_clcb_by_conn_id(p_res->conn_id);

    if (p_res->status != BTA_GATT_OK)
    {
        BRCM_PLATFORM_TRACE("GATT write failure code= 0x%02x"LINE_ENDING, p_res->status);
        return;
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_proc_notify
**
** Description      process notification data
**
** Returns          void
**
*******************************************************************************/
void btapp_gattc_profile_proc_notify(tBTA_GATTC_NOTIFY *p_res)
{
    tBTAPP_GATTC_CLCB *p_clcb = btapp_gattc_profile_find_clcb_by_conn_id(p_res->conn_id);

    if (p_clcb == NULL)
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_profile_proc_notify: Unknown notification for conn_id: %d"LINE_ENDING,
                        p_res->conn_id);
        return;
    }
}

void btapp_gattc_enc_cback (BD_ADDR bd_addr, tBTA_TRANSPORT transport, tBTA_STATUS result)
{
    if (result != 0)
    {
        BRCM_PLATFORM_TRACE("btapp_gattc_enc_cback failed"LINE_ENDING);
       /* BTA_DmCloseACL(bd_addr, TRUE); */
    }
}

/*******************************************************************************
**
** Function         btapp_gattc_profile_cback
**
** Description      GATT Client Profile callback.
**
** Returns          void
**
*******************************************************************************/
static void btapp_gattc_profile_cback(tBTA_GATTC_EVT event,  tBTA_GATTC *p_data)
{
    tBTAPP_GATTC_PROFILE_CB   *p_cb = &btapp_gattc_profile_cb;
    tBTAPP_REM_DEVICE * p_device_record;

    BRCM_PLATFORM_TRACE("btapp_gattc_profile_cback event = %d "LINE_ENDING, event);

    switch (event)
    {
        case BTA_GATTC_REG_EVT:
            BRCM_PLATFORM_TRACE("btapp_gattc_profile_cback  rcv BTA_GATTC_REG_EVT client_if= %d status=%d"LINE_ENDING,
                            p_data->reg_oper.client_if, p_data->reg_oper.status );

            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                p_cb->enabled = TRUE;
                p_cb->client_if = p_data->reg_oper.client_if;

            }
            break;

        case BTA_GATTC_DEREG_EVT:
            p_cb->enabled = FALSE;
            p_cb->client_if = 0;
            break;

        case BTA_GATTC_OPEN_EVT:
            if (p_data->open.status == 0)
            {
                if ( btapp_gattc_profile_clcb_alloc(p_data->open.conn_id, p_data->open.remote_bda)== NULL)
                {
                    BRCM_PLATFORM_TRACE ("btapp_gattc_profile_clcb_alloc: no_resource"LINE_ENDING);
                }

                p_device_record = btapp_get_device_record(p_data->open.remote_bda);
                p_device_record = p_device_record;
                /* disable auto encryption for BTA to test data signing
                if (p_device_record &&
                    (p_device_record->link_key_present || ((p_device_record->key_mask & BTA_LE_KEY_PENC) != 0)))
                {
                    BTA_DmSetEncryption(p_data->open.remote_bda, p_data->open.transport, btapp_gattc_enc_cback, BTA_DM_BLE_SEC_ENCRYPT);
                }
                */
            }
            break;


        case BTA_GATTC_CLOSE_EVT:
            BRCM_PLATFORM_TRACE("btapp_gattc_profile_cback rcv BTA_GATTC_CLOSE_EVT "LINE_ENDING);

            if (p_data->close.status == 0 && p_data->close.client_if == p_cb->client_if)
            {
                if (!btapp_gattc_profile_dealloc(p_data->close.conn_id))
                {
                    BRCM_PLATFORM_TRACE ("btapp_gattc_profile_dealloc: no_resource"LINE_ENDING);
                }
            }
            else
            {
                BRCM_PLATFORM_TRACE ("Ignore BTA_GATTC_DISCONNECT_EVT wrong client_if: data %d, cb %d, status %d"LINE_ENDING,
                    p_data->close.client_if, p_cb->client_if, p_data->close.status);
            }
            break;

        case BTA_GATTC_SEARCH_RES_EVT:
            btapp_gattc_profile_proc_search_res(&p_data->srvc_res);
            break;

        case BTA_GATTC_SEARCH_CMPL_EVT:
            btapp_gattc_profile_proc_search_cmpl(&p_data->search_cmpl);
            break;

        case BTA_GATTC_READ_CHAR_EVT:
        case BTA_GATTC_READ_DESCR_EVT:
            btapp_gattc_profile_proc_read_res(&p_data->read);
            break;

        case BTA_GATTC_WRITE_CHAR_EVT:
        case BTA_GATTC_WRITE_DESCR_EVT:
            btapp_gattc_profile_proc_write_res(&p_data->write);
            break;

        case BTA_GATTC_NOTIF_EVT:
            btapp_gattc_profile_proc_notify(&p_data->notify);
            break;

        case BTA_GATTC_SRVC_CHG_EVT:
            BRCM_PLATFORM_TRACE("receive service change notification"LINE_ENDING);
            break;

        default:
            break;
    }
}

#endif
