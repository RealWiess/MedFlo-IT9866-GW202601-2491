/****************************************************************************
**                                                                           
**  Name:          btapp_tx_power.c                                           
**                 
**  Description:   Contains btui functions for Link Loss service
**                 
**                                                                           
**  Copyright (c) 2003-2010, Broadcom Corp, All Rights Reserved.              
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
******************************************************************************/

#include "bt_target.h"

//#if( defined BTA_TX_POWER_INCLUDED ) && (BTA_TX_POWER_INCLUDED == TRUE)
#if BLE_INCLUDED == TRUE && BTA_GATT_INCLUDED == TRUE


#include "bta_api.h"
//#include "btui.h"
//#include "btui_int.h"
#include "btapp_tx_power.h"

#define BTUI_GATT_INVALID_PHY_CONN  0x80


/* BTUI Link Loss main control block */
tBTUI_TX_POWER_CB btui_tx_power_cb = {0};

static tBT_UUID    btui_tx_power_app_uuid = {2, {BTUI_TX_POWER_APP_UUID}};
static tBT_UUID    btui_tx_power_service_uuid = {2, {UUID_SERVCLASS_TX_POWER}};
static tBT_UUID    btui_tx_power_level_uuid = {2, {GATT_UUID_TX_POWER_LEVEL}};

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_tx_power_cback(tBTA_GATTS_EVT event,   tBTA_GATTS *p_data);

/*******************************************************************************
**
** Function         btapp_tx_power_clcb_alloc
**
** Description      The function allocates a   connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
tBTUI_TX_POWER_CLCB *btapp_tx_power_clcb_alloc (UINT16 conn_id, BD_ADDR bda)
{
    UINT8         i_clcb = 0;
    tBTUI_TX_POWER_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btui_tx_power_cb.clcb; i_clcb < BTUI_TX_POWER_MAX_CL; i_clcb++, p_clcb++)
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
** Function         btapp_tx_power_dealloc
**
** Description      The function deallocates a connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
BOOLEAN btapp_tx_power_dealloc (UINT16 conn_id)
{
    UINT8         i_clcb = 0;
    tBTUI_TX_POWER_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btui_tx_power_cb.clcb; i_clcb < BTUI_TX_POWER_MAX_CL; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && (p_clcb->conn_id == conn_id))
        {
            memset(p_clcb, 0, sizeof(tBTUI_TX_POWER_CLCB));
            return TRUE;
        }
    }
    return FALSE;
}



/*******************************************************************************
**
** Function         btapp_tx_power_init
**
** Description      Initializes Link Loss Service Application
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_init(void)
{
    if (!btui_tx_power_cb.enabled)
    {
        memset(&btui_tx_power_cb, 0, sizeof(tBTUI_TX_POWER_CB));

        BTA_GATTS_AppRegister(&btui_tx_power_app_uuid, btapp_tx_power_cback);
    }
}

/*******************************************************************************
**
** Function         btapp_tx_power_instatiate
**
** Description      Instatiate a TX power Service
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_instatiate(tBTA_GATT_CHAR_PRES *p_pre_fmt, BOOLEAN notify_spt,
                               BOOLEAN is_pri,
                               tBTUI_TX_PWR_CREATE_CBACK *p_create_cback)
{
    APPL_TRACE_DEBUG0("btapp_tx_power_instatiate");
    if (btui_tx_power_cb.p_create_cback == NULL)
    {
        APPL_TRACE_DEBUG0("create tx_power service ");
        btui_tx_power_cb.p_create_cback = p_create_cback;
        btui_tx_power_cb.notify_spt = notify_spt;
        memcpy(&btui_tx_power_cb.pre_fmt, p_pre_fmt, sizeof(tBTA_GATT_CHAR_PRES));

        BTA_GATTS_CreateService(btui_tx_power_cb.server_if, 
                                &btui_tx_power_service_uuid,
                                btui_tx_power_cb.inst_id, 
                                BTUI_TX_POWER_HANDLE_NUM,
                                is_pri);  /* always create as primary service */
    }
}

/*******************************************************************************
**
** Function         btapp_tx_power_disable
**
** Description      disable a Link Loss Service
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_disable(UINT8 inst_id)
{
    BTA_GATTS_StopService(btui_tx_power_cb.srvc_inst[inst_id].service_id);  
    BTA_GATTS_DeleteService(btui_tx_power_cb.srvc_inst[inst_id].service_id);
}
/*******************************************************************************
**
** Function         btapp_create_service_cback
**
** Description      create service callback
**                  
** Returns          void
**
*******************************************************************************/
void btapp_create_service_cback(UINT8 inst_id, UINT8 status)
{
    tBTUI_TX_POWER_CB   *p_cb = &btui_tx_power_cb;
    tBTUI_TX_PWR_CREATE_CBACK   *p_cback = p_cb->p_create_cback;
    UINT8       cb_inst_id = inst_id;

    p_cb->p_create_cback = NULL;

    if (status != BTA_GATT_OK)
    {
        cb_inst_id = BTUI_TX_PWR_INST_INVALID;

        if (inst_id < BTUI_TX_POWER_INST_MAX)
        {
            if (p_cb->srvc_inst[inst_id].service_id != 0)
                BTA_GATTS_DeleteService(p_cb->srvc_inst[inst_id].service_id);
            memset(&p_cb->srvc_inst[inst_id], 0, sizeof(tBTUI_TX_POWER_INTS));
        }
    }
    /* listen for all */
    BTA_GATTS_Listen(btui_tx_power_cb.server_if, TRUE, NULL);

    if (p_cback)
        (*p_cback)(cb_inst_id, status);

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
void btapp_create_service_cmpl(tBTA_GATT_STATUS status, UINT16 service_id, UINT8 inst_id)
{
    tBTUI_TX_POWER_CB   *p_cb = &btui_tx_power_cb;
    UINT8               prop = BTUI_TX_PWR_LEVEL_PROP_DEF;

    APPL_TRACE_DEBUG1("btapp_create_service_cmpl STATUS = %d", status);

    if (status == BTA_GATT_OK)
    {
        if (p_cb->notify_spt)
            prop |= BTA_GATT_CHAR_PROP_BIT_NOTIFY;

        p_cb->srvc_inst[inst_id].service_id = service_id;

        BTA_GATTS_AddCharacteristic (p_cb->srvc_inst[inst_id].service_id, 
                                     &btui_tx_power_level_uuid,
                                     BTUI_TX_PWR_LEVEL_PERM,
                                     prop);

    }
    else
    {
        APPL_TRACE_DEBUG0("Initatiate TX poer service failed.");
        btapp_create_service_cback(BTUI_TX_PWR_INST_INVALID, status);
    }
    return;
}
/*******************************************************************************
**
** Function         btapp_tx_power_add_char_cmpl
**
** Description      process add characteristic complete.
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_add_char_cmpl(tBTUI_TX_POWER_CB *p_cb, UINT16 service_id, UINT16 attr_id)
{
    UINT8   i;
    tBT_UUID            descr_uuid = {2, {GATT_UUID_CHAR_CLIENT_CONFIG}};

    for (i = 0; i < BTUI_TX_POWER_INST_MAX; i ++)
    {
        if (p_cb->srvc_inst[i].service_id == service_id)
            break;
    }
    if (i != BTUI_TX_POWER_INST_MAX && attr_id != 0)
    {
        p_cb->srvc_inst[i].tx_pwr_id = attr_id;
        p_cb->srvc_inst[i].tx_pwr_level = BTUI_TX_PWR_VALUE_DEF;
        APPL_TRACE_DEBUG1("add tx power char ID = 0x%04x", attr_id);

        p_cb->descr_step = 1;

        BTA_GATTS_AddCharDescriptor (p_cb->srvc_inst[i].service_id, 
                                     BTA_GATT_PERM_READ|BTA_GATT_PERM_WRITE,
                                     &descr_uuid);

    }
    else
    {
        if (attr_id == 0)
        {
            APPL_TRACE_ERROR0("add char failed");
        }
        else
        {
            APPL_TRACE_DEBUG0("no matching service found");
        }
        btapp_create_service_cback(i, BTA_GATT_ERROR);
    }

}
/*******************************************************************************
**
** Function         btapp_tx_power_add_char_descr_cmpl
**
** Description      add descriptor complete
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_add_char_descr_cmpl(tBTUI_TX_POWER_CB *p_cb, UINT16 service_id, UINT16 attr_id)
{
    UINT8   i;
    tBT_UUID            descr_uuid = {2, {GATT_UUID_CHAR_PRESENT_FORMAT}};

    for (i = 0; i < BTUI_TX_POWER_INST_MAX; i ++)
    {
        if (p_cb->srvc_inst[i].service_id == service_id)
            break;
    }
    if (i != BTUI_TX_POWER_INST_MAX && attr_id != 0)
    {
        if (p_cb->descr_step == 1)
            p_cb->srvc_inst[i].config_id = attr_id;
        if (p_cb->descr_step == 2)
            p_cb->srvc_inst[i].pre_fmt_id = attr_id;

        if (p_cb->descr_step == 1 && p_cb->notify_spt)
        {
            p_cb->descr_step = 2;
            BTA_GATTS_AddCharDescriptor (p_cb->srvc_inst[i].service_id, 
                                         (BTA_GATT_PERM_READ|BTA_GATT_PERM_WRITE),
                                         &descr_uuid);
        }
        else
        {
            p_cb->descr_step = 0;
            BTA_GATTS_StartService(p_cb->srvc_inst[i].service_id, BTA_GATT_TRANSPORT_LE_BR_EDR);
        }
    }
    else
    {
        APPL_TRACE_ERROR1("add descr (step %d) failed", p_cb->descr_step);
        btapp_create_service_cback(i, BTA_GATT_ERROR);
    }
}
/*******************************************************************************
**
** Function         btapp_tx_power_start_cmpl
**
** Description      start service complete.
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_start_cmpl(UINT16 service_id, UINT8 status)
{
    UINT8   i;
    tBTUI_TX_POWER_CB   *p_cb = &btui_tx_power_cb;
    APPL_TRACE_DEBUG2("btapp_tx_power_start_cmpl service_id=%d status =%d ", service_id, status);

    for (i = 0; i < BTUI_TX_POWER_INST_MAX; i ++)
    {
        if (p_cb->srvc_inst[i].service_id == service_id)
            break;
    }
    btapp_create_service_cback(i, status);
}
/*******************************************************************************
**
** Function         btapp_tx_power_srvc_cback
**
** Description      read tx power level complete
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_cmpl(void *p)
{
    tBTM_TX_POWER_RESULTS *p_result = (tBTM_TX_POWER_RESULTS *)p;
    tBTUI_TX_POWER_CB   *p_cb = &btui_tx_power_cb;
    UINT8               i = (p_cb->pending_tx_read_inst) ? p_cb->pending_tx_read_inst - 1 : 0;
    tBTA_GATTS_RSP      rsp, *p_rsp = NULL;
    UINT8           *pp = rsp.attr_value.value;
    tBTA_GATT_STATUS    status = BTUI_GATT_INVALID_PHY_CONN;

    APPL_TRACE_DEBUG1("btapp_tx_power_cmpl pending_inst=%d(one base)", p_cb->pending_tx_read_inst);

    if (p_cb->pending_tx_read_inst == 0) /* no pending read */
        return;

    p_cb->pending_tx_read_inst = 0;

    if (p_result != NULL && p_result->status == BTM_SUCCESS)
    {

        status = BTA_GATT_OK;
        p_rsp = &rsp;
        rsp.attr_value.handle = p_cb->srvc_inst[i].tx_pwr_id;
        rsp.attr_value.len = sizeof(UINT8);
        UINT8_TO_STREAM(pp, p_result->tx_power);
        APPL_TRACE_DEBUG1("btapp_tx_power_cmpl result OK tx_power=%d", p_result->tx_power);

    }
    else
    {
        APPL_TRACE_ERROR0("btapp_tx_power_cmpl result FAILED");
    }

    BTA_GATTS_SendRsp(p_cb->srvc_inst[i].conn_id,
                      p_cb->srvc_inst[i].trans_id,
                      status,
                      p_rsp);


}
/*******************************************************************************
**
** Function         btapp_tx_power_proc_read
**
** Description      process a read alert level attribute request
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_proc_read(tBTUI_TX_POWER_CB *p_cb,  tBTA_GATTS_REQ *p_req_data)
{
    tBTA_GATT_STATUS    status = BTA_GATT_OK;
    tBTA_GATTS_RSP      rsp, *p_rsp = &rsp;
    tBTA_GATT_VALUE     *p_value = &rsp.attr_value;
    UINT8       i, j, *pp = p_value->value;

    for (i = 0; i < BTUI_TX_POWER_INST_MAX; i ++)
    {
        if ((p_cb->srvc_inst[i].tx_pwr_id == p_req_data->p_data->read_req.handle) ||
            (p_cb->srvc_inst[i].config_id == p_req_data->p_data->read_req.handle))
            break;
    }
    if (i != BTUI_TX_POWER_INST_MAX)
    {
        if (p_req_data->p_data->read_req.is_long)
        {
            status = BTA_GATT_NOT_LONG;
            p_rsp = NULL;
        }
#if 0
        /* needed  special firmware */
        else if (p_cb->srvc_inst[i].tx_pwr_id == p_req_data->p_data->read_req.handle)
        {
            /* for now, only support one read at a time */
            if (p_cb->pending_tx_read_inst == 0)
            {
                p_cb->pending_tx_read_inst = i + 1;
                p_cb->srvc_inst[i].trans_id = p_req_data->trans_id;
                p_cb->srvc_inst[i].conn_id = p_req_data->conn_id;
                BTM_ReadTxPower(p_req_data->remote_bda, TRUE, btapp_tx_power_cmpl);
                return;
            }
            else
            {
                APPL_TRACE_ERROR0("one pending read, request rejected");
                status = BTA_GATT_ERROR;
                p_rsp = NULL;
            }
        }
#else
        else if (p_cb->srvc_inst[i].tx_pwr_id == p_req_data->p_data->read_req.handle)
        {
            status = BTA_GATT_OK;
            p_rsp = &rsp;
            rsp.attr_value.handle = p_cb->srvc_inst[i].tx_pwr_id;
            rsp.attr_value.len = sizeof(UINT8);
            rsp.attr_value.value[0] = 0x04 ; //4dBm

            BTA_GATTS_SendRsp(p_req_data->conn_id,
                              p_req_data->trans_id,
                              status,
                              p_rsp);
        }
#endif
        /* suppose never happen 
        else if (p_cb->srvc_inst[i].pre_fmt_id == p_req_data->p_data->read_req.handle)
        {
        }*/
        else if (p_cb->srvc_inst[i].config_id == p_req_data->p_data->read_req.handle)
        {
            p_value->handle = p_cb->srvc_inst[i].config_id;
            for (j = 0; j < BTUI_TX_MAX_CLIENT_NUM; j ++)
            {
                if (!memcmp(p_cb->srvc_inst[i].client_cfg[j].client_bda, p_req_data->remote_bda, BD_ADDR_LEN))
                {
                    p_value->len = sizeof(tBTA_GATT_CLT_CHAR_CONFIG);
                    UINT8_TO_STREAM(pp, p_cb->srvc_inst[i].tx_pwr_level);
                    break;
                }
                else
                    APPL_TRACE_ERROR0("un-find bda\r\n");
            }
            if (j == BTUI_TX_MAX_CLIENT_NUM)
            {
                APPL_TRACE_ERROR0("no client configuration found for this device");
                status = BTA_GATT_ERR_UNLIKELY;
                p_rsp = NULL;
            }
        }
        else
        {
            status = BTA_GATT_INVALID_HANDLE;
            p_rsp = NULL;
        }

        BTA_GATTS_SendRsp(p_req_data->conn_id, 
                          p_req_data->trans_id,
                          status,
                          p_rsp);

    }
}
/*******************************************************************************
**
** Function         btapp_tx_power_proc_write
**
** Description      process a write alert level attribute request
**                  
** Returns          void
**
*******************************************************************************/
void btapp_tx_power_proc_write(tBTUI_TX_POWER_CB *p_cb,  tBTA_GATTS_REQ *p_req_data)
{
    tBTA_GATT_WRITE_REQ *p_write_req = &p_req_data->p_data->write_req;
    tBTA_GATT_STATUS    status = BTA_GATT_INVALID_HANDLE;
    UINT8               i, j;
    UINT8               *p;

    for (i = 0; i < BTUI_TX_POWER_INST_MAX; i ++)
    {
        if ((p_cb->srvc_inst[i].tx_pwr_id == p_write_req->handle) ||
            (p_cb->srvc_inst[i].config_id == p_write_req->handle) ||
            (p_cb->srvc_inst[i].pre_fmt_id == p_write_req->handle))
            break;
    }
    if (i != BTUI_TX_POWER_INST_MAX)
    {
        if (p_cb->srvc_inst[i].config_id == p_write_req->handle)
        {
            /* do not support prepare write for clinet configuration for now */
            if (p_write_req->is_prep)
            {
                status = BTA_GATT_REQ_NOT_SUPPORTED;
            }
            else
            {
                if (p_write_req->len != sizeof(tBTA_GATT_CLT_CHAR_CONFIG))
                    status = BTA_GATT_INVALID_ATTR_LEN;
                else
                {
                    for (j= 0; j < BTUI_TX_MAX_CLIENT_NUM; j ++)
                    {
                        if (!p_cb->srvc_inst[i].client_cfg[j].in_use ||
                            !memcmp(p_cb->srvc_inst[i].client_cfg[j].client_bda, p_req_data->remote_bda, BD_ADDR_LEN))
                            break;
                    }
                    if (j != BTUI_TX_MAX_CLIENT_NUM )
                    {
                        p_cb->srvc_inst[i].client_cfg[j].in_use = TRUE;
                        status = BTA_GATT_OK;
                        //p_cb->srvc_inst[i].client_cfg[j].config = p_write_req->value[0];
                        p = p_write_req->value;
                        STREAM_TO_UINT16(p_cb->srvc_inst[i].client_cfg[j].config, p);
                    }
                    else
                    {
                        status = BTA_GATT_ERR_UNLIKELY;
                        APPL_TRACE_ERROR0("max client oonfiguration reached, application error");
                    }
                }
            }
        }
        else
        {
            /* in theroy, not possible
               all other handle does not support write */
            if (p_cb->srvc_inst[i].tx_pwr_id == p_write_req->handle ||
                p_cb->srvc_inst[i].pre_fmt_id == p_write_req->handle)
                status = BTA_GATT_WRITE_NOT_PERMIT;
        }
    }
    if (p_write_req->need_rsp)
    {
        BTA_GATTS_SendRsp(p_req_data->conn_id, 
                          p_req_data->trans_id,
                          status,
                          NULL);
    }

}
/*******************************************************************************
**
** Function         btapp_tx_power_cback
**
** Description      link loss service callback.
**                  
** Returns          void
**
*******************************************************************************/
static void btapp_tx_power_cback(tBTA_GATTS_EVT event,  tBTA_GATTS *p_data)
{
    tBTUI_TX_POWER_CB   *p_cb = &btui_tx_power_cb;

    switch (event)
    {
        case BTA_GATTS_REG_EVT:

            APPL_TRACE_DEBUG2("btapp_tx_power_cback  rcv BTA_GATTS_REG_EVT server_if= %d status=%d", 
                              p_data->reg_oper.server_if, p_data->reg_oper.status );
            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                p_cb->enabled = TRUE;
                p_cb->server_if = p_data->reg_oper.server_if;
            }
            break;

        case BTA_GATTS_CREATE_EVT:
            /* add characteristic to this service */
            APPL_TRACE_DEBUG1("btapp_tx_power_cback  rcv BTA_GATTS_CREATE_EVT status= %d", p_data->create.status);
            btapp_create_service_cmpl(p_data->create.status, p_data->create.service_id, p_cb->inst_id);
            break;

        case BTA_GATTS_ADD_CHAR_EVT:
            APPL_TRACE_DEBUG1("btapp_tx_power_cback rcv BTA_GATTS_ADD_CHAR_EVT status = 0x%d", p_data->srvc_oper.status);
            if (p_data->add_result.status == BTA_GATT_OK)
            {
                btapp_tx_power_add_char_cmpl(p_cb, p_data->add_result.service_id, p_data->add_result.attr_id);
            }
            break;

        case BTA_GATTS_ADD_CHAR_DESCR_EVT:
            if (p_data->add_result.status == BTA_GATT_OK)
            {
                btapp_tx_power_add_char_descr_cmpl(p_cb, p_data->add_result.service_id, p_data->add_result.attr_id);
            }
            break;

        case BTA_GATTS_START_EVT:
            btapp_tx_power_start_cmpl(p_data->srvc_oper.service_id, p_data->srvc_oper.status);
            APPL_TRACE_DEBUG1("service start status = 0x%d", p_data->status);
            break;

        case BTA_GATTS_READ_EVT:
            APPL_TRACE_DEBUG0("btapp_tx_power_cback rcv BTA_GATTS_READ_EVT ");
            btapp_tx_power_proc_read(p_cb, &p_data->req_data);
            break;

        case BTA_GATTS_WRITE_EVT:
            APPL_TRACE_DEBUG0("btapp_tx_power_cback rcv BTA_GATTS_WRITE_EVT ");
            btapp_tx_power_proc_write(p_cb, &p_data->req_data);
            break;
        case BTA_GATTS_CONNECT_EVT:

            if (p_data->conn.server_if == p_cb->server_if)
            {
                if (btapp_tx_power_clcb_alloc(p_data->conn.conn_id, p_data->conn.remote_bda)== NULL)
                {
                    APPL_TRACE_ERROR0 ("btapp_tx_power_clcb_alloc: no_resource");
                }
            }
            else
            {
                APPL_TRACE_ERROR1 ("Ignore BTA_GATTS_CONN_EVT wrong sever_if=%d",p_data->conn.server_if);
            }

            break;
        case BTA_GATTS_DISCONNECT_EVT:

            if (p_data->conn.server_if == p_cb->server_if)
            {
                btapp_tx_power_dealloc(p_data->conn.conn_id);
            }
            else
            {
                APPL_TRACE_ERROR1 ("Ignore BTA_GATTS_DISCONN_EVT wrong sever_if=%d",p_data->conn.server_if);
            }
            break;
    }
}
//#endif
#endif

