#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr (0xB0200080 + 0x18)
#define CODEC_RW_GAP                                                           \
    ((d->WP >= d->RP) ? (d->WP - d->RP) : ((d->codelen - d->RP) + d->WP))

typedef struct _M4aGrab
{
    void *            fd;
    PlayerState       state;
    int               rate;
    int               bitsize;
    int               nchannels;
    int               loop_after;
    int               codecType;
    int               framesize;
    bool              fileRead;

    uint8_t *         codecbuf;
    int               codelen;
    unsigned int      RP;
    unsigned int      WP;
    pthread_mutex_t   mutex;

    AVCodecContext *  avctx;
    AVFormatContext * format_context;
    AVFrame *         frame;
    AVPacket          pkt1;
    AVPacket *        pkt;
    double            curpts;
} M4aGrab;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static int m4agrab_close (IteFilter * f, void * arg)
{
    M4aGrab * d = (M4aGrab *)f->data;
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
    if (d->pkt)
    {
        av_free_packet(d->pkt);
    }
    if (i2s_get_DA_running())
    {
        iteAudioStopQuick();
        // i2s_deinit_DAC();
    }
    pthread_mutex_unlock(&d->mutex);
    return 0;
}

static mblk_ite * codec_data (M4aGrab * d, int length)
{
    mblk_ite * rm       = NULL;
    uint32_t   rp       = d->RP;
    uint8_t *  codecbuf = d->codecbuf;
    uint32_t   codelen  = d->codelen;
    uint32_t   size     = length;

    if (length < 0)
    {
        size = CODEC_RW_GAP;
    }

    rm = allocb_ite(size);

    if (rp + size <= codelen)
    {
        ithInvalidateDCacheRange(codecbuf + rp, size);
        memcpy(rm->b_wptr, codecbuf + rp, size);
        rp += size;
    }
    else
    { // rp > wp
        uint32_t szsec0 = codelen - rp;
        uint32_t szsec1 = size - szsec0;
        if (szsec0)
        {
            ithInvalidateDCacheRange(codecbuf + rp, szsec0);
            memcpy(rm->b_wptr, codecbuf + rp, szsec0);
        }
        ithInvalidateDCacheRange(codecbuf, szsec1);
        memcpy(rm->b_wptr + szsec0, codecbuf, szsec1);
        rp = szsec1;
    }

    rm->b_wptr += size;
    d->RP = rp;
    ithWriteRegA(DrvDumpPCM_RdPtr, d->RP); /*record PCM RP */

    return rm;
}
/*
static uint32_t getpkt_timestamp(AVPacket *pkt,M4aGrab *d){//just for audio
    uint32_t cur_stamp=((double)pkt->pts/d->rate)*1000*1000;
    return cur_stamp;
}
*/

static void m4aPktDecoderPut (IteFilter * f)
{
    M4aGrab *     d         = (M4aGrab *)f->data;
    unsigned long availSize = 0;
    int           got_frame = 0;
    if (d->pkt != NULL)
    {
        iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER, &availSize);
        if (availSize > d->pkt->size)
        {
            avcodec_decode_audio4(d->avctx, d->frame, &got_frame, d->pkt);
            d->curpts = (double)d->pkt->pts;
            if (d->pkt)
            {
                av_free_packet(d->pkt);
            }
            d->pkt = NULL;
        }
    }
}

static void m4agrab_do (IteFilter * f)
{
    M4aGrab * d = (M4aGrab *)f->data;
    iteAudioUpdateMessageQ();
    if (d->fileRead)
    {
        if (d->pkt)
        {
            m4aPktDecoderPut(f);
        }
        else
        {
            d->pkt = &d->pkt1;
            if (av_read_frame(d->format_context, d->pkt) != 0)
            {
                d->fileRead = false;
            }
        }
    }
    else
    {
        if (IteAudioQueueIsEmpty(f, 0))
        {
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
                d->fileRead = true;
            }
        }
    }

    // get pcm data(decoded) from codec and put to next filter
    d->WP = iteAudioCodecGetPcmIdx();
    if (d->RP != d->WP)
    {
        if (d->state == Closed)
        { // wait codec parsing i2s info
            // d->RP=d->WP;
            // ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);
            // printf("d->state==Closed\n");
        }
        else
        { // get pcm data from codec;
            if (CODEC_RW_GAP >= d->framesize)
            {
                IteQueueblk blk = {0};
                mblk_ite *  om  = codec_data(d, d->framesize);
                blk.datap       = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
        }
    }
}

static void m4agrab_open (IteFilter * f, void * arg)
{
    M4aGrab *   d      = (M4aGrab *)f->data;
    char *      file   = (char *)arg;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    int         tmp = 0, cnt = 0;
    int         idx;
    AVCodec *   codec;
    m4agrab_close(f, NULL);
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
    // iteAudioOpenEngine(d->codecType);//load codec engine
    d->codecType = audiomgrCodecType(file);
    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr, d->RP);
    iteAudioSetMusicCodecDump(1); // set dump flag
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT, &tmp);

    while (tmp == 0)
    { // wait codec parsing i2s info;
        m4agrab_do(f);

        if (cnt++ > 200)
        {
            printf("timeout error %s %d\n", __FUNCTION__, __LINE__);
            if (cnt > 210)
            {
                printf("error init i2s\n");
            }
            break;
        }
        usleep(10000);
        iteAudioGetAttrib(ITE_AUDIO_I2S_INIT, &tmp);
        iteAudioUpdateMessageQ();
    }
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_CHANNEL, &d->nchannels);
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_SAMPLE_RATE, &d->rate);
    iteAudioGetAttrib(ITE_AUDIO_I2S_PTR, &d->codecbuf);
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_BUFFER_LENGTH, &d->codelen);
    printf("%d %d %d\n", d->rate, d->nchannels, tmp);
    d->framesize = 20 * d->rate * d->nchannels * 2 / 1000;
    EVEN(d->framesize);
    // if(!d->bypass)
    //     tmp=Castor3snd_reinit_for_diff_rate(d->rate,16,d->nchannels);

    flower->dInfo.sr        = d->rate;
    flower->dInfo.nch       = d->nchannels;
    flower->dInfo.bytes20ms = d->framesize; // 20ms data
    flower->dInfo.bitsize   = 16;
    flower->dInfo.codectype = d->codecType;
    flower->dInfo.init      = true;
    d->framesize            = flower->dInfo.bytes20ms;
    d->state                = Playing;

    return;
}

#if 0
static void m4agrab_start(IteFilter *f, void *arg){
    M4aGrab *d=(M4aGrab*)f->data;
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

static void m4agrab_seek_pos (IteFilter * f, void * arg)
{
    M4aGrab *    d           = (M4aGrab *)f->data;
    unsigned int shift_sec   = *((int *)arg);
    uint64_t     seek_target = (uint64_t)shift_sec * 1000 * 1000;

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
    printf("m4agrab_seek_pos to %ud sec\n", shift_sec);
}

static void m4agrab_shift_pos (IteFilter * f, void * arg)
{
    M4aGrab * d           = (M4aGrab *)f->data;
    int32_t   shift_sec   = *((int *)arg);
    int32_t   seek_target = shift_sec * 1000 * 1000;

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
    printf("m4agrab_shift_pos %ld sec\n", shift_sec);
}

static void m4agrab_get_pos (IteFilter * f, void * arg)
{
    M4aGrab * d = (M4aGrab *)f->data;
    int       cur_pos;

    pthread_mutex_lock(&d->mutex);
    cur_pos = (d->curpts / d->rate) * 1000;
    pthread_mutex_unlock(&d->mutex);
    *(int *)arg = cur_pos;

    printf("player_get_pos %d ms\n", cur_pos);
}

static void m4agrab_loop (IteFilter * f, void * arg)
{
    M4aGrab * d   = (M4aGrab *)f->data;
    d->loop_after = *((int *)arg);
}

//=============================================================================
//                              filter flow
//=============================================================================

static void m4agrab_init (IteFilter * f)
{
    M4aGrab * d = (M4aGrab *)ite_new(M4aGrab, 1);

    if (d != NULL)
    {
        d->fd         = NULL;
        d->state      = Closed;
        d->rate       = 8000;
        d->bitsize    = 16;
        d->nchannels  = 1;
        d->loop_after = -1; /*by default, don't loop*/
        d->framesize  = 320;
        d->RP         = 0;
        d->WP         = 0;
        d->codecType  = -1;
        d->curpts     = 0.0;
        d->fileRead   = true;
        d->pkt        = NULL;
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

static void m4agrab_uninit (IteFilter * f)
{
    M4aGrab * d = (M4aGrab *)f->data;

    if (d != NULL)
    {
        m4agrab_close(f, NULL);
        av_free(d->frame);
        iteAudioSetMusicCodecDump(0);
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void m4agrab_process (IteFilter * f)
{
    M4aGrab *   d      = (M4aGrab *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;

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
            m4agrab_do(f);
        }
        if (d->state == Eof)
        {
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        pthread_mutex_unlock(&d->mutex);

        if (d->state == Dummy)
        {
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static IteMethodDes m4agrab_methods[] = {
    { ITE_FILTER_FILEOPEN,      m4agrab_open},
    { ITE_FILTER_SEEK_POS,  m4agrab_seek_pos},
    {ITE_FILTER_SHIFT_POS, m4agrab_shift_pos},
    {ITE_FILTER_GET_PARAM,   m4agrab_get_pos},
    { ITE_FILTER_LOOPPLAY,      m4agrab_loop},
    {                   0,              NULL}
};

// clang-format off
IteFilterDes FilterM4aGrab = {
    ITE_FILTER_M4AGRAB_ID,
    m4agrab_init,
    m4agrab_uninit,
    m4agrab_process,
    m4agrab_methods
};
// clang-format on
