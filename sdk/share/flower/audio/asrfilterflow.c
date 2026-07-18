#include "flower/flower.h"
#include "i2s/i2s.h"
#include "ite/audio.h"
#include "openrtos/FreeRTOS.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define CHECK_(condition)                                                      \
    if (!(condition))                                                          \
    {                                                                          \
        result = -1;                                                           \
        break;                                                                 \
    }

extern STRC_I2S_SPEC spec_ad;
extern STRC_I2S_SPEC spec_da;
/*ASR start*/

void                 AsrFlowStart (IteFlower * f, cb_sound_t cb)
{
#if CFG_BUILD_ASR
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        // ite_flower_init();
        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsndread = Flow_filter_new(ITE_FILTER_ADREAD_ID, &f);
        CHECK_(s->Fsndread != NULL);

        s->Fasr = Flow_filter_new(ITE_FILTER_ASR_ID, &f);
        CHECK_(s->Fasr != NULL);

        if (s->Fasr)
        {
            ite_filter_call_method(s->Fasr, ITE_FILTER_SET_CB, cb);
        }

        // Castor3snd_reinit_for_diff_rate(16000,16,1);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
        ite_filterChain_link(&s->fc, 0, s->Fasr, 0);

        ite_filterChain_run(&s->fc);

        f->asrstream = s;
        i2s_ADC_channel_switch(1, 0);
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }
#endif
}

void AsrFlowStop (IteFlower * f)
{
#if CFG_BUILD_ASR
    IteAudioFlower * s = f->asrstream;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fasr, 0);
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
#endif
}

/*ASR end*/

/*resample play sound start*/
int ResmplePlayFlowStart (IteFlower * f, const char * filepath,
                          PlaySoundCase mode, cb_sound_t cb, int out_rate,
                          int out_channel)
{
#if CFG_MIXENABLE
    resample_parm_alter(out_rate, out_channel, true);
#endif
    return PlayFlowStart(f, filepath, mode, cb);
}

void ResmplePlayFlowStop (IteFlower * f)
{
    PlayFlowStop(f);
}
/*resample play sound end*/
