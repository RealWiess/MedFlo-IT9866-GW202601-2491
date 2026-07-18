/****************************************************************************
**                                                                           
**  Name:          btapp_linkloss.c                                           
**                 
**  Description:   Contains btui functions for Link Loss service
**                 
**                                                                           
**  Copyright (c) 2003-2010, Broadcom Corp, All Rights Reserved.              
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
******************************************************************************/

#include "bt_target.h"

//#if( defined BTA_LINKLOSS_INCLUDED ) && (BTA_LINKLOSS_INCLUDED == TRUE)
#if( BLE_INCLUDED == TRUE) && (BTA_GATT_INCLUDED == TRUE)


#include "bta_api.h"
#include "btapp.h"
#include "btapp_int.h"
#include "btapp_linkloss.h"


/* BTUI Link Loss main control block */
tBTUI_LINKLOSS_CB btui_linkloss_cb = {0};

static tBT_UUID    btui_linkloss_app_uuid = {2, {BTUI_LINKLOSS_APP_UUID}};
static tBT_UUID    btui_linkloss_service_uuid = {2, {UUID_SERVCLASS_LINKLOSS}};
static tBT_UUID    btui_alertlevel_uuid = {2, {GATT_UUID_ALERT_LEVEL}};

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_linkloss_cback(tBTA_GATTS_EVT event, tBTA_GATTS *p_data);


/*******************************************************************************
**
** Function         btapp_linkloss_clcb_alloc
**
** Description      The function allocates a   connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
tBTUI_LINKLOSS_CLCB *btapp_linkloss_clcb_alloc (UINT16 conn_id, BD_ADDR bda)
{
    UINT8         i_clcb = 0;
    tBTUI_LINKLOSS_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btui_linkloss_cb.clcb; i_clcb < BTUI_LINKLOSS_MAX_CL; i_clcb++, p_clcb++)
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
** Function         btapp_linkloss_dealloc
**
** Description      The function deallocates a connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
BOOLEAN btapp_linkloss_dealloc (UINT16 conn_id)
{
    UINT8         i_clcb = 0;
    tBTUI_LINKLOSS_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btui_linkloss_cb.clcb; i_clcb < BTUI_LINKLOSS_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && (p_clcb->conn_id == conn_id))
        {
            memset(p_clcb, 0, sizeof(tBTUI_LINKLOSS_CLCB));
            return TRUE;
        }
    }
    return FALSE;
}




/*******************************************************************************
**
** Function         btapp_linkloss_init
**
** Description      Initializes Link Loss Service Application
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_linkloss_init(void)
{
    APPL_TRACE_DEBUG0("btapp_linkloss_init");

    if (!btui_linkloss_cb.enabled)
    {
        memset(&btui_linkloss_cb, 0, sizeof(tBTUI_LINKLOSS_CB));

        BTA_GATTS_AppRegister(&btui_linkloss_app_uuid, btapp_linkloss_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_linkloss_init
**
** Description      Initializes Link Loss Service Application
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_linkloss_disable(void)
{
    APPL_TRACE_DEBUG0("btapp_linkloss_disable");

    BTA_GATTS_AppDeregister(btui_linkloss_cb.server_if);  

    memset(&btui_linkloss_cb, 0, sizeof(tBTUI_LINKLOSS_CB));
}
/*******************************************************************************
**
** Function         btapp_linkloss_instatiate
**
** Description      Instatiate a Link Loss Service
**                  
** Returns          void
**
*******************************************************************************/
void btapp_linkloss_instatiate(void)
{
    APPL_TRACE_DEBUG0("btapp_linkloss_instatiate ");

    BTA_GATTS_CreateService(btui_linkloss_cb.server_if, 
                            &btui_linkloss_service_uuid,
                            btui_linkloss_cb.inst_id,    /* theretical only on e instance per device */
                            BTUI_LINKLOSS_HANDLE_NUM,
                            TRUE);  /* always create as primary service */
}

/*******************************************************************************
**
** Function         btapp_linkloss_stop_service
**
** Description      disable a Link Loss Service
**                  
** Returns          void
**
*******************************************************************************/
void btapp_linkloss_stop_service(UINT8 inst_id)
{
    APPL_TRACE_DEBUG0("btapp_linkloss_disable ");
    BTA_GATTS_StopService(btui_linkloss_cb.srvc_inst[inst_id].service_id);  
}
/*******************************************************************************
**
** Function         btapp_build_linkloss_service
**
** Description      build and start a link loss service database
**                  
** Returns          void
**
*******************************************************************************/
void btapp_build_linkloss_service(UINT8 inst_id)
{
    tBTUI_LINKLOSS_CB   *p_cb = &btui_linkloss_cb;
    APPL_TRACE_DEBUG2("btapp_build_linkloss_service inst_id=%d service_id= %d",inst_id,  p_cb->srvc_inst[inst_id].service_id);
    BTA_GATTS_AddCharacteristic (p_cb->srvc_inst[inst_id].service_id, 
                                 &btui_alertlevel_uuid,
                                 BTUI_ALERTLEVEL_PERM,
                                 BTUI_ALERTLEVEL_PROP);

    BTA_GATTS_StartService(p_cb->srvc_inst[inst_id].service_id, BTA_GATT_TRANSPORT_LE_BR_EDR);

    return;
}
/*******************************************************************************
**
** Function         btapp_linkloss_add_char_cmpl
**
** Description      process add characteristic complete.
**                  
** Returns          void
**
*******************************************************************************/
void btapp_linkloss_add_char_cmpl(tBTUI_LINKLOSS_CB *p_cb, UINT16 service_id, UINT16 attr_id)
{
    UINT8   i;

    APPL_TRACE_DEBUG1("btapp_linkloss_add_char_cmpl service_id= %d", service_id);

    for (i = 0; i < BTUI_LINKLOSS_INST_MAX; i ++)
    {
        if (p_cb->srvc_inst[i].service_id == service_id)
        {
            APPL_TRACE_DEBUG2("btapp_linkloss_add_char_cmpl index=%d service_id= %d", i, service_id);
            break;
        }
    }
    if (i != BTUI_LINKLOSS_INST_MAX && attr_id != 0)
    {
        p_cb->srvc_inst[i].attr_id = attr_id;
        p_cb->srvc_inst[i].alert_value = BTUI_LL_ALERT_VALUE_DEF;
        APPL_TRACE_DEBUG1("add char ID = 0x%04x", attr_id);
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
** Function         btapp_linkloss_proc_read
**
** Description      process a read alert level attribute request
**                  
** Returns          void
**
*******************************************************************************/
void btapp_linkloss_proc_read(tBTUI_LINKLOSS_CB *p_cb, tBTA_GATTS_REQ *p_req_data)
{
    tBTA_GATT_STATUS    status = BTA_GATT_OK;
    tBTA_GATTS_RSP      rsp, *p_rsp = &rsp;
    tBTA_GATT_VALUE     *p_value = &rsp.attr_value;
    UINT8       i, *pp = p_value->value;
    APPL_TRACE_DEBUG1("btapp_linkloss_proc_read handle = %d ", p_req_data->p_data->read_req.handle); 
    for (i = 0; i < BTUI_LINKLOSS_INST_MAX; i ++)
    {
        if (p_cb->srvc_inst[i].attr_id == p_req_data->p_data->read_req.handle)
        {
            APPL_TRACE_DEBUG2("btapp_linkloss_proc_read found inst=%d handle = %d ", i, p_req_data->p_data->read_req.handle); 
            break;
        }
    }
    if (i != BTUI_LINKLOSS_INST_MAX)
    {
        if (p_req_data->p_data->read_req.is_long)
        {
            APPL_TRACE_ERROR0("btapp_linkloss_proc_read request is long send an error request should not be long"); 
            status = BTA_GATT_NOT_LONG;
            p_rsp = NULL;
        }
        else
        {
            p_value->handle = p_cb->srvc_inst[i].attr_id;
            p_value->len = sizeof(tBTUI_ALERT_LEVEL);
            UINT8_TO_STREAM(pp, p_cb->srvc_inst[i].alert_value);
            APPL_TRACE_DEBUG4("btapp_linkloss_proc_read value handle=%d len=%d value=%d service_id=%d",
                              p_value->handle, p_value->len, p_cb->srvc_inst[i].alert_value, p_cb->srvc_inst[i].service_id); 

        }

        BTA_GATTS_SendRsp(p_req_data->conn_id, 
                          p_req_data->trans_id,
                          status,
                          p_rsp);
    }
}
/*******************************************************************************
**
** Function         btapp_linkloss_proc_write
**
** Description      process a write alert level attribute request
**                  
** Returns          void
**
*******************************************************************************/
void btapp_linkloss_proc_write(tBTUI_LINKLOSS_CB *p_cb,  tBTA_GATTS_REQ *p_req_data)
{
    tBTA_GATT_WRITE_REQ *p_write_req = &p_req_data->p_data->write_req;
    tBTA_GATT_STATUS    status = BTA_GATT_INVALID_HANDLE;
    tBTA_GATTS_RSP      rsp;
    tBTA_GATT_VALUE     *p_value = NULL;
    UINT8               i, *pp = p_value->value;

    APPL_TRACE_DEBUG1("btapp_linkloss_proc_write p_write_req->handle = %d ", p_write_req->handle);
    for (i = 0; i < BTUI_LINKLOSS_INST_MAX; i ++)
    {
        if (p_cb->srvc_inst[i].attr_id == p_write_req->handle)
        {
            APPL_TRACE_DEBUG2("btapp_linkloss_proc_write found inst=%d handle = %d ", i, p_req_data->p_data->read_req.handle); 
            break;
        }
    }
    if (i != BTUI_LINKLOSS_INST_MAX)
    {
        if (p_write_req->is_prep)
        {
            APPL_TRACE_DEBUG0("btapp_linkloss_proc_write prep_write"); 
            if (p_write_req->offset > sizeof(tBTUI_ALERT_LEVEL))
            {
                APPL_TRACE_ERROR0("btapp_linkloss_proc_write prep_write status = BTA_GATT_INVALID_OFFSET");    
                status = BTA_GATT_INVALID_OFFSET;
            }

            else if (p_cb->prep_num >= BTUI_LINKLOSS_PREP_MAX)
            {
                APPL_TRACE_ERROR0("btapp_linkloss_proc_write prep_write status = BTA_GATT_PREPARE_Q_FULL");      
                status = BTA_GATT_PREPARE_Q_FULL;
            }
            else
            {
                status = BTA_GATT_OK;

                p_value = &rsp.attr_value;
                p_value->handle = p_cb->srvc_inst[i].attr_id;
                p_value->len = sizeof(tBTUI_ALERT_LEVEL);
                UINT8_TO_STREAM(pp, p_write_req->value);
                /* put into prep q */
                p_cb->prep_q[p_cb->prep_num ++] = p_write_req->value[0];
                APPL_TRACE_DEBUG1("btapp_linkloss_proc_write prep_write status = BTA_GATT_OK value=%d", p_cb->prep_q[p_cb->prep_num - 1]);      
            }
        }
        else
        {
            if (p_write_req->len != sizeof(tBTUI_ALERT_LEVEL))
            {
                APPL_TRACE_ERROR0("btapp_linkloss_proc_write write status = BTA_GATT_INVALID_ATTR_LEN");     
                status = BTA_GATT_INVALID_ATTR_LEN;
            }
            else
            {
                status = BTA_GATT_OK;
                p_cb->srvc_inst[i].alert_value = p_write_req->value[0];
                APPL_TRACE_DEBUG2("btapp_linkloss_proc_write write status = BTA_GATT_OK index=%d value=%d",i, p_cb->srvc_inst[i].alert_value);   
            }
        }
    }

    if (p_write_req->need_rsp)
    {
        BTA_GATTS_SendRsp(p_req_data->conn_id, 
                          p_req_data->trans_id,
                          status,
                          &rsp);
    }

}
/*******************************************************************************
**
** Function         btapp_linkloss_cback
**
** Description      link loss service callback.
**                  
** Returns          void
**
*******************************************************************************/
static void btapp_linkloss_cback(tBTA_GATTS_EVT event,  tBTA_GATTS *p_data)
{
    tBTUI_LINKLOSS_CB   *p_cb = &btui_linkloss_cb;

    APPL_TRACE_DEBUG1("btapp_linkloss_cback event = %d ", event);

    switch (event)
    {
        case BTA_GATTS_REG_EVT:
            APPL_TRACE_DEBUG2("btapp_linkloss_cback  rcv BTA_GATTS_REG_EVT server_if= %d status=%d", 
                              p_data->reg_oper.server_if, p_data->reg_oper.status );
            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                p_cb->enabled = TRUE;
                p_cb->server_if = p_data->reg_oper.server_if;

                btapp_linkloss_instatiate();
            }
            break;

        case BTA_GATTS_DEREG_EVT:
            p_cb->server_if = BTA_GATTS_INVALID_IF;
            break;

        case BTA_GATTS_CREATE_EVT:
            APPL_TRACE_DEBUG1("btapp_linkloss_cback  rcv BTA_GATTS_CREATE_EVT status= %d", p_data->create.status);
            if (p_data->create.status == BTA_GATT_OK)
            {
                p_cb->srvc_inst[p_cb->inst_id].service_id = p_data->create.service_id;
                /* add characteristic to this service */
                btapp_build_linkloss_service(p_cb->inst_id);
                p_cb->inst_id ++;
            }
            break;

        case BTA_GATTS_ADD_CHAR_EVT:
            APPL_TRACE_DEBUG1("btapp_linkloss_cback rcv BTA_GATTS_ADD_CHAR_EVT status = 0x%d", p_data->srvc_oper.status);
            if (p_data->add_result.status == BTA_GATT_OK)
            {
                btapp_linkloss_add_char_cmpl(p_cb, p_data->add_result.service_id, p_data->add_result.attr_id);
            }
            break;

        case BTA_GATTS_START_EVT:
            APPL_TRACE_DEBUG1("btapp_linkloss_cback rcv BTA_GATTS_START_EVT status = 0x%d", p_data->srvc_oper.status);
            break;

        case BTA_GATTS_READ_EVT:
            APPL_TRACE_DEBUG0("btapp_linkloss_cback rcv BTA_GATTS_READ_EVT ");
            btapp_linkloss_proc_read(p_cb, &p_data->req_data);
            break;

        case BTA_GATTS_WRITE_EVT:
            APPL_TRACE_DEBUG0("btapp_linkloss_cback rcv BTA_GATTS_WRITE_EVT ");
            btapp_linkloss_proc_write(p_cb, &p_data->req_data);
            break;
        case BTA_GATTS_CONNECT_EVT:
            APPL_TRACE_DEBUG0("btapp_linkloss_cback rcv BTA_GATTS_CONNECT_EVT ");
            if (p_data->conn.server_if == p_cb->server_if)
            {
                if ( btapp_linkloss_clcb_alloc(p_data->conn.conn_id, p_data->conn.remote_bda)== NULL)
                {
                    APPL_TRACE_ERROR0 ("btapp_linkloss_clcb_alloc: no_resource");
                }
            }
            else
            {
                APPL_TRACE_ERROR1 ("Ignore BTA_GATTS_CONN_EVT wrong sever_if=%d",p_data->conn.server_if);
            }

            break;
        case BTA_GATTS_DISCONNECT_EVT:
            APPL_TRACE_DEBUG0("btapp_linkloss_cback rcv BTA_GATTS_DISCONNECT_EVT ");
            if (p_data->conn.server_if == p_cb->server_if)
            {
                if (!btapp_linkloss_dealloc(p_data->conn.conn_id))
                {
                    APPL_TRACE_ERROR0 ("btapp_linkloss_dealloc: no_resource");
                }
            }
            else
            {
                APPL_TRACE_ERROR1 ("Ignore BTA_GATTS_DISCONN_EVT wrong sever_if=%d",p_data->conn.server_if);
            }
            break;
        default:
            break;
    }
}
//#endif
#endif

