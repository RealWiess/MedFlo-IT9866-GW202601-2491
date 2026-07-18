#include <stdio.h>
#include "flower/flower.h"
#include "audio/include/audioqueue.h"
#include "audio/include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

#define PREBUFFERSIZE 1024 * 64

typedef struct _DeM4a
{
    void *            fd;
    PlayerState       state;
    int               rate;
    int               nch;
    int               loop_after;

    pthread_mutex_t   mutex;

    AVCodecContext *  avctx;
    AVFormatContext * format_context;
    AVFrame *         frame;
    double            curpts;
} DeM4a;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static void dem4a_close (IteFilter * f, void * arg)
{
    DeM4a * d = (DeM4a *)f->data;
    pthread_mutex_lock(&d->mutex);
    d->state = Closed;
    if (d->fd)
    {
        // avcodec_close(d->avctx);
        avformat_close_input(&d->format_context);
        // castor3snd_reinit_for_video_memo_play();
    }
    d->fd             = NULL;
    d->format_context = NULL;
    pthread_mutex_unlock(&d->mutex);
}
/*
static uint32_t getpkt_timestamp(AVPacket *pkt,DeM4a *d){//just for audio
    uint32_t cur_stamp=((double)pkt->pts/d->rate)*1000*1000;
    return cur_stamp;
}
*/
static void dem4a_do (IteFilter * f)
{
    DeM4a *         d = (DeM4a *)f->data;
    unsigned char * datap;
    uint32_t        bytes;
    IteQueueblk     blk = {0};
    AVPacket        pkt1, *pkt = &pkt1;

    if (av_read_frame(d->format_context, pkt) == 0)
    {
        mblk_ite * om;
        int        got_frame = 0;
        avcodec_decode_audio4(d->avctx, d->frame, &got_frame, pkt);
        datap = d->frame->data[0];
        bytes = d->frame->linesize[0];
        om    = allocb_ite(bytes);
        memcpy(om->b_wptr, datap, bytes);
        om->b_wptr += bytes;
        blk.datap = om;
        ite_queue_put(f->output[0].Qhandle, &blk);
        d->curpts = (double)pkt->pts;
        av_free_packet(pkt);
    }
    else
    {
        if (IteAudioQueueIsEmpty(f, 0))
        { // end of play;
            if (d->loop_after == Normal)
            {
                d->state = Eof;
                printf("eof m4a play\n");
            }
            else
            {
                printf("play repeat\n");
                avformat_seek_file(d->format_context, -1, INT64_MIN, 0,
                                   INT64_MAX, 0);
            }
        }
    }
}

static void dem4a_open (IteFilter * f, void * arg)
{
    DeM4a *      d      = (DeM4a *)f->data;
    IteFlower *  flower = (IteFlower *)f->srcFlow;
    const char * file   = (const char *)arg;
    int          idx;
    AVPacket     pkt1, *pkt = &pkt1;
    AVCodec *    codec;
    dem4a_close(f, NULL);
    avformat_open_input(&d->format_context, file, NULL, NULL);
    d->fd = d->format_context;
    if (d->fd == NULL)
    {
        printf("%s openfile error\n", file);
        d->state = Dummy;
        return;
    }
    idx = av_find_best_stream(d->format_context, AVMEDIA_TYPE_AUDIO, -1, -1,
                              NULL, 0);
    d->avctx = d->format_context->streams[idx]->codec;
    codec    = avcodec_find_decoder(d->avctx->codec_id);
    if (!codec || avcodec_open2(d->avctx, codec, NULL) < 0)
    {
        printf("[err] %s %d\n", __FUNCTION__, __LINE__);
        while (1)
            ;
    }

    if (av_read_frame(d->format_context, pkt) == 0)
    {
        int got_frame = 0;
        avcodec_decode_audio4(d->avctx, d->frame, &got_frame, pkt);
        av_free_packet(pkt);
        d->rate           = d->avctx->sample_rate;
        d->nch            = d->avctx->channels;

        flower->dInfo.sr  = d->rate;
        flower->dInfo.nch = d->nch;
        flower->dInfo.bytes20ms = 20 * d->rate * d->nch * 2 / 1000; // 20ms data
        flower->dInfo.bitsize   = 16;
        flower->dInfo.codectype = ITE_AAC_DECODE;
        flower->dInfo.init      = true;
        avformat_seek_file(d->format_context, -1, INT64_MIN, 0, INT64_MAX, 0);
        d->state = Playing;
        return;
    }
}

#if 0
static void dem4a_start(IteFilter *f, void *arg){
    DeM4a *d=(DeM4a*)f->data;
    pthread_mutex_lock(&d->mutex);
    if (d->state==Paused)
        d->state = Playing;
    else{
        d->state = Dummy;
        printf("MSdummyPlaying scilent\n");
    }
    pthread_mutex_unlock(&d->mutex);
}
#endif

static void dem4a_seek_pos (IteFilter * f, void * arg)
{
    DeM4a *      d           = (DeM4a *)f->data;
    unsigned int shift_sec   = *((int *)arg);
    int64_t      seek_target = (int64_t)shift_sec * 1000 * 1000;

    if (d->state != Playing)
    {
        printf("m4a not start play yet!!\n");
        return;
    }

    pthread_mutex_lock(&d->mutex);
    ite_mblk_queue_flush(f->output[0].Qhandle);
    avformat_seek_file(d->format_context, -1, INT64_MIN, seek_target, INT64_MAX,
                       0);
    pthread_mutex_unlock(&d->mutex);
    printf("dem4a_seek_pos to %ud sec\n", shift_sec);
}

static void dem4a_shift_pos (IteFilter * f, void * arg)
{
    DeM4a * d           = (DeM4a *)f->data;
    int32_t shift_sec   = *((int *)arg);
    int32_t seek_target = shift_sec * 1000 * 1000;

    if (d->state != Playing)
    {
        printf("m4a not start play yet!!\n");
        return;
    }

    pthread_mutex_lock(&d->mutex);
    ite_mblk_queue_flush(f->output[0].Qhandle);
    seek_target += (d->curpts / d->rate) * 1000 * 1000; // get current time
                                                        // stamp
    if (seek_target < 0)
    {
        seek_target = 0;
    }
    avformat_seek_file(d->format_context, -1, INT64_MIN, seek_target, INT64_MAX,
                       0);
    pthread_mutex_unlock(&d->mutex);
    printf("dem4a_shift_pos %ld sec\n", shift_sec);
}

static void dem4a_get_pos (IteFilter * f, void * arg)
{
    DeM4a * d = (DeM4a *)f->data;
    int     cur_pos;

    pthread_mutex_lock(&d->mutex);
    cur_pos = (d->curpts / d->rate) * 1000;
    pthread_mutex_unlock(&d->mutex);
    *(int *)arg = cur_pos;

    printf("player_get_pos %d ms\n", cur_pos);
}

static void dem4a_loop (IteFilter * f, void * arg)
{
    DeM4a * d     = (DeM4a *)f->data;
    d->loop_after = *((int *)arg);
}

//=============================================================================
//                              filter flow
//=============================================================================

static void dem4a_init (IteFilter * f)
{
    DeM4a * d = (DeM4a *)ite_new(DeM4a, 1);

    if (d != NULL)
    {
        d->fd         = NULL;
        d->state      = Closed;
        d->rate       = 8000;
        d->loop_after = -1; /*by default, don't loop*/
        d->curpts     = 0.0;
        pthread_mutex_init(&d->mutex, NULL);
        d->format_context = NULL;
        d->frame          = avcodec_alloc_frame();
        av_register_all();
        avcodec_register_all();
#ifdef CFG_NET_ENABLE
        avformat_network_init();
#endif
    }
    f->data = d;
}

static void dem4a_uninit (IteFilter * f)
{
    DeM4a * d = (DeM4a *)f->data;

    if (d != NULL)
    {
        dem4a_close(f, NULL);
        av_free(d->frame);
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void dem4a_process (IteFilter * f)
{
    DeM4a *     d      = (DeM4a *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    int         gbytes = flower->dInfo.bytes20ms;

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
        {
            dem4a_do(f);
        }

        if (d->state == Eof)
        {
            blk.datap    = allocb0_ite(gbytes);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        pthread_mutex_unlock(&d->mutex);

        if (d->state == Dummy)
        {
            blk.datap    = allocb0_ite(gbytes);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static IteMethodDes dem4a_methods[] = {
    { ITE_FILTER_FILEOPEN,      dem4a_open},
    { ITE_FILTER_SEEK_POS,  dem4a_seek_pos},
    {ITE_FILTER_SHIFT_POS, dem4a_shift_pos},
    {ITE_FILTER_GET_PARAM,   dem4a_get_pos},
    { ITE_FILTER_LOOPPLAY,      dem4a_loop},
    {                   0,            NULL}
};

// clang-format off
IteFilterDes FilterDeM4A = {
    ITE_FILTER_DEM4A_ID,
    dem4a_init,
    dem4a_uninit,
    dem4a_process,
    dem4a_methods
};
// clang-format on
