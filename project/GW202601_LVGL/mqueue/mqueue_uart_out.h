#ifndef MQUEUE_UART_OUT_H
#define MQUEUE_UART_OUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_SIZE     1024

/* PARSER UART FUNCTION COMMUNICATE WITH UART IN BUFFER */
struct _uartOutStruct {
    uint32_t readLen;
    uint8_t outDataBuf[BUFFER_SIZE];
};

typedef struct _uartOutStruct uartOutStruct;

#endif

