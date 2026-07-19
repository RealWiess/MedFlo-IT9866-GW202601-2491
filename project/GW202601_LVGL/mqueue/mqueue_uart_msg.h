#ifndef MQUEUE_UART_MSG_H
#define MQUEUE_UART_MSG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>

#define UART_MSG_LEN    11
#define BODY_LEN        (UART_MSG_LEN - 3)

typedef union _UartMsgStruct {
    uint8_t uartByte[UART_MSG_LEN];
    struct{
        size_t lenBody;
        uint8_t controlId;
        union {
            uint8_t cmdId;
            uint16_t stdId;
            uint32_t extId;
        };
        uint8_t body[BODY_LEN];
    } signal;
} UartMsg_t;

#endif

