#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "ite/itv.h"

// extern pthread_mutex_t  h264_mutex;

extern int itv_set_pb_mode (int pb_mode);

typedef struct _DecData
{
    AVCodecContext av_context;
    uint8_t *      bitstream;
    int            bitstream_size;
} DecData;

static void mjpeg_dec_init (IteFilter * f)
{
    DecData * d = (DecData *)ite_new(DecData, 1);
    if (d != NULL)
    {
        AVCodec * codec;
        int       error;

        DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
        avcodec_init();
        avcodec_register_all();
        codec = avcodec_find_decoder(CODEC_ID_MJPEG);
        if (codec == NULL)
        {
            printf("Could not find MJPEG decoder in ffmpeg.\n");
        }
        avcodec_get_context_defaults(&d->av_context);

        error = avcodec_open(&d->av_context, codec);
        if (error != 0)
        {
            printf("avcodec_open() failed.\n");
        }

        d->bitstream_size = 512 * 1024; // 512k
        d->bitstream      = malloc(d->bitstream_size);

        f->data           = d;
        itv_set_pb_mode(1);
    }
    else
    {
        f->data = NULL;
    }
}

static void mjpeg_dec_uninit (IteFilter * f)
{
    DecData * d = (DecData *)(f->data);
    if (d != NULL)
    {
        DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
        itv_set_pb_mode(0);

        avcodec_close(&d->av_context);

        free(d->bitstream);
        free(d);
    }
}

static void mjpeg_dec_process (IteFilter * f)
{
    DecData *   d           = (DecData *)(f->data);
    IteQueueblk blk         = {0};
    IteQueueblk blk_output0 = {0};
    IteQueueblk blk_output1 = {0};
    mblk_ite *  m           = NULL;
    AVFrame     orig;

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            AVPacket pkt;
            int      got_picture = 0;
            avcodec_get_frame_defaults(&orig);
            av_init_packet(&pkt);

            m = (mblk_ite *)blk.datap;
            if (m != NULL)
            {
                memcpy(d->bitstream, m->b_rptr, m->b_wptr - m->b_rptr);
                pkt.data = d->bitstream;
                pkt.size = m->b_wptr - m->b_rptr;

                if(avcodec_decode_video2(&d->av_context, &orig, &got_picture, &pkt) < 0)
                {
                    freemsg_ite(m);
                    av_free_packet(&pkt);
                    continue;
                }

                if (got_picture)
                {
                    mblk_ite * yuv_msg = NULL;
                    yuv_msg            = allocb_ite(sizeof(AVFrame));
                    if(yuv_msg != NULL)
                    {
                        memcpy(yuv_msg->b_rptr, &orig, sizeof(AVFrame));
                        yuv_msg->b_wptr += sizeof(AVFrame);
                        blk_output0.datap = yuv_msg;
                        ite_queue_put(f->output[0].Qhandle, &blk_output0);
                    }

                    if (f->output[1].Qhandle)
                    {
                        mblk_ite * yuv_data = NULL;
                        yuv_data            = allocb_ite(sizeof(AVFrame));
                        if(yuv_data != NULL)
                        {
                            memcpy(yuv_data->b_rptr, &orig, sizeof(AVFrame));
                            yuv_data->b_wptr += sizeof(AVFrame);
                            blk_output1.datap = yuv_data;
                            ite_queue_put(f->output[1].Qhandle, &blk_output1);
                        }
                    }
                }
            }
            freemsg_ite(m);

            av_free_packet(&pkt);
        }
        usleep(1000);
    }
}

// clang-format off
IteFilterDes FilterMJpegDEC = {
    ITE_FILTER_MJPEGDEC_ID,
    mjpeg_dec_init,
    mjpeg_dec_uninit,
    mjpeg_dec_process,
};
// clang-format on
