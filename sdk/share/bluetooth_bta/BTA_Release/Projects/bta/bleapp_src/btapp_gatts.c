/****************************************************************************
**
**  Name:          btapp_gatts.c
**
**  Description:   Contains btapp functions for GATT server
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/
//#define TEST_LINK
#include "bt_target.h"
#include "string.h"
#if( defined BLE_INCLUDED ) && (BLE_INCLUDED == TRUE)
#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)

#include "bta_platform.h"
#include "bte_glue.h"
#ifdef TEST_LINK
#include "doorbell.h"
#endif

#define BTAPP_GATTS_ADV_INT_MIN       200 //625us *200 = 125ms
#define BTAPP_GATTS_ADV_INT_MAX       300 //625us *300

//If the local gatt server database is complicated, we should add service and chars one by one under synchronous operation.
#define BTAPP_GATTS_APP_UUID            0xAAAA
#define BTAPP_GATTS_SERV_UUID           0xAAA0
#define BTAPP_GATTS_CHAR1_UUID          0xAAA1
#define BTAPP_GATTS_CHAR1_PERM          (BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE)
#define BTAPP_GATTS_CHAR1_PROP          (BTA_GATT_CHAR_PROP_BIT_READ | BTA_GATT_CHAR_PROP_BIT_WRITE | BTA_GATT_CHAR_PROP_BIT_NOTIFY)
#define BTAPP_GATTS_CHAR1_CFG_PERM      (BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE)
#define BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR 0x2902

#define BTAPP_GATTS_CHAR2_UUID          0xAAA2
#define BTAPP_GATTS_CHAR2_PERM          (BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE)
#define BTAPP_GATTS_CHAR2_PROP          (BTA_GATT_CHAR_PROP_BIT_READ)
#define BTAPP_GATTS_CHAR2_CFG_PERM      (BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE)
#define BTAPP_GATTS_CHAR2_CLIENT_DESCRIPTOR 0x2903

#define BTAPP_GATTS_CHAR3_UUID          0xAAA3
#define BTAPP_GATTS_CHAR3_PERM          (BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE)
#define BTAPP_GATTS_CHAR3_PROP          (BTA_GATT_CHAR_PROP_BIT_READ | BTA_GATT_CHAR_PROP_BIT_WRITE | BTA_GATT_CHAR_PROP_BIT_NOTIFY | BTA_GATT_CHAR_PROP_BIT_INDICATE)
#define BTAPP_GATTS_CHAR3_CFG_PERM      (BTA_GATT_PERM_READ | BTA_GATT_PERM_WRITE)
#define BTAPP_GATTS_CHAR3_CLIENT_DESCRIPTOR 0x2904

#define BTAPP_GATTS_CHAR_CFG_NOTIFICATION_MASK  0x0001
#define BTAPP_GATTS_CHAR_CFG_INDICATION_MASK    0x0002

#define BTAPP_GATTS_MAX_CHAR_NUM        12//BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV *BTAPP_GATTS_MAX_SERV, in this case, we just create 1 server
#define BTAPP_GATTS_HANDLE_NUM          ((3*BTAPP_GATTS_MAX_CHAR_NUM)+1)

#define BTAPP_GATTS_MAX_ADV_DATA_LEN    25

/* to sdh tBTA_GATTS_HNDL_RANGE   btapp_gatts_handle_map[BTAPP_GATT_MAX_HANDLE_MAP_SIZE]; */
tBTAPP_GATTS_HNDL_RANGE_DB    btapp_gatts_hndl_range_db;
tBTAPP_GATTS_SRV_CHG_DB       btapp_gatts_srv_chg_db;
tBTAPP_GATTS_CB               btapp_gatts_cb;

static tBTA_BLE_MANU*         p_ble_manu_buf = NULL;
static UINT8*                 p_ble_manu_val = NULL;
static tBTA_BLE_SERVICE*      p_ble_service_buf = NULL;
static tBTA_BLE_SERVICE_DATA* p_ble_service_data_buf = NULL;
static UINT8*                 p_ble_service_data_val = NULL;
static UINT16                 btapp_gatts_mtu_size   = GATT_DEF_BLE_MTU_SIZE;
static tBTA_GATTS_RSP         btapp_gatts_rsp_data;

static tBT_UUID btapp_gatts_app_uuid = {2, {BTAPP_GATTS_APP_UUID}};
static tBT_UUID btapp_gatts_service_uuid = {2, {BTAPP_GATTS_SERV_UUID}};
static tBT_UUID btapp_gatts_serv_char_uuid[BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV] = {
	{ 2, { BTAPP_GATTS_CHAR1_UUID } },
	{ 2, { BTAPP_GATTS_CHAR2_UUID } },
	{ 2, { BTAPP_GATTS_CHAR1_UUID } },
	{ 2, { BTAPP_GATTS_CHAR2_UUID } },
	{ 2, { BTAPP_GATTS_CHAR1_UUID } },
	{ 2, { BTAPP_GATTS_CHAR2_UUID } },
	{ 2, { BTAPP_GATTS_CHAR1_UUID } },
	{ 2, { BTAPP_GATTS_CHAR2_UUID } },
	{ 2, { BTAPP_GATTS_CHAR1_UUID } },
	{ 2, { BTAPP_GATTS_CHAR3_UUID } },
	{ 2, { BTAPP_GATTS_CHAR3_UUID } },
};
static tBT_UUID btapp_gatts_serv_char_descriptor_uuid[BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV] = {
	{ 2, { BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR2_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR2_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR2_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR2_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR } },
	{ 2, { BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR } }
};

static int srv_char_index = 0;
int btapp_gatts_total_srv_num = 0;
//static tBT_UUID btapp_gatts_serv_char1_uuid = {2, {BTAPP_GATTS_CHAR1_UUID}};
//static tBT_UUID btapp_gatts_serv_char1_descriptor_uuid = {2, {BTAPP_GATTS_CHAR1_CLIENT_DESCRIPTOR}};

GATTS_READ_PROCESS_CB GattsReadCBFunc[BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV] = {NULL};
GATTS_WRITE_PROCESS_CB GattsWriteCBFunc[BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV] = {NULL};

bool gbBleIsConnected = false, gbBleStateChanged = false;



extern uint8_t ssid[128], pw[128];


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
void btapp_gatts_cback(tBTA_GATTS_EVT event, tBTA_GATTS *p_data);

/*******************************************************************************
**
** Function         btapp_gatts_start
**
** Description      start btapp gatt server
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_start(void)
{
    if(!btapp_gatts_cb.registered)
    {
        btapp_gatts_register(&btapp_gatts_app_uuid);
        btapp_gatts_adv_start();
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gatts_start has started!!!"LINE_ENDING);
    }
}

/*******************************************************************************
**
** Function         btapp_gatts_stop
**
** Description      stop btapp gatt server
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_stop(void)
{
    if(btapp_gatts_cb.registered)
    {
        btapp_gatts_deregister(btapp_gatts_cb.server_if);
        btapp_gatts_adv_stop();
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gatts_stop has started!!!"LINE_ENDING);
    }
}

/*******************************************************************************
 **
 ** Function        btapp_gatts_adv_config_cb
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
void btapp_gatts_adv_config_cb(tBTA_STATUS status)
{
    if(status != BTA_SUCCESS)
    {
        BRCM_PLATFORM_TRACE("gatts adv config not success, status:%d"LINE_ENDING, status);
    }

    if(p_ble_manu_buf != NULL)
    {
        GKI_freebuf(p_ble_manu_buf);
    }

    if(p_ble_manu_val != NULL)
    {
        GKI_freebuf(p_ble_manu_val);
    }

    if(p_ble_service_buf != NULL)
    {
        GKI_freebuf(p_ble_service_buf);
    }

    if(p_ble_service_data_buf != NULL)
    {
        GKI_freebuf(p_ble_service_data_buf);
    }

    if(p_ble_service_data_val != NULL)
    {
        GKI_freebuf(p_ble_service_data_val);
    }
}

/*******************************************************************************
 **
 ** Function        btapp_gatts_scan_rsp_config_cb
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
void btapp_gatts_scan_rsp_config_cb(tBTA_STATUS status)
{

}

/*******************************************************************************
**
** Function         btapp_gatts_adv_start
**
** Description      start btapp gatt server adv
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_adv_start(void)
{
    tBTA_BLE_AD_MASK  adv_data_mask;
    tBTA_BLE_ADV_DATA adv_cfg_data;
    tBTA_BLE_ADV_DATA scan_rsp_data;

    adv_data_mask = BTA_BLE_AD_BIT_FLAGS | BTA_BLE_AD_BIT_SERVICE | BTA_DM_BLE_AD_BIT_SERVICE_DATA | BTA_BLE_AD_BIT_MANU;

    /* set adv data*/
    if (adv_data_mask & BTA_BLE_AD_BIT_FLAGS)
    {
        adv_cfg_data.flag = 0x1A;
    }

    if (adv_data_mask & BTA_BLE_AD_BIT_SERVICE)
    {
        p_ble_service_buf = (tBTA_BLE_SERVICE*)GKI_getbuf(sizeof(tBTA_BLE_SERVICE));
        p_ble_service_buf->num_service = 1;
        p_ble_service_buf->list_cmpl   = 0;
        p_ble_service_buf->p_uuid      = &btapp_gatts_app_uuid.uu.uuid16;
        adv_cfg_data.p_services = p_ble_service_buf;
    }

    if (adv_data_mask & BTA_BLE_AD_BIT_MANU)
    {
        p_ble_manu_buf = (tBTA_BLE_MANU*)GKI_getbuf(sizeof(tBTA_BLE_MANU));
        p_ble_manu_val = (UINT8*)GKI_getbuf(sizeof(UINT8)*(BTAPP_GATTS_MAX_ADV_DATA_LEN));

        adv_cfg_data.p_manu = p_ble_manu_buf;
        adv_cfg_data.p_manu->len = sizeof(BD_ADDR);
        adv_cfg_data.p_manu->p_val= p_ble_manu_val;
        memcpy(p_ble_manu_val, btapp_cfg.set_local_addr, sizeof(BD_ADDR));
    }

    if (adv_data_mask & BTA_DM_BLE_AD_BIT_SERVICE_DATA)
    {
        p_ble_service_data_buf =  (tBTA_BLE_SERVICE_DATA*)GKI_getbuf(sizeof(tBTA_BLE_SERVICE_DATA));
        p_ble_service_data_val =  (UINT8*)GKI_getbuf(sizeof(UINT8)*(BTAPP_GATTS_MAX_ADV_DATA_LEN));

        adv_cfg_data.p_service_data = p_ble_service_data_buf;
        memcpy(&p_ble_service_data_buf->service_uuid, &btapp_gatts_app_uuid, sizeof(tBT_UUID));
        p_ble_service_data_buf->len = strlen("Google GATTS");
        p_ble_service_data_buf->p_val = p_ble_service_data_val;
        memcpy(p_ble_service_data_val, "Google GATTS", strlen("Google GATTS"));
    }

    BTA_DmBleSetAdvConfig(adv_data_mask, &adv_cfg_data, btapp_gatts_adv_config_cb);

    /* set scan response data*/
    adv_data_mask = BTA_BLE_AD_BIT_DEV_NAME;

    /* since set the device name field, the scan_rsp_data don't assign dedicated value;
       if set others mask, need assign meaningful value to scan_rsp_data, if use the GKI_getbuf,
       need free in the tBTA_SET_ADV_DATA_CMPL_CBACK */
    BTA_DmBleSetScanRspConfig(adv_data_mask, &scan_rsp_data, NULL);

    BTA_DmSetBleAdvParams(BTAPP_GATTS_ADV_INT_MIN, BTAPP_GATTS_ADV_INT_MAX, NULL);

    BTA_DmSetBleVisibility(TRUE);
}

/*******************************************************************************
**
** Function         btapp_gatts_adv_stop
**
** Description      stop btapp gatt server adv
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_adv_stop(void)
{
    BTA_DmSetBleVisibility(FALSE);
}

/*******************************************************************************
**
** Function         btapp_gatts_register
**
** Description      Initializes GATT Server application
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_register(tBT_UUID *p_app_uuid)
{
    memcpy(&btapp_gatts_cb.app_id, p_app_uuid, sizeof(tBT_UUID));
    BTA_GATTS_AppRegister(p_app_uuid, btapp_gatts_cback);
}

/*******************************************************************************
**
** Function         btapp_gatts_deregister
**
** Description      deregister GATT Server application
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_deregister(tBTA_GATTS_IF sif)
{
    if(btapp_gatts_cb.clcb[0].connected)
    {//If current connected with others, we need close the gatt server first.
        btapp_gatts_cb.need_dereg = TRUE;
        BTA_GATTS_Close(btapp_gatts_cb.clcb[0].conn_id);
    }
    else
    {
        BTA_GATTS_DeleteService(btapp_gatts_cb.serv_inst[0].service_id);
        BTA_GATTS_AppDeregister(btapp_gatts_cb.serv_inst[0].service_id);
    }
}

/*******************************************************************************
**
** Function         btapp_gatts_open
**
** Description      Open a GATT connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_open(tBTA_GATTS_IF sif, BD_ADDR remote_bda, BOOLEAN is_direct, UINT8 use_br_edr)
{
    tBTA_TRANSPORT  transport = BTA_TRANSPORT_LE;

    if (use_br_edr)
        transport = BTA_TRANSPORT_BR_EDR;

    BTA_GATTS_Open(sif, remote_bda, is_direct, transport);
}

/*******************************************************************************
**
** Function         btapp_gatts_cancel_open
**
** Description      Open a GATT connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_cancel_open(tBTA_GATTS_IF sif, BD_ADDR remote_bda, BOOLEAN is_direct)
{
    BTA_GATTS_CancelOpen(sif, remote_bda, is_direct);
}

/*******************************************************************************
**
** Function         btapp_gatts_close
**
** Description      Close a GATT connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_close(tBTA_GATTS_IF sif, BD_ADDR remote_bda, UINT8 use_br_edr)
{
    tBTAPP_GATTS_CLCB *p_clcb;

    if ((p_clcb = btapp_gatts_find_clcb(sif, remote_bda, use_br_edr))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    BTA_GATTS_Close(p_clcb->conn_id);
}

/*******************************************************************************
**
** Function         btapp_gatts_close
**
** Description      notify a attributes
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_notify(tBTA_GATTS_IF sif, BD_ADDR remote_bda,
                        UINT16 attr_handle, UINT16 len, UINT8* p_value, UINT8 use_br_edr)
{
    tBTAPP_GATTS_CLCB *p_clcb;

    if ((p_clcb = btapp_gatts_find_clcb(sif, remote_bda, use_br_edr))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    BTA_GATTS_HandleValueIndication(p_clcb->conn_id,
                                    attr_handle,
                                    len,
                                    p_value,
                                    FALSE); /* notification does not need confirmation */
}

/*******************************************************************************
**
** Function         btapp_gatts_send_notify_pkt
**
** Description      send a notification packet for notify loop testing.
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_send_notify_pkt (tBTAPP_GATTS_CLCB *p_clcb, UINT32 loop)
{
    char * value = "testnotifyloop";

    BRCM_PLATFORM_TRACE("send  notification in loop: #%d to send"LINE_ENDING, loop);

    if (loop && p_clcb && !p_clcb->congested)
    {
        BTA_GATTS_HandleValueIndication(p_clcb->conn_id,
                                        0x100,
                                        strlen(value),
                                        (UINT8 *)value,
                                        FALSE); /* notification does not need confirmation */
    }
}

/*******************************************************************************
**
** Function         btapp_gatts_notify_loop
**
** Description      notify a attributes in loop for testing
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_notify_loop (tBTA_GATTS_IF sif, BD_ADDR remote_bda,
                               UINT8 use_br_edr, UINT32 loop)
{
    tBTAPP_GATTS_CLCB *p_clcb;


    if ((p_clcb = btapp_gatts_find_clcb(sif, remote_bda, use_br_edr))==NULL)
    {
        BRCM_PLATFORM_TRACE("Can not find the conn_id"LINE_ENDING);
        return;
    }
    p_clcb->loop = loop;

    btapp_gatts_send_notify_pkt(p_clcb, p_clcb->loop);
}

/*******************************************************************************
**
** Function         btapp_gatts_server_load_bonded
**
** Description      load  bonded device into background connection list
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_server_load_bonded(tBTA_GATTS_IF server_if, UINT32 service_mask)
{
    UINT8 i, j;

    /* enable background connection*/
    BTA_DmBleSetBgConnType(BTA_DM_BLE_CONN_AUTO, NULL);

    for (i = 0; i < BTAPP_NUM_REM_DEVICE; i ++)
    {
        if (btapp_device_db.device[i].ble_services & BTAPP_BLE_GATT_SERVICE_MASK &&
            //(btapp_device_db.device[i].services & service_mask) &&
            (btapp_device_db.device[i].key_mask & BTA_LE_KEY_PENC))
        {
            for (j = 0; j < BTAPP_GATTS_BG_CONN_MAX; j ++)
            {
                if (!btapp_gatts_cb.bg_conn_list[j].in_use)
                {
                    btapp_gatts_cb.bg_conn_list[j].in_use = TRUE;
                    btapp_gatts_cb.bg_conn_list[j].server_if = server_if;
                    memcpy(btapp_gatts_cb.bg_conn_list[j].bda, btapp_device_db.device[i].bd_addr, BD_ADDR_LEN);

                    BRCM_PLATFORM_TRACE("start backgournd connection for a saved device"LINE_ENDING);
                    /* initiate a background connection to a peer device */
                    BTA_GATTS_Open(server_if,
                                   btapp_device_db.device[i].bd_addr,
                                   FALSE,
                                   BTA_TRANSPORT_LE);
                    break;
                }
            }

            if (j == BTAPP_GATTS_BG_CONN_MAX)
            {
                BRCM_PLATFORM_TRACE("max bg conn reached"LINE_ENDING);
                break;
            }
        }
    }
}

/*******************************************************************************
**
** Function         btapp_gatts_server_unload_bond
**
** Description      load bonded device into background connection list
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_server_unload_bond(tBTA_GATTS_IF server_if)
{
    UINT8 j;
    tBTAPP_GATTS_BG_CONN *p_bg_conn = &btapp_gatts_cb.bg_conn_list[0];

    for (j = 0; j < BTAPP_GATTS_BG_CONN_MAX; j ++, p_bg_conn++)
    {
        if (p_bg_conn->in_use &&
            p_bg_conn->server_if == server_if)
        {
            BRCM_PLATFORM_TRACE("remove backgournd connection for a saved device"LINE_ENDING);
            /* initiate a background connection to a peer device */
            BTA_GATTS_CancelOpen(server_if,
                                 p_bg_conn->bda,
                                 FALSE);

            memset(p_bg_conn, 0, sizeof(tBTAPP_GATTS_BG_CONN));
        }
    }
}

/*******************************************************************************
**
** Function         btapp_gatts_find_clcb
**
** Description      The function searches all LCB with macthing bd address
**
** Returns          total number of clcb found.
**
*******************************************************************************/
tBTAPP_GATTS_CLCB *btapp_gatts_find_clcb( tBTA_GATTS_IF sif, BD_ADDR bda, UINT8 use_br_edr)
{
    UINT8 i_clcb;
    tBTAPP_GATTS_CLCB    *p_clcb = NULL;
    tBTA_TRANSPORT  transport = BTA_TRANSPORT_LE;

    if (use_br_edr)
        transport = BTA_TRANSPORT_BR_EDR;

    for (i_clcb = 0, p_clcb= btapp_gatts_cb.clcb; i_clcb < GATT_CL_MAX_LCB; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && p_clcb->transport == transport &&
            (p_clcb->server_if == sif) && !memcmp(p_clcb->bda, bda, BD_ADDR_LEN))
        {
            return p_clcb;
        }
    }

    return p_clcb;
}

/*******************************************************************************
**
** Function         btapp_gatts_clcb_alloc
**
** Description      The function allocates a   connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
tBTAPP_GATTS_CLCB *btapp_gatts_clcb_alloc (UINT16 conn_id, tBTA_GATTS_IF sif, BD_ADDR bda,
                                          tBTA_TRANSPORT  transport)
{
    UINT8         i_clcb = 0;
    tBTAPP_GATTS_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btapp_gatts_cb.clcb; i_clcb < GATT_CL_MAX_LCB; i_clcb++, p_clcb++)
    {
        if (!p_clcb->in_use)
        {
            p_clcb->in_use      = TRUE;
            p_clcb->conn_id     = conn_id;
            p_clcb->connected   = TRUE;
            p_clcb->server_if   = sif;
            p_clcb->transport   = transport;
            memcpy (p_clcb->bda, bda, BD_ADDR_LEN);
            return p_clcb;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         btapp_gatts_clcb_dealloc
**
** Description      The function deallocates a connection link control block
**
** Returns           NULL if not found. Otherwise pointer to the connection link block.
**
*******************************************************************************/
BOOLEAN btapp_gatts_clcb_dealloc (UINT16 conn_id)
{
    UINT8         i_clcb = 0;
    tBTAPP_GATTS_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btapp_gatts_cb.clcb; i_clcb < GATT_CL_MAX_LCB; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && (p_clcb->conn_id == conn_id))
        {
            memset(p_clcb, 0, sizeof(tBTAPP_GATTS_CLCB));
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
**
** Function         btapp_gatts_find_clcb
**
** Description      The function searches all LCB with macthing bd address
**
** Returns          total number of clcb found.
**
*******************************************************************************/
tBTAPP_GATTS_CLCB *btapp_gatts_find_clcb_by_conn_id( UINT16 conn_id)
{
    UINT8 i_clcb;
    tBTAPP_GATTS_CLCB    *p_clcb = NULL;

    for (i_clcb = 0, p_clcb= btapp_gatts_cb.clcb; i_clcb < GATT_CL_MAX_LCB; i_clcb++, p_clcb++)
    {
        if (p_clcb->in_use && p_clcb->connected && p_clcb->conn_id == conn_id)
        {
            return p_clcb;
        }
    }

    return p_clcb;
}

/*******************************************************************************
**
** Function         btapp_gatts_build_services
**
** Description      build local gatt server whole needed services
**
** Paramters        inst_id: service index
**
** Returns
**
*******************************************************************************/
void btapp_gatts_build_services(UINT8 inst_id, UINT8 char_num)
{
    tBTAPP_GATTS_CB   *p_cb = &btapp_gatts_cb;

	//As an example, we just use the fixed macro, if use multiple chars, we use the index to find the expected char uuid,permission,properties
	int i;
	for (i = 0; i < char_num; i++)
	{
		if(i < 8)
		{
		BTA_GATTS_AddCharacteristic(p_cb->serv_inst[inst_id].service_id,
			&btapp_gatts_serv_char_uuid[i],
			BTAPP_GATTS_CHAR1_PERM,
			BTAPP_GATTS_CHAR1_PROP);

		}
		else
		{
			BTA_GATTS_AddCharacteristic(p_cb->serv_inst[inst_id].service_id,
			&btapp_gatts_serv_char_uuid[i],
			BTAPP_GATTS_CHAR2_PERM,
			BTAPP_GATTS_CHAR2_PROP);
		}

		BTA_GATTS_AddCharDescriptor(p_cb->serv_inst[inst_id].service_id,
			BTAPP_GATTS_CHAR1_CFG_PERM,
			&btapp_gatts_serv_char_descriptor_uuid[i]);
	}

    BTA_GATTS_StartService(p_cb->serv_inst[inst_id].service_id, BTA_GATT_TRANSPORT_LE);
}

/*******************************************************************************
**
** Function         btapp_gatts_proc_read
**
** Description      proc local gatt server read event
**
** Paramters        p_cb: local gatt server control block pointer; p_req_data
**
** Returns
**
*******************************************************************************/

void btapp_gatts_proc_read(tBTAPP_GATTS_CB *p_cb, tBTA_GATTS_REQ *p_req_data)
{
	UINT16 read_handle = p_req_data->p_data->read_req.handle;
	UINT16 req_offset = p_req_data->p_data->read_req.offset;
	BOOLEAN is_long = p_req_data->p_data->read_req.is_long;
	UINT16 req_len = btapp_gatts_mtu_size - 1;
	UINT16 sent_len = 0;
	UINT16 req_cfg_len = 2;
	UINT8  rsp_len = 0;
	tBTA_GATT_STATUS status = BTA_GATT_NOT_FOUND;
	tBTA_GATTS_RSP *p_rsp = &btapp_gatts_rsp_data;

	BRCM_PLATFORM_TRACE("btapp_gatts_proc_read need read handle:%d, is_long:%d, offset:%d"LINE_ENDING,
		p_req_data->p_data->read_req.handle,
		p_req_data->p_data->read_req.is_long,
		p_req_data->p_data->read_req.offset);

	//The best way is use the read_handle to find the read region
	int i = 0;
	for(i = 0; i < BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV; i++)
	{
	
		if (read_handle == p_cb->serv_inst[0].char_inst[i].attr_id)
		{
			if (req_offset < BTAPP_GATTS_SIMPLE_CHAR_SIZE)
			{
				status = BTA_GATT_OK;
				sent_len = ((BTAPP_GATTS_SIMPLE_CHAR_SIZE - req_offset) > req_len) ? req_len : (BTAPP_GATTS_SIMPLE_CHAR_SIZE - req_offset);
			}
			else
			{
				status = BTA_GATT_OK;
				sent_len = 0;
			}

			p_rsp->attr_value.conn_id = 0;    //could ingore this field
			p_rsp->attr_value.handle = read_handle;
			p_rsp->attr_value.len = sent_len;
			p_rsp->attr_value.offset = req_offset;
			p_rsp->attr_value.auth_req = GATT_AUTH_REQ_NONE;

			if(GattsReadCBFunc[i]!=NULL)
			{
				tBTAPP_GATTS_CB_RET* pRep;
				GATTS_READ_PROCESS_CB pfun =  GattsReadCBFunc[i];
				pRep = pfun(); 
				//printf("==== pRep->resp_val: %s\n",pRep->resp_val);
				//printf("==== pRep->len: %d\n",pRep->len);
				
				memset(&p_rsp->attr_value.value[0], 0, GATT_MAX_ATTR_LEN);
				memcpy(&p_rsp->attr_value.value[0], &pRep->resp_val[0], pRep->len);
				p_rsp->attr_value.len = pRep->len;
				
			}
			else
				memcpy(&p_rsp->attr_value.value[0], &p_cb->serv_inst[0].char_inst[i].attr_val[req_offset], sent_len);
			
			BTA_GATTS_SendRsp(p_req_data->conn_id, p_req_data->trans_id, status, &btapp_gatts_rsp_data);
			break;
		}
		else if (read_handle == p_cb->serv_inst[0].char_cfg[i].attr_id)
		{
			status = BTA_GATT_OK;
			p_rsp->attr_value.conn_id = 0;    //could ingore this field
			p_rsp->attr_value.handle = read_handle;
			p_rsp->attr_value.len = req_cfg_len;
			p_rsp->attr_value.offset = req_offset;
			p_rsp->attr_value.auth_req = GATT_AUTH_REQ_NONE;
			memcpy(&p_rsp->attr_value.value[0], &p_cb->serv_inst[0].char_cfg[i].cccd_cfg, req_cfg_len);
			BTA_GATTS_SendRsp(p_req_data->conn_id, p_req_data->trans_id, status, &btapp_gatts_rsp_data);
			break;
		}
	}

}

static pthread_t test_task;
static UINT8 test_task_enable = 0;
static void test_task_main(void * parm)
{
    tBTAPP_GATTS_CB* p_cb = &btapp_gatts_cb;
    uint32_t test_value=0;
    test_task_enable = 1;
    do{
        usleep(1*1000*1000);
        test_value ++;
        BTA_GATTS_HandleValueIndication(p_cb->clcb[0].conn_id,
                                        p_cb->serv_inst[0].char_inst[0].attr_id,
                                        sizeof(test_value),
                                        (UINT8 *)&test_value,
                                        FALSE); /* notification does not need confirmation */

    }while(p_cb->serv_inst[0].char_cfg[0].cccd_cfg);

    test_task_enable = 0;
    pthread_exit(NULL);
}

/*******************************************************************************
**
** Function         btapp_gatts_proc_write
**
** Description      proc local gatt server write event
**
** Paramters        p_cb: local gatt server control block pointer; p_req_data
**
** Returns
**
*******************************************************************************/

void btapp_gatts_proc_write(tBTAPP_GATTS_CB *p_cb, tBTA_GATTS_REQ *p_req_data)
{
	UINT16 write_handle = p_req_data->p_data->write_req.handle;
	UINT16 wrote_len = p_req_data->p_data->write_req.len;
	UINT16 wrote_offset = p_req_data->p_data->write_req.offset;
	BOOLEAN need_rsp = p_req_data->p_data->write_req.need_rsp;
	BOOLEAN is_prep = p_req_data->p_data->write_req.is_prep;
	UINT8*  p_wrote_val = &p_req_data->p_data->write_req.value[0];
	tBTA_GATT_STATUS status = BTA_GATT_NOT_FOUND;
	tBTA_GATTS_RSP *p_rsp = &btapp_gatts_rsp_data;
	int i = 0;

	BRCM_PLATFORM_TRACE("btapp_gatts_proc_write handle:%d, len:%d, offset:%d, need_rsp:%d, is_prep:%d"LINE_ENDING,
		write_handle,
		wrote_len,
		wrote_offset,
		need_rsp,
		is_prep);
	
	for(i = 0;i < BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV; i++)
	{
		if (write_handle == p_cb->serv_inst[0].char_inst[i].attr_id)
		{
			memcpy(&p_cb->serv_inst[0].char_inst[i].attr_val[wrote_offset], p_wrote_val, wrote_len);
			BRCM_PLATFORM_TRACE("Peer wrote content:   |%s| "LINE_ENDING, p_wrote_val);

			if(GattsWriteCBFunc[i]!=NULL)
			{
				GATTS_WRITE_PROCESS_CB pfun =  GattsWriteCBFunc[i];
				pfun(p_wrote_val); 
			}

			if (need_rsp)
			{
				BTA_GATTS_SendRsp(p_req_data->conn_id, p_req_data->trans_id, status, &btapp_gatts_rsp_data);
			}
			break;
		}
		else if (write_handle == p_cb->serv_inst[0].char_cfg[i].attr_id)
		{
			status = BTA_GATT_OK;
			memcpy(&p_cb->serv_inst[0].char_cfg[i].cccd_cfg, p_wrote_val, wrote_len);
			if (need_rsp)
			{
				BTA_GATTS_SendRsp(p_req_data->conn_id, p_req_data->trans_id, status, &btapp_gatts_rsp_data);
			}
			break;
		}
	}
    
}


/*******************************************************************************
**
** Function         btapp_gatts_proc_exec_write
**
** Description      proc local gatt server execute write event
**
** Paramters        p_cb: local gatt server control block pointer; p_req_data
**
** Returns
**
*******************************************************************************/
void btapp_gatts_proc_exec_write(tBTAPP_GATTS_CB *p_cb, tBTA_GATTS_REQ *p_req_data)
{
    UINT8 exec_flag     = p_req_data->p_data->exec_write;
    UINT16 write_handle = p_req_data->p_data->write_req.handle;
    UINT16 wrote_len    = p_req_data->p_data->write_req.len;
    UINT16 wrote_offset = p_req_data->p_data->write_req.offset;
    BOOLEAN need_rsp    = p_req_data->p_data->write_req.need_rsp;
    BOOLEAN is_prep     = p_req_data->p_data->write_req.is_prep;
    UINT8*  p_wrote_val = &p_req_data->p_data->write_req.value[0];
    tBTA_GATT_STATUS status = BTA_GATT_NOT_FOUND;
    tBTA_GATTS_RSP *p_rsp = &btapp_gatts_rsp_data;

    if(exec_flag == GATT_PREP_WRITE_CANCEL)
    {
        BRCM_PLATFORM_TRACE("btapp_gatts_proc_exec_write cancel write"LINE_ENDING);
    }
    else if(exec_flag == GATT_PREP_WRITE_EXEC)
    {
        BRCM_PLATFORM_TRACE("btapp_gatts_proc_exec_write execute write"LINE_ENDING);
    }

    status = BTA_GATT_OK;

    if(need_rsp)
        BTA_GATTS_SendRsp(p_req_data->conn_id, p_req_data->trans_id, status, &btapp_gatts_rsp_data);
}

/*******************************************************************************
**
** Function         btapp_gatts_proc_mtu_exchange
**
** Description      proc local gatt server gatt MTU exchange event
**
** Paramters        p_cb: local gatt server control block pointer; p_req_data
**
** Returns
**
*******************************************************************************/
void btapp_gatts_proc_mtu_exchange(tBTAPP_GATTS_CB *p_cb, tBTA_GATTS_REQ *p_req_data)
{
    UINT16 mtu_size = p_req_data->p_data->mtu;

    btapp_gatts_mtu_size = mtu_size;

    BRCM_PLATFORM_TRACE("MTU exchange len:%d"LINE_ENDING, mtu_size);
}

/*******************************************************************************
**
** Function         btapp_gatts_send_2_peer
**
** Description      send data to peer through the example char
**
** Paramters
**
** Returns
**
** Note             Just for test example API
**
*******************************************************************************/
void btapp_gatts_send_2_peer(UINT8* p_data, UINT8 len)
{
    tBTAPP_GATTS_CB* p_cb = &btapp_gatts_cb;
    BOOLEAN  need_confirm = FALSE;
    BOOLEAN  allow_sent   = FALSE;

    if(p_cb->serv_inst[0].char_cfg[0].cccd_cfg & BTAPP_GATTS_CHAR_CFG_NOTIFICATION_MASK || p_cb->serv_inst[0].char_cfg[0].cccd_cfg & BTAPP_GATTS_CHAR_CFG_INDICATION_MASK)
    {
        allow_sent = TRUE;
        if(p_cb->serv_inst[0].char_cfg[0].cccd_cfg & BTAPP_GATTS_CHAR_CFG_INDICATION_MASK)
        {
            need_confirm = TRUE;
        }
    }
    else
    {
        BRCM_PLATFORM_TRACE("Operation doesn't allow, please grant the cccd is configured by peer!!!"LINE_ENDING);
    }

    if(allow_sent)
    {
        BTA_GATTS_HandleValueIndication(p_cb->clcb[0].conn_id,
                                p_cb->serv_inst[0].char_inst[0].attr_id,
                                len,
                                p_data,
                                need_confirm);
    }
}

/*******************************************************************************
**
** Function         btapp_gatts_proc_disconnect
**
** Description      proc
**
** Paramters
**
** Returns
**
** Note
**
*******************************************************************************/
void btapp_gatts_proc_disconnect(tBTAPP_GATTS_CB *p_cb, tBTA_GATTS_CONN* p_conn)
{
    UINT8 i;

    for(i = 0; i < BTAPP_GATTS_MAX_CCCD_IN_ONE_SERV; i ++)
    {
        p_cb->serv_inst[0].char_cfg[i].cccd_cfg = 0x0000;
    }
}

/*******************************************************************************
**
** Function         btapp_gatts_cback
**
** Description      GATTC UI Callback function.  Handles all GATTC events.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_cback(tBTA_GATTS_EVT event, tBTA_GATTS *p_data)
{
	tBTAPP_GATTS_CLCB *p_clcb;
	tBTAPP_GATTS_CB   *p_cb = &btapp_gatts_cb;

	BRCM_PLATFORM_TRACE("btapp_gatts_cback event:%d"LINE_ENDING, event);

	switch (event)
	{
	case BTA_GATTS_REG_EVT:
		BRCM_PLATFORM_TRACE("GATTS registered...sif:%d, status:%d"LINE_ENDING, p_data->reg_oper.server_if, p_data->reg_oper.status);
		if (p_data->reg_oper.status == BTA_GATT_OK)
		{
			btapp_gatts_cb.registered = TRUE;
			btapp_gatts_cb.server_if = p_data->reg_oper.server_if;

			//While the gatt server application registered success, we'll create the local gatt services database.
			BTA_GATTS_CreateService(btapp_gatts_cb.server_if,
				&btapp_gatts_service_uuid,
				0,                       /* theoretical only one instance per device */
				BTAPP_GATTS_HANDLE_NUM,
				TRUE);                   /* always create as primary service */
		}
		break;

	case BTA_GATTS_DEREG_EVT:
		BRCM_PLATFORM_TRACE("GATTS deregistered...sif:%d, status:%d"LINE_ENDING, p_data->reg_oper.server_if, p_data->reg_oper.status);

		if (p_data->reg_oper.status == BTA_GATT_OK)
		{
			btapp_gatts_cb.registered = FALSE;
		}

		btapp_gatts_cb.server_if = 0;

		break;

	case BTA_GATTS_CREATE_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_CREATE_EVT"LINE_ENDING);
		if (p_data->create.status == BTA_GATT_OK)
		{
			//If create muliple services, should record the created service index at the application layer,
			//then use the index for the serv_inst.
			p_cb->serv_inst[0].service_id = p_data->create.service_id;
			/* add characteristic to this service */
			btapp_gatts_build_services(0, btapp_gatts_total_srv_num);
		}

		break;

	case BTA_GATTS_DELELTE_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_DELELTE_EVT"LINE_ENDING);

		if (p_data->srvc_oper.status == BTA_GATT_OK)
		{
			BTA_GATTS_AppDeregister(p_data->srvc_oper.server_if);
		}

		break;

	case BTA_GATTS_CONNECT_EVT:
            /* update BLE connParams */
            BTA_DmBleUpdateConnectionParam(p_data->conn.remote_bda, 100, 100, 0, 150);

		if (!btapp_gatt_is_gattc_type(p_data->conn.remote_bda))
		{
			BRCM_PLATFORM_TRACE("%s BTA_GATTS_CONNECT_EVT server_if:%d remote_addr:%s conn_id:%d reason:%d transport:%d"LINE_ENDING, __func__,
				p_data->conn.server_if,
				utl_bd_addr_to_string(p_data->conn.remote_bda),
				p_data->conn.conn_id,
				p_data->conn.reason,
				p_data->conn.transport);

			if (btapp_gatts_clcb_alloc(p_data->conn.conn_id,
				p_data->conn.server_if,
				p_data->conn.remote_bda,
				p_data->conn.transport) == NULL)
			{
				BRCM_PLATFORM_TRACE("btapp_gatts_cback: no CLCB can be allocated"LINE_ENDING);
				return;
			}

			/*As a example, we just treat the local gatt server only connect one peer device, so when
			*the peer device connected with local gatt server, we'll stop the ble advertisement.
			*But we're able to the local gatt server connect to multiple peer devices, it should
			*careful use this feature, beacause the application need care more things.
			*/
			
			btapp_gatts_adv_stop();
			gbBleIsConnected = true;
			gbBleStateChanged = true;

		}
		else
		{
			BRCM_PLATFORM_TRACE("%s recv one gatt client device connected event!!!"LINE_ENDING, __FUNCTION__);
		}

		break;

	case BTA_GATTS_DISCONNECT_EVT:
		if (!btapp_gatt_is_gattc_type(p_data->conn.remote_bda))
		{
			BRCM_PLATFORM_TRACE("%s BTA_GATTS_DISCONNECT_EVT server_if:%d remote_addr:%s conn_id:%d reason:%d transport:%d"LINE_ENDING, __func__,
				p_data->conn.server_if,
				utl_bd_addr_to_string(p_data->conn.remote_bda),
				p_data->conn.conn_id,
				p_data->conn.reason,
				p_data->conn.transport);

			btapp_gatts_proc_disconnect(p_cb, &p_data->conn);

			btapp_gatts_clcb_dealloc(p_data->conn.conn_id);

			if (btapp_gatts_cb.need_dereg)
			{
				btapp_gatts_cb.need_dereg = FALSE;
				BTA_GATTS_DeleteService(btapp_gatts_cb.serv_inst[0].service_id);
				BTA_GATTS_AppDeregister(btapp_gatts_cb.serv_inst[0].service_id);
			}
			else
			{
				//start discovery
				btapp_gatts_adv_start();
			}
		}
		else
		{
			BRCM_PLATFORM_TRACE("%s recv one gatt client device disconnected event!!!"LINE_ENDING, __FUNCTION__);
		}
		gbBleIsConnected = false;
		gbBleStateChanged = true;
		break;

	case BTA_GATTS_CONF_EVT:
		p_clcb = btapp_gatts_find_clcb_by_conn_id(p_data->confirm.conn_id);

		if (p_clcb != NULL)
		{
			if (p_data->confirm.status == BTA_GATT_OK ||
				p_data->confirm.status == BTA_GATT_CONGESTED)
			{
				if (p_clcb->loop > 0) p_clcb->loop--;
			}
			//btapp_gatts_send_notify_pkt(p_clcb, p_clcb->loop);
		}

		break;

	case BTA_GATTS_CONGEST_EVT:
		p_clcb = btapp_gatts_find_clcb_by_conn_id(p_data->congest.conn_id);
		p_clcb->congested = p_data->congest.congested;

		BRCM_PLATFORM_TRACE("channel congestion indicator: %d"LINE_ENDING, p_clcb->congested);

		break;

	case BTA_GATTS_READ_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_READ_EVT"LINE_ENDING);
		btapp_gatts_proc_read(p_cb, &p_data->req_data);
		break;

	case BTA_GATTS_WRITE_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_WRITE_EVT"LINE_ENDING);
		btapp_gatts_proc_write(p_cb, &p_data->req_data);
		break;

	case BTA_GATTS_EXEC_WRITE_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_EXEC_WRITE_EVT"LINE_ENDING);
		btapp_gatts_proc_exec_write(p_cb, &p_data->req_data);
		break;

	case BTA_GATTS_MTU_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_MTU_EVT"LINE_ENDING);
		btapp_gatts_proc_mtu_exchange(p_cb, &p_data->req_data);
            /* update BLE connParams */
            BTA_DmBleUpdateConnectionParam(p_data->conn.remote_bda, 100, 100, 0, 150);
		break;

	case BTA_GATTS_ADD_INCL_SRVC_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_ADD_INCL_SRVC_EVT"LINE_ENDING);

		break;

	case BTA_GATTS_ADD_CHAR_EVT:
		BRCM_PLATFORM_TRACE("%d btapp_gatts_cback BTA_GATTS_ADD_CHAR_EVT status:%d, attr_id:%d"LINE_ENDING, srv_char_index, p_data->add_result.status, p_data->add_result.attr_id);
		if (p_data->add_result.status == BTA_GATT_OK)
		{//The index should be find from serv_inst and char_inst, as example, just simply.
                p_cb->serv_inst[0].char_inst[srv_char_index].attr_id = p_data->add_result.attr_id;
                p_cb->serv_inst[0].char_inst[srv_char_index].p_char_uuid = p_data->add_result.p_uuid;
		}
		break;

	case BTA_GATTS_ADD_CHAR_DESCR_EVT:
		BRCM_PLATFORM_TRACE("%d btapp_gatts_cback BTA_GATTS_ADD_CHAR_DESCR_EVT status:%d, attr_id:%d"LINE_ENDING, srv_char_index, p_data->add_result.status, p_data->add_result.attr_id);
		if (p_data->add_result.status == BTA_GATT_OK)
		{
			printf("srv_char_index %d\n",srv_char_index);
			p_cb->serv_inst[0].char_cfg[srv_char_index++].attr_id = p_data->add_result.attr_id;
		}
		break;

	case BTA_GATTS_START_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_START_EVT"LINE_ENDING);

		break;

	case BTA_GATTS_STOP_EVT:
		BRCM_PLATFORM_TRACE("btapp_gatts_cback BTA_GATTS_STOP_EVT"LINE_ENDING);

		break;

	default:
		break;
	}
}

/*******************************************************************************
**
** Function         btapp_gatts_update_handle_range
**
** Description      update the handle range information into NV ram.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_gatts_update_handle_range(BOOLEAN is_add, tBTA_GATTS_HNDL_RANGE *p_hndl_range)
{
    tBTAPP_GATTS_HNDL_RANGE_DB   *p_db = &btapp_gatts_hndl_range_db;
    UINT8                       num_services = p_db->num_services;
    UINT8                       max_index = BTAPP_GATT_MAX_HANDLE_MAP_SIZE -1;
    tBTA_GATTS_HNDL_RANGE       *p_handle = &btapp_gatts_hndl_range_db.hndl_range[0];
    BOOLEAN                     done = FALSE;
    UINT8                       i, j;


    BRCM_PLATFORM_TRACE("btapp_gatts_update_handle_range: %s handle range [0x%04x - 0x%04x] current num_services=%d"LINE_ENDING,
                      (is_add ?"add" :"delete"), p_hndl_range->s_handle, p_hndl_range->e_handle, num_services);

    for (i = 0; i < num_services ; i ++, p_handle ++)
    {
        /* find matching entry */
        if ((!memcmp(&p_handle->app_uuid128, &p_hndl_range->app_uuid128, sizeof(tBT_UUID))) &&
            (!memcmp(&p_handle->svc_uuid, &p_hndl_range->svc_uuid, sizeof(tBT_UUID))) &&
            (p_handle->svc_inst == p_hndl_range->svc_inst))
        {
            done = TRUE;
            if (!is_add)
            {
                if (i != max_index)
                {
                    for (j = i+1; j < num_services; j ++)
                        memcpy(&p_db->hndl_range[j - 1], &p_db->hndl_range[j], sizeof(tBTA_GATTS_HNDL_RANGE));
                }
                /* reset the last entry after update the array */
                memset(&p_db->hndl_range[num_services-1], 0, sizeof(tBTA_GATTS_HNDL_RANGE));
                p_db->num_services --;
            }
            else
            {
                /* this should not happen */
                BRCM_PLATFORM_TRACE("btapp_gatts_update_handle_range: Can not add to an existing entry"LINE_ENDING);
            }
            break;
        }
    }
    if (!done)
    {
        if (!is_add)
        {
            BRCM_PLATFORM_TRACE("btapp_gatts_update_handle_range: no matching record"LINE_ENDING);
        }
        else
        {
            /* add an entry */
            if (num_services == BTAPP_GATT_MAX_HANDLE_MAP_SIZE)
            {
                BRCM_PLATFORM_TRACE("btapp_gatts_update_handle_range: No space to add"LINE_ENDING);
            }
            else
            {
                memcpy(&p_db->hndl_range[num_services], p_hndl_range, sizeof(tBTA_GATTS_HNDL_RANGE));
                p_db->num_services ++;
            }
        }
    }

    btapp_nv_store_gatts_hndl_range_db();

    BRCM_PLATFORM_TRACE("btapp_gatts_update_handle_range: updated num_services=%d "LINE_ENDING, p_db->num_services);
}

/*******************************************************************************
**
** Function         btapp_gatts_get_handle_range
**
** Description      Get the handle range information from NV ram.
**                  This function shoudl be called sequentially until it
**                  return FALSE
** Returns          TRUE - success FALSE- fail (no more handle range for services)
**
*******************************************************************************/
BOOLEAN btapp_gatts_get_handle_range(UINT8 index,
                                     tBTA_GATTS_HNDL_RANGE *p_out_handle)
{
    tBTAPP_GATTS_HNDL_RANGE_DB *p_db = &btapp_gatts_hndl_range_db;
    BOOLEAN status = FALSE;

    BRCM_PLATFORM_TRACE("btapp_gatts_get_handle_range"LINE_ENDING);

    if (index < p_db->num_services)
    {
        if (p_db->hndl_range[index].s_handle != 0 && p_db->hndl_range[index].e_handle != 0)
        {
            memcpy(p_out_handle, &p_db->hndl_range[index], sizeof(tBTA_GATTS_HNDL_RANGE));
            status = TRUE;
        }
    }

    BRCM_PLATFORM_TRACE("btapp_gatts_get_handle_range status=%d index=%d (zero based) num_services=%d"LINE_ENDING,
                      status, index, p_db->num_services );

    return status;
}

/*******************************************************************************
**
** Function         btapp_gatts_srv_chg
**
** Description      process srv change request
**
**
** Returns          void
**
*******************************************************************************/
BOOLEAN btapp_gatts_srv_chg(tBTA_GATTS_SRV_CHG_CMD cmd,
                            tBTA_GATTS_SRV_CHG_REQ *p_req,
                            tBTA_GATTS_SRV_CHG_RSP *p_rsp)
{
    BOOLEAN     status = TRUE, update_db = TRUE, found = FALSE;
    UINT8       i,j, idx, last_idx;
    tBTAPP_GATTS_SRV_CHG_DB *p_db= &btapp_gatts_srv_chg_db;

    BRCM_PLATFORM_TRACE("btapp_gatts_srv_chg: cmd=%d "LINE_ENDING, cmd );

    switch (cmd)
    {
        case BTA_GATTS_SRV_CHG_CMD_ADD_CLIENT:
            BRCM_PLATFORM_TRACE("Rcv GATTS_SRV_CHG_CMD_ADD_CLIENT, current num_clients=%d"LINE_ENDING, p_db->num_clients);

            if (p_db->num_clients < BTAPP_GATT_MAX_SRV_CHG_CLT_SIZE)
            {
                memcpy(&p_db->srv_chg[p_db->num_clients], &p_req->srv_chg, sizeof(tBTA_GATTS_SRV_CHG));
                p_db->num_clients ++;
                BRCM_PLATFORM_TRACE( "Add the clinet to the srv chg db; srv_chg=%d, new num_clients=%d"LINE_ENDING,
                                   p_db->srv_chg[p_db->num_clients-1].srv_changed,
                                   p_db->num_clients);
            }
            else
            {
                status = FALSE;
                BRCM_PLATFORM_TRACE( "No space to add new srv chg client"LINE_ENDING);
            }
            break;

        case BTA_GATTS_SRV_CHG_CMD_UPDATE_CLIENT:

            BRCM_PLATFORM_TRACE( "Rcv BTA_GATTS_SRV_CHG_CMD_UPDATE_CLIENT BDA: %08x%04x srv_chg=%d"LINE_ENDING,
                               (p_req->srv_chg.bda[0]<<24)+(p_req->srv_chg.bda[1]<<16)+(p_req->srv_chg.bda[2]<<8)+p_req->srv_chg.bda[3],
                               (p_req->srv_chg.bda[4]<<8)+p_req->srv_chg.bda[5], p_req->srv_chg.srv_changed);

            for (i=0; i< p_db->num_clients; i++)
            {
                if (!memcmp(p_db->srv_chg[i].bda, p_req->srv_chg.bda, sizeof(BD_ADDR)))
                {
                    found = TRUE;
                    memcpy(&p_db->srv_chg[i], &p_req->srv_chg, sizeof(tBTA_GATTS_SRV_CHG));
                    BRCM_PLATFORM_TRACE( "Found the clinet to be updated; index=%d srv_chg=%d,  num_clients=%d"LINE_ENDING,
                                       i,
                                       p_db->srv_chg[i].srv_changed,
                                       p_db->num_clients);

                    break;
                }
            }

            if (!found)
            {
                status = FALSE;
                update_db = FALSE;
                BRCM_PLATFORM_TRACE( "Client to be updated Not Found"LINE_ENDING);
            }

            break;
        case BTA_GATTS_SRV_CHG_CMD_REMOVE_CLIENT:
            BRCM_PLATFORM_TRACE( "Rcv BTA_GATTS_SRV_CHG_CMD_REMOVE_CLIENT BDA: %08x%04x num_clinets=%d"LINE_ENDING,
                               (p_req->srv_chg.bda[0]<<24)+(p_req->srv_chg.bda[1]<<16)+(p_req->srv_chg.bda[2]<<8)+p_req->srv_chg.bda[3],
                               (p_req->srv_chg.bda[4]<<8)+p_req->srv_chg.bda[5], p_db->num_clients);

            for (i=0; i< p_db->num_clients; i++)
            {
                if (!memcmp(p_db->srv_chg[i].bda, p_req->srv_chg.bda, sizeof(BD_ADDR)))
                {
                    found = TRUE;
                    last_idx = p_db->num_clients -1;

                    if (i != last_idx )
                    {
                        /* update the array so there is no gap */
                        for (j=i; j < last_idx ; j++ )
                        {
                            memcpy(&p_db->srv_chg[j], &p_db->srv_chg[j+1], sizeof(tBTA_GATTS_SRV_CHG));
                        }

                    }
                    /* reset the last client and update num_clients */
                    memset(&p_db->srv_chg[last_idx], 0, sizeof(tBTA_GATTS_SRV_CHG));
                    p_db->num_clients --;
                    BRCM_PLATFORM_TRACE( "found the client to be rmoved index=%d new num_clinets=%d"LINE_ENDING, i, p_db->num_clients);

                    break;
                }
            }

            if (!found)
            {
                status = FALSE;
                update_db = FALSE;
                BRCM_PLATFORM_TRACE( "Client to be removed Not Found, num_clinets=%d"LINE_ENDING, p_db->num_clients);
            }
            break;

        case BTA_GATTS_SRV_CHG_CMD_READ_NUM_CLENTS:
            BRCM_PLATFORM_TRACE( "Rcv BTA_GATTS_SRV_CHG_CMD_READ_NUM_CLENTS num_clinets=%d"LINE_ENDING, p_db->num_clients);
            p_rsp->num_clients = p_db->num_clients ;
            update_db = FALSE;
            break;

        case BTA_GATTS_SRV_CHG_CMD_READ_CLENT:
            BRCM_PLATFORM_TRACE( "Rcv BTA_GATTS_SRV_CHG_CMD_READ_CLENT clinet index=%d (one based)"LINE_ENDING, p_req->client_read_index);
            idx = p_req->client_read_index - 1;

            if (idx < p_db->num_clients )
            {
                memcpy(&p_rsp->srv_chg, &p_db->srv_chg[idx], sizeof(tBTA_GATTS_SRV_CHG));
            }
            else
            {
                status = FALSE;
                BRCM_PLATFORM_TRACE("btapp_gatts_srv_chg: Unknown clinet index=%d "LINE_ENDING, p_req->client_read_index );
            }
            update_db = FALSE;
            break;

        default:
            BRCM_PLATFORM_TRACE("btapp_gatts_srv_chg: Unknown cmd=%d "LINE_ENDING, cmd );
            status = FALSE;
            break;
    }

    if (status && update_db)
        btapp_nv_store_gatts_srv_chg_db();

    BRCM_PLATFORM_TRACE("btapp_gatts_srv_chg: return status=%d "LINE_ENDING, status );
    return status;
}


/*******************************************************************************
**
** Function         btapp_gatts_add_service_uuid
**
** Description      add service
**					p_app_uuid: uuid
**					idx : index
**
**
** Returns          void
**
*******************************************************************************/

void btapp_gatts_add_service_uuid(tBT_UUID *p_app_uuid, int idx)
{
	if(idx >= BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV)
		return;

	memcpy(&btapp_gatts_serv_char_uuid[idx], p_app_uuid, sizeof(tBT_UUID));
	printf("btapp_gatts_add_service_uuid: 0x%02x\n", btapp_gatts_serv_char_uuid[idx].uu);
	btapp_gatts_total_srv_num++;
}

void btapp_gatts_register_read_cb_function(int idx, GATTS_READ_PROCESS_CB pFun)
{
	if(idx >= BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV)
		return;
	if(pFun!=NULL)
		GattsReadCBFunc[idx] = pFun;
}

void btapp_gatts_register_write_cb_function(int idx, GATTS_WRITE_PROCESS_CB pFun)
{
	if(idx >= BTAPP_GATTS_MAX_ATTR_IN_ONE_SERV)
		return;
	if(pFun!=NULL)
		GattsWriteCBFunc[idx] = pFun;
}



#endif
#endif

