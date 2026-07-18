#include <malloc.h>
#include <string.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"

#ifdef CFG_RESERVE_FILTER
int FlowQ_link_precheck (IteFlower * fsrc, IteFlower * fdist, int sec)
{
    int ret     = 1;
    int timeout = 0;
    int wait    = 50000;
    // int enter=1;
    // IteAudioFlower *s =fsrc->audiostream;

    while (!fsrc->dInfo.init)
    {
        usleep(wait);
        // printf("wait %d\n",fsrc->audiocase);
        if (fsrc->audiocase == AudioIdle && !fsrc->fQ.flagEof)
        {
            printf("%s player AudioIdle :fail link\n", __FUNCTION__);
            ret = 0;
            break;
        }
        if (sec > 0)
        {
            timeout += wait;
            if (timeout >= sec * 1000 * 1000)
            {
                printf("%s timeout=%d :fail link\n", __FUNCTION__, timeout);
                ret = 0;
                break;
            }
        }
    }
    if (fdist->audiocase == AudioIdle && ret)
    {
        I2SFlowStart(fdist, NULL);
    }
    return ret; // success init
}

int FlowQ_A_link_B (IteFlower * fsrc, int pin, IteFlower * fdist, int pout)
{
    if (fsrc && fdist)
    {
        IteAudioFlower * s = fdist->audiostream;
        if (s && s->Fsource->filterDes.id == ITE_FILTER_MULTIMIX_ID)
        {
            if (!fsrc->dInfo.init)
            {
                printf("%s : fsrc->dInfo.init=false link error", __FUNCTION__);
                return 0;
            }
            fsrc->userp = (void *)pout; // mix pin for filter select
            switch (pin)
            {
                case 0:
                    ite_filter_call_method(s->Fsource, ITE_FILTER_A_Method,
                                           &fsrc);
                    break;
                case 1:
                    ite_filter_call_method(s->Fsource, ITE_FILTER_B_Method,
                                           &fsrc);
                    break;
                case 2:
                    ite_filter_call_method(s->Fsource, ITE_FILTER_C_Method,
                                           &fsrc);
                    break;
                case 3:
                    ite_filter_call_method(s->Fsource, ITE_FILTER_D_Method,
                                           &fsrc);
                    break;
                default:
                    printf("error pin:%d\n", pin);
                    break;
            }
            fsrc->userp = (void *)pin; // fsrc input pin record(player id
                                       // record)
            Flow_status_set(fdist, FILTER_RESUME);
            return 1;
        }
    }
    return 0;
}

int FlowQ_A_unlink_B (IteFlower * fsrc, IteFlower * fdist)
{
    if (fsrc && fdist)
    {
        IteAudioFlower * s = fdist->audiostream;
        if (s && s->Fsource->filterDes.id == ITE_FILTER_MULTIMIX_ID && fsrc &&
            fdist)
        {
            ite_filter_call_method(s->Fsource, ITE_FILTER_F_Method, &fsrc);
        }
    }
    return 0;
}

/*get mix pin value*/
void FlowQ_Get_Link_pin (IteFlower * fdist, int * array)
{
    IteAudioFlower * s = fdist->audiostream;
    if (s && s->Fsource->filterDes.id == ITE_FILTER_MULTIMIX_ID)
    {
        ite_filter_call_method(s->Fsource, ITE_FILTER_GET_VALUE, array);
    }
    else
    {
        *array       = -1;
        *(array + 1) = -1;
    }
}

/*find empty mix pin*/
int FlowQ_Get_Idle_mixpin (IteFlower * fdist)
{
    int pin[2];
    int result = -1;
    FlowQ_Get_Link_pin(fdist, &pin[0]);
    for (int i = 0; i < 2; i++)
    {
        if (pin[i] == -1)
        {
            result = i;
            break;
        }
    }
    if (result == -1)
    {
        printf("mixpin are full\n");
    }
    return result;
}

bool FlowQ_Check_MixIdle (IteFlower * fdist)
{
    int pin[2];
    FlowQ_Get_Link_pin(fdist, (int *)&pin);
    if (pin[0] == -1 && pin[1] == -1)
    {
        return true; // no mix
    }
    else
    {
        return false; // at least one pin be linked
    }
}
#endif

int FlowQ_swdB_ctrl (IteFlower * fsrc, int dB)
{
    fsrc->dInfo.sw_dB = dB;
    return dB;
}

void Flow_status_set (IteFlower * f, IteFilterStatus status)
{
    IteAudioFlower * s = f->audiostream;
    if (s == NULL)
    {
        printf("%s stream is NULL %d\n", __FUNCTION__, __LINE__);
        return;
    }
    ite_filterChain_setStatus(&s->fc, status);
}

IteFilterStatus Flow_status_get (IteFlower * f)
{
    IteAudioFlower * s = f->audiostream;
    IteFilterStatus  status;
    if (s == NULL)
    {
        printf("%s stream is NULL %d\n", __FUNCTION__, __LINE__);
        return FILTER_ERR;
    }
    status = ite_filterChain_getStatus(&s->fc);
    /*switch(status){
        case FILTER_ERR   : printf("FILTER_ERR\n");break;
        case FILTER_NONE  : printf("FILTER_NONE\n");break;
        case FILTER_RESUME: printf("FILTER_RESUME\n");break;
        case FILTER_PAUSE : printf("FILTER_PAUSE\n");break;
        case FILTER_FLUSH : printf("FILTER_FLUSH\n");break;
    }*/
    return status;
}

void Flow_init_i2s (IteFlower * f)
{
    while (f->mixInfo.init == false && f->dInfo.init == false)
    {
        usleep(10000);
        printf("wait mixInfo.init = %d dInfo.init = %d\n", f->mixInfo.init,
               f->dInfo.init);
    }
    if (f->mixInfo.init == true)
    {
        if (dawrite_reinit_for_diff_rate(f->mixInfo.sr, f->mixInfo.bitsize,
                                         f->mixInfo.nch))
        {
            printf("resample i2s sr:%d ch:%d bitsize:%d\n", f->mixInfo.sr,
                   f->mixInfo.nch, f->mixInfo.bitsize);
        }
    }
    else
    {
        if (dawrite_reinit_for_diff_rate(f->dInfo.sr, f->dInfo.bitsize,
                                         f->dInfo.nch))
        {
            printf("reinit i2s sr:%d ch:%d bitsize:%d\n", f->dInfo.sr,
                   f->dInfo.nch, f->dInfo.bitsize);
        }
    }
}
