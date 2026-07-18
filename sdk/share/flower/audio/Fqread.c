#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

typedef struct _QReadData
{
    PlayerState     state;
    mblkq           dataQ;
    pthread_mutex_t mutex;
    int             chipsize;
    cb_link_t       fn_cb;
} QReadData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void qread_init (IteFilter * f)
{
    QReadData * d = (QReadData *)ite_new(QReadData, 1);
    if (d != NULL)
    {
        mblkQShapeInit(&d->dataQ, 8 * 4096);
        d->chipsize = 0;
#if CFG_I2S_FOR_BT
        d->chipsize = 4096; // 512;
#endif
        pthread_mutex_init(&d->mutex, NULL);
        d->state = Playing;
        d->fn_cb = NULL;
    }
    f->data = d;
}

static void qread_uninit (IteFilter * f)
{
    QReadData * d = (QReadData *)f->data;
    if (d != NULL)
    {
        mblkQShapeUninit(&d->dataQ);
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void qread_do (IteFilter * f)
{
    QReadData * d   = (QReadData *)f->data;
    IteQueueblk blk = {0};
    mblk_ite *  im;

    pthread_mutex_lock(&d->mutex);
    im = getmblkq(&d->dataQ);
    pthread_mutex_unlock(&d->mutex);

    if (im)
    {
        if (im->size == 0)
        {
            if (im)
            {
                freemsg_ite(im);
            }
            d->state = Eof;
            return;
        }
        blk.datap = im;
        ite_queue_put(f->output[0].Qhandle, &blk);
    }
}

static void qread_process (IteFilter * f)
{
    QReadData * d      = (QReadData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    IteQueueblk blk    = {0};

    while (f->run)
    {
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }

        if (d->state == Playing)
        {
            if(d->fn_cb)
            {//user define allback func :put data to dataQ
                d->fn_cb(flower);
            }
            qread_do(f);
        }
        if (d->state == Eof)
        {
            blk.datap    = allocb0_ite(flower->dInfo.bytes20ms);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void qread_data_put (IteFilter * f, void * arg)
{
    void **     p = arg;
    QReadData * d = (QReadData *)f->data;
    mblk_ite *  m = *p;
    pthread_mutex_lock(&d->mutex);
    if (d->chipsize > 0)
    {
        mblkQShapePut(&d->dataQ, m, d->chipsize);
    }
    else
    {
        putmblkq(&d->dataQ, m);
    }

    pthread_mutex_unlock(&d->mutex);
}

static void qread_get_dataQ_size (IteFilter * f, void * arg)
{
    QReadData * d = (QReadData *)f->data;
    *(int *)arg   = getmblkqavail(&d->dataQ);
}

static void qread_set_chipsize (IteFilter * f, void * arg)
{
    QReadData * d = (QReadData *)f->data;
    d->chipsize   = *((int *)arg);
}

static void qread_set_cb (IteFilter * f, void * arg)
{
    QReadData * d = (QReadData *)f->data;
    d->fn_cb   = (cb_link_t)arg;
}

static IteMethodDes qread_methods[] = {
    {ITE_FILTER_SET_PARAM,       qread_data_put},
    {ITE_FILTER_GET_VALUE, qread_get_dataQ_size},
    {ITE_FILTER_SET_VALUE,   qread_set_chipsize},
    {ITE_FILTER_SET_CB   ,         qread_set_cb},
    {                   0,                 NULL}
};

// clang-format off
IteFilterDes FilterQRead = {
    ITE_FILTER_QREAD_ID,
    qread_init,
    qread_uninit,
    qread_process,
    qread_methods
};
// clang-format on
