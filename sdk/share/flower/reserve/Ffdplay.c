#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "audio/include/fileheader.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct _FdGrabData
{
    void *          fd;
    PlayerState     state;
    int             hsize;
    int             loop_after;
    int             framesize;
    pthread_mutex_t mutex;

} FdGrabData;
//=============================================================================
//                              Private Function Declaration
//=============================================================================

/*static void fdgrab_close(IteFilter *f, void *arg){
    FdGrabData *d=(FdGrabData*)f->data;
    pthread_mutex_lock(&d->mutex);
    if (d->fd!=NULL)   fclose(d->fd);
    d->fd=NULL;
    d->state=Closed;
    pthread_mutex_unlock(&d->mutex);
}*/

static void fdgrab_open (IteFilter * f, void * arg)
{
    FdGrabData *    d    = (FdGrabData *)f->data;
    SoundPoolParm * parm = (SoundPoolParm *)arg;
    pthread_mutex_lock(&d->mutex);
    d->fd        = parm->fd;
    d->framesize = 20 * (2 * parm->info.sr * parm->info.nch) / 1000;
    d->state     = Playing;
    fseek(d->fd, d->hsize, SEEK_SET);
    pthread_mutex_unlock(&d->mutex);
}

static void fdgrab_hsize (IteFilter * f, void * arg)
{
    FdGrabData * d = (FdGrabData *)f->data;
    pthread_mutex_lock(&d->mutex);
    d->hsize = *((int *)arg);
}

static void fdgrab_loop (IteFilter * f, void * arg)
{
    FdGrabData * d = (FdGrabData *)f->data;
    d->loop_after  = *((int *)arg);
}

//=============================================================================
//                              filter flow
//=============================================================================
static void fdgrab_init (IteFilter * f)
{
    FdGrabData * d = (FdGrabData *)ite_new(FdGrabData, 1);
    if (d != NULL)
    {
        d->fd         = NULL;
        d->state      = Closed;
        d->hsize      = 44;  // wav header usually 44 bytes
        d->loop_after = -1;  /*by default, don't loop*/
        d->framesize  = 320; // 20*(2*d->rate*d->nchannels)/1000;//20ms data
        pthread_mutex_init(&d->mutex, NULL);
    }
    f->data = d;
}

static void fdgrab_uninit (IteFilter * f)
{
    FdGrabData * d = (FdGrabData *)f->data;
    if (d != NULL)
    {
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void fdgrab_do (IteFilter * f)
{
    FdGrabData * d = (FdGrabData *)f->data;
    int          rbytes;
    mblk_ite *   om = allocb_ite(d->framesize);
    rbytes          = fread(om->b_wptr, 1, d->framesize, d->fd);

    if (rbytes)
    {
        IteQueueblk blk = {0};
        om->b_wptr += rbytes;
        if (rbytes < d->framesize)
        {
            int tail = d->framesize - rbytes;
            memset(om->b_wptr, 0, tail);
            om->b_wptr += tail;
        }
        blk.datap = om;
        ite_queue_put(f->output[0].Qhandle, &blk);
    }
    else
    {
        if (d->loop_after == Normal)
        {
            d->state = Eof;
        }
        else
        { // Repeat,Scilent,
            fseek(d->fd, d->hsize, SEEK_SET);
        }
        if (om)
        {
            freemsg_ite(om);
        }
    }
}

static void fdgrab_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    FdGrabData * d = (FdGrabData *)f->data;
    // IteFlower *flower=(IteFlower*)f->srcFlow;

    while (f->run)
    {
        IteQueueblk blk = {0};
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }
        // printf("%d\n",ite_queue_get_size(f->output[0].Qhandle));
        pthread_mutex_lock(&d->mutex);
        if (d->state == Playing)
        {
            fdgrab_do(f);
        }

        if (d->state == Eof && IteAudioQueueIsEmpty(f, 0))
        { // add null time(data) prevent abnormal sound
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        pthread_mutex_unlock(&d->mutex);

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static IteMethodDes fdgrab_methods[] = {
    { ITE_FILTER_FILEOPEN,  fdgrab_open},
    { ITE_FILTER_LOOPPLAY,  fdgrab_loop},
    {ITE_FILTER_SET_PARAM, fdgrab_hsize},
    {                   0,         NULL}
};

// clang-format off
IteFilterDes FilterFdGrab = {
    ITE_FILTER_FDGRAB_ID,
    fdgrab_init,
    fdgrab_uninit,
    fdgrab_process,
    fdgrab_methods
};
// clang-format on
