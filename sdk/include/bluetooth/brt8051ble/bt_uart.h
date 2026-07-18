#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "hci_tp_dbg.h"
#include "osif.h"

void hci_uart_rx_ind(uint32_t write_idx);

bool rtk_hci_ps_uart_init(uint8_t proto, void *rx_ind);

///RX////
uint16_t rtk_hci_uart_rx(uint8_t **data, uint16_t *num);

bool rtk_hci_ps_uart_deinit(void);

bool rtk_hci_ps_uart_tx(uint8_t *p_buf, uint16_t len, void *tx_cb);

bool rtk_hci_uart_set_baudrate(uint32_t baudrate);

