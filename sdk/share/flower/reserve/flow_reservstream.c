#include <malloc.h>
#include <string.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"

pthread_mutex_t   StreamFlow_mutex = PTHREAD_MUTEX_INITIALIZER;
static Callback_t I2s_cb           = NULL;
IteFlower *       fout;

void              setI2Scallback (Callback_t cb)
{
    I2s_cb = cb;
}

void FlowPlayerCancel (IteFlower * f)
{ // stop player
    if (f->audiostream != NULL)
    {
        PlayerFlowStop(f);
        PacketFlowStop(f);
        CurlM4aFlowStop(f);
        CurlFlowStop(f);
        FFmpegFlowStop(f);
        StreamerFlowStop(f);
#ifdef CFG_AV_MIX_FILTER
        AVPlayerFlowStop(f);
#endif
        SoundFlowStop(f);
        if (FlowQ_Check_MixIdle(f))
        {
            RelayFlush(f);
        }
        if (f->audiocase != AudioIdle)
        {
            printf("Warning !!! FlowPlayerCancel not close fully %d\n",
                   f->audiocase);
        }
    }
}

void FlowPlayerFlush (IteFlower * f)
{                              // stop player & flush link
    FlowQ_A_unlink_B(f, fout); // unlink play & i2sout
    FlowPlayerCancel(f);
    flowQflush(f);
}

void FlowI2SCancel (IteFlower * f)
{ // stop i2s
    if (f->audiostream != NULL)
    {
        I2SFlowStop(f);
        if (f->audiocase != AudioIdle)
        {
            printf("Warning !!! FlowI2SCancel not close fully\n");
        }
    }
}

void FlowI2SCheckIdel (IteFlower * f)
{
    int timeout = 1000;
    while (f->audiocase != AudioIdle)
    {
        usleep(10000);
        timeout--;
        if (timeout == 0)
        {
            printf("audiocase=%d timeout, force Cancel\n", f->audiocase);
            FlowI2SCancel(f);
        }
    }
}

void autostop_FlowI2SCancel (IteFlower * f)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void FlowI2SSetDucking (IteFlower * f)
{
    int              duckpin = -1;
    int              Inpin[2];
    int              fpinID;
    IteAudioFlower * s = fout->audiostream;
    if (fout->audiostream == NULL)
    {
        return;
    }
    if (f != NULL)
    {
        fpinID = (int)f->userp;
        FlowQ_Get_Link_pin(fout, (int *)&Inpin);
        if (fpinID == Inpin[0])
        {
            duckpin = 0;
        }
        if (fpinID == Inpin[1])
        {
            duckpin = 1;
        }
        printf("fpinID=%d Inpin[0]=%d Inpin[1]=%d duckpin=%d\n", fpinID,
               Inpin[0], Inpin[1], duckpin);
    }
    if (fout->audiocase == AudioMixer)
    {
        if (s->Fsource->filterDes.id == ITE_FILTER_MULTIMIX_ID)
        {
            ite_filter_call_method(s->Fsource, ITE_FILTER_SET_PARAM, &duckpin);
        }
    }
}

/*==========================================================================*/

void flowstream_start_play (IteFlower * f, const char * filepath,
                            PlaySoundCase mode, Callback_t func)
{

    pthread_mutex_lock(&StreamFlow_mutex);
    // FlowI2SCancel(fout);

    FlowPlayerFlush(f);
    PlayerFlowStart(f, filepath, mode, func);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 0, fout, 0);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_stop_play (IteFlower * f)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    FlowPlayerFlush(f);
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }

    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_mix_sound (IteFlower * f, const char * filepath,
                           PlaySoundCase mode, Callback_t func)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    FlowPlayerFlush(f);
    PlayerFlowStart(f, filepath, mode, func);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 1, fout, 1);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

#ifdef CFG_AV_MIX_FILTER
/*for video sound mix*/
void flowstream_start_AVplay (IteFlower * f)
{
    // FlowI2SCheckIdel(fout);//wait time for i2s flow idel
    pthread_mutex_lock(&StreamFlow_mutex);
    AVPlayerFlowStart(f);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 0, fout, 0);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_stop_AVplay (IteFlower * f)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    FlowPlayerFlush(f);
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}
#endif
/*end*/

void flowstream_start_ffmpegplay (IteFlower * f, const char * filepath,
                                  Callback_t func)
{

    pthread_mutex_lock(&StreamFlow_mutex);
    FlowPlayerFlush(f);
    FFmpegFlowStart(f, filepath, func);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 0, fout, 0);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_stop_ffmpegplay (IteFlower * f)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    FlowPlayerFlush(f);
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void link_callback0 (void * arg)
{
    IteFlower * f = (IteFlower *)arg;
    pthread_mutex_lock(&StreamFlow_mutex);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 0, fout, 0);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void link_callback1 (void * arg)
{
    IteFlower * f = (IteFlower *)arg;
    pthread_mutex_lock(&StreamFlow_mutex);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 1, fout, 1);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_start_memory_play (IteFlower * f, uint8_t * data, size_t dlen,
                                   AudioCodecType codec)
{
    DataInfo di;
    di.codectype = codec; // set codec type :mp3 aac wav
    di.init      = true;
    pthread_mutex_lock(&StreamFlow_mutex);
    FlowPlayerFlush(f);
    StreamerFlowStart(f, &di);
    if (data != NULL)
    {
        QReadFilterPutDataPtr(f, data, dlen); // put data to playstream
        QReadFilterPutDataPtr(f, data, 0);    // put eof zero
    }
    pthread_mutex_unlock(&StreamFlow_mutex);

    switch ((int)f->userp)
    {
        case 0:
            f->cb_link = link_callback0;
            break;
        case 1:
            f->cb_link = link_callback1;
            break;
        default:
            f->cb_link = link_callback0;
            break;
    }
}

void flowstream_stop_memory_play (IteFlower * f)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    FlowPlayerFlush(f);
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_status_set (IteFlower * f, IteFilterStatus status)
{
    if (Flow_status_get(f) == FILTER_ERR || Flow_status_get(f) == status)
    {
        return;
    }
    Flow_status_set(f, status);
    switch (status)
    {
        case FILTER_RESUME:
            if (FlowQ_link_precheck(f, fout, -1))
            {
                int mixpin = FlowQ_Get_Idle_mixpin(fout);
                FlowQ_A_link_B(f, (int)f->userp, fout, mixpin);
            }
            break;
        case FILTER_PAUSE:
            FlowQ_A_unlink_B(f, fout);
            break;
        default:
            break;
    }
}

int flowstream_check_unmix (void)
{
    int pin[2];
    FlowQ_Get_Link_pin(fout, &pin[0]);
    if (pin[0] == -1 && pin[1] == -1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
