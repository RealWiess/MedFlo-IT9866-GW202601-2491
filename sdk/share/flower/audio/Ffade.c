#include <stdio.h>
#include <math.h>
#include "flower/flower.h"

typedef struct _FadeData
{
    dBctrl          dk;
    pthread_mutex_t mutex;
} FadeData;

static void fadeing_do (FadeData * d, mblk_ite * m)
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

static void fade_init (IteFilter * f)
{
    FadeData * s = (FadeData *)ite_new(FadeData, 1);
    if (s != NULL)
    {
        pthread_mutex_init(&s->mutex, NULL);
        s->dk = (dBctrl){0, 0, 0, 0, 0, 2000};
    }
    f->data = s;
}

static void fade_uninit (IteFilter * f)
{
    FadeData * s = (FadeData *)f->data;
    if (s != NULL)
    {
        pthread_mutex_destroy(&s->mutex);
        free(s);
    }
}

static void fade_set_mode (IteFilter * f, void * arg)
{
    FadeData *    s    = (FadeData *)f->data;
    PlaySoundCase mode = *((int *)arg);
    pthread_mutex_lock(&s->mutex);
    s->dk = *((dBctrl *)arg);
    pthread_mutex_unlock(&s->mutex);
}

static void fade_process (IteFilter * f)
{
    FadeData *  s   = (FadeData *)f->data;
    IteQueueblk blk = {0};

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

        pthread_mutex_lock(&s->mutex);
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * m = blk.datap;
            fadeing_do(s, m);
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
        pthread_mutex_unlock(&s->mutex);
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static IteMethodDes fade_methods[] = {
    {ITE_FILTER_SET_VALUE, fade_set_mode},
    {                   0,          NULL}
};

// clang-format off
IteFilterDes FilterFade = {
    ITE_FILTER_FADE_ID,
    fade_init,
    fade_uninit,
    fade_process,
    fade_methods
};
// clang-format on
