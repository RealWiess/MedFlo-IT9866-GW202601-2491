#include <stdio.h>
#include "flower/flower.h"
#include "i2s/i2s.h"
#include "ite/audio.h"
#include "audio/include/volumeapi.h"

void *     autostop_task (IteFlower * f);
extern int gbytes;

enum
{
    SINGLEPLAYER = 0,
    MULTIPLAYER,
    NONPLAYER,
};

typedef struct _QLayData
{
    int        byte20ms;
    Volume *   v;
    cb_sound_t fn_cb;
    int        autorelease;
    int        option;
    dBctrl     dk;
} QLayData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static bool _check_eof (IteFilter * f, IteQueueblk * blk)
{
    QLayData *  d      = (QLayData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    bool        result = false;
    if (flower->audiocase == AudioRelay)
    {
        if ((PlaySoundCase)blk->private1 == Eofclose &&
            (PlaySoundCase)blk->private2 == Eofclose)
        {
            result = true;
        }
    }
    else
    {
        if ((PlaySoundCase)blk->private1 == Eofsound)
        {
            result = true;
        }
    }
    return result;
}

static void fadeing_do (QLayData * d, mblk_ite * om)
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
        VOICE_set_dB_gain(d->v, (float)s->curdB);
    }
    if (s->curdB != 0.0)
    {
        VOICE_applyProcess(d->v, om);
    }
}

static void QLAY (IteFilter * f)
{
#if CFG_RESERVE_FILTER
    QLayData *  d      = (QLayData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    while (f->run)
    {
        IteQueueblk blk = {0};
        if (IteFilterController(f) == FILTER_PAUSE)
        {
            continue;
        }
        if (checkQcount(flower, 32))
        {
            continue;
        }

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om = blk.datap;

            d->dk.tardB   = flower->dInfo.sw_dB;
            fadeing_do(d, om);

            if (om)
            {
                flowQPut(flower, om->b_rptr, om->size);
                freemsg_ite(om);
            }
            if (_check_eof(f, &blk))
            {
                flowQEof(flower);
                while (flowQCount(flower))
                { // wait some time data play over
                    usleep(1000 * (flowQCount(flower) + 1));
                    if (!f->run)
                    {
                        goto QLAY_end;
                    }
                }
                if (d->autorelease == true)
                {
                    flower->cb_func = d->fn_cb;
                    CreateDetachedThread(autostop_task, flower);
                }
            }

            if (flower->cb_link)
            { // link callback
                flower->cb_link(flower);
                flower->cb_link = NULL;
            }
        }
        usleep(1000 * (flowQCount(flower) + 1));
    }
QLAY_end:;
#endif
}

static void QSPEEDCTRL (IteFilter * f)
{
    // QLayData *d=(QLayData*)f->data;
    IteFlower *  flower = (IteFlower *)f->srcFlow;
    unsigned int cbyte  = 0;
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

        if (GET_DA_RW_RGAP > gbytes * 4)
        {
            usleep(100 * DAWAITR(gbytes));
            continue;
        }

        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * om = blk.datap;
            if (om)
            {
                cbyte += om->size;
            }
            flower->dInfo.currentms = 20 * (cbyte / gbytes);
            if ((PlaySoundCase)blk.private1 == Eofsound)
            {
                blk.private1 = (void *)Eofclose;
                blk.private2 = (void *)Eofclose;
            }
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void QBYPASS (IteFilter * f)
{

    QLayData *  d      = (QLayData *)f->data;
    IteFlower * flower = (IteFlower *)f->srcFlow;
    // unsigned int cbyte=0;
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
            mblk_ite * om = blk.datap;
            d->dk.tardB   = flower->dInfo.sw_dB;
            fadeing_do(d, om);
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
        usleep(1000 * (IteAudioQueueNum(f, 0) + 1));
    }
}

static void qlay_init (IteFilter * f)
{
    QLayData * d = (QLayData *)ite_new(QLayData, 1);
    if (d != NULL)
    {
        d->byte20ms    = 512;
        d->v           = VOICE_INIT();
        d->autorelease = true;
        d->option      = MULTIPLAYER;
        d->dk          = (dBctrl){0, 0, 0, 0, 0, 1000};
    }
    f->data = d;
}

static void qlay_uninit (IteFilter * f)
{
    QLayData * d = (QLayData *)f->data;
    if (d != NULL)
    {
        IteFlower * flower = (IteFlower *)f->srcFlow;
        VOICE_UNINIT(d->v);
        free(d);
    }
}

static void qlay_process (IteFilter * f)
{
    QLayData * d = (QLayData *)f->data;

    switch (d->option)
    {
        case MULTIPLAYER:
            QLAY(f);
            break;
        case SINGLEPLAYER:
            QSPEEDCTRL(f);
            break;
        case NONPLAYER:
            QBYPASS(f);
            break;
    }
}

static void qlay_set_cb (IteFilter * f, void * arg)
{
    QLayData * d = (QLayData *)f->data;
    d->fn_cb     = (cb_sound_t)arg;
}

static void qlay_set_option (IteFilter * f, void * arg)
{
    QLayData * d = (QLayData *)f->data;
    if (arg == NULL)
    {
        d->option = SINGLEPLAYER;
    }
    else if (*((int *)arg) == 2)
    {
        d->option = NONPLAYER;
    }
}

static void qlay_set_autorelease (IteFilter * f, void * arg)
{
    QLayData * d   = (QLayData *)f->data;
    d->autorelease = *((int *)arg);
}

static IteMethodDes qlay_methods[] = {
    {   ITE_FILTER_SET_CB,          qlay_set_cb},
    {ITE_FILTER_SET_VALUE,      qlay_set_option},
    {ITE_FILTER_SET_PARAM, qlay_set_autorelease},
    {                   0,                 NULL}
};

// clang-format off
IteFilterDes FilterQLay = {
    ITE_FILTER_QLAYE_ID,
    qlay_init,
    qlay_uninit,
    qlay_process,
    qlay_methods
};
// clang-format on
