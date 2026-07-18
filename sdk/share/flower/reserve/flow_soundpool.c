#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"

#define MAXFILENUM 10

extern pthread_mutex_t StreamFlow_mutex;
extern IteFlower *     fout;

SoundPoolParm          SPparm[MAXFILENUM];

/*init soundpool*/
int                    SoundPoolCreate (int num)
{
    int i = 0;
    // DataInfo *Info=&SPparm[i].info;
    for (i = 0; i < num; i++)
    {
        if (SPparm[i].fd != NULL)
        {
            fclose(SPparm[i].fd);
        }
        SPparm[i].fd    = NULL;
        SPparm[i].id    = -1;
        SPparm[i].data  = 0;
        SPparm[i].datap = NULL;
        dinfoSetDefault(&SPparm[i].info);
    }
    return 0;
}

/*load file to sound pool*/
int SoundPoolPreLoad (int id, const char * path)
{
    SoundPoolParm * parm = &SPparm[id];
    if (id > (MAXFILENUM - 1))
    {
        printf("error :id = %d \n", id);
        return -1;
    }

    FILE * fd = NULL;
    if (fd != NULL)
    {
        fclose(fd);
        fd = NULL;
    }
    if ((fd = fopen(path, "rb")) == NULL)
    {
        printf("open file error\n");
        return -1;
    }

    (void)parsing_wav(fd, &parm->info); // seconds
    parm->info.init = true;
    setvbuf(fd, NULL, _IOFBF,
            parm->info.bytes20ms *
                100); // increase stream buf,improve efficiency(2sec len);
    parm->fd = fd;
    return 1;
}

/*reset sound pool*/
void SoundPoolRelease (void)
{
    SoundPoolCreate(MAXFILENUM);
}

/*play from sound pool (output0)*/
void flow_soundpool_play (IteFlower * f, int SPid, PlaySoundCase mode,
                          Callback_t func)
{

    pthread_mutex_lock(&StreamFlow_mutex);

    FlowPlayerFlush(f);
    SoundFlowStart(f, &SPparm[SPid], mode, func);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 0, fout, 0);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

/*play from sound pool (output1)*/
void flow_soundpool_mix_play (IteFlower * f, int SPid, PlaySoundCase mode,
                              Callback_t func)
{

    pthread_mutex_lock(&StreamFlow_mutex);

    FlowPlayerFlush(f);
    SoundFlowStart(f, &SPparm[SPid], mode, func);
    if (FlowQ_link_precheck(f, fout, -1))
    {
        FlowQ_A_link_B(f, 1, fout, 1);
    }
    pthread_mutex_unlock(&StreamFlow_mutex);
}

/*stop soundpool play*/
void flow_soundpool_stop (IteFlower * f)
{
    pthread_mutex_lock(&StreamFlow_mutex);
    if (FlowQ_Check_MixIdle(fout))
    {
        FlowI2SCancel(fout);
    }
    FlowPlayerFlush(f);
    pthread_mutex_unlock(&StreamFlow_mutex);
}
