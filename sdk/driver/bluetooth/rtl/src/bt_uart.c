/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */
#include <stdbool.h>
#include <stdint.h>
#include "bt_uart.h"
#include "osif_tp.h"
#include "hci_tp_dbg.h"

#include "hci_uart.h"
#include "hci_ps.h"

rtk_hci_uart_config *hci_uart_config;
//config
#if 1
#define HCI_UART_RX_BUF_SIZE        4*1024   /* RX buffer size 4K */
#define HCI_UART_RX_ENABLE_COUNT    (HCI_UART_RX_BUF_SIZE - 2 * (1021 + 5))   /* Enable RX */
#define HCI_UART_RX_DISABLE_COUNT   (HCI_UART_RX_BUF_SIZE - 1021 - 5 - 10)   /* Disable RX */
#endif

static uint32_t  ring_buffer_size;
static uint32_t  ring_buffer_enable_size;
static uint32_t  ring_buffer_disable_size;
static uint32_t  *bt_write_idx_ptr;
static uint32_t  *bt_read_idx_ptr;
static void *rtk_task_rx_ind;
static void *ring_buffer_address_temp;

#ifdef HCI_RX_TASK
void hci_rx_task_init(rtk_hci_uart_config *rtk_hci_uart);
void hci_rx_task_deinit(void);
#endif

//ext
//===================HCI==UART=====================================

static uint32_t ringbuffer_rx_len(uint32_t write_idx, uint32_t read_idx, uint32_t ringbuffer_size)
{
    return (write_idx + ringbuffer_size - read_idx) % ringbuffer_size;
}

static uint32_t ringbuffer_space_len(uint32_t write_idx, uint32_t read_idx,
                                     uint32_t ringbuffer_size)
{
    return (read_idx + ringbuffer_size - write_idx - 1) % ringbuffer_size;
}

static uint32_t hci_rx_data_len()
{
    return ringbuffer_rx_len(*bt_write_idx_ptr, *bt_read_idx_ptr, ring_buffer_size);
}
static uint16_t hci_rx_space_len()
{
    return ringbuffer_space_len(*bt_write_idx_ptr, *bt_read_idx_ptr, ring_buffer_size);
}

void hci_uart_rx_ind(uint32_t write_idx)
{
    *bt_write_idx_ptr = write_idx;
    if (hci_rx_space_len() == 0)
    {
        hci_tp_err("rx_data is full");
        return;
    }

    if ((hci_rx_data_len() > ring_buffer_disable_size) && hci_uart_config->rx_disabled == false)
    {
        hci_tp_warn("====rx disable, data len %d, write_idx = %d, read_idx = %d\r\n", hci_rx_data_len(),
                    write_idx, *bt_read_idx_ptr);
        hci_uart_config->rx_disabled = true;
        rtk_hci_uart_rx_disable();
    }

    if (rtk_task_rx_ind)
    {
        ((P_HCI_UART_RX_IND)rtk_task_rx_ind)();
    }
}

bool rtk_hci_ps_uart_init(uint8_t proto, void *rx_ind)
{

    hci_uart_config = (rtk_hci_uart_config *) tp_osif_malloc(sizeof(rtk_hci_uart_config));
    memset(hci_uart_config, 0, sizeof(rtk_hci_uart_config));
    //hci_tp_err("hci_uart_config :%p, %x",hci_uart_config, sizeof(rtk_hci_uart_config));
    hci_uart_config->ring_buffer = (uint8_t *) tp_osif_malloc(HCI_UART_RX_BUF_SIZE);
    ring_buffer_address_temp = hci_uart_config->ring_buffer;
    memset(hci_uart_config->ring_buffer, 0, sizeof(HCI_UART_RX_BUF_SIZE));
    //hci_tp_err("hci_uart_config->ring_buffer :%p,%x",hci_uart_config->ring_buffer, HCI_UART_RX_BUF_SIZE);
    hci_uart_config->ring_buffer_size = HCI_UART_RX_BUF_SIZE;
    if ((hci_uart_config->ring_buffer == NULL) || (hci_uart_config == NULL))
    {
        hci_tp_err("hci_uart_config->ring_buffer: %p, hci_uart_config:%p", hci_uart_config->ring_buffer,
                   hci_uart_config);
    }
    hci_uart_config->rx_disabled = false;
    bt_write_idx_ptr = &hci_uart_config->write_idx_ptr;
    bt_read_idx_ptr  = &hci_uart_config->read_idx_ptr;
    ring_buffer_size = hci_uart_config->ring_buffer_size;
    rtk_task_rx_ind = rx_ind;
#define RTLBT_UART_H4    4
#define RTLBT_UART_H5    5
    if (proto == RTLBT_UART_H5)
    {
        hci_uart_config->parity = UART_EVEN;
        hci_uart_config->word_length = 9;
    }
    hci_uart_config->baudrate = 115200;
    hci_uart_config->flowctrl = true; //default true
    hci_uart_config->rx_ind = hci_uart_rx_ind;

    rtk_hci_uart_init(hci_uart_config);
#ifdef HCI_RX_TASK
    hci_rx_task_init(hci_uart_config);
#endif
    if (HCI_UART_RINGBUFFER_CHANGED & hci_uart_config->func_bit)
    {

        hci_tp_info("We use the HCI RINGBUFFER FROM USER:ringbuffer:%p, ringbuffersize:%d",
                    hci_uart_config->ring_buffer, hci_uart_config->ring_buffer_size);
        tp_osif_free(ring_buffer_address_temp);
        ring_buffer_size = hci_uart_config->ring_buffer_size;
    }
    else
    {
        if (ring_buffer_address_temp != hci_uart_config->ring_buffer)
        {
            while (true)
            {
                hci_tp_err("the func bit not set HCI_UART_RINGBUFFER_CHANGED, please set");
                tp_osif_delay(2000);
            }
        }
        //never support to here
    }
    /*TODO load the default value, or changed*/
    ring_buffer_enable_size = ring_buffer_size - 2 * (1021 + 5);
    ring_buffer_disable_size = ring_buffer_size - 1021 - 5 - 10;
    if (CHECK_SW(HCI_TP_DEBUG_HCI_UART_RX_IDX))
    {
        hci_tp_dbg("\r\nring_buffer: %p, rx_read_index: %x, write_idx: %x,ring_buffer_enable_size %x,ring_buffer_disable_size %x\r\n",
                   hci_uart_config->ring_buffer, *bt_read_idx_ptr, *bt_write_idx_ptr, ring_buffer_enable_size,
                   ring_buffer_disable_size);
    }
    //changed the size
    return true;
}
///RX////
uint16_t rtk_hci_uart_rx(uint8_t **data, uint16_t *num)
{
    rtk_hci_uart_config  *hci_uart_ptr = hci_uart_config;
    uint32_t rx_data_len;

    rx_data_len = hci_rx_data_len();
    if (rx_data_len >= HCI_UART_RX_DISABLE_COUNT)
    {
        hci_tp_warn("\r\n!!!!the rx_data_len maynot right!!!!!!rx_len: %x, rx_read_index: %x, write_idx: %x,\r\n",
                    rx_data_len, *bt_read_idx_ptr,
                    *bt_write_idx_ptr);
    }
    if (CHECK_SW(HCI_TP_DEBUG_HCI_UART_RX_IDX))
    {
        hci_tp_dbg("\r\nrx_len: %x, rx_read_index: %x, write_idx: %x,\r\n", rx_data_len, *bt_read_idx_ptr,
                   *bt_write_idx_ptr);
    }
    if (rx_data_len == 0)
    {
        //indicate more but get all package once will cause this, not a problem
        //hci_tp_info("rx ring no unreaded data, ri %x, wi %x\n", *bt_read_idx_ptr, *bt_write_idx_ptr);
        return 0;
    }
    *data = &hci_uart_ptr->ring_buffer[*bt_read_idx_ptr];
    if (*bt_read_idx_ptr + rx_data_len <= ring_buffer_size)
    {
        *num = rx_data_len;
    }
    else
    {
        *num = ring_buffer_size - *bt_read_idx_ptr;
    }
    //hci_tp_dbg("\r\n===9999=rx_len: %x, rx_read_index: %x, consumed: %x, *num : %x\r\n", rx_data_len, *bt_read_idx_ptr,rx_data_len,*num);
    *bt_read_idx_ptr += *num;
    *bt_read_idx_ptr %= ring_buffer_size;
    if (hci_uart_ptr->rx_disabled == true) /* flow control */
    {
        if (hci_rx_data_len() < ring_buffer_enable_size)
        {
            hci_uart_config->rx_disabled = false;
            rtk_hci_uart_rx_enable();
        }
    }

    if (CHECK_SW(HCI_TP_DEBUG_HCI_UART_RX))
    {
        hci_ps_util_hexdump('R', *data, *num);
    }
    return (rx_data_len - *num);
}

bool rtk_hci_ps_uart_deinit(void)
{
#ifdef HCI_RX_TASK
    hci_rx_task_deinit();
#endif
    rtk_hci_uart_deinit();
    if ((hci_uart_config->ring_buffer != NULL) && (hci_uart_config != NULL))
    {
        if (!(HCI_UART_RINGBUFFER_CHANGED & hci_uart_config->func_bit))
        {
            tp_osif_free(hci_uart_config->ring_buffer);
        }
        tp_osif_free(hci_uart_config);
    }
    else
    {
        hci_tp_err("WRONG hci_uart_config->ring_buffer :%p, hci_uart_config: %p",
                   hci_uart_config->ring_buffer, hci_uart_config);
    }
    hci_uart_config = NULL;
    return true;
}


bool rtk_hci_ps_uart_tx(uint8_t *p_buf, uint16_t len, void *tx_cb)
{
    if (CHECK_SW(HCI_TP_DEBUG_HCI_UART_TX))
    {
        hci_ps_util_hexdump('S', p_buf, len);
    }
    return rtk_hci_uart_tx(p_buf, len, (bool (*)(void))tx_cb);
}


bool rtk_hci_uart_set_baudrate(uint32_t baudrate)
{
#ifdef HCI_RX_TASK
    hci_rx_task_deinit();
#endif
    rtk_hci_uart_deinit();
    hci_uart_config->baudrate = baudrate;
    hci_uart_config->read_idx_ptr = 0;
    hci_uart_config->write_idx_ptr = 0;
    rtk_hci_uart_init(hci_uart_config);
#ifdef HCI_RX_TASK
    hci_rx_task_init(hci_uart_config);
#endif
    return true;
}
//====================================================
