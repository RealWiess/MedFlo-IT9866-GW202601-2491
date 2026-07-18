#include <assert.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "mqueue_uart_out.h"

static mqd_t outQueue = -1;
static volatile bool quit;

void uartOutQueueInit() {
    struct mq_attr qattr;

    qattr.mq_flags = 0;
    qattr.mq_maxmsg = 1;
    qattr.mq_msgsize = sizeof(uartOutStruct);

    outQueue = mq_open("uart_out", O_CREAT | O_NONBLOCK, 0644, &qattr);
    assert(outQueue != -1);

    quit = false;
}

void uartOutQueueExit() {
    quit = true;
    mq_close(outQueue);
    outQueue = -1;
}

int uartOutQueueReceive(uartOutStruct* ev) {
    assert(ev);

    if (quit)
        return 0;
    if (mq_receive(outQueue, (char*)ev, sizeof(uartOutStruct), 0) > 0)
        return ev->readLen;

    return 0;
}

int uartOutQueueSend(uartOutStruct* ev) {
    assert(ev);

    if (quit)
        return -1;

    return mq_send(outQueue, (char*)ev, sizeof(uartOutStruct), 0);
}
