#include <stdio.h>
#include <malloc.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite/itv.h"
#include "filter_nv12_jpgenc.h"
#if defined(CFG_JPEG_HW_ENABLE)
    #include "jpg/ite_jpg.h"
#endif

typedef struct
{
    pthread_mutex_t mutex;
} NV12JpgEnc;

typedef void (*pfPktReleaseCb)(void * pkt);

typedef struct NV12InputPkt
{
    ITV_DBUF_PROPERTY NV12_frame;
} NV12InputPkt;

typedef struct QueuePktList
{
    void *                pkt;
    struct QueuePktList * next;
} QueuePktList;

typedef struct PktQueue
{
    QueuePktList *  firstPkt, *lastPkt;
    int             numPackets;
    int             maxPackets;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    pfPktReleaseCb  pfPktRelease;
} PktQueue;

PktQueue    gNV12InputQueue;

static void _nv12InputPktRelease (void * pkt)
{
    NV12InputPkt * ptNV12InputPkt = (NV12InputPkt *)pkt;
    if (ptNV12InputPkt != NULL)
    {
        free(ptNV12InputPkt);
    }
}

static int _packetQueuePut (PktQueue * q, void * pkt)
{
    QueuePktList * pkt1;

    /* duplicate the packet */
    if (q->mutex)
    {
        while (q->numPackets > q->maxPackets)
        {
            printf("queue is full: cur: %d, max: %d\n", q->numPackets,
                   q->maxPackets);
            usleep(1000 * 100);
        }
        pkt1 = malloc(sizeof(QueuePktList));
        if (!pkt1)
        {
            return -1;
        }
        pkt1->pkt  = pkt;
        pkt1->next = NULL;

        pthread_mutex_lock(&q->mutex);

        if (!q->lastPkt)
        {
            q->firstPkt = pkt1;
        }
        else
        {
            q->lastPkt->next = pkt1;
        }
        q->lastPkt = pkt1;
        q->numPackets++;
        /* XXX: should duplicate packet data in DV case */
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    else
    {
        return -1;
    }
}

static int _packetQueueGet (PktQueue * q, void ** pkt, int block)
{
    QueuePktList * pkt1;
    int            ret;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);

        for (;;)
        {
            pkt1 = q->firstPkt;
            if (pkt1)
            {
                q->firstPkt = pkt1->next;
                if (!q->firstPkt)
                {
                    q->lastPkt = NULL;
                }
                *pkt = pkt1->pkt;
                free(pkt1);
                q->numPackets--;
                ret = 1;
                break;
            }
            else if (block == 0) // non-block
            {
                ret = 0;
                break;
            }
            else // block
            {
                pthread_cond_wait(&q->cond, &q->mutex);
            }
        }
        pthread_mutex_unlock(&q->mutex);
        return ret;
    }
    else
    {
        return -1;
    }
}

/* packet queue handling */
static void _packetQueueInit (PktQueue * q, void * pfPktRelease, int maxPackets)
{
    memset(q, 0, sizeof(PktQueue));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->maxPackets   = maxPackets;
    q->pfPktRelease = pfPktRelease;
}

static void _packetQueueFlush (PktQueue * q)
{
    QueuePktList *pkt, *pkt1;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);
        for (pkt = q->firstPkt; pkt != NULL; pkt = pkt1)
        {
            pkt1 = pkt->next;
            if (q->pfPktRelease)
            {
                q->pfPktRelease(pkt->pkt);
            }
            free(pkt);
        }
        q->lastPkt    = NULL;
        q->firstPkt   = NULL;
        q->numPackets = 0;
        pthread_mutex_unlock(&q->mutex);
    }
}

static void _packetQueueEnd (PktQueue * q)
{
    if (q->mutex)
    {
        _packetQueueFlush(q);
        pthread_mutex_destroy(&q->mutex);
        pthread_cond_destroy(&q->cond);
        memset(q, 0, sizeof(PktQueue));
    }
}

void NV12_pushdata (void * arg)
{
    ITV_DBUF_PROPERTY * pkt     = (ITV_DBUF_PROPERTY *)arg;
    NV12InputPkt *      nv12Pkt = (NV12InputPkt *)malloc(sizeof(NV12InputPkt));
    if (nv12Pkt != NULL)
    {
        memcpy(&nv12Pkt->NV12_frame, pkt, sizeof(ITV_DBUF_PROPERTY));
        _packetQueuePut(&gNV12InputQueue, nv12Pkt);
    }
}

static void nv12_jpgenc_init (IteFilter * f)
{
    NV12JpgEnc * s = (NV12JpgEnc *)ite_new(NV12JpgEnc, 1);
    if (s != NULL)
    {
        DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
        pthread_mutex_init(&s->mutex, NULL);
        _packetQueueInit(&gNV12InputQueue, _nv12InputPktRelease, 20);
    }
    f->data = s;
}

static void nv12_jpgenc_uninit (IteFilter * f)
{
    NV12JpgEnc * s = (NV12JpgEnc *)f->data;
    if (s != NULL)
    {
        DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
        _packetQueueEnd(&gNV12InputQueue);
        pthread_mutex_destroy(&s->mutex);
        free(s);
    }
}

static void nv12_jpgenc_process (IteFilter * f)
{
#if defined(CFG_JPEG_HW_ENABLE)
    NV12JpgEnc *    s              = (NV12JpgEnc *)f->data;
    NV12InputPkt *  ptNV12Pkt      = NULL;
    IteQueueblk     blk_output     = {0};

    HJPG *          pHJpeg         = 0;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    uint8_t *       ya_out = 0, *ua_out = 0,
            *va_out    = 0; // address of YUV decoded video buffer
    uint32_t src_w_out = 0, src_h_out = 0;
    uint32_t pitch_y    = 0;
    uint32_t pitch_uv   = 0;
    uint32_t jpgEncSize = 0;

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    while (f->run)
    {
        pthread_mutex_lock(&s->mutex);
        if (_packetQueueGet(&gNV12InputQueue, (void **)&ptNV12Pkt, 0) > 0)
        {
            if (ptNV12Pkt != NULL)
            {
                mblk_ite * CompressedData = NULL;
                CompressedData          = allocb_ite(DEF_BitStream_BUF_LENGTH);

                src_w_out               = ptNV12Pkt->NV12_frame.src_w;
                src_h_out               = ptNV12Pkt->NV12_frame.src_h;
                ya_out                  = ptNV12Pkt->NV12_frame.ya;
                ua_out                  = ptNV12Pkt->NV12_frame.ua;
                va_out                  = ptNV12Pkt->NV12_frame.va;
                pitch_y                 = ptNV12Pkt->NV12_frame.pitch_y;
                pitch_uv                = ptNV12Pkt->NV12_frame.pitch_uv;

                // printf("YC: src_w_out = %d, src_h_out = %d, pitch_y = %d,
                // pitch_uv = %d, ya_out = %x, ua_out = %x\n", src_w_out,
                // src_h_out, pitch_y, pitch_uv, ya_out, ua_out);
                initParam.codecType     = JPG_CODEC_ENC_JPG;
                initParam.exColorSpace  = JPG_COLOR_SPACE_NV12;
                initParam.outColorSpace = JPG_COLOR_SPACE_YUV420;
                initParam.width         = src_w_out;
                initParam.height        = src_h_out;
                initParam.encQuality    = 70; // 85;

                iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

                inStreamInfo.streamIOType           = JPG_STREAM_IO_READ;
                inStreamInfo.streamType             = JPG_STREAM_MEM;
                // Y
                inStreamInfo.jstream.mem[0].pAddr   = (uint8_t *)ya_out;
                inStreamInfo.jstream.mem[0].pitch   = pitch_y;

                // U
                inStreamInfo.jstream.mem[1].pAddr   = (uint8_t *)ua_out;
                inStreamInfo.jstream.mem[1].pitch   = pitch_uv;

                // V
                inStreamInfo.jstream.mem[2].pAddr   = (uint8_t *)va_out;
                inStreamInfo.jstream.mem[2].pitch   = pitch_uv;

                inStreamInfo.validCompCnt           = 3;

                outStreamInfo.streamIOType          = JPG_STREAM_IO_WRITE;
                outStreamInfo.streamType            = JPG_STREAM_MEM;
                outStreamInfo.jpg_reset_stream_info = 0;
                outStreamInfo.jstream.mem[0].pAddr  = CompressedData->b_wptr;
                outStreamInfo.jstream.mem[0].length = DEF_BitStream_BUF_LENGTH;
                outStreamInfo.validCompCnt          = 1;

                /*
                printf("\n\n\tencode input: Y=%p, u=%p, v=%p\n",
                       inStreamInfo.jstream.mem[0].pAddr,
                       inStreamInfo.jstream.mem[1].pAddr,
                       inStreamInfo.jstream.mem[2].pAddr);
                */

                iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, &outStreamInfo, 0);
                iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);

                iteJpg_Setup(pHJpeg, 0);

                iteJpg_Process(pHJpeg, &entropyBufInfo, &jpgEncSize, 0);

                iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
                printf("\n\tresult = %d, encode size = %ld bytes\n",
                       jpgUserInfo.status, jpgEncSize);

                iteJpg_DestroyHandle(&pHJpeg, 0);

                // printf("YC: CompressedData->b_wptr = %x, jpgEncSize = %ld\n",
                // CompressedData->b_wptr, jpgEncSize); while(1);
                CompressedData->b_wptr += jpgEncSize;
                blk_output.datap = CompressedData;
                ite_queue_put(f->output[0].Qhandle, &blk_output);
            }
            _nv12InputPktRelease(ptNV12Pkt);
        }
        pthread_mutex_unlock(&s->mutex);
        usleep(1000);
    }
#endif
}

// clang-format off
IteFilterDes FilterNV12JpgEnc = {
    ITE_FILTER_NV12_JPGENC_ID,
    nv12_jpgenc_init,
    nv12_jpgenc_uninit,
    nv12_jpgenc_process,
};
// clang-format on
