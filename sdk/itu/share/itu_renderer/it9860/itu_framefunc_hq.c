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

#if defined(CFG_VIDEO_ENABLE) && !defined(CFG_FFMPEG_H264_SW)
    #include "ite/itv.h"
    #include "ith/ith_video.h"
#endif

#include "itu_private.h"
#include "isp/mmp_isp.h"

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Macro Definition
//=============================================================================

//=============================================================================
//                              Structure Definition
//=============================================================================

//=============================================================================
//                              Static Data Definition
//=============================================================================
static bool g_inited;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
#if defined(CFG_VIDEO_ENABLE) && !defined(CFG_FFMPEG_H264_SW)
static void VideoInit (void);
static void VideoExit (void);
#endif // CFG_VIDEO_ENABLE

//=============================================================================
//                              Public Function Definition
//=============================================================================
void ituFrameFuncInit (void)
{
#ifndef CFG_FFMPEG_H264_SW
#endif

    if (g_inited)
    {
        return;
    }

#ifndef CFG_FFMPEG_H264_SW
#endif

#ifndef CFG_FFMPEG_H264_SW
    VideoInit();
#endif

    g_inited = true;
}

void ituFrameFuncExit (void)
{
    if (!g_inited)
    {
        return;
    }

#ifndef CFG_FFMPEG_H264_SW
    VideoExit();
#endif

    g_inited = false;
}

void ituDrawVideoSurface (ITUSurface * dest, int startX, int startY, int width, int height)
{
#ifndef CFG_FFMPEG_H264_SW
    int                 index    = 0;
    ITUColor            color    = {0};
    MMP_ISP_OUTPUT_INFO out_info = {0};
    M2dSurface *        dstSurf  = (M2dSurface *)dest;
#endif

    if (!g_inited)
    {
        return;
    }

#ifndef CFG_FFMPEG_H264_SW
    out_info.startX   = startX;
    out_info.startY   = startY;
    out_info.width    = width;
    out_info.height   = height;
    out_info.pitchRGB = width * 2;
    out_info.addrRGB  = dstSurf->m2dSurf->imageData;
    out_info.format   = MMP_ISP_OUT_DITHER565A;
    itv_hw_overlay_task(&out_info);

    index = itv_get_vidSurf_index();

    if (index == -1 || index == -2)
    {
        ituColorFill(dest, startX, startY, width, height, &color);
    }
#endif
}

int ituDrawVideoSurface_ex (ITUSurface * dest, int startX, int startY, int width, int height)
{
#ifndef CFG_FFMPEG_H264_SW
    int                 index    = 0;
    ITUColor            color    = {0};
    MMP_ISP_OUTPUT_INFO out_info = {0};
    M2dSurface *        dstSurf  = (M2dSurface *)dest;
#endif

    if (!g_inited)
    {
        return;
    }

#ifndef CFG_FFMPEG_H264_SW
    out_info.startX   = startX;
    out_info.startY   = startY;
    out_info.width    = width;
    out_info.height   = height;
    out_info.pitchRGB = width * 2;
    out_info.addrRGB  = dstSurf->m2dSurf->imageData;
    out_info.format   = MMP_ISP_OUT_DITHER565A;
    itv_hw_overlay_task(&out_info);

    index = itv_get_vidSurf_index();

    if (index == -1 || index == -2)
    {
        return -1;
    }
#endif

    return 0;
}

//=============================================================================
//                              Private Function Definition
//=============================================================================
#if defined(CFG_VIDEO_ENABLE) && !defined(CFG_FFMPEG_H264_SW)
static void VideoInit (void)
{
    #if (CFG_CHIP_FAMILY == 9860)
    ithVideoInit(NULL);
    #endif

    itv_init();
}

static void VideoExit (void)
{
    itv_deinit();

    #if (CFG_CHIP_FAMILY == 9860)
    (void)printf("%s(%d)\n", __FUNCTION__, __LINE__);
    ithVideoExit();
    (void)printf("%s(%d)\n", __FUNCTION__, __LINE__);
    #endif
}
#endif // CFG_VIDEO_ENABLE
