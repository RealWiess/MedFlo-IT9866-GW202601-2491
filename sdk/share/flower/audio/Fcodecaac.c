#include <stdio.h>
#include "flower/flower.h"
// #include "audio/include/audioqueue.h"
#include "audio/include/fileheader.h"
#include "aacdec.h"
#include "aaccommon.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

#define RINGBUFFSIZE (1024 * 6)
#define PCMSZIE      (1024 * 4 * 2)

typedef struct _CodecAACData
{
    PlayerState   state;
    HAACDecoder * hAACDecoder;
    rbuf_ite *    rBuf;
    int           framesize;
    bool          eof;
    int           keep;
} CodecAACData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void _aac_decoder (IteFilter * f)
{
    CodecAACData * d          = (CodecAACData *)f->data;
    AACDecInfo *   aacDecInfo = (AACDecInfo *)d->hAACDecoder;
    int            streamSize, nextSync, bytesLeft, lastFrameEnd = 0, ret;
    uint8_t        outSample[PCMSZIE];
    uint8_t *      inBuff;
    IteQueueblk    blk = {0};

    streamSize         = ite_rbuf_get_avail_size(d->rBuf);
    bytesLeft          = streamSize;
    while (bytesLeft >= d->keep && f->run)
    { // assume every frame > 512
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }
        nextSync =
            AACFindSyncWord((unsigned char *)(d->rBuf->b_rptr + lastFrameEnd),
                            streamSize - lastFrameEnd);
        lastFrameEnd += nextSync; // nextSync = 0 (usually)
        inBuff    = (unsigned char *)(d->rBuf->b_rptr + lastFrameEnd);
        bytesLeft = streamSize - lastFrameEnd;

        ret       = AACDecode(d->hAACDecoder, &inBuff, (int *)&bytesLeft,
                              (short *)outSample);

        if (ret)
        {
            // Error, skip the frame...
            printf("AAC decode error %d\n", ret);
            lastFrameEnd = streamSize;
            if (d->keep < aacDecInfo->nFrameLength)
            {
                d->keep = aacDecInfo->nFrameLength;
            }
            break;
        }
        else
        {
            mblk_ite * om = NULL;
            lastFrameEnd += aacDecInfo->nFrameLength;
            AACFrameInfo fi;
            AACGetLastFrameInfo(d->hAACDecoder, &fi);
            int nPCMBytes = fi.bitsPerSample / 8 * fi.outputSamps;
            if (d->state == Playing)
            {
                om = allocb_ite(nPCMBytes);
                memcpy(om->b_rptr, outSample, nPCMBytes);
                om->b_wptr += nPCMBytes;
                blk.datap = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
        }
    }
    ite_rbuf_rp_shift(d->rBuf, lastFrameEnd);
    ite_rbuf_rearrange(d->rBuf);
}

static void codec_do (IteFilter * f)
{
    CodecAACData * d   = (CodecAACData *)f->data;
    IteQueueblk    blk = {0};
    if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
    { // put data to codec(Closed &Playing) , put to next filter(Playing)
        mblk_ite * im = blk.datap;
        if (im && im->size == 0 || blk.private1 == (void *)Eofsound)
        {
            printf("finish stream Recv\n");
            freemsg_ite(im);
            d->eof = true;
            return;
        }
        if (im)
        {
            ite_rbuf_put(d->rBuf, im->b_rptr, im->size);
            _aac_decoder(f);
            freemsg_ite(im);
        }
    }
    else
    { // get data from codec put to next filter
        if (d->state == Playing)
        {
            _aac_decoder(f);
            if (d->eof && IteAudioQueueIsEmpty(f, 0))
            {
                printf("play eof\n");
                d->state = Eof;
            }
        }
    }
}

static void codecaac_open (IteFilter * f, void * arg)
{
    CodecAACData * d    = (CodecAACData *)f->data;
    const char *   file = (const char *)arg;
}

static int codecaac_cpu1 (IteFilter * f, void * arg)
{
    CodecAACData *   d          = (CodecAACData *)f->data;
    AACDecInfo *     aacDecInfo = (AACDecInfo *)d->hAACDecoder;
    IteFlower *      flower     = (IteFlower *)f->srcFlow;
    IteAudioFlower * s          = flower->audiostream;
    rbuf_ite *       RingBuf    = d->rBuf;
    unsigned char *  inBuff;
    uint8_t          outSample[PCMSZIE];
    int              nextSync, bytesLeft, ret;
    int              availSize = 0;
    IteQueueblk      blk       = {0};

    while (availSize < 2048 && f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * im = blk.datap;
            if (im)
            {
                ite_rbuf_put(RingBuf, im->b_rptr, im->size);
                if (im)
                {
                    freemsg_ite(im);
                }
            }
        }
        availSize = ite_rbuf_get_avail_size(RingBuf);
    }

    nextSync  = AACFindSyncWord((unsigned char *)(d->rBuf->b_rptr), availSize);
    bytesLeft = availSize - nextSync;
    inBuff    = (unsigned char *)(d->rBuf->b_rptr + nextSync);
    ret       = AACDecode(d->hAACDecoder, &inBuff, (int *)&bytesLeft,
                          (short *)outSample);

    if (flower->dInfo.init == false)
    {
        flower->dInfo.sr =
            aacDecInfo->sampRate * (aacDecInfo->sbrEnabled ? 2 : 1);
        flower->dInfo.nch     = aacDecInfo->nChans;
        flower->dInfo.bitsize = 16;
        flower->dInfo.bytes20ms =
            20 * flower->dInfo.sr * flower->dInfo.nch * 2 / 1000;
        flower->dInfo.codectype = ITE_AAC_DECODE;
        flower->dInfo.init      = true;
    }

    if (s->Fdestinat && s->Fdestinat->filterDes.id == ITE_FILTER_DAWRITE_ID)
    {
        dawrite_reinit_for_diff_rate(flower->dInfo.sr, 16, flower->dInfo.nch);
    }

    d->framesize = flower->dInfo.bytes20ms; // 20ms data
    d->keep      = aacDecInfo->nFrameLength * 1.5;
    EVEN(d->framesize);

    d->state = Playing;
    d->eof   = false;
    printf("decode AAC CPU1\n");
    return 0;
}

static void codecaac_init (IteFilter * f)
{
    CodecAACData * d = (CodecAACData *)ite_new(CodecAACData, 1);
    if (d != NULL)
    {
        d->state             = Closed;
        d->framesize         = 320;
        d->rBuf              = ite_rbuf_init(RINGBUFFSIZE);
        d->keep              = 512;
        d->hAACDecoder       = (HAACDecoder *)AACInitDecoder();
        d->eof               = false;
        f->pthread_stacksize = 10 * 1024;
    }
    f->data = d;
}

static void codecaac_uninit (IteFilter * f)
{
    CodecAACData * d = (CodecAACData *)f->data;
    if (d != NULL)
    {
        ite_rbuf_free(d->rBuf);
        free(d);
    }
}

static void codecaac_process (IteFilter * f)
{
    CodecAACData * d      = (CodecAACData *)f->data;
    IteFlower *    flower = (IteFlower *)f->srcFlow;
    codecaac_cpu1(f, NULL);

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

static IteMethodDes codecaac_methods[] = {
    {ITE_FILTER_FILEOPEN, codecaac_open},
    {                  0,          NULL}
};

// clang-format off
IteFilterDes FilterCodecAAC = {
    ITE_FILTER_CODECAAC_ID,
    codecaac_init,
    codecaac_uninit,
    codecaac_process,
    codecaac_methods
};
// clang-format on
