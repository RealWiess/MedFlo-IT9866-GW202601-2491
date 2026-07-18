#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"

void               flowQInit (IteFlower * f);
extern IteFlower * fout;
static int         cold_run = 1;

/*Init Flow*/
IteFlower *        ItoFlowInit (int Qflag)
{
    IteFlower * f = (IteFlower *)ite_new0(IteFlower, 1);
    if (cold_run)
    {
        ite_flower_init();
        cold_run = 0;
        flow_init_DA();
        streamQInit(0);
        if (fout == NULL)
        {
            fout = ItoFlowInit(0);
        }
    }
    if (Qflag)
    {
        flowQInit(f);
    }
    f->audiostream = NULL;
    dinfoSetDefault(&f->dInfo);
    f->audiocase = AudioIdle;
    return f;
}

void ItoFlowUninit (IteFlower * f)
{
    flowQUninit(f);
    if (f && f->audiostream)
    {
        printf("error f->stream exit ,Close it!!\n");
        return;
    }
    if (f)
    {
        free(f);
    }
    // f=NULL;
}

/******************************Queue tool*****************************/

void flowQInit (IteFlower * f)
{
    mblkQShapeInit(&f->fQ.dataQ, 2048 * 4);
    pthread_mutex_init(&f->fQ.mutex, NULL);
    f->fQ.startcount = 10;
    f->fQ.flagEof    = false;
}

/*uninit global Q */
void flowQUninit (IteFlower * f)
{
    if (f)
    {
        pthread_mutex_lock(&f->fQ.mutex);
        mblkQShapeUninit(&f->fQ.dataQ);
        pthread_mutex_unlock(&f->fQ.mutex);
        pthread_mutex_destroy(&f->fQ.mutex);
        f->fQ.flagEof = false;
    }
}

/*put data to Q
@inbuf: data point
@size:  data size
@256 :  shape inbuf size to 256 byte than put to dataQ [user can modify]
*/
void flowQPut (IteFlower * f, unsigned char * inbuf, int size)
{
    if (!f->fQ.startcount)
    {
        flowQInit(f);
    }
    if (f->fQ.flagEof == true)
    {
        f->fQ.flagEof = false;
    }
    int bytes =
        (f->mixInfo.init == true) ? f->mixInfo.bytes20ms : f->dInfo.bytes20ms;
    pthread_mutex_lock(&f->fQ.mutex);
    srcQShapePut(&f->fQ.dataQ, inbuf, size,
                 bytes); // chip data to 20 ms data frame
    pthread_mutex_unlock(&f->fQ.mutex);
}

/*get data to Q*/
mblk_ite * flowQGet (IteFlower * f)
{
    mblk_ite * om;
    if (!f->fQ.startcount)
    {
        flowQInit(f);
    }

    pthread_mutex_lock(&f->fQ.mutex);
    om = getmblkq(&f->fQ.dataQ);
    if (om != NULL)
    {
        f->dInfo.currentms += 20;
    }
    pthread_mutex_unlock(&f->fQ.mutex);

    return om;
}

/**/
void flowQEof (IteFlower * f)
{
    unsigned char psrc[128] = {0};
    flowQPut(f, psrc, 0);
    f->fQ.flagEof = true;
}

/*get data to Q*/
int flowQCount (IteFlower * f)
{
    mblkq * q = &f->fQ.dataQ;
    return q->qcount;
}

/*flush all data in Q */
void flowQflush (IteFlower * f)
{
    pthread_mutex_lock(&f->fQ.mutex);
    mblkQShapeFlush(&f->fQ.dataQ);
    pthread_mutex_unlock(&f->fQ.mutex);
    f->fQ.flagEof = false;
}

/******************************Queue tool end*****************************/

int checkQcount (IteFlower * f, int count)
{
    if (flowQCount(f) > count)
    {
        usleep(100000);
        return 1;
    }
    return 0;
}
