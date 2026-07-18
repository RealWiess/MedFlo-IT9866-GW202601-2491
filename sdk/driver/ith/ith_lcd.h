#ifndef ITE_ITH_LCD_H
#define ITE_ITH_LCD_H

#include <stdbool.h>
#include "ith/ith_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ith_lcd LCD
 *  @{
 */
typedef struct EV_MATRIX_TAG
{
    float M_color_correction[3][3];
    float V_color_correction[3][1];
} EV_MATRIX;

#define GAMMA_CURVE_LUT_SIZE    65

typedef struct GAMMA_FUNC_CURVE_TAG
{
    float x[GAMMA_CURVE_LUT_SIZE];
    float y[GAMMA_CURVE_LUT_SIZE];
}GAMMA_FUNC_CURVE;

#define ITH_LCD_SET1_REG                0x0000U
#define ITH_LCD_LAYER_SYNC_RQ_BIT       4
#define ITH_LCD_ON_FLY_EN_BIT           7
#define ITH_LCD_CCIR_MODE_BIT           12
#define ITH_LCD_HW_FLIP_BIT             16
#define ITH_LCD_SW_FLIP_MODE_BIT        17
#define ITH_LCD_HSYNC_FLIP_BIT          18
#define ITH_LCD_VIDEO_FLIP_EN_BIT       20
#define ITH_LCD_VIDEO_FLIP_MODE_BIT     21
#define ITH_LCD_FLIP_BUF_CTRL_BIT       22
#define ITH_LCD_ROT_MODE_BIT            28U
#define ITH_LCD_ROT_MODE_MASK           (N02_BITS_MSK << ITH_LCD_ROT_MODE_BIT)
#define ITH_LCD_SCAN_TYPE_BIT           30U

#define ITH_LCD_SET_MODE_REG            0x0004U
#define ITH_LCD_DISPLAY_MODE_BIT        0
#define ITH_LCD_DISPLAY_MODE_MASK       (N12_BITS_MSK << ITH_LCD_DISPLAY_MODE_BIT)

#define ITH_LCD_SWFLIPNUM_REG           0x0004U
#define ITH_LCD_SWFLIPNUM_BIT           30
#define ITH_LCD_SWFLIPNUM_MASK          (N02_BITS_MSK << ITH_LCD_SWFLIPNUM_BIT)

#define ITH_LCD_SRCFMT_REG              0x0004U
#define ITH_LCD_SRCFMT_BIT              12
#define ITH_LCD_SRCFMT_MASK             (N03_BITS_MSK << ITH_LCD_SRCFMT_BIT)

#define ITH_LCD_BASEB_REG               0x0014U
#define ITH_LCD_BASEB_BIT               0U
#define ITH_LCD_BASEB_MASK              (0xFFFFFFF8U << ITH_LCD_BASEB_BIT)

#define ITH_LCD_BASEC_REG               0x0018U
#define ITH_LCD_BASEC_BIT               0U
#define ITH_LCD_BASEC_MASK              (0xFFFFFFF8U << ITH_LCD_BASEC_BIT)

#define ITH_LCD_UPDATE_REG              0x001CU
#define ITH_LCD_LAYER1UPDATE_BIT        31U
#define ITH_LCD_LAYER1UPDATE_MASK       (N01_BITS_MSK << ITH_LCD_LAYER1UPDATE_BIT)
#define ITH_LCD_DISPEN_BIT              1U
#define ITH_LCD_DISPEN_MASK             (N01_BITS_MSK << ITH_LCD_DISPEN_BIT)
#define ITH_LCD_SYNCFIRE_BIT            0U
#define ITH_LCD_SYNCFIRE_MASK           (N01_BITS_MSK << ITH_LCD_SYNCFIRE_BIT)

#define ITH_LCD_TEST_COLOR_SET_REG      0x0020U
#define ITH_LCD_COLOR_EN_BIT            24
#define ITH_LCD_COLOR_RING_BIT          25
#define ITH_LCD_COLOR_CYCLE_BIT         26
#define ITH_LCD_COLOR_CYCLE_MASK        (N02_BITS_MSK << ITH_LCD_COLOR_CYCLE_BIT)
#define ITH_LCD_COLOR_UPDATE_BIT        31

#define ITH_LCD_HWC_EN_REG              0x0034U
#define ITH_LCD_HWC_EN_BIT              30

#define ITH_LCD_HWC_WIDTH_REG           0x0024U
#define ITH_LCD_HWC_WIDTH_BIT           0
#define ITH_LCD_HWC_WIDTH_MASK          (N11_BITS_MSK << ITH_LCD_HWC_WIDTH_BIT)

#define ITH_LCD_HWC_HEIGHT_REG          0x0024U
#define ITH_LCD_HWC_HEIGHT_BIT          16
#define ITH_LCD_HWC_HEIGHT_MASK         (N11_BITS_MSK << ITH_LCD_HWC_HEIGHT_BIT)

#define ITH_LCD_HWC_PITCH_REG           0x0028U
#define ITH_LCD_HWC_PITCH_BIT           0U
#define ITH_LCD_HWC_PITCH_MASK          (0x1FFC << ITH_LCD_HWC_PITCH_BIT)

#define ITH_LCD_HWC_POSX_REG            0x002CU
#define ITH_LCD_HWC_POSX_BIT            0
#define ITH_LCD_HWC_POSX_MASK           (N11_BITS_MSK << ITH_LCD_HWC_POSX_BIT)

#define ITH_LCD_HWC_POSY_REG            0x002CU
#define ITH_LCD_HWC_POSY_BIT            16
#define ITH_LCD_HWC_POSY_MASK           (N11_BITS_MSK << ITH_LCD_HWC_POSY_BIT)

#define ITH_LCD_HWC_BASE_REG            0x0030U
#define ITH_LCD_HWC_BASE_BIT            0U
#define ITH_LCD_HWC_BASE_MASK           (0xFFFFFFF8U << ITH_LCD_HWC_BASE_BIT)

#define ITH_LCD_HWC_CR_REG              0x0034U
#define ITH_LCD_HWC_ABLDEN_BIT          27
#define ITH_LCD_HWC_DEFDSTEN_BIT        29
#define ITH_LCD_HWC_DEFINVDST_BIT       28

#define ITH_LCD_HWC_INVCOLORWEI_REG     0x0034U
#define ITH_LCD_HWC_INVCOLORWEI_BIT     16U
#define ITH_LCD_HWC_INVCOLORWEI_MASK    (N08_BITS_MSK << ITH_LCD_HWC_INVCOLORWEI_BIT)

#define ITH_LCD_HWC_DEFCOLOR_REG        0x003CU
#define ITH_LCD_HWC_DEFCOLOR_BIT        0U
#define ITH_LCD_HWC_DEFCOLOR_MASK       (N16_BITS_MSK << ITH_LCD_HWC_DEFCOLOR_BIT)

#define ITH_LCD_HWC_FORECOLOR_REG       0x0038U
#define ITH_LCD_HWC_FORECOLOR_BIT       0U
#define ITH_LCD_HWC_FORECOLOR_MASK      (N16_BITS_MSK << ITH_LCD_HWC_FORECOLOR_BIT)

#define ITH_LCD_HWC_BACKCOLOR_REG       0x0038U
#define ITH_LCD_HWC_BACKCOLOR_BIT       16
#define ITH_LCD_HWC_BACKCOLOR_MASK      (N16_BITS_MSK << ITH_LCD_HWC_BACKCOLOR_BIT)

#define ITH_LCD_HWC_FORECOLORWEI_REG    0x0034U
#define ITH_LCD_HWC_FORECOLORWEI_BIT    0
#define ITH_LCD_HWC_FORECOLORWEI_MASK   (N08_BITS_MSK << ITH_LCD_HWC_FORECOLORWEI_BIT)

#define ITH_LCD_HWC_BACKCOLORWEI_REG    0x0034U
#define ITH_LCD_HWC_BACKCOLORWEI_BIT    8U
#define ITH_LCD_HWC_BACKCOLORWEI_MASK   (N08_BITS_MSK << ITH_LCD_HWC_BACKCOLORWEI_BIT)

#define ITH_LCD_HWC_FIFO_RQ_THD_REG     0x0034U
#define ITH_LCD_HWC_FIFO_RQ_THD_BIT     24
#define ITH_LCD_HWC_FIFO_RQ_THD_MASK    (N03_BITS_MSK << ITH_LCD_HWC_FIFO_RQ_THD_BIT)

#define ITH_LCD_HWC_UPDATE_REG          0x0034
#define ITH_LCD_HWC_UPDATE_BIT          31

#define ITH_LCD_GAMMA_FUN_EN_REG        0x0040U
#define ITH_LCD_GAMMA_FUN_EN_BIT        31

#define ITH_LCD_GAMMA_R_PTR_REG         0x0040U
#define ITH_LCD_GAMMA_R_PTR_BIT         0
#define ITH_LCD_GAMMA_R_PTR_MASK        (N07_BITS_MSK << ITH_LCD_GAMMA_R_PTR_BIT)

#define ITH_LCD_GAMMA_R_VALUE_REG       0x0040U
#define ITH_LCD_GAMMA_R_VALUE_BIT       16
#define ITH_LCD_GAMMA_R_VALUE_MASK      (N10_BITS_MSK << ITH_LCD_GAMMA_R_VALUE_BIT)

#define ITH_LCD_GAMMA_G_PTR_REG         0x0044U
#define ITH_LCD_GAMMA_G_PTR_BIT         0
#define ITH_LCD_GAMMA_G_PTR_MASK        (N07_BITS_MSK << ITH_LCD_GAMMA_G_PTR_BIT)

#define ITH_LCD_GAMMA_G_VALUE_REG       0x0044U
#define ITH_LCD_GAMMA_G_VALUE_BIT       16
#define ITH_LCD_GAMMA_G_VALUE_MASK      (N10_BITS_MSK << ITH_LCD_GAMMA_G_VALUE_BIT)

#define ITH_LCD_GAMMA_B_PTR_REG         0x0048U
#define ITH_LCD_GAMMA_B_PTR_BIT         0
#define ITH_LCD_GAMMA_B_PTR_MASK        (N07_BITS_MSK << ITH_LCD_GAMMA_B_PTR_BIT)

#define ITH_LCD_GAMMA_B_VALUE_REG       0x0048U
#define ITH_LCD_GAMMA_B_VALUE_BIT       16
#define ITH_LCD_GAMMA_B_VALUE_MASK      (N10_BITS_MSK << ITH_LCD_GAMMA_B_VALUE_BIT)

#define ITH_LCD_RGB2YUV_REG             0x0050U
#define ITH_LCD_RGB2YUV_EN_BIT          0
#define ITH_LCD_YUV_PACKET_BIT          1
#define ITH_LCD_YUV_FORMAT_BIT          2
#define ITH_LCD_YUV_FORMAT_MASK         (N02_BITS_MSK << ITH_LCD_YUV_FORMAT_BIT)

#define ITH_LCD_RGB2YUV11_REG           0x0054U
#define ITH_LCD_RGB2YUV11_BIT           0
#define ITH_LCD_RGB2YUV11_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV11_BIT)

#define ITH_LCD_RGB2YUV12_REG           0x0054U
#define ITH_LCD_RGB2YUV12_BIT           16
#define ITH_LCD_RGB2YUV12_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV12_BIT)

#define ITH_LCD_RGB2YUV13_REG           0x0058U
#define ITH_LCD_RGB2YUV13_BIT           0
#define ITH_LCD_RGB2YUV13_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV13_BIT)

#define ITH_LCD_RGB2YUVC1_REG           0x0058U
#define ITH_LCD_RGB2YUVC1_BIT           16
#define ITH_LCD_RGB2YUVC1_MASK          (N09_BITS_MSK << ITH_LCD_RGB2YUVC1_BIT)

#define ITH_LCD_RGB2YUV21_REG           0x005CU
#define ITH_LCD_RGB2YUV21_BIT           0
#define ITH_LCD_RGB2YUV21_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV21_BIT)

#define ITH_LCD_RGB2YUV22_REG           0x005CU
#define ITH_LCD_RGB2YUV22_BIT           16
#define ITH_LCD_RGB2YUV22_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV22_BIT)

#define ITH_LCD_RGB2YUV23_REG           0x0060U
#define ITH_LCD_RGB2YUV23_BIT           0
#define ITH_LCD_RGB2YUV23_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV23_BIT)

#define ITH_LCD_RGB2YUVC2_REG           0x0060U
#define ITH_LCD_RGB2YUVC2_BIT           16
#define ITH_LCD_RGB2YUVC2_MASK          (N09_BITS_MSK << ITH_LCD_RGB2YUVC2_BIT)

#define ITH_LCD_RGB2YUV31_REG           0x0064U
#define ITH_LCD_RGB2YUV31_BIT           0
#define ITH_LCD_RGB2YUV31_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV31_BIT)

#define ITH_LCD_RGB2YUV32_REG           0x0064U
#define ITH_LCD_RGB2YUV32_BIT           16
#define ITH_LCD_RGB2YUV32_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV32_BIT)

#define ITH_LCD_RGB2YUV33_REG           0x0068U
#define ITH_LCD_RGB2YUV33_BIT           0
#define ITH_LCD_RGB2YUV33_MASK          (N10_BITS_MSK << ITH_LCD_RGB2YUV33_BIT)

#define ITH_LCD_RGB2YUVC3_REG           0x0068U
#define ITH_LCD_RGB2YUVC3_BIT           16
#define ITH_LCD_RGB2YUVC3_MASK          (N09_BITS_MSK << ITH_LCD_RGB2YUVC3_BIT)

#define ITH_LCD_TCON_CTG_REG            0x0070U
#define ITH_LCD_CTG0_EN_BIT             0
#define ITH_LCD_CTG1_EN_BIT             1
#define ITH_LCD_CTG2_EN_BIT             2
#define ITH_LCD_CTG3_EN_BIT             3
#define ITH_LCD_CTG4_EN_BIT             4
#define ITH_LCD_CTG5_EN_BIT             5
#define ITH_LCD_CTG6_EN_BIT             6
#define ITH_LCD_CTG7_EN_BIT             7
#define ITH_LCD_CTG_RST_BIT             8

#define ITH_LCD_CTG_HTOTAL_REG          0x0074U
#define ITH_LCD_CTG_HTOTAL_BIT          0
#define ITH_LCD_CTG_HTOTAL_MASK         (N12_BITS_MSK << ITH_LCD_CTG_HTOTAL_BIT)

#define ITH_LCD_CTG_VTOTAL_REG          0x0074U
#define ITH_LCD_CTG_VTOTAL_BIT          16
#define ITH_LCD_CTG_VTOTAL_MASK         (N12_BITS_MSK << ITH_LCD_CTG_VTOTAL_BIT)

#define ITH_LCD_CTG0_SET_REG            0x0078U

#define ITH_LCD_CTG_P1_Y_REG            0x0078U
#define ITH_LCD_CTG0_P2_ACT_REG         0x007CU
#define ITH_LCD_CTG0_P2_Y_REG           0x007CU
#define ITH_LCD_CTG0_P3_ACT_REG         0x0080U
#define ITH_LCD_CTG0_P3_Y_REG           0x0080U
#define ITH_LCD_CTG0_P4_ACT_REG         0x0084U
#define ITH_LCD_CTG0_P4_Y_REG           0x0084U

#define ITH_LCD_CTG1_SET_REG            0x0088U

#define ITH_LCD_CTG1_P1_Y_REG           0x0088U
#define ITH_LCD_CTG1_P2_ACT_REG         0x008CU
#define ITH_LCD_CTG1_P2_Y_REG           0x008CU
#define ITH_LCD_CTG1_P3_ACT_REG         0x0090U
#define ITH_LCD_CTG1_P3_Y_REG           0x0090U
#define ITH_LCD_CTG1_P4_ACT_REG         0x0094U
#define ITH_LCD_CTG1_P4_Y_REG           0x0094U

#define ITH_LCD_CTG2_SET_REG            0x0098U

#define ITH_LCD_CTG2_P1_Y_REG           0x0098U
#define ITH_LCD_CTG2_P2_ACT_REG         0x009CU
#define ITH_LCD_CTG2_P2_Y_REG           0x009CU
#define ITH_LCD_CTG2_P3_ACT_REG         0x00A0U
#define ITH_LCD_CTG2_P3_Y_REG           0x00A0U
#define ITH_LCD_CTG2_P4_ACT_REG         0x00A4U
#define ITH_LCD_CTG2_P4_Y_REG           0x00A4U

#define ITH_LCD_CTG3_SET_REG            0x00A8U

#define ITH_LCD_CTG3_P1_Y_REG           0x00A8U
#define ITH_LCD_CTG3_P2_ACT_REG         0x00ACU
#define ITH_LCD_CTG3_P2_Y_REG           0x00ACU
#define ITH_LCD_CTG3_P3_ACT_REG         0x00B0U
#define ITH_LCD_CTG3_P3_Y_REG           0x00B0U
#define ITH_LCD_CTG3_P4_ACT_REG         0x00B4U
#define ITH_LCD_CTG3_P4_Y_REG           0x00B4U

#define ITH_LCD_CTGH_CNT_REG            0x0168U
#define ITH_LCD_CTGH_CNT_BIT            0
#define ITH_LCD_CTGH_CNT_MASK           (N12_BITS_MSK << ITH_LCD_CTGH_CNT_BIT)

#define ITH_LCD_CTGV_CNT_BIT            16
#define ITH_LCD_CTGV_CNT_MASK           (N12_BITS_MSK << ITH_LCD_CTGV_CNT_BIT)

#define ITH_LCD_READ_STATUS1_REG        0x0170U
#define ITH_LCD_FLIP_NUM_BIT            0
#define ITH_LCD_FLIP_NUM_MASK           (N02_BITS_MSK << ITH_LCD_FLIP_NUM_BIT)

#define ITH_LCD_X_CNT_REG               0x016CU
#define ITH_LCD_X_CNT_BIT               0
#define ITH_LCD_X_CNT_MASK              (N12_BITS_MSK << ITH_LCD_X_CNT_BIT)

#define ITH_LCD_CRC_CHECK               0x0180

#define ITH_LCD_INT_CLR_REG             0x004CU
#define ITH_LCD_INT_CLR_BIT             31
#define ITH_LCD_INT_CLR_MASK            (N01_BITS_MSK << ITH_LCD_INT_CLR_BIT)

#define ITH_LCD_INT_CTRL_REG            0x004CU
#define ITH_LCD_INT_EN_BIT              16
#define ITH_LCD_INT_FIELDMODE1_BIT      17

#define ITH_LCD_INT_LINE1_REG           0x004CU
#define ITH_LCD_INT_LINE1_BIT           0
#define ITH_LCD_INT_LINE1_MASK          (N12_BITS_MSK << ITH_LCD_INT_LINE1_BIT)

#define ITH_LCD_CPUSERIAL_SET4_REG      0x00F8U

#define ITH_LCD_IO_EN_REG               0x0110U
#define ITH_LCD_IO_EN_BIT               2

#define ITH_LCD_LVDS_SET1_REG           0x01C0U

#define ITH_LCD_MIPI_SET1_REG           0x0230U

/** @} */ // end of ith_lcd

#ifdef __cplusplus
}
#endif

#endif
