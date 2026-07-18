/*
 * Copyright (c) 2015 ITE technology Corp. All Rights Reserved.
 */
/** @file itv.h
 * Used to do H/W video overlay
 *
 * @author I-Chun Lai
 * @version 0.1
 */
#ifndef ITV_H
#define ITV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ite/ith.h"
#include "ite/itu.h"
#include "isp/mmp_isp.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define ITV_MAX_NDBUF    3
#define ITV_MAX_DISP_BUF 3

//=============================================================================
//                              Structure Definition
//=============================================================================
typedef struct
{
    uint8_t  *ya;       /// address of Y decoded video buffer
    uint8_t  *ua;       /// address of U decoded video buffer
    uint8_t  *va;       /// address of V decoded video buffer
    uint32_t src_w;     /// width of decoded video buffer
    uint32_t src_h;     /// height of decoded video buffer
#ifdef CFG_ITV_VP_DUAL_WINDOWS
    uint8_t  *ya_2nd;   /// 2nd address of Y decoded video buffer
    uint8_t  *ua_2nd;   /// 2nd address of U decoded video buffer
    uint8_t  *va_2nd;   /// 2nd address of V decoded video buffer
    uint32_t src_w_2nd; /// 2nd width of decoded video buffer
    uint32_t src_h_2nd; /// 2nd height of decoded video buffer   
#endif
    int      bidx;
    uint32_t pitch_y;   /// pitch of Y
    uint32_t pitch_uv;  /// pitch of UV
    uint32_t format;    /// YUV format. see MMP_ISP_INFORMAT.
    
} ITV_DBUF_PROPERTY;

typedef struct
{
    uint32_t startX;
    uint32_t startY;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;

    /// color key value ([A]lpha, [R]ed, [G]reen, [B]lue)
    uint32_t colorKeyR;
    uint32_t colorKeyG;
    uint32_t colorKeyB;
#if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)
    uint32_t EnAlphaBlend;
#endif
    uint32_t constAlpha;
} ITV_UI_PROPERTY;

typedef struct
{
    uint32_t startX;
    uint32_t startY;
    uint32_t width;
    uint32_t height;
} ITV_DISPLAY_WND;

//=============================================================================
//                Public Function Definition
//=============================================================================
int      itv_init(void);
int      itv_deinit(void);
int      itv_set_pb_mode(int pb_mode);


#ifdef CFG_TIME_FRAME_FUN_ENABLE
int itv_osd_setup_base(int id, int bid, uint8_t *base);
int itv_osd_enable(int id, int enable);
int itv_update_osdbuf_anchor(int id, ITV_UI_PROPERTY *osdprop);
int itv_update_osddbuf_anchor(ITV_DBUF_PROPERTY *prop);
uint8_t *itv_get_osdbuf_anchor(int id);
void itv_flush_osdbuf(int id);
void itv_set_osd_window(uint32_t startX, uint32_t startY, uint32_t width, uint32_t height);
#else
/* FRAME FUNCTION */
int itv_ff_setup_base(int id, int bid, uint8_t *base);
int itv_ff_enable(int id, int enable);
uint8_t *itv_get_uibuf_anchor(int id);
int itv_update_uibuf_anchor(int id, ITV_UI_PROPERTY *uiprop);
void itv_flush_uibuf(int id);
#endif

uint8_t *itv_get_dbuf_anchor(void);
int itv_update_dbuf_anchor(ITV_DBUF_PROPERTY *prop);
void itv_flush_dbuf(void);
void itv_set_rotation(ITURotation rot);
void itv_set_video_window(uint32_t startX, uint32_t startY, uint32_t width, uint32_t height);
#ifdef CFG_ITV_VP_DUAL_WINDOWS
void itv_set_2nd_video_window(uint32_t startX, uint32_t startY, uint32_t width, uint32_t height);
void itv_enable_video_window_dual_mode(bool enabled);
#endif
ISP_RESULT itv_enable_isp_feature(MMP_ISP_CAPS cap);
void itv_set_vidSurf_buf(uint8_t *addr, uint8_t index);
int itv_get_vidSurf_index(void);
int itv_update_vidSurf_anchor(void);
void itv_stop_vidSurf_anchor(void);
ITURotation itv_get_rotation(void);
bool itv_get_new_video(void);
void itv_set_color_correction(MMP_ISP_COLOR_CTRL colorCtrl, int value);
void itv_set_isp_onfly(MMP_ISP_SHARE isp_share, bool interlaced);
void itv_enable_video_center_mode(bool enabled);
void itv_update_video_window(int width, int height);
#ifdef FAST_DISPLAY_ENABLE
void itv_enable_video_lowlatency_mode (bool enabled);
#endif

#if ((CFG_CHIP_FAMILY == 9860) && defined(CFG_ITV_VP_HIGH_QUALITY))
void itv_hw_overlay_task(MMP_ISP_OUTPUT_INFO *out_info);
#endif


#ifdef __cplusplus
}
#endif

#endif // ITV_H