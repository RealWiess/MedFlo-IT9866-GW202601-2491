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
#include "ite/ith.h"
#include "ite/itp.h"

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

#define HCI_RX_TASK

#ifdef CFG_DBG_UART0
#undef CFG_UART0_INTR
#undef CFG_UART0_DMA
#undef CFG_UART0_FIFO
#undef CFG_UART0_ENABLE
#endif
#ifdef CFG_DBG_UART1
#undef CFG_UART1_INTR
#undef CFG_UART1_DMA
#undef CFG_UART1_FIFO
#undef CFG_UART1_ENABLE
#endif
#ifdef CFG_DBG_UART2
#undef CFG_UART2_INTR
#undef CFG_UART2_DMA
#undef CFG_UART2_FIFO
#undef CFG_UART2_ENABLE
#endif
#ifdef CFG_DBG_UART3
#undef CFG_UART3_INTR
#undef CFG_UART3_DMA
#undef CFG_UART3_FIFO
#undef CFG_UART3_ENABLE
#endif
#ifdef CFG_DBG_UART4
#undef CFG_UART4_INTR
#undef CFG_UART4_DMA
#undef CFG_UART4_FIFO
#undef CFG_UART4_ENABLE
#endif
#ifdef CFG_DBG_UART5
#undef CFG_UART5_INTR
#undef CFG_UART5_DMA
#undef CFG_UART5_FIFO
#undef CFG_UART5_ENABLE
#endif

#if defined(CFG_UART0_ENABLE)
#define TEST_PORT       ITP_DEVICE_UART0
#define TEST_ITH_PORT	ITH_UART0
#define TEST_DEVICE     itpDeviceUart0
#define TEST_BAUDRATE   CFG_UART0_BAUDRATE
#define TEST_GPIO_RX    CFG_GPIO_UART0_RX
#define TEST_GPIO_TX    CFG_GPIO_UART0_TX
#elif defined(CFG_UART1_ENABLE)
#define TEST_PORT       ITP_DEVICE_UART1
#define TEST_ITH_PORT	ITH_UART1
#define TEST_DEVICE     itpDeviceUart1
#define TEST_BAUDRATE   CFG_UART1_BAUDRATE
#define TEST_GPIO_RX    CFG_GPIO_UART1_RX
#define TEST_GPIO_TX    CFG_GPIO_UART1_TX
#elif defined(CFG_UART2_ENABLE)
#define TEST_PORT       ITP_DEVICE_UART2
#define TEST_ITH_PORT	ITH_UART2
#define TEST_DEVICE     itpDeviceUart2
#define TEST_BAUDRATE   CFG_UART2_BAUDRATE
#define TEST_GPIO_RX    CFG_GPIO_UART2_RX
#define TEST_GPIO_TX    CFG_GPIO_UART2_TX
#elif defined(CFG_UART3_ENABLE)
#define TEST_PORT       ITP_DEVICE_UART3
#define TEST_ITH_PORT	ITH_UART3
#define TEST_DEVICE     itpDeviceUart3
#define TEST_BAUDRATE   CFG_UART3_BAUDRATE
#define TEST_GPIO_RX    CFG_GPIO_UART3_RX
#define TEST_GPIO_TX    CFG_GPIO_UART3_TX
#elif defined(CFG_UART4_ENABLE)
#define TEST_PORT       ITP_DEVICE_UART4
#define TEST_ITH_PORT	ITH_UART4
#define TEST_DEVICE     itpDeviceUart4
#define TEST_BAUDRATE   CFG_UART4_BAUDRATE
#define TEST_GPIO_RX    CFG_GPIO_UART4_RX
#define TEST_GPIO_TX    CFG_GPIO_UART4_TX
#elif defined(CFG_UART5_ENABLE)
#define TEST_PORT       ITP_DEVICE_UART5
#define TEST_ITH_PORT	ITH_UART5
#define TEST_DEVICE     itpDeviceUart5
#define TEST_BAUDRATE   CFG_UART5_BAUDRATE
#define TEST_GPIO_RX    CFG_GPIO_UART5_RX
#define TEST_GPIO_TX    CFG_GPIO_UART5_TX
#endif

#if defined(CFG_UART0_INTR) || defined(CFG_UART1_INTR)\
	|| defined(CFG_UART2_INTR) || defined(CFG_UART3_INTR)\
	|| defined(CFG_UART4_INTR) || defined(CFG_UART5_INTR)
#define TEST_MODE UART_INTR_MODE
#endif

#if defined(CFG_UART0_DMA) || defined(CFG_UART1_DMA)\
	|| defined(CFG_UART2_DMA) || defined(CFG_UART3_DMA)\
	|| defined(CFG_UART4_DMA) || defined(CFG_UART5_DMA)
#define TEST_MODE UART_DMA_MODE
#endif

#if defined(CFG_UART0_FIFO) || defined(CFG_UART1_FIFO)\
	|| defined(CFG_UART2_FIFO) || defined(CFG_UART3_FIFO)\
	|| defined(CFG_UART4_FIFO) || defined(CFG_UART5_FIFO)
#define TEST_MODE UART_FIFO_MODE
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
