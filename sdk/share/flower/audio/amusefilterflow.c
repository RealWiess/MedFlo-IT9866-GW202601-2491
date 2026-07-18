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

IteFilter * select_open_filter_new (const char * file, IteFlower * f)
{
    int         type = audiomgrExtensionName((char *)file);
    IteFilter * Fp   = NULL;
    switch (type)
    {
        case ITE_WAV:
            Fp = Flow_filter_new(ITE_FILTER_PCMGRAB_ID, (IteFlower **)f);
            break; //{CPU1)
#ifdef CFG_BUILD_MP3DEC
        case ITE_MP3:
            Fp = Flow_filter_new(ITE_FILTER_MP3GRAB_ID, (IteFlower **)f);
            break; //(CPU1)
#else
        case ITE_MP3:
            Fp = Flow_filter_new(ITE_FILTER_MGRGRAB_ID, (IteFlower **)f);
            break; //(CPU2)
#endif
#ifdef CFG_BUILD_AACDEC
        case ITE_AAC:
            Fp = Flow_filter_new(ITE_FILTER_AACGRAB_ID, (IteFlower **)f);
            break; //(CPU1)
#else
        case ITE_AAC:
            Fp = Flow_filter_new(ITE_FILTER_MGRGRAB_ID, (IteFlower **)f);
            break; //(CPU2)
#endif
        case ITE_WMA:
            Fp = Flow_filter_new(ITE_FILTER_MGRGRAB_ID, (IteFlower **)f);
            break; //(CPU2)
#if defined(CFG_BUILD_AACDEC) && defined(CFG_FFMPEG_AUDIO_DECODE)
        case ITE_M4A:
            Fp = Flow_filter_new(ITE_FILTER_DEM4A_ID, (IteFlower **)f);
            break; //(CPU1)
#else
        case ITE_M4A:
            Fp = Flow_filter_new(ITE_FILTER_M4AGRAB_ID, (IteFlower **)f);
            break; //(CPU2)
#endif
        case ITE_FLAC:
            Fp = Flow_filter_new(ITE_FILTER_MGRGRAB_ID, (IteFlower **)f);
            break; //(CPU2)
#ifdef CFG_BUILD_VORBISDEC
        case ITE_VORBIS:
            Fp = Flow_filter_new(ITE_FILTER_VORBISGRAB_ID, (IteFlower **)f);
            break; //(CPU1)
#else
        case ITE_VORBIS:
            Fp = Flow_filter_new(ITE_FILTER_MGRGRAB_ID, (IteFlower **)f);
            break; //(CPU2)
#endif
#ifdef CFG_BUILD_OPUS
        case ITE_OPUS:
            Fp = Flow_filter_new(ITE_FILTER_OPUSGRAB_ID, (IteFlower **)f);
            break; //(CPU1)
#else
        case ITE_OPUS:
            Fp = Flow_filter_new(ITE_FILTER_MGRGRAB_ID, (IteFlower **)f);
            break; //(CPU2)
#endif
        default:
            break;
    }

    return Fp;
}

/*play sound start*/
int PlayFlowStart (IteFlower * f, const char * filepath, PlaySoundCase mode,
                   cb_sound_t cb)
{
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        if (access(filepath, 0) < 0)
        {
            printf("%s is not exist! PlayFlowStart not do\n", filepath);
            result = -1;
            break;
        }

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        dinfoSetDefault(&f->dInfo);

        s->Fsource = select_open_filter_new(filepath, (IteFlower *)&f);
        CHECK_(s->Fsource != NULL);

#if CFG_MIXENABLE
        resample_filter_new(&s, (IteFlower *)f); // resample & channel adapt
#endif

#if CFG_FADE_FILTER
        s->Ftee = Flow_filter_new(ITE_FILTER_FADE_ID, &f);
        CHECK_(s->Ftee != NULL);
#endif

        s->Fmix = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fmix != NULL);

        s->Fsndwrite = Flow_filter_new(ITE_FILTER_DAWRITE_ID, &f);
        CHECK_(s->Fsndwrite != NULL);

        ite_filter_call_method(s->Fsource, ITE_FILTER_FILEOPEN,
                               (void *)filepath);
        ite_filter_call_method(s->Fsource, ITE_FILTER_LOOPPLAY, &mode);
        
        ite_filter_call_method(s->Fsndwrite, ITE_FILTER_SET_CB, cb);

        ite_filter_call_method(s->Fmix, ITE_FILTER_SET_VALUE, NULL);

        Flow_init_i2s(f); // init i2s

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
#if CFG_MIXENABLE
        if (f->mixInfo.init)
        {
            resample_filter_ctrl_link(&s, true);
        }
#endif
        if (s->Ftee != NULL)
        {
            ite_filterChain_link(&s->fc, 0, s->Ftee, 0);
        }
        ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
        ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

        ite_filterChain_run(&s->fc);

        f->audiocase   = AudioPlayFile;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void PlayFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioPlayFile)
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
    if (s->Ftee)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Ftee, 0);
    }
    ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
    if(f->cb_func) 
    {
        f->cb_func(Eofclose,NULL);
        f->cb_func=NULL;
    }
}

/*get have played total time*/
void PlayFlowGetTime (IteFlower * f, uint32_t * timestamp)
{
    // IteAudioFlower *s=f->audiostream;
    *timestamp = f->dInfo.currentms;
}

/*seek to +sec play*/
void PlayFlowSeekPos (IteFlower * f, int sec)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_PCMGRAB_ID ||
        s->Fsource->filterDes.id == ITE_FILTER_M4AGRAB_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_SEEK_POS, &sec);
    }
    else
    {
        printf("only support .wav .m4a!!\n");
    }
}

/*shift +sec/-sec play*/
void PlayFlowShiftPos (IteFlower * f, int sec)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_PCMGRAB_ID ||
        s->Fsource->filterDes.id == ITE_FILTER_M4AGRAB_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_SHIFT_POS, &sec);
    }
    else
    {
        printf("only support .wav .m4a!!\n");
    }
}

/*get current play time (ms)*/
void PlayFlowGetPos (IteFlower * f, int * timestamp)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_PCMGRAB_ID ||
        s->Fsource->filterDes.id == ITE_FILTER_M4AGRAB_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_GET_PARAM, timestamp);
    }
    else
    {
        printf("only support .wav .m4a!!\n");
    }
}

/*change file*/
void PlayFlowOpenFile (IteFlower * f, const char * filepath)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsource->filterDes.id >= ITE_FILTER_PCMGRAB_ID &&
        s->Fsource->filterDes.id <= ITE_FILTER_M4AGRAB_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_FILEOPEN,
                               (void *)filepath);
    }
    else
    {
        printf("only support %s\n", filepath);
    }
}

void PlayFlowSetReleaseMode (IteFlower * f, bool release)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsndwrite->filterDes.id == ITE_FILTER_DAWRITE_ID)
    {
        ite_filter_call_method(s->Fsndwrite, ITE_FILTER_SET_PARAM, &release);
    }
    else
    {
        printf("error s->Fsource->filterDes.id=%d != \n",
               s->Fsource->filterDes.id);
    }
}

/*play sound end*/

/*record sound start*/
void RecFlowStart (IteFlower * f, const char * filename, int rate, int channel)
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

        s->Fdestinat = Flow_filter_new(ITE_FILTER_FILEREC_ID, &f);
        CHECK_(s->Fdestinat != NULL);

        adread_reinit_for_diff_rate(rate, 16, channel);

        ite_filter_call_method(s->Fdestinat, ITE_FILTER_FILEOPEN,
                               (void *)filename);
        ite_filter_call_method(s->Fdestinat, ITE_FILTER_SETRATE, &rate);
        ite_filter_call_method(s->Fdestinat, ITE_FILTER_SET_NCHANNELS,
                               &channel);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);
        f->audiocase   = AudioRecFile;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }
}

void RecFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioRecFile)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
}

/*record sound end*/

/*sound RW start*/
void SndrwFlowStart (IteFlower * f, int rate, AudioCodecType type)
{
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

        s->Fsndwrite = Flow_filter_new(ITE_FILTER_DAWRITE_ID, &f);
        CHECK_(s->Fsndwrite != NULL);

        adread_reinit_for_diff_rate(rate, 16, 1);
        dawrite_reinit_for_diff_rate(rate, 16, 1);
        // Castor3snd_reinit_for_diff_rate(rate,16,1);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
        ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

        // ite_filterChain_print(&s->fc);

        ite_filterChain_run(&s->fc);

        f->audiocase   = AnalogLocalBroadcast;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }
}

void SndrwFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AnalogLocalBroadcast)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
}
/*sound RW end*/

/*stream play*/

IteFilter * select_codec_filter_new (int type, IteFlower * f)
{

    IteFilter * Fp = NULL;
    switch (type)
    {
        /*case ITE_WAV:
        Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID,(IteFlower**)f);
        ite_filter_call_method(Fp,ITE_FILTER_FILEOPEN ,(void*)"dummy.wav");
        break;*/
        case ITE_MP3:
#ifdef CFG_BUILD_MP3DEC
            Fp = Flow_filter_new(ITE_FILTER_CODECMP3_ID, (IteFlower **)f);
#else
            Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID, (IteFlower **)f);
#endif
            ite_filter_call_method(Fp, ITE_FILTER_FILEOPEN,
                                   (void *)"dummy.mp3");
            break;
        case ITE_AAC:
#ifdef CFG_BUILD_AACDEC
            Fp = Flow_filter_new(ITE_FILTER_CODECAAC_ID, (IteFlower **)f);
#else
            Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID, (IteFlower **)f);
#endif
            ite_filter_call_method(Fp, ITE_FILTER_FILEOPEN,
                                   (void *)"dummy.aac");
            break;
        default:
            break;
    }
    return Fp;
}

void PlayStreamFlowStart (IteFlower * f, DataInfo * di)
{
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };
    int chipsize = 1024;

    do
    {
        dinfoSetDefault(&f->dInfo);

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_QREAD_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fdecoder = select_codec_filter_new(di->codectype, (IteFlower *)&f);

        s->Fmix     = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fmix != NULL);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_DAWRITE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

        ite_filter_call_method(s->Fsource, ITE_FILTER_SET_VALUE, &chipsize);
        if (s->Fdecoder == NULL)
        {
            // raw data stream init i2s
            dawrite_reinit_for_diff_rate(di->sr, di->bitsize, di->nch);
            dinfoSetValue(&f->dInfo, di);
        }
        ite_filter_call_method(s->Fmix, ITE_FILTER_SET_VALUE, NULL);

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        if (s->Fdecoder != NULL)
        {
            ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
        }
        ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);
        f->audiocase   = StreamPlay;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }
}

void PlayStreamFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != StreamPlay)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if (s->Fdecoder)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdecoder, 0);
    }
    ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);

    f->audiocase = AudioIdle;
    audio_flower_free(s);
    f->audiostream = NULL;
}
