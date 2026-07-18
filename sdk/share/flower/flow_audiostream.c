#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"

pthread_mutex_t AudioFlow_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef CFG_RESERVE_FILTER
extern pthread_mutex_t StreamFlow_mutex;
#endif
static Callback_t asr_cb = NULL;

static void       flow_filter_callbackfunc (int state, void * arg)
{

    if (asr_cb)
    {
        asr_cb(state, arg);
    }
}

void AudioStreamCheckIdel (IteFlower * f)
{
    int timeout = 1000;
    while (f->audiocase != AudioIdle)
    {
        usleep(10000);
        timeout--;
        if (timeout == 0)
        {
            printf("audiocase=%d timeout, force Cancel\n", f->audiocase);
            AudioStreamCancel(f);
        }
    }
}

void AudioStreamCancel (IteFlower * f)
{
    // check audiostream be NULL;
    if (f->audiostream != NULL)
    {
        PlayFlowStop(f);
        RecFlowStop(f);
        SndrwFlowStop(f);
#ifdef CFG_BUILD_ASR
        ResmplePlayFlowStop(f);
#endif

#ifdef CFG_RESERVE_FILTER
        FlowPlayerFlush(f);
        RelayFlowStop(f);
#endif
        PlayStreamFlowStop(f);
        if (f->audiocase != AudioIdle)
        {
            printf("Warning !!! audio stream not close fully\n");
        }
    }
}

void * autostop_task (IteFlower * f)
{
    if (f->audiocase == AudioMixer)
    {
#ifdef CFG_RESERVE_FILTER
        autostop_FlowI2SCancel(f);
#endif
    }
    else
    {
#ifdef CFG_RESERVE_FILTER
        pthread_mutex_lock(&StreamFlow_mutex);
        AudioStreamCancel(f);
        pthread_mutex_unlock(&StreamFlow_mutex);
#else
        pthread_mutex_lock(&AudioFlow_mutex);
        AudioStreamCancel(f);
        pthread_mutex_unlock(&AudioFlow_mutex);
#endif
    }
    return 0;
}

/*static void *asrevent_task(IteFlower *f, void *arg){
    flow_filter_callbackfunc(ASR_SUCCESS_ARG,arg);
    return NULL;
}*/

void CreateDetachedThread (void * func, void * arg)
{
    pthread_t      Thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32 * 1024);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&Thread, &attr, func, arg);
}

void sound_playback_event_handler (PlaySoundCase nEventID, void * arg)
{
    switch (nEventID)
    {
        case Asrevent:
            printf("asr event\n");
            flow_filter_callbackfunc(ASR_SUCCESS_ARG, arg);
            break;
        default:
            break;
    }
}

/*play api*/
void flow_start_sound_play (IteFlower * f, const char * filepath,
                            PlaySoundCase mode, Callback_t func)
{
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    PlayFlowStart(f, filepath, mode, func);
    pthread_mutex_unlock(&AudioFlow_mutex);
}

void flow_start_resample_play (IteFlower * f, const char * filepath,
                               PlaySoundCase mode, Callback_t func, int rate,
                               int ch)
{
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    ResmplePlayFlowStart(f, filepath, mode, func, rate, ch);
    pthread_mutex_unlock(&AudioFlow_mutex);
}

void flow_stop_sound_play (IteFlower * f)
{
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
}

void flow_start_memory_play (IteFlower * f, uint8_t * data, size_t dlen,
                             AudioCodecType codec)
{
    DataInfo di;
    dinfoSetDefault(&di);
    di.codectype = codec; // set codec type :mp3 aac wav
    di.init      = true;
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    PlayStreamFlowStart(f, &di);
    if (data != NULL)
    {
        QReadFilterPutDataPtr(f, data, dlen); // put data to playstream
        QReadFilterPutDataPtr(f, data, 0);    // put eof zero
    }
    pthread_mutex_unlock(&AudioFlow_mutex);
}

void flow_stop_memory_play (IteFlower * f)
{
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
}

void flow_start_AVsound_play (IteFlower * f)
{
    flowstream_start_AVplay(f);
}

void flow_stop_AVsound_play (IteFlower * f)
{
    flowstream_stop_AVplay(f);
}

void flow_start_mix_sound (IteFlower * f, const char * filepath,
                           PlaySoundCase mode, Callback_t func)
{
    printf("nomore use %s %d\n", __FUNCTION__, __LINE__);
}

/*player end*/

/*record api*/
void flow_start_sound_record (IteFlower * f, const char * filepath, int rate,
                              int channel)
{

    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);

    RecFlowStart(f, filepath, rate, channel);
}

void flow_stop_sound_record (IteFlower * f)
{
    AudioStreamCancel(f);
}

void flow_stop_audioflow (IteFlower * f)
{
    AudioStreamCancel(f);
}
/*record end*/

/*asr api*/
void flow_stop_asr (IteFlower * f)
{

    if (f->asrstream != NULL)
    {
        AsrFlowStop(f);
        asr_cb       = NULL;
        f->asrstream = NULL;
    }
}

void flow_start_asr (IteFlower * f, Callback_t func)
{

    if (f->asrstream != NULL)
    {
        flow_stop_asr(f);
    }
    asr_cb = func;
    AsrFlowStart(f, sound_playback_event_handler);
}
/*asr end*/

/*mic & spk work*/
void flow_start_soundrw (IteFlower * f)
{

    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);

    SndrwFlowStart(f, 8000, ULAW);
}

void flow_stop_soundrw (IteFlower * f)
{
    AudioStreamCancel(f);
}
/*end*/

int flow_start_udp_audiostream (IteFlower * f, int rate, AudioCodecType type,
                                const char * rem_ip, unsigned short rem_port)
{
    printf("%s not support now\n", __FUNCTION__);
    return 0;
}

/*start audio tcp stream*/
int flow_start_tcp_audiostream (IteFlower * f, int rate, AudioCodecType type,
                                const char * rem_ip, unsigned short rem_port)
{
    printf("%s not support now\n", __FUNCTION__);
    return 0;
}

/*stop audio udp stream*/
int flow_stop_udp_audiostream (IteFlower * f)
{
    printf("%s not support now\n", __FUNCTION__);
    return 0;
}

/*stop audio tcp stream*/
int flow_stop_tcp_audiostream (IteFlower * f)
{
    printf("%s not support now\n", __FUNCTION__);
    return 0;
}
