#include <stdio.h>
#include <stdlib.h>
#include "flower/flower.h"
#include "i2s/i2s.h"

/* Stubs for missing pre-built time_pv library in SDK 2491 */
static void time_pv_stretch (int16_t * data, int16_t * out, void * pv_parms) { (void)data; (void)out; (void)pv_parms; }
static void time_pv_init (int32_t fs, float rate, void ** pv_parms) { (void)fs; (void)rate; (void)pv_parms; }
static void time_pv_free (void * pv_parms) { (void)pv_parms; }

typedef struct _StretchData
{
    mblkq  dataQ;
    int    sample;
    int    rate;
    float  speed;
    void * pv_parms;
} StretchData;

static void stretch_init (IteFilter * f)
{
    StretchData * s = (StretchData *)ite_new(StretchData, 1);
    if (s != NULL)
    {
        mblkQShapeInit(&s->dataQ, 4096);
        s->speed             = 1.0;
        s->rate              = 16000;
        s->pv_parms          = NULL;
        f->pthread_stacksize = 32 * 1024;
    }
    f->data = s;
}

static void stretch_uninit (IteFilter * f)
{
    StretchData * s = (StretchData *)f->data;
    if (s != NULL)
    {
        mblkQShapeUninit(&s->dataQ);
        time_pv_free(s->pv_parms);
        free(s);
    }
}

static void stretch_process (IteFilter * f)
{
    StretchData * s         = (StretchData *)f->data;
    IteFlower *   flower    = (IteFlower *)f->srcFlow;
    IteQueueblk   blk       = {0};
    int           framesize = 0;

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

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * m = blk.datap;
            framesize    = m->size;
            mblkQShapePut(&s->dataQ, m, s->sample * 2);
        }

        while (getmblkqavail(&s->dataQ))
        {
            mblk_ite * m      = getmblkq(&s->dataQ);
            int        outlen = s->sample * s->speed;
            outlen *= 2;
            mblk_ite * om = allocb_ite(outlen);

            time_pv_stretch((int16_t *)m->b_rptr, (int16_t *)om->b_wptr,
                            s->pv_parms);

            om->b_wptr += outlen;
            blk.datap = om;
            ite_queue_put(f->output[0].Qhandle, &blk);
            if (m)
            {
                freemsg_ite(m);
            }
        }

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void stretch_set_speed (IteFilter * f, void * arg)
{
    StretchData * s = (StretchData *)f->data;
    s->speed        = *(float *)arg;
}

static void stretch_set_rate (IteFilter * f, void * arg)
{
    StretchData * s = (StretchData *)f->data;
    s->rate         = *(int *)arg;
}

static void stretch_set_pvinit (IteFilter * f, void * arg)
{
    StretchData * s = (StretchData *)f->data;
    time_pv_init(s->rate, s->speed, &s->pv_parms);
    if (s->rate >= 32000)
    {
        s->sample = 128;
    }
    else
    {
        s->sample = 64;
    }
}

static IteMethodDes stretch_methods[] = {
    {ITE_FILTER_SET_VALUE,  stretch_set_speed},
    {ITE_FILTER_SET_PARAM,   stretch_set_rate},
    { ITE_FILTER_A_Method, stretch_set_pvinit},
    {                   0,               NULL}
};

// clang-format off
IteFilterDes FilterStretch = {
    ITE_FILTER_STRETCH_ID,
    stretch_init,
    stretch_uninit,
    stretch_process,
    stretch_methods
};
// clang-format on
