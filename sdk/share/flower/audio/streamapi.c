#include "flower/flower.h"
#include "ite/audio.h"
/*
creat a global Q buffer [A->Qbuffer->B]
example
1.Get data from online then put to queque ,than combine to audioflow do
something :(play ,mix ,save file...) 2.Use audioflow put to queeue,than send
data to online

typedef struct _gQueueData{
    int gcodecType;
    mblkq dataQ;
    pthread_mutex_t mutex;
}gQueueData;
*/
/*========================================================================*/
gQueueData         gQ;
extern IteFlower * gflower;

/*init global Q */
void               streamQInit (int cotype)
{
    mblkQShapeInit(&gQ.dataQ, 2048 * 2);
    pthread_mutex_init(&gQ.mutex, NULL);
    gQ.gcodecType = cotype;
    gQ.startcount = 10;
    gQ.flagEof    = false;
}
/*uninit global Q */
void streamQUninit (void)
{
    pthread_mutex_lock(&gQ.mutex);
    mblkQShapeUninit(&gQ.dataQ);
    pthread_mutex_unlock(&gQ.mutex);
    pthread_mutex_destroy(&gQ.mutex);
    gQ.gcodecType = -1;
    gQ.startcount = 0;
    gQ.flagEof    = false;
}
/*put data to Q
@inbuf: data point
@size:  data size
@512 :  shape inbuf size to 512 byte than put to dataQ [user can modify]
*/
void streamQPut (unsigned char * inbuf, int size)
{
    if (!gQ.startcount)
    {
        printf("!gQ.startcount\n");
        streamQInit(ITE_MP3_DECODE);
    }

    pthread_mutex_lock(&gQ.mutex);
    srcQShapePut(&gQ.dataQ, inbuf, size, 512);
    pthread_mutex_unlock(&gQ.mutex);
}

/*put data to Q
@inbuf: data point
@size:  data size
@chopsize :  shape inbuf size to chopsize byte than put to dataQ [user can
modify]
*/
void streamQPutShape (unsigned char * inbuf, int size, int chopsize)
{
    if (!gQ.startcount)
    {
        printf("!gQ.startcount\n");
        streamQInit(ITE_MP3_DECODE);
    }

    pthread_mutex_lock(&gQ.mutex);
    // srcQShapePut(&gQ.dataQ,inbuf,size,256);//mp3
    srcQShapePut(&gQ.dataQ, inbuf, size, chopsize); // aac
    pthread_mutex_unlock(&gQ.mutex);
}

/*put mblk data to Q
@mblk_ite: ite data memory block
*/
void streamQmblkPut (mblk_ite * im)
{
    if (!gQ.startcount)
    {
        printf("!gQ.startcount\n");
        streamQInit(ITE_MP3_DECODE);
    }

    pthread_mutex_lock(&gQ.mutex);
    mblkQShapePut(&gQ.dataQ, im, im->size);
    pthread_mutex_unlock(&gQ.mutex);
}

/*get data to Q*/
mblk_ite * streamQGet (void)
{
    mblk_ite * om;
    if (!gQ.startcount)
    {
        streamQInit(ITE_MP3_DECODE);
    }

    pthread_mutex_lock(&gQ.mutex);
    om = getmblkq(&gQ.dataQ);
    pthread_mutex_unlock(&gQ.mutex);

    return om;
}

/**/
void streamQEof (void)
{
    unsigned char psrc[128] = {0};
    printf("stream eof put\n");
    streamQPut(psrc, 0);
    gQ.flagEof = true;
}

/*get data to Q*/
int streamQCount (void)
{
    mblkq * q = &gQ.dataQ;
    return q->qcount;
}

/*flush all data in Q */
void streamQflush (void)
{
    pthread_mutex_lock(&gQ.mutex);
    mblkQShapeFlush(&gQ.dataQ);
    pthread_mutex_unlock(&gQ.mutex);
    // gQ.flagEof=false;
}

/*set data Q codec type :
ITE_MP3_DECODE
ITE_AAC_DECODE...
*/
void streamSetCodec (ITE_AUDIO_ENGINE cotype)
{
    gQ.gcodecType = cotype;
}

/*Get data Q codec type*/
int streamGetCodec (void)
{
    return gQ.gcodecType;
}
/*===================================*/
