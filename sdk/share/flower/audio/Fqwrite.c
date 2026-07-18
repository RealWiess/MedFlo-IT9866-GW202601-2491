#include <pthread.h>
#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

#define SET_CHIPSIZE

typedef struct _QWriteData
{
    mblkq           dataQ;
    pthread_mutex_t mutex;
#ifdef SET_CHIPSIZE
    int chipsize;
#endif
    cb_link_t       fn_cb;
} QWriteData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void qwrite_init (IteFilter * f)
{
    QWriteData * d = (QWriteData *)ite_new(QWriteData, 1);

    if (d != NULL)
    {
#ifdef SET_CHIPSIZE
        d->chipsize = 240;
        mblkQShapeInit(&d->dataQ, 4 * 1024);
#else
        mblkqinit(&d->dataQ);
#endif
        pthread_mutex_init(&d->mutex, NULL);
        d->fn_cb = NULL;
    }
    f->data = d;
}

static void qwrite_uninit (IteFilter * f)
{
    QWriteData * d = (QWriteData *)f->data;

    if (d != NULL)
    {
#ifdef SET_CHIPSIZE
        mblkQShapeUninit(&d->dataQ);
#else
        flushmblkq(&d->dataQ);
#endif
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void qwrite_process (IteFilter * f)
{
    QWriteData * d   = (QWriteData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    IteQueueblk  blk = {0};

    while (f->run)
    {
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om = blk.datap;
            pthread_mutex_lock(&d->mutex);
            if (om)
            {
#ifdef SET_CHIPSIZE
                srcQShapePut(&d->dataQ, om->b_rptr, om->size, d->chipsize);
                freemsg_ite(om);
#else
                putmblkq(&d->dataQ, om);
#endif
            }
            pthread_mutex_unlock(&d->mutex);
        }
        if(d->fn_cb)
        {//user define allback func :get data from dataQ
            d->fn_cb(flower);
        }
        usleep(1000);
    }
}

static void qwrite_data_get (IteFilter * f, void * arg)
{
    void **      p = arg;
    QWriteData * d = (QWriteData *)f->data;

    pthread_mutex_lock(&d->mutex);
    *p = getmblkq(&d->dataQ);
    pthread_mutex_unlock(&d->mutex);
}

static void qwrite_get_dataQ_size (IteFilter * f, void * arg)
{
    QWriteData * d = (QWriteData *)f->data;
    pthread_mutex_lock(&d->mutex);
    *(int *)arg = getmblkqavail(&d->dataQ);
    pthread_mutex_unlock(&d->mutex);
}

static void qwrite_set_cb (IteFilter * f, void * arg)
{
    QWriteData * d = (QWriteData *)f->data;
    d->fn_cb   = (cb_link_t)arg;
}

#ifdef SET_CHIPSIZE
static void qwrite_set_chipsize (IteFilter * f, void * arg)
{
    QWriteData * d = (QWriteData *)f->data;
    d->chipsize    = *((int *)arg);
}
#endif

static IteMethodDes qwrite_methods[] = {
    {ITE_FILTER_GET_PARAM,       qwrite_data_get},
    {ITE_FILTER_GET_VALUE, qwrite_get_dataQ_size},
    {ITE_FILTER_SET_CB   ,         qwrite_set_cb},
#ifdef SET_CHIPSIZE
    {ITE_FILTER_SET_VALUE,   qwrite_set_chipsize},
#endif
    {                   0,                  NULL}
};

// clang-format off
IteFilterDes FilterQWrite = {
    ITE_FILTER_QWRITE_ID,
    qwrite_init,
    qwrite_uninit,
    qwrite_process,
    qwrite_methods
};
// clang-format on
