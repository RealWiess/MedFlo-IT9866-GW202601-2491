#include <stdio.h>
#include <pthread.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
extern STRC_I2S_SPEC spec_ad;
//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct RecState
{
    FILE *          fd;
    pthread_mutex_t lock;
    int             rate;
    int             channels;
    unsigned int    size;
    RecorderState   state;
} RecState;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void write_wav_header (FILE * fd, int rate, int channels, int size)
{
    wave_header_t header;
    memcpy(&header.riff_block.id, "RIFF", 4);
    header.riff_block.data_size = le_uint32(size + 36);
    memcpy(&header.riff_block.type, "WAVE", 4);

    memcpy(&header.format_block.id, "fmt ", 4);
    header.format_block.data_size      = le_uint32(0x10);
    header.format_block.code           = le_uint16(0x1);
    header.format_block.nchannel       = le_uint16(channels);
    header.format_block.samplerate     = le_uint32(rate);
    header.format_block.byte_per_sec   = le_uint32(rate * 2);
    header.format_block.block_align    = le_uint16(2);
    header.format_block.bit_per_sample = le_uint16(16);

    memcpy(&header.data_block.id, "data", 4);
    header.data_block.size = le_uint32(size);
    fseek(fd, 0, SEEK_SET);
    if (!fwrite(&header, sizeof(header), 1, fd))
    {
        printf("Fail to write wav header.");
    }
}

static void rec_close (IteFilter * f, void * arg)
{
    RecState * s = (RecState *)f->data;
    pthread_mutex_lock(&s->lock);
    s->state = RecorderClosed;
    if (s->fd != NULL)
    {
        write_wav_header(s->fd, s->rate, s->channels, s->size);
        fclose(s->fd);
        s->fd = NULL;
    }
    pthread_mutex_unlock(&s->lock);
}

static void rec_open (IteFilter * f, void * arg)
{
    RecState *   s        = (RecState *)f->data;
    const char * filename = (const char *)arg;
    if (s->fd != NULL)
    {
        rec_close(f, NULL);
    }
    pthread_mutex_lock(&s->lock);
    if ((s->fd = fopen(filename, "wb")) == NULL)
    {
        printf("Cannot open %s\n", filename);
        pthread_mutex_unlock(&s->lock);
        return;
    }
    s->state = RecorderPaused;
    pthread_mutex_unlock(&s->lock);
    s->rate     = spec_ad.sample_rate;
    s->channels = spec_ad.channels;
    setvbuf(s->fd, NULL, _IOFBF,
            2 * s->rate * s->channels *
                2); // increase stream buf,improve efficiency(2sec len);
}

static void rec_start (IteFilter * f, void * arg)
{
    RecState * s = (RecState *)f->data;
    pthread_mutex_lock(&s->lock);
    s->state = RecorderRunning;
    pthread_mutex_unlock(&s->lock);
}

/*static void rec_stop(IteFilter *f, void *arg){
    RecState *s=(RecState*)f->data;
    pthread_mutex_lock(&s->lock);
    s->state=RecorderPaused;
    pthread_mutex_unlock(&s->lock);
}*/

static void set_rate (IteFilter * f, void * arg)
{
    RecState * s = (RecState *)f->data;
    s->rate      = *((int *)arg);
}

static void set_channels (IteFilter * f, void * arg)
{
    RecState * s = (RecState *)f->data;
    s->channels  = *((int *)arg);
}

/*static void set_state(IteFilter *f, void *arg)
{
    RecState *s=(RecState*)f->data;
    pthread_mutex_lock(&s->lock);
    s->state=*((int*)arg);
    pthread_mutex_unlock(&s->lock);
}*/

/*static void get_state(IteFilter *f, void *arg)
{
    RecState *s=(RecState*)f->data;
    *((RecorderState*)arg)=s->state;
}*/

//=============================================================================
//                              filter flow
//=============================================================================
static void rec_init (IteFilter * f)
{
    RecState * s = ite_new(RecState, 1);
    if (s != NULL)
    {
        s->fd       = NULL;
        s->rate     = 8000;
        s->channels = 1;
        s->size     = 0;
        s->state    = RecorderClosed;
        pthread_mutex_init(&s->lock, NULL);
    }
    f->data = s;
}

static void rec_uninit (IteFilter * f)
{
    RecState * s = (RecState *)f->data;
    if (s != NULL)
    {
        if (s->fd != NULL)
        {
            rec_close(f, NULL);
        }
        pthread_mutex_destroy(&s->lock);
        free(s);
    }
}

static void rec_process (IteFilter * f)
{
    RecState *  s   = (RecState *)f->data;
    IteQueueblk blk = {0};
    if (s->state != RecorderClosed)
    {
        rec_start(f, NULL);
    }
    while (f->run)
    {
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om = blk.datap;

            if (om && s->state == RecorderRunning)
            {
                if (!fwrite(om->b_rptr, om->size, 1, s->fd))
                {
                    printf("fail to write %i\n", om->size);
                }
                s->size += om->size;
            }

            if (om)
            {
                freemsg_ite(om);
                om        = NULL;
                blk.datap = NULL;
            }
        }
        usleep(10000);
    }
}

static IteMethodDes rec_methods[] = {
    //{ITE_FILTER_REC, rec_method},
    {     ITE_FILTER_FILEOPEN,     rec_open},
    {    ITE_FILTER_FILECLOSE,    rec_close},
    {      ITE_FILTER_SETRATE,     set_rate},
    {ITE_FILTER_SET_NCHANNELS, set_channels},
    //{ITE_FILTER_GET_STATE, get_state},
    //{ITE_FILTER_SET_STATE, set_state},
    {                       0,         NULL}
};

// clang-format off
IteFilterDes FilterFileRec = {
    ITE_FILTER_FILEREC_ID,
    rec_init,
    rec_uninit,
    rec_process,
    rec_methods
};
// clang-format on
