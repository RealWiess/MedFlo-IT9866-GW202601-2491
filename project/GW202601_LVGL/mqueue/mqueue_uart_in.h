#ifndef MQUEUE_UART_IN_H
#define MQUEUE_UART_IN_H

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_SIZE     1024

/* PARSER UART FUNCTION COMMUNICATE WITH UART IN BUFFER */
struct _uartInStruct {
    uint32_t readLen;
    uint8_t inDataBuf[BUFFER_SIZE];
};

typedef struct _uartInStruct uartInStruct;

int uartInQueueReceive(uartInStruct* ev);

#endif

