/*
 * Copyright (c) 2015 ITE technology Corp. All Rights Reserved.
 */
/** @file itv.h
 * Used to do H/W video overlay
 *
 * @author I-Chun Lai
 * @version 0.1
 */
#ifndef ITV_2WAY_H
#define ITV_2WAY_H

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
    
} ITV_DBUF_PROPERTY_2WAY;

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
} ITV_UI_PROPERTY_2WAY;

#ifdef CFG_IPTV_RX
typedef enum
{
    ITV_1080p60 = 0,
    ITV_1080p50,            //1
    ITV_1080i60,            //2
    ITV_1080i50,            //3
    ITV_720p60,             //4
    ITV_720p50,             //5
    ITV_576p50,             //6
    ITV_576i50,             //7
    ITV_480p60,             //8
    ITV_480i60,             //9
    ITV_LCD_800_600,        //10
    ITV_576p50_16x9,        //11
    ITV_576i50_16x9,        //12
    ITV_480p60_16x9,        //13
    ITV_480i60_16x9,        //14
    ITV_RESOLUTION_UNKNOWN, //15
} ITV_ENUM_DISPLAY_FORMAT;

typedef struct
{
    int startX;
    int startY;
    int width;
    int height;
} ITV_CLIPPING_WND;

typedef enum {
    MSG_SYS_CMD,
    MSG_CHANGE_DISPLAY_FORMAT,
    MSG_MCTL_CMD,
} ITV_EVENT_MESSAGE;

/*
 * These function pointers should be implemented in ffmpeg
 * filter components, if doing so, then itv can central
 * control behavior of filters in media controller.
 */
#define __USR_CALLBACK__
typedef struct
{
    // member
    void *component_prop;

    // behavior
    /**
     * flush - callback method
     *  Flush source stream buffer
     *
     * @caller: Media Controller
     * @maintain: URLProtocol implementation
     *
     * @param: priv_data
     *  User defined structure in URLProtocol
     */
    __USR_CALLBACK__ int (*flush)(void *component_property);
} FF_COMPONENT;

typedef enum {
    ITV_SRC_COMPONENT = 0,
    ITV_DEMUX_COMPONENT,
    ITV_DECODER_COMPONENT,
} ITV_COMPONENT_TYPE;

enum {
    MCTL_CMD_FLUSH,
};// MCTL_CMD

#endif

typedef struct
{
    uint32_t startX;
    uint32_t startY;
    uint32_t width;
    uint32_t height;
} ITV_DISPLAY_WND_2WAY;

//=============================================================================
//                Public Function Definition
//=============================================================================
int      itv_init_2way(void);
int      itv_deinit_2way(void);
int      itv_set_pb_mode_2way(int pb_mode);

/* FRAME FUNCTION */
int itv_ff_setup_base_2way(int id, int bid, uint8_t *base);
int itv_ff_enable_2way(int id, int enable);

uint8_t *itv_get_uibuf_anchor_2way(int id);
int itv_update_uibuf_anchor_2way(int id, ITV_UI_PROPERTY_2WAY *uiprop);
void itv_flush_uibuf_2way(int id);

uint8_t *itv_get_dbuf_anchor_2way(void);
int itv_update_dbuf_anchor_2way(ITV_DBUF_PROPERTY_2WAY *prop);
void itv_flush_dbuf_2way(void);

void itv_set_rotation_2way(ITURotation rot);
void itv_set_video_window_2way(uint32_t startX, uint32_t startY, uint32_t width, uint32_t height);
ISP_RESULT itv_enable_isp_feature_2way(MMP_ISP_CAPS cap);

#if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)

void itv_set_vidSurf_buf_2way(uint8_t *addr, uint8_t index);
int itv_get_vidSurf_index_2way(void);
int itv_update_vidSurf_anchor_2way(void);
void itv_stop_vidSurf_anchor_2way(void);
ITURotation itv_get_rotation_2way(void);
bool itv_get_new_video_2way(void);
void itv_set_color_correction_2way(MMP_ISP_COLOR_CTRL colorCtrl, int value);
void itv_set_isp_onfly_2way(MMP_ISP_SHARE isp_share, bool interlaced);
void itv_enable_video_center_mode_2way(bool enabled);
void itv_update_video_window_2way(int width, int height);
#endif

#if ((CFG_CHIP_FAMILY == 9860) && defined(CFG_ITV_VP_HIGH_QUALITY))
void itv_hw_overlay_task(MMP_ISP_OUTPUT_INFO *out_info);
#endif

#ifdef __cplusplus
}
#endif

#endif // ITV_H