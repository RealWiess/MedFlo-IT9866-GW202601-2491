#include <assert.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "mqueue_uart_in.h"

static mqd_t inQueue = -1;
static volatile bool quit;

void uartInQueueInit() {
    struct mq_attr qattr;

    qattr.mq_flags = 0;
    qattr.mq_maxmsg = 1;
    qattr.mq_msgsize = sizeof(uartInStruct);

    inQueue = mq_open("uart_in", O_CREAT | O_NONBLOCK, 0644, &qattr);
    assert(inQueue != -1);

    quit = false;
}

void uartInQueueExit() {
    quit = true;
    mq_close(inQueue);
    inQueue = -1;
}

int uartInQueueReceive(uartInStruct* ev) {
    assert(ev);

    if (quit)
        return 0;
    if (mq_receive(inQueue, (char*)ev, sizeof(uartInStruct), 0) > 0)
        return ev->readLen;

    return 0;
}

int uartInQueueSend(uartInStruct* ev) {
    assert(ev);

    if (quit)
        return -1;

    return mq_send(inQueue, (char*)ev, sizeof(uartInStruct), 0);
}
