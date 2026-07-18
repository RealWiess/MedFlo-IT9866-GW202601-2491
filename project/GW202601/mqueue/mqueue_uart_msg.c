#include <assert.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "mqueue_uart_msg.h"

#define MAX_QUEUE_SIZE  8

static mqd_t inQueue = -1;
static volatile bool quit;

void uartMsgQueueInit() {
    struct mq_attr qattr;

    qattr.mq_flags = 0;
    qattr.mq_maxmsg = MAX_QUEUE_SIZE;
    qattr.mq_msgsize = sizeof(UartMsg_t);

    inQueue = mq_open("uart_msg", O_CREAT | O_NONBLOCK, 0644, &qattr);
    assert(inQueue != -1);

    quit = false;
}

void uartMsgQueueExit() {
    quit = true;
    mq_close(inQueue);
    inQueue = -1;
}

int MsgQueueReceive(UartMsg_t* ev) {
    assert(ev);

    if (quit)
        return 0;
    if (mq_receive(inQueue, (char*)ev, sizeof(UartMsg_t), 0) > 0)
        return 1;

    return 0;
}

int uartMsgQueueSend(UartMsg_t* ev) {
    assert(ev);

    if (quit)
        return -1;

    return mq_send(inQueue, (char*)ev, sizeof(UartMsg_t), 0);
}
