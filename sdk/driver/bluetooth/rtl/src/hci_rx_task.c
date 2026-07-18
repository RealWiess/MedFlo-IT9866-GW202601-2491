/**
 *******************************************************************************
 * Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * @file     hci_rx_task.c
 * @brief    only for rock-chip
 * @details  none
 * @author   harvey_guo
 * @date     2020-3-2
 * @version  v0.2
 * *****************************************************************************
 */

#include <stdio.h>
#include "osif_tp.h"
#include "hci_tp_dbg.h"
#include "hci_uart.h"

#ifdef CFG_BUILD_SDK
#define TEST_MODE       (0)
#define TEST_PORT       (7 << 8)
#define TEST_ITH_PORT   0xD1600000
#define TEST_DEVICE     itpDeviceUart0
#define TEST_BAUDRATE   115200
#define TEST_GPIO_RX    29
#define TEST_GPIO_TX    30
#define CFG_GPIO_BT_REG_ON 27
#endif

#ifdef HCI_RX_TASK
void *rx_task_handle;

void hci_rx_task(void *p_param)
{
    uint8_t read_char = 0;
    bool error_posted = false;
    uint16_t package_length_count = 0;
    bool packet_mid_flag = false;

    uint32_t  ring_buffer_size = ((rtk_hci_uart_config *)p_param)->ring_buffer_size;
    uint8_t   *ring_buffer = ((rtk_hci_uart_config *)p_param)->ring_buffer;
    uint32_t  write_idx = 0;
    uint32_t  *read_idx_ptr = &(((rtk_hci_uart_config *)p_param)->read_idx_ptr);
    void (*rx_ind)(uint32_t write_idx) = ((rtk_hci_uart_config *)p_param)->rx_ind;

    rtk_hci_uart_config *rtk_hci_uart_config = p_param;

    while (1)
	{
		if (read(TEST_PORT, &read_char, 1))//if (!hci_get_char_block(&read_char, 0xffffffffUL)) //this function can be replaced by user interface
        {
            package_length_count++;
            ring_buffer[write_idx++] = read_char;
            write_idx %= rtk_hci_uart_config->ring_buffer_size;
            if (package_length_count > 1024)
            {
                tp_osif_printf("unexpected long package, unread length:%d\r\n",
                               (write_idx + ring_buffer_size - *read_idx_ptr) % ring_buffer_size);
                rx_ind(write_idx);
                package_length_count = 0;
            }
            if (read_char == 0xc0)
            {
                if (error_posted)
                {
                    tp_osif_printf("rx check back to normal, package length:%d\r\n", package_length_count);
                    error_posted = false;
                }
                if (packet_mid_flag == false)
                {
                    packet_mid_flag = true;
                    continue;
                }
                packet_mid_flag = false;
                if (rx_ind)
                {
                    //               os_if_printf("rx_ind:%x,%x\r\n",read_char, hci_uart_obj->rx_ind);
                    rx_ind(write_idx);
                    package_length_count = 0;
                }
                continue;
            }
            else
            {
                if (packet_mid_flag == false)
                {
                    if (!error_posted) //above user buffer size
                    {
                        tp_osif_printf("hci_get_char_block char check error %x\r\n", read_char);
                        error_posted = true;
                        tp_osif_delay(1);
                    }
                    packet_mid_flag = true;
                    continue;
                }
                continue;
            }
        }
        else
        {
            //tp_osif_printf("hci_get_char_block failed to block task: HCI RX\r\n");
            tp_osif_delay(1);
        }
    }
}

void hci_rx_task_init(rtk_hci_uart_config *rtk_hci_uart_config)
{
    tp_osif_task_create(&rx_task_handle, "HCI RX", hci_rx_task, rtk_hci_uart_config,
                        0x800, 6);
}

void hci_rx_task_deinit(void)
{
    if (rx_task_handle != NULL)
    {
        tp_osif_task_delete(rx_task_handle);
        rx_task_handle = NULL;
    }
}
#endif
