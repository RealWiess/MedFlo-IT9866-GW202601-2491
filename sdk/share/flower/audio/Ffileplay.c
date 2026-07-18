#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "audio/include/fileheader.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct _PcmGrabData
{
    void *          fd;
    PlayerState     state;
    int             rate;
    int             bitsize;
    int             nchannels;
    int             codectype;
    int             hsize;
    int             loop_after;
    int             framesize;
    int             pcmsize;
    int             rsize;
    pthread_mutex_t mutex;
    double          wavtime;
} PcmGrabData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int parse_wav_header (PcmGrabData * d)
{
    char          header_chunk1[sizeof(riff_t)];
    char          header_chunk2[sizeof(format_t)];
    char          header_chunk3[sizeof(data_t)];
    int           count;
    // size_t ret;

    riff_t *      riff_type   = (riff_t *)header_chunk1;
    format_t *    format_type = (format_t *)header_chunk2;
    data_t *      data_type   = (data_t *)header_chunk3;

    unsigned long len         = 0;

    len = fread(header_chunk1, 1, sizeof(header_chunk1), d->fd);
    if (len != sizeof(header_chunk1))
    {
        goto not_wav_file;
    }

    if (0 != strncmp(riff_type->id, "RIFF", 4) ||
        0 != strncmp(riff_type->type, "WAVE", 4))
    {
        goto not_wav_file;
    }

    len = fread(header_chunk2, 1, sizeof(header_chunk2), d->fd);
    if (len != sizeof(header_chunk2))
    {
        goto not_wav_file;
    }

    d->rate      = format_type->samplerate;
    d->nchannels = format_type->nchannel;
    d->codectype = format_type->code;
    d->bitsize   = format_type->bit_per_sample;

    if (format_type->data_size - 0x10 > 0)
    {
        (void)fseek(d->fd, (format_type->data_size - 0x10), SEEK_CUR);
    }

    d->hsize = sizeof(wave_header_t) - 0x10 + format_type->data_size;

    len      = fread(header_chunk3, 1, sizeof(header_chunk3), d->fd);
    if (len != sizeof(header_chunk3))
    {
        goto not_wav_file;
    }
    count = 0;
    while (strncmp(data_type->id, "data", 4) != 0 && count < 30)
    {
        (void)fseek(d->fd, data_type->size, SEEK_CUR);
        count++;
        d->hsize = d->hsize + len + data_type->size;

        len      = fread(header_chunk3, 1, sizeof(header_chunk3), d->fd);
        if (len != sizeof(header_chunk3))
        {
            goto not_wav_file;
        }
    }
    d->wavtime = (double)data_type->size / (double)format_type->byte_per_sec;
    d->pcmsize = data_type->size;
    d->rsize   = 0;
    return 0;

not_wav_file:
    (void)fseek(d->fd, 0, SEEK_SET);
    d->hsize = 0;
    return -1;
}

static void pcmgrab_close (IteFilter * f, void * arg)
{
    PcmGrabData * d = (PcmGrabData *)f->data;
    pthread_mutex_lock(&d->mutex);
    if (d->fd != NULL)
    {
        fclose(d->fd);
    }
    d->fd    = NULL;
    d->state = Closed;
    pthread_mutex_unlock(&d->mutex);
}

static void pcmgrab_open (IteFilter * f, void * arg)
{
    PcmGrabData * d      = (PcmGrabData *)f->data;
    IteFlower *   flower = (IteFlower *)f->srcFlow;
    FILE *        fd;
    char *        file = (char *)arg;

    if (d->fd != NULL)
    {
        pcmgrab_close(f, NULL);
        ite_mblk_queue_flush(f->output[0].Qhandle); // flush previous data
    }
    if ((fd = fopen(file, "rb")) == NULL)
    {
        // ms_warning("Failed to open %s",file);
        return;
    }
    d->fd = fd;
    if (strstr(file, ".wav") || strstr(file, ".WAV"))
    {
        if (parse_wav_header(d) != 0)
        {
            printf(
                "File %s has .wav extension but wav header could be found.\n",
                file);
        }
    }
    else if (strstr(file, ".svm"))
    {
        d->codectype = 0; // uknow codec not .wav file
    }
    else
    {
        printf("not .wav ,read raw data file\n");
        d->state = Playing;
        return;
    }

    flower->dInfo.sr        = d->rate;
    flower->dInfo.nch       = d->nchannels;
    flower->dInfo.bitsize   = d->bitsize;
    flower->dInfo.bytes20ms = 20 * d->rate * d->nchannels * 2 / 1000;
    flower->dInfo.duration  = d->wavtime;
    EVEN(flower->dInfo.bytes20ms);
    d->framesize       = flower->dInfo.bytes20ms; // 20ms data
    flower->dInfo.init = true;
    printf("totale time =%d\n", flower->dInfo.duration);
    d->state = Playing;
    setvbuf(fd, NULL, _IOFBF,
            flower->dInfo.bytes20ms *
                100); // increase stream buf,improve efficiency(2sec len);
    // dawrite_reinit_for_diff_rate(d->rate,d->bitsize,d->nchannels);//chane to
    // diff sampling rate
}

static void pcmgrab_seek_pos (IteFilter * f, void * arg)
{
    PcmGrabData * d         = (PcmGrabData *)f->data;
    // int ret;
    unsigned int  shift_sec = *((int *)arg);
    unsigned int  offset    = d->hsize + shift_sec * 2 * d->rate * d->nchannels;

    pthread_mutex_lock(&d->mutex);
    ite_mblk_queue_flush(f->output[0].Qhandle);
    (void)fseek(d->fd, offset, SEEK_SET);
    pthread_mutex_unlock(&d->mutex);
    printf("pcmgrab_seek_pos to %d sec\n", shift_sec);
}

static void pcmgrab_shift_pos (IteFilter * f, void * arg)
{
    PcmGrabData * d         = (PcmGrabData *)f->data;
    int           shift_sec = *((int *)arg);
    int           offset    = shift_sec * 2 * d->rate * d->nchannels;
    int           cur_pos;
    // size_t ret;

    // if(offset<=d->hsize) offset=d->hsize;
    pthread_mutex_lock(&d->mutex);
    ite_mblk_queue_flush(f->output[0].Qhandle);
    cur_pos = fseek(d->fd, 0, SEEK_CUR);
    if (cur_pos + offset < 0)
    {
        (void)fseek(d->fd, d->hsize, SEEK_SET);
    }
    else
    {
        (void)fseek(d->fd, offset, SEEK_CUR);
    }
    pthread_mutex_unlock(&d->mutex);
    printf("pcmgrab_shift_pos %d sec\n", shift_sec);
}

static void pcmgrab_get_pos (IteFilter * f, void * arg)
{
    PcmGrabData * d = (PcmGrabData *)f->data;
    int           cur_pos;

    pthread_mutex_lock(&d->mutex);
    cur_pos = fseek(d->fd, 0, SEEK_CUR);
    cur_pos /= (2 * d->rate * d->nchannels / 1000);
    pthread_mutex_unlock(&d->mutex);
    *(int *)arg = cur_pos;

    printf("pcmgrab_get_pos %d ms\n", cur_pos);
}

static void pcmgrab_loop (IteFilter * f, void * arg)
{
    PcmGrabData * d = (PcmGrabData *)f->data;
    d->loop_after   = *((int *)arg);
}

static void pcmgrab_get_rate (IteFilter * f, void * arg)
{
    PcmGrabData * d = (PcmGrabData *)f->data;
    *(int *)arg     = d->rate;
}

static void pcmgrab_get_channels (IteFilter * f, void * arg)
{
    PcmGrabData * d = (PcmGrabData *)f->data;
    *(int *)arg     = d->nchannels;
}

//=============================================================================
//                              filter flow
//=============================================================================
static void pcmgrab_init (IteFilter * f)
{
    PcmGrabData * d = (PcmGrabData *)ite_new(PcmGrabData, 1);

    if (d != NULL)
    {
        d->fd         = NULL;
        d->state      = Closed;
        d->rate       = 8000;
        d->bitsize    = 16;
        d->nchannels  = 1;
        d->codectype  = -1;
        d->hsize      = 0;
        d->loop_after = -1;  /*by default, don't loop*/
        d->framesize  = 320; // 20*(2*d->rate*d->nchannels)/1000;//20ms data
        pthread_mutex_init(&d->mutex, NULL);
    }
    f->data = d;
}

static void pcmgrab_uninit (IteFilter * f)
{
    PcmGrabData * d = (PcmGrabData *)f->data;

    if (d != NULL)
    {
        if (d->fd != NULL)
        {
            pcmgrab_close(f, NULL);
        }
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void pcmgrab_do (IteFilter * f)
{
    PcmGrabData * d = (PcmGrabData *)f->data;
    int           rbytes;
    mblk_ite *    om = allocb_ite(d->framesize);
    rbytes           = fread(om->b_wptr, 1, d->framesize, d->fd);

    if (rbytes)
    {
        IteQueueblk blk = {0};
        om->b_wptr += rbytes;
        d->rsize += rbytes;
        if (rbytes < d->framesize)
        { // eof file
            int tail = d->framesize - rbytes;
            memset(om->b_wptr, 0, tail);
            om->b_wptr += tail;
        }
        if (d->rsize >= d->pcmsize)
        { // somefiles dummy bytes tag in file tail,discard that.
            int tail = d->rsize - d->pcmsize;
            // size_t ret;
            memset(om->b_rptr + (rbytes - tail), 0, tail);
            (void)fseek(d->fd, 0, SEEK_END);
        }
        blk.datap = om;
        ite_queue_put(f->output[0].Qhandle, &blk);
    }
    else
    {
        if (d->loop_after == Normal)
        {
            d->state = Eof;
        }
        else
        { // Repeat,Scilent,
            // size_t ret;
            (void)fseek(d->fd, d->hsize, SEEK_SET);
            d->rsize = 0;
        }
        freemsg_ite(om);
    }
}

static void pcmgrab_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    PcmGrabData * d      = (PcmGrabData *)f->data;
    IteFlower *   flower = (IteFlower *)f->srcFlow;

    while (f->run && !flower->dInfo.init && d->state == Closed)
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
        // printf("%d\n",ite_queue_get_size(f->output[0].Qhandle));
        pthread_mutex_lock(&d->mutex);
        if (d->state == Playing)
        {
            pcmgrab_do(f);
        }

        if (d->state == Eof && IteAudioQueueIsEmpty(f, 0))
        { // add null time(data) prevent abnormal sound
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

static IteMethodDes pcmgrab_methods[] = {
    {     ITE_FILTER_FILEOPEN,         pcmgrab_open},
    {     ITE_FILTER_SEEK_POS,     pcmgrab_seek_pos},
    {    ITE_FILTER_SHIFT_POS,    pcmgrab_shift_pos},
    {    ITE_FILTER_GET_PARAM,      pcmgrab_get_pos},
    {     ITE_FILTER_LOOPPLAY,         pcmgrab_loop},
    {      ITE_FILTER_GETRATE,     pcmgrab_get_rate},
    {ITE_FILTER_GET_NCHANNELS, pcmgrab_get_channels},
    {                       0,                 NULL}
};

// clang-format off
IteFilterDes FilterPcmGrab = {
    ITE_FILTER_PCMGRAB_ID,
    pcmgrab_init,
    pcmgrab_uninit,
    pcmgrab_process,
    pcmgrab_methods
};
// clang-format on
