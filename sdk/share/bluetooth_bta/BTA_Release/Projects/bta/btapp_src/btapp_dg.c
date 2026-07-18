/****************************************************************************
**
**  Name:          btapp_dg.c
**
**  Description:   Contains application code for  data gateway
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

#if( defined BTA_DG_INCLUDED ) && (BTA_DG_INCLUDED == TRUE)

#define APP_ID_SPP         0
#define APP_ID_IAP2        1
#define APP_ID_CARPLAY     2
#define APP_ID_SPP2        3

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/

void btapp_dg_iap2_test_echo(void);

void btapp_dg_cback(tBTA_DG_EVT event, tBTA_DG *p_data);

tBTAPP_DG_CB btapp_dg_cb;

/*******************************************************************************
**
** Function         btapp_dg_init
**
** Description      Initialises data gateway
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_init(void)
{
    memset((UINT8*)&btapp_dg_cb, 0, sizeof(tBTAPP_DG_CB));
    BTA_DgEnable(btapp_dg_cback);
}

/*******************************************************************************
**
** Function         btapp_dg_register_entry
**
** Description      register entry in the dg database.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_register_entry(UINT8 index)
{
    if(index >= 0 && index <= BTAPP_DG_ID_SPP_3)
    {
        if(btapp_dg_cb.app_cb[index].is_used == FALSE)
        {
            switch(index)
            {
            case APP_ID_IAP2:
                btapp_dg_cb.app_cb[index].is_used = TRUE;
                BTA_DgListen(BTA_DUN_SERVICE_ID, btapp_cfg.sppdg_security,
                "Wireless IAP2", index);
                break;
            case APP_ID_CARPLAY:
                btapp_dg_cb.app_cb[index].is_used = TRUE;
                BTA_DgListen(BTA_FAX_SERVICE_ID, btapp_cfg.sppdg_security,
                "Wireless Carplay", index);
                break;
            default:
            btapp_dg_cb.app_cb[index].is_used = TRUE;
            BTA_DgListen(BTA_SPP_SERVICE_ID, btapp_cfg.sppdg_security,
                     btapp_cfg.sppdg_service_name, index);
                break;
            }
        }
        else
        {
            APPL_TRACE_ERROR1("btapp_dg_register_entry %d had used", index);
        }
    }
    else
    {
        APPL_TRACE_ERROR0("btapp_dg_register_entry out of index");
    }
}

/*******************************************************************************
**
** Function         btapp_dg_deregister_entry
**
** Description      deregister entry in the dg database.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_deregister_entry(UINT8 index)
{
    if(index >= 0 && index <= BTAPP_DG_ID_SPP_3)
    {
        if(btapp_dg_cb.app_cb[index].is_used == TRUE)
        {
            btapp_dg_cb.app_cb[index].is_used = FALSE;
            BTA_DgShutdown(btapp_dg_cb.app_cb[index].api_handle);
        }
        else
        {
            APPL_TRACE_ERROR1("btapp_dg_deregister_entry %d had deregistered", index);
        }
    }
    else
    {
        APPL_TRACE_ERROR0("btapp_dg_deregister_entry out of index");
    }
}

/*******************************************************************************
**
** Function         btapp_dg_register_entry_nums
**
** Description      register nums entry in the dg database.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_register_entry_nums(UINT8 nums)
{
    UINT8 num_free = BTAPP_DG_ID_SPP_3 + 1;
    UINT8 i = 0;
    UINT8 registered = 0;

    if(nums == 0)
    {
        APPL_TRACE_ERROR0("DG can not accept zero num register");
        return;
    }

    for(i = 0; i <= BTAPP_DG_ID_SPP_3; i ++ )
    {
        if(btapp_dg_cb.app_cb[i].is_used == TRUE)
        {
           num_free --;
        }
    }

    if(nums > num_free)
    {
        APPL_TRACE_ERROR2("DG only remain %d entries to register, pls use no larger %d to register", num_free, num_free);
        return;
    }

    for(i = 0; i <= BTAPP_DG_ID_SPP_3; i ++)
    {
        if(btapp_dg_cb.app_cb[i].is_used == FALSE)
        {
            btapp_dg_register_entry(i);
            registered ++;
        }

        if(registered == nums)
        {
            break;
        }
    }
}

/*******************************************************************************
**
** Function         btapp_dg_deregister_all
**
** Description      deregister all entry in the dg database.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_deregister_all(void)
{
    UINT8 i = 0;

    for(i = 0; i <= BTAPP_DG_ID_SPP_3; i ++)
    {
        if(btapp_dg_cb.app_cb[i].is_used == TRUE)
        {
            btapp_dg_deregister_entry(i);
        }
    }
}

/*******************************************************************************
**
** Function         btapp_dg_entry_status
**
** Description      dump dg entry status in the dg database.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_entry_status(void)
{
    UINT8 i = 0;

    for(i = 0; i <= BTAPP_DG_ID_SPP_3; i ++)
    {
        APPL_TRACE_DEBUG2("DG entry :%d is %s", i, (btapp_dg_cb.app_cb[i].is_used == TRUE) ? "USED" : "UNUSED");
    }
}

/*******************************************************************************
**
** Function         btapp_dg_close_connection
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_close_connection(UINT8 conn_index)
{
    if(btapp_dg_cb.app_cb[conn_index].is_open == TRUE)
    {
        BTA_DgClose(btapp_dg_cb.app_cb[conn_index].api_handle);
    }
    else
    {
        APPL_TRACE_DEBUG0("btapp_dg_close_connection had closed");
    }
}

/*******************************************************************************
**
** Function         btapp_dg_set_device_authorized
**
** Description      Bond with the device
**
**
** Returns          void
*******************************************************************************/
void btapp_dg_set_device_authorized (tBTAPP_REM_DEVICE * p_device_rec)
{
    /* update BTA with this information.If a device is set as trusted, BTA will
    not ask application for authorization, */

    p_device_rec->is_trusted = TRUE;
    p_device_rec->trusted_mask |= BTA_SPP_SERVICE_MASK; //|BTA_DUN_SERVICE_MASK | BTA_FAX_SERVICE_MASK | BTA_LAP_SERVICE_MASK;
    btapp_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);

}

/*******************************************************************************
**
** Function         btapp_dg_cback
**
** Description      Callback function from BTA data gateway
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_cback(tBTA_DG_EVT event, tBTA_DG *p_data)
{
    APPL_TRACE_DEBUG1("btapp_dg_cback event:%d", event);

    if (event == BTA_DG_ENABLE_EVT)
    {
        //SerialPort, option
        //btapp_dg_register_entry(APP_ID_SPP);

#if 1
        /* Apple CarPlay */
        btapp_dg_register_entry(APP_ID_IAP2);
        btapp_dg_register_entry(APP_ID_CARPLAY);

        const UINT8 acc_iap_uuid[] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde,
                                      0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xff};
        const UINT8 carplay_uuid[] = {0xEC, 0x88, 0x43, 0x48, 0xCD, 0x41, 0x40, 0xA2,
                                      0x97, 0x27, 0x57, 0x5D, 0x50, 0xBF, 0x1F, 0xD3};

        tBT_UUID iap_uuid;
        iap_uuid.len = LEN_UUID_128;
        memcpy(iap_uuid.uu.uuid128, acc_iap_uuid, LEN_UUID_128);
        btapp_dm_add_custom_uuid(&iap_uuid);

        memcpy(iap_uuid.uu.uuid128, carplay_uuid, LEN_UUID_128);
        btapp_dm_add_custom_uuid(&iap_uuid);
#endif
    }

    if (event == BTA_DG_LISTEN_EVT)
    {
        APPL_TRACE_DEBUG2(" DG listen handle:%d, app_id:%d", p_data->listen.handle, p_data->listen.app_id);

        /* save the handle for the services */
        btapp_dg_cb.app_cb[p_data->listen.app_id].api_handle = p_data->listen.handle;
    }
    else if (event == BTA_DG_OPENING_EVT)
    {
        APPL_TRACE_DEBUG2(" DG opening handle:%d, app_id:%d", p_data->opening.handle, p_data->opening.app_id);

        /* save the handle */
        btapp_dg_cb.app_cb[p_data->opening.app_id].api_handle = p_data->opening.handle;

        /* set is_open flag */
        btapp_dg_cb.app_cb[p_data->opening.app_id].is_open = TRUE;
    }
    else if (event == BTA_DG_OPEN_EVT)
    {
        APPL_TRACE_DEBUG3(" DG open handle:%d, service:%d,app_id:%d", p_data->open.handle, p_data->open.service, p_data->open.app_id);
        APPL_TRACE_DEBUG6(" bd addr %x:%x:%x:%x:%x:%x", p_data->open.bd_addr[0], p_data->open.bd_addr[1],
                          p_data->open.bd_addr[2], p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);

        /* set is_open flag */
        btapp_dg_cb.app_cb[p_data->open.app_id].is_open = TRUE;
        btapp_dg_cb.app_cb[p_data->open.app_id].mtu     = p_data->open.mtu;

        /* test IAP2 */
        //if(p_data->open.app_id == APP_ID_IAP2)
        //    btapp_dg_iap2_test_echo();
    }
    else if (event == BTA_DG_CLOSE_EVT)
    {
        APPL_TRACE_DEBUG1(" DG close handle:%d", p_data->close.handle);

        /* if this is a close event when the connection went down */
        if(btapp_dg_cb.app_cb[p_data->close.app_id].is_open)
        {
            /* clear is_open flag */
            btapp_dg_cb.app_cb[p_data->close.app_id].is_open = FALSE;
        }
    }
}

/*******************************************************************************
**
** Function         btapp_dg_timer_cback
**
** Description      Callback function from dg timer
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_timer_cback(void *p_tle)
{
    APPL_TRACE_DEBUG0("btapp_dg_timer_cback");
}

/*******************************************************************************
**
** Function         btapp_dg_spp_send_data
**
** Description      send data to peer device
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_spp_send_data(UINT8 app_id, UINT8* pbuf, UINT16 len)
{
    BUFFER_Q* p_q;
    BT_HDR* p_msg = NULL;
    UINT8 put_data = 0;
    UINT16 sent_len = 0;

    if(btapp_dg_cb.app_cb[app_id].is_open == TRUE)
    {
        do
        {
            p_q = &btapp_dg_cb.app_cb[app_id].sent_data_q;

            if(p_q->count <= BTAPP_DG_SENT_Q_MAX_CNTS)
            {
                p_msg = GKI_getbuf(len + L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET + sizeof(BT_HDR));
                if(p_msg != NULL)
                {
                    p_msg->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET;
                    sent_len = (len > btapp_dg_cb.app_cb[app_id].mtu) ? btapp_dg_cb.app_cb[app_id].mtu : len;
                    memcpy((UINT8*)((UINT8*)(p_msg + 1) + p_msg->offset), pbuf, sent_len);
                    p_msg->len    = sent_len;
                    GKI_enqueue(p_q, p_msg);
                    put_data = TRUE;

                    //notify dg co to handle the rx buffer which pass from application layer.
                    bta_dg_ci_rx_ready(btapp_dg_cb.app_cb[app_id].port_handle);
                }
                else
                {
                    APPL_TRACE_ERROR2("%s app_id:%d can't get gki buffers", __FUNCTION__, app_id);
                    break;
                }
            }
            else
            {
                GKI_delay(1);
            }
        }
        while(!put_data);
    }
    else
    {
        APPL_TRACE_ERROR1("btapp_dg_spp_send_data app_id:%d not opened by peer yet or pending", app_id);
    }
}

/*******************************************************************************
**
** Function         btapp_dg_spp_rcv_data
**
** Description      receive data from peer device
**                  pbuf content need to be copied immediately, since the API return,the pointer will be free.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_spp_rcv_data(UINT8 app_id, UINT8* pbuf, UINT16 len)
{
    UINT16 i;

    APPL_TRACE_DEBUG2("btapp_dg_spp_rcv_data app_id:%d, len:%d", app_id, len);

    printf("\r\n========%s Recv========\r\n", app_id == APP_ID_IAP2?    "IAP2":
                                              app_id == APP_ID_CARPLAY? "CARPLAY" :
                                                                        "SPP");
    for(i=0;i<len;)
    {
        printf("%02x ", pbuf[i++]);
        if(i%8==0)
            printf("\n");
    }
    printf("\r\n====================\r\n");
    
    if(btapp_dg_cb.parseDataCB)
        btapp_dg_cb.parseDataCB(pbuf, len);
}

/*******************************************************************************
**
** Function         btapp_dg_spp_test_tx
**
** Description      test spp transmit data to peer.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_spp_test_send(void)
{
    UINT8 test_buf[] = { 0xFF, 0x55, 0x02, 0x00, 0xee, 0x10 };
    UINT8 i = 0;
    for(i =0;  i <= BTAPP_DG_ID_SPP_3; i ++)
    {
        if(btapp_dg_cb.app_cb[i].is_open == TRUE)
        {
            btapp_dg_spp_send_data(i, test_buf, sizeof(test_buf));
        }
    }
}

void btapp_dg_iap2_test_echo(void)
{
    UINT8 test_buf[] = { 0xFF, 0x55, 0x02, 0x00, 0xee, 0x10 };

    if(btapp_dg_cb.app_cb[APP_ID_IAP2].is_open == TRUE)
    {
        btapp_dg_spp_send_data(APP_ID_IAP2, test_buf, sizeof(test_buf));
    }
}

bool btapp_dg_spp_is_link(UINT8 app_id)
{
    return btapp_dg_cb.app_cb[app_id].is_open;
}

void btapp_dg_spp_set_parseCB(UINT8 app_id, btapp_dg_parsedataCB_t parseDataCB)
{
    btapp_dg_cb.parseDataCB = parseDataCB;
}

/*******************************************************************************
**
** Function         btapp_dg_spp_send_raw
**
** Description      test spp transmit data to peer.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_spp_send_raw(UINT8 test_cnt, UINT16 send_interval, UINT8* buf, UINT16 len)
{
    UINT8 i =0, j = 0;

    for(j = 0; j < test_cnt; j ++)
    {
        for(i =0;  i <= BTAPP_DG_ID_SPP_3; i ++)
        {
            if(btapp_dg_cb.app_cb[i].is_open == TRUE)
            {
                while(btapp_dg_cb.app_cb[i].rx_sent == TRUE)
                  GKI_delay(1);

                btapp_dg_spp_send_data(i, buf, len);
            }
        }

        GKI_delay(send_interval);
    }
}

/*******************************************************************************
**
** Function         btapp_dg_spp_send_tht
**
** Description      test spp transmit data throughput.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dg_spp_send_tht(UINT8 app_id, UINT16 test_cnt)
{
    UINT8 i = 0;
    UINT8* buf;
    UINT16 len;
    UINT8 test_data[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', };

    if(btapp_dg_cb.app_cb[app_id].is_open == TRUE)
    {
        len = btapp_dg_cb.app_cb[app_id].mtu;
        buf = GKI_getbuf(len);
        if(buf != NULL)
        {
            for(i = 0; i < test_cnt; i ++)
            {
                APPL_TRACE_DEBUG1("test_cnt:%d", i);
                memset(buf, test_data[i%10], len);
                btapp_dg_spp_send_data(app_id, buf, len);
            }

            GKI_freebuf(buf);
        }
    }
    else
    {
        APPL_TRACE_ERROR1("btapp_dg_spp_send_tht app_id:%d not open yet", app_id);
    }
}

#endif
