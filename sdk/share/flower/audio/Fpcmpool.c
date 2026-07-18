#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

#define CODEC_RW_GAP                                                           \
    ((d->WP >= d->RP) ? (d->WP - d->RP) : ((d->codelen - d->RP) + d->WP))
/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr (0xB0200080 + 0x18)

extern int gbytes;

typedef struct _PcmPoolData
{
    PlayerState  state;
    int          rate;
    int          bitsize;
    int          nchannels;
    int          codecType;
    int          framesize;

    uint8_t *    codecbuf;
    int          codelen;
    unsigned int RP;
    unsigned int WP;
} PcmPoolData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int pcmpool_close (IteFilter * f, void * arg)
{
    PcmPoolData * d = (PcmPoolData *)f->data;
    d->state        = Closed;

    if (i2s_get_DA_running())
    {
        iteAudioStopQuick();
        // i2s_deinit_DAC();
    }
    return 0;
}

static mblk_ite * codec_data (PcmPoolData * d, int length)
{
    mblk_ite * rm       = NULL;
    uint32_t   rp       = d->RP;
    uint32_t   wp       = d->WP;
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

static void pcmpool_open (IteFilter * f, void * arg)
{
    PcmPoolData * d      = (PcmPoolData *)f->data;
    IteFlower *   flower = (IteFlower *)f->srcFlow;
    int           tmp = 0, cnt = 0;

    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr, d->RP);
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT, &tmp);

    while (tmp == 0)
    { // wait codec parsing i2s info;

        if (cnt++ > 400)
        {
            printf("timeout error %s %d\n", __FUNCTION__, __LINE__);
            if (cnt > 410)
            {
                printf("error init i2s\n");
                break;
            }
        }
        usleep(10000);
        iteAudioGetAttrib(ITE_AUDIO_I2S_INIT, &tmp);
        iteAudioUpdateMessageQ();
    }
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_CHANNEL, &d->nchannels);
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_SAMPLE_RATE, &d->rate);
    iteAudioGetAttrib(ITE_AUDIO_I2S_PTR, &d->codecbuf);
    iteAudioGetAttrib(ITE_AUDIO_CODEC_SET_BUFFER_LENGTH, &d->codelen);
    d->framesize = 20 * d->rate * d->nchannels * 2 / 1000;
    EVEN(d->framesize);
    printf("mgrpool do\n");
    flower->dInfo.sr        = d->rate;
    flower->dInfo.nch       = d->nchannels;
    flower->dInfo.bytes20ms = d->framesize;
    flower->dInfo.codectype = d->codecType;
    flower->dInfo.bitsize   = 16;
    flower->dInfo.init      = true;

    d->state                = Playing;
}

static void pcmpool_init (IteFilter * f)
{
    PcmPoolData * d = (PcmPoolData *)ite_new(PcmPoolData, 1);
    if (d != NULL)
    {
        d->state     = Closed;
        d->rate      = 8000;
        d->bitsize   = 16;
        d->nchannels = 1;
        d->framesize = 320;
        d->RP        = 0;
        d->WP        = 0;
        f->data      = d;
        d->codecType = -1;
        iteAudioSetMusicCodecDump(1); // set dump flag
    }
    else
    {
        f->data = NULL;
    }
}

static void pcmpool_uninit (IteFilter * f)
{
    PcmPoolData * d = (PcmPoolData *)f->data;
    if (d != NULL)
    {
        pcmpool_close(f, NULL);
        iteAudioSetMusicCodecDump(0);
        free(d);
    }
}

static void pcmpool_process (IteFilter * f)
{
    PcmPoolData * d      = (PcmPoolData *)f->data;
    IteFlower *   flower = (IteFlower *)f->srcFlow;
    IteQueueblk   blk    = {0};

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
        if (d->state == Playing)
        {
            iteAudioUpdateMessageQ();
            d->WP = iteAudioCodecGetPcmIdx();
            if (CODEC_RW_GAP >= d->framesize)
            {
                mblk_ite * om = codec_data(d, d->framesize);
                blk.datap     = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
        }

        if (d->state == Eof && flowQCount(flower) == 0)
        {
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
        }

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static IteMethodDes pcmpool_methods[] = {
    {ITE_FILTER_FILEOPEN, pcmpool_open},
    {                  0,         NULL}
};

// clang-format off
IteFilterDes FilterPcmpool = {
    ITE_FILTER_PCMPOOL_ID,
    pcmpool_init,
    pcmpool_uninit,
    pcmpool_process,
    pcmpool_methods
};
// clang-format on
