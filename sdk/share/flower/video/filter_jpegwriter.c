#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#if defined(CFG_JPEG_HW_ENABLE)
    #include "jpg/ite_jpg.h"
#endif
#include "libavcodec/avcodec.h"

typedef struct
{
    pthread_mutex_t mutex;
    bool            Runstate;
    char            filepath[DEF_FileStream_Name_LENGTH];
} JpegWriter;

static void take_snapshot (IteFilter * f, void * arg)
{
    JpegWriter * s = (JpegWriter *)f->data;

    pthread_mutex_lock(&s->mutex);
    s->Runstate = true;
    strncpy(s->filepath, (char *)arg, sizeof(s->filepath) - 1);
    pthread_mutex_unlock(&s->mutex);
}

static void jpegwriter_init (IteFilter * f)
{
    JpegWriter * s = (JpegWriter *)ite_new(JpegWriter, 1);
    if (s != NULL)
    {
        DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
        pthread_mutex_init(&s->mutex, NULL);
        s->Runstate = false;
    }
    f->data = s;
}

static void jpegwriter_uninit (IteFilter * f)
{
    JpegWriter * s = (JpegWriter *)f->data;
    if (s != NULL)
    {
        DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
        pthread_mutex_destroy(&s->mutex);
        free(s);
    }
}

static void jpegwriter_process (IteFilter * f)
{
#if defined(CFG_JPEG_HW_ENABLE)
    JpegWriter *    s              = (JpegWriter *)f->data;
    IteQueueblk     blk            = {0};
    IteQueueblk     blk_output     = {0};
    mblk_ite *      im             = NULL;
    AVFrame *       picture        = NULL;
    HJPG *          pHJpeg         = 0;
    JPG_INIT_PARAM  initParam      = {0};
    JPG_STREAM_INFO inStreamInfo   = {0};
    JPG_STREAM_INFO outStreamInfo  = {0};
    JPG_BUF_INFO    entropyBufInfo = {0};
    JPG_USER_INFO   jpgUserInfo    = {0};
    uint8_t *       ya_out = 0, *ua_out = 0,
            *va_out    = 0; // address of YUV decoded video buffer
    uint32_t src_w_out = 0, src_h_out = 0;
    uint32_t H264_pitch_y  = 0;
    uint32_t H264_pitch_uv = 0;
    uint32_t jpgEncSize    = 0;

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while (f->run)
    {
        pthread_mutex_lock(&s->mutex);
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            im = (mblk_ite *)blk.datap;
            if (im)
            {
                picture       = (AVFrame *)im->b_rptr;
                src_w_out     = picture->width;
                src_h_out     = picture->height;
                ya_out        = picture->data[0];
                ua_out        = picture->data[1];
                va_out        = picture->data[2];
                H264_pitch_y  = picture->linesize[0];
                H264_pitch_uv = picture->linesize[1];

                if (s->Runstate == true)
                {
                    mblk_ite * CompressedData = NULL;
                    // encode
                    initParam.codecType       = JPG_CODEC_ENC_JPG;
                    initParam.exColorSpace    = JPG_COLOR_SPACE_NV12;
                    initParam.outColorSpace   = JPG_COLOR_SPACE_YUV420;
                    initParam.width           = src_w_out;
                    initParam.height          = src_h_out;
                    initParam.encQuality      = 70; // 85;

                    iteJpg_CreateHandle(&pHJpeg, &initParam, 0);

                    inStreamInfo.streamIOType         = JPG_STREAM_IO_READ;
                    inStreamInfo.streamType           = JPG_STREAM_MEM;
                    // Y
                    inStreamInfo.jstream.mem[0].pAddr = (uint8_t *)ya_out;
                    inStreamInfo.jstream.mem[0].pitch = H264_pitch_y;

                    // U
                    inStreamInfo.jstream.mem[1].pAddr = (uint8_t *)ua_out;
                    inStreamInfo.jstream.mem[1].pitch = H264_pitch_uv;

                    // V
                    inStreamInfo.jstream.mem[2].pAddr = (uint8_t *)va_out;
                    inStreamInfo.jstream.mem[2].pitch = H264_pitch_uv;

                    inStreamInfo.validCompCnt         = 3;

                    CompressedData = allocb_ite(DEF_BitStream_BUF_LENGTH +
                                                DEF_FileStream_Name_LENGTH);
                    strcpy(CompressedData->b_wptr, s->filepath);
                    CompressedData->b_wptr += DEF_FileStream_Name_LENGTH;

                    outStreamInfo.streamIOType          = JPG_STREAM_IO_WRITE;
                    outStreamInfo.streamType            = JPG_STREAM_MEM;
                    outStreamInfo.jpg_reset_stream_info = 0;
                    outStreamInfo.jstream.mem[0].pAddr = CompressedData->b_wptr;
                    outStreamInfo.jstream.mem[0].length =
                        DEF_BitStream_BUF_LENGTH;
                    outStreamInfo.validCompCnt = 1;

                    printf("\n\n\tencode input: Y=%p, u=%p, v=%p\n",
                           inStreamInfo.jstream.mem[0].pAddr,
                           inStreamInfo.jstream.mem[1].pAddr,
                           inStreamInfo.jstream.mem[2].pAddr);

                    iteJpg_SetStreamInfo(pHJpeg, &inStreamInfo, &outStreamInfo,
                                         0);
                    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
                    iteJpg_Setup(pHJpeg, 0);

                    iteJpg_Process(pHJpeg, &entropyBufInfo, &jpgEncSize, 0);

                    iteJpg_GetStatus(pHJpeg, &jpgUserInfo, 0);
                    printf("\n\tresult = %d, encode size = %ld bytes\n",
                           jpgUserInfo.status, jpgEncSize);

                    iteJpg_DestroyHandle(&pHJpeg, 0);

                    CompressedData->b_wptr += jpgEncSize;
                    blk_output.datap = CompressedData;
                    ite_queue_put(f->output[0].Qhandle, &blk_output);

                    if (im)
                    {
                        freemsg_ite(im);
                    }
                    im          = NULL;
                    s->Runstate = false;
                }
                else
                {
                    if (im)
                    {
                        freemsg_ite(im);
                    }
                    im = NULL;
                }
            }
        }
        pthread_mutex_unlock(&s->mutex);
        usleep(1000);
    }
#endif
}

static IteMethodDes jpegwriter_methods[] = {
    {ITE_FILTER_JPEG_SNAPSHOT, take_snapshot},
    {                       0,          NULL}
};

// clang-format off
IteFilterDes FilterJpegWriter = {
    ITE_FILTER_JPEG_WRITER_ID,
    jpegwriter_init,
    jpegwriter_uninit,
    jpegwriter_process,
    jpegwriter_methods
};
// clang-format on
