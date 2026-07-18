/*
 * Copyright (c) 2015 ITE technology Corp. All Rights Reserved.
 */
/** @file itv.c
 * Used to do H/W video overlay
 *
 * @author I-Chun Lai
 * @version 0.1
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include "ite/ith.h"
#include "ite/itp.h"
#include "ite/itv_2way.h"
#include "isp/mmp_isp.h"
#include "fc_sync.h"
//#include "ith/ith_video.h"
#if (CFG_CHIP_FAMILY != 970 && CFG_CHIP_FAMILY != 9860)
    #include "ite/ite_m2d.h"
    #include "m2d/m2d_graphics.h"
#endif
#ifdef CFG_ITV_VP_DUAL_WINDOWS
    #include "gfx/gfx.h"
#endif
// =============================================================================
//                              Constant Definition
// =============================================================================
#if (defined(WIN32) && defined(_DEBUG))
    #define DEBUG_FLIP
#endif

// #define FRAME_FUNCTION_ENABLE

#define ITV_FF_NSET             2
#define ITV_FF_NBUF             3

#define MAX_ROTATE_BUFFER_COUNT 2

#define ITV_MAX_VSBUF           3

// =============================================================================
//                              Macro Definition
// =============================================================================
#define WAIT_UNTIL(expr)            while (!(expr)) {usleep(20000);}
#define MIDDLE_WAIT_UNTIL(expr)     while (!(expr)) {usleep(3000);}
#define SHORT_WAIT_UNTIL(expr)      while (!(expr)) {usleep(1000);}

#define QUEUE_IS_FULL(w, r, size)   ((((w) + 1) % (size)) == (r))

#define IS_VERTICAL_DISPLAY()       ((gRotation) == ITU_ROT_90 || (gRotation) == ITU_ROT_270)
#define IS_ROTATE_DISPLAY()         ((gRotation) != ITU_ROT_0)

// =============================================================================
//                              Structure Definition
// =============================================================================
typedef struct
{
    uint8_t         * uibuf[ITV_FF_NBUF];
    ITV_UI_PROPERTY_2WAY uiprop[ITV_FF_NBUF];
    volatile int    ui_ridx;
    volatile int    ui_widx;
} ITV_STRC_FF;

typedef struct
{
    /* DECODE VIDEO BUFFER */
    bool                bVideo;
    bool                video_update;
    uint8_t             * dbuf[ITV_MAX_NDBUF];          // decoded video buffer address
    ITV_DBUF_PROPERTY_2WAY   dbufprop[ITV_MAX_NDBUF];        // property of the decoded video buffer
    ITV_DBUF_PROPERTY_2WAY   blank_yuv_frame;
    ITV_DBUF_PROPERTY_2WAY   * curr_video;
    volatile int        disp_buf_ridx;
    volatile int        disp_buf_widx;

    /* VIDEO SURFACE INDEX */
    bool                check_video_surf;
    bool                check_capvideo_surf;
    volatile int        vid_surf_cur_idx;
    volatile int        vid_surf_ridx;
    volatile int        vid_surf_widx;

    /* VIDEO WINDOW POSITION */
    ITV_DISPLAY_WND_2WAY     vid_window;
#ifdef CFG_ITV_VP_DUAL_WINDOWS
    ITV_DISPLAY_WND_2WAY     vid_window_2nd;
    bool                bDualWindowMode;
#endif
    bool                bCenterMode;
    int                 video_width;
    int                 video_height;

    /* PLAYER STC SYNC */
    // FC_STRC_SYNC      itv_sync;

    /* FLUSH COMMAND */
    volatile int        pcmd_flush_dbuf;
    volatile int        pcmd_flush_uibuf[ITV_FF_NSET];  // frame function

    /* MTAL */
    int                 mtal_pb_mode_isr;               // indicate whether the player currently is in playback mode or not.
    volatile int        mtal_pb_mode_u;

    /* FRAME FUNCTION */
    bool                ui_update;
    int                 ff_mode_isr[ITV_FF_NSET];
    volatile int        ff_mode_u[ITV_FF_NSET];
    ITV_STRC_FF         ff[ITV_FF_NSET];
    volatile int        ff_setbase;
    volatile int        ff_setbase_id;
    volatile int        ff_setbase_bid;
    volatile uint8_t    * ff_setbase_base;
} ITV_STRC_RESOURCE;

typedef struct LCD_FLIP_TAG
{
    sem_t       tScanlineEvent;
    uint32_t    bInited;
    pthread_t   hw_overlay_tid;
    pthread_t   lcd_isr_tid;
} LCD_FLIP;

// =============================================================================
//                              Extern Reference
// =============================================================================

// =============================================================================
//                              Global Data Definition
// =============================================================================
static ITV_STRC_RESOURCE    itv_rcs_2way; // a blank 16 x 16 frame will be defalut input
static ITV_DBUF_PROPERTY_2WAY    gCapPROP;
static ISP_DEVICE           gIspDev;
static LCD_FLIP             gtFlipCtrl;
static ITURotation          gRotation = ITU_ROT_0;

static bool                 g_stop_itv_hw_overlay_task;
static uint32_t             g_blank_buf_2way;
static sem_t                gtISPEvent;
static int                  g_lcd_index;
static int                  g_lcd_index_counter;

#if (CFG_CHIP_FAMILY != 970 && CFG_CHIP_FAMILY != 9860)
static uint32_t         gRotateBuffer[MAX_ROTATE_BUFFER_COUNT];
static MMP_M2D_SURFACE  gRoateSurf[MAX_ROTATE_BUFFER_COUNT];
static MMP_M2D_SURFACE  * _gRoateSurf;
static MMP_M2D_SURFACE  _gLcdSurf[ITV_MAX_VSBUF];
#endif

// =============================================================================
//                              Private Function Declaration
// =============================================================================
static void
itv_lcdISR (void * arg);
static void
itv_isp0ISR (void * arg);
static void
itv_isp1ISR (void * arg);
static void
itv_change_display_format ();
static int
itv_check_vidSurf_anchor (void);

static int
next_lcd_index ()
{
    g_lcd_index_counter = (g_lcd_index_counter + 1) % ITV_MAX_DISP_BUF;

    return g_lcd_index_counter;
}

static void
update_ui ()
{
    int         i;
    ITV_STRC_FF * ff;
    int         widx, ridx, ridx_last;

    for (i = 0; i < ITV_FF_NSET; i++)
    {
        ff      = &(itv_rcs_2way.ff[i]);
        widx    = ff->ui_widx;
        ridx    = ff->ui_ridx;

        if (widx != ridx)
        {
            if (itv_rcs_2way.ff_mode_isr[i] == 1)
            {
                if (widx)
                {
                    ridx_last = widx - 1;
                }
                else
                {
                    ridx_last = ITV_FF_NBUF - 1;
                }

                if (itv_check_vidSurf_anchor() == 1)
                {
                    itv_rcs_2way.ui_update = true;
                }

                switch (i)
                {
                    case 0:
                        mmpIspEnable(gIspDev, MMP_ISP_FRAME_FUNCTION_0);
                        mmpIspSetFrameFunction(
                            gIspDev,
                            MMP_ISP_FRAME_FUNCTION_0,
                            ff->uibuf[ridx_last],
                            ff->uiprop[ridx_last].startX,
                            ff->uiprop[ridx_last].startY,
                            ff->uiprop[ridx_last].width,
                            ff->uiprop[ridx_last].height,
                            ff->uiprop[ridx_last].pitch,
                            ff->uiprop[ridx_last].colorKeyR,
                            ff->uiprop[ridx_last].colorKeyG,
                            ff->uiprop[ridx_last].colorKeyB,
                            ff->uiprop[ridx_last].EnAlphaBlend,
                            ff->uiprop[ridx_last].constAlpha,
                            MMP_PIXEL_FORMAT_RGB565,
                            ridx_last);
                        break;

                    case 1:
                        mmpIspEnable(gIspDev, MMP_ISP_FRAME_FUNCTION_1);
                        mmpIspSetFrameFunction(
                            gIspDev,
                            MMP_ISP_FRAME_FUNCTION_1,
                            ff->uibuf[ridx_last],
                            ff->uiprop[ridx_last].startX,
                            ff->uiprop[ridx_last].startY,
                            ff->uiprop[ridx_last].width,
                            ff->uiprop[ridx_last].height,
                            ff->uiprop[ridx_last].pitch,
                            ff->uiprop[ridx_last].colorKeyR,
                            ff->uiprop[ridx_last].colorKeyG,
                            ff->uiprop[ridx_last].colorKeyB,
                            ff->uiprop[ridx_last].EnAlphaBlend,
                            ff->uiprop[ridx_last].constAlpha,
                            MMP_PIXEL_FORMAT_RGB565,
                            ridx_last);
                        break;

                    default:
                        break;
                }
            }
            else
            {
                switch (i)
                {
                    case 0:
                        mmpIspDisable(gIspDev, MMP_ISP_FRAME_FUNCTION_0);
                        break;

                    case 1:
                        mmpIspDisable(gIspDev, MMP_ISP_FRAME_FUNCTION_1);
                        break;

                    default:
                        break;
                }
            }
        }
        else
        {
            itv_rcs_2way.pcmd_flush_uibuf[i] = 0;

            if (itv_rcs_2way.ff_mode_isr[i] == 0)
            {
                switch (i)
                {
                    case 0:
                        mmpIspDisable(gIspDev, MMP_ISP_FRAME_FUNCTION_0);
                        break;

                    case 1:
                        mmpIspDisable(gIspDev, MMP_ISP_FRAME_FUNCTION_1);
                        break;

                    default:
                        break;
                }
            }
        }
    }
}

static void
flip_lcd (
    int lcd_index)
{
    MMP_ISP_OUTPUT_INFO out_info    = {0};
    MMP_ISP_SHARE       isp_share   = {0};
    ITURotation         rot;

#ifdef FRAME_FUNCTION_ENABLE
    if (itv_rcs_2way.curr_video != &itv_rcs_2way.blank_yuv_frame)
    {
        if ((NULL == itv_rcs_2way.curr_video) ||
                (NULL == itv_rcs_2way.curr_video->ya) ||
                (NULL == itv_rcs_2way.curr_video->ua) ||
                (NULL == itv_rcs_2way.curr_video->va) ||
                (0 == itv_rcs_2way.curr_video->src_w) ||
                (0 == itv_rcs_2way.curr_video->src_h) ||
                (0 == itv_rcs_2way.curr_video->pitch_y) ||
                (0 == itv_rcs_2way.curr_video->pitch_uv))
        {
            itv_rcs_2way.curr_video = &itv_rcs_2way.blank_yuv_frame;
            break;
        }

        if (itv_rcs_2way.curr_video->format == 0)
        {
            itv_rcs_2way.curr_video->format = MMP_ISP_IN_YUV420;
        }
    }
#endif

    isp_share.addrY     = (uint32_t)itv_rcs_2way.curr_video->ya;
    isp_share.addrU     = (uint32_t)itv_rcs_2way.curr_video->ua;
    isp_share.addrV     = (uint32_t)itv_rcs_2way.curr_video->va;
    isp_share.width     = itv_rcs_2way.curr_video->src_w;
    isp_share.height    = itv_rcs_2way.curr_video->src_h;
    isp_share.pitchY    = itv_rcs_2way.curr_video->pitch_y;
    isp_share.pitchUv   = itv_rcs_2way.curr_video->pitch_uv;
    isp_share.format    = itv_rcs_2way.curr_video->format;

#if (CFG_CHIP_FAMILY != 970 && CFG_CHIP_FAMILY != 9860)
    if (IS_ROTATE_DISPLAY())
    {
        _gRoateSurf         = (_gRoateSurf == &gRoateSurf[0]) ? &gRoateSurf[1] : &gRoateSurf[0];
        out_info.addrRGB    = (MMP_UINT32)((M2D_SURFACE *)(*_gRoateSurf))->baseScanPtr;
    }
    else
#endif
    {
        lcd_index           = next_lcd_index();
        out_info.addrRGB    = (uint32_t)itv_rcs_2way.dbuf[lcd_index];
    }

#if (CFG_CHIP_FAMILY != 970 && CFG_CHIP_FAMILY != 9860)
    if (IS_VERTICAL_DISPLAY())
    {
        out_info.width      = ithLcdGetHeight();
        out_info.height     = ithLcdGetWidth();
        out_info.pitchRGB   = out_info.width * (ithLcdGetPitch() / ithLcdGetWidth());
    }
    else
#endif
    {
        rot = itv_get_rotation_2way();

        switch (rot)
        {
            case ITU_ROT_90:
            case ITU_ROT_270:
                out_info.width      = ithLcdGetHeight();
                out_info.height     = ithLcdGetWidth();
                out_info.pitchRGB   = ithLcdGetPitch() * ithLcdGetHeight() / ithLcdGetWidth();
                break;

            default:
                out_info.width      = ithLcdGetWidth();
                out_info.height     = ithLcdGetHeight();
                out_info.pitchRGB   = ithLcdGetPitch();
                break;
        }
    }
    out_info.format = MMP_ISP_OUT_DITHER565A;

    if (isp_share.format != MMP_ISP_IN_RGB565)
    {
        if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE))
        {
            out_info.width  = itv_rcs_2way.vid_window.width;
            out_info.height = itv_rcs_2way.vid_window.height;

            rot             = itv_get_rotation_2way();

            switch (rot)
            {
                case ITU_ROT_90:
                case ITU_ROT_270:
                    out_info.pitchRGB = ithLcdGetPitch() * ithLcdGetHeight() / ithLcdGetWidth();
                    break;

                default:
                    out_info.pitchRGB = ithLcdGetPitch();
                    break;
            }

            out_info.addrRGB    = (uint32_t)itv_rcs_2way.dbuf[0];
            out_info.startX     = itv_rcs_2way.vid_window.startX;
            out_info.startY     = itv_rcs_2way.vid_window.startY;

            if (mmpIspQuery(gIspDev, MMP_ISP_OUTPUT_FIELD_MODE))
            {
                itv_rcs_2way.vid_window.height   /= 2;
                out_info.height             /= 2;
                out_info.startY             /= 2;
            }

            mmpIspSetOutputWindow(gIspDev, &out_info);
            mmpIspSetVideoWindow(gIspDev,
                    0,
                    0,
                    itv_rcs_2way.vid_window.width,
                    itv_rcs_2way.vid_window.height);
        }
        else
        {
            mmpIspSetOutputWindow(gIspDev, &out_info);
            mmpIspSetVideoWindow(gIspDev,
                    itv_rcs_2way.vid_window.startX,
                    itv_rcs_2way.vid_window.startY,
                    itv_rcs_2way.vid_window.width,
                    itv_rcs_2way.vid_window.height);
        }

        // #ifdef CFG_LCD_PQ_TUNING
#if defined(CFG_LCD_PQ_TUNING) || defined(CFG_JPG_INTERACTIVE_ENABLE)
        pthread_mutex_lock(&ISP_CORE_0_MUTEX);
#endif

        mmpIspPlayImageProcess(gIspDev, &isp_share);

        if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE))
        {
            return;
        }

        // #ifndef CFG_LCD_PQ_TUNING
#if !defined(CFG_LCD_PQ_TUNING) && !defined(CFG_JPG_INTERACTIVE_ENABLE)
    #ifdef _WIN32
        SHORT_WAIT_UNTIL(mmpIspIsEngineIdle(gIspDev));
    #else
        sem_wait(&gtISPEvent);
    #endif
#else
        SHORT_WAIT_UNTIL(mmpIspIsEngineIdle(gIspDev));
        pthread_mutex_unlock(&ISP_CORE_0_MUTEX);
#endif

#ifdef CFG_ITV_VP_DUAL_WINDOWS
        // process 2nd video or not
        if (itv_rcs_2way.bDualWindowMode)
        {
    #if defined(CFG_LCD_PQ_TUNING) || defined(CFG_JPG_INTERACTIVE_ENABLE)
            pthread_mutex_lock(&ISP_CORE_0_MUTEX);
    #endif

            isp_share.addrY     = (uint32_t)(itv_rcs_2way.curr_video->ya_2nd);
            isp_share.addrU     = (uint32_t)(itv_rcs_2way.curr_video->ua_2nd);
            isp_share.addrV     = (uint32_t)(itv_rcs_2way.curr_video->va_2nd);
            isp_share.width     = itv_rcs_2way.curr_video->src_w_2nd;
            isp_share.height    = itv_rcs_2way.curr_video->src_h_2nd;

            out_info.startX     = itv_rcs_2way.vid_window_2nd.startX;
            out_info.startY     = itv_rcs_2way.vid_window_2nd.startY;
            out_info.width      = itv_rcs_2way.vid_window_2nd.width;
            out_info.height     = itv_rcs_2way.vid_window_2nd.height;

            mmpIspSetOutputWindow(gIspDev, &out_info);
            mmpIspSetVideoWindow(gIspDev, 0, 0, itv_rcs_2way.vid_window_2nd.width, itv_rcs_2way.vid_window_2nd.height);

            mmpIspPlayImageProcess(gIspDev, &isp_share);
            gfxwaitEngineIdle();
            mmpIspWaitEngineIdle(gIspDev);

    #if defined(CFG_LCD_PQ_TUNING) || defined(CFG_JPG_INTERACTIVE_ENABLE)
            pthread_mutex_unlock(&ISP_CORE_0_MUTEX);
    #endif
        }
#endif
    }

#if (CFG_CHIP_FAMILY != 970 && CFG_CHIP_FAMILY != 9860)
    if (IS_ROTATE_DISPLAY())
    {
        MMP_M2D_SURFACE * m2dtempSurf;
        float           angle;
        MMP_INT         dX;
        MMP_INT         dY;
        MMP_INT         cX;
        MMP_INT         cY;

        lcd_index = next_lcd_index();

        switch (gRotation)
        {
            case ITU_ROT_90:
                angle   = 90.0f;
                dX      = ithLcdGetWidth() - 1;
                dY      = 0;
                cX      = 0;
                cY      = 0;
                break;

            case ITU_ROT_270:
                angle   = 270.0f;
                dX      = 0;
                dY      = 0;
                cX      = ithLcdGetHeight();
                cY      = 0;
                break;

            default:
                angle = 0;
                break;
        }

        m2dtempSurf = &_gLcdSurf[lcd_index];
    #ifdef CFG_M2D_ENABLE
        mmpM2dTransformations(
            *m2dtempSurf,
            dX,
            dY,
            *_gRoateSurf,
            cX,
            cY,
            angle,
            1.0f);

        mmpM2dWaitIdle();
    #endif
    }
#endif

    g_lcd_index = lcd_index;
}

static void
itv_lcdISR (
    void * arg __attribute__((__unused__)))
{
    if (gtFlipCtrl.bInited)
    {
        sem_post(&gtFlipCtrl.tScanlineEvent);
    }

    ithLcdIntrClear();
}

static void
itv_isp0ISR (
    void * arg __attribute__((__unused__)))
{
    sem_post(&gtISPEvent);
    mmpIspClearInterrupt(gIspDev);

    if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE) && !itv_rcs_2way.check_capvideo_surf)
    {
        itv_rcs_2way.check_capvideo_surf = true;
    }
}

static void
itv_isp1ISR (
    void * arg __attribute__((__unused__)))
{
    sem_post(&gtISPEvent);
    mmpIspClearInterrupt(gIspDev);

    if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE) && !itv_rcs_2way.check_capvideo_surf)
    {
        itv_rcs_2way.check_capvideo_surf = true;
    }
}

static void
_itv_hw_overlay_task (
    void)
{
    int         i;
    ITV_STRC_FF * ff;
    int         widx, ridx;

    if (itv_rcs_2way.mtal_pb_mode_u != -1)
    {
#ifndef FRAME_FUNCTION_ENABLE
        if ((itv_rcs_2way.mtal_pb_mode_isr == 0) && (itv_rcs_2way.mtal_pb_mode_u == 1))
        {
            itv_rcs_2way.mtal_pb_mode_u = 2;
        }
#endif

        itv_rcs_2way.mtal_pb_mode_isr    = itv_rcs_2way.mtal_pb_mode_u;
        itv_rcs_2way.mtal_pb_mode_u      = -1;
    }

#ifndef FRAME_FUNCTION_ENABLE
    if (itv_rcs_2way.mtal_pb_mode_isr == 2)
    {
        itv_rcs_2way.check_video_surf    = true;
        itv_rcs_2way.mtal_pb_mode_isr    = 1;
        itv_rcs_2way.disp_buf_ridx       = 0;
        itv_rcs_2way.disp_buf_widx       = 1;
        itv_rcs_2way.vid_surf_ridx       = itv_rcs_2way.vid_surf_widx = 0;

        for (i = 0; i < ITV_FF_NSET; i++)
        {
            ff          = &(itv_rcs_2way.ff[i]);
            ff->ui_widx = ff->ui_ridx = 0;
        }

        if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE))
        {
            memcpy((void *)&itv_rcs_2way.dbufprop[itv_rcs_2way.disp_buf_ridx], (void *)&gCapPROP, sizeof(ITV_DBUF_PROPERTY_2WAY));
        }
        else
        {
            memcpy((void *)&itv_rcs_2way.dbufprop[itv_rcs_2way.disp_buf_ridx], (void *)&itv_rcs_2way.blank_yuv_frame, sizeof(ITV_DBUF_PROPERTY_2WAY));
        }
    }
#endif

#ifdef FRAME_FUNCTION_ENABLE
    #if defined(CFG_LCD_ENABLE)
    if (g_lcd_index >= 0)
    {
        ithLcdSwFlip(g_lcd_index);

        if (gtFlipCtrl.bInited)
        {
            sem_wait(&gtFlipCtrl.tScanlineEvent);
        }
        g_lcd_index = -1;
    }
    #endif
#endif

    for (i = 0; i < ITV_FF_NSET; i++)
    {
        if (itv_rcs_2way.ff_mode_u[i] != -1)
        {
            itv_rcs_2way.ff_mode_isr[i]  = itv_rcs_2way.ff_mode_u[i];
            itv_rcs_2way.ff_mode_u[i]    = -1;
        }
    }

    if (itv_rcs_2way.ff_setbase == 1)
    {
        const int   id      = itv_rcs_2way.ff_setbase_id;
        const int   bid     = itv_rcs_2way.ff_setbase_bid;
        ITV_STRC_FF * ff    = &(itv_rcs_2way.ff[id]);

        ff->uibuf[bid]      = (uint8_t *)itv_rcs_2way.ff_setbase_base;
        itv_rcs_2way.ff_setbase  = 0;
    }

    update_ui();

    if (itv_rcs_2way.mtal_pb_mode_isr == 0)
    {
        memcpy((void *)&itv_rcs_2way.dbufprop[itv_rcs_2way.disp_buf_ridx], (void *)&itv_rcs_2way.blank_yuv_frame, sizeof(ITV_DBUF_PROPERTY_2WAY));
    }

    if ((itv_rcs_2way.disp_buf_ridx != itv_rcs_2way.disp_buf_widx) && (itv_check_vidSurf_anchor() == 1))
    {
        itv_rcs_2way.curr_video      = &itv_rcs_2way.dbufprop[itv_rcs_2way.disp_buf_ridx];
        itv_rcs_2way.video_update    = true;
    }
    else
    {
        itv_rcs_2way.pcmd_flush_dbuf = 0;    // set this flag to 0 means all queued display buffers have been flushed.
    }

    if (itv_rcs_2way.video_update || itv_rcs_2way.ui_update)
    {
        flip_lcd(itv_rcs_2way.disp_buf_ridx);

        if (itv_rcs_2way.video_update)
        {
            itv_rcs_2way.disp_buf_ridx = (itv_rcs_2way.disp_buf_ridx + 1) % ITV_MAX_DISP_BUF;
        }

        if (itv_rcs_2way.ui_update)
        {
            for (i = 0; i < ITV_FF_NSET; i++)
            {
                ff      = &(itv_rcs_2way.ff[i]);
                widx    = ff->ui_widx;
                ridx    = ff->ui_ridx;

                if (itv_rcs_2way.ff_mode_isr[i] == 1)
                {
                    ff->ui_ridx = (ridx + 1) % ITV_FF_NBUF;
                }
            }
        }

#ifndef FRAME_FUNCTION_ENABLE
        itv_rcs_2way.vid_surf_widx   = (itv_rcs_2way.vid_surf_widx + 1) % ITV_MAX_VSBUF;
#endif

        itv_rcs_2way.video_update    = false;
        itv_rcs_2way.ui_update       = false;

        if ((itv_rcs_2way.curr_video)->src_w != 16)
        {
            itv_rcs_2way.bVideo = true;
        }
        else
        {
            itv_rcs_2way.bVideo = false;
        }
    }
}

#ifdef _WIN32
static g_stop_lcd_isr_task;

static void *
lcd_isr_task (
    void * arg)
{
    #ifdef DEBUG_FLIP
    (void)printf("[LCD ISR] Enter\n");
    #endif

    g_stop_lcd_isr_task = false;
    while (!g_stop_lcd_isr_task)
    {
        itv_lcdISR(arg);
        usleep(16000);
    }

    #ifdef DEBUG_FLIP
    (void)printf("[LCD ISR] Leave\n");
    #endif

    pthread_exit(NULL);

    return (void *)0;
}
#endif

static void *
itv_hw_overlay_task (
    void * arg)
{
    g_stop_itv_hw_overlay_task = false;

    while (!g_stop_itv_hw_overlay_task || itv_rcs_2way.mtal_pb_mode_isr != 0)
    {
        _itv_hw_overlay_task();
        usleep(1000);
    }

    pthread_exit(NULL);

    return (void *)0;
}

static void
itv_change_display_format ()
{
    MMP_ISP_CORE_INFO   ISPCOREINFO = {0};
    MMP_ISP_OUTPUT_INFO outInfo     = {0};
    int                 width       = ithLcdGetWidth();
    int                 height      = ithLcdGetHeight();
    ITURotation         rot;

    rot = itv_get_rotation_2way();

    switch (rot)
    {
        case ITU_ROT_90:
        case ITU_ROT_270:
            width   = ithLcdGetHeight();
            height  = ithLcdGetWidth();
            break;

        default:
            width   = ithLcdGetWidth();
            height  = ithLcdGetHeight();
            break;
    }

    mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);

    ISPCOREINFO.EnPreview   = false;
    ISPCOREINFO.PreScaleSel = MMP_ISP_PRESCALE_NORMAL;
    mmpIspSetCore(gIspDev, &ISPCOREINFO);
    mmpIspSetMode(gIspDev, MMP_ISP_MODE_PLAY_VIDEO);
    mmpIspSetDisplayWindow(gIspDev, 0, 0, width, height);
    mmpIspSetOutputFormat(gIspDev, MMP_ISP_OUT_DITHER565A);

#ifdef _WIN32
    mmpIspDisableInterrupt(gIspDev);
#else
    mmpIspEnableInterrupt(gIspDev);
#endif

#ifdef FRAME_FUNCTION_ENABLE
    ithLcdSwFlip(ithLcdGetFlip());
    ithLcdDisableHwFlip();
#endif

    if (!gtFlipCtrl.bInited)
    {
        gtFlipCtrl.bInited = !sem_init(&gtFlipCtrl.tScanlineEvent, 0, 1);
    }

    sem_init(&gtISPEvent, 0, 1);
    sem_wait(&gtISPEvent);

    if (gtFlipCtrl.bInited)
    {
        g_lcd_index = -1;
        pthread_create(&gtFlipCtrl.hw_overlay_tid, NULL, itv_hw_overlay_task, NULL);

#ifndef WIN32
    #ifdef FRAME_FUNCTION_ENABLE
        uint32_t lcd_height = ithLcdGetHeight();
        ithIntrDisableIrq(ITH_INTR_LCD);
        ithLcdIntrClear();
        ithIntrClearIrq(ITH_INTR_LCD);
        ithIntrRegisterHandlerIrq(ITH_INTR_LCD, itv_lcdISR, NULL);
        ithLcdIntrSetScanLine1(lcd_height);
        ithLcdIntrSetScanLine2(lcd_height);
        ithLcdIntrCtrlDisable(ITH_LCD_INTR_OUTPUT2);
        ithLcdIntrCtrlDisable(ITH_LCD_INTR_FIELDMODE2);
        ithLcdIntrCtrlDisable(ITH_LCD_INTR_OUTPUT1);
        ithLcdIntrCtrlDisable(ITH_LCD_INTR_FIELDMODE1);
        ithLcdIntrCtrlEnable(ITH_LCD_INTR_OUTPUT2);
        ithIntrEnableIrq(ITH_INTR_LCD);
        ithLcdIntrEnable();
    #endif

        // #ifndef CFG_LCD_PQ_TUNING
    #if !defined(CFG_LCD_PQ_TUNING) && !defined(CFG_JPG_INTERACTIVE_ENABLE)
        mmpISPRegisterIRQ(gIspDev, itv_isp0ISR);
        // mmpISPRegisterIRQ(gIspDev, itv_isp1ISR);
    #endif
#else
        pthread_create(&gtFlipCtrl.lcd_isr_tid, NULL, lcd_isr_task, NULL);
#endif
    }
}

int
itv_init_2way (
    void)
{
    int i;

    (void)printf("ITV# 2way init !\n");

    if (ITV_MAX_NDBUF > 3)
    {
        (void)printf("ERROR# invalid ITV_MAX_NDBUF (%d), %s:%d !\n", ITV_MAX_NDBUF, __FILE__, __LINE__);
        return -1;
    }

    g_blank_buf_2way = itpVmemAlloc(16 * 16 * 2);

    memset((void *)&itv_rcs_2way, 0, sizeof(ITV_STRC_RESOURCE));
    memset((void *)&gCapPROP, 0, sizeof(ITV_DBUF_PROPERTY_2WAY));

    itv_rcs_2way.blank_yuv_frame.ya  = (uint8_t *)g_blank_buf_2way;
    itv_rcs_2way.blank_yuv_frame.ua  = (uint8_t *)g_blank_buf_2way + 256;
    itv_rcs_2way.blank_yuv_frame.va  = (uint8_t *)g_blank_buf_2way + 256 + 64;

    memset((void *) itv_rcs_2way.blank_yuv_frame.ya, 0x10, 256);
    memset((void *) itv_rcs_2way.blank_yuv_frame.ua, 0x80, 64);
    memset((void *) itv_rcs_2way.blank_yuv_frame.va, 0x80, 64);

    itv_rcs_2way.blank_yuv_frame.src_w       = 16;
    itv_rcs_2way.blank_yuv_frame.src_h       = 16;
    itv_rcs_2way.blank_yuv_frame.pitch_y     = 16;
    itv_rcs_2way.blank_yuv_frame.pitch_uv    = 16;
    itv_rcs_2way.blank_yuv_frame.format      = MMP_ISP_IN_YUV420;
    itv_rcs_2way.curr_video                  = &itv_rcs_2way.blank_yuv_frame;

#ifdef CFG_LCD_ENABLE
    itv_rcs_2way.dbuf[0]                     = (uint8_t *)ithLcdGetBaseAddrA();
    itv_rcs_2way.dbuf[1]                     = (uint8_t *)ithLcdGetBaseAddrB();
    itv_rcs_2way.dbuf[2]                     = (uint8_t *)ithLcdGetBaseAddrC();
    g_lcd_index_counter                 = ithLcdGetFlip();
#endif

    itv_rcs_2way.mtal_pb_mode_u              = -1;
    itv_rcs_2way.vid_surf_cur_idx            = -1;

    for (i = 0; i < ITV_FF_NSET; i++)
    {
        itv_rcs_2way.ff_mode_u[i] = -1;
    }

    itv_change_display_format();

    // fc_init_sync(&itv_rcs_2way.itv_sync);    // sync module should be done in the player

    return 0;
}

int
itv_deinit_2way (
    void)
{
    (void)printf("ITV# 2way de-init !\n");

    if (gtFlipCtrl.bInited)
    {
        g_stop_itv_hw_overlay_task = true;
        pthread_join(gtFlipCtrl.hw_overlay_tid, NULL);

#ifndef WIN32
    #ifdef FRAME_FUNCTION_ENABLE
        ithLcdIntrDisable();
    #endif
#else
        g_stop_lcd_isr_task = true;
        pthread_join(gtFlipCtrl.lcd_isr_tid, NULL);
#endif

        sem_destroy(&(gtFlipCtrl.tScanlineEvent));
        memset(&gtFlipCtrl, 0, sizeof(gtFlipCtrl));

        sem_destroy(&gtISPEvent);
        memset(&gtISPEvent, 0, sizeof(gtISPEvent));
    }

    if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE))
    {
        mmpIspResetEngine(gIspDev);
        ithIspResetAllReg();
        ithIspResetReg(ITH_ISP_CORE0);
    }

    mmpIspTerminate(&gIspDev);

    // fc_deinit_sync(&itv_rcs_2way.itv_sync);

    memset((void *)&itv_rcs_2way, 0, sizeof(ITV_STRC_RESOURCE));
    memset((void *)&gCapPROP, 0, sizeof(ITV_DBUF_PROPERTY_2WAY));

    itpVmemFree(g_blank_buf_2way);

    return 0;
}

uint8_t *
itv_get_dbuf_anchor_2way (
    void)
{
    if (QUEUE_IS_FULL(itv_rcs_2way.disp_buf_widx, itv_rcs_2way.disp_buf_ridx, ITV_MAX_DISP_BUF))
    {
        return NULL;
    }

    return itv_rcs_2way.dbuf[itv_rcs_2way.disp_buf_widx];
}

int
itv_update_dbuf_anchor_2way (
    ITV_DBUF_PROPERTY_2WAY * prop)
{
    if (QUEUE_IS_FULL(itv_rcs_2way.disp_buf_widx, itv_rcs_2way.disp_buf_ridx, ITV_MAX_DISP_BUF))
    {
        return -1;
    }

    memcpy((void *)&itv_rcs_2way.dbufprop[itv_rcs_2way.disp_buf_widx], (void *)prop, sizeof(ITV_DBUF_PROPERTY_2WAY));
    itv_rcs_2way.disp_buf_widx = (itv_rcs_2way.disp_buf_widx + 1) % ITV_MAX_DISP_BUF;

    return 0;
}

void
itv_flush_dbuf_2way (
    void)
{
    itv_rcs_2way.pcmd_flush_dbuf = 1;

    WAIT_UNTIL(itv_rcs_2way.pcmd_flush_dbuf == 0);
}

void
itv_set_vidSurf_buf_2way (
    uint8_t * addr,
    uint8_t index)
{
    itv_rcs_2way.dbuf[index] = addr;
}

bool
itv_get_new_video_2way (
    void)
{
    if (itv_rcs_2way.vid_surf_ridx != itv_rcs_2way.vid_surf_widx)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#ifndef CFG_LCD_PQ_TUNING

int
itv_get_vidSurf_index_2way (
    void)
{
    unsigned int value = 0;

    if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE))
    {
        value   = ithReadRegA((0xD0300000 + 0x3FC));  // read last finish index of VP
        value   = (value & 0x300) >> 8;

        if ((itv_rcs_2way.mtal_pb_mode_isr != 1) || !itv_rcs_2way.check_capvideo_surf)
        {
            return -1;
        }
        else if (!itv_rcs_2way.bVideo)
        {
            return -2;
        }

        return value;
    }
    else
    {
        if (itv_rcs_2way.vid_surf_ridx != itv_rcs_2way.vid_surf_widx)
        {
            itv_rcs_2way.vid_surf_ridx       = (itv_rcs_2way.vid_surf_ridx + 1) % ITV_MAX_VSBUF;
            itv_rcs_2way.vid_surf_cur_idx    = g_lcd_index;
        }

        if (itv_rcs_2way.mtal_pb_mode_isr != 1)
        {
            return -1;
        }
        else if (!itv_rcs_2way.bVideo)
        {
            return -2;
        }

        return itv_rcs_2way.vid_surf_cur_idx;
    }
}
#else

int
itv_get_vidSurf_index_2way (
    void)
{
    unsigned int value = 0;

    if (mmpIspQuery(gIspDev, MMP_ISP_CAP_ONFLY_MODE))
    {
        return -1;
    }
    else
    {
        if (itv_rcs_2way.vid_surf_ridx != itv_rcs_2way.vid_surf_widx)
        {
            itv_rcs_2way.vid_surf_cur_idx = g_lcd_index;
        }

        if (itv_rcs_2way.mtal_pb_mode_isr != 1)
        {
            return -1;
        }
        else if (!itv_rcs_2way.bVideo)
        {
            return -2;
        }

        return itv_rcs_2way.vid_surf_cur_idx;
    }
}

int
itv_update_vidSurf_anchor_2way (
    void)
{
    unsigned int value = 0;

    if (itv_rcs_2way.vid_surf_ridx != itv_rcs_2way.vid_surf_widx)
    {
        itv_rcs_2way.vid_surf_ridx       = (itv_rcs_2way.vid_surf_ridx + 1) % ITV_MAX_VSBUF;
        itv_rcs_2way.vid_surf_cur_idx    = g_lcd_index;
    }

    if (itv_rcs_2way.mtal_pb_mode_isr != 1)
    {
        return -1;
    }
    else if (!itv_rcs_2way.bVideo)
    {
        return -2;
    }

    return itv_rcs_2way.vid_surf_cur_idx;
}
#endif

static int
itv_check_vidSurf_anchor (
    void)
{
#ifndef FRAME_FUNCTION_ENABLE
    if (QUEUE_IS_FULL(itv_rcs_2way.vid_surf_widx, itv_rcs_2way.vid_surf_ridx, ITV_MAX_VSBUF) && itv_rcs_2way.check_video_surf)
    {
        return -1;
    }
#endif

    return 1;
}

void
itv_stop_vidSurf_anchor_2way (
    void)
{
#ifndef FRAME_FUNCTION_ENABLE
    itv_rcs_2way.check_video_surf = false;
#endif
}

int
itv_ff_setup_base_2way (
    int     id,
    int     bid,
    uint8_t * base)
{
    itv_rcs_2way.ff_setbase_id   = id;
    itv_rcs_2way.ff_setbase_bid  = bid;
    itv_rcs_2way.ff_setbase_base = base;
    itv_rcs_2way.ff_setbase      = 1;

    WAIT_UNTIL(itv_rcs_2way.ff_setbase == 0);

    return 0;
}

int
itv_ff_enable_2way (
    int id,
    int enable)
{
    (void)printf("ITV# %s, id(%d):%d +\n", __func__, id, enable);

    itv_rcs_2way.ff_mode_u[id] = enable;
    WAIT_UNTIL(itv_rcs_2way.ff_mode_u[id] == -1);

    (void)printf("ITV# %s, id(%d):%d -\n", __func__, id, enable);

    return 0;
}

uint8_t *
itv_get_uibuf_anchor_2way (
    int id)
{
    ITV_STRC_FF * ff    = &(itv_rcs_2way.ff[id]);
    const int   widx    = ff->ui_widx;
    const int   ridx    = ff->ui_ridx;

    if (QUEUE_IS_FULL(widx, ridx, ITV_FF_NBUF))
    {
        return NULL;
    }

    return ff->uibuf[widx];
}

int
itv_update_uibuf_anchor_2way (
    int             id,
    ITV_UI_PROPERTY_2WAY * uiprop)
{
    ITV_STRC_FF * ff    = &(itv_rcs_2way.ff[id]);
    const int   widx    = ff->ui_widx;
    const int   ridx    = ff->ui_ridx;

    if (QUEUE_IS_FULL(widx, ridx, ITV_FF_NBUF))
    {
        return -1;
    }

    memcpy((void *)&(ff->uiprop[widx]), (void *)uiprop, sizeof(ITV_UI_PROPERTY_2WAY));
    ff->ui_widx = (widx + 1) % ITV_FF_NBUF;

    return 0;
}

void
itv_flush_uibuf_2way (
    int id)
{
    itv_rcs_2way.pcmd_flush_uibuf[id] = 1;

    WAIT_UNTIL(itv_rcs_2way.pcmd_flush_uibuf[id] == 0);
}

// MTAL pb mode, 1 means in playback mode, 0 means not in playback mode
int
itv_set_pb_mode_2way (
    int pb_mode)
{
    (void)printf("%s(%d) pb_mode(%d)\n", __FUNCTION__, __LINE__, pb_mode);

    WAIT_UNTIL(itv_rcs_2way.mtal_pb_mode_u == -1);
    itv_rcs_2way.mtal_pb_mode_u = pb_mode;
    SHORT_WAIT_UNTIL(itv_rcs_2way.mtal_pb_mode_u == -1);

    (void)printf("%s(%d)\n", __FUNCTION__, __LINE__);

    return itv_rcs_2way.mtal_pb_mode_u;
}

ITURotation
itv_get_rotation_2way (
    void)
{
    return gRotation;
}

void
itv_set_rotation_2way (
    ITURotation rot)
{
    gRotation = rot;

#if (CFG_CHIP_FAMILY != 970 && CFG_CHIP_FAMILY != 9860)
    if (IS_ROTATE_DISPLAY() && (!gRotateBuffer[0]))
    {
        int i;
        int size = ithLcdGetPitch() * ithLcdGetHeight() * 2;

        gRotateBuffer[0] = itpVmemAlloc(size);
        for (i = 1; i < MAX_ROTATE_BUFFER_COUNT; i++)
        {
            gRotateBuffer[i] = gRotateBuffer[i - 1] + ithLcdGetPitch() * ithLcdGetHeight();
        }

    #ifdef CFG_M2D_ENABLE
        for (i = 0; i < MAX_ROTATE_BUFFER_COUNT; i++)
        {
            mmpM2dCreateVirtualSurface(
                ithLcdGetHeight(),
                ithLcdGetWidth(),
                MMP_M2D_IMAGE_FORMAT_RGB565,
                (MMP_UINT8 *)gRotateBuffer[i],
                &gRoateSurf[i]);
        }
        for (i = 0; i < ITV_MAX_VSBUF; i++)
        {
            mmpM2dCreateVirtualSurface(
                ithLcdGetWidth(),
                ithLcdGetHeight(),
                MMP_M2D_IMAGE_FORMAT_RGB565,
                itv_rcs_2way.dbuf[i],
                &_gLcdSurf[i]);
        }
    #endif
    }
#endif
}

void
itv_set_video_window_2way (
    uint32_t    startX,
    uint32_t    startY,
    uint32_t    width,
    uint32_t    height)
{
	printf("YC: ##### x = %d, y = %d, width = %d, height = %d #####\n", startX, startY, width, height);
    itv_rcs_2way.vid_window.startX   = startX;
    itv_rcs_2way.vid_window.startY   = startY;
    itv_rcs_2way.vid_window.width    = width;
    itv_rcs_2way.vid_window.height   = height;
}

ISP_RESULT
itv_enable_isp_feature_2way (
    MMP_ISP_CAPS cap)
{
    return mmpIspEnable(gIspDev, cap);
}

void
itv_set_color_correction_2way (
    MMP_ISP_COLOR_CTRL  colorCtrl,
    int                 value)
{
    mmpIspSetColorCtrl(gIspDev, colorCtrl, value);
}

void
itv_set_isp_onfly_2way (
    MMP_ISP_SHARE   isp_share,
    bool            interlaced)
{
    MMP_ISP_WRITE_BUFFER_INFO   ispWbInfo   = {0};
    ITURotation                 rot         = {0};

    rot = itv_get_rotation_2way();

    if (interlaced)
    {
        isp_share.height /= 2;
        mmpIspEnable(gIspDev, MMP_ISP_OUTPUT_FIELD_MODE);
        (void)printf("ISP_OUTPUT_FIELD_MODE\n");

        switch (rot)
        {
            case ITU_ROT_90:
            case ITU_ROT_270:
                ispWbInfo.addrYRGB_1    = (uint32_t)itv_rcs_2way.dbuf[1] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * ithLcdGetHeight() / ithLcdGetWidth() * itv_rcs_2way.vid_window.startY / 2);
                ispWbInfo.addrYRGB_2    = (uint32_t)itv_rcs_2way.dbuf[2] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * ithLcdGetHeight() / ithLcdGetWidth() * itv_rcs_2way.vid_window.startY / 2);
                break;

            default:
                ispWbInfo.addrYRGB_1    = (uint32_t)itv_rcs_2way.dbuf[1] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * itv_rcs_2way.vid_window.startY / 2);
                ispWbInfo.addrYRGB_2    = (uint32_t)itv_rcs_2way.dbuf[2] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * itv_rcs_2way.vid_window.startY / 2);
                break;
        }
    }
    else
    {
        mmpIspDisable(gIspDev, MMP_ISP_OUTPUT_FIELD_MODE);
        (void)printf("DISABLE ISP_OUTPUT_FIELD_MODE\n");

        switch (rot)
        {
            case ITU_ROT_90:
            case ITU_ROT_270:
                ispWbInfo.addrYRGB_1    = (uint32_t)itv_rcs_2way.dbuf[1] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * ithLcdGetHeight() / ithLcdGetWidth() * itv_rcs_2way.vid_window.startY);
                ispWbInfo.addrYRGB_2    = (uint32_t)itv_rcs_2way.dbuf[2] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * ithLcdGetHeight() / ithLcdGetWidth() * itv_rcs_2way.vid_window.startY);
                break;

            default:
                ispWbInfo.addrYRGB_1    = (uint32_t)itv_rcs_2way.dbuf[1] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * itv_rcs_2way.vid_window.startY);
                ispWbInfo.addrYRGB_2    = (uint32_t)itv_rcs_2way.dbuf[2] + 2 * itv_rcs_2way.vid_window.startX +
                    (ithLcdGetPitch() * itv_rcs_2way.vid_window.startY);
                break;
        }
    }

    mmpIspSetWriteBufferInfo(gIspDev, &ispWbInfo);
    mmpIspSetWriteBufferNum(gIspDev, ITV_MAX_VSBUF - 1);

    mmpIspEnableCAPOnFlyMode(gIspDev);

    gCapPROP.ya         = (uint8_t *)isp_share.addrY;
    gCapPROP.ua         = (uint8_t *)isp_share.addrU;
    gCapPROP.va         = (uint8_t *)isp_share.addrV;
    gCapPROP.src_w      = (uint32_t)isp_share.width;
    gCapPROP.src_h      = (uint32_t)isp_share.height;
    gCapPROP.pitch_y    = (uint32_t)isp_share.pitchY;
    gCapPROP.pitch_uv   = (uint32_t)isp_share.pitchUv;
    gCapPROP.format     = (uint32_t)isp_share.format;
}

void
itv_enable_video_center_mode_2way (
    bool enabled)
{
    itv_rcs_2way.bCenterMode = enabled;
}

void
itv_update_video_window_2way (
    int width,
    int height)
{
    int scal_x;
    int scal_y;
    int scal_w;
    int scal_h;

    if (itv_rcs_2way.bCenterMode)
    {
        if ((width == 0) && (height == 0))
        {
            width   = itv_rcs_2way.video_width;
            height  = itv_rcs_2way.video_height;
        }
        else
        {
            itv_rcs_2way.video_width     = width;
            itv_rcs_2way.video_height    = height;
        }
        (void)printf("ori: display width = %d, height = %d\n", itv_rcs_2way.vid_window.width, itv_rcs_2way.vid_window.height);
        scal_w  = itv_rcs_2way.vid_window.width;
        scal_h  = scal_w * height / width;

        if (scal_h > itv_rcs_2way.vid_window.height)
        {
            // Cut left and right of the origin window
            scal_h  = itv_rcs_2way.vid_window.height;
            scal_w  = scal_h * width / height;
        }

        scal_x                      = itv_rcs_2way.vid_window.startX + (itv_rcs_2way.vid_window.width - scal_w) / 2;
        scal_y                      = itv_rcs_2way.vid_window.startY + (itv_rcs_2way.vid_window.height - scal_h) / 2;

        itv_rcs_2way.vid_window.startX   = scal_x;
        itv_rcs_2way.vid_window.startY   = scal_y;
        itv_rcs_2way.vid_window.width    = scal_w;
        itv_rcs_2way.vid_window.height   = scal_h;

        (void)printf("center mode: display width = %d, height = %d\n", itv_rcs_2way.vid_window.width, itv_rcs_2way.vid_window.height);
    }
}
