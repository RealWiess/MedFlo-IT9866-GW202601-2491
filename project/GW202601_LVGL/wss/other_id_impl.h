#ifndef OTHER_ID_IMPL_H
#define OTHER_ID_IMPL_H

#include "uart_data.h"
#include "mqueue_uart_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

void findSystemMsgImpl(unsigned char* input, int cmdLen, int bodyLen);
void findUartMsgImpl(unsigned char* input, int cmdLen, int bodyLen, CONTROL_ID_TYPE id);


void uartMsgQueueInit();
void uartMsgQueueExit();
int uartMsgQueueReceive(UartMsg_t* ev);
int uartMsgQueueSend(UartMsg_t* ev) ;


#ifdef __cplusplus
}
#endif

#endif // OTHER_ID_IMPL_H
