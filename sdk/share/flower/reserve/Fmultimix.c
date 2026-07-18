#include <stdio.h>
#include <math.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "audio/include/fileheader.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

//=============================================================================
//                              struct Definition
//=============================================================================
static int32_t compressor_do (int32_t value)
{
    float   threshhold = 32767 * 0.9; //(32767*0.9);
    int32_t result     = value;
    if (value > threshhold)
    {
        float dBgain     = 10 * log10((float)abs(value) / threshhold);
        float compressor = (float)0.99; // initial value;
        do
        {
            result = value * (pow(10, (dBgain * (compressor - 1) / 10)));
            compressor *= 0.9; // regression speed 0>x>1
        } while (abs(result) >= 32767);
    }
    return result;
}

static mblk_ite * mblkvolmixctrl (mblk_ite * m0, float gain0, mblk_ite * m1,
                                  float gain1)
{
    mblk_ite * o = allocb_ite(m0->size);
    for (; m0->b_rptr < m0->b_wptr;
         m0->b_rptr += 2, m1->b_rptr += 2, o->b_wptr += 2)
    {
        int32_t data32 = (gain0 * (int32_t)*(int16_t *)m0->b_rptr) +
                         (gain1 * (int32_t)*(int16_t *)m1->b_rptr);
        // data32=compressor_do(data32);
        *((int16_t *)(o->b_wptr)) = clamp16(data32); // prvent mix overflow
    }
    return o;
}

typedef struct _MultimixData
{
    PlayerState     state;
    int             framesize;
    int             pin0;
    int             pin1;
    IteFlower *     srcFlow[4];
    pthread_mutex_t mutex;
    int             duckpin;
    dBctrl          dk;
} MultimixData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void ducking_do (MultimixData * d, mblk_ite * m)
{
    dBctrl * s = &d->dk;
    if (s->curdB != s->tardB)
    { // updata sw_dB
        if (s->predB != s->tardB)
        {
            if (s->delayms == 0)
            {
                s->delayms = 20;
            }
            s->gap   = (s->tardB - s->curdB) / (s->delayms / 20);
            s->predB = s->tardB;
        }
        s->curdB += s->gap;
        if ((((s->curdB - s->tardB) * (s->curdB + s->gap - s->tardB)) <= 0))
        {
            s->curdB = s->tardB;
            s->gap   = 0.0;
        }
        s->alpha = pow(10, (s->curdB) / 10);
    }
    if (s->curdB != 0.0)
    {
        mblkvolctrl(m, s->alpha);
    }
}

static void multimix_get_pin_value (IteFilter * f, void * arg)
{
    MultimixData * d = (MultimixData *)f->data;
    int *          p = (int *)arg;
    *p               = d->pin0;
    *(p + 1)         = d->pin1;
}

static void multimix_set_duckpin (IteFilter * f, void * arg)
{
    MultimixData * d = (MultimixData *)f->data;
    d->duckpin       = *(int *)arg;
}

/*static void multimix_get_status(IteFilter *f, void *arg){
    MultimixData *d=(MultimixData*)f->data;
    *(int*)arg = d->state;
}*/
/*
Link example:
  [src]        [mixpin]
srcFlow[0]------pin0=0
srcFlow[1]
srcFlow[2]   |--pin1=3
srcFlow[3]---|
*/

static void multimix_cut_flowerp (IteFilter * f, void * arg)
{
    MultimixData * d        = (MultimixData *)f->data;
    void **        p        = arg;
    IteFlower *    flownew  = *p;
    IteFlower *    flowcur0 = d->srcFlow[d->pin0];
    IteFlower *    flowcur1 = d->srcFlow[d->pin1];

    pthread_mutex_lock(&d->mutex);
    if (flowcur0 == flownew)
    {
        Flow_status_set((IteFlower *)flowcur0, FILTER_PAUSE);
        d->pin0 = -1;
    }
    if (flowcur1 == flownew)
    {
        Flow_status_set((IteFlower *)flowcur1, FILTER_PAUSE);
        d->pin1 = -1;
    }
    pthread_mutex_unlock(&d->mutex);
}

static void fplinktofilter (IteFilter * f, void * arg, int pin)
{
    MultimixData * d       = (MultimixData *)f->data;
    IteFlower *    flownew = arg;
    IteFlower *    flowcur = NULL;
    int            mixpin  = (int)flownew->userp;
    if (!flownew->mixInfo.init && mixpin)
    {
        printf("mix not Enable ,switch to pin0\n");
        mixpin = 0;
    }
    pthread_mutex_lock(&d->mutex);

    switch (mixpin)
    {
        case 0:
            if (d->pin0 >= 0)
            {
                flowcur = d->srcFlow[d->pin0];
                Flow_status_set(flowcur, FILTER_PAUSE);
            }

            if (d->pin1 == pin)
            {
                d->pin1 = -1; // pin0==pin1 , disconnet pin1
                Flow_status_set(d->srcFlow[d->pin0], FILTER_PAUSE);
            }
            d->pin0 = pin;
            Flow_status_set(flownew, FILTER_RESUME);

            break;
        case 1:
            if (d->pin1 >= 0)
            {
                flowcur = d->srcFlow[d->pin1];
                Flow_status_set(flowcur, FILTER_PAUSE);
            }

            if (d->pin0 == pin)
            {
                Flow_status_set(d->srcFlow[d->pin0], FILTER_PAUSE);
                d->pin0 = -1; // pin1==pin0 , disconnet pin0
            }
            d->pin1 = pin;
            Flow_status_set(flownew, FILTER_RESUME);
            break;
    }

    d->srcFlow[pin] = flownew;

    if (flownew->mixInfo.init == true)
    {
        if (dawrite_reinit_for_diff_rate(flownew->mixInfo.sr,
                                         flownew->mixInfo.bitsize,
                                         flownew->mixInfo.nch))
        {
            printf("resample i2s sr:%d ch:%d bitsize:%d\n", flownew->mixInfo.sr,
                   flownew->mixInfo.nch, flownew->mixInfo.bitsize);
        }
        d->framesize = flownew->mixInfo.bytes20ms;
    }
    else
    {
        if (dawrite_reinit_for_diff_rate(
                flownew->dInfo.sr, flownew->dInfo.bitsize, flownew->dInfo.nch))
        {
            printf("reinit i2s sr:%d ch:%d bitsize:%d\n", flownew->dInfo.sr,
                   flownew->dInfo.nch, flownew->dInfo.bitsize);
            i2s_pause_DAC(false);
        }
        d->framesize = flownew->dInfo.bytes20ms;
    }
    printf("srcFlow%d link to pin%d\n", pin, mixpin);
    pthread_mutex_unlock(&d->mutex);
}

static void multimix_set_flowerp3 (IteFilter * f, void * arg)
{
    void ** p = arg;
    fplinktofilter(f, *p, 3);
}
static void multimix_set_flowerp2 (IteFilter * f, void * arg)
{
    void ** p = arg;
    fplinktofilter(f, *p, 2);
}
static void multimix_set_flowerp1 (IteFilter * f, void * arg)
{
    void ** p = arg;
    fplinktofilter(f, *p, 1);
}
static void multimix_set_flowerp0 (IteFilter * f, void * arg)
{
    void ** p = arg;
    fplinktofilter(f, *p, 0);
}

//=============================================================================
//                              filter flow
//=============================================================================
static void multimix_init (IteFilter * f)
{
    MultimixData * d = (MultimixData *)ite_new(MultimixData, 1);
    if (d != NULL)
    {
        d->state     = Closed;
        d->framesize = 320; // 20*(2*d->rate*d->nchannels)/1000;//20ms data
        d->pin0      = -1;
        d->pin1      = -1;
        d->duckpin   = -1; // -1:noducking 0:pin0 ducking 1:pin1 ducking
        d->dk        = (dBctrl){0, 0, 0, 0, 0, 1200};
        for (int i = 0; i < 4; i++)
        {
            d->srcFlow[i] = NULL;
        }
        pthread_mutex_init(&d->mutex, NULL);
    }
    f->data = d;
}

static void multimix_uninit (IteFilter * f)
{
    MultimixData * d = (MultimixData *)f->data;
    if (d != NULL)
    {
        for (int i = 0; i < 4; i++)
        {
            d->srcFlow[i] = NULL;
        }
        pthread_mutex_destroy(&d->mutex);
        free(d);
    }
}

static void Playing_do (IteFilter * f)
{
    MultimixData * d       = (MultimixData *)f->data;
    IteFlower *    flower0 = d->srcFlow[d->pin0];
    IteQueueblk    blk     = {0};
    mblk_ite *     m0      = flowQGet(flower0); // get data from Q
    if (m0)
    {
        if (m0->size == 0)
        {
            blk.datap    = m0;
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
            d->pin0  = -1;
            printf("Playing_do pin0 EOF\n");
            return;
        }
        else
        {
            if (d->duckpin == 0)
            {
                ducking_do(d, m0);
            }
            blk.datap = m0;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
    }
}

static void Subplaying_do (IteFilter * f)
{
    MultimixData * d       = (MultimixData *)f->data;
    IteFlower *    flower1 = d->srcFlow[d->pin1];
    IteQueueblk    blk     = {0};
    mblk_ite *     m1      = flowQGet(flower1); // get data from Q

    if (m1)
    {
        if (m1->size == 0)
        {
            blk.datap    = m1;
            blk.private2 = (void *)Eofmixsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Closed;
            d->pin1  = -1;
            printf("Subplaying_do pin1 EOF\n");
            return;
        }
        else
        {
            if (d->duckpin == 1)
            {
                ducking_do(d, m1);
            }
            blk.datap = m1;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
    }
}

static void Mixing_do (IteFilter * f)
{
    MultimixData * d       = (MultimixData *)f->data;
    IteFlower *    flower0 = d->srcFlow[d->pin0];
    IteFlower *    flower1 = d->srcFlow[d->pin1];
    IteQueueblk    blk     = {0};
    int bytes = flower0->mixInfo.bytes20ms; // flower1->mixInfo.bytes20ms

    if (flower0->mixInfo.bytes20ms != flower1->mixInfo.bytes20ms)
    {
        printf("ERROR multimix\n");
    }

    while ((flowQCount(flower0) > 0) && (flowQCount(flower1) > 0))
    {
        if (IteAudioQueueController(f, 0, 30, 5) == -1)
        {
            break;                         // check output[0] full or not
        }
        mblk_ite * m0 = flowQGet(flower0); // get data from Q0
        mblk_ite * m1 = flowQGet(flower1); // get data from Q1

        if (m0 && m1 && m0->size == 0 && m1->size == 0)
        {
            freemsg_ite(m0);
            freemsg_ite(m1);
            d->pin0 = -1;
            d->pin1 = -1;
            printf("pin0 pin1 EOF\n");
            d->state = Closed;
            break;
        }

        if (m1 && m0 && m0->size == 0)
        {
            mblk_ite * o = allocb_ite(bytes);
            memcpy(o->b_wptr, m1->b_rptr, bytes);
            o->b_wptr += bytes;
            if (d->duckpin == 1)
            {
                d->dk.tardB = 0.0;
                ducking_do(d, o);
            }
            blk.datap    = o;
            blk.private1 = (void *)Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);

            freemsg_ite(m0);
            freemsg_ite(m1);
            d->pin0 = -1;
            // printf("background music play over but mix sound exit\n");
            printf("Mixing_do pin0 EOF\n");
            break;
        }

        if (m1 && m0 && m1->size == 0)
        { // eof mix
            /*non fading*/
            mblk_ite * o = allocb_ite(bytes);
            memcpy(o->b_wptr, m0->b_rptr, bytes);
            o->b_wptr += bytes;
            blk.private2 = (void *)Eofmixsound;
            if (d->duckpin == 0)
            {
                d->dk.tardB = 0.0;
                ducking_do(d, o);
            }
            blk.datap = o;
            ite_queue_put(f->output[0].Qhandle, &blk);

            freemsg_ite(m0);
            freemsg_ite(m1);

            d->pin1 = -1;
            printf("Mixing_do pin1 EOF\n");
            break;
        }

        if (m1 && m0)
        {
            mblk_ite * o;
            switch (d->duckpin)
            {
                case 0:
                    o = mblkvolmixctrl(m0, d->dk.alpha, m1, 1.0);
                    break;
                case 1:
                    o = mblkvolmixctrl(m0, 1.0, m1, d->dk.alpha);
                    break;
                default:
                    o = mblkvolmixctrl(m0, 1.0, m1, 1.0);
                    break;
            }

            freemsg_ite(m0);
            freemsg_ite(m1);
            blk.datap = o;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
        if (GET_DA_RW_RGAP > d->framesize * 5)
        {
            break;
        }
    }
}

static void _check_state (IteFilter * f)
{
    MultimixData * d = (MultimixData *)f->data;

    if (d->pin0 != -1 && d->pin1 == -1)
    {
        d->state = Playing;
    }
    else if (d->pin0 == -1 && d->pin1 != -1)
    {
        d->state = Subplaying;
    }
    else if (d->pin0 != -1 && d->pin1 != -1)
    {
        if (d->duckpin != -1)
        {
            d->dk.tardB = -10;
        }

        if (d->dk.curdB == d->dk.tardB)
        {
            d->state = Mixing;
        }
        else
        {
            switch (d->duckpin)
            {
                case 0:
                    d->state = Playing;
                    break;
                case 1:
                    d->state = Subplaying;
                    break;
                default:
                    d->state = Mixing;
                    break;
            }
        }
    }
    else
    { // d->pin0==-1 && d->pin1==-1
        d->state = Eof;
    }
}

static void multimix_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    MultimixData * d = (MultimixData *)f->data;

    if (d->pin0 == -1 && d->pin1 == -1)
    {
        f->status = FILTER_PAUSE; // befor first pin be linked,pause the filter
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

        if (GET_DA_RW_RGAP > d->framesize * 5)
        {
            usleep(5000);
            continue;
        }

        pthread_mutex_lock(&d->mutex);
        _check_state(f);
        if (d->state == Playing)
        {
            Playing_do(f);
        }
        if (d->state == Subplaying)
        {
            Subplaying_do(f);
        }
        if (d->state == Mixing)
        {
            Mixing_do(f);
        }
        pthread_mutex_unlock(&d->mutex);

        if (d->state == Closed)
        { // add null time(data) prevent abnormal sound
            blk.datap    = allocb0_ite(d->framesize);
            blk.private1 = (void *)Eofclose;
            blk.private2 = (void *)Eofclose;
            ite_queue_put(f->output[0].Qhandle, &blk);
            d->state = Eof;
        }

        if (d->state == Eof && IteAudioQueueIsEmpty(f, 0))
        {
            blk.datap = allocb0_ite(d->framesize);
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

// clang-format off
static IteMethodDes multimix_methods[] = {
    {ITE_FILTER_D_Method   , multimix_set_flowerp3},
    {ITE_FILTER_C_Method   , multimix_set_flowerp2},
    {ITE_FILTER_B_Method   , multimix_set_flowerp1},
    {ITE_FILTER_A_Method   , multimix_set_flowerp0},
    {ITE_FILTER_F_Method   , multimix_cut_flowerp },
    {ITE_FILTER_GET_VALUE  , multimix_get_pin_value},
    {ITE_FILTER_SET_PARAM  , multimix_set_duckpin},
    {0, NULL}
};

IteFilterDes FilterMultiMix = {
    ITE_FILTER_MULTIMIX_ID,
    multimix_init,
    multimix_uninit,
    multimix_process,
    multimix_methods
};
// clang-format on
