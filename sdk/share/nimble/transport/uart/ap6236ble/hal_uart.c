#include "hal/hal_uart.h"
#include "os/os.h"
#include "uart/uart.h"
#include "hci_if.h"

int(*nim_tx_cb)(void*);
int(*nim_rx_cb)(void*, uint8_t);

void* bt_init(void* arg);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int hal_uart_init_cbs(int port, 
	tx_cb cb_tx, void* arg_tx, 
	rx_cb cb_rx, void* arg_rx)
{ 
	nim_tx_cb = cb_tx;
	nim_rx_cb = cb_rx;
	return 0; 
}

static void *rxHandler(void* arg)
{
	unsigned char recv[64] = {};
	int i = -1, length = -1;
	while (1)
	{
		length = read(TEST_PORT, &recv, 64);
		for (int i = 0; i < length; i++)
		{
			nim_rx_cb((void*)0x1, recv[i]);
		}
		memset(recv, 0, 64);
	}
}

int hal_uart_config(
	int UART_PORT, int UART_BAUD, int UART_DATA_BITS, 
	int UART_STOP_BITS, int UART_PARITY, int UART_FLOW_CTRL)
{
	bt_init((void*)0xA5);// pass dummy arg

	int res;
	pthread_t task;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	res = pthread_create(&task, &attr, rxHandler, NULL);

	return true;
}

void hal_uart_start_tx(int a)
{
	pthread_mutex_lock(&mutex);
	int count = 0, tmp = -1;
	uint8_t hci_data[255] = { 0 };
	while(1)
    {
	    tmp = nim_tx_cb((void*)0x11);
	    if(tmp == -1)
	        break;
        hci_data[count++] = (uint8_t)tmp;
    }

	write(TEST_PORT, hci_data, count);

	count = 0;
	memset(hci_data, 0, sizeof(hci_data));
	pthread_mutex_unlock(&mutex);
}

int hal_uart_close(int a){ return 1; }