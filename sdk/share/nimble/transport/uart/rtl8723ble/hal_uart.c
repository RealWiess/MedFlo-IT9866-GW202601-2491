#include "hal/hal_uart.h"
#include "os/os.h"
#include "uart/uart.h"
#include "hci_if.h"
#include "hci_if_task.h"
#include <stdio.h>
//#define DBG
int(*nim_tx_cb)(void*);
int(*nim_rx_cb)(void*, uint8_t);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int hal_uart_init_cbs(int port,
	tx_cb cb_tx, void* arg_tx,
	rx_cb cb_rx, void* arg_rx)
{
	printf("!!!!!!UART init\n");
	nim_tx_cb = cb_tx;
	nim_rx_cb = cb_rx;
	return 0;
}

bool hci_callback(uint8_t evt, bool status, uint8_t *p_buf, uint32_t len)
{
	bool result = true;

	switch (evt)
	{
	case HCI_IF_EVT_OPENED:     /* hci I/F open completed */
#ifdef DBG
		printf("hci_if_callback HCI_IF_EVT_OPENED %x:\r\n", status);
#endif
	case HCI_IF_EVT_CLOSED:
#ifdef DBG
		printf("hci_if_callback HCI_IF_EVT_CLOSED %x:\r\n", status);
#endif
		break;
	case HCI_IF_EVT_DATA_IND:
#ifdef DBG
		printf("hci_if_callback HCI_IF_EVT_DATA_IND %x:\r\n", status);
		printf("Receive Data:\r\n");
#endif
		for (int i = 0; i < len; i++)
		{
#ifdef DBG
			printf("0x%x ", p_buf[i]);
#endif
			nim_rx_cb((void*)0x1, p_buf[i]);
		}
#ifdef DBG
		printf("\r\n");
#endif
	case HCI_IF_EVT_DATA_XMIT:
#ifdef DBG
		printf("hci_if_callback HCI_IF_EVT_DATA_XMIT %x:\r\n", status);
#endif
		break;
	case HCI_IF_EVT_ERROR:
#ifdef DBG
		printf("hci_if_callback HCI_IF_EVT_ERROR %x:\r\n", status);
#endif
		break;
	default:
		printf("hci_if_callback other evt:%x\r\n", evt);
		break;
	}
	return result;
}


int hal_uart_config(
	int UART_PORT, int UART_BAUD, int UART_DATA_BITS,
	int UART_STOP_BITS, int UART_PARITY, int UART_FLOW_CTRL)
{
	//bte_init();
	int ret = hci_if_open((P_HCI_IF_CALLBACK)hci_callback);
	while (!hci_if_check_opened()) // wait for RTL8723 initalizing...
		usleep(1000 * 100);

	return true;
}

void hal_uart_start_tx(int a)
{
	pthread_mutex_lock(&mutex);
	int count = 0, tmp = -1;
	uint8_t hci_data[255] = { 0 };
	while (1)
	{
		tmp = nim_tx_cb((void*)0x11);
		if (tmp == -1)
			break;
		hci_data[count++] = (uint8_t)tmp;
	}

#ifdef DBG
	printf("Send Data:\r\n");
	for (int i = 0; i < count; i++)
	{
		printf("0x%x ", hci_data[i]);
	}
	printf("\r\n");
#endif

	if (hci_if_write(hci_data, count) == false)
	{
		printf("%s, %d:fail\r\n", __func__, __LINE__);
	}
	count = 0;
	memset(hci_data, 0, sizeof(hci_data));
	pthread_mutex_unlock(&mutex);
}

int hal_uart_close(int a)
{
	hci_if_deinit();

	rtkbt_disable();
}