/**
  ******************************************************************************
  * File           : hci_if_task.c
  * Author         : Harvey_Guo
  ******************************************************************************
 * Copyright (C) 2019 Realtek Semiconductor Corporation
 */

/* Includes ------------------------------------------------------------------*/
#include "hci_if.h"
#include "hci_if_task.h"
#include "hci_protocol.h"

#include "osif_tp.h"
#include "hci_tp_dbg.h"

#include "hci_ps.h"

//#include "hci_board_if.h"
#include <version_hci.h>
#include <string.h>
#include "bt_uart.h"

#ifdef UPPERSTACK_LIB
#include "otp.h"
#endif
/* Private macro -------------------------------------------------------------*/
#ifdef HCI_IF_USE_ADDIN_CFG
#include "add_config.h"
#else
//#define HCI_IF_DEBUG_MESSAGE_ON
#define HCI_IF_TASK_PRIORITY 5
#endif

#ifdef HCI_IF_DEBUG_MESSAGE_ON
#define HCI_IF_ERR  hci_tp_err
#define HCI_IF_WARN hci_tp_warn
#define HCI_IF_INFO  hci_tp_info
#define HCI_IF_DBG  hci_tp_warn
#else
#define HCI_IF_ERR  hci_tp_err
#define HCI_IF_WARN(...)
#define HCI_IF_INFO(...)
#define HCI_IF_DBG(...)
#endif

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
    uint8_t    *p_buf;
    uint16_t    len;
} T_HCI_XMIT_DATA;

typedef struct T_HCI_IF_CTRL
{
    uint8_t                         state;
    P_HCI_IF_CALLBACK               upperstack_callback;      /* hci I/F event callback */

    void                            *hci_if_task_handle;   /* task handle */
    void                            *initializing_handle;  /* Timer / mutex */

    void                            *msg_q;         /* task msg queue */
    void                            *xmit_q;        /* tx req queue */
    void                            *cfm_q;         /* rx cfm queue */

    HCI_PROTOCOL                    *hci_protocol;

} HCI_IF_CTRL;
/* Private define ------------------------------------------------------------*/
#define HCI_IF_TASK_PRIORITY            5
#define HCI_IF_STACK_SIZE               0x400

#define HCI_IF_TIMER_ID_BASE            50
#define HCI_IF_INITIAL_TIMER_ID         HCI_IF_TIMER_ID_BASE + 0

/* Private variables ---------------------------------------------------------*/
static HCI_IF_CTRL          hci_if_ctrl = {0};
static uint8_t              hci_if_is_transmiting = 0;
/* Private function prototypes -----------------------------------------------*/

/* Private code --------------------------------------------------------------*/
void hci_if_send_message_to_ifctrl(uint8_t message)
{
    if (hci_if_ctrl.msg_q)
    {
        if ((hci_if_is_transmiting == true) && (message == HCI_IF_MSG_TX_UART_TRANSMIT))
        {
            HCI_IF_WARN("Tx required when is txing");
            return;
        }
        if (!tp_osif_msg_send(hci_if_ctrl.msg_q, &message, 0))
        {
            HCI_IF_ERR("Message send fail %d", message);
        }
        HCI_IF_DBG("msg_q send %d\r\n", message);
    }
}

bool hci_if_send_message_to_upperstack(T_HCI_IF_EVT evt, bool status, uint8_t *p_buf, uint32_t len)
{
    if (hci_if_ctrl.upperstack_callback)
    {
        return hci_if_ctrl.upperstack_callback(evt, status, p_buf, len);
    }
    HCI_IF_ERR("Upperstack reject message");
    return false;
}

bool hci_if_Tx_finish_callback(void)
{
    hci_if_is_transmiting = false;
    if (hci_if_ctrl.hci_protocol->tx_finish_callback != NULL)
    {
        return hci_if_ctrl.hci_protocol->tx_finish_callback();
    }
    return false;
}

/** Timout routine*/
void hci_if_ctrl_initial_timeout_routine(void *user_data)
{
    HCI_IF_ERR("The bt controller has no answer,hci if initial timeout,hci_if state =%d",
               hci_if_ctrl.state);
    hci_error_indicate(0);
    hci_if_send_message_to_ifctrl(HCI_IF_MSG_FAIL);
}

bool hci_if_ctrl_rx_ind(void)
{
    //HCI_IF_DBG("hci_if_ctrl_rx_ind indicated\r\n");
    if (hci_if_ctrl.hci_if_task_handle != NULL)
    {
        hci_if_send_message_to_ifctrl(HCI_IF_MSG_RX_IND_UNPACK);
        return true;
    }
    HCI_IF_WARN("Rx should not start before HCI ready, HCI state %d", hci_if_ctrl.state);
    return false;
}

void hci_if_ctrl_recv(void)
{
    uint16_t remainder;
    uint8_t *ptr = NULL;
    uint16_t count = 0;
    //HCI_IF_DBG("hci_if_ctrl_recv indicated\r\n");
    remainder = rtk_hci_uart_rx(&ptr, &count);

    if (count == 0)
    {
        return;
    }
    int ret =  hci_if_ctrl.hci_protocol->unpack(ptr, count);
    if (remainder)
    {
        //HCI_IF_DBG("More to recv,remainder %d", remainder);
        hci_if_send_message_to_ifctrl(HCI_IF_MSG_RX_IND_UNPACK);
    }
    if (hci_if_ctrl.state != HCI_IF_STATE_READY)
    {
        hci_if_ctrl.hci_protocol->connect_response(&hci_if_ctrl.state);
    }
    else if (hci_if_ctrl.hci_protocol->rx_finish_callback)
    {
        hci_if_ctrl.hci_protocol->rx_finish_callback();
    }
}

void hci_if_task(void *p_param)
{
    uint8_t msg;
    T_HCI_XMIT_DATA tx_data;
    uint8_t *p_buf;
    uint16_t len;
    (void)p_param;

    while (true)
    {
        if (tp_osif_msg_recv(hci_if_ctrl.msg_q, &msg, 0xFFFFFFFF) == true)
        {
            HCI_IF_DBG("msg_q recv %d\r\n", msg);
            switch (msg)
            {
            /* control */
            case HCI_IF_MSG_OPEN:
                HCI_IF_DBG("HCI_IF_MSG_OPEN\r\n");
                if (hci_if_ctrl.state == HCI_IF_STATE_UNINITIALIZED)
                {
                    hci_if_ctrl.state = HCI_IF_STATE_SYNC;
                    rtk_hci_pc_init(hci_if_ctrl_rx_ind);
                    tp_osif_timer_start(&hci_if_ctrl.initializing_handle);
                    hci_if_ctrl.hci_protocol->open();
                }
                else
                {
                    HCI_IF_ERR("hci re-open\r\n");
                }
                break;
            case HCI_IF_MSG_READY:
                HCI_IF_DBG("HCI_IF_MSG_READY\n\r");
                hci_if_ctrl.state = HCI_IF_STATE_READY;
                if (hci_if_ctrl.initializing_handle)
                {
                    tp_osif_timer_stop(&hci_if_ctrl.initializing_handle);
                    tp_osif_timer_delete(&hci_if_ctrl.initializing_handle);
                    hci_if_ctrl.initializing_handle = NULL;
                }
                hci_if_ctrl.upperstack_callback(HCI_IF_EVT_OPENED, true, NULL, 0);
                break;
            case HCI_IF_MSG_FAIL:
                HCI_IF_ERR("========hci fail !!!!  state %d =====\r\n", hci_if_ctrl.state);
                hci_if_ctrl.upperstack_callback(HCI_IF_EVT_ERROR, false, NULL, 0);
                if (hci_if_ctrl.initializing_handle)
                {
                    tp_osif_timer_stop(&hci_if_ctrl.initializing_handle);
                    tp_osif_timer_delete(&hci_if_ctrl.initializing_handle);
                    hci_if_ctrl.initializing_handle = NULL;
                }
                //hci_if_ctrl.state = HCI_IF_STATE_UNINITIALIZED;
                break;
            case HCI_IF_MSG_CLOSE:
                HCI_IF_DBG("HCI_IF_MSG_CLOSE\r\n");
                if ((hci_if_ctrl.state != HCI_IF_STATE_UNINITIALIZED) &&
                    (hci_if_ctrl.state != HCI_IF_STATE_CLOSING))
                {
                    hci_if_ctrl.state = HCI_IF_STATE_CLOSING;
                    rtk_hci_ps_uart_deinit();
                    hci_if_ctrl.hci_protocol->close();
                    hci_protocol_deinit();
                    hci_if_ctrl.upperstack_callback(HCI_IF_EVT_CLOSED, true, NULL, 0);
                    hci_if_ctrl.state = HCI_IF_STATE_UNINITIALIZED;
                    HCI_IF_WARN("hci_if closed\r\n");
                }
                break;
            /* tx */
            case HCI_IF_MSG_TX_PACK_REQ:
                if (tp_osif_msg_recv(hci_if_ctrl.xmit_q, &tx_data, 0) == true)
                {
                    //HCI_IF_DBG("HCI_IF_MSG_TX_PACK_REQ %x %x\n\r", tx_data.p_buf, tx_data.len);
                    hci_if_ctrl.hci_protocol->pack(tx_data.p_buf, tx_data.len);
                }
                break;
            case HCI_IF_MSG_TX_UART_TRANSMIT:
                if (hci_if_ctrl.hci_protocol->get_package_tx(&p_buf, &len))
                {
                    HCI_IF_DBG("HCI_IF_MSG_TX_UART_TRANSMIT %08x %02x\n\r", (uint32_t)p_buf, len);
                    hci_if_is_transmiting = true;
                    rtk_hci_ps_uart_tx(p_buf, len, hci_if_Tx_finish_callback);
                }
                break;
            case HCI_IF_MSG_TX_INFORM_UPPERSTACK:
                if (hci_if_ctrl.state == HCI_IF_STATE_READY)
                {
                    //HCI_IF_DBG("HCI_IF_MSG_TX_INFORM_UPPERSTACK %x %x\r\n", tx_data.p_buf, tx_data.len);
                    hci_if_ctrl.upperstack_callback(HCI_IF_EVT_DATA_XMIT, true, tx_data.p_buf, tx_data.len);
                    hci_if_ctrl.hci_protocol->tx_package_prune(tx_data.p_buf, tx_data.len);
                    hci_if_send_message_to_ifctrl(HCI_IF_MSG_TX_UART_TRANSMIT); // for if more tx package
                }
                break;
            /* rx */
            case HCI_IF_MSG_RX_IND_UNPACK:
                //HCI_IF_DBG("HCI_IF_MSG_RX_IND_UNPACK\r\n");
                hci_if_ctrl_recv();
                break;
            case HCI_IF_MSG_RX_INFORM_UPPERSTACK:
                if (hci_if_ctrl.hci_protocol->get_package_rx(&p_buf, &len))
                {
                    HCI_IF_DBG("HCI_IF_MSG_RX_INFORM_UPPERSTACK %08x %02x\n\r", (uint32_t)p_buf, len);
                    hci_if_ctrl.upperstack_callback(HCI_IF_EVT_DATA_IND, true, p_buf, len);
                    hci_if_send_message_to_ifctrl(HCI_IF_MSG_RX_INFORM_UPPERSTACK); // for if more rx package
                }
                break;
            case HCI_IF_MSG_RX_CFM:
                if (tp_osif_msg_recv(hci_if_ctrl.cfm_q, &p_buf, 0) == true)
                {
                    //HCI_IF_DBG("HCI_IF_MSG_RX_CFM %08x %02x\n\r", (uint32_t)p_buf, len);
                    hci_if_ctrl.hci_protocol->rx_package_prune((uint8_t *)p_buf, 0);
                }
                break;
            default:
                HCI_IF_ERR("hci_if_task: unknown msg 0x%02x", msg);
                break;
            }
        }
        //HCI_IF_DBG("hci_if_task round end %d\r\n", msg);
    }
}


/**
* @brief  upper use this api for start the procedure of transport layer
* @param  p_callback  the callback defined by upper stack for interaction.
* @param  p_context  some info defined by upper stack.
* @return  bool type indicate if the com open success
* @retval   true: the com open success
* @retval   false: the com open fail
* @note
*/
bool hci_if_open(P_HCI_IF_CALLBACK p_callback)
{
    //default;
    uint16_t hci_task_size = 4 * 1024;
    uint16_t hci_priority  = HCI_IF_TASK_PRIORITY;
    uint8_t rtl_count = 6;

//    rtk_hci_set_hci_task(&hci_task_size, &hci_priority);
    if (hci_if_ctrl.hci_if_task_handle == NULL)
    {
        hci_if_ctrl.state    = HCI_IF_STATE_UNINITIALIZED;
        hci_if_ctrl.upperstack_callback = p_callback;
        //hci_if.proto    = &hci_h5_proto;

        rtl_count -= tp_osif_msg_queue_create(&(hci_if_ctrl.msg_q), 64, sizeof(uint8_t));
        rtl_count -= tp_osif_msg_queue_create(&(hci_if_ctrl.xmit_q), 32, sizeof(T_HCI_XMIT_DATA));
        rtl_count -= tp_osif_msg_queue_create(&(hci_if_ctrl.cfm_q), 32, sizeof(uint8_t *));

#ifdef UPPERSTACK_LIB
        rtl_count -= os_task_create(&hci_if_ctrl.task_handle, "HCI I/F", hci_if_task, NULL,
                                    otp.upper.hci_task_size, otp.upper.hci_task_priority);
#else
        rtl_count -= tp_osif_task_create(&hci_if_ctrl.hci_if_task_handle, "HCI I/F", hci_if_task, NULL,
                                         hci_task_size, hci_priority);
#endif

#ifdef      RTK_COEX
        coext_init();
#endif

        rtl_count -= tp_osif_timer_create(&hci_if_ctrl.initializing_handle, "HCI_IF_Init",
                                          HCI_IF_INITIAL_TIMER_ID,
                                          12000, 0, hci_if_ctrl_initial_timeout_routine);
        /* protocal init */
        rtl_count -= hci_protocol_init(&(hci_if_ctrl.hci_protocol));

        if (rtl_count)
        {
            HCI_IF_ERR("unexpect here~~~~~~");
            tp_osif_task_delete(hci_if_ctrl.hci_if_task_handle);
            hci_if_ctrl.hci_if_task_handle = NULL;
            tp_osif_msg_queue_delete(&(hci_if_ctrl.msg_q));
            tp_osif_msg_queue_delete(&(hci_if_ctrl.xmit_q));
            tp_osif_msg_queue_delete(&(hci_if_ctrl.cfm_q));
            hci_protocol_deinit();
            return false;
        }
        else
        {
            hci_if_send_message_to_ifctrl(HCI_IF_MSG_OPEN);
            return true;
        }
    }
    else
    {
        HCI_IF_ERR("========hci_if_open: reopen, please deinit first");
    }
    return false;
}

void hci_if_deinit(void)
{
    switch (hci_if_ctrl.state)
    {
    case HCI_IF_STATE_SYNC:
    case HCI_IF_STATE_CONFIG:
    case HCI_IF_STATE_PATCH:
    case HCI_IF_STATE_READY:
        hci_if_send_message_to_ifctrl(HCI_IF_MSG_CLOSE);//no break,need delay
    case HCI_IF_STATE_CLOSING:
        while (hci_if_ctrl.state != HCI_IF_STATE_UNINITIALIZED)
        {
            tp_osif_delay(10);
        }
    case HCI_IF_STATE_UNINITIALIZED:
        break;
    }
    if (hci_if_ctrl.hci_if_task_handle != NULL)
    {
        tp_osif_task_delete(hci_if_ctrl.hci_if_task_handle);
        hci_if_ctrl.hci_if_task_handle = NULL;
    }
    if (hci_if_ctrl.msg_q)
    {
        tp_osif_msg_queue_delete(hci_if_ctrl.msg_q);
        hci_if_ctrl.msg_q = NULL;
    }
    if (hci_if_ctrl.xmit_q)
    {
        tp_osif_msg_queue_delete(hci_if_ctrl.xmit_q);
        hci_if_ctrl.xmit_q = NULL;
    }
    if (hci_if_ctrl.cfm_q)
    {
        tp_osif_msg_queue_delete(hci_if_ctrl.cfm_q);
        hci_if_ctrl.cfm_q = NULL;
    }
    tp_osif_printf("=======BT closeD==========");
}

void hci_if_del_task()
{
    hci_if_close();
}

/**
* @brief  upper use this api for stop the procedure of transport layer
* @param  none
* @return  bool type indicate if the com close success
* @retval   true: the com close success
* @retval   false: the com close fail
* @note
*/
bool hci_if_close(void)
{
    hci_if_send_message_to_ifctrl(HCI_IF_MSG_CLOSE);
    return true;
}

/**
* @brief  upper use this api for write the buffer to be sent by uart tx
* @param  p_buf: the buf to be sent
* @param  len: the len of the p_buf to be sent
* @return  bool type indicate if data to be sent success
* @retval   true: the data send success
* @retval   false: the data send fail
* @note
*/
bool hci_if_write(uint8_t *p_buf, uint32_t len)
{

    T_HCI_XMIT_DATA tx_data;
    /* HCI_PRINT_TRACE2("hci_if_write: buf %p, len %d", p_buf, len); */
    // HCI_PRINT_TRACE2("hci_if_write: buf %p, len %d", p_buf, len);

    tx_data.p_buf = p_buf;
    tx_data.len   = len;

    if (tp_osif_msg_send(hci_if_ctrl.xmit_q, &tx_data, 0) == true)
    {
        hci_if_send_message_to_ifctrl(HCI_IF_MSG_TX_PACK_REQ);
        return true;
    }
    else
    {
        HCI_IF_ERR("hci_if_write:ERROR: buf %p, len %ud", p_buf, len);
        return false;
    }
}

/**
* @brief  upper use this api for recv the buffer by uart rx
* @param  p_buf: the buf to recv
* @return  bool type indicate if data recv success
* @retval   true: the data recv success
* @retval   false: the data recv fail
* @note called by upperstack
*/
bool hci_if_confirm(uint8_t *p_buf)
{
    //HCI_IF_DBG("hci_if_confirm:  buf %p", p_buf);
    if (tp_osif_msg_send(hci_if_ctrl.cfm_q, &p_buf, 0) == true)
    {
        hci_if_send_message_to_ifctrl(HCI_IF_MSG_RX_CFM);
        return true;
    }
    else
    {
        return false;
    }
}

uint32_t hci_error_time = 0;
uint32_t hci_error_count = 0;
void hci_error_indicate(int level)
{
    if (hci_error_count++ > 10)
    {
        uint32_t system_time;
        system_time = osif_sys_time_get();
        if ((uint32_t)(system_time - hci_error_time) < 100)
        {
            hci_if_ctrl.upperstack_callback(HCI_IF_EVT_ERROR, false, (uint8_t *)NULL, 0);
            hci_error_count = 0;
            HCI_IF_ERR("\r\n======hci_error_indicate!!!!=======\r\n");
        }
        else
        {
            hci_error_count = 0;
            hci_error_time = system_time;
        }
    }
}

bool hci_get_version_info(char *p_ver, char *p_time, char *p_lib, uint32_t *p_gcid_l,
                          uint32_t *p_gcid_h)
{
    if (p_ver == NULL || p_time == NULL || p_lib == NULL || p_gcid_l == NULL || p_gcid_h == NULL)
    {
        return false;
    }
    memcpy(p_ver, VERSION_BUILD_STR, sizeof(VERSION_BUILD_STR));
    memcpy(p_time, BUILDING_TIME, sizeof(BUILDING_TIME));
    memcpy(p_lib, LIB_NAME, sizeof(LIB_NAME));
    *p_gcid_l = VERSION_GCID;
    *p_gcid_h = VERSION_GCIDH;
    return true;

}

bool hci_if_check_opened(void)
{
	return (hci_if_ctrl.state == HCI_IF_STATE_READY) ? true : false;
}