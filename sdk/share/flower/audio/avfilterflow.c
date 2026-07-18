#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "ite/audio.h"

#define CHECK_(condition)                                                      \
    if (!(condition))                                                          \
    {                                                                          \
        result = -1;                                                           \
        break;                                                                 \
    }

/*MixerPlay start*/
int AVPlayerFlowStart (IteFlower * f)
{
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        if (f->audiostream != NULL)
        {
            printf("stream is exit!! change another \n");
            result = -1;
            break;
        }

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        dinfoSetDefault(&f->dInfo);

        s->Fsource = Flow_filter_new(ITE_FILTER_PCMPOOL_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

#if CFG_MIXENABLE
        resample_filter_new(&s, f); // resample & channel adapt
#endif

        ite_filter_call_method(s->Fsource, ITE_FILTER_FILEOPEN, NULL);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
#if CFG_MIXENABLE
        if (f->mixInfo.init)
        {
            resample_filter_ctrl_link(&s, true);
        }
#endif
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);
        f->audiocase   = AudioAVPlay;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void AVPlayerFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioAVPlay)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
#if CFG_MIXENABLE
    if (f->mixInfo.init)
    {
        resample_filter_ctrl_link(&s, false);
    }
#endif
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
    f->audiostream = NULL;
    f->audiocase   = AudioIdle;
}
/*AVPlay end*/
