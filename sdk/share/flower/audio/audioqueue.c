#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "i2s/i2s.h"

/**
 * Duplicate a message block.
 *
 * @param mp The message block to be duplicated.
 *
 * @return A duplicated message block.
 */
mblk_ite * dupmblk (mblk_ite * mp)
{
    mblk_ite * newm = NULL;

    do
    {
        if (mp == NULL)
        {
            break;
        }

        newm = allocb_ite(mp->size);
        if (newm == NULL)
        {
            break;
        }

        memcpy(newm->b_wptr, mp->b_rptr, newm->size);
        newm->b_wptr += mp->size;
    } while (false);

    return newm;
}

/**
 * Initialize a ring buffer.
 *
 * @param size The size of the ring buffer.
 *
 * @return The ring buffer.
 */
rbuf_ite * ite_rbuf_init (size_t size)
{
    return (rbuf_ite *)allocb_ite(size);
}

/**
 * Free a ring buffer.
 *
 * @param m The ring buffer to be freed.
 */
void ite_rbuf_free (rbuf_ite * m)
{
    freemsg_ite(m);
}

/**
 * @brief Get the available size in the ring buffer.
 *
 * This function calculates the amount of data currently available in the
 * ring buffer for reading. It accounts for the wrap-around nature of the
 * buffer by checking if the write pointer has wrapped past the read pointer.
 *
 * @param m Pointer to the ring buffer structure.
 * @return The number of bytes available for reading in the ring buffer.
 */
unsigned int ite_rbuf_get_avail_size (rbuf_ite * m)
{
    unsigned int size = 0U;

    do
    {
        if (m == NULL)
        {
            break;
        }

        if (m->b_wptr >= m->b_rptr)
        {
            size = m->b_wptr - m->b_rptr;
        }
        else
        {
            size = m->b_wptr - m->b_rptr + m->size;
        }
    } while (false);

    return size;
}

/**
 * @brief Put data into the ring buffer.
 *
 * This function puts data into the ring buffer and wraps around to the
 * start of the buffer if the write pointer reaches the end of the buffer.
 *
 * @param mp Pointer to the ring buffer structure.
 * @param src Pointer to the data to be put into the ring buffer.
 * @param sample Number of bytes to put into the ring buffer.
 *
 * @return The number of bytes written to the ring buffer.
 */
int ite_rbuf_put (rbuf_ite * mp, unsigned char * src, int sample)
{
    int result = 0;

    do
    {
        int wp;
        int mpsize;

        if ((mp == NULL) || (src == NULL) || (sample <= 0))
        {
            break;
        }

        wp     = mp->b_wptr - mp->b_datap;
        mpsize = mp->size;

        if ((ite_rbuf_get_avail_size(mp) + sample) >= mpsize)
        {
            printf("ring buffer full\n");
            break;
        }

        if (wp + sample < mpsize)
        {
            memcpy(mp->b_wptr, src, sample);
            mp->b_wptr += sample;
        }
        else
        {
            int szsec0 = mpsize - wp;
            int szsec1 = sample - szsec0;
            if (szsec0 > 0)
            {
                memcpy(mp->b_wptr, src, szsec0);
            }
            memcpy(mp->b_datap, src + szsec0, szsec1);
            mp->b_wptr = mp->b_datap + szsec1;
        }
        result = sample;
    } while (false);

    return result;
}

/**
 * @brief Get data from the ring buffer.
 *
 * This function gets data from the ring buffer. It returns the actual number
 * of bytes copied. If the available size of the ring buffer is less than the
 * requested sample size, it returns 0.
 *
 * @param dst The destination buffer to hold the data.
 * @param mp The ring buffer.
 * @param sample The requested sample size.
 *
 * @return The actual number of bytes copied.
 */
int ite_rbuf_get (unsigned char * dst, rbuf_ite * mp, int sample)
{
    int result = 0;

    do
    {
        int rp;
        int mpsize;

        if ((mp == NULL) || (dst == NULL) || (sample <= 0))
        {
            break;
        }

        rp     = mp->b_rptr - mp->b_datap;
        mpsize = mp->size;

        if (ite_rbuf_get_avail_size(mp) < sample)
        {
            break;
        }

        if (rp + sample < mpsize)
        {
            memcpy(dst, mp->b_rptr, sample);
            mp->b_rptr += sample;
        }
        else
        {
            int szsec0 = mpsize - rp;
            int szsec1 = sample - szsec0;
            if (szsec0 > 0)
            {
                memcpy(dst, mp->b_rptr, szsec0);
            }
            memcpy(dst + szsec0, mp->b_datap, szsec1);
            mp->b_rptr = mp->b_datap + szsec1;
        }
        result = sample;
    } while (false);

    return result;
}

/**
 * Shift the read pointer of a ring buffer by a specified amount.
 *
 * @param mp The ring buffer to modify.
 * @param shift The number of bytes to shift the read pointer. Can be positive
 *              or negative.
 * @return None
 */
void ite_rbuf_rp_shift (rbuf_ite * mp, int shift)
{
    if (mp != NULL)
    {
        unsigned char * dataPhead = mp->b_datap;
        unsigned char * dataPeof  = mp->b_datap + mp->size;
        unsigned char * newRp     = mp->b_rptr + shift;

        if (newRp > dataPeof)
        {
            newRp -= mp->size;
        }
        if (newRp < dataPhead)
        {
            newRp += mp->size;
        }
        mp->b_rptr = newRp;
    }
}

/**
 * Rearrange the ring buffer to move data to the beginning.
 *
 * This function rearranges the data in the ring buffer such that the unread
 * data is moved to the beginning of the buffer. It resets the read and write
 * pointers to the start of the buffer and rewrites the existing data.
 *
 * @param mp The ring buffer to be rearranged.
 */
void ite_rbuf_rearrange (rbuf_ite * mp)
{
    if (mp != NULL)
    {
        int avail = ite_rbuf_get_avail_size(mp);
        if (avail > 0)
        {
            unsigned char * buf = malloc(avail);
            if (buf != NULL)
            {
                if (ite_rbuf_get(buf, mp, avail) > 0)
                {
                    mp->b_rptr = mp->b_datap;
                    mp->b_wptr = mp->b_datap;
                    ite_rbuf_put(mp, buf, avail);
                }

                free(buf);
            }
        }
        else
        {
            mp->b_rptr = mp->b_datap;
            mp->b_wptr = mp->b_datap;
        }
    }
}

/**
 * Flush the ring buffer.
 *
 * This function sets the read and write pointers to the beginning of the ring
 * buffer and clears all the data in the buffer.
 *
 * @param mp The ring buffer to be flushed.
 */
void ite_rbuf_flush (rbuf_ite * mp)
{
    if (mp != NULL)
    {
        mp->b_rptr = mp->b_datap;
        mp->b_wptr = mp->b_datap;
        memset(mp->b_datap, 0, mp->size);
    }
}

/**
 * Initialize a message block.
 *
 * This function initializes a message block by setting its pointers and size
 * to zero or NULL.
 *
 * @param mp The message block to be initialized.
 */
void mblk_init (mblk_ite * mp)
{
    if (mp != NULL)
    {
        memset(mp, 0, sizeof(mblk_ite));
    }
}

/**
 * Initialize a message block queue.
 *
 * This function initializes a message block queue by setting up its sentinal
 * node and setting its count and memory size to zero.
 *
 * @param q The message block queue to be initialized.
 */
void mblkqinit (mblkq * q)
{
    if (q != NULL)
    {
        mblk_init(&q->qstop);
        q->qstop.b_next = &q->qstop;
        q->qstop.b_prev = &q->qstop;
        q->qcount       = 0;
    }
}

/**
 * Put a message block to the tail of a message block queue.
 *
 * This function links a message block to the tail of a message block queue.
 *
 * @param q The message block queue.
 * @param mp The message block to be linked.
 */
void putmblkq (mblkq * q, mblk_ite * mp)
{
    if ((q != NULL) && (mp != NULL))
    {
        q->qstop.b_prev->b_next = mp;
        mp->b_prev              = q->qstop.b_prev;
        mp->b_next              = &q->qstop;
        q->qstop.b_prev         = mp;
        q->qcount++;
    }
}

/**
 * Get a message block from the head of a message block queue.
 *
 * This function gets a message block from the head of a message block queue.
 * It removes the message block from the queue and returns it.
 *
 * @param q The message block queue.
 *
 * @return A message block from the head of the queue, or NULL if the queue is
 * empty.
 */
mblk_ite * getmblkq (mblkq * q)
{
    mblk_ite * tmp = NULL;

    do
    {
        if (q == NULL)
        {
            break;
        }

        tmp = q->qstop.b_next;
        if (tmp == &q->qstop)
        {
            tmp = NULL;
            break;
        }

        q->qstop.b_next     = tmp->b_next;
        tmp->b_next->b_prev = &q->qstop;
        tmp->b_prev         = NULL;
        tmp->b_next         = NULL;
        q->qcount--;
    } while (false);

    return tmp;
}

/**
 * Pop a message block from the end of a message block queue.
 *
 * This function pops a message block from the end of a message block queue.
 * It removes the message block from the queue and returns it.
 *
 * @param q The message block queue.
 *
 * @return A message block from the end of the queue, or NULL if the queue is
 * empty.
 */
mblk_ite * popmblkq (mblkq * q)
{
    mblk_ite * tmp = NULL;

    do
    {
        if (q == NULL)
        {
            break;
        }

        tmp = q->qstop.b_prev;
        if (tmp == &q->qstop)
        {
            tmp = NULL;
            break;
        }

        q->qstop.b_prev     = tmp->b_prev;
        tmp->b_prev->b_next = &q->qstop;
        tmp->b_prev         = NULL;
        tmp->b_next         = NULL;
        q->qcount--;
    } while (false);

    return tmp;
}

/**
 * Flushes a message block queue.
 *
 * This function removes all message blocks from the specified message block
 * queue and frees the associated memory for each block.
 *
 * @param q The message block queue to be flushed.
 */
void flushmblkq (mblkq * q)
{
    mblk_ite * mp;

    for (;;)
    {
        mp = getmblkq(q);
        if (mp == NULL)
        {
            break;
        }
        freemsg_ite(mp);
    }
}

/**
 * Get the number of message blocks in a message block queue.
 *
 * This function returns the count of message blocks present in the specified
 * message block queue.
 *
 * @param q The message block queue.
 *
 * @return The number of message blocks in the queue.
 */
int getmblkqavail (mblkq * q)
{
    int result = 0;

    if (q != NULL)
    {
        result = q->qcount;
    }

    return result;
}

/**
 * Initializes a message block queue for shaping.
 *
 * This function initializes a message block queue by initializing the queue
 * and its associated ring buffer for shaping. The ring buffer size is specified
 * by the bufsize parameter.
 *
 * @param q The message block queue to be initialized.
 * @param bufsize The size of the ring buffer for shaping.
 */
void mblkQShapeInit (mblkq * q, int bufsize)
{
    if (q != NULL)
    {
        mblkqinit(q);
        q->rb = ite_rbuf_init(bufsize);
    }
}

/**
 * Flushes a message block queue and its associated ring buffer.
 *
 * This function flushes a message block queue by removing all message blocks
 * from the queue and freeing the associated memory for each block. It also
 * flushes the associated ring buffer.
 *
 * @param q The message block queue to be flushed.
 */
void mblkQShapeFlush (mblkq * q)
{
    if (q != NULL)
    {
        flushmblkq(q);
        ite_rbuf_flush(q->rb);
    }
}

/**
 * Uninitializes a message block queue that has been initialized for shaping.
 *
 * This function uninitializes a message block queue by flushing the queue and
 * freeing the associated memory for each block, and freeing the associated ring
 * buffer.
 *
 * @param q The message block queue to be uninitialized.
 */
void mblkQShapeUninit (mblkq * q)
{
    if (q != NULL)
    {
        flushmblkq(q);
        ite_rbuf_free(q->rb);
    }
}

/**
 * Inserts a message block into a shaped message block queue.
 *
 * This function processes a message block and inserts it into the specified
 * message block queue for shaping. It uses `srcQShapePut` to handle the
 * insertion when specific conditions are met, freeing the message block
 * afterwards. If the conditions are not met, it puts the data into a ring
 * buffer and then processes it in chunks of the specified size, creating
 * new message blocks as necessary.
 *
 * @param q The message block queue where the message block is to be inserted.
 * @param m The message block to be inserted into the queue.
 * @param resize The size to which message blocks should be shaped.
 */

void mblkQShapePut (mblkq * q, mblk_ite * m, int resize)
{
    do
    {
        if ((q == NULL) || (m == NULL))
        {
            break;
        }

#if 1
        srcQShapePut(q, m->b_rptr, m->size, resize);
        freemsg_ite(m);
#else
        unsigned char tmp[4096];
        if (m && m->size == resize)
        {
            putmblkq(q, m);
        }
        else
        {
            if (m)
            {
                ite_rbuf_put(q->rb, m->b_rptr, m->size);
            }
            while (ite_rbuf_get(tmp, q->rb, resize))
            {
                mblk_ite * om = allocb_ite(resize);
                memcpy(om->b_wptr, tmp, resize);
                om->b_wptr += resize;
                putmblkq(q, om);
            }
            if (m)
            {
                freemsg_ite(m);
            }
        }
#endif
    } while (false);
}

/**
 * Processes a message block and inserts it into the specified message block
 * queue for shaping.
 *
 * This function processes a message block and inserts it into the specified
 * message block queue for shaping. It uses the associated ring buffer to
 * handle the insertion when specific conditions are met, freeing the message
 * block afterwards. If the conditions are not met, it puts the data into a ring
 * buffer and then processes it in chunks of the specified size, creating new
 * message blocks as necessary.
 *
 * @param q The message block queue where the message block is to be inserted.
 * @param inptr The data block to be inserted into the queue.
 * @param length The size of the data block to be inserted.
 * @param resize The size to which message blocks should be shaped.
 */
void srcQShapePut (mblkq * q, unsigned char * inptr, int length, int resize)
{
    do
    {
        int availSize;
        int count;
        int offset;
        int remain;
        int residual;

        if (q == NULL)
        {
            break;
        }

        availSize = ite_rbuf_get_avail_size(q->rb);
        count     = 0;
        remain    = 0;
        residual  = length;

        if (length == 0)
        {
            // eof : put 0 to Q
            mblk_ite * om;

            if (availSize > 0)
            { // rbuf with data
                om = allocb_ite(resize);
                if (om == NULL)
                {
                    break;
                }
                memset(om->b_wptr, 0, resize);
                ite_rbuf_get(om->b_wptr, q->rb, availSize);
                om->b_wptr += resize;
                putmblkq(q, om);
            }

            om = allocb_ite(length); // size==0
            if (om == NULL)
            {
                break;
            }
            putmblkq(q, om);
            // printf("put Zeor Q\n");
            return;
        }

        if (availSize + residual >= resize)
        {
            if (availSize > 0)
            { // rbuf with data
                mblk_ite * om = allocb_ite(resize);
                if (om == NULL)
                {
                    break;
                }
                remain = resize - availSize;
                ite_rbuf_get(om->b_wptr, q->rb, availSize);
                om->b_wptr += availSize;
                memcpy(om->b_wptr, inptr, remain);
                om->b_wptr += remain;
                putmblkq(q, om);
                residual -= remain;
            }

            while (residual > resize)
            {
                offset        = count * resize + remain;
                mblk_ite * om = allocb_ite(resize);
                if (om == NULL)
                {
                    break;
                }
                memcpy(om->b_wptr, inptr + offset, resize);
                om->b_wptr += resize;
                putmblkq(q, om);
                count++;
                residual -= resize;
            }
        }

        if (residual > 0)
        {
            offset = count * resize + remain;
            ite_rbuf_put(q->rb, inptr + offset, residual);
        }
    } while (false);
}

/**
 * Adjust volume of a mblk_ite by multiplying each sample by gain.
 * @param m   mblk_ite to adjust volume
 * @param gain    volume gain, range from 0.0 to 1.0
 */
void mblkvolctrl (mblk_ite * m, float gain)
{
    do
    {
        unsigned char * ptr;

        if (m == NULL)
        {
            break;
        }

        ptr = m->b_rptr;
        if (ptr == NULL)
        {
            break;
        }

        for (; ptr < m->b_wptr; ptr += 2)
        {
            *((int16_t *)(ptr)) = clamp16((gain * (int32_t)*(int16_t *)ptr));
        }
    } while (false);
}

/**
 * Put a silent block to the queue of output pin of the filter.
 *
 * The silent block is a mblk_ite with all data set to zero.
 * The blk->private1 is set to Scilent.
 *
 * @param blk      The IteQueueblk to be used to put to the queue
 * @param f        The filter which output pin is to be put
 * @param pin      The pin number of the filter
 * @param size     The size of the silent block
 */
void ite_queue_put_scilent (IteQueueblk * blk, IteFilter * f, int pin, int size)
{
    do
    {
        mblk_ite * om;

        if ((blk == NULL) || (f == NULL))
        {
            break;
        }

        om = allocb_ite(size);
        if (om == NULL)
        {
            break;
        }

        memset(om->b_wptr, 0, size);
        om->b_wptr += size;
        blk->datap    = om;
        blk->private1 = (void *)Scilent;
        ite_queue_put(f->output[pin].Qhandle, blk);
    } while (false);
}

/**
 * Transfers message blocks from a temporary message block queue to the output
 * queue of a filter.
 *
 * This function retrieves message blocks from a temporary queue and inserts
 * them into the specified filter's output queue until the output queue reaches
 * a size of 32 or the temporary queue is empty.
 *
 * @param blk The IteQueueblk structure used to transfer message blocks.
 * @param f The filter whose output queue will receive the message blocks.
 * @param pin The pin number of the filter's output queue.
 * @param tmpQ The temporary message block queue from which to retrieve message
 * blocks.
 */
void ite_queue_put_from_mblkQ (IteQueueblk * blk, IteFilter * f, int pin,
                               mblkq * tmpQ)
{
    if ((blk != NULL) && (f != NULL))
    {
        while (ite_queue_get_size(f->output[pin].Qhandle) < 32)
        {
            blk->datap = getmblkq(tmpQ);
            if (blk->datap != NULL)
            {
                ite_queue_put(f->output[pin].Qhandle, blk);
            }
            else
            {
                break;
            }
        }
    }
}

/**
 * Retrieves message blocks from the specified filter's input queue and inserts
 * them into a temporary message block queue after shaping.
 *
 * This function retrieves message blocks from the specified filter's input
 * queue until the queue is empty and inserts them into a temporary message
 * block queue after shaping. The message blocks are retrieved from the queue
 * until the queue is empty.
 *
 * @param blk The IteQueueblk structure used to transfer message blocks.
 * @param f The filter whose input queue will be retrieved from.
 * @param pin The pin number of the filter's input queue.
 * @param tmpQ The temporary message block queue which will receive the message
 * blocks.
 * @param resize The size to which the message blocks should be shaped.
 */
void ite_queue_put_to_mblkQShape (IteQueueblk * blk, IteFilter * f, int pin,
                                  mblkq * tmpQ, int resize)
{
    if ((f != NULL) && (blk != NULL))
    {
        while (ite_queue_get(f->input[pin].Qhandle, blk) == 0)
        {
            mblkQShapePut(tmpQ, blk->datap, resize);
            blk->datap = NULL;
        }
    }
}

/**
 * Controls the size of a filter's output queue by sleeping until the
 * size of the queue is within the specified range.
 *
 * This function controls the size of a filter's output queue by sleeping
 * until the size of the queue is within the specified range. It checks
 * the size of the queue and if it is greater than or equal to the
 * specified maximum size, it sleeps until the size of the queue is
 * less than or equal to the specified minimum size. If the filter is
 * not running, this function immediately returns.
 *
 * @param f The filter whose output queue will be controlled.
 * @param pin The pin number of the filter's output queue.
 * @param max The maximum size of the output queue.
 * @param min The minimum size of the output queue.
 *
 * @return 1 if the size of the queue is within the specified range,
 * -1 if a timeout occurs, 0 if the filter is not running.
 */
int IteAudioQueueController (IteFilter * f, int pin, int max, int min)
{
    int timeout = 0;

    if (ite_queue_get_size(f->output[pin].Qhandle) >= max)
    {
        while (ite_queue_get_size(f->output[pin].Qhandle) > min && f->run)
        {
            usleep(50 * 1000);
            timeout++;
            if (timeout > 50)
            {
                printf("IteAudioQueueController timeout\n");
                return -1;
            }
        }
        return 1;
    }
    return 0;
}

/**
 * Checks if the output queue of a filter is empty.
 *
 * @param f The filter whose output queue will be checked.
 * @param pin The pin number of the filter's output queue.
 *
 * @return true if the output queue is empty, false otherwise.
 */
bool IteAudioQueueIsEmpty (IteFilter * f, int pin)
{
    bool result = true;

    if (f != NULL)
    {
        // (void)printf("out :%d\n",ite_queue_get_size(f->output[pin].Qhandle));
        result = (ite_queue_get_size(f->output[pin].Qhandle) == 0);
    }

    return result;
}

/**
 * Gets the number of blocks in the output queue of a filter.
 *
 * @param f The filter whose output queue will be counted.
 * @param pin The pin number of the filter's output queue.
 *
 * @return The number of blocks in the output queue.
 */
int IteAudioQueueNum (IteFilter * f, int pin)
{
    int result = 0;
    if (f != NULL)
    {
        // (void)printf("out
        // num:%d\n",ite_queue_get_size(f->output[pin].Qhandle));
        result = ite_queue_get_size(f->output[pin].Qhandle);
    }
    return result;
}

/**
 * Controls the status of a filter.
 *
 * This function checks the current status of the given filter and returns
 * a corresponding status code. If the filter is in a paused state, the function
 * pauses execution for a short duration before returning.
 *
 * @param f The filter whose status is to be controlled.
 *
 * @return A status code indicating the filter's current state. Possible return
 * values are FILTER_NONE, FILTER_RESUME, FILTER_PAUSE, or FILTER_FLUSH.
 */
int IteFilterController (IteFilter * f)
{
    int result = FILTER_ERR;

    if (f != NULL)
    {
        switch (f->status)
        {
            case FILTER_NONE:
                result = FILTER_NONE;
                break;
            case FILTER_RESUME:
                result = FILTER_RESUME;
                break;
            case FILTER_PAUSE:
                usleep(10000);
                result = FILTER_PAUSE;
                break;
            case FILTER_FLUSH:
                result = FILTER_FLUSH;
                break;
            default:
                /* Do nothing */
                break;
        }
    }

    return result;
}

/**
 * Initializes a DataInfo struct with default values.
 *
 * The default values are:
 * - sr: 0
 * - nch: 0
 * - bitsize: 16
 * - codectype: 0
 * - bytes20ms: 320
 * - duration: 0
 * - currentms: 0
 * - sw_vol: 0.0
 * - init: false
 */
void dinfoSetDefault (DataInfo * Info)
{
    if (Info != NULL)
    {
        Info->sr        = 0;
        Info->nch       = 0;
        Info->bitsize   = 16;
        Info->codectype = 0;
        Info->bytes20ms = 320;
        Info->duration  = 0;
        Info->currentms = 0;
        // Info->sw_vol=0.0;
        Info->init      = false;
    }
}

/**
 * Copies the values from the source DataInfo to the target DataInfo.
 *
 * This function will copy the following values from source to target:
 * - sr: sample rate
 * - nch: number of channels
 * - bitsize: bits per sample
 * - codectype: codec type
 * - bytes20ms: number of bytes per 20ms
 * - init: whether the DataInfo is initialized or not
 */
void dinfoSetValue (DataInfo * target, DataInfo * source)
{
    if ((target != NULL) && (source != NULL))
    {
        target->sr        = source->sr;
        target->nch       = source->nch;
        target->bitsize   = source->bitsize;
        target->codectype = source->codectype;
        target->bytes20ms = source->bytes20ms;
        target->init      = source->init;
    }
}

/**
 * Creates a new IteFilter instance, and sets its source flower.
 *
 * @param id The filter id of the new filter.
 * @param arg The source flower that will be associated with the new filter.
 *
 * @return A newly created IteFilter instance.
 */
IteFilter * Flow_filter_new (IteFilterId id, IteFlower ** arg)
{
    IteFilter * newPtr = ite_filter_new(id);
    if (newPtr != NULL)
    {
        newPtr->srcFlow = *arg;
    }
    return newPtr;
}
