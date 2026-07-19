#include "SDL/SDL.h"
#include "ite/itp.h"
#include "scene.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "other_id_impl.h"
#include "uart_data.h"

void findUartMsgImpl(unsigned char* input, int cmdLen, int bodyLen,
                           CONTROL_ID_TYPE id)
{
    unsigned int cmdId = 0;
    unsigned char* bodyBuf = input + cmdLen;
    UartMsg_t uartMsg;

    if (1 == cmdLen)
        cmdId = input[0];

    uartMsg.signal.lenBody = bodyLen;
    uartMsg.signal.controlId = id;
    uartMsg.signal.cmdId = cmdId;

#if 0 
    printf("ctl id = %d, cmd id = 0x%X\n", id, cmdId);
    for (int i = 0; i < bodyLen; i++) {
        printf("%X ", input[i + cmdLen]);
    }
    printf("\n");
#endif

    if (bodyLen >= 0 && bodyLen <= BODY_LEN)
        memcpy(uartMsg.signal.body, bodyBuf, bodyLen);

    uartMsgQueueSend(&uartMsg);
}



