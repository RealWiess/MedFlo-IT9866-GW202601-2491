#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"

#define CHECK_(condition)                                                      \
    if (!(condition))                                                          \
    {                                                                          \
        result = -1;                                                           \
        break;                                                                 \
    }

pthread_mutex_t VideoFlow_mutex = PTHREAD_MUTEX_INITIALIZER;

static void     video_flower_free (IteVideoFlower * s)
{
    if (s != NULL)
    {
        if (s->Fudprecv != NULL)
        {
            ite_filter_delete(s->Fudprecv);
        }
        if (s->Ftcprecv != NULL)
        {
            ite_filter_delete(s->Ftcprecv);
        }
        if (s->Fh264dec != NULL)
        {
            ite_filter_delete(s->Fh264dec);
        }
        if (s->Fdisplay != NULL)
        {
            ite_filter_delete(s->Fdisplay);
        }
        if (s->Fjpegwriter != NULL)
        {
            ite_filter_delete(s->Fjpegwriter);
        }
        if (s->Ffilewriter != NULL)
        {
            ite_filter_delete(s->Ffilewriter);
        }
        if (s->Frec_avi != NULL)
        {
            ite_filter_delete(s->Frec_avi);
        }
        if (s->Fipcam != NULL)
        {
            ite_filter_delete(s->Fipcam);
        }
        if (s->Fcap != NULL)
        {
            ite_filter_delete(s->Fcap);
        }
        if (s->Fuvc != NULL)
        {
            ite_filter_delete(s->Fuvc);
        }
        if (s->Fjpegdec != NULL)
        {
            ite_filter_delete(s->Fjpegdec);
        }

        free(s);
    }
}

/*video udp stream*/
int flow_start_udp_videostream (IteFlower * f, Call_info * call_list,
                                unsigned short rem_port, VideoStreamDir dir)
{
    int result = 0;

    if (dir == VideoStreamRecvOnly)
    {
        char *           args[] = {"-Q=32"};
        udp_config_t     udp_conf;
        IteVideoFlower * vstream;

        do
        {
            vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
            CHECK_(vstream != NULL);

            vstream->Fudprecv = ite_filter_new(ITE_FILTER_UDP_RECV_ID);
            CHECK_(vstream->Fudprecv != NULL);
            vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
            CHECK_(vstream->Fh264dec != NULL);
            vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
            CHECK_(vstream->Fdisplay != NULL);
            vstream->Fjpegwriter = ite_filter_new(ITE_FILTER_JPEG_WRITER_ID);
            CHECK_(vstream->Fjpegwriter != NULL);
            vstream->Ffilewriter = ite_filter_new(ITE_FILTER_FILE_WRITER_ID);
            CHECK_(vstream->Ffilewriter != NULL);
            vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);
            CHECK_(vstream->Frec_avi != NULL);

            memset(&udp_conf, '\0', sizeof(udp_config_t));
            udp_conf.remote_port = rem_port;
            udp_conf.cur_socket  = -1;
            udp_conf.c_type      = VIDEO_INPUT;

            ite_filter_call_method(vstream->Fudprecv,
                                   ITE_FILTER_UDP_RECV_SET_PARA,
                                   (void *)&udp_conf);

            ite_filterChain_build(&vstream->fc, "FC 1");
            ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

            ite_filterChain_link(&vstream->fc, -1, vstream->Fudprecv, -1);
            ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
            ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

            // avi recoder
            ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fudprecv, 1,
                                     vstream->Frec_avi, 0);

            // snapshot
            ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fh264dec, 1,
                                     vstream->Fjpegwriter, 0);
            ite_filterChain_link(&vstream->fc, 0, vstream->Ffilewriter, 0);

            // printf("===Filter Chain run===\n");
            ite_filterChain_run(&vstream->fc);

            f->videostream = vstream;

        } while (false);

        if (result != 0 && vstream != NULL)
        {
            video_flower_free(vstream);
        }
    }

    return result;
}

/*video tcp stream*/
int flow_start_tcp_videostream (IteFlower * f, Call_info * call_list,
                                unsigned short rem_port, VideoStreamDir dir)
{
    int result = 0;

    if (dir == VideoStreamRecvOnly)
    {
        char *           args[] = {"-Q=32"};
        tcp_config_t     tcp_conf;
        IteVideoFlower * vstream;

        do
        {
            vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
            CHECK_(vstream != NULL);

            vstream->Ftcprecv = ite_filter_new(ITE_FILTER_TCP_RECV_ID);
            CHECK_(vstream->Ftcprecv != NULL);
            vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
            CHECK_(vstream->Fh264dec != NULL);
            vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
            CHECK_(vstream->Fdisplay != NULL);
            vstream->Fjpegwriter = ite_filter_new(ITE_FILTER_JPEG_WRITER_ID);
            CHECK_(vstream->Fjpegwriter != NULL);
            vstream->Ffilewriter = ite_filter_new(ITE_FILTER_FILE_WRITER_ID);
            CHECK_(vstream->Ffilewriter != NULL);
            vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);
            CHECK_(vstream->Frec_avi != NULL);

            memset(&tcp_conf, '\0', sizeof(tcp_config_t));
            tcp_conf.remote_port = rem_port;
            tcp_conf.remote_ip   = call_list;
            tcp_conf.cur_socket  = -1;
            tcp_conf.c_type      = VIDEO_INPUT;

            ite_filter_call_method(vstream->Ftcprecv,
                                   ITE_FILTER_TCP_RECV_SET_PARA,
                                   (void *)&tcp_conf);

            ite_filterChain_build(&vstream->fc, "FC 1");
            ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

            ite_filterChain_link(&vstream->fc, -1, vstream->Ftcprecv, -1);
            ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
            ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

            // avi recoder
            ite_filterChain_A_link_B(false, &vstream->fc, vstream->Ftcprecv, 1,
                                     vstream->Frec_avi, 0);

            // snapshot
            ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fh264dec, 1,
                                     vstream->Fjpegwriter, 0);
            ite_filterChain_link(&vstream->fc, 0, vstream->Ffilewriter, 0);

            // printf("===Filter Chain run===\n");
            ite_filterChain_run(&vstream->fc);

            f->videostream = vstream;
        } while (false);

        if (result != 0 && vstream != NULL)
        {
            video_flower_free(vstream);
        }
    }

    return result;
}

/*stop video udp stream*/
int flow_stop_udp_videostream (IteFlower * f, VideoStreamDir dir)
{
    IteVideoFlower * s = f->videostream;

    ite_filterChain_stop(&s->fc);
    if (dir == VideoStreamRecvOnly)
    {
        if (s->Fudprecv)
        {
            ite_filterChain_unlink(&s->fc, -1, s->Fudprecv, -1);
        }
        if (s->Fh264dec)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Fh264dec, 0);
        }
        if (s->Fdisplay)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
        }
        if (s->Frec_avi)
        {
            ite_filterChain_A_unlink_B(false, &s->fc, s->Fudprecv, 1,
                                       s->Frec_avi, 0);
        }
        if (s->Fjpegwriter)
        {
            ite_filterChain_A_unlink_B(false, &s->fc, s->Fh264dec, 1,
                                       s->Fjpegwriter, 0);
        }
        if (s->Ffilewriter)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Ffilewriter, 0);
        }
    }
    ite_filterChain_delete(&s->fc);

    video_flower_free(s);
    return 0;
}

/*stop video tcp stream*/
int flow_stop_tcp_videostream (IteFlower * f, VideoStreamDir dir)
{
    IteVideoFlower * s = f->videostream;

    ite_filterChain_stop(&s->fc);
    if (dir == VideoStreamRecvOnly)
    {
        if (s->Ftcprecv)
        {
            ite_filterChain_unlink(&s->fc, -1, s->Ftcprecv, -1);
        }
        if (s->Fh264dec)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Fh264dec, 0);
        }
        if (s->Fdisplay)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
        }
        if (s->Frec_avi)
        {
            ite_filterChain_A_unlink_B(false, &s->fc, s->Ftcprecv, 1,
                                       s->Frec_avi, 0);
        }
        if (s->Fjpegwriter)
        {
            ite_filterChain_A_unlink_B(false, &s->fc, s->Fh264dec, 1,
                                       s->Fjpegwriter, 0);
        }
        if (s->Ffilewriter)
        {
            ite_filterChain_unlink(&s->fc, 0, s->Ffilewriter, 0);
        }
    }
    ite_filterChain_delete(&s->fc);

    video_flower_free(s);
    return 0;
}

int flow_start_recv_ipcamstream (IteFlower * f)
{
    int              result = 0;
    char *           args[] = {"-Q=32"};
    IteVideoFlower * vstream;

    do
    {
        vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
        CHECK_(vstream != NULL);

        vstream->Fipcam = ite_filter_new(ITE_FILTER_IPCAM_ID);
        CHECK_(vstream->Fipcam != NULL);
        vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
        CHECK_(vstream->Fh264dec != NULL);
        vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
        CHECK_(vstream->Fdisplay != NULL);
        vstream->Fjpegwriter = ite_filter_new(ITE_FILTER_JPEG_WRITER_ID);
        CHECK_(vstream->Fjpegwriter != NULL);
        vstream->Ffilewriter = ite_filter_new(ITE_FILTER_FILE_WRITER_ID);
        CHECK_(vstream->Ffilewriter != NULL);
        vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);
        CHECK_(vstream->Frec_avi != NULL);

        ite_filterChain_build(&vstream->fc, "FC 1");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&vstream->fc, -1, vstream->Fipcam, -1);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

        // avi recoder
        ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fipcam, 1,
                                 vstream->Frec_avi, 0);
        // snapshot
        ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fh264dec, 1,
                                 vstream->Fjpegwriter, 0);
        ite_filterChain_link(&vstream->fc, 0, vstream->Ffilewriter, 0);

        ite_filterChain_run(&vstream->fc);
        f->videostream = vstream;
    } while (false);

    if (result != 0 && vstream != NULL)
    {
        video_flower_free(vstream);
    }

    return result;
}

int flow_stop_recv_ipcamstream (IteFlower * f)
{
    IteVideoFlower * s = f->videostream;

    ite_filterChain_stop(&s->fc);

    if (s->Fipcam)
    {
        ite_filterChain_unlink(&s->fc, -1, s->Fipcam, -1);
    }
    if (s->Fh264dec)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fh264dec, 0);
    }
    if (s->Fdisplay)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
    }
    if (s->Frec_avi)
    {
        ite_filterChain_A_unlink_B(false, &s->fc, s->Fipcam, 1, s->Frec_avi, 0);
    }
    if (s->Fjpegwriter)
    {
        ite_filterChain_A_unlink_B(false, &s->fc, s->Fh264dec, 1,
                                   s->Fjpegwriter, 0);
    }
    if (s->Ffilewriter)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Ffilewriter, 0);
    }

    ite_filterChain_delete(&s->fc);

    video_flower_free(s);
    return 0;
}

/*start camera show flow*/
int flow_start_camera_show (IteFlower * f, int ch,
                            VideoInputWindowsInfo * w_info)
{
    int              result = 0;
    char *           args[] = {"-Q=32"};
    IteVideoFlower * vstream;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
        CHECK_(vstream != NULL);

        vstream->Fcap = ite_filter_new(ITE_FILTER_CAP_ID);
        CHECK_(vstream->Fcap != NULL);
        vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_DUAL_ID);
        CHECK_(vstream->Fdisplay != NULL);

        // If sensor support multi-channel.
        ite_filter_call_method(vstream->Fcap, ITE_FILTER_CAP_SETSENSORINIT,
                               (void *)ch);

        // Set input windows infomation.
        ite_filter_call_method(vstream->Fdisplay,
                               ITE_FILTER_DISPLAY_SET_INPUT_WINDOWS,
                               (void *)w_info);

        ite_filterChain_build(&vstream->fc, "FC 1");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&vstream->fc, -1, vstream->Fcap, -1);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

        ite_filterChain_run(&vstream->fc);
        f->videostream = vstream;
    } while (false);

    if (result != 0 && vstream != NULL)
    {
        video_flower_free(vstream);
    }

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

/*stop camera show flow*/
void flow_stop_camera_show (IteFlower * f)
{
    pthread_mutex_lock(&VideoFlow_mutex);

    IteVideoFlower * s = f->videostream;

    ite_filterChain_stop(&s->fc);

    if (s->Fcap)
    {
        ite_filterChain_unlink(&s->fc, -1, s->Fcap, -1);
    }
    if (s->Fdisplay)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
    }

    ite_filterChain_delete(&s->fc);
    video_flower_free(s);

    pthread_mutex_unlock(&VideoFlow_mutex);
}

/* This API will simultaneously display the camera feed and overlay on-screen
 * display (OSD) on the screen. */
int flow_start_camera_with_osd (IteFlower * f, int ch)
{
    int              result = 0;
    IteVideoFlower * vstream;
    IteVideoFlower * rstream = NULL;
    char *           args[]  = {"-Q=32"};

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
        CHECK_(vstream != NULL);

        vstream->Fcap = ite_filter_new(ITE_FILTER_CAP_ID);
        CHECK_(vstream->Fcap != NULL);
        vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_OSD_ID);
        CHECK_(vstream->Fdisplay != NULL);

        // If sensor support multi-channel.
        ite_filter_call_method(vstream->Fcap, ITE_FILTER_CAP_SETSENSORINIT,
                               (void *)ch);

        ite_filterChain_build(&vstream->fc, "FC 1");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&vstream->fc, -1, vstream->Fcap, -1);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

        ite_filterChain_run(&vstream->fc);
        f->videostream = vstream;

        /*record stream*/
        rstream        = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
        CHECK_(rstream != NULL);

        rstream->Fjpegwriter = ite_filter_new(ITE_FILTER_NV12_JPGENC_ID);
        CHECK_(rstream->Fjpegwriter != NULL);
        rstream->Frec_avi = ite_filter_new(ITE_FILTER_MJPEG_REC_AVI_ID);
        CHECK_(rstream->Frec_avi != NULL);

        ite_filterChain_build(&rstream->fc, "FC VideoAVI");
        ite_filterChain_setConfig(&rstream->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&rstream->fc, -1, rstream->Fjpegwriter, -1);
        ite_filterChain_link(&rstream->fc, 0, rstream->Frec_avi, 0);

        ite_filterChain_run(&rstream->fc);
        f->recordstream = rstream;
    } while (false);

    if (result != 0)
    {
        if (vstream != NULL)
        {
            video_flower_free(vstream);
            f->videostream = NULL;
        }
        if (rstream != NULL)
        {
            video_flower_free(rstream);
        }
    }

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

/*stop camera with OSD flow*/
void flow_stop_camera_with_osd (IteFlower * f)
{

    pthread_mutex_lock(&VideoFlow_mutex);

    IteVideoFlower * s  = f->videostream;
    IteVideoFlower * rs = f->recordstream;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_stop(&rs->fc);

    if (s->Fcap)
    {
        ite_filterChain_unlink(&s->fc, -1, s->Fcap, -1);
    }
    if (s->Fdisplay)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
    }

    if (rs->Fjpegwriter)
    {
        ite_filterChain_unlink(&rs->fc, -1, rs->Fjpegwriter, -1);
    }
    if (rs->Frec_avi)
    {
        ite_filterChain_unlink(&rs->fc, 0, rs->Frec_avi, 0);
    }

    ite_filterChain_delete(&s->fc);
    ite_filterChain_delete(&rs->fc);

    video_flower_free(s);
    video_flower_free(rs);

    pthread_mutex_unlock(&VideoFlow_mutex);
}

int flow_start_camera_with_osd_record (IteFlower * f, char * file_path,
                                       int width, int height, int fps)
{
    int             result = 0;
    VideoMemoInfo * info;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        info = (VideoMemoInfo *)ite_new0(VideoMemoInfo, 1);
        CHECK_(info != NULL);

        strlcpy(info->videomemo_file, file_path, sizeof(info->videomemo_file));
        info->video_width  = width;
        info->video_height = height;
        info->video_fps    = fps;
        ite_filter_call_method(f->recordstream->Frec_avi,
                               ITE_FILTER_MJPEG_REC_AVI_OPEN, (void *)info);
        free(info);
    } while (false);

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

int flow_start_camera_with_osd_record_split (IteFlower * f, char * file_path,
                                             int width, int height, int fps)
{
    int             result = 0;
    VideoMemoInfo * info;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        info = (VideoMemoInfo *)ite_new0(VideoMemoInfo, 1);
        CHECK_(info != NULL);

        strlcpy(info->videomemo_file, file_path, sizeof(info->videomemo_file));
        info->video_width  = width;
        info->video_height = height;
        info->video_fps    = fps;
        ite_filter_call_method(f->recordstream->Frec_avi,
                               ITE_FILTER_MJPEG_REC_AVI_OPEN_PARTIAL,
                               (void *)info);
        free(info);
    } while (false);

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

void flow_stop_camera_with_osd_record (IteFlower * f)
{
    pthread_mutex_lock(&VideoFlow_mutex);
    ite_filter_call_method(f->recordstream->Frec_avi,
                           ITE_FILTER_MJPEG_REC_AVI_CLOSE, NULL);
    pthread_mutex_unlock(&VideoFlow_mutex);
}

bool flow_polling_camera_status (IteFlower * f)
{
    bool StableStatus = false;

    if (f->videostream->Fcap != NULL)
    {
        ite_filter_call_method(f->videostream->Fcap,
                               ITE_FILTER_CAP_GETSENSORSTABLE,
                               (void *)&StableStatus);
    }
    return StableStatus;
}

#ifdef CFG_UVC_ENABLE

/*YUV Foramt Control flow*/
int flow_start_uvc_yuv (IteFlower * f, int width, int height, int fps)
{
    int              result = 0;
    char *           args[] = {"-Q=32"};
    IteUVCStream *   uvc_ptr;
    IteVideoFlower * vstream = NULL;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        uvc_ptr = (IteUVCStream *)ite_new0(IteUVCStream, 1);
        CHECK_(uvc_ptr != NULL);
        vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
        CHECK_(vstream != NULL);

        vstream->Fuvc = ite_filter_new(ITE_FILTER_UVC_ID);
        CHECK_(vstream->Fuvc != NULL);
        vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
        CHECK_(vstream->Fdisplay != NULL);

        ite_filterChain_build(&vstream->fc, "FC UVC");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

        uvc_ptr->UVC_format = FLOWER_UVC_FRAME_FORMAT_YUYV;
        uvc_ptr->UVC_Width  = width;
        uvc_ptr->UVC_Height = height;
        uvc_ptr->UVC_FPS    = fps;

        ite_filter_call_method(vstream->Fuvc, ITE_FILTER_UVC_SETTING,
                               (void *)uvc_ptr);

        free(uvc_ptr);

        ite_filterChain_link(&vstream->fc, -1, vstream->Fuvc, -1);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);
        ite_filterChain_run(&vstream->fc);

        f->videostream = vstream;
    } while (false);

    if (result != 0)
    {
        if (uvc_ptr != NULL)
        {
            free(uvc_ptr);
        }
        if (vstream != NULL)
        {
            video_flower_free(vstream);
            f->videostream = NULL;
        }
    }

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

int flow_stop_uvc_yuv (IteFlower * f)
{
    pthread_mutex_lock(&VideoFlow_mutex);

    IteVideoFlower * s = f->videostream;

    ite_filterChain_stop(&s->fc);

    if (s->Fuvc)
    {
        ite_filterChain_unlink(&s->fc, -1, s->Fuvc, -1);
    }
    if (s->Fdisplay)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
    }
    ite_filterChain_delete(&s->fc);

    video_flower_free(s);

    f->videostream = NULL;

    pthread_mutex_unlock(&VideoFlow_mutex);

    return 0;
}
/*YUV Foramt Control flow End*/

/*MJPEG Foramt Control flow*/
int flow_start_uvc_mjpeg (IteFlower * f, int width, int height, int fps)
{
    int              result = 0;
    char *           args[] = {"-Q=32"};
    IteUVCStream *   uvc_ptr;
    IteVideoFlower * vstream = NULL;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        uvc_ptr = (IteUVCStream *)ite_new0(IteUVCStream, 1);
        CHECK_(uvc_ptr != NULL);
        vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
        CHECK_(vstream != NULL);

        vstream->Fuvc = ite_filter_new(ITE_FILTER_UVC_ID);
        CHECK_(vstream->Fuvc != NULL);
        vstream->Fjpegdec = ite_filter_new(ITE_FILTER_MJPEGDEC_ID);
        CHECK_(vstream->Fjpegdec != NULL);
        vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
        CHECK_(vstream->Fdisplay != NULL);
        vstream->Frec_avi = ite_filter_new(ITE_FILTER_MJPEG_REC_AVI_ID);
        CHECK_(vstream->Frec_avi != NULL);

        ite_filterChain_build(&vstream->fc, "FC UVC");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

        uvc_ptr->UVC_format = FLOWER_UVC_FRAME_FORMAT_MJPEG;
        uvc_ptr->UVC_Width  = width;
        uvc_ptr->UVC_Height = height;
        uvc_ptr->UVC_FPS    = fps;

        ite_filter_call_method(vstream->Fuvc, ITE_FILTER_UVC_SETTING,
                               (void *)uvc_ptr);

        free(uvc_ptr);

        ite_filterChain_link(&vstream->fc, -1, vstream->Fuvc, -1);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fjpegdec, 0);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

        // MJPEG to AVI recoder
        ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fuvc, 1,
                                 vstream->Frec_avi, 0);

        ite_filterChain_run(&vstream->fc);

        f->videostream = vstream;
    } while (false);

    if (result != 0)
    {
        if (uvc_ptr != NULL)
        {
            free(uvc_ptr);
        }
        if (vstream != NULL)
        {
            video_flower_free(vstream);
            f->videostream = NULL;
        }
    }

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

int flow_stop_uvc_mjpeg (IteFlower * f)
{
    pthread_mutex_lock(&VideoFlow_mutex);

    IteVideoFlower * s = f->videostream;

    ite_filterChain_stop(&s->fc);

    if (s->Fuvc)
    {
        ite_filterChain_unlink(&s->fc, -1, s->Fuvc, -1);
    }
    if (s->Fjpegdec)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fjpegdec, 0);
    }
    if (s->Fdisplay)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
    }
    // MJPEG to AVI recoder
    if (s->Frec_avi)
    {
        ite_filterChain_A_unlink_B(false, &s->fc, s->Fuvc, 1, s->Frec_avi, 0);
    }

    ite_filterChain_delete(&s->fc);

    video_flower_free(s);

    f->videostream = NULL;

    pthread_mutex_unlock(&VideoFlow_mutex);

    return 0;
}

int flow_start_uvc_mjpeg_record (IteFlower * f, char * file_path, int width,
                                 int height, int fps)
{
    int             result = 0;
    VideoMemoInfo * info;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        info = (VideoMemoInfo *)ite_new0(VideoMemoInfo, 1);
        CHECK_(info != NULL);

        strlcpy(info->videomemo_file, file_path, sizeof(info->videomemo_file));
        info->video_width  = width;
        info->video_height = height;
        info->video_fps    = fps;

        ite_filter_call_method(f->videostream->Frec_avi,
                               ITE_FILTER_MJPEG_REC_AVI_OPEN, (void *)info);

        free(info);
    } while (false);

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

void flow_stop_uvc_mjpeg_record (IteFlower * f)
{
    pthread_mutex_lock(&VideoFlow_mutex);
    ite_filter_call_method(f->videostream->Frec_avi,
                           ITE_FILTER_MJPEG_REC_AVI_CLOSE, NULL);
    pthread_mutex_unlock(&VideoFlow_mutex);
}
/*MJPEG Control flow End*/

/*H264 Foramt Control flow*/
int flow_start_uvc_h264 (IteFlower * f, int width, int height, int fps)
{
    int              result = 0;
    char *           args[] = {"-Q=32"};
    IteUVCStream *   uvc_ptr;
    IteVideoFlower * vstream = NULL;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        uvc_ptr = (IteUVCStream *)ite_new0(IteUVCStream, 1);
        CHECK_(uvc_ptr != NULL);
        vstream = (IteVideoFlower *)ite_new0(IteVideoFlower, 1);
        CHECK_(vstream != NULL);

        vstream->Fuvc = ite_filter_new(ITE_FILTER_UVC_ID);
        CHECK_(vstream->Fuvc != NULL);
        vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
        CHECK_(vstream->Fh264dec != NULL);
        vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
        CHECK_(vstream->Fdisplay != NULL);
        vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);
        CHECK_(vstream->Frec_avi != NULL);

        ite_filterChain_build(&vstream->fc, "FC UVC");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

        uvc_ptr->UVC_format = FLOWER_UVC_FRAME_FORMAT_H264;
        uvc_ptr->UVC_Width  = width;
        uvc_ptr->UVC_Height = height;
        uvc_ptr->UVC_FPS    = fps;

        ite_filter_call_method(vstream->Fuvc, ITE_FILTER_UVC_SETTING,
                               (void *)uvc_ptr);
        free(uvc_ptr);

        ite_filterChain_link(&vstream->fc, -1, vstream->Fuvc, -1);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
        ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

        // MJPEG to AVI recoder
        ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fuvc, 1,
                                 vstream->Frec_avi, 0);

        ite_filterChain_run(&vstream->fc);

        f->videostream = vstream;
    } while (false);

    if (result != 0)
    {
        if (uvc_ptr != NULL)
        {
            free(uvc_ptr);
        }
        if (vstream != NULL)
        {
            video_flower_free(vstream);
            f->videostream = NULL;
        }
    }

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

int flow_stop_uvc_h264 (IteFlower * f)
{
    pthread_mutex_lock(&VideoFlow_mutex);

    IteVideoFlower * s = f->videostream;

    ite_filterChain_stop(&s->fc);

    if (s->Fuvc)
    {
        ite_filterChain_unlink(&s->fc, -1, s->Fuvc, -1);
    }
    if (s->Fh264dec)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fjpegdec, 0);
    }
    if (s->Fdisplay)
    {
        ite_filterChain_unlink(&s->fc, 0, s->Fdisplay, 0);
    }
    // MJPEG to AVI recoder
    if (s->Frec_avi)
    {
        ite_filterChain_A_unlink_B(false, &s->fc, s->Fuvc, 1, s->Frec_avi, 0);
    }

    ite_filterChain_delete(&s->fc);

    video_flower_free(s);

    f->videostream = NULL;

    pthread_mutex_unlock(&VideoFlow_mutex);

    return 0;
}

int flow_start_uvc_h264_record (IteFlower * f, char * file_path, int width,
                                int height, int fps)
{
    int             result = 0;
    VideoMemoInfo * info;

    pthread_mutex_lock(&VideoFlow_mutex);

    do
    {
        info = (VideoMemoInfo *)ite_new0(VideoMemoInfo, 1);
        CHECK_(info != NULL);

        strlcpy(info->videomemo_file, file_path, sizeof(info->videomemo_file));
        info->video_width  = width;
        info->video_height = height;
        info->video_fps    = fps;

        ite_filter_call_method(f->videostream->Frec_avi,
                               ITE_FILTER_REC_AVI_OPEN, (void *)info);

        free(info);
    } while (false);

    pthread_mutex_unlock(&VideoFlow_mutex);

    return result;
}

void flow_stop_uvc_h264_record (IteFlower * f)
{
    pthread_mutex_lock(&VideoFlow_mutex);
    ite_filter_call_method(f->videostream->Frec_avi, ITE_FILTER_REC_AVI_CLOSE,
                           NULL);
    pthread_mutex_unlock(&VideoFlow_mutex);
}

/*H264 Control flow End*/

bool flow_polling_uvc_opened (IteFlower * f)
{
    bool OpenedStatus = false;
    ite_filter_call_method(f->videostream->Fuvc, ITE_FILTER_UVC_GETOPENED,
                           (void *)&OpenedStatus);
    return OpenedStatus;
}

bool flow_polling_uvc_connected (IteFlower * f)
{
    bool connectedStatus = false;
    ite_filter_call_method(f->videostream->Fuvc, ITE_FILTER_UVC_GETCONNECTED,
                           (void *)&connectedStatus);
    return connectedStatus;
}

#endif
