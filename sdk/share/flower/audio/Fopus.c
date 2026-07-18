#include <stdio.h>
#include "flower/flower.h"
#include "include/opus.h"

typedef struct _OpusData
{
    OpusEncoder * encoder;
    OpusDecoder * decoder;
} OpusData;

static void opus_codec_creat(IteFilter *f,int sr,int ch)
{
    OpusData *  d      = (OpusData *)f->data;
    if (sr != 48000 && sr != 24000 && sr != 16000 && sr != 12000 && sr != 8000)
    {
        printf("opus codec creat error,unsupport sr=%d!!!\n", sr);
        for (;;)
        {
        }
    }

    switch (f->filterDes.id)
    {
        case ITE_FILTER_CODECOPUSDE_ID:
            d->decoder = opus_decoder_create(sr, ch, &err);
            break;
        case ITE_FILTER_CODECOPUSEN_ID:
            d->encoder =
                opus_encoder_create(sr, ch, OPUS_APPLICATION_VOIP, &err);
            break;
    }
}

static void opus_init (IteFilter * f)
{
    OpusData * d = (OpusData *)ite_new(OpusData, 1);
    f->data      = d;
}

static void opus_uninit (IteFilter * f)
{
    OpusData * d = (OpusData *)f->data;
    if (d != NULL)
    {
        switch (f->filterDes.id)
        {
            case ITE_FILTER_CODECOPUSDE_ID:
                opus_decoder_destroy(d->decoder);
                break;
            case ITE_FILTER_CODECOPUSEN_ID:
                opus_encoder_destroy(d->encoder);
                break;
        }
        free(d);
    }
}

static void opusdec_process (IteFilter * f)
{
#define DECSIZE 1024 * 8 // max decode frame 120ms
    OpusData *  d   = (OpusData *)f->data;
    IteFlower *flower=(IteFlower*)f->srcFlow;
    int sr=flower->dInfo.sr;
    int ch=flower->dInfo.nch;
    IteQueueblk blk = {0};
    uint16_t    decBuf[DECSIZE];
    int frame_len = 0; // sample number (assume 1 pcm sample = 16bit(2byte))

    opus_codec_creat(f,sr,ch);

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

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * m = blk.datap;
            mblk_ite * om;

            frame_len = opus_decode(
                d->decoder, (const unsigned char *)m->b_rptr,
                (opus_int32)(m->size), (opus_int16 *)decBuf, DECSIZE, 0);
            frame_len*=(2*ch);
            if (frame_len < 0 || frame_len >= DECSIZE)
            {
                printf("%s %d frame_len*2=%d error\n", __FUNCTION__, __LINE__,
                       frame_len * sizeof(uint16_t));
            }

            om = allocb_ite(frame_len);
            memcpy(om->b_wptr, decBuf, frame_len);
            om->b_wptr += (frame_len);
            blk.datap = om;
            ite_queue_put(f->output[0].Qhandle, &blk);
            if (m)
            {
                freemsg_ite(m);
            }
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void opusenc_process (IteFilter * f)
{
#define ENCSIZE 512
    OpusData *  d   = (OpusData *)f->data;
    IteFlower *flower=(IteFlower*)f->srcFlow;
    int sr=flower->dInfo.sr;
    int ch=flower->dInfo.nch;
    IteQueueblk blk = {0};
    char        encBuf[ENCSIZE];
    int         len; // encoder byte len

    opus_codec_creat(f,sr,ch);

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

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * m = blk.datap;
            mblk_ite * om;
            len = opus_encode(d->encoder, (const opus_int16 *)m->b_rptr,
                              m->size / (2*ch), encBuf, sizeof(encBuf));
            if (len < 0 || len >= ENCSIZE)
            {
                printf("%s %d len=%d error\n", __FUNCTION__, __LINE__, len);
            }
            om = allocb_ite(len);
            memcpy(om->b_wptr, encBuf, len);
            om->b_wptr += len;
            blk.datap = om;
            ite_queue_put(f->output[0].Qhandle, &blk);
            if (m)
            {
                freemsg_ite(m);
            }
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void opus_set_method (IteFilter * f, void * arg)
{
}

static IteMethodDes opus_methods[] = {
    {ITE_FILTER_A_Method, opus_set_method},
    {                  0,            NULL}
};

// clang-format off
IteFilterDes FilterOpusDec = {
    ITE_FILTER_CODECOPUSDE_ID,
    opus_init,
    opus_uninit,
    opusdec_process,
    opus_methods
};

IteFilterDes FilterOpusEnc = {
    ITE_FILTER_CODECOPUSEN_ID,
    opus_init,
    opus_uninit,
    opusenc_process,
    opus_methods
};
// clang-format on
