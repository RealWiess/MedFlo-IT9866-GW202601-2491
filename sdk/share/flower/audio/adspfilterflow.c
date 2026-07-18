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

#define CHECK_(condition)                                                      \
    if (!(condition))                                                          \
    {                                                                          \
        result = -1;                                                           \
        break;                                                                 \
    }

// QRecFlowStart start
int QRecFlowStart (IteFlower * f, int rate, int ch)
{
    //[sndread]-[Qwrite]
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsndread = Flow_filter_new(ITE_FILTER_ADREAD_ID, &f);
        CHECK_(s->Fsndread != NULL);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QWRITE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

        adread_reinit_for_diff_rate(rate, 16, ch);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);
        ite_filterChain_run(&s->fc);

        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void QRecFlowStop (IteFlower * f)
{

    IteAudioFlower * s = f->audiostream;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
}

/*get QWtrte filter queue data: return mblk_ite/NULL ,*/
mblk_ite * QWriteFilterGetData (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    mblk_ite *       m = NULL;
    if (!s)
    {
        printf("QWRITE filter not exit\n");
        return m;
    }
    if (s->Fdestinat->filterDes.id == ITE_FILTER_QWRITE_ID)
    {
        ite_filter_call_method(s->Fdestinat, ITE_FILTER_GET_PARAM, &m);
    }
    return m;
}

/*get QWrite filter queue number: return current queue num*/
int QWriteFilterGetDataQSize (IteFlower * f)
{
    IteAudioFlower * s   = f->audiostream;
    int              num = 0;
    if (!s)
    {
        printf("QWRITE filter not exit\n");
        return num;
    }
    if (s->Fdestinat->filterDes.id == ITE_FILTER_QWRITE_ID)
    {
        ite_filter_call_method(s->Fdestinat, ITE_FILTER_GET_VALUE, &num);
    }
    return num;
}

/*get QWrite filter queue data: return len/0 */
int QWriteFilterGetDataPtr (IteFlower * f, unsigned char * pscr, int * len)
{
    mblk_ite * m = QWriteFilterGetData(f);
    if (m)
    {
        memcpy(pscr, m->b_rptr, m->size);
        *len = m->size;
        freemsg_ite(m);
        return *len;
    }
    return 0;
}

// QRecFlowStart end

/*DspResampleFlow start */

int DspResampleFlowStart (IteFlower * f, int in_rate, int out_rate)
{
    //[Qread]-[Resample]-[Qwrite]
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        (void)printf("IN:%d resample to OUT%d\n", in_rate, out_rate);
        dinfoSetDefault(&f->dInfo);

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_QREAD_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fdecoder = Flow_filter_new(ITE_FILTER_RESAMPLE_ID, &f);
        CHECK_(s->Fdecoder != NULL);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QWRITE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

        ite_filter_call_method(s->Fdecoder, ITE_FILTER_INPUT_RATE, &in_rate);
        ite_filter_call_method(s->Fdecoder, ITE_FILTER_OUTPUT_RATE, &out_rate);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void DspBTResampleFlowStart (IteFlower * f, int in_rate, int out_rate,
                             int out_chn)
{
    //[Qread]-[Resample]-[Qwrite]
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        printf("IN:%d resample to OUT%d\n", in_rate, out_rate);
        dinfoSetDefault(&f->dInfo);

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_QREAD_ID, &f);
        CHECK_(s->Fsource != NULL);

        if (in_rate != out_rate)
        {
            s->Fdecoder = Flow_filter_new(ITE_FILTER_RESAMPLE_ID, &f);
            CHECK_(s->Fdecoder != NULL);
        }

        s->Fmix = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fmix != NULL);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_DAWRITE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

        int chn = out_chn;
        if (s->Fdecoder)
        {
            ite_filter_call_method(s->Fdecoder, ITE_FILTER_INPUT_RATE,
                                   &in_rate);
            ite_filter_call_method(s->Fdecoder, ITE_FILTER_OUTPUT_RATE,
                                   &out_rate);
            ite_filter_call_method(s->Fdecoder, ITE_FILTER_SET_NCHANNELS, &chn);
        }

        int mode = 2;
        ite_filter_call_method(s->Fmix, ITE_FILTER_SET_VALUE, &mode);

        dawrite_deinit_state();
        dawrite_reinit_for_diff_rate(out_rate, 16, out_chn);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        if (s->Fdecoder)
        {
            ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
        }
        ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }
}

void DspResampleFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if (s->Fdecoder)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdecoder, 0);
    }
    if (s->Fmix)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    }
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
    f->audiostream = NULL;
}

/*put data to Qread filter queue*/
void QReadFilterPutData (IteFlower * f, mblk_ite * m)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("QRead filter not exit\n");
        if (m)
        {
            freemsg_ite(m);
        }
        return;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_QREAD_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_SET_PARAM, &m);
    }
}

/*get Qread filter queue number: return current queue num*/
int QReadFilterGetDataQSize (IteFlower * f)
{
    IteAudioFlower * s   = f->audiostream;
    int              num = 0;
    if (!s)
    {
        printf("QRead filter not exit\n");
        return num;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_QREAD_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_GET_VALUE, &num);
    }
    return num;
}

/*put data to Qread filter queue*/
void QReadFilterPutDataPtr (IteFlower * f, unsigned char * psrc, int len)
{
    mblk_ite * im = allocb_ite(len);
    memcpy(im->b_wptr, psrc, len);
    im->b_wptr += len;
    QReadFilterPutData(f, im);
}

/*DspResampleFlow end */

#if CFG_FADE_FILTER
void FadeFilterParmSet (IteFlower * f, PlaySoundCase fademode, int sustainms)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Ftee->filterDes.id == ITE_FILTER_FADE_ID)
    {
        dBctrl dk = (dBctrl){0, 0, 0, 0, 0, sustainms};
        switch (fademode)
        {
            case FADEIN:
                dk = (dBctrl){-30, 0, -30, 0, 0, sustainms};
                break;
            case FADEOUT:
                dk = (dBctrl){0, -30, 0, 0, 0, sustainms};
                break;
        }
        ite_filter_call_method(s->Ftee, ITE_FILTER_SET_VALUE, &dk);
    }
}
#endif

int MicAECFlowStart (IteFlower * f, int rate, int channel)
{
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsndread = Flow_filter_new(ITE_FILTER_ADREAD_ID, &f);
        CHECK_(s->Fsndread != NULL);

#if CFG_AEC_ENABLE
        s->Faec = Flow_filter_new(ITE_FILTER_AEC_ID, &f);
        CHECK_(s->Faec != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_LOOPBACK_ID, &f);
        CHECK_(s->Fsource != NULL);
#endif

        s->Fdestinat =
            Flow_filter_new(ITE_FILTER_QWRITE_ID, &f); // chip size 240
        CHECK_(s->Fdestinat != NULL);

        adread_reinit_for_diff_rate(rate, 16, channel);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

#if CFG_AEC_ENABLE
        ite_filterChain_build(&s->fcc, "FC 2");
        ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
#endif

        ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);

#if CFG_AEC_ENABLE

        ite_filterChain_link(&s->fc, 0, s->Faec, 1);
#endif
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

#if CFG_AEC_ENABLE
        ite_filterChain_link(&s->fcc, -1, s->Fsource, -1);
        ite_filterChain_link(&s->fcc, 0, s->Faec, 0);
#endif
        ite_filterChain_run(&s->fc);
#if CFG_AEC_ENABLE
        ite_filterChain_run(&s->fcc);
#endif

        f->audiocase   = AudioRecFile;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void MicAECFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioRecFile)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    if (s->Faec != NULL)
    {
        ite_filterChain_stop(&s->fcc);
    }

    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    if (s->Faec != NULL)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Faec, 1);
    }
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);

    if (s->Faec != NULL)
    {
        ite_filterChain_unlink(&s->fcc, -1, s->Fsource, -1);
        ite_filterChain_unlink(&s->fcc, 0, s->Faec, 0);
    }

    if (s->Faec != NULL)
    {
        ite_filterChain_delete(&s->fcc);
    }
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
}

void HFPPacketFlowStart (IteFlower * f, int in_rate, int in_chan)
{
    //[Qread]-[DAwrite]
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_QREAD_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fsndwrite = Flow_filter_new(ITE_FILTER_DAWRITE_ID, &f);
        CHECK_(s->Fsndwrite != NULL);

        dawrite_reinit_for_diff_rate(in_rate, 16, in_chan);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

        ite_filterChain_run(&s->fc);

        f->audiocase   = StreamPlay;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }
}

void HFPPacketFlowStop (IteFlower * f)
{

    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != StreamPlay)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);

    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
}
