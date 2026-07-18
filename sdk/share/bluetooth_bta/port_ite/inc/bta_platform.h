#ifndef BTA_PLATFORM_H
#define BTA_PLATFORM_H

#include <stdint.h>
#include "bt_trace.h"

#include "ite/ith.h"
#include "ite/itp.h"
#include "uart/uart.h"

#define LINE_ENDING                     "\r\n"
#if 1
#define BRCM_PLATFORM_TRACE(...)    \
do \
{ \
 printf(__VA_ARGS__) ; \
} while(0)
#else
#define BRCM_PLATFORM_TRACE(...)    \
do \
{ \
 LogMsg(TRACE_TYPE_DEBUG, __VA_ARGS__) ; \
}while(0)
#endif

typedef struct bt_uart{
	UART_OBJ info;
	uint32_t cts;
	uint32_t rts;
	uint32_t reg_on;
	uint32_t host_wake;
	uint32_t device_wake;
}bt_bus_t;

#endif

