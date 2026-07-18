/*
read file(mp3 aac wma(0) ,wav(x)),and decode to pcm data
*/
#include <stdio.h>
#include "flower/flower.h"
#include "audio/include/audioqueue.h"
#include "audio/include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr (0xB0200080 + 0x18)
#define BUFSIZE          (d->readsize) // 512
#define CODEC_RW_GAP                                                           \
    ((d->WP >= d->RP) ? (d->WP - d->RP) : ((d->codelen - d->RP) + d->WP))

typedef struct _MgrGrabData
{
    void *          fd;
    PlayerState     state;
    int             rate;
    int             bitsize;
    int             nchannels;
    int             loop_after;
    int             codecType;

    uint8_t *       codecbuf;
    int             codelen;
    unsigned int    RP;
    unsigned int    WP;
    int             offset;
    int             framesize;
    pthread_mutex_t mutex;
    bool            fileRead;
    int             readsize;
} MgrGrabData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

/*static int mgrgrab_stop(IteFilter *f, void *arg){
    MgrGrabData *d=(MgrGrabData*)f->data;

    return 0;
}*/

static int mgrgrab_close (IteFilter * f, void * arg)
{
    MgrGrabData * d = (MgrGrabData *)f->data;
    pthread_mutex_lock(&d->mutex);
    d->state = Closed;
    if (d->fd)
    {
        fclose(d->fd);
        // castor3snd_reinit_for_video_memo_play();
    }
    d->fd = NULL;
    d->WP = 0;
    d->RP = 0;
    iteAudioStopEngine(); // reset music codec
    pthread_mutex_unlock(&d->mutex);
    return 0;
}

static mblk_ite * codec_data (MgrGrabData * d, int length)
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

static void mgrgrab_do (IteFilter * f)
{
    MgrGrabData * d = (MgrGrabData *)f->data;
    iteAudioUpdateMessageQ();
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

    /*put data(undecoded) to codec for decoder*/
    if (d->fileRead)
    {
        unsigned long availSize = 0;
        iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER, &availSize);
        if (availSize > BUFSIZE)
        {
            int     rbytes;
            uint8_t buf[BUFSIZE];
            rbytes = fread(buf, 1, BUFSIZE, d->fd);
            if (rbytes == 0)
            {
                d->fileRead = false;
                if (IteAudioQueueNum(f, 0) < 4)
                {
                    usleep(30000); // workaround for short file(<0.2sec);
                }
                printf("finish file read\n");
                return;
            }
            iteAudioWriteStream(buf, rbytes); // write data for decoder
        }
    }
    else
    {
        if (IteAudioQueueIsEmpty(f, 0))
        {
            if (d->loop_after == Normal)
            {
                d->state = Eof;
            }
            else
            {
                // size_t ret;
                printf("play repeat\n");
                (void)fseek(d->fd, d->offset, SEEK_SET);
                d->fileRead = true;
            }
        }
    }
}

static void mgrgrab_readsize_setting (IteFilter * f)
{
    MgrGrabData * d = (MgrGrabData *)f->data;
    switch (d->codecType)
    {
        case ITE_MP3_DECODE:
            d->readsize = 512;
            break;
        case ITE_FLAC_DECODE:
            d->readsize = 8 * 1024;
            break;
        case ITE_AAC_DECODE:
            d->readsize = 2 * 1024;
            break;
        case ITE_OGG_DECODE:
            d->readsize = 4 * 1024;
            break;
    }
}

static void mgrgrab_open (IteFilter * f, void * arg)
{
    MgrGrabData * d   = (MgrGrabData *)f->data;
    int           tmp = 0, cnt = 0;
    char *        file   = (char *)arg;
    IteFlower *   flower = (IteFlower *)f->srcFlow;
    if (d->fd)
    {
        mgrgrab_close(f, NULL);
        ite_mblk_queue_flush(f->output[0].Qhandle); // flush previous data
    }
    d->fd = fopen(file, "rb");
    if (d->fd == NULL)
    {
        printf("%s openfile error\n", file);
        d->state = Dummy;
        return;
    }
    setvbuf(d->fd, NULL, _IOFBF,
            64 * 1024);               // increase stream buf,improve efficiency
    d->codecType = audiomgrCodecType(file);
    iteAudioOpenEngine(d->codecType); // load codec engine
    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr, d->RP);
    iteAudioSetMusicCodecDump(1); // set dump flag
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT, &tmp);

    if (d->codecType == ITE_MP3_DECODE)
    {
        parsingMp3IsID3v2((FILE *)d->fd, NULL, 0, &d->offset);
        audioGetTotalTime(file, &flower->dInfo);
    }
    mgrgrab_readsize_setting(f);

    while (tmp == 0)
    { // wait codec parsing i2s info;

        mgrgrab_do(f);

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
    d->framesize            = 20 * d->rate * d->nchannels * 2 / 1000;

    flower->dInfo.sr        = d->rate;
    flower->dInfo.nch       = d->nchannels;
    flower->dInfo.bytes20ms = d->framesize;
    flower->dInfo.codectype = d->codecType;
    flower->dInfo.bitsize   = 16;
    flower->dInfo.init      = true;
    EVEN(d->framesize);
    // printf("totale time =%d
    // type=%d\n",flower->dInfo.duration,flower->dInfo.codectype);
    d->state = Playing;

    // dawrite_reinit_for_diff_rate(d->rate,16,d->nchannels);
}

/*static int mgrgrab_start(IteFilter *f, void *arg){
    MgrGrabData *d=(MgrGrabData*)f->data;
    if (d->state==Paused)
        d->state = Playing;
    else{
        d->state = Dummy;
        printf("MSdummyPlaying scilent\n");
    }
    return 0;
}*/

static void mgrgrab_pause (IteFilter * f, void * arg)
{
    MgrGrabData * d     = (MgrGrabData *)f->data;
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
}

static void mgrgrab_loop (IteFilter * f, void * arg)
{
    MgrGrabData * d = (MgrGrabData *)f->data;
    d->loop_after   = *((int *)arg);
}

static void mgrgrab_get_rate (IteFilter * f, void * arg)
{
    MgrGrabData * d = (MgrGrabData *)f->data;
    *(int *)arg     = d->rate;
}

static void mgrgrab_get_channels (IteFilter * f, void * arg)
{
    MgrGrabData * d = (MgrGrabData *)f->data;
    *(int *)arg     = d->nchannels;
}

//=============================================================================
//                              filter flow
//=============================================================================

static void mgrgrab_init (IteFilter * f)
{
    MgrGrabData * d = (MgrGrabData *)ite_new(MgrGrabData, 1);
    if (d != NULL)
    {
        d->fd         = NULL;
        d->state      = Closed;
        d->rate       = 8000;
        d->bitsize    = 16;
        d->nchannels  = 1;
        d->loop_after = -1; /*by default, don't loop*/
        d->RP         = 0;
        d->WP         = 0;
        d->codecType  = -1;
        d->offset     = 0;
        d->framesize  = 320; // 20*(2*d->rate*d->nchannels)/1000;//20ms data
        d->readsize   = 512;
        pthread_mutex_init(&d->mutex, NULL);
        d->fileRead = true;
    }
    f->data = d;
    // f->pthread_stacksize = 8*1024; //ogg flac
}

static void mgrgrab_uninit (IteFilter * f)
{
    MgrGrabData * d = (MgrGrabData *)f->data;
    if (d != NULL)
    {
        mgrgrab_close(f, NULL);
        iteAudioSetMusicCodecDump(0);
        pthread_mutex_destroy(&d->mutex);
        if (i2s_get_DA_running())
        {
            iteAudioStopQuick();
            // i2s_deinit_DAC();
        }
        free(d);
    }
}

static void mgrgrab_process (IteFilter * f)
{
    MgrGrabData * d      = (MgrGrabData *)f->data;
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
        {
            mgrgrab_do(f);
        }
        pthread_mutex_unlock(&d->mutex);

        if (d->state == Dummy)
        {
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
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

static IteMethodDes mgrgrab_methods[] = {
    {     ITE_FILTER_FILEOPEN,         mgrgrab_open},
    {     ITE_FILTER_LOOPPLAY,         mgrgrab_loop},
    {        ITE_FILTER_PAUSE,        mgrgrab_pause},
    {      ITE_FILTER_GETRATE,     mgrgrab_get_rate},
    {ITE_FILTER_GET_NCHANNELS, mgrgrab_get_channels},
    //{ITE_FILTER_SET_FILEPATH ,mgrgrab_set_filepath},
    {                       0,                 NULL}
};

// clang-format off
IteFilterDes FilterMgrGrab = {
    ITE_FILTER_MGRGRAB_ID,
    mgrgrab_init,
    mgrgrab_uninit,
    mgrgrab_process,
    mgrgrab_methods
};
// clang-format on
