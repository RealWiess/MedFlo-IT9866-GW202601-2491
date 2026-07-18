#include <stdio.h>
#include <stdlib.h>
#include "flower/flower.h"
#include "include/audioqueue.h"

typedef struct _ChseparateData
{
    void * datap;
} ChSeparateData;

static void separate_init (IteFilter * f)
{
    ChSeparateData * s = (ChSeparateData *)ite_new(ChSeparateData, 1);
    f->data            = s;
}

static void separate_uninit (IteFilter * f)
{
    ChSeparateData * s = (ChSeparateData *)f->data;
    if (s != NULL)
    {
        free(s);
    }
}

static void separate_process (IteFilter * f)
{
    ChSeparateData * s    = (ChSeparateData *)f->data;
    IteQueueblk      blkR = {0};
    IteQueueblk      blkL = {0};
    IteQueueblk      blk  = {0};
    int              length;
#if CFG_RESERVE_FILTER
    IteFlower * flower = (IteFlower *)f->srcFlow;
    while (f->run && !flower->dInfo.init)
    {
        usleep(100000);
    }
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
            mblk_ite * oL, *oR;
            int        nbyte = im->size;
            oL               = allocb_ite(nbyte / 2);
            oR               = allocb_ite(nbyte / 2);

            for (; im->b_rptr < im->b_wptr;
                 oL->b_wptr += 2, oR->b_wptr += 2, im->b_rptr += 4)
            {
                *((int16_t *)(oL->b_wptr)) = (int)*(int16_t *)(im->b_rptr);
                *((int16_t *)(oR->b_wptr)) = (int)*(int16_t *)(im->b_rptr + 2);
            }

            blkL.datap = oL;
            blkR.datap = oR;

            ite_queue_put(f->output[0].Qhandle, &blkL);
            ite_queue_put(f->output[1].Qhandle, &blkR);

            if (im)
            {
                freemsg_ite(im);
            }
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void separate_set_flush (IteFilter * f, void * arg)
{
    ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->output[0].Qhandle);
    ite_mblk_queue_flush(f->output[1].Qhandle);
}

static IteMethodDes separate_methods[] = {
    {ITE_FILTER_SET_BYPASS, separate_set_flush},
    {                    0,               NULL}
};

// clang-format on
IteFilterDes FilterChseparate = {
    ITE_FILTER_CHSEPARATE_ID,
    separate_init,
    separate_uninit,
    separate_process,
    separate_methods
};
// clang-format off
