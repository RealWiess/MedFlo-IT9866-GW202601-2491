/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2009 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

#if !defined(WIN32)
#if (CFG_CHIP_FAMILY != 9830) && (CFG_CHIP_FAMILY != 9870) && (CFG_CHIP_FAMILY != 9880)
#define VP_BLUR_EFFECT
#endif
#endif

static const char blurName[] = "ITUBlur";
#ifdef VP_BLUR_EFFECT
#include "isp/mmp_isp.h"
static ISP_DEVICE gIspDev;
#define KERNEL_SIZE 4
#else
#define KERNEL_SIZE 7
#endif
#define HALF_KERNEL KERNEL_SIZE / 2

#ifdef VP_BLUR_EFFECT
static void isp_scale_down(uint32_t* src, uint32_t* dst, int src_width, int src_height, int src_pitch, int dst_width, int dst_height, int dst_pitch)
{
	int                 result = 0;
	MMP_ISP_OUTPUT_INFO outInfo  = {0};
    MMP_ISP_SHARE       ispInput = {0};
	MMP_ISP_CORE_INFO   ISPCOREINFO = {0};

	ispInput.width    = src_width;
    ispInput.height   = src_height;
	ispInput.addrY    = (uint32_t)src;
	ispInput.pitchY   = src_pitch;
	ispInput.format   = MMP_ISP_IN_RGB888;

	outInfo.addrY   = (uint32_t)dst;
	outInfo.addrU   = (uint32_t)dst + dst_width * dst_height;
	outInfo.addrV   = (uint32_t)dst + dst_width * dst_height * 2;
	outInfo.width     = dst_width;
    outInfo.height    = dst_height;
    outInfo.pitchY    = dst_pitch;
	outInfo.pitchUv   = dst_pitch;
	outInfo.format    = MMP_ISP_OUT_YUV444;
	
	result = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
	if (result)
    {
        (void)printf("mmpIspInitialize() error (0x%x) !!\n", result);
    }

	ISPCOREINFO.EnPreview    = false;
	ISPCOREINFO.PreScaleSel  = MMP_ISP_PRESCALE_NORMAL;
	result = mmpIspSetCore(gIspDev, &ISPCOREINFO);
	if (result)
	{
		printf("mmpIspSetCore() error (0x%x) !!\n", result);
	}
	result = mmpIspSetMode(gIspDev, MMP_ISP_MODE_TRANSFORM);
	if (result)
	{
		printf("mmpIspSetMode() error (0x%x) !! \n", result);
	}
	mmpIspSetOutputWindow(gIspDev, &outInfo);
	mmpIspSetVideoWindow(gIspDev, 0, 0, outInfo.width, outInfo.height);
	mmpIspEnableBlur(gIspDev);

	result = mmpIspPlayImageProcess(gIspDev, &ispInput);
	if (result)
	{
		printf("mmpIspPlayImageProcess() error (0x%x) !!\n", result);
	}
	result = mmpIspWaitEngineIdle(gIspDev);
	if (result)
	{
		printf("mmpIspWaitEngineIdle() error (0x%x) !!\n", result);
	}
	mmpIspTerminate(&gIspDev);
}

static void isp_scale_up(uint32_t* src, uint32_t* dst, int src_width, int src_height, int src_pitch, int dst_width, int dst_height, int dst_pitch)
{
	int                 result = 0;
	MMP_ISP_OUTPUT_INFO outInfo  = {0};
    MMP_ISP_SHARE       ispInput = {0};
	MMP_ISP_CORE_INFO   ISPCOREINFO = {0};

	ispInput.width    = src_width;
    ispInput.height   = src_height;
	ispInput.addrY    = (uint32_t)src;
	ispInput.addrU    = (uint32_t)src + src_width * src_height;
	ispInput.addrV    = (uint32_t)src + src_width * src_height * 2;
	ispInput.pitchY   = src_pitch;
	ispInput.pitchUv  = src_pitch;
	ispInput.format   = MMP_ISP_IN_YUV444;

	outInfo.addrRGB   = (uint32_t)dst;
	outInfo.width     = dst_width;
    outInfo.height    = dst_height;
    outInfo.pitchRGB  = dst_pitch;
	outInfo.format    = MMP_ISP_OUT_DITHER565A;
	
	result = mmpIspInitialize(&gIspDev, MMP_ISP_CORE_0);
	if (result)
    {
        (void)printf("mmpIspInitialize() error (0x%x) !!\n", result);
    }

	ISPCOREINFO.EnPreview    = false;
	ISPCOREINFO.PreScaleSel  = MMP_ISP_PRESCALE_NORMAL;
	result = mmpIspSetCore(gIspDev, &ISPCOREINFO);
	if (result)
	{
		printf("mmpIspSetCore() error (0x%x) !!\n", result);
	}
	result = mmpIspSetMode(gIspDev, MMP_ISP_MODE_TRANSFORM);
	if (result)
	{
		printf("mmpIspSetMode() error (0x%x) !! \n", result);
	}
	mmpIspSetOutputWindow(gIspDev, &outInfo);
	mmpIspSetVideoWindow(gIspDev, 0, 0, outInfo.width, outInfo.height);
	mmpIspEnableBlur(gIspDev);

	result = mmpIspPlayImageProcess(gIspDev, &ispInput);
	if (result)
	{
		printf("mmpIspPlayImageProcess() error (0x%x) !!\n", result);
	}
	result = mmpIspWaitEngineIdle(gIspDev);
	if (result)
	{
		printf("mmpIspWaitEngineIdle() error (0x%x) !!\n", result);
	}
	mmpIspTerminate(&gIspDev);
}

#endif

static void blur_impl_horizontal_pass_generic(uint32_t* src, uint32_t* dst, int width, int height)
{
    uint32_t* o_src = src;
    int        row, column;
    for (row = 0; row < height; row++)
    {
        for (column = 0; column < width; column++, src++)
        {
            uint32_t rgbaIn[KERNEL_SIZE + 1];

            // handle borders
            int      leftBorder = column < HALF_KERNEL;
            int      rightBorder = column > width - HALF_KERNEL;
            int      i = 0, k;
            uint32_t acc[4] = { 0 };

            if (leftBorder)
            {
                // for kernel size 7x7 and column == 0, we have:
                // x x x P0 P1 P2 P3
                // first loop mirrors P{0..3} to fill x's,
                // second one loads P{0..3}
                for (; i < HALF_KERNEL - column; i++)
                {
                    rgbaIn[i] = *(src + (HALF_KERNEL - i));
                }
                for (; i < KERNEL_SIZE; i++)
                {
                    rgbaIn[i] = *(src - (HALF_KERNEL - i));
                }
            }
            else if (rightBorder)
            {
                for (; i < width - column; i++)
                {
                    rgbaIn[i] = *(src + i);
                }
                for (k = 0; i < KERNEL_SIZE; i++, k++)
                {
                    rgbaIn[i] = *(src - k);
                }
            }
            else
            {
                for (; i < KERNEL_SIZE; i++)
                {
                    if ((uintptr_t)((src + 4 * i - HALF_KERNEL) + 1) > (uintptr_t)(o_src + (height * width)))
                    {
                        break;
                    }
                    rgbaIn[i] = *(src + i - HALF_KERNEL);
                }
            }

            for (i = 0; i < KERNEL_SIZE; i++)
            {
                acc[0] += (rgbaIn[i] & 0xFF000000U) >> 24U;
                acc[1] += (rgbaIn[i] & 0x00FF0000U) >> 16U;
                acc[2] += (rgbaIn[i] & 0x0000FF00U) >> 8U;
                acc[3] += (rgbaIn[i] & 0x000000FFU) >> 0U;
            }

            for (i = 0; i < 4; i++)
            {
                acc[i] = (uint32_t)(1.0f * (float)(acc[i]) / (float)KERNEL_SIZE);
            }

            *(dst + height * column + row) = (acc[0] << 24) | (acc[1] << 16) | (acc[2] << 8) | (acc[3] << 0);
        }
    }
}

void ituBlurExit (ITUWidget * widget)
{
    ITUBlur * blur = (ITUBlur *)widget;
    assert(blur);
    ITU_ASSERT_THREAD();

    if (blur->maskSurf)
    {
        ituSurfaceRelease(blur->maskSurf);
        blur->maskSurf = NULL;
    }

    ituWidgetExitImpl(&blur->widget);
}

bool ituBlurClone (ITUWidget * widget, ITUWidget ** cloned)
{
    ITUBlur * newBlur;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget * newWidget = malloc(sizeof(ITUBlur));
        if (newWidget == NULL)
        {
            return false;
        }

        (void)memcpy(newWidget, widget, sizeof(ITUBlur));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned                                                                  = newWidget;
    }

    newBlur           = (ITUBlur *)*cloned;
    newBlur->maskSurf = NULL;

    return ituWidgetCloneImpl(widget, cloned);
}

bool ituBlurUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool      result = false;
    ITUBlur * blur   = (ITUBlur *)widget;
    assert(blur);

    if (ev == ITU_EVENT_LAYOUT)
    {
        if (blur->maskSurf)
        {
            ituSurfaceRelease(blur->maskSurf);
            blur->maskSurf = NULL;
        }
    }
    result |= ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);

    return widget->visible ? result : false;
}

#ifdef VP_BLUR_EFFECT
void ituBlurDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    int            destx, desty;
    ITUBlur *      blur = (ITUBlur *)widget;
    ITURectangle * rect = (ITURectangle *)&widget->rect;
    assert(blur);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;

    if (!blur->maskSurf && blur->factor > 0)
    {
        ITUColor eColor = { 0,0,0,0 };
        ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, ITU_ARGB8888, NULL, 0);
		ITUSurface * out_surf = ituCreateSurface(rect->width, rect->height, 0, ITU_RGB565, NULL, 0);
        if (surf != NULL && out_surf != NULL)
        {
        	ITUSurface * preSurf = NULL;
            //ITUSurface * temp_surf1 = ituCreateSurface(rect->width/4, rect->height/4, 0, ITU_ARGB8888, NULL, 0);
			//ITUSurface * temp_surf2 = ituCreateSurface(rect->width/4, rect->height/4, 0, ITU_ARGB8888, NULL, 0);

            int temp_rect_width = rect->width, temp_rect_height = rect->height;
            //ITUSurface * temp_surf1 = NULL, * temp_surf2 = NULL;

            // multiple of 16
            if ((temp_rect_width / 4) % 4)
            {
                temp_rect_width = (4 - (temp_rect_width / 4) % 4) * 4 + temp_rect_width;
            }
            if ((temp_rect_height / 4) % 4)
            {
                temp_rect_height = (4 - (temp_rect_height / 4) % 4) * 4 + temp_rect_height;
            }

            {
                uint32_t * srcPtr;
				uint32_t * dstPtr;
				
                ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);
				
                srcPtr = (uint32_t *)ituLockSurface(surf, 0, 0, rect->width, rect->height);
				
                if (srcPtr)
                {				
					int tempbuf_size = temp_rect_width/4 * temp_rect_height/4 * 3;
					uint32_t * tmpPtr1 = memalign(32, ITH_ALIGN_UP(tempbuf_size, 32));
					uint32_t * tmpPtr2 = memalign(32, ITH_ALIGN_UP(tempbuf_size, 32));
					uint32_t * tmpPtr3 = memalign(32, ITH_ALIGN_UP(tempbuf_size, 32));
                    if (tmpPtr1 != NULL && tmpPtr2 != NULL && tmpPtr3 != NULL)
                    {
                        int i;
						ithFlushDCacheRange(tmpPtr1, ITH_ALIGN_UP(tempbuf_size, 32));
						ithFlushDCacheRange(tmpPtr2, ITH_ALIGN_UP(tempbuf_size, 32));
						ithFlushDCacheRange(tmpPtr3, ITH_ALIGN_UP(tempbuf_size, 32));
						
						isp_scale_down(srcPtr, tmpPtr1, surf->width, surf->height, surf->pitch, temp_rect_width/4, temp_rect_height/4, temp_rect_width/4);
						ithInvalidateDCacheRange(tmpPtr1, ITH_ALIGN_UP(tempbuf_size, 32));						
						memcpy(tmpPtr3, tmpPtr1, temp_rect_width/4 * temp_rect_height/4 * 3); //memcpy YUV444
						
                        for (i = 0; i < blur->iter; i++)
                        {
                        	//blur_impl_horizontal_pass_generic(tmpPtr1, tmpPtr2, rect->width/4, rect->height/4);
                            //blur_impl_horizontal_pass_generic(tmpPtr2, tmpPtr1, rect->height/4, rect->width/4);
                            mmpIspBlurAverage((uint32_t)tmpPtr2, (uint32_t)tmpPtr1, temp_rect_width/4, temp_rect_height/4);
							mmpIspBlurAverage((uint32_t)tmpPtr1, (uint32_t)tmpPtr2, temp_rect_width/4, temp_rect_height/4);
                        }

						ithInvalidateDCacheRange(tmpPtr1, ITH_ALIGN_UP(tempbuf_size, 32));
						memcpy(tmpPtr3, tmpPtr1, temp_rect_width/4 * temp_rect_height/4); //memcpy Y plane
						ithFlushDCacheRange(tmpPtr3, ITH_ALIGN_UP(tempbuf_size, 32));

						dstPtr = (uint32_t *)ituLockSurface(out_surf, 0, 0, rect->width, rect->height);
						isp_scale_up(tmpPtr3, dstPtr, temp_rect_width/4, temp_rect_height/4, temp_rect_width/4, out_surf->width, out_surf->height, out_surf->pitch);
						ithInvalidateDCacheRange(dstPtr, out_surf->pitch*out_surf->height);

						free(tmpPtr1);
						free(tmpPtr2);
						free(tmpPtr3);
						
                    }
                }
            }

			blur->maskSurf = out_surf;
			ituUnlockSurface(surf);
			ituUnlockSurface(out_surf);
			
			ituDestroySurface(surf);
        }
    }

    if (blur->maskSurf)
    {
        ituBitBlt(dest, destx, desty, rect->width, rect->height, blur->maskSurf, 0, 0);
        if (widget->flags & ITU_PROGRESS)
        {
            ituSurfaceRelease(blur->maskSurf);
            blur->maskSurf = NULL;
        }
    }
    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}
#else
void ituBlurDraw (ITUWidget * widget, ITUSurface * dest, int x, int y, uint8_t alpha)
{
    int            destx, desty;
    ITUBlur *      blur = (ITUBlur *)widget;
    ITURectangle * rect = (ITURectangle *)&widget->rect;
    assert(blur);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;

    if (!blur->maskSurf && blur->factor > 0)
    {
        ITUColor eColor = { 0,0,0,0 };
        ITUSurface * surf = ituCreateSurface(rect->width, rect->height, 0, ITU_ARGB8888, NULL, 0);
        if (surf)
        {
            ITUSurface * surf2 = ituCreateSurface(rect->width, rect->height, 0, ITU_ARGB8888, NULL, 0);
            ituColorFill(surf, 0, 0, rect->width, rect->height, &eColor);
            if (surf2)
            {
                uint32_t * srcPtr;
                ituColorFill(surf2, 0, 0, rect->width, rect->height, &eColor);
                ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);
                srcPtr = (uint32_t *)ituLockSurface(surf, 0, 0, rect->width, rect->height);
                if (srcPtr)
                {
                    uint32_t * destPtr = (uint32_t *)ituLockSurface(surf2, 0, 0, rect->width, rect->height);
                    if (destPtr)
                    {
                        int i;
                        for (i = 0; i < blur->iter; i++)
                        {
                            blur_impl_horizontal_pass_generic(srcPtr, destPtr, rect->width, rect->height);
                            blur_impl_horizontal_pass_generic(destPtr, srcPtr, rect->height, rect->width);
                        }
                        ituUnlockSurface(surf2);
                        blur->maskSurf = surf;
                    }
                    ituUnlockSurface(surf);
                }
                ituDestroySurface(surf2);
            }
        }
    }

    if (blur->maskSurf)
    {
        ituBitBlt(dest, destx, desty, rect->width, rect->height, blur->maskSurf, 0, 0);
        if (widget->flags & ITU_PROGRESS)
        {
            ituSurfaceRelease(blur->maskSurf);
            blur->maskSurf = NULL;
        }
    }
    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}
#endif

void ituBlurInit (ITUBlur * blur)
{
    assert(blur);
    ITU_ASSERT_THREAD();

    (void)memset(blur, 0, sizeof(ITUBlur));

    ituWidgetInit(&blur->widget);

    ituWidgetSetType(blur, ITU_LAYER);
    ituWidgetSetName(blur, blurName);
    ituWidgetSetExit(blur, ituBlurExit);
    ituWidgetSetClone(blur, ituBlurClone);
    ituWidgetSetUpdate(blur, ituBlurUpdate);
    ituWidgetSetDraw(blur, ituBlurDraw);
}

void ituBlurLoad (ITUBlur * blur, uint32_t base)
{
    assert(blur);

    ituWidgetLoad((ITUWidget *)blur, base);
    ituWidgetSetExit(blur, ituBlurExit);
    ituWidgetSetClone(blur, ituBlurClone);
    ituWidgetSetUpdate(blur, ituBlurUpdate);
    ituWidgetSetDraw(blur, ituBlurDraw);
}
