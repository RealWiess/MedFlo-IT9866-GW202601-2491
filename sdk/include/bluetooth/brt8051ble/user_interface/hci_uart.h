/**
 * Copyright (c) 2017, Realsil Semiconductor Corporation. All rights reserved.
 *
 */
#ifndef _HCI_UART_H_
#define _HCI_UART_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

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


typedef bool (*P_HCI_UART_RX_IND)(void);
typedef enum
{
    UART_NONE,
	UART_ODD,
    UART_EVEN,
} T_HCI_UART_PARITY;

#define HCI_UART_115200 115200
#define HCI_UART_921600 921600
#define HCI_UART_1500000 1500000

//FUNC_BIT
#define HCI_UART_RINGBUFFER_CHANGED 0x01

typedef struct
{
    T_HCI_UART_PARITY             parity;
    uint8_t                       word_length;
    uint32_t                      baudrate;
    bool                          flowctrl;
    bool                          rx_disabled;
    uint8_t                       *ring_buffer;
    uint32_t                      ring_buffer_size;
    uint32_t                      write_idx_ptr;
    uint32_t                      read_idx_ptr;
    void (*rx_ind)(uint32_t write_idx);
    uint32_t                      func_bit;
} rtk_hci_uart_config;

bool rtk_hci_uart_init(rtk_hci_uart_config *rtk_hci_uart);
bool rtk_hci_uart_deinit(void);

bool rtk_hci_uart_set_baudrate(uint32_t baudrate);

bool rtk_hci_uart_tx(uint8_t *p_buf, uint16_t len, bool (*tx_cb)(void));
bool rtk_hci_uart_rx_disable(void);
bool rtk_hci_uart_rx_enable(void);
//API FOR RX
//void rtk_hci_uart_rx_ind(uint32_t write_idx);

#ifdef __cplusplus
}
#endif

#endif /* _HCI_UART_H_ */
