#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "audio/include/fileheader.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
#include "ite/audio.h"
#include "include/opusfile.h"
#include "io/internal.h"

#define RINGBUFFSIZE 1024 * 25

typedef struct _CodecOpusData
{
    OggOpusFile *   of;
    PlayerState     state;
    bool            eof;
    int             framesize;
    rbuf_ite *      rBuf;
    pthread_mutex_t mutex;
} CodecOpusData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int OPUSdecoder (IteFilter * f)
{
    CodecOpusData * d      = (CodecOpusData *)f->data;
    IteFlower *     flower = (IteFlower *)f->srcFlow;
    OggOpusFile *   OPUS   = d->of;
    int             ch     = flower->dInfo.nch;
    char            buf[4096];
    int             ret = op_read(OPUS, (void *)buf, sizeof(buf), NULL);
    if (ret > 0)
    {
        int         len = 2 * ret * ch;
        IteQueueblk blk = {0};
        mblk_ite *  om  = allocb_ite(len);
        memcpy(om->b_wptr, buf, len);
        om->b_wptr += len;
        blk.datap = om;
        ite_queue_put(f->output[0].Qhandle, &blk);
    }
    return ret;
}

static void opus_do (IteFilter * f)
{
    CodecOpusData * d       = (CodecOpusData *)f->data;
    IteQueueblk     blk     = {0};
    rbuf_ite *      RingBuf = d->rBuf;
    OggOpusFile *   OPUS    = d->of;

    if (OPUSdecoder(f) <= 0)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        { // put data to codec(Closed &Playing) , put to next filter(Playing)
            mblk_ite * im    = blk.datap;
            int        avail = 0;
            if (im && im->size == 0 || (PlaySoundCase)blk.private1 == Eofsound)
            {
                printf("finish stream Recv\n");
                if (im)
                {
                    freemsg_ite(im);
                }
                d->eof = true;
                return;
            }
            ite_rbuf_rp_shift(RingBuf, (int)OPUS->offset);
            ite_rbuf_rearrange(RingBuf); // memove to header
            if (im)
            {
                ite_rbuf_put(RingBuf, im->b_rptr, im->size);
                avail = ite_rbuf_get_avail_size(RingBuf);
                op_raw_mem_seek(OPUS, RingBuf->b_rptr, avail, (opus_int64)0);
                // printf("OPUS->offset=%d\n",(int)OPUS->offset);
                freemsg_ite(im);
            }
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

static void codecopus_open (IteFilter * f, void * arg)
{
    CodecOpusData * d = (CodecOpusData *)f->data;
}

static void codecopus_cpu1 (IteFilter * f, void * arg)
{

    CodecOpusData * d       = (CodecOpusData *)f->data;
    IteFlower *     flower  = (IteFlower *)f->srcFlow;
    rbuf_ite *      RingBuf = d->rBuf;
    IteQueueblk     blk     = {0};
    int             ret;
    int             availSize = 0;
    while (availSize < 1024 * 24 && f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * im = blk.datap;
            if (im)
            {
                ite_rbuf_put(RingBuf, im->b_rptr, im->size);
                freemsg_ite(im);
            }
        }
        availSize = ite_rbuf_get_avail_size(RingBuf);
    }
    d->of = op_open_memory(d->rBuf->b_rptr, RINGBUFFSIZE, NULL);
    {
        const OpusHead * head = op_head(d->of, 0);
        printf("sr=%d ch=%d\n", head->input_sample_rate, head->channel_count);
        flower->dInfo.sr        = head->input_sample_rate;
        flower->dInfo.nch       = head->channel_count;
        flower->dInfo.bytes20ms = 20 * 2 * head->input_sample_rate *
                                  head->channel_count / 1000; // 20ms data
        flower->dInfo.init = true;
        EVEN(flower->dInfo.bytes20ms);
        d->framesize = flower->dInfo.bytes20ms;
    }
    op_raw_mem_seek(d->of, RingBuf->b_rptr, availSize, (opus_int64)0);
    d->state = Playing;
    printf("decode OPUS CPU1 %d\n", (int)d->of->offset);
}

static void codecopus_init (IteFilter * f)
{
    CodecOpusData * d = (CodecOpusData *)ite_new(CodecOpusData, 1);
    if (d != NULL)
    {
        d->state     = Closed;
        d->framesize = 320; // 20*(2*d->rate*d->nchannels)/1000;//20ms data
        d->rBuf      = ite_rbuf_init(RINGBUFFSIZE);
        d->eof       = false;

        pthread_mutex_init(&d->mutex, NULL);
    }
    f->data = d;
}

static void codecopus_uninit (IteFilter * f)
{
    CodecOpusData * d = (CodecOpusData *)f->data;
    if (d != NULL)
    {
        ite_rbuf_free(d->rBuf);
        if (d->of != NULL)
        {
            op_free(d->of);
        }
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void codecopus_process (IteFilter * f)
{
    CodecOpusData * d      = (CodecOpusData *)f->data;
    IteFlower *     flower = (IteFlower *)f->srcFlow;
    codecopus_cpu1(f, NULL);

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
            opus_do(f);
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

static IteMethodDes codecopus_methods[] = {
    {ITE_FILTER_FILEOPEN, codecopus_open},
    {                  0,           NULL}
};

// clang-format off
IteFilterDes FilterCodecOpus = {
    ITE_FILTER_CODECOPUS_ID,
    codecopus_init,
    codecopus_uninit,
    codecopus_process,
    codecopus_methods
};
// clang-format on
