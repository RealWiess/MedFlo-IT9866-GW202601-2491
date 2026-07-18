#include <malloc.h>
#include <string.h>
#include "flower/ite_queue.h"
#include "ite/ith.h"
#include "flower/flower.h"

#define CHECK_(condition)                                                      \
    if (!(condition))                                                          \
    {                                                                          \
        result = -1;                                                           \
        break;                                                                 \
    }

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Function Declaration
//=============================================================================

void ite_mblk_queue_flush (QueueHandle_t QHandler)
{
    if (QHandler != NULL)
    {
        mblk_ite * om = NULL; // Pointer for message block

        // Loop while there are messages in the queue
        while (uxQueueMessagesWaiting(QHandler) > 0)
        {
            IteQueueblk qblk = {0}; // Temporary variable to store queue blocks

            // Receive (remove) the next message block from the queue
            if (xQueueReceive(QHandler, &qblk, 0))
            {
                // Access the data pointer from the queue block
                om = qblk.datap;

                // Free the memory associated with the message block
                freemsg_ite(om);
            }
        }
    }
}

QueueHandle_t ite_queue_new (int QSize, int TSize)
{
    // Initialize a temporary variable to store the queue handler
    QueueHandle_t tmpQ_handler = NULL;

    // Create a new queue with the specified size and type size
    // QSize: The maximum number of items the queue can hold
    // TSize: The size of each item in the queue
    tmpQ_handler               = xQueueCreate(QSize, TSize);

    // Check if the queue creation was successful
    if (tmpQ_handler == NULL)
    {
        // If the queue handler is NULL, print an error message indicating queue
        // creation failure
        (void)printf("[%s] ERROR: Create queue fail\n", __FUNCTION__);
    }

    // Return the handle to the newly created queue or NULL if creation failed
    return tmpQ_handler;
}

void ite_queue_free (QueueHandle_t QHandler)
{
    do
    {
        // Check if the queue handler is valid
        if (QHandler == NULL)
        {
            // If the queue handler is NULL, print an error message indicating
            // that the queue handler is empty
            (void)printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);

            // Break out of the do-while loop
            break;
        }

        // Flush the queue, i.e., remove all messages from the queue and free
        // the memory associated with each message
        ite_mblk_queue_flush(QHandler);

        // Delete the queue itself
        vQueueDelete(QHandler);
    } while (false);
}

int ite_queue_get (QueueHandle_t QHandler, IteQueueblk * qblk)
{
    int result = -1; // The return value of this function

    do
    {
        if (QHandler == NULL)
        {
            // If the queue handler is NULL, print an error message indicating
            // that the queue handler is empty
            (void)printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
            break;
        }

        if (qblk == NULL)
        {
            // If the queue block is NULL, print an error message indicating
            // that the queue block is empty
            (void)printf("[%s] ERROR: Queue block is empty\n", __FUNCTION__);
            break;
        }

        if (uxQueueMessagesWaiting(QHandler) > 0)
        {
            // If there are messages waiting in the queue, receive one of them
            if (xQueueReceive(QHandler, qblk, 0))
            {
                result = 0; // Set the return value to 0 to indicate successful
                            // retrieval
            }
        }
    } while (false);

    return result; // Return 0 or -1 depending on whether the retrieval was
                   // successful
}

IteQueueblk * ite_queue_get_byRef (QueueHandle_t QHandler)
{
    IteQueueblk * result = NULL; // Will be set to the retrieved data block

    do
    {
        // Check if the queue handler is valid
        if (QHandler == NULL)
        {
            // Print an error message to the console if the queue handler is
            // NULL
            (void)printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
            // Break out of the do-while loop and return NULL
            break;
        }

        // Check if there are messages waiting in the queue
        if (uxQueueMessagesWaiting(QHandler) > 0)
        {
            // Initialize a temporary pointer to store the data block retrieved
            // from the queue
            IteQueueblk * tmp = NULL;
            // If messages are waiting, receive (remove) the next data block
            // from the queue
            if (xQueueReceive(QHandler, &tmp, 0))
            {
                // Set the 'result' pointer to the retrieved data block
                result = tmp;
            }
        }
    } while (false);

    // Return the pointer to the retrieved data block
    return result;
}

int ite_queue_get_fromISR (QueueHandle_t QHandler, IteQueueblk * qblk,
                           portBASE_TYPE xHigherPriorityTaskWoken)
{
    int result = -1;

    do
    {
        if (QHandler == NULL)
        {
            ithPrintf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
            break;
        }

        if (uxQueueMessagesWaiting(QHandler) > 0)
        {
            if (xQueueReceiveFromISR(QHandler, qblk, &xHigherPriorityTaskWoken))
            {
                result = 0;
            }
        }
    } while (false);

    return result;
}

int ite_queue_put (QueueHandle_t QHandler, IteQueueblk * qblk)
{
    int result = -1; // Return value, initially set to failure

    do
    {
        // Check if the queue handler is valid
        if (QHandler == NULL)
        {
            // Print an error message to the console if the queue handler is
            // NULL
            (void)printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
            // Break out of the do-while loop and return failure
            break;
        }

        // Attempt to put the data block into the queue
        if (xQueueSend(QHandler, qblk, 0))
        {
            // If the data block was successfully put into the queue, set the
            // result to 0 (success)
            result = 0;
        }
    } while (false);

    // Return the result
    return result;
}

int ite_queue_put_byRef (QueueHandle_t QHandler, IteQueueblk * qblk)
{
    int ret = 0;

    if (QHandler == NULL)
    {
        printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return -1;
    }

    // write to Queue
    if (xQueueSend(QHandler, &qblk, 0) != pdPASS)
    {
        // Queue is FULL
        ret = -1;
    }

    return ret;
}

int ite_queue_put_fromISR (QueueHandle_t QHandler, IteQueueblk * qblk,
                           portBASE_TYPE xHigherPriorityTaskWoken)
{
    int ret = 0;

    if (QHandler == NULL)
    {
        ithPrintf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return -1;
    }

    // write to Queue
    if (xQueueSendFromISR(QHandler, qblk, &xHigherPriorityTaskWoken) != pdPASS)
    {
        // Queue is FULL
        ret = -1;
    }

    return ret;
}

int ite_queue_put_head (QueueHandle_t QHandler, IteQueueblk * qblk)
{
    int ret = 0;

    if (QHandler == NULL)
    {
        printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return -1;
    }

    // write to Queue
    if (xQueueSendToFront(QHandler, qblk, 0) != pdPASS)
    {
        // Queue is FULL
        ret = -1;
    }

    return ret;
}

int ite_queue_put_head_fromISR (QueueHandle_t QHandler, IteQueueblk * qblk,
                                portBASE_TYPE xHigherPriorityTaskWoken)
{
    int ret = 0;

    if (QHandler == NULL)
    {
        ithPrintf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return -1;
    }

    // write to Queue
    if (xQueueSendToFrontFromISR(QHandler, qblk, &xHigherPriorityTaskWoken) !=
        pdPASS)
    {
        // Queue is FULL
        ret = -1;
    }

    return ret;
}

int ite_queue_get_size (QueueHandle_t QHandler)
{
    if (QHandler == NULL)
    {
        printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return -1;
    }

    return uxQueueMessagesWaiting(QHandler);
}

void ite_queue_reset (QueueHandle_t QHandler)
{
    if (QHandler == NULL)
    {
        printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return;
    }

    // set queue to emtpy
    xQueueReset(QHandler);
}

int ite_queue_peek_head (QueueHandle_t QHandler, IteQueueblk * qblk)
{
    int ret = 0;

    if (QHandler == NULL)
    {
        printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return -1;
    }

    if (uxQueueMessagesWaiting(QHandler))
    {
        xQueuePeek(QHandler, qblk, 0);
    }
    else
    {
        // Queue is empty
        ret = -1;
    }

    return ret;
}

int ite_queue_peek_head_fromISR (QueueHandle_t QHandler, IteQueueblk * qblk)
{
    int ret = 0;

    if (QHandler == NULL)
    {
        ithPrintf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return -1;
    }

    if (uxQueueMessagesWaiting(QHandler))
    {
        xQueuePeekFromISR(QHandler, qblk);
    }
    else
    {
        // Queue is empty
        ret = -1;
    }

    return ret;
}

mblk_ite * allocb_ite (size_t size)
{
    int32_t    result = 0;
    mblk_ite * mp;

    mp = (mblk_ite *)malloc(sizeof(mblk_ite));
    do
    {
        CHECK_(mp != NULL);

        memset(mp, 0, sizeof(mblk_ite));

        if (size > 0)
        {
            mp->b_datap = malloc(size);
            CHECK_(mp->b_datap != NULL);
            mp->b_rptr = mp->b_datap;
            mp->b_wptr = mp->b_datap;
        }
        mp->size = size;
    } while (false);

    if (result != 0)
    {
        if (mp != NULL)
        {
            free(mp);
            mp = NULL;
        }
    }

    return mp;
}

mblk_ite * allocb0_ite (size_t size)
{
    mblk_ite * mp = allocb_ite(size);

    do
    {
        if (mp == NULL)
        {
            break;
        }

        if (size > 0)
        {
            memset(mp->b_wptr, 0, size);
            mp->b_wptr += size;
        }
    } while (false);

    return mp;
}

void freemsg_ite (mblk_ite * mp)
{
    if (mp != NULL)
    {
        if (mp->b_datap != NULL)
        {
            free(mp->b_datap);
        }
        free(mp);
    }
}
