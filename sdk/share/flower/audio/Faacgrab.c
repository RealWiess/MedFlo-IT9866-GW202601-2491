#include <stdio.h>
#include <sys/stat.h>
#include "audio/include/fileheader.h"
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "aacdec.h"
#include "aaccommon.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

#define RINGBUFFSIZE (1024 * 6)
#define READSIZE     (1024 * 4)
#define PCMSZIE      (1024 * 4 * 2)

typedef struct _AACgrabData
{
    HAACDecoder *   hAACDecoder;
    rbuf_ite *      rBuf;
    void *          fd;
    int             rate;
    int             nchannels;
    int             loop_after;
    int             offset;
    PlayerState     state;
    int             framesize;
    pthread_mutex_t mutex;
    int             keep;
} AACgrabData;

static void _AACdataInfoSet (IteFilter * f)
{
    AACgrabData *   d          = (AACgrabData *)f->data;
    IteFlower *     flower     = (IteFlower *)f->srcFlow;
    AACDecInfo *    aacDecInfo = (AACDecInfo *)d->hAACDecoder;
    uint8_t         Buff[1024];
    uint8_t         outSample[PCMSZIE];
    unsigned char * inBuff;
    int             nextSync, bytesLeft, ret;
    int             ReadSize = fread(Buff, 1, 1024, d->fd);
    nextSync                 = AACFindSyncWord(Buff, 1024);
    bytesLeft                = 1024 - nextSync;
    inBuff                   = (unsigned char *)(Buff + nextSync);
    ret               = AACDecode(d->hAACDecoder, &inBuff, (int *)&bytesLeft,
                                  (short *)outSample);

    flower->dInfo.sr  = aacDecInfo->sampRate * (aacDecInfo->sbrEnabled ? 2 : 1);
    flower->dInfo.nch = aacDecInfo->nChans;
    flower->dInfo.bitsize = 16;
    flower->dInfo.bytes20ms =
        20 * flower->dInfo.sr * flower->dInfo.nch * 2 / 1000;
    flower->dInfo.codectype = ITE_AAC_DECODE;

    d->rate                 = flower->dInfo.sr;
    d->nchannels            = flower->dInfo.nch;
    d->framesize            = flower->dInfo.bytes20ms; // 20ms data
    d->keep                 = aacDecInfo->nFrameLength * 1.5;
    flower->dInfo.init      = true;
    EVEN(d->framesize);

    fseek(d->fd, d->offset, SEEK_SET);
}

static void _AAC_decoder (IteFilter * f)
{
    AACgrabData * d          = (AACgrabData *)f->data;
    AACDecInfo *  aacDecInfo = (AACDecInfo *)d->hAACDecoder;
    int           streamSize, nextSync, bytesLeft, lastFrameEnd = 0, ret;
    uint8_t       outSample[PCMSZIE];
    uint8_t *     inBuff;
    IteQueueblk   blk = {0};

    streamSize        = ite_rbuf_get_avail_size(d->rBuf);
    bytesLeft         = streamSize;
    while (bytesLeft >= d->keep)
    { // assume every frame > d->readsize
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

static void decode_do (IteFilter * f)
{
    AACgrabData * d      = (AACgrabData *)f->data;

    int           rbytes = fread(d->rBuf->b_wptr, 1, READSIZE, d->fd);
    d->rBuf->b_wptr += rbytes;
    if (rbytes > 0)
    {
        _AAC_decoder(f);
    }
    else
    {
        if (IteAudioQueueIsEmpty(f, 0))
        { // end of play;
            if (d->loop_after == Normal)
            {
                d->state = Eof;
                printf("eof aac play\n");
            }
            else
            {
                printf("play repeat\n");
                fseek(d->fd, d->offset, SEEK_SET);
            }
        }
    }
}

static void aacgrab_close (IteFilter * f)
{
    AACgrabData * d = (AACgrabData *)f->data;
    pthread_mutex_lock(&d->mutex);
    d->state = Closed;
    if (d->fd)
    {
        fclose(d->fd);
    }
    d->fd = NULL;
    pthread_mutex_unlock(&d->mutex);
}

static void aacgrab_open (IteFilter * f, void * arg)
{
    AACgrabData * d    = (AACgrabData *)f->data;
    const char *  file = (const char *)arg;

    if (d->fd)
    {
        aacgrab_close(f);
        ite_mblk_queue_flush(f->output[0].Qhandle); // flush previous data
    }

    d->fd = fopen(file, "rb");
    if (d->fd == NULL)
    {
        printf("%s openfile error\n", file);
        d->state = Dummy;
        return;
    }
    _AACdataInfoSet(f);
    printf("rate =%d ch = %d\n", d->rate, d->nchannels);
    d->state = Playing;
}

static void aacgrab_loop (IteFilter * f, void * arg)
{
    AACgrabData * d = (AACgrabData *)f->data;
    d->loop_after   = *((int *)arg);
}

//=============================================================================
//                              filter flow
//=============================================================================

static void aacgrab_init (IteFilter * f)
{
    AACgrabData * d = (AACgrabData *)ite_new(AACgrabData, 1);
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
        d->keep       = 512;
        pthread_mutex_init(&d->mutex, NULL);
        d->hAACDecoder       = (HAACDecoder *)AACInitDecoder();
        f->data              = d;
        f->pthread_stacksize = 10 * 1024;
        if (RINGBUFFSIZE < READSIZE)
        {
            printf("aacgrab_init error!!!!!!\n");
        }
    }
    else
    {
        f->data = NULL;
    }
}

static void aacgrab_uninit (IteFilter * f)
{
    AACgrabData * d = (AACgrabData *)f->data;
    if (d != NULL)
    {
        aacgrab_close(f);
        pthread_mutex_destroy(&d->mutex);
        ite_rbuf_free(d->rBuf);
        free(d);
    }
}

static void aacgrab_process (IteFilter * f)
{
    AACgrabData * d      = (AACgrabData *)f->data;
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
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        if (d->state == Eof)
        { // add dummy time(data) prevent abnormal sound
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static IteMethodDes aacgrab_methods[] = {
    {ITE_FILTER_FILEOPEN, aacgrab_open},
    {ITE_FILTER_LOOPPLAY, aacgrab_loop},
    {                  0,         NULL}
};

// clang-format off
IteFilterDes FilterAACGrab = {
    ITE_FILTER_AACGRAB_ID,
    aacgrab_init,
    aacgrab_uninit,
    aacgrab_process,
    aacgrab_methods
};
// clang-format on
