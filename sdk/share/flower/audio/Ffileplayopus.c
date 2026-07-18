#include <stdio.h>
#include "include/opusfile.h"
#include "audio/include/fileheader.h"
#include "flower/flower.h"
#include "i2s/i2s.h"

typedef struct _OpusfData
{
    // FILE *fd;
    OggOpusFile *   fd;
    PlayerState     state;
    int             loop_after;
    int             framesize;
    pthread_mutex_t mutex;
} OpusfData;

static void opusfgrab_close (IteFilter * f, void * arg)
{
    OpusfData * d = (OpusfData *)f->data;
    pthread_mutex_lock(&d->mutex);
    if (d->fd != NULL)
    {
        op_free(d->fd);
    }
    d->fd    = NULL;
    d->state = Closed;
    pthread_mutex_unlock(&d->mutex);
}

static void opusfgrab_open (IteFilter * f, void * arg)
{
    OpusfData * d      = (OpusfData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    int         ret;
    char *      file = (char *)arg;
    if (d->fd != NULL)
    {
        opusfgrab_close(f, NULL);
    }
    d->fd = op_open_file(file, &ret);
    if (d->fd == NULL)
    {
        printf("Input does not appear to be an Ogg bitstream.\n");
        return;
    }
    {
        const OpusHead * head   = op_head(d->fd, 0);
        flower->dInfo.sr        = head->input_sample_rate;
        flower->dInfo.nch       = head->channel_count;
        flower->dInfo.bytes20ms = 20 * 2 * head->input_sample_rate *
                                  head->channel_count / 1000; // 20ms data
        flower->dInfo.init = true;
        EVEN(flower->dInfo.bytes20ms);
        d->framesize = flower->dInfo.bytes20ms;
    }
    d->state = Playing;
}

static void opusf_init (IteFilter * f)
{
    OpusfData * d = (OpusfData *)ite_new(OpusfData, 1);

    if (d != NULL)
    {
        d->fd         = NULL;
        d->state      = Closed;
        d->framesize  = 320;
        d->loop_after = -1; /*by default, don't loop*/
        pthread_mutex_init(&d->mutex, NULL);

        f->pthread_stacksize = 20 * 1024;
    }
    f->data = d;
}

static void opusf_uninit (IteFilter * f)
{
    OpusfData * d = (OpusfData *)f->data;

    if (d != NULL)
    {
        if (d->fd != NULL)
        {
            opusfgrab_close(f, NULL);
        }
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void opusfgrab_do (IteFilter * f)
{
    OpusfData * d      = (OpusfData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    int         ch     = flower->dInfo.nch;
    // int current_section;
    long        ret;
    char        buf[4096];
    ret = op_read(d->fd, (opus_int16 *)buf, sizeof(buf), NULL);
    if (ret)
    {
        int         len = 2 * ret * ch;
        IteQueueblk blk = {0};
        mblk_ite *  om  = allocb_ite(len);
        memcpy(om->b_wptr, buf, len);
        om->b_wptr += len;
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
            op_raw_seek(d->fd, 0);
        }
    }
}

static void opusf_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    OpusfData * d      = (OpusfData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;

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
            opusfgrab_do(f);
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

static void opusfgrab_loop (IteFilter * f, void * arg)
{
    OpusfData * d = (OpusfData *)f->data;
    d->loop_after = *((int *)arg);
}

static IteMethodDes opusf_methods[] = {
    {ITE_FILTER_FILEOPEN, opusfgrab_open},
    {ITE_FILTER_LOOPPLAY, opusfgrab_loop},
    {                  0,           NULL}
};

// clang-format off
IteFilterDes FilterOpusGrab = {
    ITE_FILTER_OPUSGRAB_ID,
    opusf_init,
    opusf_uninit,
    opusf_process,
    opusf_methods
};
// clang-format on
