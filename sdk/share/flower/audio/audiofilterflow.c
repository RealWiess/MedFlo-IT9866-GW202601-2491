#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "ite/audio.h"
#include "i2s/i2s.h"

#if CFG_MIXENABLE
static int   RESAMPLE_RATE = 32000;
static int   RESAMPLE_CH   = 1;
static bool  RESAMPLE_INIT = true;
static float SPEED         = 1.0;

void         resample_parm_alter (int sr, int ch, bool yesno)
{
    // printf("sr(%d)->(%d) ch:(%d)->(%d)
    // mixInfo.init=%d\n",RESAMPLE_RATE,sr,RESAMPLE_CH,ch,yseno);
    RESAMPLE_RATE = sr;
    RESAMPLE_CH   = ch;
    RESAMPLE_INIT = yesno;
}

void resample_speed_alter (float speed)
{
    SPEED = speed;
}

void resample_filter_new (IteAudioFlower ** s0, IteFlower * f)
{
    IteAudioFlower * s = *s0;
    dinfoSetDefault(&f->mixInfo);
    f->mixInfo.sr        = RESAMPLE_RATE;
    f->mixInfo.nch       = RESAMPLE_CH;
    f->mixInfo.bitsize   = 16;
    f->mixInfo.bytes20ms = 20 * f->mixInfo.sr * f->mixInfo.nch * 2 / 1000;
    f->mixInfo.duration  = f->dInfo.duration;
    f->mixInfo.init      = RESAMPLE_INIT;
    if (!f->mixInfo.init)
    {
        return;
    }
    s->Ftcpsend = Flow_filter_new(ITE_FILTER_CHADAPT_ID, &f);  // channel adapt
    s->Ftcprecv = Flow_filter_new(ITE_FILTER_RESAMPLE_ID, &f); // resample
    int in_chn  = f->dInfo.nch;
    ite_filter_call_method(s->Ftcpsend, ITE_FILTER_SET_VALUE, &in_chn);
    ite_filter_call_method(s->Ftcpsend, ITE_FILTER_SET_NCHANNELS,
                           &f->mixInfo.nch);
    ite_filter_call_method(s->Ftcprecv, ITE_FILTER_SET_NCHANNELS,
                           &f->mixInfo.nch);
    ite_filter_call_method(s->Ftcprecv, ITE_FILTER_OUTPUT_RATE, &f->mixInfo.sr);
    #if CFG_BUILD_AUDIO_PREPROCESS
    if (SPEED != 1.0 && RESAMPLE_CH == 1)
    {
        s->Fsrc = Flow_filter_new(ITE_FILTER_STRETCH_ID, &f);
        ite_filter_call_method(s->Fsrc, ITE_FILTER_SET_VALUE, &SPEED);
        ite_filter_call_method(s->Fsrc, ITE_FILTER_SET_PARAM, &f->mixInfo.sr);
        ite_filter_call_method(s->Fsrc, ITE_FILTER_A_Method, NULL);
    }
    #endif
}

void resample_filter_ctrl_link (IteAudioFlower ** s0, bool link)
{
    IteAudioFlower * s = *s0;
    if (link)
    {
        if (s->Ftcpsend)
        {
            ite_filterChain_link(&s->fc, 0, s->Ftcpsend, 0);
        }
        if (s->Ftcprecv)
        {
            ite_filterChain_link(&s->fc, 0, s->Ftcprecv, 0);
        }
        if (s->Fsrc)
        {
            ite_filterChain_link(&s->fc, 0, s->Fsrc, 0);
        }
    }
    else
    {
        if (s->Ftcpsend)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Ftcpsend, 0);
        }
        if (s->Ftcprecv)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Ftcprecv, 0);
        }
        if (s->Fsrc)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Fsrc, 0);
        }
    }
}

#endif

void audio_flower_free (IteAudioFlower * s)
{
    if (s != NULL)
    {
        if (s->Fsndwrite)
        {
            ite_filter_delete(s->Fsndwrite);
        }
        if (s->Fsndread)
        {
            ite_filter_delete(s->Fsndread);
        }
        if (s->Fsource)
        {
            ite_filter_delete(s->Fsource);
        }
        if (s->Fdestinat)
        {
            ite_filter_delete(s->Fdestinat);
        }
        if (s->Fudprecv)
        {
            ite_filter_delete(s->Fudprecv);
        }
        if (s->Fudpsend)
        {
            ite_filter_delete(s->Fudpsend);
        }
        if (s->Ftcprecv)
        {
            ite_filter_delete(s->Ftcprecv);
        }
        if (s->Ftcpsend)
        {
            ite_filter_delete(s->Ftcpsend);
        }
        if (s->Fmix)
        {
            ite_filter_delete(s->Fmix);
        }
        if (s->Faec)
        {
            ite_filter_delete(s->Faec);
        }
        if (s->Fdecoder)
        {
            ite_filter_delete(s->Fdecoder);
        }
        if (s->Fencoder)
        {
            ite_filter_delete(s->Fencoder);
        }
        if (s->Fasr)
        {
            ite_filter_delete(s->Fasr);
        }
        // if(s->Frec_avi)  ite_filter_delete(s->Frec_avi);
        if (s->Ftee)
        {
            ite_filter_delete(s->Ftee);
        }
        if (s->Fsrc)
        {
            ite_filter_delete(s->Fsrc);
        }
        // ite_flower_deinit();
        free(s);
    }
}
