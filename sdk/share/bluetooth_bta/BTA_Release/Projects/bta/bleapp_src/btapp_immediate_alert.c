/****************************************************************************
**                                                                           
**  Name:          btapp_immediate_alert.c                                           
**                 
**  Description:   Contains btui functions for Link Loss service
**                 
**                                                                           
**  Copyright (c) 2003-2010, Broadcom Corp, All Rights Reserved.              
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
******************************************************************************/

#include "bt_target.h"

//#if( defined BTA_IMMEDIATE_ALERT_INCLUDED ) && (BTA_IMMEDIATE_ALERT_INCLUDED == TRUE)
#if( BLE_INCLUDED == TRUE) && (BTA_GATT_INCLUDED == TRUE)


#include "bta_api.h"
#include "btapp.h"
#include "btapp_int.h"
#include "btapp_gattc_profile.h"
#include "btapp_immediate_alert.h"


/* BTUI Link Loss main control block */
tBTUI_IMMEDIATE_ALERT_CB btui_immediate_alert_cb = {0};

static tBT_UUID    btui_immediate_alert_app_uuid = {2, {BTUI_IMMEDIATE_ALERT_APP_UUID}};
static tBT_UUID    btui_immediate_alert_service_uuid = {2, {UUID_SERVCLASS_IMMEDIATE_ALERT}};
static tBT_UUID    btui_alertlevel_uuid = {2, {GATT_UUID_ALERT_LEVEL}};

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_immediate_alert_cback(tBTA_GATTS_EVT event, tBTA_GATTS *p_data);


/*******************************************************************************
**
** Function         btapp_immediate_alert_clcb_alloc
**
** Description      The function allocates a   connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
tBTUI_IMMEDIATE_ALERT_CLCB *btapp_immediate_alert_clcb_alloc (UINT16 conn_id, BD_ADDR bda)
{
    UINT8         i_clcb = 0;
    tBTUI_IMMEDIATE_ALERT_CLCB    *p_clcb = NULL;

    APPL_TRACE_DEBUG0("btapp_immediate_alert_clcb_alloc");
    for (i_clcb = 0, p_clcb= btui_immediate_alert_cb.clcb; i_clcb < BTUI_IMMEDIATE_ALERT_MAX_CL; i_clcb++, p_clcb++)
    {
        if (!p_clcb->in_use)
        {
            p_clcb->in_use      = TRUE;
            p_clcb->conn_id     = conn_id;
            p_clcb->connected   = TRUE;            
            memcpy (p_clcb->bda, bda, BD_ADDR_LEN);
            return p_clcb;
        }
    }
    return NULL;
}
/*******************************************************************************
**
** Function         btapp_immediate_alert_dealloc
**
** Description      The function deallocates a connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
BOOLEAN btapp_immediate_alert_dealloc (UINT16 conn_id)
{
    UINT8         i_clcb = 0;
    tBTUI_IMMEDIATE_ALERT_CLCB    *p_clcb = NULL;

    APPL_TRACE_DEBUG0("btapp_immediate_alert_clcb_alloc");

    for (i_clcb = 0, p_clcb= btui_immediate_alert_cb.clcb; i_clcb < BTUI_IMMEDIATE_ALERT_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && (p_clcb->conn_id == conn_id))
        {
            memset(p_clcb, 0, sizeof(tBTUI_IMMEDIATE_ALERT_CLCB));
            return TRUE;
        }
    }
    return FALSE;
}


/*******************************************************************************
**
** Function         btapp_immediate_alert_init
**
** Description      Initializes Link Loss Service Application
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_immediate_alert_init(void)
{
    if (!btui_immediate_alert_cb.enabled)
    {
        APPL_TRACE_DEBUG0("btapp_immediate_alert_init");

        memset(&btui_immediate_alert_cb, 0, sizeof(tBTUI_IMMEDIATE_ALERT_CB));

        BTA_GATTS_AppRegister(&btui_immediate_alert_app_uuid, btapp_immediate_alert_cback);
    }
    else
    {
        APPL_TRACE_DEBUG0("immediate alert already been initialized");
    }

    btui_immediate_alert_cb.profile_count ++;
}

/*******************************************************************************
**
** Function         btapp_immediate_alert_instatiate
**
** Description      Instatiate a Link Loss Service
**                  
** Returns          void
**
*******************************************************************************/
void btapp_immediate_alert_instatiate(void)
{
    /* only allow for one instance now by checking instance service ID, 
    ** since the client can not dirrenciate the two instances
    ** for different profiles  */
    if (btui_immediate_alert_cb.srvc_inst.service_id == 0)
    {
        BTA_GATTS_CreateService(btui_immediate_alert_cb.server_if, 
                                &btui_immediate_alert_service_uuid,
                                0,    /* theretical only on e instance per device */
                                BTUI_IMMEDIATE_ALERT_HANDLE_NUM,
                                TRUE);  /* always create as primary service */
    }
    else
    {
        APPL_TRACE_DEBUG0("immediate alert has already been instantiate");
    }
}

/*******************************************************************************
**
** Function         btapp_immediate_alert_disable
**
** Description      disable a Link Loss Service
**                  
** Returns          void
**
*******************************************************************************/
void btapp_immediate_alert_disable(void)
{
    if (btui_immediate_alert_cb.profile_count > 0) 
        btui_immediate_alert_cb.profile_count --;

    if (btui_immediate_alert_cb.profile_count == 0)
    {
        APPL_TRACE_DEBUG0("btapp_immediate_alert_disable");
        BTA_GATTS_StopService(btui_immediate_alert_cb.srvc_inst.service_id);
        BTA_GATTS_AppDeregister(btui_immediate_alert_cb.server_if);
    }
}
/*******************************************************************************
**
** Function         btapp_build_service
**
** Description      build and start a link loss service database
**                  
** Returns          void
**
*******************************************************************************/
void btapp_build_service(void)
{
    tBTUI_IMMEDIATE_ALERT_CB   *p_cb = &btui_immediate_alert_cb;

    APPL_TRACE_DEBUG0("btapp_build_service add char and start service ");

    BTA_GATTS_AddCharacteristic (p_cb->srvc_inst.service_id, 
                                 &btui_alertlevel_uuid,
                                 BTUI_IA_ALERTLEVEL_PERM,
                                 BTUI_IA_ALERTLEVEL_PROP);

    BTA_GATTS_StartService(p_cb->srvc_inst.service_id, BTA_GATT_TRANSPORT_LE_BR_EDR);

    return;
}
/*******************************************************************************
**
** Function         btapp_immediate_alert_add_char_cmpl
**
** Description      process add characteristic complete.
**                  
** Returns          void
**
*******************************************************************************/
void btapp_immediate_alert_add_char_cmpl(tBTUI_IMMEDIATE_ALERT_CB *p_cb, UINT16 service_id, UINT16 attr_id)
{
    APPL_TRACE_DEBUG1("btapp_immediate_alert_add_char_cmpl service_id= %d", service_id);

    if (p_cb->srvc_inst.service_id == service_id)
    {
        p_cb->srvc_inst.attr_id = attr_id;
        p_cb->srvc_inst.alert_value = BTUI_IA_ALERT_VALUE_DEF;
        APPL_TRACE_DEBUG1("btapp_immediate_alert_add_char_cmpl service_id= %d", service_id);
    }
    else
    {
        if (attr_id == 0)
        {
            APPL_TRACE_ERROR0("add char failed");
        }
        else
        {
            APPL_TRACE_ERROR0("no matching service found");
        }
    }

}
/*******************************************************************************
**
** Function         btapp_immediate_alert_proc_read
**
** Description      process a read alert level attribute request
**                  
** Returns          void
**
*******************************************************************************/
void btapp_immediate_alert_proc_read(tBTUI_IMMEDIATE_ALERT_CB *p_cb, tBTA_GATTS_REQ *p_req_data)
{
    tBTA_GATT_STATUS    status = BTA_GATT_NOT_FOUND;

    APPL_TRACE_DEBUG1("btapp_immediate_alert_proc_read handle = %d ", p_req_data->p_data->read_req.handle); 

    if (p_cb->srvc_inst.attr_id == p_req_data->p_data->read_req.handle)
    {
        APPL_TRACE_DEBUG1("btapp_linkloss_proc_read found handle = %d ",p_req_data->p_data->read_req.handle); 
        status = BTA_GATT_READ_NOT_PERMIT;

    }
    else
    {
        APPL_TRACE_ERROR1("unknow attribute handle = %d", p_req_data->p_data->read_req.handle);
    }
        
    BTA_GATTS_SendRsp(p_req_data->conn_id, 
                      p_req_data->trans_id,
                      status,
                      NULL);
}
/*******************************************************************************
**
** Function         btapp_immediate_alert_proc_write
**
** Description      process a write alert level attribute request
**                  
** Returns          void
**
*******************************************************************************/
void btapp_immediate_alert_proc_write(tBTUI_IMMEDIATE_ALERT_CB *p_cb,   tBTA_GATTS_REQ *p_req_data)
{
    tBTA_GATT_WRITE_REQ *p_write_req = &p_req_data->p_data->write_req;
    tBTA_GATT_STATUS    status = BTA_GATT_INVALID_HANDLE;

    APPL_TRACE_DEBUG0("btapp_immediate_alert_proc_write" );

    if (p_cb->srvc_inst.attr_id == p_write_req->handle)
    {
        APPL_TRACE_DEBUG1("btapp_immediate_alert_proc_write found  handle = %d ",  p_write_req->handle);     
        if (p_write_req->is_prep || p_write_req->need_rsp)
        {
            APPL_TRACE_ERROR0("btapp_immediate_alert_proc_write prep_write status = BTA_GATT_REQ_NOT_SUPPORTED");   
            status = BTA_GATT_REQ_NOT_SUPPORTED;

            BTA_GATTS_SendRsp(p_req_data->conn_id, 
                              p_req_data->trans_id,
                              status,
                              NULL);

        }
        else /* write no response */
        {
            if (p_write_req->len != sizeof(tBTUI_IA_ALERT_LEVEL))
            {
                APPL_TRACE_ERROR0("btapp_immediate_alert_proc_write prep_write status = BTA_GATT_INVALID_ATTR_LEN");       
                status = BTA_GATT_INVALID_ATTR_LEN;
            }
            else
            {
                status = BTA_GATT_OK;
                p_cb->srvc_inst.alert_value = p_write_req->value[0];
                APPL_TRACE_DEBUG1("btapp_immediate_alert_proc_write VALUE=%d", p_cb->srvc_inst.alert_value);  
                /* simulate noise */
            }
        }
    }
    else
    {
        APPL_TRACE_ERROR1("unknow attribute handle = %d", p_write_req->handle);
    }
}
/*******************************************************************************
**
** Function         btapp_immediate_alert_cback
**
** Description      link loss service callback.
**                  
** Returns          void
**
*******************************************************************************/
static void btapp_immediate_alert_cback(tBTA_GATTS_EVT event,   tBTA_GATTS *p_data)
{
    tBTUI_IMMEDIATE_ALERT_CB   *p_cb = &btui_immediate_alert_cb;

    APPL_TRACE_DEBUG1("btapp_immediate_alert_cback event = %d", event);

    switch (event)
    {
        case BTA_GATTS_REG_EVT:
            APPL_TRACE_DEBUG1("btapp_immediate_alert_cback registered  server_if= %d", p_data->reg_oper.server_if );
            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                p_cb->server_if = p_data->reg_oper.server_if;
                p_cb->enabled = TRUE;
                btapp_immediate_alert_instatiate();
            }
            break;

        case BTA_GATTS_DEREG_EVT:
            p_cb->enabled = FALSE;
            p_cb->server_if = BTA_GATTS_INVALID_IF;
            break;

        case BTA_GATTS_CREATE_EVT:
            APPL_TRACE_DEBUG2("btapp_immediate_alert_cback BTA_GATTS_CREATE_EVT status = %d service_id=%d", 
                              p_data->create.status, p_data->create.service_id);

            if (p_data->create.status == BTA_GATT_OK)
            {
                p_cb->srvc_inst.service_id = p_data->create.service_id;
                /* add characteristic to this service */
                btapp_build_service();
            }
            break;

        case BTA_GATTS_ADD_CHAR_EVT:

            APPL_TRACE_DEBUG2("btapp_immediate_alert_cback rcv BTA_GATTS_ADD_CHAR_EVT status = %d attr_id=%d", 
                              p_data->add_result.status, p_data->add_result.attr_id);
            if (p_data->add_result.status == BTA_GATT_OK)
            {
                btapp_immediate_alert_add_char_cmpl(p_cb, p_data->add_result.service_id, 
                                                    p_data->add_result.attr_id);
            }
            break;

        case BTA_GATTS_START_EVT:
            APPL_TRACE_DEBUG1("btapp_immediate_alert_cback rcv BTA_GATTS_START_EVT status = 0x%d", p_data->srvc_oper.status);
            break;

        case BTA_GATTS_READ_EVT:
            APPL_TRACE_DEBUG0("btapp_immediate_alert_cback rcv BTA_GATTS_READ_EVT ");
            btapp_immediate_alert_proc_read(p_cb, &p_data->req_data);
            break;

        case BTA_GATTS_WRITE_EVT:
            APPL_TRACE_DEBUG0("btapp_immediate_alert_cback rcv BTA_GATTS_WRITE_EVT ");
            btapp_immediate_alert_proc_write(p_cb,&p_data->req_data);
            break;

        case BTA_GATTS_CONNECT_EVT:
            APPL_TRACE_DEBUG2("btapp_immediate_alert_cback rcv BTA_GABTA_GATTS_CONNECT_EVT service_if=%d  conn_id= 0x%d", 
                              p_data->conn.server_if, p_data->conn.conn_id);

            if (p_data->conn.server_if == p_cb->server_if)
            {
                if (btapp_immediate_alert_clcb_alloc(p_data->conn.conn_id, p_data->conn.remote_bda)== NULL)
                {
                    APPL_TRACE_DEBUG0 ("btapp_immediate_alert_clcb_alloc: no_resource");
                }
            }
            else
            {
                APPL_TRACE_ERROR1 ("Ignore BTA_GATTS_CONN_EVT wrong sever_if=%d",p_data->conn.server_if);
            }

            break;
        case BTA_GATTS_DISCONNECT_EVT:

            APPL_TRACE_DEBUG2("btapp_immediate_alert_cback rcv BTA_GATTS_DISCONNECT_EVT service_if=%d  conn_id= 0x%d", 
                              p_data->conn.server_if, p_data->conn.conn_id);

            if (p_data->conn.server_if == p_cb->server_if)
            {
                btapp_immediate_alert_dealloc(p_data->conn.conn_id);
            }
            else
            {
                APPL_TRACE_ERROR1 ("Ignore BTA_GATTS_DISCONN_EVT wrong sever_if=%d",p_data->conn.server_if);
            }
            break;
    }
}

/*******************************************************************************
**
** Function         btapp_immediate_alert_write_alert_level
**
** Description      write immediate alert level
**                  
** Returns          void
**
*******************************************************************************/
void btapp_immediate_alert_write_alert_level(BD_ADDR remote_bda, tBTUI_IA_ALERT_LEVEL alert_level)
{
    tBTAPP_GATTC_CLCB  *p_clcb = btapp_gattc_profile_find_clcb_by_bda(remote_bda);
    tBTA_GATTC_CHAR_ID  char_id;

    if (p_clcb->conn_id != BTA_GATT_INVALID_CONN_ID)
    {
        memcpy(&char_id.srvc_id, &p_clcb->ia_srvc_id, sizeof(tBTA_GATT_SRVC_ID));
        memcpy(&char_id.char_id, &p_clcb->ia_alert_level_id, sizeof(tBTA_GATT_ID));

        BTA_GATTC_WriteCharValue(p_clcb->conn_id, 
                                 &char_id, 
                                 BTA_GATTC_TYPE_WRITE_NO_RSP,
                                 1, /* one byte */
                                 &alert_level,
                                 BTA_GATT_AUTH_REQ_NONE,
                                 NULL);
        
        p_clcb->ia_alert_level_value = alert_level;
    }
}

//#endif
#endif










