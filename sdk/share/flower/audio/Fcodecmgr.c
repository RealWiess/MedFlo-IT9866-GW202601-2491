#include <stdio.h>
#include "flower/flower.h"
// #include "audio/include/audioqueue.h"
#include "audio/include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr (0xB0200080 + 0x18)
#define CODEC_RW_GAP                                                           \
    ((d->WP >= d->RP) ? (d->WP - d->RP) : ((d->codelen - d->RP) + d->WP))

    // extern int gbytes;
typedef struct _CodecMgrData
{
    PlayerState  state;
    int          rate;
    int          bitsize;
    int          nchannels;
    int          codecType;
    mblk_ite *   bufm;

    int          nbytes;
    uint8_t *    codecbuf;
    int          codelen;
    unsigned int RP;
    unsigned int WP;
    int          framesize;
    bool         eof;
} CodecMgrData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static mblk_ite * codec_data (CodecMgrData * d, int length)
{
    mblk_ite * rm       = NULL;
    uint32_t   rp       = d->RP;
    // uint32_t wp=d->WP;
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

static void codec_do (IteFilter * f)
{
    CodecMgrData * d   = (CodecMgrData *)f->data;
    IteQueueblk    blk = {0};
    iteAudioUpdateMessageQ();

    if (!d->eof || d->bufm != NULL)
    {
        mblk_ite * im = d->bufm;
        if (d->bufm == NULL && (ite_queue_get(f->input[0].Qhandle, &blk) == 0))
        { // get data for last Q or bufm
            im = blk.datap;
            if ((im && (im->size == 0)) || (blk.private1 == (void *)Eofsound))
            {
                printf("finish stream Recv\n");
                if (im)
                {
                    freemsg_ite(im);
                }
                d->eof = true;
                return;
            }
        }

        if (im)
        {
            unsigned long availSize = 0;
            iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER,
                                             &availSize);
            if (availSize > im->size)
            {                                  // check cpu2 buf>input size
                iteAudioWriteStream(im->b_rptr,
                                    im->size); // write data for decoder
                if (im)
                {
                    freemsg_ite(im);
                }
                d->bufm = NULL;
            }
            else
            {
                d->bufm = im;
            }
        }
    }
    else
    {
        if (IteAudioQueueIsEmpty(f, 0))
        {
            printf("play eof\n");
            d->state = Eof;
        }
    }

    // data put to next filter
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

static void codecmgr_open (IteFilter * f, void * arg)
{
    CodecMgrData * d    = (CodecMgrData *)f->data;
    const char *   file = (const char *)arg;
    int            tmp  = 0;
    d->codecType        = audiomgrCodecType((char *)file);
    iteAudioSetMusicCodecDump(1);
    iteAudioOpenEngine(d->codecType);
    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr, d->RP); /*record PCM RP */
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT, &tmp);
}

static int codecmgr_cpu2 (IteFilter * f, void * arg)
{
    CodecMgrData *   d      = (CodecMgrData *)f->data;
    IteFlower *      flower = (IteFlower *)f->srcFlow;
    IteAudioFlower * s      = flower->audiostream;
    int              tmp    = 0;
    // IteQueueblk blk ={0};
    if (d->codecType == ITE_MP3_DECODE)
    {
        rbuf_ite * rBuf;
        rBuf = ite_rbuf_init(8 * 1024);
        id3tagParsing(f, rBuf);
        iteAudioWriteStream(rBuf->b_rptr, ite_rbuf_get_avail_size(
                                              rBuf)); // write data to decoder
        ite_rbuf_free(rBuf);
    }

    while (tmp == 0 && f->run)
    {

        codec_do(f);
        // if(cnt++>200){
        //     printf("timeout error %s %d\n",__FUNCTION__,__LINE__);
        //     if(cnt>210) {printf("error init i2s\n"); init=0 ;break;}
        // }
        usleep(500000);
        printf("wait data input\n");
        iteAudioGetAttrib(ITE_AUDIO_I2S_INIT, &tmp);
        iteAudioUpdateMessageQ();
    }

    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_CHANNEL, &d->nchannels);
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_SAMPLE_RATE, &d->rate);
    iteAudioGetAttrib(ITE_AUDIO_I2S_PTR, &d->codecbuf);
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_BUFFER_LENGTH, &d->codelen);
    printf("%d %d %d\n", d->rate, d->nchannels, tmp);

    d->framesize = 20 * (2 * d->rate * d->nchannels) / 1000; // 20ms data
    EVEN(d->framesize);

    flower->dInfo.sr        = d->rate;
    flower->dInfo.nch       = d->nchannels;
    flower->dInfo.bitsize   = 16;
    flower->dInfo.bytes20ms = d->framesize;
    flower->dInfo.codectype = d->codecType;
    flower->dInfo.init      = true;

    if (s->Fdestinat && s->Fdestinat->filterDes.id == ITE_FILTER_DAWRITE_ID)
    {
        dawrite_reinit_for_diff_rate(flower->dInfo.sr, 16, flower->dInfo.nch);
    }

    d->state = Playing;
    d->eof   = false;
    return 0;
}

static void codecmgr_init (IteFilter * f)
{
    CodecMgrData * d = (CodecMgrData *)ite_new(CodecMgrData, 1);
    if (d != NULL)
    {
        d->state     = Closed;
        d->rate      = 8000;
        d->bitsize   = 16;
        d->nchannels = 1;
        d->RP        = 0;
        d->WP        = 0;
        d->framesize = 320;
        d->codecType = -1;
        d->eof       = false;
        d->bufm      = NULL;
    }
    f->data = d;
}

static void codecmgr_uninit (IteFilter * f)
{
    CodecMgrData * d = (CodecMgrData *)f->data;
    if (d != NULL)
    {
        iteAudioStopQuick();
        if (d->bufm != NULL)
        {
            freemsg_ite(d->bufm);
        }
        free(d);
        iteAudioSetMusicCodecDump(0);
    }
}

static void codecmgr_process (IteFilter * f)
{
    CodecMgrData * d = (CodecMgrData *)f->data;
    // IteFlower *flower=(IteFlower*)f->srcFlow;
    codecmgr_cpu2(f, NULL);

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

static IteMethodDes codecmgr_methods[] = {
    {ITE_FILTER_FILEOPEN, codecmgr_open},
    {                  0,          NULL}
};

// clang-format off
IteFilterDes FilterCodecMgr = {
    ITE_FILTER_CODECMGR_ID,
    codecmgr_init,
    codecmgr_uninit,
    codecmgr_process,
    codecmgr_methods
};
// clang-format on