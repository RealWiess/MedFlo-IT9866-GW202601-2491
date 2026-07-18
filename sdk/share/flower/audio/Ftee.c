#include <stdio.h>
#include <stdlib.h>
#include "flower/flower.h"

typedef struct _TeeData
{
    bool branch;
} TeeData;

static void tee_init (IteFilter * f)
{
    TeeData * s = (TeeData *)ite_new(TeeData, 1);
    if (s != NULL)
    {
        s->branch = 1;
    }
    f->data = s;
}

static void tee_uninit (IteFilter * f)
{
    TeeData * s = (TeeData *)f->data;
    if (s != NULL)
    {
        free(s);
    }
}

mblk_ite * mblk_copy (mblk_ite * src)
{
    mblk_ite * dest = NULL;

    do
    {
        if (src == NULL)
        {
            break;
        }

        dest = allocb_ite(src->size);
        if (dest == NULL)
        {
            break;
        }

        if (src->size > 0)
        {
            memcpy(dest->b_rptr, src->b_rptr, src->size);
            dest->b_wptr += src->size;
        }
    } while (false);

    return dest;
}

static void tee_process (IteFilter * f)
{
    TeeData *   s      = (TeeData *)f->data;
    IteQueueblk blk    = {0};
    IteQueueblk blkdup = {0};
    while (f->run)
    {

        if (IteAudioQueueController(f, 1, 30, 5) == -1 ||
            IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            continue;
        }

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om = blk.datap;
            if (om && s->branch)
            {
                mblk_ite * dupom = mblk_copy(om);
                blkdup.datap     = dupom;
                ite_queue_put(f->output[1].Qhandle, &blkdup);
            }
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
        usleep(10000);
    }
}

static void tee_set_branch (IteFilter * f, void * arg)
{
    TeeData * d = (TeeData *)f->data;
    d->branch   = *((int *)arg);
}

static IteMethodDes tee_methods[] = {
    {ITE_FILTER_SET_VALUE, tee_set_branch},
    {                   0,           NULL}
};

// clang-format off
IteFilterDes FilterTee = {
    ITE_FILTER_TEE_ID,
    tee_init,
    tee_uninit,
    tee_process,
    tee_methods
};
// clang-format on
