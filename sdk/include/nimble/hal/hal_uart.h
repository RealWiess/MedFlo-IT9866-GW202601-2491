#include "ite/ith.h"
#include "ite/itp.h"
#include "uart/uart.h"

#ifdef CFG_BT_HAL_UART0
#define TEST_PORT       ITP_DEVICE_UART0
#define TEST_ITH_PORT	ITH_UART0
#define TEST_DEVICE     itpDeviceUart0
#define TEST_BAUDRATE   CFG_UART0_BAUDRATE
#define TEST_GPIO_RX    CFG_GPIO_UART0_RX
#define TEST_GPIO_TX    CFG_GPIO_UART0_TX
#endif

#define MYNEWT_VAL_BLE_HCI_UART_PORT		TEST_PORT
#define MYNEWT_VAL_BLE_HCI_UART_BAUD		CFG_UART0_BAUDRATE
#define MYNEWT_VAL_BLE_HCI_UART_DATA_BITS	8
#define MYNEWT_VAL_BLE_HCI_UART_STOP_BITS	1
#define MYNEWT_VAL_BLE_HCI_UART_PARITY		0
#define MYNEWT_VAL_BLE_HCI_UART_FLOW_CTRL	0

typedef int(*tx_cb) (void*);
typedef int(*rx_cb) (void*, uint8_t);

//int tx_char_cb(void *arg)

int hal_uart_init_cbs(int port, tx_cb cb_tx, void* a, rx_cb cb_rx, void* b);

int hal_uart_config(int, int, int, int, int, int);

void hal_uart_start_tx(int);

int hal_uart_close(int);