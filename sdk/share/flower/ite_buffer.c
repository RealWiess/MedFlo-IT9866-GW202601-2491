#include <malloc.h>
#include <string.h>
#include "flower/ite_buffer.h"
#include "flower/flower.h"

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Function Declaration
//=============================================================================

int ite_bufferWrap_init (IteBufferWrap * BWrap, int QSize)
{
    int result = -1;

    do
    {
        // check parameters
        if (BWrap == NULL)
        {
            (void)printf("[%s] ERROR: No such BufferWrap\n", __FUNCTION__);
            break;
        }

        // create a queue
        BWrap->Qhandler = xQueueCreate(QSize, sizeof(IteBuffer));
        if (BWrap->Qhandler == NULL)
        {
            (void)printf("[%s] ERROR: Create queue fail\n", __FUNCTION__);
            break;
        }

        // initialize the read index
        BWrap->ridx = 0;
        result      = 0;
    } while (false);

    // return the result
    return result;
}

int ite_buffer_reset (IteBuffer * blk)
{
    if (blk == NULL)
    {
        printf("[%s] ERROR: No such IteBuffer\n", __FUNCTION__);
        return -1;
    }

    memset(blk->buffer, 0, sizeof(uint8_t) * FLOWER_BUFFER_SIZE);
    blk->size = 0;

    return 0;
}

int ite_buffer_get (IteBufferWrap * BWrap, uint8_t * data, int getSize)
{
    IteBuffer tmp;
    int       bufferSize = 0;
    int       data_widx  = 0;
    int       q_ridx     = 0;

    if (BWrap->Qhandler == NULL)
    {
        printf("[%s] ERROR: No such Qhanlder\n", __FUNCTION__);
        return -1;
    }

    while (data_widx < getSize)
    {
        // init tmp
        ite_buffer_reset(&tmp);

        // check if Queue has entry or not
        if (uxQueueMessagesWaiting(BWrap->Qhandler) == 0)
        {
            printf("[%s] WARNING: Queue is empty\n", __FUNCTION__);
            break;
        }

        // copy queue's data to tmp
        xQueuePeek(BWrap->Qhandler, &tmp, 0);

        if (BWrap->ridx > 0)
        {
            // start at the ridx location of the old entry
            bufferSize = bufferSize + (tmp.size - BWrap->ridx);
            q_ridx     = BWrap->ridx;
        }
        else
        {
            // start from the new entry
            bufferSize = bufferSize + tmp.size;
            q_ridx     = 0;
        }

        BUFFER_DEBUG("[%s] bufferSize=%d buffer=%s\n", __FUNCTION__, bufferSize,
                     tmp.buffer);

        if (bufferSize > getSize)
        {
            // the last one. only use the partial data of queue entry
            BWrap->ridx = tmp.size - (bufferSize - getSize);
            memcpy(data + data_widx, tmp.buffer + q_ridx,
                   (BWrap->ridx - q_ridx));
            data_widx = data_widx + (BWrap->ridx - q_ridx);
            BUFFER_DEBUG("[%s] (bufferSize > getSize) ridx=%d data_widx=%d\n",
                         __FUNCTION__, BWrap->ridx, data_widx);

            break;
        }
        else if (bufferSize == getSize)
        {
            // the last one. use the complete data of queue entry
            memcpy(data + data_widx, tmp.buffer + q_ridx,
                   (tmp.size - BWrap->ridx));
            data_widx   = data_widx + (tmp.size - BWrap->ridx);
            BWrap->ridx = 0;
            BUFFER_DEBUG("[%s] (bufferSize = getSize) ridx=%d data_widx=%d\n",
                         __FUNCTION__, BWrap->ridx, data_widx);

            // remove tmp from Queue
            if(xQueueReceive(BWrap->Qhandler, &tmp, 0) == pdFALSE)
            {
                BUFFER_DEBUG("[%s] ERROR: Remove queue fail\n", __FUNCTION__);
            }

            break;
        }
        else
        {
            // bufferSize < getSize, keep getting data from Queue
            memcpy(data + data_widx, tmp.buffer + q_ridx,
                   (tmp.size - BWrap->ridx));
            data_widx   = data_widx + (tmp.size - BWrap->ridx);
            BWrap->ridx = 0;
            BUFFER_DEBUG("[%s] (bufferSize < getSize) ridx=%d data_widx=%d\n",
                         __FUNCTION__, BWrap->ridx, data_widx);

            // remove tmp from Queue
            if(xQueueReceive(BWrap->Qhandler, &tmp, 0) == pdFALSE)
            {
                BUFFER_DEBUG("[%s] ERROR: Remove queue fail\n", __FUNCTION__);
            }
        }
    }

    return data_widx;
}

int ite_buffer_put (IteBufferWrap * BWrap, uint8_t * data, int blkSize)
{
    int       ret       = 0;
    int       Qcount    = 0;
    int       i         = 0;
    int       data_widx = 0;
    IteBuffer tmp;

    if (BWrap->Qhandler == NULL)
    {
        printf("[%s] ERROR: No such Qhanlder\n", __FUNCTION__);
        return -1;
    }

    // calulate the number of Queue entry
    Qcount = blkSize / FLOWER_BUFFER_SIZE;
    if (blkSize % FLOWER_BUFFER_SIZE > 0)
    {
        Qcount = Qcount + 1;
    }

    BUFFER_DEBUG("[%s] Qcount=%d\n", __FUNCTION__, Qcount);

    while (i < Qcount)
    {
        // init tmp
        ite_buffer_reset(&tmp);

        if (blkSize < FLOWER_BUFFER_SIZE)
        {
            // the last one
            tmp.size = blkSize;
        }
        else
        {
            tmp.size = FLOWER_BUFFER_SIZE;
        }

        memcpy(tmp.buffer, data + data_widx, sizeof(uint8_t) * tmp.size);
        BUFFER_DEBUG("[%s] buffer=%s size=%d\n", __FUNCTION__, tmp.buffer,
                     tmp.size);

        // Send tmp to Buffer Queue
        if (xQueueSend(BWrap->Qhandler, &tmp, 0) != pdPASS)
        {
            ret = -1;
            break;
        }

        data_widx = data_widx + FLOWER_BUFFER_SIZE;
        blkSize   = blkSize - FLOWER_BUFFER_SIZE;

        i++;
    }

    return ret;
}

void ite_bufferWrap_deinit (IteBufferWrap * BWrap)
{
    if (BWrap->Qhandler == NULL)
    {
        printf("[%s] ERROR: Queue handler is empty\n", __FUNCTION__);
        return;
    }

    vQueueDelete(BWrap->Qhandler);
    BWrap->ridx = 0;
}
