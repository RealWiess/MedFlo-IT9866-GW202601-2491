#include <stdio.h>
#include <malloc.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "filter_ipcam.h"

typedef void (*pfPktReleaseCb)(void * pkt);

typedef struct VideoInputPkt
{
    uint8_t * pInputBuffer;
    uint32_t  bufferSize;
} VideoInputPkt;

typedef struct QueuePktList
{
    void *                pkt;
    struct QueuePktList * next;
} QueuePktList;

typedef struct PktQueue
{
    QueuePktList *  firstPkt, *lastPkt;
    int             numPackets;
    int             maxPackets;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    pfPktReleaseCb  pfPktRelease;
} PktQueue;

PktQueue    gIPCamVideoInputQueue;

///////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//
///////////////////////////////////////////////////////////////////////////////////////////
static void _videoInputPktRelease (void * pkt)
{
    VideoInputPkt * ptVideoInputPkt = (VideoInputPkt *)pkt;

    if (ptVideoInputPkt && ptVideoInputPkt->pInputBuffer &&
        ptVideoInputPkt->bufferSize)
    {
        free(ptVideoInputPkt->pInputBuffer);
    }
    free(pkt);
}

static int _packetQueuePut (PktQueue * q, void * pkt)
{
    QueuePktList * pkt1;

    /* duplicate the packet */
    if (q->mutex)
    {
        while (q->numPackets > q->maxPackets)
        {
            printf("queue is full: cur: %d, max: %d\n", q->numPackets,
                   q->maxPackets);
            usleep(1000 * 100);
        }
        pkt1 = malloc(sizeof(QueuePktList));
        if (!pkt1)
        {
            return -1;
        }
        pkt1->pkt  = pkt;
        pkt1->next = NULL;

        pthread_mutex_lock(&q->mutex);

        if (!q->lastPkt)
        {
            q->firstPkt = pkt1;
        }
        else
        {
            q->lastPkt->next = pkt1;
        }
        q->lastPkt = pkt1;
        q->numPackets++;

        /* XXX: should duplicate packet data in DV case */
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    else
    {
        return -1;
    }
}

static int _packetQueueGet (PktQueue * q, void ** pkt, int block)
{
    QueuePktList * pkt1;
    int            ret;

    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);

        for (;;)
        {
            pkt1 = q->firstPkt;
            if (pkt1)
            {
                q->firstPkt = pkt1->next;
                if (!q->firstPkt)
                {
                    q->lastPkt = NULL;
                }
                *pkt = pkt1->pkt;
                free(pkt1);
                q->numPackets--;
                ret = 1;
                break;
            }
            else if (block == 0) // non-block
            {
                ret = 0;
                break;
            }
            else // block
            {
                pthread_cond_wait(&q->cond, &q->mutex);
            }
        }
        pthread_mutex_unlock(&q->mutex);
        return ret;
    }
    else
    {
        return -1;
    }
}

/* packet queue handling */
static void _packetQueueInit (PktQueue * q, void * pfPktRelease, int maxPackets)
{
    memset(q, 0, sizeof(PktQueue));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->maxPackets   = maxPackets;
    q->pfPktRelease = pfPktRelease;
}

static void _packetQueueFlush (PktQueue * q)
{
    QueuePktList *pkt, *pkt1;

    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);
        for (pkt = q->firstPkt; pkt != NULL; pkt = pkt1)
        {
            pkt1 = pkt->next;
            if (q->pfPktRelease)
            {
                q->pfPktRelease(pkt->pkt);
            }
            free(pkt);
        }
        q->lastPkt    = NULL;
        q->firstPkt   = NULL;
        q->numPackets = 0;
        pthread_mutex_unlock(&q->mutex);
    }
}

static void _packetQueueEnd (PktQueue * q)
{
    if (q->mutex)
    {
        _packetQueueFlush(q);
        pthread_mutex_destroy(&q->mutex);
        pthread_cond_destroy(&q->cond);
        memset(q, 0, sizeof(PktQueue));
    }
}

int PutIntoPacketQueue (unsigned char * inputbuf, int inputbuf_size,
                        double timestamp)
{
    VideoInputPkt * videoPkt = (VideoInputPkt *)malloc(sizeof(VideoInputPkt));

    if (videoPkt)
    {
        videoPkt->pInputBuffer = inputbuf;
        videoPkt->bufferSize   = inputbuf_size;
        _packetQueuePut(&gIPCamVideoInputQueue, videoPkt);
    }

    return 0;
}

static void ipcam_init (IteFilter * f)
{
    _packetQueueInit(&gIPCamVideoInputQueue, _videoInputPktRelease, 150);
}

static void ipcam_uninit (IteFilter * f)
{
    _packetQueueEnd(&gIPCamVideoInputQueue);
}

static void ipcam_process (IteFilter * f)
{
    VideoInputPkt * ptVideoPkt  = NULL;
    IteQueueblk     blk_output0 = {0};
    IteQueueblk     blk_output1 = {0};
    mblk_ite *      im          = NULL;

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while (f->run)
    {
        if (_packetQueueGet(&gIPCamVideoInputQueue, (void **)&ptVideoPkt, 0) >
            0)
        {
            im = allocb_ite(ptVideoPkt->bufferSize);

            if (im != NULL)
            {
                memcpy(im->b_wptr, ptVideoPkt->pInputBuffer,
                       ptVideoPkt->bufferSize);
                im->b_wptr += ptVideoPkt->bufferSize;
                blk_output0.datap = im;
                ite_queue_put(f->output[0].Qhandle, &blk_output0);
            }

            if (f->output[1].Qhandle)
            {
                mblk_ite * data = NULL;
                data            = allocb_ite(ptVideoPkt->bufferSize);
                memcpy(data->b_wptr, ptVideoPkt->pInputBuffer,
                       ptVideoPkt->bufferSize);
                data->b_wptr += ptVideoPkt->bufferSize;
                blk_output1.datap = data;
                ite_queue_put(f->output[1].Qhandle, &blk_output1);
            }
            _videoInputPktRelease(ptVideoPkt);
        }
        usleep(1000);
    }
}

// clang-format off
IteFilterDes FilterIPCam = {
    ITE_FILTER_IPCAM_ID,
    ipcam_init,
    ipcam_uninit,
    ipcam_process,
};
// clang-format on
