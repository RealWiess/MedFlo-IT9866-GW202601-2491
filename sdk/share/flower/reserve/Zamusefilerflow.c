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

/*play sound start*/
int PlayerFlowStart (IteFlower * f, const char * filepath, PlaySoundCase mode,
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
            (void)printf("%s is not exist! PlayerFlowStart not do\n", filepath);
            result = -1;
            break;
        }
        if (f->audiostream != NULL)
        {
            (void)printf("audiostream is exit!! change another \n");
            result = -1;
            break;
        }

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        dinfoSetDefault(&f->dInfo);

        s->Fsource = select_open_filter_new(filepath, (IteFlower *)&f);
        CHECK_(s->Fsource != NULL);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

#if CFG_MIXENABLE
        resample_filter_new(&s, f); // resample & channel adapt
#endif

#if CFG_FADE_FILTER
        s->Ftee = Flow_filter_new(ITE_FILTER_FADE_ID, &f);
        CHECK_(s->Ftee != NULL);
#endif

        ite_filter_call_method(s->Fsource, ITE_FILTER_FILEOPEN,
                               (void *)filepath);
        ite_filter_call_method(s->Fsource, ITE_FILTER_LOOPPLAY, &mode);
        
        ite_filter_call_method(s->Fdestinat, ITE_FILTER_SET_CB, cb);

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
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);

        f->audiocase   = AudioMixerFile;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void PlayerFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioMixerFile)
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
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);
    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
    if(f->cb_func) 
    {
        f->cb_func(PLAY_EOF_FILE,NULL);
    }
}

/*for Qread, raw data packet. ex:a2dp, hfp,  start*/
void PacketFlowStart (IteFlower * f, int in_rate, int in_chan)
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
        printf("input rate :%d \n", in_rate);

        dinfoSetDefault(&f->dInfo);
        f->dInfo.sr        = in_rate;
        f->dInfo.nch       = in_chan;
        f->dInfo.bytes20ms = 20 * in_rate * in_chan * 2 / 1000;
        f->dInfo.init      = true;

        s                  = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_QREAD_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

#if CFG_MIXENABLE
        resample_filter_new(&s, f); // resample & channel adapt
#endif

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
#if CFG_MIXENABLE
        if (f->mixInfo.init == true)
        {
            resample_filter_ctrl_link(&s, true);
        }
#endif
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);

        f->audiocase   = AudioMixerPacket;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }
}

void PacketFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioMixerPacket)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
#if CFG_MIXENABLE
    if (f->mixInfo.init == true)
    {
        resample_filter_ctrl_link(&s, false);
    }
#endif
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);
    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
}

/*seek to +sec play*/
void PlayerFlowSeekPos (IteFlower * f, int sec)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_PCMGRAB_ID ||
        s->Fsource->filterDes.id == ITE_FILTER_DEM4A_ID ||
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
void PlayerFlowShiftPos (IteFlower * f, int sec)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_PCMGRAB_ID ||
        s->Fsource->filterDes.id == ITE_FILTER_DEM4A_ID ||
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
void PlayerFlowGetPos (IteFlower * f, int * timestamp)
{
    IteAudioFlower * s = f->audiostream;
    if (!s)
    {
        printf("Fsource filter not exit\n");
        return;
    }
    if (s->Fsource->filterDes.id == ITE_FILTER_PCMGRAB_ID ||
        s->Fsource->filterDes.id == ITE_FILTER_DEM4A_ID ||
        s->Fsource->filterDes.id == ITE_FILTER_M4AGRAB_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_GET_PARAM, timestamp);
    }
    else
    {
        printf("only support .wav .m4a!!\n");
    }
}
/*play sound end*/

/*I2SFlowStart start*/
int I2SFlowStart (IteFlower * f, cb_sound_t cb)
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
            printf("audiostream is exit!! change another\n");
            result = -1;
            break;
        }

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_MULTIMIX_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fsndwrite = Flow_filter_new(ITE_FILTER_DAWRITE_ID, &f);
        CHECK_(s->Fsndwrite != NULL);

        if (cb != NULL)
        {
            f->cb_func = cb;
        }

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

        ite_filterChain_run(&s->fc);

        f->audiocase   = AudioMixer;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void I2SFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioMixer)
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
    if(f->cb_func) 
    {
        f->cb_func(Eofclose,NULL);
        f->cb_func=NULL;
    }
}
/*I2SFlowStart end*/

IteFilter * select_url_filter_new (const char * url, IteFlower * p,
                                   IteFlower * arg)
{
    int         type = audiomgrExtensionName((char *)url);
    IteFilter * Fp   = NULL;
    if (arg->userp == (void *)ITE_TTS)
    {
        type = ITE_TTS;
    }
    switch (type)
    {
        case ITE_MP3:
            Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID,
                                 (IteFlower **)p); // decodec on CPU 2
            ite_filter_call_method(Fp, ITE_FILTER_FILEOPEN, (void *)url);
            break;
        case ITE_TTS:
#ifdef CFG_BUILD_MP3DEC
            Fp = Flow_filter_new(ITE_FILTER_CODECMP3_ID,
                                 (IteFlower **)p); // decodec on CPU 1
#else
            Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID,
                                 (IteFlower **)p); // decodec on CPU 2
#endif
            ite_filter_call_method(Fp, ITE_FILTER_FILEOPEN, (void *)url);
            break;
        case ITE_FLAC:
            Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID,
                                 (IteFlower **)p); // decodec on CPU 2
            ite_filter_call_method(Fp, ITE_FILTER_FILEOPEN, (void *)url);
            break;
        case ITE_VORBIS:
            Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID,
                                 (IteFlower **)p); // decodec on CPU 2
            ite_filter_call_method(Fp, ITE_FILTER_FILEOPEN, (void *)url);
            break;
        case ITE_OPUS:
            Fp = Flow_filter_new(ITE_FILTER_CODECMGR_ID,
                                 (IteFlower **)p); // decodec on CPU 2
            ite_filter_call_method(Fp, ITE_FILTER_FILEOPEN, (void *)url);
            break;
        case ITE_M4A:
        default:
#ifdef CFG_BUILD_AACDEC
            Fp = Flow_filter_new(ITE_FILTER_DEM4A_ID, (IteFlower **)p); //(CPU2)
#else
            Fp = Flow_filter_new(ITE_FILTER_M4AGRAB_ID,
                                 (IteFlower **)p); //(CPU1)
#endif
            ite_filter_call_method(Fp, ITE_FILTER_SET_FILEPATH, NULL);
            break;
    }
    return Fp;
}

/*CurlFlowStart start*/
int CurlFlowStart (IteFlower * f, const char * url, cb_sound_t cb)
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
            printf("audiostream is exit!! change another\n");
            result = -1;
            break;
        }

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        dinfoSetDefault(&f->dInfo);

        s->Fsource = Flow_filter_new(ITE_FILTER_CURL_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fdecoder  = select_url_filter_new(url, (IteFlower *)&f, f);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

#if CFG_MIXENABLE
        resample_filter_new(&s, f); // resample & channel adapt
#endif

        ite_filter_call_method(s->Fsource, ITE_FILTER_FILEOPEN, (void *)url);

        if (cb != NULL)
        {
            f->cb_func = cb;
        }

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        if (s->Fdecoder != NULL)
        {
            ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
        }
        if (s->Fencoder != NULL)
        {
            ite_filterChain_link(&s->fc, 0, s->Fencoder, 0);
        }
#if CFG_MIXENABLE
        if (f->mixInfo.init)
        {
            resample_filter_ctrl_link(&s, true);
        }
#endif
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);

        f->audiocase   = AudioNetCurl;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void CurlFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioNetCurl)
    {
        return;
    }

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if (s->Fdecoder)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdecoder, 0);
    }
    if (s->Fencoder)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fencoder, 0);
    }
#if CFG_MIXENABLE
    if (f->mixInfo.init)
    {
        resample_filter_ctrl_link(&s, false);
    }
#endif
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
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

int CurlMp3FlowStart (IteFlower * f, const char * url, cb_sound_t cb)
{
    int ret  = 0;
    f->userp = (void *)ITE_TTS;
    ret      = CurlFlowStart(f, url, cb);
    f->userp = 0;
    return ret;
}

void CurlMp3FlowStop (IteFlower * f)
{
    CurlFlowStop(f);
}

/*CurlFlowStop end*/

/*FFmpegFlowStart start*/
int FFmpegFlowStart (IteFlower * f, const char * filepath, cb_sound_t cb)
{
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };
#ifdef CFG_BUILD_AACDEC
    int type = audiomgrExtensionName((char *)filepath);
#endif

    do
    {
        if (f->audiostream != NULL)
        {
            printf("audiostream is exit!! change another\n");
            result = -1;
            break;
        }

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        dinfoSetDefault(&f->dInfo);

        s->Fsource = Flow_filter_new(ITE_FILTER_FFMPEGGRAB_ID, &f);
        CHECK_(s->Fsource != NULL);

#ifdef CFG_BUILD_AACDEC
        if (type != ITE_MP3)
        {
            s->Fdecoder = Flow_filter_new(ITE_FILTER_CODECAAC_ID, &f);
            CHECK_(s->Fdecoder != NULL);
        }
#endif

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

#if CFG_MIXENABLE
        resample_filter_new(&s, f); // resample & channel adapt
#endif

        ite_filter_call_method(s->Fsource, ITE_FILTER_FILEOPEN,
                               (void *)filepath);

        if (cb != NULL)
        {
            f->cb_func = cb;
        }

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        if (s->Fdecoder != NULL)
        {
            ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
        }
#if CFG_MIXENABLE
        if (f->mixInfo.init)
        {
            resample_filter_ctrl_link(&s, true);
        }
#endif
        ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

        ite_filterChain_run(&s->fc);
        f->audiocase   = AudioFFmpegPlay;
        f->audiostream = s;
    } while (false);

    if (result != 0 && s != NULL)
    {
        audio_flower_free(s);
    }

    return result;
}

void FFmpegFlowStop (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    if (f->audiocase != AudioFFmpegPlay)
    {
        return;
    }
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if (s->Fdecoder)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdecoder, 0);
    }
#if CFG_MIXENABLE
    if (f->mixInfo.init)
    {
        resample_filter_ctrl_link(&s, false);
    }
#endif
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);
    audio_flower_free(s);
    f->audiocase   = AudioIdle;
    f->audiostream = NULL;
    if(f->cb_func) 
    {
        f->cb_func(Eofclose,NULL);
    }
}
/*FFmpegFlowStart end*/

int M3u8FlowStart (IteFlower * f, const char * url, cb_sound_t cb)
{
    return FFmpegFlowStart(f, url, cb); /*M3u8 start use ffmpeg*/
}

void M3u8FlowStop (IteFlower * f)
{
    FFmpegFlowStop(f); /*M3u8 stop*/
}

int CurlM4aFlowStart (IteFlower * f, const char * url, cb_sound_t cb)
{
    return FFmpegFlowStart(f, url, cb); /*M4a start use ffmpeg*/
}

void CurlM4aFlowStop (IteFlower * f)
{
    FFmpegFlowStop(f); /*M4a stop*/
}

int Mp3FlowStart (IteFlower * f, const char * url, cb_sound_t cb)
{
    return FFmpegFlowStart(f, url, cb); /*Mp3 start use ffmpeg*/
}

void Mp3FlowStop (IteFlower * f)
{
    FFmpegFlowStop(f); /*Mp3 stop*/
}

void StreamerFlowStart (IteFlower * f, DataInfo * di)
{
    int              result = 0;
    IteAudioFlower * s      = NULL;
    char *           args[] = {
        "-S",
        "-Q=32",
    };

    do
    {
        dinfoSetDefault(&f->dInfo);

        s = (IteAudioFlower *)ite_new0(IteAudioFlower, 1);
        CHECK_(s != NULL);

        s->Fsource = Flow_filter_new(ITE_FILTER_QREAD_ID, &f);
        CHECK_(s->Fsource != NULL);

        s->Fdecoder  = select_codec_filter_new(di->codectype, (IteFlower *)&f);

        s->Fdestinat = Flow_filter_new(ITE_FILTER_QLAYE_ID, &f);
        CHECK_(s->Fdestinat != NULL);

#if CFG_MIXENABLE
        resample_filter_new(&s, f); // resample & channel adapt
#endif

        if (s->Fdecoder == NULL)
        {
            // raw data stream init i2s
            #ifndef CFG_MIXENABLE
            dawrite_reinit_for_diff_rate(di->sr, di->bitsize, di->nch);
            #endif
            dinfoSetValue(&f->dInfo, di);
        }

        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
        if (s->Fdecoder != NULL)
        {
            ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
        }
#if CFG_MIXENABLE
        if (f->mixInfo.init)
        {
            resample_filter_ctrl_link(&s, true);
        }
#endif
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

void StreamerFlowStop (IteFlower * f)
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
#if CFG_MIXENABLE
    if (f->mixInfo.init)
    {
        resample_filter_ctrl_link(&s, false);
    }
#endif
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);

    f->audiocase = AudioIdle;
    audio_flower_free(s);
    f->audiostream = NULL;
}
