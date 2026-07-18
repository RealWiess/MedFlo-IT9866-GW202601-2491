#include <stdio.h>
#include "ivorbiscodec.h"
#include "ivorbisfile.h"
#include "audio/include/fileheader.h"
#include "flower/flower.h"
#include "i2s/i2s.h"

typedef struct _VorbisData
{
    FILE *          fd;
    OggVorbis_File  vf;
    PlayerState     state;
    int             loop_after;
    int             framesize;
    pthread_mutex_t mutex;
} VorbisData;

static void vorbisgrab_close (IteFilter * f, void * arg)
{
    VorbisData * d = (VorbisData *)f->data;
    pthread_mutex_lock(&d->mutex);
    if (d->fd != 0)
    {
        ov_clear(&d->vf);
        fclose(d->fd);
    }
    d->fd    = NULL;
    d->state = Closed;
    pthread_mutex_unlock(&d->mutex);
}

static void vorbisgrab_open (IteFilter * f, void * arg)
{
    VorbisData * d      = (VorbisData *)f->data;
    IteFlower *  flower = (IteFlower *)f->srcFlow;
    char *       file   = (char *)arg;
    if (d->fd != NULL)
    {
        vorbisgrab_close(f, NULL);
    }
    d->fd = fopen(file, "rb");
    if (ov_open(d->fd, &d->vf, NULL, 0) < 0)
    {
        printf("Input does not appear to be an Ogg bitstream.\n");
        return;
    }
    {
        vorbis_info * vi = ov_info(&d->vf, -1);
        printf("%s time = %d(ms) \n", file, ov_time_total(&d->vf, -1));
        flower->dInfo.sr  = vi->rate;
        flower->dInfo.nch = vi->channels;
        flower->dInfo.bytes20ms =
            20 * 2 * vi->rate * vi->channels / 1000; // 20ms data
        flower->dInfo.init = true;
        EVEN(flower->dInfo.bytes20ms);
        d->framesize = flower->dInfo.bytes20ms;
    }
    d->state = Playing;
}

static void vorbis_init (IteFilter * f)
{
    VorbisData * d = (VorbisData *)ite_new(VorbisData, 1);
    if (d != NULL)
    {
        d->fd         = NULL;
        d->state      = Closed;
        d->framesize  = 320;
        d->loop_after = -1; /*by default, don't loop*/
        pthread_mutex_init(&d->mutex, NULL);
        f->pthread_stacksize = 10 * 1024;
    }
    f->data = d;
}

static void vorbis_uninit (IteFilter * f)
{
    VorbisData * d = (VorbisData *)f->data;
    if (d != NULL)
    {
        if (d->fd != NULL)
        {
            vorbisgrab_close(f, NULL);
        }
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void vorbisgrab_do (IteFilter * f)
{
    VorbisData * d = (VorbisData *)f->data;
    int          current_section;
    long         ret;
    char         buf[4096];
    ret = ov_read(&d->vf, buf, 4096, &current_section);
    if (ret)
    {
        IteQueueblk blk = {0};
        mblk_ite *  om  = allocb_ite(ret);
        memcpy(om->b_wptr, buf, ret);
        om->b_wptr += ret;
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
        {
            ov_raw_seek(&d->vf, 0);
        }
    }
}

static void vorbis_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    VorbisData * d      = (VorbisData *)f->data;
    IteFlower *  flower = (IteFlower *)f->srcFlow;

    while (f->run && !flower->dInfo.init && d->state == Closed)
    {
        usleep(100000);
    }

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
            vorbisgrab_do(f);
        }
        pthread_mutex_unlock(&d->mutex);

        if (d->state == Eof && IteAudioQueueIsEmpty(f, 0))
        { // add null time(data) prevent abnormal sound
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        if (d->state == Dummy)
        {
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void vorbisgrab_loop (IteFilter * f, void * arg)
{
    VorbisData * d = (VorbisData *)f->data;
    d->loop_after  = *((int *)arg);
}

static IteMethodDes vorbis_methods[] = {
    {ITE_FILTER_FILEOPEN, vorbisgrab_open},
    {ITE_FILTER_LOOPPLAY, vorbisgrab_loop},
    {                  0,            NULL}
};

// clang-format off
IteFilterDes FilterVorbisGrab = {
    ITE_FILTER_VORBISGRAB_ID,
    vorbis_init,
    vorbis_uninit,
    vorbis_process,
    vorbis_methods
};
// clang-format on
