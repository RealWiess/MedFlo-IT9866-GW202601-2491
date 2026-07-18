/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * ITE Queue.
 *
 * @author
 * @version 1.0
 */
#ifndef ITE_QUEUE_H
#define ITE_QUEUE_H
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef bool
typedef unsigned char	bool;
#define  true	1
#define  false	0
#endif

//=============================================================================
//                              Structure Definition
//=============================================================================

typedef struct _msgb
{
	struct _msgb * b_prev;
	struct _msgb * b_next;
	unsigned char * b_datap;
	unsigned char * b_rptr;
	unsigned char * b_wptr;
    int size;
} mblk_ite;
typedef mblk_ite rbuf_ite;

typedef struct _mblkq
{
    mblk_ite qstop;
    int qcount;             /* num of count in q */
    unsigned int qmsize;    /* size of memory use in q */
    rbuf_ite * rb;
} mblkq;

typedef struct _gQueueData{
    pthread_mutex_t mutex;
    mblkq dataQ;
    int gcodecType;
    int startcount;
    bool flagEof;
}gQueueData;

/**
 * Queue block data definition
 */
struct _IteQueueblk {
    uint8_t data;
    // TBD: more complex data struct
    mblk_ite * datap;
    void * private1;
    void * private2;
};
typedef struct _IteQueueblk IteQueueblk;

//=============================================================================
//                              Global Data Definition
//=============================================================================

//=============================================================================
//                              Public Function Definition
//=============================================================================
/**
 * Flushes all messages from the queue.
 *
 * This function iterates through all the messages in the specified queue,
 * removes each message, and frees the associated memory.
 *
 * @param QHandler The handler of the queue to be flushed.
 */
void ite_mblk_queue_flush(QueueHandle_t QHandler);

/**
 * Creates a new queue with the specified size and type size.
 *
 * @param QSize The maximum number of items the queue can hold
 * @param TSize The size of each item in the queue
 *
 * @return Returns the handle to the newly created queue or NULL if creation failed
 */
QueueHandle_t ite_queue_new(int QSize, int TSize);

/**
 * @brief Frees a queue and all associated memory.
 *
 * @param QHandler The handle of the queue to free.
 *
 * This function frees a queue and all associated memory by removing all
 * entries from the queue and freeing the memory allocated for each item.
 * Finally, it deletes the queue itself.
 */
void ite_queue_free(QueueHandle_t QHandler);

/**
 * @brief Get a data block from queue
 *
 * This function retrieves a data block from the specified queue. It
 * waits indefinitely until a data block is available from the queue.
 *
 * @param QHandler The handler of the queue
 * @param qblk A pointer to the data block which will be retrieved from the queue
 * @return 0 if the retrieval was successful, -1 if the queue handler is NULL
 */
int ite_queue_get(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * @brief Retrieves a data block from the specified queue by reference
 *
 * This function retrieves a data block from the specified queue by reference.
 * It waits indefinitely until a data block is available from the queue.
 *
 * @param QHandler The handler of the queue
 *
 * @return a pointer to the retrieved data block, or NULL if the retrieval fails
 */
IteQueueblk *ite_queue_get_byRef(QueueHandle_t QHandler);

/**
 * Retrieves a data block from the specified queue for interrupt-safe version
 *
 * This function retrieves a data block from the specified queue in an
 * interrupt-safe manner. It waits indefinitely until a data block is
 * available from the queue.
 *
 * @param QHandler The handler of the queue
 * @param qblk A pointer to the data block which will be retrieved from the queue
 * @param xHigherPriorityTaskWoken Set pdTRUE if sending to the queue caused a
 * task to unblock, and the unblocked task has a priority higher than the
 * currently running task.
 *
 * @return 0 if the retrieval was successful, -1 if the queue handler is NULL
 */
int ite_queue_get_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk, portBASE_TYPE xHigherPriorityTaskWoken);

/**
 * @brief Put a data block to the queue
 *
 * This function puts a data block into the specified queue. It
 * waits indefinitely until the data block can be put into the
 * queue.
 *
 * @param QHandler The handler of the queue
 * @param qblk A pointer to the data block which will be put into the queue
 *
 * @return 0 if the data block was successfully put into the queue, -1 if
 * the queue handler is NULL
 */
int ite_queue_put(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Put a data pointer to queue
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data pointer which put to queue
 */
int ite_queue_put_byRef(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Put a data block to queue for interrupt-safe version
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which put to queue
 * @param xHigherPriorityTaskWoken Set pdTRUE if sending to the queue caused a task
 * to unblock, and the unblocked task has a priority higher than the currently
 * running task.
 */
int ite_queue_put_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk, portBASE_TYPE xHigherPriorityTaskWoken);

/**
 * Put a data block to queue's head
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which put to queue
 */
int ite_queue_put_head(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Put a data block to queue's head for interrupt-safe version
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which put to queue
 * @param xHigherPriorityTaskWoken Set pdTRUE if sending to the queue caused a task
 * to unblock, and the unblocked task has a priority higher than the currently
 * running task.
 */
int ite_queue_put_head_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk, portBASE_TYPE xHigherPriorityTaskWoken);

/**
 * Get the size of queue
 *
 * @param QHandler The handler of queue
 */
int ite_queue_get_size(QueueHandle_t QHandler);

/**
 * Reset the queue
 *
 * @param QHandler The handler of queue
 */
void ite_queue_reset(QueueHandle_t QHandler);

/**
 * Get a data block from queue without the data being removed from the queue
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which get from the queue
 */
int ite_queue_peek_head(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Get a data block from queue without the data being removed from the queue for interrupt-safe version
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which get from the queue
 */
int ite_queue_peek_head_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Create a memory block
 *
 * @param memory block size
 *
 */
mblk_ite *allocb_ite(size_t size);
mblk_ite *allocb0_ite(size_t size);

/**
 * free memory block
 *
 * @param mp A memory block for free
 *
 */
void freemsg_ite(mblk_ite *mp);

#ifdef __cplusplus
}
#endif

#endif /* ITE_STREAMER_H */
