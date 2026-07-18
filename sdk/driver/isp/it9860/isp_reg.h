#ifndef __ISP_REG_H_I7U1OSBH_HFH5_KK3Q_XKST_5P4DY8OJKVRN__
#define __ISP_REG_H_I7U1OSBH_HFH5_KK3Q_XKST_5P4DY8OJKVRN__

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                              Constant Definition
//=============================================================================
#define ISP_REG_BASE                        0xD0300000UL
#define ISP_CORE_1_BASE                     0x00000200UL
#define ISP_MASK_LOWBIT                     0xFFFF0000UL
#define ISP_MASK_HIGHBIT                    0x0000FFFFUL

//================================================================
// 0x0500
//================================================================
#define ISP_REG_SET500                      (ISP_REG_BASE + 0x0000U)

#define ISP_BIT_ISP_CLEAR_INTERRUPT_EN      0x0001        //0000 0001 0000 0000
#define ISP_BIT_ISP_WIRTE_SLICE_FIRE_EN     0x0001        //0000 0000 0001 0000
#define ISP_BIT_ISP_UPDATE_PARM_EN          0x0001        //0000 0000 0000 1000
#define ISP_BIT_DRIVER_FIRE_EN              0x0001        //0000 0000 0000 0001

#define ISP_SHT_ISP_CLEAR_INTERRUPT_EN      8
#define ISP_SHT_ISP_WIRTE_SLICE_FIRE_EN     4
#define ISP_SHT_ISP_UPDATE_PARM_EN          3
#define ISP_SHT_DRIVER_FIRE_EN              0

//================================================================
// 0x0502
//================================================================
#define ISP_REG_SET502                      (ISP_REG_BASE + 0x0002U)

#define ISP_BIT_SWWRFLIPNUM                 0x0007UL    // 1110 0000 0000 0000
#define ISP_BIT_WRITE_ROTATE_DIR            0x0007UL    // 0001 1100 0000 0000
#define ISP_BIT_WRFLIPNUM                   0x0007UL    // 0000 0011 1000 0000
#define ISP_BIT_PROGFIELD_MODE_EN           0x0001UL    // 0000 0000 0100 0000
#define ISP_BIT_NEGATIVE_EFFECT_EN          0x0001UL    // 0000 0000 0010 0000
#define ISP_BIT_FIELD_SCALE_EN              0x0001UL    // 0000 0000 0001 0000
#define ISP_BIT_BOTTOM_FIELD_SCALE_FIRST    0x0001UL    // 0000 0000 0000 1000
#define ISP_BIT_COLOR_CORRECT_EN            0x0001UL    // 0000 0000 0000 0100
#define ISP_BIT_COLOR_SPECE_EN              0x0001UL    // 0000 0000 0000 0010
#define ISP_BIT_VIDEO_OUT_EN                0x0001UL    // 0000 0000 0000 0001

#define ISP_SHT_SWWRFLIPNUM                 (13UL + 16UL)
#define ISP_SHT_WRITE_ROTATE_DIR            (10UL + 16UL)
#define ISP_SHT_WRFLIPNUM                   (7UL + 16UL)
#define ISP_SHT_PROGFIELD_MODE_EN           (6UL + 16UL)
#define ISP_SHT_NEGATIVE_EFFECT_EN          (5UL + 16UL)
#define ISP_SHT_FIELD_SCALE_EN              (4UL + 16UL)
#define ISP_SHT_BOTTOM_FIELD_SCALE_FIRST    (3UL + 16UL)
#define ISP_SHT_COLOR_CORRECT_EN            (2UL + 16UL)
#define ISP_SHT_COLOR_SPECE_EN              (1UL + 16UL)
#define ISP_SHT_VIDEO_OUT_EN                (0UL + 16UL)

//================================================================
// 0x0504
// Deinterlace function
//================================================================
#define ISP_REG_SET_DEINTERLACE             (ISP_REG_BASE + 0x0004U)

#define ISP_BIT_OUT_MOTIONDETECTION_EN      0x0001          //0100 0000 0000 0000
#define ISP_BIT_LOWLEVELBYPASSBLEND         0x0001          //0001 0000 0000 0000
#define ISP_BIT_LOWLEVELOUTSIDE_EN          0x0001          //0000 1000 0000 0000
#define ISP_BIT_LOWLEVELMODE                0x0001          //0000 0100 0000 0000
#define ISP_BIT_LOWLEVELEDGE_EN             0x0001          //0000 0010 0000 0000
#define ISP_BIT_UVREPEAT_MODE               0x0001          //0000 0001 0000 0000
#define ISP_BIT_CHROMA_EDGEDET_EN           0x0001          //0000 0000 1000 0000
#define ISP_BIT_LUMA_EDGEDET_EN             0x0001          //0000 0000 0100 0000
#define ISP_BIT_DEINTERLACE_EN              0x0001          //0000 0000 0010 0000
#define ISP_BIT_2D_DEINTER_MODE_EN          0x0001          //0000 0000 0001 0000
#define ISP_BIT_SRC_BOTTOM_FIELD_FIRST      0x0001          //0000 0000 0000 1000
#define ISP_BIT_DEINTER_BOTTOM_EN           0x0001          //0000 0000 0000 0100
#define ISP_BIT_SRC_LPFITR_EN               0x0001          //0000 0000 0000 0010
#define ISP_BIT_AUTO_SWAP_FIELD             0x0001          //0000 0000 0000 0001

#define ISP_SHT_OUT_MOTIONDETECTION_EN      14
#define ISP_SHT_LOWLEVELBYPASSBLEND         12
#define ISP_SHT_LOWLEVELOUTSIDE_EN          11
#define ISP_SHT_LOWLEVELMODE                10
#define ISP_SHT_LOWLEVELEDGE_EN             9
#define ISP_SHT_UVREPEAT_MODE               8
#define ISP_SHT_CHROMA_EDGEDET_EN           7
#define ISP_SHT_LUMA_EDGEDET_EN             6
#define ISP_SHT_DEINTERLACE_EN              5
#define ISP_SHT_2D_DEINTER_MODE_EN          4
#define ISP_SHT_SRC_BOTTOM_FIELD_FIRST      3
#define ISP_SHT_DEINTER_BOTTOM_EN           2
#define ISP_SHT_SRC_LPFITR_EN               1
#define ISP_SHT_AUTO_SWAP_FIELD             0

//================================================================
// 0x0506
// Input format
//================================================================
#define ISP_REG_INPUT_FORMAT                (ISP_REG_BASE + 0x0006U)

#define ISP_BIT_IN_YUV255RANGE_EN           0x0001          //0010 0000 0000 0000
#define ISP_BIT_IN_PACKET_FORMAT            0x0003          //0000 1100 0000 0000
#define ISP_BIT_IN_PLANE_FORMAT             0x0003          //0000 0011 0000 0000
#define ISP_BIT_IN_RDRQ_DOUBLE_LINE         0x0001          //0000 0000 1000 0000
#define ISP_BIT_IN_RGB888_EN                0x0001          //0000 0000 0010 0000
#define ISP_BIT_IN_RGB565_EN                0x0001          //0000 0000 0001 0000
#define ISP_BIT_IN_YUVPACKET_EN             0x0001          //0000 0000 0000 1000
#define ISP_BIT_IN_YUVPLANE_EN              0x0001          //0000 0000 0000 0100
#define ISP_BIT_IN_NV_FORMAT                0x0001          //0000 0000 0000 0010
#define ISP_BIT_IN_NV_EN                    0x0001          //0000 0000 0000 0001

#define ISP_SHT_IN_YUV255RANGE_EN           (13UL + 16UL)
#define ISP_SHT_IN_PACKET_FORMAT            (10UL + 16UL)
#define ISP_SHT_IN_PLANE_FORMAT             (8UL + 16UL)
#define ISP_SHT_IN_RDRQ_DOUBLE_LINE         (7UL + 16UL)
#define ISP_SHT_IN_RGB888_EN                (5UL + 16UL)
#define ISP_SHT_IN_RGB565_EN                (4UL + 16UL)
#define ISP_SHT_IN_YUVPACKET_EN             (3UL + 16UL)
#define ISP_SHT_IN_YUVPLANE_EN              (2UL + 16UL)
#define ISP_SHT_IN_NV_FORMAT                (1UL + 16UL)
#define ISP_SHT_IN_NV_EN                    (0UL + 16UL)

//================================================================
// 0x0508
//================================================================
#define ISP_REG_ENGINEMODE_PARM             (ISP_REG_BASE + 0x0008U)

#define ISP_BIT_TOTALSLICENUM               0x00FF          //1111 1111 0000 0000
#define ISP_BIT_JPEGDECODE_MODE             0x0001          //0000 0000 1000 0000
#define ISP_BIT_LOWLATENCY_EN               0x0001          //0000 0000 0100 0000
#define ISP_BIT_OUT_NV_FORMAT               0x0001          //0000 0000 0010 0000
#define ISP_BIT_OUT_NV_EN                   0x0001          //0000 0000 0001 0000
#define ISP_BIT_OUTPUT_FIELD_MODE           0x0001          //0000 0000 0000 1000
#define ISP_BIT_CAPTURE_ONFLY_EN            0x0001          //0000 0000 0000 0100
#define ISP_BIT_READ_MEMORY_MODE            0x0001          //0000 0000 0000 0010
#define ISP_BIT_SWWRFLIP_EN                 0x0001          //0000 0000 0000 0001

#define ISP_SHT_TOTALSLICENUM               8
#define ISP_SHT_JPEGDECODE_MODE             7
#define ISP_SHT_LOWLATENCY_EN               6
#define ISP_SHT_OUT_NV_FORMAT               5
#define ISP_SHT_OUT_NV_EN                   4
#define ISP_SHT_OUTPUT_FIELD_MODE           3
#define ISP_SHT_CAPTURE_ONFLY_EN            2
#define ISP_SHT_READ_MEMORY_MODE            1
#define ISP_SHT_SWWRFLIP_EN                 0
//================================================================
// 0x050A
//================================================================
#define ISP_REG_OUTPUT_FORMAT               (ISP_REG_BASE + 0x000AU)

#define ISP_BIT_OUT_RGB_FORMAT              0x0003          //1100 0000 0000 0000
#define ISP_BIT_OUT_YUVPACKET_FORMAT        0x0003          //0011 0000 0000 0000
#define ISP_BIT_OUT_YUVPLANE_FORMAT         0x0003          //0000 1100 0000 0000
#define ISP_BIT_OUT_FORMAT                  0x0003          //0000 0011 0000 0000
#define ISP_BIT_OUT_DITHER_MODE             0x0001          //0000 0000 1000 0000
#define ISP_BIT_LCD_RGB2YUV_EN              0x0001          //0000 0000 0100 0000
#define ISP_BIT_QUEUE_FIRE_EN               0x0001          //0000 0000 0010 0000
#define ISP_BIT_KEEP_LAST_FIELD_EN          0x0001          //0000 0000 0000 0100
#define ISP_BIT_LCD_ONFLY_EN                0x0001          //0000 0000 0000 0100
#define ISP_BIT_ONFLY_WRITE_MEM_EN          0x0001          //0000 0000 0000 0010
#define ISP_BIT_DOUBLE_FRAMERATE_EN         0x0001          //0000 0000 0000 0001

#define ISP_SHT_OUT_RGB_FORMAT              (14UL + 16UL)
#define ISP_SHT_OUT_YUVPACKET_FORMAT        (12UL + 16UL)
#define ISP_SHT_OUT_YUVPLANE_FORMAT         (10UL + 16UL)
#define ISP_SHT_OUT_FORMAT                  (8UL + 16UL)
#define ISP_SHT_OUT_DITHER_MODE             (7UL + 16UL)
#define ISP_SHT_LCD_RGB2YUV_EN              (6UL + 16UL)
#define ISP_SHT_QUEUE_FIRE_EN               (5UL + 16UL)
#define ISP_SHT_KEEP_LAST_FIELD_EN          (4UL + 16UL)
#define ISP_SHT_LCD_ONFLY_EN                (2UL + 16UL)
#define ISP_SHT_ONFLY_WRITE_MEM_EN          (1UL + 16UL)
#define ISP_SHT_DOUBLE_FRAMERATE_EN         (0UL + 16UL)

//================================================================
// 0x050C
//================================================================
#define ISP_REG_RAWDATA_HANDSHARK           (ISP_REG_BASE + 0x000CU)

#define ISP_BIT_RAWDATA_SLICE_NUM           0x00FF          //1111 1111 0000 0000
#define ISP_BIT_RAWDATA_HANDSHARK_EN        0x0001          //0000 0000 0000 0001

#define ISP_SHT_RAWDATA_SLICE_NUM           8
#define ISP_SHT_RAWDATA_HANDSHARK_EN        0

//================================================================
// 0x050E
//================================================================
#define ISP_REG_SET50E                      (ISP_REG_BASE + 0x000EU)

#define ISP_BIT_POST_SUBTITLE_EN            0x0001          //0000 0000 0000 0100
#define ISP_BIT_ISP_INTERRUPT_EN            0x0001          //0000 0000 0000 0010
#define ISP_BIT_OUTYUV235RANGE_EN           0x0001          //0000 0000 0000 0001

#define ISP_SHT_POST_SUBTITLE_EN            (2UL + 16UL)
#define ISP_SHT_ISP_INTERRUPT_EN            (1UL + 16UL)
#define ISP_SHT_OUTYUV235RANGE_EN           (0UL + 16UL)

//================================================================
// 0x0510 ~ 0x052E
// Input buffer base, width, height and pitch
//================================================================
// Decode input buffer
#define ISP_REG_INPUT_ADDR_YL               (ISP_REG_BASE + 0x0010U)
#define ISP_REG_INPUT_ADDR_YH               (ISP_REG_BASE + 0x0012U)
#define ISP_REG_INPUT_ADDR_UL               (ISP_REG_BASE + 0x0014U)
#define ISP_REG_INPUT_ADDR_UH               (ISP_REG_BASE + 0x0016U)
#define ISP_REG_INPUT_ADDR_VL               (ISP_REG_BASE + 0x0018U)
#define ISP_REG_INPUT_ADDR_VH               (ISP_REG_BASE + 0x001AU)

#define ISP_REG_INPUT_ADDR_YPL              (ISP_REG_BASE + 0x001CU)
#define ISP_REG_INPUT_ADDR_YPH              (ISP_REG_BASE + 0x001EU)
#define ISP_REG_INPUT_ADDR_UPL              (ISP_REG_BASE + 0x0020U)
#define ISP_REG_INPUT_ADDR_UPH              (ISP_REG_BASE + 0x0022U)
#define ISP_REG_INPUT_ADDR_VPL              (ISP_REG_BASE + 0x0024U)
#define ISP_REG_INPUT_ADDR_VPH              (ISP_REG_BASE + 0x0026U)

#define ISP_REG_INPUT_PITCH_Y               (ISP_REG_BASE + 0x0028U)
#define ISP_REG_INPUT_PITCH_UV              (ISP_REG_BASE + 0x002AU)
#define ISP_REG_INPUT_HEIGHT                (ISP_REG_BASE + 0x002CU)
#define ISP_REG_INPUT_WIDTH                 (ISP_REG_BASE + 0x002EU)

#define ISP_BIT_INPUT_ADDR_L                0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_INPUT_ADDR_H                0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_INPUT_PITCH_Y               0x7FFF          //0111 1111 1111 1111
#define ISP_BIT_INPUT_PITCH_UV              0x7FFF          //0111 1111 1111 1111
#define ISP_BIT_INPUT_HEIGHT                0x3FFF          //0111 1111 1111 1111
#define ISP_BIT_INPUT_WIDTH                 0x7FFF          //0011 1111 1111 1111

//================================================================
// 0x0530 ~ 0x053E
// Scaling factor
//================================================================
#define ISP_REG_SCALE_HCIP_L                (ISP_REG_BASE + 0x0030U)   // HCIP
#define ISP_REG_SCALE_HCIP_H                (ISP_REG_BASE + 0x0032U)   // HCIP
#define ISP_REG_SCALE_VCIP                  (ISP_REG_BASE + 0x0034U)   // VCIP
#define ISP_REG_SCALE_OPNPLB                (ISP_REG_BASE + 0x0036U)   // OPNPLB
#define ISP_REG_SCALE_HCI_L                 (ISP_REG_BASE + 0x0038U)   // HCI
#define ISP_REG_SCALE_HCI_H                 (ISP_REG_BASE + 0x003AU)   // HCI
#define ISP_REG_SCALE_VCI_L                 (ISP_REG_BASE + 0x003CU)   // VCI
#define ISP_REG_SCALE_VCI_H                 (ISP_REG_BASE + 0x003EU)   // VCI

#define ISP_BIT_SCALE_HCIP_L                0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_SCALE_HCIP_H                0x007F          //0000 0000 0111 1111
#define ISP_BIT_SCALE_VCIP                  0x007F          //0000 0000 0111 1111
#define ISP_BIT_SCALE_OPNPLB                0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_SCALE_L                     0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_SCALE_H                     0x000F          //0000 0000 0000 1111

//================================================================
// 0x0540
// UV Interpolation
//================================================================
#define ISP_REG_UV_INTERPOLATION            (ISP_REG_BASE + 0x0040U)

//================================================================
// 0x0542 ~ 0x0558
// Transfer matrix for YUV to RGB
//================================================================
#define ISP_REG_YUV_TO_RGB_11               (ISP_REG_BASE + 0x0042U)
#define ISP_REG_YUV_TO_RGB_12               (ISP_REG_BASE + 0x0044U)
#define ISP_REG_YUV_TO_RGB_13               (ISP_REG_BASE + 0x0046U)
#define ISP_REG_YUV_TO_RGB_21               (ISP_REG_BASE + 0x0048U)
#define ISP_REG_YUV_TO_RGB_22               (ISP_REG_BASE + 0x004AU)
#define ISP_REG_YUV_TO_RGB_23               (ISP_REG_BASE + 0x004CU)
#define ISP_REG_YUV_TO_RGB_31               (ISP_REG_BASE + 0x004EU)
#define ISP_REG_YUV_TO_RGB_32               (ISP_REG_BASE + 0x0050U)
#define ISP_REG_YUV_TO_RGB_33               (ISP_REG_BASE + 0x0052U)
#define ISP_REG_YUV_TO_RGB_CONST_R          (ISP_REG_BASE + 0x0054U)
#define ISP_REG_YUV_TO_RGB_CONST_G          (ISP_REG_BASE + 0x0056U)
#define ISP_REG_YUV_TO_RGB_CONST_B          (ISP_REG_BASE + 0x0058U)

#define ISP_BIT_YUV_TO_RGB                  0x07FF          //0000 0111 1111 1111
#define ISP_BIT_YUV_TO_RGB_CONST            0x03FF          //0000 0011 1111 1111

//================================================================
// 0x055A ~ 0x0570
// Color correction matrix & delta R/G/B
//================================================================
#define ISP_REG_COL_COR_11                  (ISP_REG_BASE + 0x005AU)
#define ISP_REG_COL_COR_12                  (ISP_REG_BASE + 0x005CU)
#define ISP_REG_COL_COR_13                  (ISP_REG_BASE + 0x005EU)
#define ISP_REG_COL_COR_21                  (ISP_REG_BASE + 0x0060U)
#define ISP_REG_COL_COR_22                  (ISP_REG_BASE + 0x0062U)
#define ISP_REG_COL_COR_23                  (ISP_REG_BASE + 0x0064U)
#define ISP_REG_COL_COR_31                  (ISP_REG_BASE + 0x0066U)
#define ISP_REG_COL_COR_32                  (ISP_REG_BASE + 0x0068U)
#define ISP_REG_COL_COR_33                  (ISP_REG_BASE + 0x006AU)

#define ISP_REG_COL_COR_DELTA_R             (ISP_REG_BASE + 0x006CU)
#define ISP_REG_COL_COR_DELTA_G             (ISP_REG_BASE + 0x006EU)
#define ISP_REG_COL_COR_DELTA_B             (ISP_REG_BASE + 0x0070U)

#define ISP_BIT_COL_COR                     0x1FFF          //0001 1111 1111 1111
#define ISP_BIT_COL_CORR_DELTA              0x01FF          //0000 0001 1111 1111

//================================================================
// 0x0572 ~ 0x05A0
// subtitle function
//================================================================
// SUBTITLE 0
#define ISP_REG_SET_SUBTITLE_0              (ISP_REG_BASE + 0x0072U)

#define ISP_REG_SUBTITLE_0_SRCWIDTH         (ISP_REG_BASE + 0x0074U)
#define ISP_REG_SUBTITLE_0_SRCHEIGHT        (ISP_REG_BASE + 0x0076U)
#define ISP_REG_SUBTITLE_0_HCI              (ISP_REG_BASE + 0x0078U)
#define ISP_REG_SUBTITLE_0_VCI              (ISP_REG_BASE + 0x007AU)
#define ISP_REG_SUBTITLE_0_START_X          (ISP_REG_BASE + 0x007CU)
#define ISP_REG_SUBTITLE_0_START_Y          (ISP_REG_BASE + 0x007EU)
#define ISP_REG_SUBTITLE_0_DSTWIDTH         (ISP_REG_BASE + 0x0080U)
#define ISP_REG_SUBTITLE_0_DSTHEIGHT        (ISP_REG_BASE + 0x0082U)
#define ISP_REG_SUBTITLE_0_ADDR_L           (ISP_REG_BASE + 0x0084U)
#define ISP_REG_SUBTITLE_0_ADDR_H           (ISP_REG_BASE + 0x0086U)
#define ISP_REG_SUBTITLE_0_PITCH            (ISP_REG_BASE + 0x0088U)

// SUBTITLE 1
#define ISP_REG_SET_SUBTITLE_1              (ISP_REG_BASE + 0x008AU)

#define ISP_REG_SUBTITLE_1_SRCWIDTH         (ISP_REG_BASE + 0x008CU)
#define ISP_REG_SUBTITLE_1_SRCHEIGHT        (ISP_REG_BASE + 0x008EU)
#define ISP_REG_SUBTITLE_1_HCI              (ISP_REG_BASE + 0x0090U)
#define ISP_REG_SUBTITLE_1_VCI              (ISP_REG_BASE + 0x0092U)
#define ISP_REG_SUBTITLE_1_START_X          (ISP_REG_BASE + 0x0094U)
#define ISP_REG_SUBTITLE_1_START_Y          (ISP_REG_BASE + 0x0096U)
#define ISP_REG_SUBTITLE_1_DSTWIDTH         (ISP_REG_BASE + 0x0098U)
#define ISP_REG_SUBTITLE_1_DSTHEIGHT        (ISP_REG_BASE + 0x009AU)
#define ISP_REG_SUBTITLE_1_ADDR_L           (ISP_REG_BASE + 0x009CU)
#define ISP_REG_SUBTITLE_1_ADDR_H           (ISP_REG_BASE + 0x009EU)
#define ISP_REG_SUBTITLE_1_PITCH            (ISP_REG_BASE + 0x00A0U)

#define ISP_BIT_SUBTITLE_UIDEC_EN           0x0001          //0000 0100 0000 0000
#define ISP_BIT_SUBTITLE_MODE               0x0003          //0000 0000 0000 0110
#define ISP_BIT_SUBTITLE_EN                 0x0001          //0000 0000 0000 0001

#define ISP_BIT_SUBTITLE_SRCWIDTH           0x07FF          //0000 0111 1111 1111
#define ISP_BIT_SUBTITLE_SRCHEIGHT          0x07FF          //0000 0111 1111 1111
#define ISP_BIT_SUBTITLE_HCI                0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_SUBTITLE_VCI                0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_SUBTITLE_START_X            0x07FF          //0000 0111 1111 1111
#define ISP_BIT_SUBTITLE_START_Y            0x07FF          //0000 0111 1111 1111
#define ISP_BIT_SUBTITLE_ADDR_L             0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_SUBTITLE_ADDR_H             0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_SUBTITLE_DSTWIDTH           0x07FF          //0000 0111 1111 1111
#define ISP_BIT_SUBTITLE_DSTHEIGHT          0x07FF          //0000 0111 1111 1111
#define ISP_BIT_SUBTITLE_PITCH              0x7FFF          //0111 1111 1111 1111

#define ISP_SHT_SUBTITLE_UIDEC_EN           10
#define ISP_SHT_SUBTITLE_MODE               1
#define ISP_SHT_SUBTITLE_EN                 0

//================================================================
// Low Latency Slice
//================================================================
#define ISP_REG_LOWLATENCY_SLICE            (ISP_REG_BASE + 0x0072U)

#define ISP_BIT_LOWLATENCY_SLICE            0x03FF          //0000 0011 1111 1111

#define ISP_SHT_LOWLATENCY_SLICE            0

//================================================================
// Output Address of Write Buffer
//================================================================
#define ISP_REG_OUT_1_ADDR_0L               (ISP_REG_BASE + 0x0074U)
#define ISP_REG_OUT_1_ADDR_0H               (ISP_REG_BASE + 0x0076U)
#define ISP_REG_OUT_1_ADDR_1L               (ISP_REG_BASE + 0x0078U)
#define ISP_REG_OUT_1_ADDR_1H               (ISP_REG_BASE + 0x007AU)
#define ISP_REG_OUT_1_ADDR_2L               (ISP_REG_BASE + 0x007CU)
#define ISP_REG_OUT_1_ADDR_2H               (ISP_REG_BASE + 0x007EU)
#define ISP_REG_OUT_2_ADDR_0L               (ISP_REG_BASE + 0x0080U)
#define ISP_REG_OUT_2_ADDR_0H               (ISP_REG_BASE + 0x0082U)
#define ISP_REG_OUT_2_ADDR_1L               (ISP_REG_BASE + 0x0084U)
#define ISP_REG_OUT_2_ADDR_1H               (ISP_REG_BASE + 0x0086U)
#define ISP_REG_OUT_2_ADDR_2L               (ISP_REG_BASE + 0x0088U)
#define ISP_REG_OUT_2_ADDR_2H               (ISP_REG_BASE + 0x008AU)
#define ISP_REG_OUT_3_ADDR_0L               (ISP_REG_BASE + 0x008CU)
#define ISP_REG_OUT_3_ADDR_0H               (ISP_REG_BASE + 0x008EU)
#define ISP_REG_OUT_3_ADDR_1L               (ISP_REG_BASE + 0x0090U)
#define ISP_REG_OUT_3_ADDR_1H               (ISP_REG_BASE + 0x0092U)
#define ISP_REG_OUT_3_ADDR_2L               (ISP_REG_BASE + 0x0094U)
#define ISP_REG_OUT_3_ADDR_2H               (ISP_REG_BASE + 0x0096U)
#define ISP_REG_OUT_4_ADDR_0L               (ISP_REG_BASE + 0x0098U)
#define ISP_REG_OUT_4_ADDR_0H               (ISP_REG_BASE + 0x009AU)
#define ISP_REG_OUT_4_ADDR_1L               (ISP_REG_BASE + 0x009CU)
#define ISP_REG_OUT_4_ADDR_1H               (ISP_REG_BASE + 0x009EU)
#define ISP_REG_OUT_4_ADDR_2L               (ISP_REG_BASE + 0x00A0U)
#define ISP_REG_OUT_4_ADDR_2H               (ISP_REG_BASE + 0x00BEU)

#define ISP_REG_OUT_1_ADDR_0L_CORE1         (ISP_REG_BASE + 0x00B6U)
#define ISP_REG_OUT_1_ADDR_0H_CORE1         (ISP_REG_BASE + 0x00B8U)
#define ISP_REG_OUT_1_ADDR_1L_CORE1         (ISP_REG_BASE + 0x00BAU)
#define ISP_REG_OUT_1_ADDR_1H_CORE1         (ISP_REG_BASE + 0x00BCU)
#define ISP_REG_OUT_1_ADDR_2L_CORE1         (ISP_REG_BASE + 0x015CU)
#define ISP_REG_OUT_1_ADDR_2H_CORE1         (ISP_REG_BASE + 0x015EU)
#define ISP_REG_OUT_2_ADDR_0L_CORE1         (ISP_REG_BASE + 0x0160U)
#define ISP_REG_OUT_2_ADDR_0H_CORE1         (ISP_REG_BASE + 0x0162U)
#define ISP_REG_OUT_2_ADDR_1L_CORE1         (ISP_REG_BASE + 0x01B4U)
#define ISP_REG_OUT_2_ADDR_1H_CORE1         (ISP_REG_BASE + 0x01B6U)
#define ISP_REG_OUT_2_ADDR_2L_CORE1         (ISP_REG_BASE + 0x01B8U)
#define ISP_REG_OUT_2_ADDR_2H_CORE1         (ISP_REG_BASE + 0x01BAU)

#define ISP_BIT_WB_OUT_ADDR_L               0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_WB_OUT_ADDR_H               0xFFFF          //1111 1111 1111 1111

//================================================================
// weighting matrix of Preview Scale
//================================================================
#define ISP_REG_PVSCALEWX0100               (ISP_REG_BASE + 0x0072U)
#define ISP_REG_PVSCALEWX0302               (ISP_REG_BASE + 0x0074U)
#define ISP_REG_PVSCALEWX1110               (ISP_REG_BASE + 0x0076U)
#define ISP_REG_PVSCALEWX1312               (ISP_REG_BASE + 0x0078U)
#define ISP_REG_PVSCALEWX2120               (ISP_REG_BASE + 0x007AU)
#define ISP_REG_PVSCALEWX2322               (ISP_REG_BASE + 0x007CU)
#define ISP_REG_PVSCALEWX3130               (ISP_REG_BASE + 0x007EU)
#define ISP_REG_PVSCALEWX3332               (ISP_REG_BASE + 0x0080U)
#define ISP_REG_PVSCALEWX4140               (ISP_REG_BASE + 0x0082U)
#define ISP_REG_PVSCALEWX4342               (ISP_REG_BASE + 0x0084U)

#define ISP_REG_PVSCALEWY0100               (ISP_REG_BASE + 0x0086U)
#define ISP_REG_PVSCALEWY0302               (ISP_REG_BASE + 0x0088U)
#define ISP_REG_PVSCALEWY1110               (ISP_REG_BASE + 0x008AU)
#define ISP_REG_PVSCALEWY1312               (ISP_REG_BASE + 0x008CU)
#define ISP_REG_PVSCALEWY2120               (ISP_REG_BASE + 0x008EU)
#define ISP_REG_PVSCALEWY2322               (ISP_REG_BASE + 0x0090U)
#define ISP_REG_PVSCALEWY3130               (ISP_REG_BASE + 0x0092U)
#define ISP_REG_PVSCALEWY3332               (ISP_REG_BASE + 0x0094U)
#define ISP_REG_PVSCALEWY4140               (ISP_REG_BASE + 0x0096U)
#define ISP_REG_PVSCALEWY4342               (ISP_REG_BASE + 0x0098U)

#define ISP_BIT_PVSCALEWEIGHT               0x00FF          //0000 0000 1111 1111

#define ISP_SHT_PVSCALEWEIGHT_H             8
#define ISP_SHT_PVSCALEWEIGHT_L             0

//================================================================
// Preview Output Address, Width, Height and Pitch
//================================================================
#define ISP_REG_PVOUT_ADDR_0L               (ISP_REG_BASE + 0x009EU)
#define ISP_REG_PVOUT_ADDR_0H               (ISP_REG_BASE + 0x00A0U)

#define ISP_REG_PVOUT_WIDTH                 (ISP_REG_BASE + 0x009AU)
#define ISP_REG_PVOUT_HEIGHT                (ISP_REG_BASE + 0x009CU)
#define ISP_REG_PVOUT_YRGB_PITCH            (ISP_REG_BASE + 0x00BEU)

#define ISP_BIT_PVOUT_ADDR_L                0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_PVOUT_ADDR_H                0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_PVOUT_WIDTH                 0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_PVOUT_HEIGHT                0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_PVOUT_PITCH                 0x7FFF          //0111 1111 1111 1111

//================================================================
// 0x05A2 ~ 0x05B4
// Output Address, Width, Height and Pitch
//================================================================
#define ISP_REG_OUT_ADDR_0L                 (ISP_REG_BASE + 0x00A2U)
#define ISP_REG_OUT_ADDR_0H                 (ISP_REG_BASE + 0x00A4U)
#define ISP_REG_OUT_ADDR_1L                 (ISP_REG_BASE + 0x00A6U)
#define ISP_REG_OUT_ADDR_1H                 (ISP_REG_BASE + 0x00A8U)
#define ISP_REG_OUT_ADDR_2L                 (ISP_REG_BASE + 0x00AAU)
#define ISP_REG_OUT_ADDR_2H                 (ISP_REG_BASE + 0x00ACU)

#define ISP_REG_OUT_WIDTH                   (ISP_REG_BASE + 0x00AEU)
#define ISP_REG_OUT_HEIGHT                  (ISP_REG_BASE + 0x00B0U)
#define ISP_REG_OUT_YRGB_PITCH              (ISP_REG_BASE + 0x00B2U)
#define ISP_REG_OUT_UV_PITCH                (ISP_REG_BASE + 0x00B4U)

#define ISP_BIT_OUT_ADDR_L                  0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_OUT_ADDR_H                  0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_OUT_WIDTH                   0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_OUT_HEIGHT                  0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_OUT_PITCH                   0x7FFF          //0111 1111 1111 1111

//================================================================
// 0x05B6 ~ 0x05BA
// 3D Deinterlace Parm
//================================================================
#define ISP_REG_3D_DEINTER_PARM_1           (ISP_REG_BASE + 0x00B6U)
#define ISP_REG_3D_DEINTER_PARM_2           (ISP_REG_BASE + 0x00B8U)
#define ISP_REG_3D_DEINTER_PARM_3           (ISP_REG_BASE + 0x00BAU)

#define ISP_BIT_3D_MDTHRED_HIGH             0x00FF          //1111 1111 0000 0000
#define ISP_BIT_3D_MDTHRED_LOW              0x00FF          //0000 0000 1111 1111

#define ISP_BIT_DISABLE_MOTIONVALUE_A       0x0001          //1000 0000 0000 0000
#define ISP_BIT_DISABLE_MOTIONVALUE_B       0x0001          //0100 0000 0000 0000
#define ISP_BIT_DISABLE_MOTIONVALUE_C       0x0001          //0010 0000 0000 0000
#define ISP_BIT_DISABLE_MOTIONVALUE_D       0x0001          //0001 0000 0000 0000
#define ISP_BIT_DISABLE_MOTIONVALUE_E       0x0001          //0000 1000 0000 0000
#define ISP_BIT_DISABLE_MOTIONVALUE_F       0x0001          //0000 0100 0000 0000
#define ISP_BIT_DISABLE_MOTIONVALUE_G       0x0001          //0000 0010 0000 0000
#define ISP_BIT_LPF_WEIGHT_EN               0x0001          //0000 0001 0000 0000
#define ISP_BIT_LPF_BLEND_EN                0x0001          //0000 0000 1000 0000
#define ISP_BIT_3D_MDTHRED_STEP             0x007F          //0000 0000 0111 1111

#define ISP_BIT_LPF_STATICPIXEL_EN          0x0001          //0000 0000 0000 0001

#define ISP_SHT_3D_MDTHRED_HIGH             8
#define ISP_SHT_3D_MDTHRED_LOW              0

#define ISP_SHT_DISABLE_MOTIONVALUE_A       15
#define ISP_SHT_DISABLE_MOTIONVALUE_B       14
#define ISP_SHT_DISABLE_MOTIONVALUE_C       13
#define ISP_SHT_DISABLE_MOTIONVALUE_D       12
#define ISP_SHT_DISABLE_MOTIONVALUE_E       11
#define ISP_SHT_DISABLE_MOTIONVALUE_F       10
#define ISP_SHT_DISABLE_MOTIONVALUE_G       9
#define ISP_SHT_LPF_WEIGHT_EN               8
#define ISP_SHT_LPF_BLEND_EN                7
#define ISP_SHT_3D_MDTHRED_STEP             0

#define ISP_SHT_LPF_STATICPIXEL_EN          0

//================================================================
// 0x05BC
// 2D Deinterlace Parm
//================================================================
#define ISP_REG_2D_DEINTER_PARM_1           (ISP_REG_BASE + 0x00BCU)

#define ISP_BIT_2D_POST_LPF_EN              0x0001          //0100 0000 0000 0000
#define ISP_BIT_2D_EDGE_WEIGHT              0x007F          //0011 1111 1000 0000
#define ISP_BIT_2D_ORG_WEIGHT               0x007F          //0000 0000 0111 1111

#define ISP_SHT_2D_POST_LPF_EN              14
#define ISP_SHT_2D_EDGE_WEIGHT              7
#define ISP_SHT_2D_ORG_WEIGHT               0

//================================================================
// 0x05C0 ~ 0x05CA
// Scale Background function
//================================================================
#define ISP_REG_BGCOLOR_RG                  (ISP_REG_BASE + 0x00C0U)
#define ISP_REG_BGCOLOR_B                   (ISP_REG_BASE + 0x00C2U)

#define ISP_REG_SCALE_DSTPOS_X              (ISP_REG_BASE + 0x00C4U)
#define ISP_REG_SCALE_DSTPOS_Y              (ISP_REG_BASE + 0x00C6U)
#define ISP_REG_SCALE_DSTWIDTH              (ISP_REG_BASE + 0x00C8U)
#define ISP_REG_SCALE_DSTHEIGHT             (ISP_REG_BASE + 0x00CAU)

#define ISP_BIT_BGCOLOR                     0x00FF          //0000 0000 1111 1111
#define ISP_BIT_SCALE_DSTPOS_X              0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_SCALE_DSTPOS_Y              0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_SCALE_DSTWIDTH              0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_SCALE_DSTHEIGHT             0x3FFF          //0011 1111 1111 1111

#define ISP_SHT_BGCOLOR_R                   0
#define ISP_SHT_BGCOLOR_G                   8

//================================================================
// Preview Scale function
//================================================================
#define ISP_REG_PVSCALE_DSTWIDTH            (ISP_REG_BASE + 0x00CCU)
#define ISP_REG_PVSCALE_DSTHEIGHT           (ISP_REG_BASE + 0x00CEU)

#define ISP_BIT_PVSCALE_DSTWIDTH            0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_PVSCALE_DSTHEIGHT           0x3FFF          //0011 1111 1111 1111

//================================================================
// 0x05D0 ~ 0x05FA
// Frame function
//================================================================
#define ISP_REG_SET_FRMFUN_0                (ISP_REG_BASE + 0x00D0U)
#define ISP_REG_CONST_ALPHA_0               (ISP_REG_BASE + 0x00D2U)
#define ISP_REG_FRMFUN_0_KEY_RG             (ISP_REG_BASE + 0x00D4U)
#define ISP_REG_FRMFUN_0_KEY_B              (ISP_REG_BASE + 0x00D6U)

#define ISP_REG_FRMFUN_0_ADDR_L             (ISP_REG_BASE + 0x00D8U)
#define ISP_REG_FRMFUN_0_ADDR_H             (ISP_REG_BASE + 0x00DAU)
#define ISP_REG_FRMFUN_0_START_X            (ISP_REG_BASE + 0x00DCU)
#define ISP_REG_FRMFUN_0_START_Y            (ISP_REG_BASE + 0x00DEU)
#define ISP_REG_FRMFUN_0_WIDTH              (ISP_REG_BASE + 0x00E0U)
#define ISP_REG_FRMFUN_0_HEIGHT             (ISP_REG_BASE + 0x00E2U)
#define ISP_REG_FRMFUN_0_PITCH              (ISP_REG_BASE + 0x00E4U)

#define ISP_REG_SET_FRMFUN_1                (ISP_REG_BASE + 0x00E6U)
#define ISP_REG_CONST_ALPHA_1               (ISP_REG_BASE + 0x00E8U)
#define ISP_REG_FRMFUN_1_KEY_RG             (ISP_REG_BASE + 0x00EAU)
#define ISP_REG_FRMFUN_1_KEY_B              (ISP_REG_BASE + 0x00ECU)

#define ISP_REG_FRMFUN_1_ADDR_L             (ISP_REG_BASE + 0x00EEU)
#define ISP_REG_FRMFUN_1_ADDR_H             (ISP_REG_BASE + 0x00F0U)
#define ISP_REG_FRMFUN_1_START_X            (ISP_REG_BASE + 0x00F2U)
#define ISP_REG_FRMFUN_1_START_Y            (ISP_REG_BASE + 0x00F4U)
#define ISP_REG_FRMFUN_1_WIDTH              (ISP_REG_BASE + 0x00F6U)
#define ISP_REG_FRMFUN_1_HEIGHT             (ISP_REG_BASE + 0x00F8U)
#define ISP_REG_FRMFUN_1_PITCH              (ISP_REG_BASE + 0x00FAU)

#define ISP_BIT_FRMFUN_UIDEC_EN             0x0001          //0000 0100 0000 0000
#define ISP_BIT_FRMFUN_GOBANG_EN            0x0001          //0000 0010 0000 0000
#define ISP_BIT_FRMFUN_RGB2YUV_EN           0x0001          //0000 0001 0000 0000
#define ISP_BIT_FRMFUN_GRID_PIXEL_MODE      0x0001          //0000 0000 0100 0000
#define ISP_BIT_FRMFUN_GRID_CONST_DATA_EN   0x0001          //0000 0000 0010 0000
#define ISP_BIT_FRMFUN_CONST_DATA_BLEND_EN  0x0001          //0000 0000 0001 0000
#define ISP_BIT_FRMFUN_MODE                 0x0003          //0000 0000 0000 1100
#define ISP_BIT_FRMFUN_ALPHA_BLEND_EN       0x0001          //0000 0000 0000 0010
#define ISP_BIT_FRMFUN_EN                   0x0001          //0000 0000 0000 0001
#define ISP_BIT_CONST_ALPHA                 0x00FF          //0000 0000 1111 1111
#define ISP_BIT_FRMFUN_KEY                  0x00FF          //0000 0000 1111 1111

#define ISP_BIT_FRMFUN_ADDR_L               0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_FRMFUN_ADDR_H               0x07FF          //0000 0111 1111 1111
#define ISP_BIT_FRMFUN_START_X              0x07FF          //0000 0111 1111 1111
#define ISP_BIT_FRMFUN_START_Y              0x07FF          //0000 0111 1111 1111
#define ISP_BIT_FRMFUN_WIDTH                0x07FF          //0000 0111 1111 1111
#define ISP_BIT_FRMFUN_HEIGHT               0x07FF          //0000 0111 1111 1111
#define ISP_BIT_FRMFUN_PITCH                0x7FFF          //0111 1111 1111 1111

#define ISP_SHT_FRMFUN_UIDEC_EN             10
#define ISP_SHT_FRMFUN_GOBANG_EN            9
#define ISP_SHT_FRMFUN_RGB2YUV_EN           8
#define ISP_SHT_FRMFUN_GRID_PIXEL_MODE      6
#define ISP_SHT_FRMFUN_GRID_CONST_DATA_EN   5
#define ISP_SHT_FRMFUN_CONST_DATA_BLEND_EN  4
#define ISP_SHT_FRMFUN_MODE                 2
#define ISP_SHT_FRMFUN_ALPHA_BLEND_EN       1
#define ISP_SHT_FRMFUN_EN                   0

#define ISP_SHT_FRMFUN_KEY_R                0
#define ISP_SHT_FRMFUN_KEY_G                8

//================================================================
// 0x05FC
// VC1 register
//================================================================
#define ISP_REG_SET_VC1                     (ISP_REG_BASE + 0x00FCU)

#define ISP_BIT_VC1_RANGE_MAP_UV_EN         0x0001          //0000 0010 0000 0000
#define ISP_BIT_VC1_RANGE_MAP_Y_EN          0x0001          //0000 0001 0000 0000

#define ISP_BIT_VC1_RANGE_MAP_UV_FACTOR     0x0007          //0000 0000 0111 0000
#define ISP_BIT_VC1_RANGE_MAP_Y_FACTOR      0x0007          //0000 0000 0000 0111

#define ISP_SHT_VC1_RANGE_MAP_UV_EN         9
#define ISP_SHT_VC1_RANGE_MAP_Y_EN          8
#define ISP_SHT_VC1_RANGE_MAP_UV_FACTOR     4
#define ISP_SHT_VC1_RANGE_MAP_Y_FACTOR      0

//================================================================
// 0x05FC
//================================================================
#define ISP_REG_PVOUTPUT_FORMAT             (ISP_REG_BASE + 0x00FCU)

#define ISP_BIT_PVOUT_RGB_FORMAT            0x0003
#define ISP_BIT_PVOUT_YUVPACKET_FORMAT      0x0003
#define ISP_BIT_PVOUT_DITHER_MODE           0x0001

#define ISP_SHT_PVOUT_YUVPACKET_FORMAT      6
#define ISP_SHT_PVOUT_RGB_FORMAT            4
#define ISP_SHT_PVOUT_DITHER_MODE           3
#define ISP_SHT_PVOUT_YUVPACKET_EN          1
#define ISP_SHT_PVOUT_RGB_EN                0

//================================================================
// 0x05FE ~ 0x0608
// Extend Source register
//================================================================
#define ISP_REG_PANEL_SRCPOS_X              (ISP_REG_BASE + 0x00FEU)
#define ISP_REG_PANEL_SRCPOS_Y              (ISP_REG_BASE + 0x0100U)
#define ISP_REG_PANEL_SRC_WIDTH             (ISP_REG_BASE + 0x0102U)
#define ISP_REG_PANEL_SRC_HEIGHT            (ISP_REG_BASE + 0x0104U)
#define ISP_REG_PANEL_COLOR_YU              (ISP_REG_BASE + 0x0106U)
#define ISP_REG_PANEL_COLOR_V               (ISP_REG_BASE + 0x0108U)

#define ISP_BIT_PANEL_SRCPOS_X              0x7FFF          //0111 1111 1111 1111
#define ISP_BIT_PANEL_SRCPOS_Y              0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_PANEL_SRC_WIDTH             0x7FFF          //0111 1111 1111 1111
#define ISP_BIT_PANEL_SRC_HEIGHT            0x3FFF          //0011 1111 1111 1111
#define ISP_BIT_PANEL_COLOR                 0x00FF          //0000 0000 1111 1111

#define ISP_SHT_PANEL_COLOR_Y               0
#define ISP_SHT_PANEL_COLOR_U               8

//================================================================
// 0x060A
// Hardware debug register
//================================================================
#define ISP_REG_CLOCK_SETTING               (ISP_REG_BASE + 0x010AU)

#define ISP_BIT_DEBUG_SEL                   0x000F          //1111 0000 0000 0000
#define ISP_BIT_ICLK_SRC_SEL                0x0003          //0000 0000 0110 0000
#define ISP_BIT_JPEG_SIMU_MODE              0x0001          //0000 0000 0001 0000
#define ISP_BIT_TEST_AUTO_FIRE_EN           0x0001          //0000 0000 0000 0010
#define ISP_BIT_TEST_SYNC_ERR               0x0001          //0000 0000 0000 0001

#define ISP_SHT_DEBUG_SEL                   12
#define ISP_SHT_ICLK_SRC_SEL                5
#define ISP_SHT_JPEG_SIMU_MODE              4
#define ISP_SHT_TEST_AUTO_FIRE_EN           1
#define ISP_SHT_TEST_SYNC_ERR               0

#if (CFG_CHIP_FAMILY == 9860)
//================================================================
// 0x060A
// Output RGB Alpha
//================================================================
#define ISP_REG_OUT_RGB_ALPHA               (ISP_REG_BASE + 0x010AU)

#define ISP_BIT_OUT_RGB_ALPHA_EN            0x0001          //0000 0001 0000 0000
#define ISP_BIT_OUT_RGB_ALPHA               0x00FF          //0000 0000 1111 1111

#define ISP_SHT_OUT_RGB_ALPHA_EN            8
#define ISP_SHT_OUT_RGB_ALPHA               0
#endif

//================================================================
// Multiple Output
//================================================================
#define ISP_REG_PREVIEWMODE_PARM            (ISP_REG_BASE + 0x010AU)

#define ISP_BIT_PRESCALE_SEL                0x0001
#define ISP_BIT_PREVIEW_EN                  0x0001

#define ISP_SHT_PRESCALE_SEL                1
#define ISP_SHT_PREVIEW_EN                  0

//================================================================
// Preview Scaling factor
//================================================================
#define ISP_REG_PVSCALE_HCI_L               (ISP_REG_BASE + 0x010CU)   // HCI
#define ISP_REG_PVSCALE_HCI_H               (ISP_REG_BASE + 0x010EU)   // HCI
#define ISP_REG_PVSCALE_VCI_L               (ISP_REG_BASE + 0x0110U)   // VCI
#define ISP_REG_PVSCALE_VCI_H               (ISP_REG_BASE + 0x0112U)   // VCI

#define ISP_BIT_PVSCALE_L                   0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_PVSCALE_H                   0x000F          //0000 0000 0000 1111

//================================================================
// 0x0614 ~ 0x063A
// Scale weighting matrix
//================================================================
#define ISP_REG_SCALEWX0100                 (ISP_REG_BASE + 0x0114U)
#define ISP_REG_SCALEWX0302                 (ISP_REG_BASE + 0x0116U)
#define ISP_REG_SCALEWX1110                 (ISP_REG_BASE + 0x0118U)
#define ISP_REG_SCALEWX1312                 (ISP_REG_BASE + 0x011AU)
#define ISP_REG_SCALEWX2120                 (ISP_REG_BASE + 0x011CU)
#define ISP_REG_SCALEWX2322                 (ISP_REG_BASE + 0x011EU)
#define ISP_REG_SCALEWX3130                 (ISP_REG_BASE + 0x0120U)
#define ISP_REG_SCALEWX3332                 (ISP_REG_BASE + 0x0122U)
#define ISP_REG_SCALEWX4140                 (ISP_REG_BASE + 0x0124U)
#define ISP_REG_SCALEWX4342                 (ISP_REG_BASE + 0x0126U)

#define ISP_REG_SCALEWY0100                 (ISP_REG_BASE + 0x0128U)
#define ISP_REG_SCALEWY0302                 (ISP_REG_BASE + 0x012AU)
#define ISP_REG_SCALEWY1110                 (ISP_REG_BASE + 0x012CU)
#define ISP_REG_SCALEWY1312                 (ISP_REG_BASE + 0x012EU)
#define ISP_REG_SCALEWY2120                 (ISP_REG_BASE + 0x0130U)
#define ISP_REG_SCALEWY2322                 (ISP_REG_BASE + 0x0132U)
#define ISP_REG_SCALEWY3130                 (ISP_REG_BASE + 0x0134U)
#define ISP_REG_SCALEWY3332                 (ISP_REG_BASE + 0x0136U)
#define ISP_REG_SCALEWY4140                 (ISP_REG_BASE + 0x0138U)
#define ISP_REG_SCALEWY4342                 (ISP_REG_BASE + 0x013AU)

#define ISP_BIT_SCALEWEIGHT                 0x00FF          //0000 0000 1111 1111

#define ISP_SHT_SCALEWEIGHT_H               8
#define ISP_SHT_SCALEWEIGHT_L               0

//================================================================
// 0x063C ~ 0x0654
// PreScale function
//================================================================
#define ISP_REG_PRESCALE_HCI_L              (ISP_REG_BASE + 0x013CU)
#define ISP_REG_PRESCALE_HCI_H              (ISP_REG_BASE + 0x013EU)
#define ISP_REG_PRESCALE_WIDTH              (ISP_REG_BASE + 0x0140U)

#define ISP_REG_PRESCALE_WX0100             (ISP_REG_BASE + 0x0142U)
#define ISP_REG_PRESCALE_WX0302             (ISP_REG_BASE + 0x0144U)
#define ISP_REG_PRESCALE_WX1110             (ISP_REG_BASE + 0x0146U)
#define ISP_REG_PRESCALE_WX1312             (ISP_REG_BASE + 0x0148U)
#define ISP_REG_PRESCALE_WX2120             (ISP_REG_BASE + 0x014AU)
#define ISP_REG_PRESCALE_WX2322             (ISP_REG_BASE + 0x014CU)
#define ISP_REG_PRESCALE_WX3130             (ISP_REG_BASE + 0x014EU)
#define ISP_REG_PRESCALE_WX3332             (ISP_REG_BASE + 0x0150U)
#define ISP_REG_PRESCALE_WX4140             (ISP_REG_BASE + 0x0152U)
#define ISP_REG_PRESCALE_WX4342             (ISP_REG_BASE + 0x0154U)

#define ISP_BIT_PRESCALE_HCI_L              0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_PRESCALE_HCI_H              0x000F          //0000 0000 0000 1111
#define ISP_BIT_PRESCALE_WIDTH              0x1FFF          //0001 1111 1111 1111
#define ISP_BIT_PRESCALEWEIGHT              0x00FF          //0000 0000 1111 1111

#define ISP_SHT_PRESCALEWEIGHT_H            8
#define ISP_SHT_PRESCALEWEIGHT_L            0

//================================================================
// 0x0656 ~ 0x065A
// LowLevelEdge
//================================================================
#define ISP_REG_CC_IN_OFFSET_R              (ISP_REG_BASE + 0x0156U)
#define ISP_REG_CC_IN_OFFSET_G              (ISP_REG_BASE + 0x0158U)
#define ISP_REG_CC_IN_OFFSET_B              (ISP_REG_BASE + 0x015AU)

#define ISP_BIT_IN_OFFSET_R                 0x01FF          //0000 0001 1111 1111
#define ISP_BIT_IN_OFFSET_G                 0x01FF          //0000 0001 1111 1111
#define ISP_BIT_IN_OFFSET_B                 0x01FF          //0000 0001 1111 1111

//================================================================
// 0x065C ~ 0x0662
// LowLevelEdge
//================================================================
#define ISP_REG_LOWLEVELEDGE_START_X        (ISP_REG_BASE + 0x015CU)
#define ISP_REG_LOWLEVELEDGE_START_Y        (ISP_REG_BASE + 0x015EU)
#define ISP_REG_LOWLEVELEDGE_WIDTH          (ISP_REG_BASE + 0x0160U)
#define ISP_REG_LOWLEVELEDGE_HEIGHT         (ISP_REG_BASE + 0x0162U)

//================================================================
// 0x0664 ~ 0x067A
//
// Transfer matrix for RGB to YUV
//================================================================
#define ISP_REG_RGB_TO_YUV_11               (ISP_REG_BASE + 0x0164U)
#define ISP_REG_RGB_TO_YUV_12               (ISP_REG_BASE + 0x0166U)
#define ISP_REG_RGB_TO_YUV_13               (ISP_REG_BASE + 0x0168U)
#define ISP_REG_RGB_TO_YUV_21               (ISP_REG_BASE + 0x016AU)
#define ISP_REG_RGB_TO_YUV_22               (ISP_REG_BASE + 0x016CU)
#define ISP_REG_RGB_TO_YUV_23               (ISP_REG_BASE + 0x016EU)
#define ISP_REG_RGB_TO_YUV_31               (ISP_REG_BASE + 0x0170U)
#define ISP_REG_RGB_TO_YUV_32               (ISP_REG_BASE + 0x0172U)
#define ISP_REG_RGB_TO_YUV_33               (ISP_REG_BASE + 0x0174U)

#define ISP_REG_RGB_TO_YUV_CONST_Y          (ISP_REG_BASE + 0x0176U)
#define ISP_REG_RGB_TO_YUV_CONST_U          (ISP_REG_BASE + 0x0178U)
#define ISP_REG_RGB_TO_YUV_CONST_V          (ISP_REG_BASE + 0x017AU)

#define ISP_BIT_RGB_TO_YUV                  0x03FF          //0000 0011 1111 1111
#define ISP_BIT_RGB_TO_YUV_CONST            0x01FF          //0000 0001 1111 1111

//================================================================
// 0x067C ~ 0x680
// update parameter
//================================================================
#define ISP_REG_UPDATE_PARA_1               (ISP_REG_BASE + 0x017CU)
#define ISP_REG_UPDATE_PARA_2               (ISP_REG_BASE + 0x017EU)
#define ISP_REG_UPDATE_PARA_3               (ISP_REG_BASE + 0x0180U)

#define ISP_BIT_UPDATE_PARA_1               0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_UPDATE_PARA_2               0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_UPDATE_PARA_3               0xFFFF          //1111 1111 1111 1111

//================================================================
// 0x0682 ~ 0x069E
// Clipping function 0 1 2
//================================================================
#define ISP_REG_SET_CLIP_0                  (ISP_REG_BASE + 0x0182U)

#define ISP_REG_CLIP_0_LEFT                 (ISP_REG_BASE + 0x0184U)
#define ISP_REG_CLIP_0_RIGHT                (ISP_REG_BASE + 0x0186U)
#define ISP_REG_CLIP_0_TOP                  (ISP_REG_BASE + 0x0188U)
#define ISP_REG_CLIP_0_BOTTOM               (ISP_REG_BASE + 0x018AU)

#define ISP_REG_SET_CLIP_1                  (ISP_REG_BASE + 0x018CU)

#define ISP_REG_CLIP_1_LEFT                 (ISP_REG_BASE + 0x018EU)
#define ISP_REG_CLIP_1_RIGHT                (ISP_REG_BASE + 0x0190U)
#define ISP_REG_CLIP_1_TOP                  (ISP_REG_BASE + 0x0192U)
#define ISP_REG_CLIP_1_BOTTOM               (ISP_REG_BASE + 0x0194U)

#define ISP_REG_SET_CLIP_2                  (ISP_REG_BASE + 0x0196U)

#define ISP_REG_CLIP_2_LEFT                 (ISP_REG_BASE + 0x0198U)
#define ISP_REG_CLIP_2_RIGHT                (ISP_REG_BASE + 0x019AU)
#define ISP_REG_CLIP_2_TOP                  (ISP_REG_BASE + 0x019CU)
#define ISP_REG_CLIP_2_BOTTOM               (ISP_REG_BASE + 0x019EU)

#define ISP_BIT_CLIP_REGION                 0x0001          //0000 0000 0000 0010
#define ISP_BIT_CLIP_EN                     0x0001          //0000 0000 0000 0001

#define ISP_REG_CLIP_LEFT                   0x3FFF          //0011 1111 1111 1111
#define ISP_REG_CLIP_RIGHT                  0x3FFF          //0011 1111 1111 1111
#define ISP_REG_CLIP_TOP                    0x3FFF          //0011 1111 1111 1111
#define ISP_REG_CLIP_BOTTOM                 0x3FFF          //0011 1111 1111 1111

#define ISP_SHT_CLIP_REGION                 1
#define ISP_SHT_CLIP_EN                     0

//================================================================
// 0x06A0 ~ 0x06AA
// Frame function Const Data
//================================================================
#define ISP_REG_CONST_DATA_R0G0_0           (ISP_REG_BASE + 0x01A0U)
#define ISP_REG_CONST_DATA_B0R1_0           (ISP_REG_BASE + 0x01A2U)
#define ISP_REG_CONST_DATA_G1B1_0           (ISP_REG_BASE + 0x01A4U)

#define ISP_REG_CONST_DATA_R0G0_1           (ISP_REG_BASE + 0x01A6U)
#define ISP_REG_CONST_DATA_B0R1_1           (ISP_REG_BASE + 0x01A8U)
#define ISP_REG_CONST_DATA_G1B1_1           (ISP_REG_BASE + 0x01AAU)

#define ISP_BIT_CONST_DATA                  0x00FF          //0000 0000 1111 1111

#define ISP_SHT_CONST_DATA_LOW              0
#define ISP_SHT_CONST_DATA_HI               8

//================================================================
// 0x06AC ~ 0x06CA
// Frame function and subtitle decompress setting (2-nd, for feild Top/Bottom case)
//================================================================
#define ISP_REG_FRM0_DEC_TTAL_BYTE_L_2      (ISP_REG_BASE + 0x01ACU)
#define ISP_REG_FRM0_DEC_TTAL_BYTE_H_2      (ISP_REG_BASE + 0x01AEU)
#define ISP_REG_FRM0_DEC_SRC_ADDR_L_2       (ISP_REG_BASE + 0x01BCU)
#define ISP_REG_FRM0_DEC_SRC_ADDR_H_2       (ISP_REG_BASE + 0x01BEU)

#define ISP_REG_FRM1_DEC_TTAL_BYTE_L_2      (ISP_REG_BASE + 0x01B0U)
#define ISP_REG_FRM1_DEC_TTAL_BYTE_H_2      (ISP_REG_BASE + 0x01B2U)
#define ISP_REG_FRM1_DEC_SRC_ADDR_L_2       (ISP_REG_BASE + 0x01C0U)
#define ISP_REG_FRM1_DEC_SRC_ADDR_H_2       (ISP_REG_BASE + 0x01C2U)

#define ISP_REG_SUBTL0_DEC_TTAL_BYTE_L_2    (ISP_REG_BASE + 0x01B4U)
#define ISP_REG_SUBTL0_DEC_TTAL_BYTE_H_2    (ISP_REG_BASE + 0x01B6U)
#define ISP_REG_SUBTL0_DEC_SRC_ADDR_L_2     (ISP_REG_BASE + 0x01C4U)
#define ISP_REG_SUBTL0_DEC_SRC_ADDR_H_2     (ISP_REG_BASE + 0x01C6U)

#define ISP_REG_SUBTL1_DEC_TTAL_BYTE_L_2    (ISP_REG_BASE + 0x01B8U)
#define ISP_REG_SUBTL1_DEC_TTAL_BYTE_H_2    (ISP_REG_BASE + 0x01BAU)
#define ISP_REG_SUBTL1_DEC_SRC_ADDR_L_2     (ISP_REG_BASE + 0x01C8U)
#define ISP_REG_SUBTL1_DEC_SRC_ADDR_H_2     (ISP_REG_BASE + 0x01CAU)

#define ISP_BIT_FRM_DEC_SRC_ADDR_L_2        0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_FRM_DEC_SRC_ADDR_H_2        0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_SUBTL_DEC_SRC_ADDR_L_2      0xFFFF          //1111 1111 1111 1111
#define ISP_BIT_SUBTL_DEC_SRC_ADDR_H_2      0xFFFF          //1111 1111 1111 1111

//================================================================
// 0x06CC ~ 0x06E2
// Transfer matrix Frmfun for RGB to YUV
//================================================================
#define ISP_REG_FRM_RGB2YUV_11              (ISP_REG_BASE + 0x01CCU)
#define ISP_REG_FRM_RGB2YUV_12              (ISP_REG_BASE + 0x01CEU)
#define ISP_REG_FRM_RGB2YUV_13              (ISP_REG_BASE + 0x01D0U)
#define ISP_REG_FRM_RGB2YUV_21              (ISP_REG_BASE + 0x01D2U)
#define ISP_REG_FRM_RGB2YUV_22              (ISP_REG_BASE + 0x01D4U)
#define ISP_REG_FRM_RGB2YUV_23              (ISP_REG_BASE + 0x01D6U)
#define ISP_REG_FRM_RGB2YUV_31              (ISP_REG_BASE + 0x01D8U)
#define ISP_REG_FRM_RGB2YUV_32              (ISP_REG_BASE + 0x01DAU)
#define ISP_REG_FRM_RGB2YUV_33              (ISP_REG_BASE + 0x01DCU)

#define ISP_REG_FRM_RGB2YUV_CONST_Y         (ISP_REG_BASE + 0x01DEU)
#define ISP_REG_FRM_RGB2YUV_CONST_U         (ISP_REG_BASE + 0x01E0U)
#define ISP_REG_FRM_RGB2YUV_CONST_V         (ISP_REG_BASE + 0x01E2U)

#define ISP_BIT_FRM_RGB2YUV                 0x03FF          //0000 0011 1111 1111
#define ISP_BIT_FRM_RGB2YUV_CONST           0x01FF          //0000 0001 1111 1111

//================================================================
// 0x06E4 ~ 0x0
// Frame function and subtitle decompress setting
//================================================================
#define ISP_REG_FRMFUN_0_DEC_LINEBYTE       (ISP_REG_BASE + 0x01E4U)
#define ISP_REG_FRMFUN_0_DEC_TOTALBYTE_L    (ISP_REG_BASE + 0x01E6U)
#define ISP_REG_FRMFUN_0_DEC_TOTALBYTE_H    (ISP_REG_BASE + 0x01E8U)

#define ISP_REG_FRMFUN_1_DEC_LINEBYTE       (ISP_REG_BASE + 0x01EAU)
#define ISP_REG_FRMFUN_1_DEC_TOTALBYTE_L    (ISP_REG_BASE + 0x01ECU)
#define ISP_REG_FRMFUN_1_DEC_TOTALBYTE_H    (ISP_REG_BASE + 0x01EEU)

#define ISP_REG_SUBTITLE_0_DEC_LINEBYTE     (ISP_REG_BASE + 0x01F0U)
#define ISP_REG_SUBTITLE_0_DEC_TOTALBYTE_L  (ISP_REG_BASE + 0x01F2U)
#define ISP_REG_SUBTITLE_0_DEC_TOTALBYTE_H  (ISP_REG_BASE + 0x01F4U)

#define ISP_REG_SUBTITLE_1_DEC_LINEBYTE     (ISP_REG_BASE + 0x01F6U)
#define ISP_REG_SUBTITLE_1_DEC_TOTALBYTE_L  (ISP_REG_BASE + 0x01F8U)
#define ISP_REG_SUBTITLE_1_DEC_TOTALBYTE_H  (ISP_REG_BASE + 0x01FAU)

#define ISP_REG_FRMFUN_DEC_LINEBYTE         0x7FFF          //0111 1111 1111 1111
#define ISP_REG_FRMFUN_DEC_TOTALBYTE_L      0xFFFF          //1111 1111 1111 1111
#define ISP_REG_FRMFUN_DEC_TOTALBYTE_H      0xFFFF          //1111 1111 1111 1111

#define ISP_BIT_SUBTITLE_DEC_LINEBYTE       0x7FFF          //0111 1111 1111 1111
#define ISP_REG_SUBTITLE_DEC_TOTALBYTE_L    0xFFFF          //1111 1111 1111 1111
#define ISP_REG_SUBTITLE_DEC_TOTALBYTE_H    0xFFFF          //1111 1111 1111 1111

//================================================================
// 0x06FC
// ISP engine status
//================================================================
#define ISP_REG_ISP_ENGINE_STATUS           (ISP_REG_BASE + 0x01FCU)

//================================================================
// 0x0700
// ISP Current Buffer status
//================================================================
#define ISP_REG_ISP_CURR_FLIP_STATUS        (ISP_REG_BASE + 0x0200U)

//================================================================
// 0x0774
// ISP UI Buffer status
//================================================================
#define ISP_REG_ISP_UIBUFFER_STATUS         (ISP_REG_BASE + 0x0274U)

#if (CFG_CHIP_FAMILY == 9860)
//================================================================
// 0x08FC
// ISP Write Buffer status
//================================================================
#define ISP_REG_WB_OUT_STATUS               (ISP_REG_BASE + 0x03FCU)

#define ISP_BIT_WB_STATUS_FINISH_INDEX      0x0003          //0110 0000 0000 0000
#define ISP_BIT_WB_STATUS_CURRENT_INDEX     0x0003          //0000 0011 0000 0000

#define ISP_SHT_WB_STATUS_FINISH_INDEX      13
#define ISP_SHT_WB_STATUS_CURRENT_INDEX     8
#endif

#ifdef __cplusplus
}
#endif

#endif
