#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "itu_cfg.h"
#include "ite/ith.h"
#include "ite/itu.h"
#include "ite/itp.h"

#if defined(CFG_M2D_ENABLE)
    #include "gfx/gfx.h"
#endif

#include "ite/itv.h"
#if defined(CFG_TWO_WAY_VIDEO_ENABLE)
#include "ite/itv_2way.h"
#endif
#include "itu_private.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define MAX_UI_BUFFER_COUNT 2
// #if defined(CFG_VIDEO_ENABLE) && !defined(CFG_FFMPEG_H264_SW)
//     #define FRAME_BUFFER_COUNT 4
// #endif
#define VIDEO_SURFACE_COUNT 3
//=============================================================================
//                              Macro Definition
//=============================================================================

//=============================================================================
//                              Structure Definition
//=============================================================================

//=============================================================================
//                              Static Data Definition
//=============================================================================
static bool     g_inited;
static uint32_t g_vid_buff_addr[VIDEO_SURFACE_COUNT];
ITUSurface *    VideoSurf[VIDEO_SURFACE_COUNT];
#if defined(CFG_TWO_WAY_VIDEO_ENABLE)
static bool     g_2way_inited = false;
static uint32_t g_2way_vid_buff_addr[VIDEO_SURFACE_COUNT];
ITUSurface *    VideoSurf_2way[VIDEO_SURFACE_COUNT];
#endif
//=============================================================================
//                              Private Function Declaration
//=============================================================================
static void     VideoInit (void);
static void     VideoExit (void);

//=============================================================================
//                              Public Function Definition
//=============================================================================
void            ituFrameFuncInit (void)
{
#ifndef CFG_FFMPEG_H264_SW
    int         width;
    int         height;
    int         pitch;
    int         i;
    ITURotation rot;
#endif

    if (g_inited)
    {
        return;
    }

#ifndef CFG_FFMPEG_H264_SW
#ifdef CFG_UVC_ENABLE
    width  = CFG_UVC_WIDTH;
    height = CFG_UVC_HEIGHT;
    pitch  = width*2;
#else
	width  = CFG_LCD_WIDTH;
	height = CFG_LCD_HEIGHT;
	if (width % 16)
    {
        width = (width / 16 + 1) * 16;
    }
	if (height % 16)
    {
        height = (height / 16 + 1) * 16;
    }
	pitch = width*2;
#endif
    if (!g_vid_buff_addr[0])
    {
        int size           = pitch * height * VIDEO_SURFACE_COUNT;
        g_vid_buff_addr[0] = itpVmemAlloc(size);

        assert(g_vid_buff_addr[0]);

        for (i = 1; i < VIDEO_SURFACE_COUNT; ++i)
        {
            g_vid_buff_addr[i] = g_vid_buff_addr[i - 1] + pitch * height;
        }
    #ifdef _DEBUG
        for (i = 0; i < VIDEO_SURFACE_COUNT; ++i)
        {
            (void)printf("video buffer(%d, %X)\n", i, g_vid_buff_addr[i]);
        }
    #endif
        // (void)memset(g_vid_buff_addr[0], 0, size);
    }

#endif

#ifndef CFG_FFMPEG_H264_SW
    VideoInit();

    for (i = 0; i < VIDEO_SURFACE_COUNT; i++)
    {
        VideoSurf[i] =
            ituCreateSurface(width, height, pitch, ITU_RGB565, (const uint8_t *)g_vid_buff_addr[i], ITU_STATIC);
        itv_set_vidSurf_buf((uint8_t *)g_vid_buff_addr[i], i);
        // (void)printf("i = %d, g_vid_buff_addr = 0x%x, width = %d, height = %d, pitch = %d\n", i, g_vid_buff_addr[i], width,
        // height, pitch);
    }

#endif

    g_inited = true;
}

void ituFrameFuncExit (void)
{
    uint8_t i;

    if (!g_inited)
    {
        return;
    }

#ifndef CFG_FFMPEG_H264_SW

    VideoExit();

    for (i = 0; i < VIDEO_SURFACE_COUNT; i++)
    {
        ituDestroySurface(VideoSurf[i]);
    }

    if (g_vid_buff_addr[0])
    {
        itpVmemFree(g_vid_buff_addr[0]);
        (void)memset(g_vid_buff_addr, 0, sizeof(g_vid_buff_addr));
    }
#endif
    g_inited = false;
}

#if defined(CFG_TWO_WAY_VIDEO_ENABLE)
void ituFrameFuncInit_2way (void)
{
    int         width;
    int         height;
    int         pitch;
    int         i;
    ITURotation rot;

    if (g_2way_inited)
    {
        return;
    }


    width  = CFG_LCD_WIDTH;
	height = CFG_LCD_HEIGHT;
	if (width % 16)
    {
        width = (width / 16 + 1) * 16;
    }
	if (height % 16)
    {
        height = (height / 16 + 1) * 16;
    }
	pitch = width*2;

    if (!g_2way_vid_buff_addr[0])
    {
        int size           = pitch * height * VIDEO_SURFACE_COUNT;
        g_2way_vid_buff_addr[0] = itpVmemAlloc(size);

        assert(g_2way_vid_buff_addr[0]);

        for (i = 1; i < VIDEO_SURFACE_COUNT; ++i)
        {
            g_2way_vid_buff_addr[i] = g_2way_vid_buff_addr[i - 1] + pitch * height;
        }
    #ifdef _DEBUG
        for (i = 0; i < VIDEO_SURFACE_COUNT; ++i)
        {
            (void)printf("video buffer(%d, %X)\n", i, g_2way_vid_buff_addr[i]);
        }
    #endif
        // (void)memset(g_vid_buff_addr[0], 0, size);
    }

	itv_init_2way();

    for (i = 0; i < VIDEO_SURFACE_COUNT; i++)
    {
        VideoSurf_2way[i] =
            ituCreateSurface(width, height, pitch, ITU_RGB565, (const uint8_t *)g_2way_vid_buff_addr[i], ITU_STATIC);
        itv_set_vidSurf_buf_2way((uint8_t *)g_2way_vid_buff_addr[i], i);
    }
	
    g_2way_inited = true;
}

void ituFrameFuncExit_2way (void)
{
    uint8_t i;

    if (!g_2way_inited)
    {
        return;
    }

	itv_stop_vidSurf_anchor_2way();
    itv_deinit_2way();

    for (i = 0; i < VIDEO_SURFACE_COUNT; i++)
    {
        ituDestroySurface(VideoSurf_2way[i]);
    }

    if (g_2way_vid_buff_addr[0])
    {
        itpVmemFree(g_2way_vid_buff_addr[0]);
        (void)memset(g_2way_vid_buff_addr, 0, sizeof(g_2way_vid_buff_addr));
    }

    g_2way_inited = false;
}
#endif

void ituDrawVideoSurface (ITUSurface * dest, int startX, int startY, int width, int height)
{
#ifndef CFG_FFMPEG_H264_SW
    int      index;
    ITUColor color;

    color.alpha = 0;
    color.red   = 0;
    color.green = 0;
    color.blue  = 0;
#endif
    if (!g_inited)
    {
        return;
    }

#ifndef CFG_FFMPEG_H264_SW
    index = itv_get_vidSurf_index();
    // (void)printf("index = %d\n", index);

    if (index == -1 || index == -2)
    {
        ituColorFill(dest, startX, startY, width, height, &color);
    }
    else
    {
    #ifdef CFG_UVC_ENABLE
        uint32_t x, y, w, h;
        itv_get_video_window(&x, &y, &w, &h);
        ituStretchBlt(dest, startX, startY, width, height, VideoSurf[index], 0, 0, w, h);
    #else
        ituBitBlt(dest, startX, startY, width, height, VideoSurf[index], startX, startY);
    #endif
    }
#endif
}

#if defined(CFG_TWO_WAY_VIDEO_ENABLE)
void ituDrawVideoSurface_2way (ITUSurface * dest, int startX, int startY, int width, int height)
{
    int      index;
    ITUColor color;

    color.alpha = 0;
    color.red   = 0;
    color.green = 0;
    color.blue  = 0;

    if (!g_2way_inited)
    {
        return;
    }

    index = itv_get_vidSurf_index_2way();

    if (index == -1 || index == -2)
    {
        ituColorFill(dest, startX, startY, width, height, &color);
    }
    else
    {
        ituBitBlt(dest, startX, startY, width, height, VideoSurf_2way[index], startX, startY);
    }
}
#endif

int ituDrawVideoSurface_ex (ITUSurface * dest, int startX, int startY, int width, int height,int angle,ITUSurface* TempSurf)
{
#ifndef CFG_FFMPEG_H264_SW
    int      index;
    ITUColor color;

    color.alpha = 0;
    color.red   = 0;
    color.green = 0;
    color.blue  = 0;
#endif
    if (!g_inited)
    {
        return -1;
    }

#ifndef CFG_FFMPEG_H264_SW
    index = itv_get_vidSurf_index();

    if (index != -1 && index != -2)
    {
    #ifdef CFG_UVC_ENABLE
        uint32_t x, y, w, h;
        itv_get_video_window(&x, &y, &w, &h);
        ituStretchBlt(dest, startX, startY, width, height, VideoSurf[index], 0, 0, w, h);
    #else
        if (angle != 0)
        {
            if (TempSurf != NULL)
		    {
			    M2dSurface* m2dtempSurf= (M2dSurface*)dest;
			    ituBitBlt(TempSurf, 0, 0, width, height, VideoSurf[index], startX, startY);
			    gfxSurfaceSetQuality(m2dtempSurf->m2dSurf, GFX_QUALITY_FASTER);
			    ituRotate(dest, startX, startY, TempSurf, TempSurf->width / 2, TempSurf->height / 2, (float)angle, 1.0f, 1.0f);
		    }
        }
        else
        {
        	ituBitBlt(dest, startX, startY, width, height, VideoSurf[index], startX, startY);
        }
    #endif
    }
    else
    {
        return -1;
    }
#endif
    return 0;
}

//=============================================================================
//                              Private Function Definition
//=============================================================================
static void VideoInit (void)
{
    itv_init();
}

static void VideoExit (void)
{
    /* release dbuf & itv */
    itv_stop_vidSurf_anchor();
    itv_flush_dbuf();
    itv_deinit();
}
