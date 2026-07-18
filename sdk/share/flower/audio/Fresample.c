#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include <speex/speex_resampler.h>
#include <speex/speex.h>
#include <stdlib.h>

//=============================================================================
//                              Constant Definition
//=============================================================================
// #define MEASUREMENTS
//=============================================================================
//                              Private Function Declaration
//=============================================================================
typedef struct _ResampleData
{
    uint32_t              input_rate;
    uint32_t              output_rate;
    int                   nchannels;
    SpeexResamplerState * handle;
} ResampleData;

static void resample_init (IteFilter * f)
{
    ResampleData * s = (ResampleData *)ite_new(ResampleData, 1);
    if (s != NULL)
    {
        s->input_rate  = 16000;
        s->output_rate = 48000;
        s->handle      = NULL;
        s->nchannels   = 1;
    }
    f->data = s;
}

static void resample_uninit (IteFilter * f)
{
    ResampleData * s = (ResampleData *)f->data;
    if (s != NULL)
    {
        if (s->handle != NULL)
        {
            speex_resampler_destroy(s->handle);
        }
        free(s);
    }
}

static void resample_process (IteFilter * f)
{
    ResampleData * s   = (ResampleData *)f->data;
    IteQueueblk    blk = {0};
#ifdef MEASUREMENTS
    uint32_t start_cnt = 0;
    uint32_t count     = 1;
    uint64_t diff      = 0;
#endif
#if CFG_RESERVE_FILTER
    IteFlower * flower = (IteFlower *)f->srcFlow;
    while (f->run && !flower->dInfo.init)
    {
        usleep(100000);
    }
    s->input_rate = flower->dInfo.sr;
#endif

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
            if (s->output_rate == s->input_rate)
            {
                ite_queue_put(f->output[0].Qhandle, &blk);
                continue;
            }
            if (s->handle != NULL)
            {
                unsigned int inrate = 0, outrate = 0;
                speex_resampler_get_rate(s->handle, &inrate, &outrate);
                if (inrate != s->input_rate || outrate != s->output_rate)
                {
                    speex_resampler_destroy(s->handle);
                    s->handle = 0;
                }
            }
            if (s->handle == NULL)
            {
                int err   = 0;
                s->handle = speex_resampler_init(
                    s->nchannels, s->input_rate, s->output_rate,
                    SPEEX_RESAMPLER_QUALITY_MIN, &err);
            }

            mblk_ite *   m     = blk.datap;
            unsigned int inlen = (m->b_wptr - m->b_rptr) / (2 * s->nchannels);
            unsigned int outlen =
                ((inlen * s->output_rate) / s->input_rate) + 1;
            unsigned int inlen_orig = inlen;
            mblk_ite *   om         = allocb_ite(outlen * 2 * s->nchannels);
#ifdef MEASUREMENTS
            start_cnt = itpGetTickCount();
#endif
            if (s->nchannels == 1)
            {
                speex_resampler_process_int(s->handle, 0, (int16_t *)m->b_rptr,
                                            &inlen, (int16_t *)om->b_wptr,
                                            &outlen);
            }
            else
            {
                speex_resampler_process_interleaved_int(
                    s->handle, (int16_t *)m->b_rptr, &inlen,
                    (int16_t *)om->b_wptr, &outlen);
            }
            if (inlen_orig != inlen)
            {
                printf("Bug in resampler ! only %u samples consumed instead of "
                       "%u, out=%u\n",
                       inlen, inlen_orig, outlen);
            }
            om->size = (outlen * 2 * s->nchannels);
            om->b_wptr += om->size;
#ifdef MEASUREMENTS
            count++;
            diff += itpGetTickDuration(start_cnt);
            if (count % 100 == 0)
            {
                printf("resample 100 time %d %d\n", (int)diff,
                       om->size * 1000 / (s->output_rate * 2 * s->nchannels));
                diff = 0;
            }
#endif
            // printf(" m size(%d) b_wptr(%d) b_rptr(%d) len(%d) inlen(%d)\n",
            // m->size, m->b_wptr, m->b_rptr, m->b_wptr - m->b_rptr, inlen);
            // printf("om size(%d) b_wptr(%d) b_rptr(%d) len(%d) outlen(%d)\n",
            // om->size, om->b_wptr, om->b_rptr, om->b_wptr - om->b_rptr,
            // outlen);
            freemsg_ite(m);
            blk.datap = om;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void resample_input_rate (IteFilter * f, void * arg)
{
    ResampleData * s = (ResampleData *)f->data;
    s->input_rate    = ((int *)arg)[0];
#if CFG_RESERVE_FILTER
    IteFlower * flower = (IteFlower *)f->srcFlow;
    flower->dInfo.init = true;
    flower->dInfo.sr   = s->input_rate;
#endif
}

static void resample_out_rate (IteFilter * f, void * arg)
{
    ResampleData * s = (ResampleData *)f->data;
    s->output_rate   = ((int *)arg)[0];
}

static void resample_set_nchannels (IteFilter * f, void * arg)
{
    ResampleData * s     = (ResampleData *)f->data;
    int            chans = *(int *)arg;

    if (s->nchannels != chans && s->handle != NULL)
    {
        speex_resampler_destroy(s->handle);
        s->handle = NULL;
    }
    s->nchannels = *(int *)arg;
}

static void resample_set_flush (IteFilter * f, void * arg)
{
    // ResampleData *s=(ResampleData*)f->data;
    ite_mblk_queue_flush(f->input[0].Qhandle);
}

static IteMethodDes resample_methods[] = {
    {   ITE_FILTER_INPUT_RATE,    resample_input_rate},
    {  ITE_FILTER_OUTPUT_RATE,      resample_out_rate},
    {ITE_FILTER_SET_NCHANNELS, resample_set_nchannels},
    {   ITE_FILTER_SET_BYPASS,     resample_set_flush},
    {                       0,                   NULL}
};

// clang-format off
IteFilterDes FilterResample = {
    ITE_FILTER_RESAMPLE_ID,
    resample_init,
    resample_uninit,
    resample_process,
    resample_methods
};
// clang-format on
