/****************************************************************************
**
**  Name:          btapp_gatt_ibeacon.c
**
**  Description:   Contains GATT ibeacon service application.
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "bta_platform.h"
#include "bte_glue.h"

#define BTAPP_IBEACON_ADV_INT_MIN       0x20    //625us * 0x20
#define BTAPP_IBEACON_ADV_INT_MAX       0x40    //625us * 0x20

char app_ble_ibeacon_write_data[APP_BLE_IBEACON_CLIENT_INFO_DATA_LEN] = "https://www.youtube.com/user/BroadcomCorporation";
char app_ble_ibeacon_received_data[APP_BLE_IBEACON_CLIENT_INFO_DATA_LEN]; //Todo: This received data/URL needs to be passed to STB

UINT8 ibeacon_adv_data[APP_BLE_IBEACON_MAX_ADV_DATA_LEN] =
                          {0x4C, 0x00, 0x02, 0x15, 0xe2, 0xc5, 0x6d, 0xb5, 0xdf,
                           0xfb, 0x48, 0xd2, 0xb0, 0x60, 0xd0, 0xf5, 0xa7, 0x10,
                           0x96, 0xe0, 0x00, 0x00, 0x00, 0x00, 0xC5};

/* ibeacon main control block */
tBTAPP_IBEACON_PERIPHERAL_CB btapp_ibeacon_cb;
static tBT_UUID btapp_ibeacon_app_uuid = {2, {APP_BLE_IBEACON_APP_IBEACON_UUID}};
static tBT_UUID btapp_ibeacon_service_uuid = {2, {APP_BLE_IBEACON_APP_SERV_UUID}};

/* characteristics UUID */
static tBT_UUID    btapp_ibeacon_char_uuid = {2, {APP_BLE_IBEACON_APP_CHAR_UUID}};

static tBTA_BLE_MANU* p_ble_manu_buf = NULL;
static UINT8*         p_ble_manu_val = NULL;

static void btapp_ibeacon_cback(tBTA_GATTS_EVT event,  tBTA_GATTS *p_data);

void btapp_ble_ibeacon_start_ibeacon(void)
{
    BTA_GATTS_AppRegister(&btapp_ibeacon_app_uuid, btapp_ibeacon_cback);
}

void btapp_build_ibeacon_service(UINT8 inst_id)
{
    tBTAPP_IBEACON_PERIPHERAL_CB   *p_cb = &btapp_ibeacon_cb;
    BRCM_PLATFORM_TRACE("%s inst_id=%d service_id= %d"LINE_ENDING,__func__,inst_id,p_cb->srvc_inst[inst_id].service_id);

    BTA_GATTS_AddCharacteristic (p_cb->srvc_inst[inst_id].service_id,
                                 &btapp_ibeacon_char_uuid,
                                 BTAPP_IBEACON_CFG_PERM,
                                 BTA_GATT_CHAR_PROP_BIT_WRITE);

    p_cb->srvc_inst[inst_id].adjust_reason = BTAPP_IBEACON_DEF_ADJUST_DEF;

    BTA_GATTS_StartService(p_cb->srvc_inst[inst_id].service_id, BTA_GATT_TRANSPORT_LE);

    return;
}

/*******************************************************************************
**
** Function         btapp_ibeacon_add_char_cmpl
**
** Description      process add characteristic complete.
**
** Returns          void
**
*******************************************************************************/
void btapp_ibeacon_add_char_cmpl(tBTAPP_IBEACON_PERIPHERAL_CB *p_cb, UINT16 service_id,
                                        UINT16 attr_id, BOOLEAN is_char)
{
    UINT8   i;

    BRCM_PLATFORM_TRACE("btapp_ibeacon_add_char_cmpl service_id= %d"LINE_ENDING, service_id);

    for (i = 0; i < BTAPP_IBEACON_INST_MAX; i ++)
    {
        if (p_cb->srvc_inst[i].service_id == service_id)
        {
            BRCM_PLATFORM_TRACE("btapp_ibeacon_add_char_cmpl index=%d service_id= %d"LINE_ENDING, i, service_id);
            break;
        }
    }
    if (i != BTAPP_IBEACON_INST_MAX && attr_id != 0)
    {
        if (is_char)
            p_cb->srvc_inst[i].ibeacon_id = attr_id;
        else
            p_cb->srvc_inst[i].clt_cfg_id = attr_id;

        BRCM_PLATFORM_TRACE("add char/descr ID = 0x%02x"LINE_ENDING, attr_id);
    }
    else
    {
        if (attr_id == 0)
        {
            BRCM_PLATFORM_TRACE("add char failed"LINE_ENDING);
        }
        else
        {
            BRCM_PLATFORM_TRACE("no matching service found"LINE_ENDING);
        }
    }

}

/*******************************************************************************
**
** Function         btapp_ibeacon_cback
**
** Description      ibeacon service callback.
**
** Returns          void
**
*******************************************************************************/
static void btapp_ibeacon_cback(tBTA_GATTS_EVT event,  tBTA_GATTS *p_data)
{
    tBTAPP_IBEACON_PERIPHERAL_CB   *p_cb = &btapp_ibeacon_cb;
    BOOLEAN is_char;
    tBTA_GATTS_RSP resp;
    tBTA_GATT_STATUS status;

    BRCM_PLATFORM_TRACE("%s event = %d "LINE_ENDING,__func__ ,event);

    switch (event)
    {
        case BTA_GATTS_REG_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_REG_EVT server_if= %d status=%d"LINE_ENDING,
                              __func__,p_data->reg_oper.server_if, p_data->reg_oper.status);
            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                p_cb->enabled = TRUE;
                p_cb->server_if = p_data->reg_oper.server_if;

                BTA_GATTS_CreateService(btapp_ibeacon_cb.server_if,
                                        &btapp_ibeacon_service_uuid,
                                        0,      /* theretical only one instance per device */
                                        BTAPP_IBEACON_HANDLE_NUM,
                                        TRUE);  /* always create as primary service */
            }
            break;

        case BTA_GATTS_DEREG_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_DEREG_EVT server_if= %d status=%d"LINE_ENDING,
                             __func__,p_data->reg_oper.server_if, p_data->reg_oper.status);
            if (p_data->reg_oper.status == BTA_GATT_OK)
            {
                p_cb->enabled = FALSE;
            }

            break;

        case BTA_GATTS_CREATE_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_CREATE_EVT status= %d"LINE_ENDING, __func__, p_data->create.status);

            if (p_data->create.status == BTA_GATT_OK)
            {
                p_cb->srvc_inst[0].service_id = p_data->create.service_id;
                /* add characteristic to this service */
                btapp_build_ibeacon_service(0);
            }
            break;

        case BTA_GATTS_OPEN_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_OPEN_EVT status=%d"LINE_ENDING, __func__, p_data->status);

            break;

        case BTA_GATTS_CLOSE_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_CLOSE_EVT status=%d"LINE_ENDING, __func__, p_data->status);

            break;

        case BTA_GATTS_ADD_CHAR_EVT:
        case BTA_GATTS_ADD_CHAR_DESCR_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_ADD_CHAR_EVT status = 0x%02x"LINE_ENDING,__func__,p_data->add_result.status);
            BRCM_PLATFORM_TRACE("BTA_GATTS_ADD_CHAR_DESCR_EVT 0x%x 0x%x "LINE_ENDING,
            p_data->add_result.service_id,p_data->add_result.attr_id);

            is_char = (event == BTA_GATTS_ADD_CHAR_EVT) ? TRUE: FALSE;
            if (p_data->add_result.status == BTA_GATT_OK){
                 btapp_ibeacon_add_char_cmpl(p_cb,
                                             p_data->add_result.service_id,
                                             p_data->add_result.attr_id,
                                             is_char);
            }
            break;

        case BTA_GATTS_DELELTE_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_DELELTE_EVT server_if=%d service_id=%d status=%d"LINE_ENDING,
                              __func__, p_data->srvc_oper.server_if, p_data->srvc_oper.service_id, p_data->srvc_oper.status);
            if(p_data->srvc_oper.status == BTA_GATT_OK)
            {
                BTA_GATTS_AppDeregister(p_data->srvc_oper.server_if);
            }

            break;

        case BTA_GATTS_START_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_START_EVT status = 0x%d"LINE_ENDING,__func__,p_data->srvc_oper.status);

            break;

        case BTA_GATTS_READ_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_READ_EVT "LINE_ENDING,__func__);
            break;

        case BTA_GATTS_WRITE_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_WRITE_EVT "LINE_ENDING,__func__);
            BRCM_PLATFORM_TRACE("Peer write len:%d offset:%d, content:%s need_rsp:%d is_prep:%d"LINE_ENDING, p_data->req_data.p_data->write_req.len,
                                                                                         p_data->req_data.p_data->write_req.offset,
                                                                                         p_data->req_data.p_data->write_req.value,
                                                                                         p_data->req_data.p_data->write_req.need_rsp,
                                                                                         p_data->req_data.p_data->write_req.is_prep);
            if(p_data->req_data.p_data->write_req.need_rsp)
            {
                resp.handle = p_data->req_data.p_data->write_req.handle;
                status = BTA_GATT_OK;
            }
            if(p_data->req_data.p_data->write_req.is_prep)
            {
                resp.handle = p_data->req_data.p_data->handle;
                status = BTA_GATT_NOT_LONG;
            }

            BTA_GATTS_SendRsp(p_data->req_data.conn_id, p_data->req_data.trans_id, status, &resp);
            break;

        case BTA_GATTS_EXEC_WRITE_EVT:
            BRCM_PLATFORM_TRACE("%s BTA_GATTS_EXEC_WRITE_EVT "LINE_ENDING,__func__);

            if(p_data->req_data.p_data->exec_write == GATT_PREP_WRITE_EXEC)
            {
                BRCM_PLATFORM_TRACE("GATT_PREP_WRITE_EXEC"LINE_ENDING);
            }
            else if(p_data->req_data.p_data->exec_write == GATT_PREP_WRITE_CANCEL)
            {
                BRCM_PLATFORM_TRACE("GATT_PREP_WRITE_CANCEL"LINE_ENDING);
            }

            resp.handle = p_data->req_data.p_data->handle;
            BTA_GATTS_SendRsp(p_data->req_data.conn_id, p_data->req_data.trans_id, BTA_GATT_OK, &resp);
            break;

        case BTA_GATTS_CONNECT_EVT:
            if(!btapp_gatt_is_gattc_type(p_data->conn.remote_bda))
            {
                BRCM_PLATFORM_TRACE("%s BTA_GATTS_CONNECT_EVT server_if:%d remote_addr:%s conn_id:%d reason:%d transport:%d"LINE_ENDING, __func__,
                                    p_data->conn.server_if,
                                    utl_bd_addr_to_string(p_data->conn.remote_bda),
                                    p_data->conn.conn_id,
                                    p_data->conn.reason,
                                    p_data->conn.transport);
                if(btapp_ibeacon_cb.clcb.in_use != TRUE)
                {
                    btapp_ibeacon_cb.clcb.in_use = TRUE;
                    btapp_ibeacon_cb.clcb.connected = TRUE;
                    btapp_ibeacon_cb.clcb.conn_id = p_data->conn.conn_id;
                    bdcpy(btapp_ibeacon_cb.clcb.bda, p_data->conn.remote_bda);

                    //stop discovery
                    btapp_ble_ibeacon_stop_adv();
                }
            }
            else
            {
                BRCM_PLATFORM_TRACE("%s recv one gatt client device connected event!!!"LINE_ENDING, __func__);
            }
            break;

        case BTA_GATTS_DISCONNECT_EVT:
            if(!btapp_gatt_is_gattc_type(p_data->conn.remote_bda))
            {
                BRCM_PLATFORM_TRACE("%s BTA_GATTS_DISCONNECT_EVT server_if:%d remote_addr:%s conn_id:%d reason:%d transport:%d"LINE_ENDING, __func__,
                                    p_data->conn.server_if,
                                    utl_bd_addr_to_string(p_data->conn.remote_bda),
                                    p_data->conn.conn_id,
                                    p_data->conn.reason,
                                    p_data->conn.transport);
                if(btapp_ibeacon_cb.clcb.in_use)
                {
                    btapp_ibeacon_cb.clcb.in_use = FALSE;
                    btapp_ibeacon_cb.clcb.connected = FALSE;
                    btapp_ibeacon_cb.clcb.conn_id = 0;
                    bdcpy(btapp_ibeacon_cb.clcb.bda, bd_addr_null);
                }

                if(btapp_ibeacon_cb.need_dereg)
                {
                    btapp_ibeacon_cb.need_dereg = FALSE;
                    BTA_GATTS_DeleteService(btapp_ibeacon_cb.srvc_inst[0].service_id);
                    BTA_GATTS_AppDeregister(btapp_gatts_cb.serv_inst[0].service_id);
                }
                else
                {
                    //start discovery
                    btapp_ble_ibeacon_start_adv(0);
                }
            }
            else
            {
                BRCM_PLATFORM_TRACE("%s recv one gatt client device disconnected event!!!"LINE_ENDING,  __func__);
            }
            break;

        case BTA_GATTS_MTU_EVT:
            BRCM_PLATFORM_TRACE("%s, TODO: BTA_GATTS_MTU_EVT >>>"LINE_ENDING, __func__);
            break;

        case BTA_GATTS_CONF_EVT:
            /* Handle of Indication confirm. */
            BRCM_PLATFORM_TRACE("%s, TODO: BTA_GATTS_CONF_EVT, remote_bda[0]: 0x%02X ,transid: %d, conn_id: %d"LINE_ENDING, __func__,
                                                                                 p_data->req_data.remote_bda[0],
                                                                                 p_data->req_data.trans_id,
                                                                                 p_data->req_data.conn_id);
            break;

        default:
            break;
    }
}

/*******************************************************************************
 **
 ** Function        btapp_ble_ibeacon_adv_config_cb
 **
 ** Description     This callback is for BTA_DmBleSetAdvConfig complete callback,
 **                 if the upper layer use GKI_getbuf to acquire one buffer, need to
 **                 free in this callback.
 **
 ** Parameters      status: BTA operate the coresponding action status.
 **
 ** Returns
 **
 *******************************************************************************/
void btapp_ble_ibeacon_adv_config_cb(tBTA_STATUS status)
{
    if(status != BTA_SUCCESS)
    {
       BRCM_PLATFORM_TRACE("ibeacon: ble adv config not success, status:%d "LINE_ENDING, status);
    }

    if(p_ble_manu_buf != NULL)
    {
        GKI_freebuf(p_ble_manu_buf);
    }

    if(p_ble_manu_val != NULL)
    {
        GKI_freebuf(p_ble_manu_val);
    }
}

/*******************************************************************************
 **
 ** Function        btapp_ble_ibeacon_scan_rsp_config_cb
 **
 ** Description     This callback is for BTA_DmBleSetScanRspConfig complete callback,
 **                 if the upper layer use GKI_getbuf to acquire one buffer, need to
 **                 free in this callback.
 **
 ** Parameters      status: BTA operate the coresponding action status.
 **
 ** Returns
 **
 *******************************************************************************/
void btapp_ble_ibeacon_scan_rsp_config_cb(tBTA_STATUS status)
{

}

/*******************************************************************************
 **
 ** Function        btapp_ble_ibeacon_start_adv
 **
 ** Description     start ibeacon advertisement
 **
 ** Parameters      inst_id : instance id for multi adv
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
void btapp_ble_ibeacon_start_adv(UINT8 inst_id)
{
    tBTA_BLE_AD_MASK  adv_data_mask;
    tBTA_BLE_ADV_DATA adv_cfg_data;
    tBTA_BLE_ADV_DATA scan_rsp_data;
    UINT8 flag_value = 0x1a;
    int i=0, j=0;

    /* Below Ibeacon format is refered from the WEB example*/
    UINT8 companyIdentifier[2] = {0x4C,0x00};
    UINT8 ibeacon_adv_ind[2]={0x02, 0x15};
    UINT8 ibeacon_uuid[16]={0xe2, 0xc5, 0x6d, 0xb5, 0xdf, 0xfb, 0x48, 0xd2, 0xb0, 0x60, 0xd0, 0xf5, 0xa7, 0x10, 0x96, 0xe0};
    UINT8 major_num[2] = {0x00, 0x00};
    UINT8 minor_num[2] = {0x00, 0x00};
    UINT8 tx_pwr_2s_compl = 0xC5;
    int len = APP_BLE_IBEACON_MAX_ADV_DATA_LEN;
    adv_data_mask = BTA_BLE_AD_BIT_FLAGS | BTA_BLE_AD_BIT_MANU;

    BRCM_PLATFORM_TRACE("ibeacon : single ADV inst id=0 "LINE_ENDING);

    /* set adv data*/
    if (adv_data_mask & BTA_BLE_AD_BIT_FLAGS)
    {
        adv_cfg_data.flag = flag_value;
    }

    if (adv_data_mask & BTA_BLE_AD_BIT_MANU)
    {
        p_ble_manu_buf = (tBTA_BLE_MANU*)GKI_getbuf(sizeof(tBTA_BLE_MANU));
        p_ble_manu_val = (UINT8*)GKI_getbuf(sizeof(UINT8)*(APP_BLE_IBEACON_MAX_ADV_DATA_LEN));

        adv_cfg_data.p_manu = p_ble_manu_buf;
        adv_cfg_data.p_manu->len = len;
        adv_cfg_data.p_manu->p_val= p_ble_manu_val;

        adv_cfg_data.p_manu->p_val[i++] = companyIdentifier[0];
        adv_cfg_data.p_manu->p_val[i++] = companyIdentifier[1];
        adv_cfg_data.p_manu->p_val[i++] = ibeacon_adv_ind[0];
        adv_cfg_data.p_manu->p_val[i++] = ibeacon_adv_ind[1];

        for (j = 0; j < sizeof(ibeacon_uuid); j++)
        {
           adv_cfg_data.p_manu->p_val[i++] = ibeacon_uuid[j];
        }
        adv_cfg_data.p_manu->p_val[i++] = major_num[0];
        adv_cfg_data.p_manu->p_val[i++] = major_num[1];
        adv_cfg_data.p_manu->p_val[i++] = minor_num[0];
        adv_cfg_data.p_manu->p_val[i++] = minor_num[1];
        adv_cfg_data.p_manu->p_val[i++] = tx_pwr_2s_compl;
    }

    BTA_DmBleSetAdvConfig(adv_data_mask, &adv_cfg_data, btapp_ble_ibeacon_adv_config_cb);

    /* set scan response data*/
    adv_data_mask = BTA_BLE_AD_BIT_DEV_NAME;

    /* since set the device name field, the scan_rsp_data don't assign dedicated value;
       if set others mask, need assign meaningful value to scan_rsp_data, if use the GKI_getbuf,
       need free in the tBTA_SET_ADV_DATA_CMPL_CBACK */
    BTA_DmBleSetScanRspConfig(adv_data_mask, &scan_rsp_data, NULL);

    BTA_DmSetBleAdvParams(BTAPP_IBEACON_ADV_INT_MIN, BTAPP_IBEACON_ADV_INT_MAX, NULL);

    BTA_DmSetBleVisibility(TRUE);
}

/*******************************************************************************
 **
 ** Function        app_ble_ibeacon_stop_ibeacon_adv
 **
 ** Description     stop ibeacon advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
void btapp_ble_ibeacon_stop_adv(void)
{
    BTA_DmSetBleVisibility(FALSE);
}

/*******************************************************************************
 **
 ** Function        btapp_ble_ibeacon_deregister
 **
 ** Description     Deregister server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static void btapp_ble_ibeacon_deregister(void)
{
	BRCM_PLATFORM_TRACE("btapp_ble_ibeacon_deregister "LINE_ENDING);

    if(btapp_ibeacon_cb.clcb.connected)
    {//If current connected with others, we need close the gatt server first.
        btapp_ibeacon_cb.need_dereg = TRUE;
        BTA_GATTS_Close(btapp_ibeacon_cb.clcb.conn_id);
    }
    else
    {
        BTA_GATTS_DeleteService(btapp_ibeacon_cb.srvc_inst[0].service_id);
        BTA_GATTS_AppDeregister(btapp_gatts_cb.serv_inst[0].service_id);
    }
}

/*******************************************************************************
 **
 ** Function        app_ble_ibeacon_stop_ibeacon
 **
 ** Description     stop ibeacon
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
void btapp_ble_ibeacon_stop_ibeacon(void)
{
    btapp_ble_ibeacon_deregister();
}

