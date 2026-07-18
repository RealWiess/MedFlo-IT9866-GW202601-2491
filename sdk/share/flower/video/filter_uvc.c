#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "libuvc/libuvc.h"
#include "isp/mmp_isp.h"
#include "libavcodec/avcodec.h"

#define SHOW_UVC_PRINT 0

uvc_context_t *       ctx           = NULL;
uvc_error_t           res           = 0;
uvc_device_t *        dev           = NULL;
uvc_stream_ctrl_t     ctrl          = {0};
uvc_device_handle_t * devh          = NULL;
IteUVCStream          uvc_setting   = {0};
bool                  find_devices  = false;
bool                  uvc_opened    = false;
bool                  uvc_streaming = false;

static void           _uvcRemoveCb ()
{
    ithPrintf("_uvcRemoveCb() : usb removed, uvc disconnect!!!\n");
    uvc_opened    = false;
    uvc_streaming = false;
}

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
static void _uvcFrameCb (uvc_frame_t * frame, void * ptr)
{
    IteFilter * fp          = (IteFilter *)ptr;
    IteQueueblk blk_output0 = {0};
    IteQueueblk blk_output1 = {0};
    int ret = 0;
    if (frame->data_bytes && frame->data)
    {
        if (frame->frame_format == UVC_FRAME_FORMAT_MJPEG)
        {
            mblk_ite * jpeg_msg = NULL;
            jpeg_msg            = allocb_ite(frame->data_bytes);
            memcpy(jpeg_msg->b_rptr, frame->data, frame->data_bytes);
            jpeg_msg->b_wptr += frame->data_bytes;
            blk_output0.datap = jpeg_msg;
            ret = ite_queue_put(fp->output[0].Qhandle, &blk_output0);
            if(ret < 0)
            {
                freemsg_ite(jpeg_msg);
            }

            if (fp->output[1].Qhandle)
            {
                mblk_ite * jpegtoavi_msg = NULL;
                jpegtoavi_msg            = allocb_ite(frame->data_bytes);
                memcpy(jpegtoavi_msg->b_rptr, frame->data, frame->data_bytes);
                jpegtoavi_msg->b_wptr += frame->data_bytes;
                blk_output1.datap = jpegtoavi_msg;
                ret = ite_queue_put(fp->output[1].Qhandle, &blk_output1);
                if(ret < 0)
                {
                    freemsg_ite(jpegtoavi_msg);
                }
            }
        }
        else if (frame->frame_format == UVC_FRAME_FORMAT_H264)
        {
            mblk_ite * h264_msg = NULL;
            h264_msg            = allocb_ite(frame->data_bytes);
            memcpy(h264_msg->b_rptr, frame->data, frame->data_bytes);
            h264_msg->b_wptr += frame->data_bytes;
            blk_output0.datap = h264_msg;
            ret = ite_queue_put(fp->output[0].Qhandle, &blk_output0);
            if(ret < 0)
            {
                freemsg_ite(h264_msg);
            }

            if (fp->output[1].Qhandle)
            {
                mblk_ite * h264toavi_msg = NULL;
                h264toavi_msg            = allocb_ite(frame->data_bytes);
                memcpy(h264toavi_msg->b_rptr, frame->data, frame->data_bytes);
                h264toavi_msg->b_wptr += frame->data_bytes;
                blk_output1.datap = h264toavi_msg;
                ret = ite_queue_put(fp->output[1].Qhandle, &blk_output1);
                if(ret < 0)
                {
                    freemsg_ite(h264toavi_msg);
                }
            }
        }
        else
        {
            AVFrame yuvframe     = {0};
            yuvframe.width       = frame->width;
            yuvframe.height      = frame->height;
            yuvframe.linesize[0] = frame->width * 2;
            yuvframe.linesize[1] = frame->width;
            yuvframe.data[0]     = frame->data;
            yuvframe.data[1]     = frame->data;
            yuvframe.data[2]     = frame->data;
            yuvframe.format      = MMP_ISP_IN_YUYV;

            mblk_ite * yuv_msg   = allocb_ite(sizeof(AVFrame));
            memcpy(yuv_msg->b_rptr, &yuvframe, sizeof(AVFrame));
            yuv_msg->b_wptr += sizeof(AVFrame);
            blk_output0.datap = yuv_msg;
            ret = ite_queue_put(fp->output[0].Qhandle, &blk_output0);
            if(ret < 0)
            {
                freemsg_ite(yuv_msg);
            }
        }
    }
}

static void f_uvc_init (IteFilter * f)
{

    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    res = uvc_init(&ctx);

    if (res < 0)
    {
        uvc_perror(res, "uvc_init");
        return;
    }
    else
    {
        find_devices = true;
    }

    DEBUG_PRINT("UVC initialized\n");
}

static void f_uvc_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    /* Release our handle on the device */
    if (uvc_opened)
    {
        DEBUG_PRINT("uvc_close...\n");
        uvc_close(devh);
        uvc_exit(ctx);
        uvc_opened = false;
    }
}

static void f_uvc_process (IteFilter * f)
{
    while (f->run)
    {

        if (find_devices)
        {
            /* Locates the first attached UVC device, stores in dev */
            res = uvc_find_device(
                ctx, &dev, 0, 0,
                NULL); /* filter devices: vendor_id, product_id, "serial_num" */

            if (res < 0)
            {
                uvc_perror(res, "uvc_find_device"); /* no devices found */
            }
            else
            {
                DEBUG_PRINT("Device found\n");

                /* Try to open the device: requires exclusive access */
                res = uvc_open(dev, &devh);
                if (res < 0)
                {
                    uvc_perror(res, "uvc_open"); /* unable to open device */
                }
                else
                {
                    DEBUG_PRINT("Device opened\n");
                    uvc_opened = true;
                    uvc_reg_rm_cb(_uvcRemoveCb);
#if SHOW_UVC_PRINT
                    /* Print out a message containing all the information that
                     * libuvc knows about the device */
                    uvc_print_diag(devh, stderr);
#endif
                    const uvc_format_desc_t * fmt_desc =
                        uvc_get_format_descs(devh);
                    switch (fmt_desc->bDescriptorSubtype)
                    {
                        case UVC_VS_FORMAT_UNCOMPRESSED:
                            // fmt_desc->guidFormat
                            break;

                        case UVC_VS_FORMAT_MJPEG:
                            break;
                    }

                    if (uvc_setting.UVC_format == FLOWER_UVC_FRAME_FORMAT_MJPEG)
                    {
                        printf("w = %d, h =%d ,fps = %d\n",
                               uvc_setting.UVC_Width, uvc_setting.UVC_Height,
                               uvc_setting.UVC_FPS);
                        res = uvc_get_stream_ctrl_format_size(
                            devh, &ctrl, /* result stored in ctrl */
                            UVC_FRAME_FORMAT_MJPEG, uvc_setting.UVC_Width,
                            uvc_setting.UVC_Height,
                            uvc_setting.UVC_FPS); /* width, height, fps */
                    }
                    else if (uvc_setting.UVC_format ==
                             FLOWER_UVC_FRAME_FORMAT_H264)
                    {
                        printf("w = %d, h =%d ,fps = %d\n",
                               uvc_setting.UVC_Width, uvc_setting.UVC_Height,
                               uvc_setting.UVC_FPS);
                        res = uvc_get_stream_ctrl_format_size(
                            devh, &ctrl, /* result stored in ctrl */
                            UVC_FRAME_FORMAT_H264, uvc_setting.UVC_Width,
                            uvc_setting.UVC_Height,
                            uvc_setting.UVC_FPS); /* width, height, fps */
                    }
                    else
                    {
                        printf("w = %d, h =%d ,fps = %d\n",
                               uvc_setting.UVC_Width, uvc_setting.UVC_Height,
                               uvc_setting.UVC_FPS);
                        res = uvc_get_stream_ctrl_format_size(
                            devh, &ctrl, /* result stored in ctrl */
                            UVC_FRAME_FORMAT_YUYV, uvc_setting.UVC_Width,
                            uvc_setting.UVC_Height,
                            uvc_setting.UVC_FPS); /* width, height, fps */
                    }

#if SHOW_UVC_PRINT
                    /* Print out the result */
                    uvc_print_stream_ctrl(&ctrl, stderr);
#endif
                    if (res < 0)
                    {
                        uvc_perror(res, "get_mode"); /* device doesn't provide a
                                                        matching stream */
                    }
                    else
                    {

                        /* Start the video stream. The library will call user
                         *function cb: cb(frame, (void*) 12345)
                         */
                        res = uvc_start_streaming(devh, &ctrl, _uvcFrameCb,
                                                  (void *)f, 0);

                        if (res < 0)
                        {
                            uvc_perror(
                                res,
                                "start_streaming"); /* unable to start stream */
                        }
                        else
                        {
                            DEBUG_PRINT("Streaming...\n");
                            uvc_streaming = true;
                            uvc_set_ae_mode(
                                devh, 2); /* e.g., turn on auto exposure */
                        }
                    }
                }
            }
            find_devices = false;
        }

        usleep(500 * 1000);
    }

    if (uvc_streaming)
    {
        /* End the stream. Blocks until last callback is serviced */
        uvc_stop_streaming(devh);
        uvc_streaming = false;
    }
}

static void f_uvc_getopened (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    *(bool *)arg = uvc_opened;
}

static void f_uvc_setting (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    memcpy(&uvc_setting, arg, sizeof(IteUVCStream));
}

static void f_uvc_getconnected (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    *(bool *)arg = (bool)uvc_get_connected();
}

static IteMethodDes filter_uvc_methods[] = {
    {     ITE_FILTER_UVC_SETTING,      f_uvc_setting},
    {   ITE_FILTER_UVC_GETOPENED,    f_uvc_getopened},
    {ITE_FILTER_UVC_GETCONNECTED, f_uvc_getconnected},
};

// clang-format off
IteFilterDes FilterUVC = {
    ITE_FILTER_UVC_ID,
    f_uvc_init,
    f_uvc_uninit,
    f_uvc_process,
    filter_uvc_methods,
};
// clang-format on
