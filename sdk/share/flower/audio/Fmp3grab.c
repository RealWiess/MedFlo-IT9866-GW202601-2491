#include <stdio.h>
#include <sys/stat.h>
#include "audio/include/fileheader.h"
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "mp3decode.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

#define RINGBUFFSIZE (1024 * 4)

typedef struct _Mp3grabData
{
    rbuf_ite *      rBuf;
    AVCodecContext  gMpegAudioContext;
    AVFrame         gFrame;
    void *          fd;
    int             rate;
    int             nchannels;
    int             loop_after;
    int             offset;
    bool            fileRead;
    PlayerState     state;
    int             framesize;
    pthread_mutex_t mutex;

} Mp3grabData;

static int MP3decoder (IteFilter * f)
{
    Mp3grabData * d           = (Mp3grabData *)f->data;
    IteQueueblk   blk         = {0};
    int           ret         = 0;
    int           got_picture = 0;
    memset(&d->gFrame, 0, sizeof(d->gFrame));
    ret = decode_frame(&d->gMpegAudioContext, &d->gFrame, &got_picture,
                       d->rBuf->b_rptr, ite_rbuf_get_avail_size(d->rBuf));

    if (got_picture)
    {
        mblk_ite *      om     = NULL;
        int             rbytes = d->gFrame.linesize[0];
        unsigned char * pcm    = d->gFrame.data[0];
        om                     = allocb_ite(rbytes);
        memcpy(om->b_wptr, pcm, rbytes);
        om->b_wptr += rbytes;
        blk.datap = om;
        ite_queue_put(f->output[0].Qhandle, &blk);
        if (ret >= 0)
        {
            ite_rbuf_rp_shift(d->rBuf,
                              ret); // d->rBuf->b_rptr+=d->info.frame_bytes;
        }
    }
    return got_picture;
}

static void decode_do (IteFilter * f)
{
    Mp3grabData * d = (Mp3grabData *)f->data;
    if (!MP3decoder(f))
    {
        if (d->fileRead)
        {
            int ReadSize, empty;
            ite_rbuf_rearrange(d->rBuf); // memove to header
            empty    = RINGBUFFSIZE - ite_rbuf_get_avail_size(d->rBuf);
            ReadSize = fread(d->rBuf->b_wptr, 1, empty,
                             d->fd); // file data to ring buffer
            d->rBuf->b_wptr += ReadSize;
            if (ReadSize == 0)
            {
                d->fileRead = false;
                printf("finish file read\n");
            }
        }
        else
        {
            if (d->loop_after == Normal)
            {
                d->state = Eof;
            }
            else
            {
                printf("play repeat\n");
                fseek(d->fd, d->offset, SEEK_SET);
                d->fileRead = true;
            }
        }
    }
}

static void mp3grab_close (IteFilter * f)
{
    Mp3grabData * d = (Mp3grabData *)f->data;
    pthread_mutex_lock(&d->mutex);
    d->state = Closed;
    if (d->fd)
    {
        fclose(d->fd);
    }
    d->fd = NULL;
    pthread_mutex_unlock(&d->mutex);
}

static void mp3grab_open (IteFilter * f, void * arg)
{
    Mp3grabData * d      = (Mp3grabData *)f->data;
    const char *  file   = (const char *)arg;
    IteFlower *   flower = (IteFlower *)f->srcFlow;
    int           tmp = 0, flag = 0;

    if (d->fd)
    {
        mp3grab_close(f);
        ite_mblk_queue_flush(f->output[0].Qhandle); // flush previous data
        flag = 1;
    }

    d->fd = fopen(file, "rb");
    if (d->fd == NULL)
    {
        printf("%s openfile error\n", file);
        d->state = Dummy;
        return 0;
    }
    parsingMp3IsID3v2(d->fd, NULL, NULL, &d->offset); // parsing ID3V2tag

    if (flower == NULL)
    {
        printf("[mp3grab_open] error!!!\n");
        return;
    }

    // audioGetTotalTime(file,&flower->dInfo);
    tmp                = parsing_mp3(d->fd, file, NULL, NULL, &flower->dInfo);
    d->rate            = flower->dInfo.sr;
    d->nchannels       = flower->dInfo.nch;
    d->framesize       = flower->dInfo.bytes20ms; // 20ms data
    flower->dInfo.init = true;
    EVEN(d->framesize);
    printf("totale time =%d\n", flower->dInfo.duration);
    d->state    = Playing;
    d->fileRead = true;
    // Castor3snd_reinit_for_diff_rate(d->rate,16,d->nchannels);

    return 0;
}

static void mp3grab_loop (IteFilter * f, void * arg)
{
    Mp3grabData * d = (Mp3grabData *)f->data;
    d->loop_after   = *((int *)arg);
    return 0;
}

static int mp3grab_get_rate (IteFilter * f, void * arg)
{
    Mp3grabData * d = (Mp3grabData *)f->data;
    *(int *)arg     = d->rate;
    return 0;
}

static int mp3grab_get_channels (IteFilter * f, void * arg)
{
    Mp3grabData * d = (Mp3grabData *)f->data;
    *(int *)arg     = d->nchannels;
    return 0;
}

static int mp3grab_pause (IteFilter * f, void * arg)
{
    Mp3grabData * d     = (Mp3grabData *)f->data;
    bool          pause = *((bool *)arg);
    pthread_mutex_lock(&d->mutex);
    if (pause)
    {
        d->state = Paused;
    }
    else
    {
        d->state = Playing;
    }
    pthread_mutex_unlock(&d->mutex);
    return 0;
}

//=============================================================================
//                              filter flow
//=============================================================================

static void mp3grab_init (IteFilter * f)
{
    Mp3grabData * d = (Mp3grabData *)ite_new(Mp3grabData, 1);

    if (d != NULL)
    {
        d->fd         = NULL;
        d->rate       = 8000;
        d->nchannels  = 1;
        d->loop_after = -1; /*by default, don't loop*/
        d->state      = Closed;
        d->offset     = 0;
        d->framesize  = 320; // 20*(2*d->rate*d->nchannels)/1000;//20ms data
        d->rBuf       = ite_rbuf_init(RINGBUFFSIZE);
        pthread_mutex_init(&d->mutex, NULL);
        memset(&d->gMpegAudioContext, 0, sizeof(d->gMpegAudioContext));
        decode_init(&d->gMpegAudioContext);
    }
    f->data = d;
}

static void mp3grab_uninit (IteFilter * f)
{
    Mp3grabData * d = (Mp3grabData *)f->data;

    if (d != NULL)
    {
        mp3grab_close(f);
        pthread_mutex_destroy(&d->mutex);
        ite_rbuf_free(d->rBuf);
        decode_deinit(&(d->gMpegAudioContext));
        free(d);
    }
}

static void mp3grab_process (IteFilter * f)
{
    Mp3grabData * d      = (Mp3grabData *)f->data;
    IteFlower *   flower = (IteFlower *)f->srcFlow;

    while (f->run && !flower->dInfo.init)
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

        pthread_mutex_lock(&d->mutex);
        if (d->state == Playing)
        { // Q count less 4 :abuot 4*20ms
            decode_do(f);
        }
        pthread_mutex_unlock(&d->mutex);

        if (d->state == Dummy)
        {
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        if (d->state == Eof && IteAudioQueueIsEmpty(f, 0))
        { // add dummy time(data) prevent abnormal sound
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }

    return NULL;
}

static IteMethodDes mp3grab_methods[] = {
    {     ITE_FILTER_FILEOPEN,         mp3grab_open},
    {     ITE_FILTER_LOOPPLAY,         mp3grab_loop},
    {      ITE_FILTER_GETRATE,     mp3grab_get_rate},
    {ITE_FILTER_GET_NCHANNELS, mp3grab_get_channels},
    {        ITE_FILTER_PAUSE,        mp3grab_pause},
    {                       0,                 NULL}
};

// clang-format off
IteFilterDes FilterMp3Grab = {
    ITE_FILTER_MP3GRAB_ID,
    mp3grab_init,
    mp3grab_uninit,
    mp3grab_process,
    mp3grab_methods
};
// clang-format on
