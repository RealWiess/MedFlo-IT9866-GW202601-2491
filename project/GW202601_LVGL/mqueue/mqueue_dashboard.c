#include <assert.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "mqueue_dashboard.h"

static mqd_t inQueue = -1;
static volatile bool quit;

void dashboardQueueInit() {
    struct mq_attr qattr;

    qattr.mq_flags = 0;
    qattr.mq_maxmsg = EXT_MAX_QUEUE_SIZE;
    qattr.mq_msgsize = sizeof(DashboardEvent_t);

    inQueue = mq_open("dashboard_in", O_CREAT | O_NONBLOCK, 0644, &qattr);
    assert(inQueue != -1);

    quit = false;
}

void dashboardQueueExit() {
    quit = true;
    mq_close(inQueue);
    inQueue = -1;
}

int dashboardQueueReceive(DashboardEvent_t* ev) {
    assert(ev);

    if (quit)
        return 0;

    if (mq_receive(inQueue, (char*)ev, ev->length, 0) > 0)
        return 1;

    return 0;
}

int dashboardQueueSend(DashboardEvent_t* ev)
{
    uint32_t* temp;
    uint32_t* buf;

    assert(ev);

    if (quit)
        return -1;

    temp = (uint32_t*)(ev->data);
/*
    printf("temp = %p dashboardQueueSend\n", temp);
    for (int i = 0; i < ev->length; i++)
        printf("%x ", temp[i]);
*/
/*
    buf = malloc(ev->length * sizeof(uint32_t));
    memcpy(buf, ev->data, ev->length * sizeof(uint32_t));
    ev->data = buf;
    for (int i = 0; i < ev->length; i++)
        printf("%x ", buf[i]);
*/

    return mq_send(inQueue, (char*)ev, ev->length, 0);
}
