#include <stdio.h>
#include "flower/flower.h"
// #include "audio/include/audioqueue.h"
#include "include/fileheader.h"
#include "mp3decode.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define RINGBUFFSIZE 1024 * 20
// extern int gbytes;
typedef struct _CodecMp3Data
{
    PlayerState     state;
    AVCodecContext  gMpegAudioContext;
    AVFrame         gFrame;
    rbuf_ite *      rBuf;
    int             rate;
    int             bitsize;
    int             nchannels;
    int             codecType;
    pthread_mutex_t mutex;
    int             framesize;
    bool            eof;
} CodecMp3Data;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int MP3decoder (IteFilter * f)
{
    CodecMp3Data * d           = (CodecMp3Data *)f->data;
    IteQueueblk    blk         = {0};
    int            ret         = 0;
    int            got_picture = 0;
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

static void codec_do (IteFilter * f)
{
    CodecMp3Data * d   = (CodecMp3Data *)f->data;
    IteQueueblk    blk = {0};

    if (!MP3decoder(f))
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        { // put data to codec(Closed &Playing) , put to next filter(Playing)
            mblk_ite * im        = blk.datap;
            int        availSize = 0;
            if (im && im->size == 0 || (PlaySoundCase)blk.private1 == Eofsound)
            {
                printf("finish stream Recv\n");
                d->eof = true;
            }
            else
            {
                ite_rbuf_rearrange(d->rBuf); // memove to header
                if (im)
                {
                    ite_rbuf_put(d->rBuf, im->b_rptr, im->size);
                }
            }
            freemsg_ite(im);
        }
        else
        {
            if (d->eof && IteAudioQueueIsEmpty(f, 0))
            {
                printf("play eof\n");
                d->state = Eof;
            }
        }
    }
}

static void codecmp3_open (IteFilter * f, void * arg)
{
    CodecMp3Data * d    = (CodecMp3Data *)f->data;
    char *         file = (char *)arg;
    d->codecType        = audiomgrCodecType(file);
}

static void codecmp3_cpu1 (IteFilter * f, void * arg)
{
    CodecMp3Data *   d       = (CodecMp3Data *)f->data;
    IteFlower *      flower  = (IteFlower *)f->srcFlow;
    IteAudioFlower * s       = flower->audiostream;
    rbuf_ite *       RingBuf = d->rBuf;

    id3tagParsing(f, RingBuf); // id3 parsing
    parsingMp3Stream(RingBuf->b_rptr, ite_rbuf_get_avail_size(RingBuf),
                     &flower->dInfo);

    d->rate            = flower->dInfo.sr;
    d->nchannels       = flower->dInfo.nch;
    d->framesize       = flower->dInfo.bytes20ms; // 20ms data
    d->codecType       = flower->dInfo.codectype;
    flower->dInfo.init = true;
    printf("sr=%d ch=%d framesize=%d\n", d->rate, d->nchannels, d->framesize);

    if (s->Fdestinat && s->Fdestinat->filterDes.id == ITE_FILTER_DAWRITE_ID)
    {
        dawrite_reinit_for_diff_rate(flower->dInfo.sr, 16, flower->dInfo.nch);
    }

    d->state = Playing;
    d->eof   = false;
    printf("decode MP3 CPU1\n");
}

static void codecmp3_get_rate (IteFilter * f, void * arg)
{
    CodecMp3Data * d = (CodecMp3Data *)f->data;
    *(int *)arg      = d->rate;
}

static void codecmp3_get_channels (IteFilter * f, void * arg)
{
    CodecMp3Data * d = (CodecMp3Data *)f->data;
    *(int *)arg      = d->nchannels;
}

static void codecmp3_init (IteFilter * f)
{
    CodecMp3Data * d = (CodecMp3Data *)ite_new(CodecMp3Data, 1);
    if (d != NULL)
    {
        d->state     = Closed;
        d->rate      = 8000;
        d->bitsize   = 16;
        d->nchannels = 1;
        d->framesize = 320;
        d->codecType = -1;
        d->rBuf      = ite_rbuf_init(RINGBUFFSIZE);
        // pthread_mutex_init(&d->mutex,NULL);
        memset(&d->gMpegAudioContext, 0, sizeof(d->gMpegAudioContext));
        decode_init(&d->gMpegAudioContext);
        d->eof               = false;
        f->pthread_stacksize = 8 * 1024;
    }
    f->data = d;
}

static void codecmp3_uninit (IteFilter * f)
{
    CodecMp3Data * d = (CodecMp3Data *)f->data;
    if (d != NULL)
    {
        ite_rbuf_free(d->rBuf);
        decode_deinit(&(d->gMpegAudioContext));
        free(d);
    }
}

static void codecmp3_process (IteFilter * f)
{
    CodecMp3Data * d      = (CodecMp3Data *)f->data;
    IteFlower *    flower = (IteFlower *)f->srcFlow;
    codecmp3_cpu1(f, NULL);

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

        if (d->state == Playing)
        {
            codec_do(f);
        }

        if (d->state == Eof)
        {
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static IteMethodDes codecmp3_methods[] = {
    {     ITE_FILTER_FILEOPEN,         codecmp3_open},
    {      ITE_FILTER_GETRATE,     codecmp3_get_rate},
    {ITE_FILTER_GET_NCHANNELS, codecmp3_get_channels},
    {                       0,                  NULL}
};

// clang-format off
IteFilterDes FilterCodecMp3 = {
    ITE_FILTER_CODECMP3_ID,
    codecmp3_init,
    codecmp3_uninit,
    codecmp3_process,
    codecmp3_methods
};
// clang-format on
