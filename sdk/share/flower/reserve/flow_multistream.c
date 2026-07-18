#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"

IteFlower *            relay0 = NULL;
IteFlower *            relay1 = NULL;
extern pthread_mutex_t StreamFlow_mutex;
extern IteFlower *     fout;

static void            relay0link0 (void * arg)
{
    IteFlower * f   = (IteFlower *)arg;
    relay0->cb_link = link_callback0;
    pthread_mutex_lock(&StreamFlow_mutex);
    if (relay0->audiocase == AudioIdle)
    {
        RelayFlowStart(relay0, NULL);
    }
    if (relay0->audiocase == AudioRelay)
    {
        FlowQ_A_link_B(f, 0, relay0, 0);
    }
    dinfoSetValue(&relay0->dInfo, &f->dInfo);
    dinfoSetValue(&relay0->mixInfo, &f->mixInfo);
    pthread_mutex_unlock(&StreamFlow_mutex);
}

static void relay0link1 (void * arg)
{
    IteFlower * f   = (IteFlower *)arg;
    relay0->cb_link = link_callback0;
    pthread_mutex_lock(&StreamFlow_mutex);
    if (relay0->audiocase == AudioIdle)
    {
        RelayFlowStart(relay0, NULL);
    }
    if (relay0->audiocase == AudioRelay)
    {
        FlowQ_A_link_B(f, 1, relay0, 1);
    }
    dinfoSetValue(&relay0->dInfo, &f->dInfo);
    dinfoSetValue(&relay0->mixInfo, &f->mixInfo);
    pthread_mutex_unlock(&StreamFlow_mutex);
}

static void relay1link0 (void * arg)
{
    IteFlower * f   = (IteFlower *)arg;
    relay1->cb_link = link_callback1;
    pthread_mutex_lock(&StreamFlow_mutex);
    if (relay1->audiocase == AudioIdle)
    {
        RelayFlowStart(relay1, NULL);
    }
    if (relay1->audiocase == AudioRelay)
    {
        FlowQ_A_link_B(f, 2, relay1, 0);
    }
    dinfoSetValue(&relay1->dInfo, &f->dInfo);
    dinfoSetValue(&relay1->mixInfo, &f->mixInfo);
    pthread_mutex_unlock(&StreamFlow_mutex);
}

static void relay1link1 (void * arg)
{
    IteFlower * f   = (IteFlower *)arg;
    relay1->cb_link = link_callback1;
    pthread_mutex_lock(&StreamFlow_mutex);
    if (relay1->audiocase == AudioIdle)
    {
        RelayFlowStart(relay1, NULL);
    }
    if (relay1->audiocase == AudioRelay)
    {
        FlowQ_A_link_B(f, 3, relay1, 1);
    }
    dinfoSetValue(&relay1->dInfo, &f->dInfo);
    dinfoSetValue(&relay1->mixInfo, &f->mixInfo);
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void RelayFlush (IteFlower * f)
{                              // stop relay & flush link
    FlowQ_A_unlink_B(f, fout); // unlink play & i2sout
    RelayFlowStop(f);
    flowQflush(f);
}

/********API*******/

void flowstream_start_quarter_play (IteFlower * f, const char * filepath,
                                    PlaySoundCase mode, Callback_t func,
                                    int pin)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    if (relay0 == NULL)
    {
        relay0 = ItoFlowInit(1); // relay0
    }
    if (relay1 == NULL)
    {
        relay1 = ItoFlowInit(1); // relay1
    }
    switch (pin)
    {
        case 0:
            f->cb_link = relay0link0;
            break;
        case 1:
            f->cb_link = relay0link1;
            break;
        case 2:
            f->cb_link = relay1link0;
            break;
        case 3:
            f->cb_link = relay1link1;
            break;
    }
    PlayerFlowStart(f, filepath, mode, func);
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_stop_quarter_play (IteFlower * f, int pin)
{
    IteFlower * relay;
    switch (pin)
    {
        case 0:
            relay = relay0;
            break;
        case 1:
            relay = relay0;
            break;
        case 2:
            relay = relay1;
            break;
        case 3:
            relay = relay1;
            break;
    }
    pthread_mutex_lock(&StreamFlow_mutex);

    FlowQ_A_unlink_B(f, relay); // unlink play & i2sout
    FlowPlayerCancel(f);
    flowQflush(f);

    if (FlowQ_Check_MixIdle(relay))
    {
        RelayFlush(relay);
    }
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}


/*stream play*/
void flowstream_start_quarter_streamplay (IteFlower * f, DataInfo * di,int pin)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    if (relay0 == NULL)
    {
        relay0 = ItoFlowInit(1); // relay0
    }
    if (relay1 == NULL)
    {
        relay1 = ItoFlowInit(1); // relay1
    }
    switch (pin)
    {
        case 0:
            f->cb_link = relay0link0;
            break;
        case 1:
            f->cb_link = relay0link1;
            break;
        case 2:
            f->cb_link = relay1link0;
            break;
        case 3:
            f->cb_link = relay1link1;
            break;
    }
    StreamerFlowStart(f, di);
    pthread_mutex_unlock(&StreamFlow_mutex);
}

void flowstream_stop_quarter_streamplay (IteFlower * f, int pin)
{
    IteFlower * relay;
    switch (pin)
    {
        case 0:
            relay = relay0;
            break;
        case 1:
            relay = relay0;
            break;
        case 2:
            relay = relay1;
            break;
        case 3:
            relay = relay1;
            break;
    }
    pthread_mutex_lock(&StreamFlow_mutex);

    FlowQ_A_unlink_B(f, relay); // unlink play & i2sout
    FlowPlayerCancel(f);
    flowQflush(f);

    if (FlowQ_Check_MixIdle(relay))
    {
        RelayFlush(relay);
    }
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}
