/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * ITE Queue.
 *
 * @author
 * @version 1.0
 */
#ifndef ITE_BUFFER_H
#define ITE_BUFFER_H
#include <stdio.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_DEBUG(...)
//#define BUFFER_DEBUG     printf

#define FLOWER_BUFFER_SIZE      32

//=============================================================================
//                              Structure Definition
//=============================================================================

/**
 * Buffer block data definition
 */
struct _IteBuffer {
    uint8_t buffer[FLOWER_BUFFER_SIZE];
    int size;
};
typedef struct _IteBuffer IteBuffer;

/**
 * Buffer block wrapper definition
 */
struct _IteBufferWrap {
    QueueHandle_t Qhandler;
    int ridx;
};
typedef struct _IteBufferWrap IteBufferWrap;

//=============================================================================
//                              Global Data Definition
//=============================================================================

//=============================================================================
//                              Public Function Definition
//=============================================================================

/**
 * @brief Initialize a buffer wrap.
 *
 * @param BWrap The buffer wrap to be initialized.
 * @param QSize The size of the queue in the buffer wrap.
 * @return 0 if the initialization is successful, -1 otherwise.
 */
int ite_bufferWrap_init(IteBufferWrap *BWrap, int QSize);

/**
 * Delete a buffer queue
 *
 * @param BWrap The struct of buffer wrapper
 */
void ite_bufferWrap_deinit(IteBufferWrap *BWrap);

/**
 * Reset a buffer block
 *
 * @param blk The block of buffer
 */
int ite_buffer_reset(IteBuffer *blk);

/**
 * Get data from buffer queue
 *
 * @param BWrap The struct of buffer wrapper
 * @param data The data which get from queue
 * @param bufferSize The size of data which you want to get
 */
int ite_buffer_get(IteBufferWrap *BWrap, uint8_t *data, int bufferSize);

/**
 * Put data to buffer queue
 *
 * @param BWrap The struct of buffer wrapper
 * @param data The data which put to queue
 * @param blkSize The size of data which you want to put
 */
int ite_buffer_put(IteBufferWrap *BWrap, uint8_t *data, int blkSize);

#ifdef __cplusplus
}
#endif

#endif /* ITE_STREAMER_H */
