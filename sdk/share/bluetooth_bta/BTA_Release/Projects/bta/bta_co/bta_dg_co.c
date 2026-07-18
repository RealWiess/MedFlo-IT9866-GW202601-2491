/*****************************************************************************
**
**  Name:           bta_dg_co.c
**
**  Description:    This file contains the data gateway callout function
**                  implementation for Insight.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

#if (defined BTA_DG_INCLUDED) && (BTA_DG_INCLUDED == TRUE)
extern void btapp_dg_timer_cback(void *p_tle);

/*******************************************************************************
**
** Function         bta_dg_co_init
**
** Description      This callout function is executed by DG when a server is
**                  started by calling BTA_DgListen().  This function can be
**                  used by the phone to initialize data paths or for other
**                  initialization purposes.  The function must return the
**                  data flow mask as described below.
**
**
** Returns          Data flow mask.
**
*******************************************************************************/
UINT8 bta_dg_co_init(UINT16 port_handle, UINT8 app_id)
{
    APPL_TRACE_DEBUG2("bta_dg_co_init port_handle:%d app_id:%d", port_handle, app_id);

    return (BTA_DG_RX_PULL | BTA_DG_TX_PULL);
}

/*******************************************************************************
**
** Function         bta_dg_co_open
**
** Description      This function is executed by DG when a connection to a
**                  server is opened.  The phone can use this function to set
**                  up data paths or perform any required initialization or
**                  set up particular to the connected service.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_open(UINT16 port_handle, UINT8 app_id, tBTA_SERVICE_ID service, UINT16 mtu)
{
    tBTAPP_DG_APP_CB *p_cb = &btapp_dg_cb.app_cb[app_id];

    APPL_TRACE_DEBUG2("bta_dg_co_open port_handle:%d app_id:%d", port_handle, app_id);

    p_cb->port_handle = port_handle;

    if((service == BTA_SPP_SERVICE_ID) && (btapp_cfg.spp_loopback_mode == TRUE))
    {

        GKI_init_q(&p_cb->loopback_data_q );
        p_cb->data_send = 0;
        p_cb->prev_data_send = 0;
        p_cb->data_recvd = 0;
        p_cb->prev_data_recvd = 0;
        p_cb->time_taken = 0;

        p_cb->data_test_tle.p_cback = btapp_dg_timer_cback;
        p_cb->data_test_tle.param = app_id;
        btapp_start_timer(&p_cb->data_test_tle, 0, BTAPP_DG_DATA_TEST_TOUT);

    }
    else if((service == BTA_SPP_SERVICE_ID) && (btapp_cfg.spp_senddata_mode == TRUE))
    {

        p_cb->data_send = 0;
        p_cb->prev_data_send = 0;
        p_cb->data_recvd = 0;
        p_cb->prev_data_recvd = 0;
        p_cb->time_taken = 0;
        p_cb->data_pattern = 0;

        p_cb->data_test_tle.p_cback = btapp_dg_timer_cback;
        p_cb->data_test_tle.param = app_id;
        btapp_start_timer(&p_cb->data_test_tle, 0, BTAPP_DG_DATA_TEST_TOUT);
        bta_dg_ci_rx_ready(p_cb->port_handle);

    }
    else
    {
        APPL_TRACE_DEBUG4("bta_dg_co_open port_handle:%d, app_id:%d, service:%d, mtu:%d",\
                                          port_handle, app_id, service, mtu);

        GKI_init_q(&p_cb->sent_data_q);
    }
}

/*******************************************************************************
**
** Function         bta_dg_co_close
**
** Description      This function is called by DG when a connection to a
**                  server is closed.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_close(UINT16 port_handle, UINT8 app_id)
{
    tBTAPP_DG_APP_CB *p_cb = &btapp_dg_cb.app_cb[app_id];
    BUFFER_Q* p_q;
    BT_HDR * p_buf ;

    APPL_TRACE_DEBUG2("bta_dg_co_close port_handle:%d app_id:%d", port_handle, app_id);

    if((p_cb->service_id == BTA_SPP_SERVICE_ID) && (btapp_cfg.spp_loopback_mode == TRUE))
    {

        while((p_buf = (BT_HDR *)GKI_dequeue(&p_cb->loopback_data_q)) != NULL)
            GKI_freebuf(p_buf);

        btapp_stop_timer(&p_cb->data_test_tle);
    }
    else
    {
        /* Free the sent_data_q if exist any elements in the queue */
        p_q = &p_cb->sent_data_q;
        while(p_q->count)
        {
            GKI_freebuf(GKI_dequeue(p_q));
        }
    }
}

/*******************************************************************************
**
** Function         bta_dg_co_tx_path
**
** Description      This function is called by DG to transfer data on the
**                  TX path; that is, data being sent from BTA to the phone.
**                  This function is used when the TX data path is configured
**                  to use the pull interface.  The implementation of this
**                  function will typically call Bluetooth stack functions
**                  PORT_Read() or PORT_ReadData() to read data from RFCOMM
**                  and then a platform-specific function to send data that
**                  data to the phone.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_tx_path(UINT16 port_handle, UINT8 app_id)
{
    BT_HDR          *p_buf;
    UINT16          write_count;
    int             status;
    UINT16          port_errors;
    tPORT_STATUS    port_status;

    tBTAPP_DG_APP_CB *p_cb = &btapp_dg_cb.app_cb[app_id];

    APPL_TRACE_DEBUG2("bta_dg_co_tx_path app_id:%d, port_handle:%d", app_id, port_handle);

    if((p_cb->service_id == BTA_SPP_SERVICE_ID) && (btapp_cfg.spp_loopback_mode == TRUE))
    {
        do
        {
            if ((status = PORT_Read(port_handle, &p_buf)) != PORT_SUCCESS)
            {
                p_buf = NULL;
                if (status == PORT_LINE_ERR)
                {
                    PORT_ClearError(port_handle, &port_errors, &port_status);
                }
            }
            if (p_buf != NULL)
            {
                p_cb->data_recvd += p_buf->len;
                GKI_enqueue(&p_cb->loopback_data_q, p_buf);
            }

        }while (p_buf != NULL);

        if (GKI_IS_QUEUE_EMPTY(&p_cb->loopback_data_q) == FALSE)
        {
            bta_dg_ci_rx_ready(p_cb->port_handle);
        }

    }
    else if((p_cb->service_id == BTA_SPP_SERVICE_ID) && (btapp_cfg.spp_senddata_mode == TRUE))
    {
        do
        {
            if ((status = PORT_Read(port_handle, &p_buf)) != PORT_SUCCESS)
            {
                p_buf = NULL;
                if (status == PORT_LINE_ERR)
                {
                    PORT_ClearError(port_handle, &port_errors, &port_status);
                }
            }
            if (p_buf != NULL)
            {
                p_cb->data_recvd += p_buf->len;
                /* ignore the data received from peer */
                GKI_freebuf(p_buf);
            }

        }while (p_buf != NULL);
    }
    else
    {
        p_cb->tx_sent = FALSE;

        /* read next data buffer from RFCOMM */
        if ((status = PORT_Read(port_handle, &p_buf)) != PORT_SUCCESS)
        {
            p_buf = NULL;
            if (status == PORT_LINE_ERR)
            {
                PORT_ClearError(port_handle, &port_errors, &port_status);
            }
        }
        if (p_buf != NULL)
        {
            p_cb->data_recvd += p_buf->len;
            /* post data to application layer what from peer device */
            btapp_dg_spp_rcv_data(app_id, ((UINT8*)(p_buf + 1) + p_buf->offset), p_buf->len);

            GKI_freebuf(p_buf);
        }
    }
}

/*******************************************************************************
**
** Function         bta_dg_co_rx_path
**
** Description      This function is called by DG to transfer data on the
**                  RX path; that is, data being sent from the phone to BTA.
**                  This function is used when the RX data path is configured
**                  to use the pull interface.  The implementation of this
**                  function will typically call a platform-specific function
**                  to read data from the phone and then call Bluetooth stack
**                  functions PORT_Write() or PORT_WriteData() to send data
**                  to RFCOMM.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_rx_path(UINT16 port_handle, UINT8 app_id, UINT16 mtu)
{
    BT_HDR          *p_buf;
    BT_HDR          *p_data;
    UINT16          serial_len;
    tBTAPP_DG_APP_CB *p_cb = &btapp_dg_cb.app_cb[app_id];
    int             status;
    UINT16          port_errors;
    tPORT_STATUS    port_status;
    APPL_TRACE_DEBUG2("bta_dg_co_rx_path port_handle:%d, mtu:%d", port_handle, mtu);

    if((p_cb->service_id == BTA_SPP_SERVICE_ID) && (btapp_cfg.spp_loopback_mode == TRUE))
    {
        if((p_buf = (BT_HDR *)GKI_dequeue(&p_cb->loopback_data_q)) != NULL)
        {

            if ((p_data = (BT_HDR *) GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) != NULL)
            {
                p_data->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET;
                p_data->len = p_buf->len;
                p_cb->data_send += p_data->len;
                memcpy(((UINT8 *)(p_data + 1) + p_data->offset), ((UINT8 *)(p_buf + 1) + p_buf->offset), p_buf->len);
                status = PORT_Write(port_handle, p_data);
                if (status == PORT_LINE_ERR)
                {
                    PORT_ClearError(port_handle, &port_errors, &port_status);
                }
            }

            GKI_freebuf(p_buf);

        }
    }
    else if((p_cb->service_id == BTA_SPP_SERVICE_ID) && (btapp_cfg.spp_senddata_mode == TRUE))
    {
        if (btapp_dg_cb.flowed_off)
        {
            btapp_dg_cb.flowed_port_handle = port_handle;
            return;
        }

        if ((p_data = (BT_HDR *) GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) != NULL)
        {
            p_data->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET;
            p_data->len = mtu > (GKI_get_buf_size(p_data)-p_data->offset)? (GKI_get_buf_size(p_data)-p_data->offset) : mtu;
            memset(((UINT8 *)(p_data + 1) + p_data->offset), p_cb->data_pattern, p_data->len);
            status = PORT_Write(port_handle, p_data);
            if (status == PORT_LINE_ERR)
            {
                PORT_ClearError(port_handle, &port_errors, &port_status);
            }
            else
            {
                p_cb->data_send += p_data->len;
            }
            p_cb->data_pattern++;
        }
    }
    else
    {
        if((p_buf = (BT_HDR *)GKI_dequeue(&p_cb->sent_data_q)) != NULL)
        {
            status = PORT_Write(port_handle, p_buf);
            if (status == PORT_LINE_ERR)
            {
                PORT_ClearError(port_handle, &port_errors, &port_status);
            }
        }
    }
}

/*******************************************************************************
**
** Function         bta_dg_co_tx_write
**
** Description      This function is called by DG to send data to the phone
**                  when the TX path is configured to use a push interface.
**                  The implementation of this function must copy the data to
**                  the phone's memory.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_tx_write(UINT16 port_handle, UINT8 app_id, UINT8 *p_data, UINT16 len)
{
    APPL_TRACE_DEBUG1("bta_dg_co_tx_write port_handle:%d", port_handle);
}

/*******************************************************************************
**
** Function         bta_dg_co_tx_writebuf
**
** Description      This function is called by DG to send data to the phone
**                  when the TX path is configured to use a push interface with
**                  zero copy.  The phone must free the buffer using function
**                  GKI_freebuf() when it is through processing the buffer.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_tx_writebuf(UINT16 port_handle, UINT8 app_id, BT_HDR *p_buf)
{
    APPL_TRACE_DEBUG1("bta_dg_co_tx_writebuf port_handle:%d", port_handle);
}

/*******************************************************************************
**
** Function         bta_dg_co_rx_flow
**
** Description      This function is called by DG to enable or disable
**                  data flow on the RX path when it is configured to use
**                  a push interface.  If data flow is disabled the phone must
**                  not call bta_dg_ci_rx_write() or bta_dg_ci_rx_writebuf()
**                  until data flow is enabled again.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_rx_flow(UINT16 port_handle, UINT8 app_id, BOOLEAN enable)
{
    APPL_TRACE_DEBUG2("bta_dg_co_rx_flow port_handle:%d enable:%d", port_handle, enable);
}

/*******************************************************************************
**
** Function         bta_dg_co_control
**
** Description      This function is called by DG to send RS-232 signal
**                  information to the phone.  This function allows these
**                  signals to be propagated from the RFCOMM channel to the
**                  phone.  If the phone does not use these signals the
**                  implementation of this function can do nothing.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_control(UINT16 port_handle, UINT8 app_id, UINT8 signals, UINT8 values)
{
    APPL_TRACE_DEBUG3("bta_dg_co_control port_handle:%d signals:0x%x values:0x%x",
                      port_handle, signals, values);
}

#endif
