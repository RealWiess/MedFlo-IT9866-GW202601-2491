#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"

typedef struct _ChAdaptData
{
    int inchannels;
    int outchannels;
} ChAdaptData;

static void chadapt_init (IteFilter * f)
{
    ChAdaptData * s = (ChAdaptData *)ite_new(ChAdaptData, 1);
    if (s != NULL)
    {
        s->inchannels  = 1;
        s->outchannels = 1;
    }
    f->data = s;
}

static void chadapt_uninit (IteFilter * f)
{
    ChAdaptData * s = (ChAdaptData *)f->data;
    if (s != NULL)
    {
        free(s);
    }
}

static void chadapt_process (IteFilter * f)
{
    ChAdaptData * s   = (ChAdaptData *)f->data;
    IteQueueblk   blk = {0};
    int           length;
#if CFG_RESERVE_FILTER
    IteFlower * flower = (IteFlower *)f->srcFlow;
    while (f->run && !flower->dInfo.init)
    {
        usleep(100000);
    }
    s->inchannels = flower->dInfo.nch;
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
            mblk_ite * im = blk.datap;
            if (s->inchannels == s->outchannels)
            {
#if CFG_CHANNEL_SELECT
                int16_t * point;
                for (point = (int16_t *)im->b_rptr;
                     point < (int16_t *)im->b_wptr; ++point)
                {
                    if (flower->mixInfo.ch_select == ITE_AUDIO_RIGHT_CHANNEL)
                    {
                        *point = 0;
                        ++point;
                    }
                    else if (flower->mixInfo.ch_select ==
                             ITE_AUDIO_LEFT_CHANNEL)
                    {
                        ++point;
                        *point = 0;
                    }
                    else
                    {
                        // do nothing
                    }
                }
#endif
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
            else if (s->inchannels == 2)
            {
                length        = im->size / 2;
                mblk_ite * om = allocb_ite(length);

                for (; im->b_rptr < im->b_wptr;
                     im->b_rptr += 4, om->b_wptr += 2)
                {
                    *(int16_t *)om->b_wptr = *(int16_t *)im->b_rptr;
                }
                blk.datap = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
                freemsg_ite(im);
            }
            else if (s->outchannels == 2)
            {
                length        = im->size * 2;
                mblk_ite * om = allocb_ite(length);

                for (; im->b_rptr < im->b_wptr;
                     im->b_rptr += 2, om->b_wptr += 4)
                {
#if CFG_CHANNEL_SELECT
                    if (flower->mixInfo.ch_select == ITE_AUDIO_RIGHT_CHANNEL)
                    {
                        ((int16_t *)om->b_wptr)[0] = 0;
                        ((int16_t *)om->b_wptr)[1] = *(int16_t *)im->b_rptr;
                    }
                    else if (flower->mixInfo.ch_select ==
                             ITE_AUDIO_LEFT_CHANNEL)
                    {
                        ((int16_t *)om->b_wptr)[0] = *(int16_t *)im->b_rptr;
                        ((int16_t *)om->b_wptr)[1] = 0;
                    }
                    else
#endif
                    {
                        ((int16_t *)om->b_wptr)[0] = *(int16_t *)im->b_rptr;
                        ((int16_t *)om->b_wptr)[1] = *(int16_t *)im->b_rptr;
                    }
                }
                blk.datap = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
                freemsg_ite(im);
            }
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void chadapt_set_in_nchannels (IteFilter * f, void * arg)
{
    ChAdaptData * s = (ChAdaptData *)f->data;
    s->inchannels   = *(int *)arg;
}

static void chadapt_set_out_nchannels (IteFilter * f, void * arg)
{
    ChAdaptData * s = (ChAdaptData *)f->data;
    s->outchannels  = *(int *)arg;
}

static void chadapt_get_in_channels (IteFilter * f, void * arg)
{
    ChAdaptData * s = (ChAdaptData *)f->data;
    *(int *)arg     = s->inchannels;
}

static void chadapt_get_out_channels (IteFilter * f, void * arg)
{
    ChAdaptData * s = (ChAdaptData *)f->data;
    *(int *)arg     = s->outchannels;
}

static void chadapt_set_flush (IteFilter * f, void * arg)
{
    // ChAdaptData *s=(ChAdaptData*)f->data;
    ite_mblk_queue_flush(f->input[0].Qhandle);
}

static IteMethodDes chadapt_methods[] = {
    {ITE_FILTER_SET_NCHANNELS, chadapt_set_out_nchannels},
    {ITE_FILTER_GET_NCHANNELS,  chadapt_get_out_channels},
    {    ITE_FILTER_SET_VALUE,  chadapt_set_in_nchannels},
    {    ITE_FILTER_GET_VALUE,   chadapt_get_in_channels},
    {   ITE_FILTER_SET_BYPASS,         chadapt_set_flush},
    {                       0,                      NULL}
};

// clang-format off
IteFilterDes FilterChadapt = {
    ITE_FILTER_CHADAPT_ID,
    chadapt_init,
    chadapt_uninit,
    chadapt_process,
    chadapt_methods
};
