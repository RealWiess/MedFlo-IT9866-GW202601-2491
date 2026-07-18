#ifndef MQUEUE_DASHBOARD_H
#define MQUEUE_DASHBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>

#define BUFFER_SIZE     1024
#define EXT_MAX_QUEUE_SIZE  8

typedef struct _DashboardEvent{
    uint32_t canid;
    void* data;
    size_t length;
} DashboardEvent_t;

#endif

