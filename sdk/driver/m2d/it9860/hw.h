/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file hw.h
 *  GFX hardware layer API header file.
 *
 * @author Awin Huang
 * @version 1.0
 * @date 2014-05-28
 */

#ifndef __GFX_HW_H__
#define __GFX_HW_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                              Extern Reference
//=============================================================================
//=============================================================================
//                              Macro Definition
//=============================================================================
#if (CFG_CHIP_FAMILY == 9830)
#define GFX_REG_BASE_ADDR                   0xE0700000
#else
#define GFX_REG_BASE_ADDR                   0xB0700000
#endif

//  Source Base Address
#define GFX_REG_SRC_ADDR                    GFX_REG_BASE_ADDR + 0x0000

//  Source Coordinate Register, SRCXY (+0x018)
#define GFX_REG_SRCXY_ADDR                  GFX_REG_BASE_ADDR + 0x0004
#define GFX_REG_SRCXY_SRCX_MASK             0x1FFFU
#define GFX_REG_SRCXY_SRCX_OFFSET           16U
#define GFX_REG_SRCXY_SRCY_MASK             0x1FFFU
#define GFX_REG_SRCXY_SRCY_OFFSET           0U

//  Source Height/Width Register, SHWR (+0x01C)
#define GFX_REG_SHWR_ADDR                   GFX_REG_BASE_ADDR + 0x0008
#define GFX_REG_SHWR_SRCWIDTH_MASK          0x1FFFU
#define GFX_REG_SHWR_SRCWIDTH_OFFSET        16U
#define GFX_REG_SHWR_SRCHEIGHT_MASK         0x1FFFU
#define GFX_REG_SHWR_SRCHEIGHT_OFFSET       0U

//  Source Pitch
#define GFX_REG_SPR_ADDR                    GFX_REG_BASE_ADDR + 0x000C
#define GFX_REG_SPR_MASK                    0x7FFFU
#define GFX_REG_SPR_OFFSET                  0U

//  Mask Base Address
#define GFX_REG_MASK_ADDR                   GFX_REG_BASE_ADDR + 0x0010

//  Mask Coordinate Register, MASKXY (+0x018)
#define GFX_REG_MASKXY_ADDR                  GFX_REG_BASE_ADDR + 0x0014
#define GFX_REG_MASKXY_MASKX_MASK            0x1FFFU
#define GFX_REG_MASKXY_MASKX_OFFSET          16U
#define GFX_REG_MASKXY_MASKY_MASK            0x1FFFU
#define GFX_REG_MASKXY_MASKY_OFFSET          0U

//  Mask Height/Width Register, MHWR (+0x01C)
#define GFX_REG_MHWR_ADDR                   GFX_REG_BASE_ADDR + 0x0018
#define GFX_REG_MHWR_MASKWIDTH_MASK         0x1FFFU
#define GFX_REG_MHWR_MASKWIDTH_OFFSET       16U
#define GFX_REG_MHWR_MASKHEIGHT_MASK        0x1FFFU
#define GFX_REG_MHWR_MASKHEIGHT_OFFSET      0U

//  Mask Pitch
#define GFX_REG_MPR_ADDR                    GFX_REG_BASE_ADDR + 0x001c
#define GFX_REG_MPR_MASK                    0x7FFFU
#define GFX_REG_MPR_OFFSET                  0U

//  Destination Base Address
#define GFX_REG_DST_ADDR                    GFX_REG_BASE_ADDR + 0x0020

//  Destination Coordinate Register, DSTXY (+0x024)
#define GFX_REG_DSTXY_ADDR                  GFX_REG_BASE_ADDR + 0x0024
#define GFX_REG_DSTXY_DSTX_MASK             0x1FFFU
#define GFX_REG_DSTXY_DSTX_OFFSET           16U
#define GFX_REG_DSTXY_DSTY_MASK             0x1FFFU
#define GFX_REG_DSTXY_DSTY_OFFSET           0U

//  Destination Height/Width Register, DHWR (+0x028)
#define GFX_REG_DHWR_ADDR                   GFX_REG_BASE_ADDR + 0x0028
#define GFX_REG_DHWR_DSTWIDTH_MASK          0x1FFFU
#define GFX_REG_DHWR_DSTWIDTH_OFFSET        16U
#define GFX_REG_DHWR_DSTHEIGHT_MASK         0x1FFFU
#define GFX_REG_DHWR_DSTHEIGHT_OFFSET       0U

//  Destination Pitch Register, DPR (+0x02C)
#define GFX_REG_DPR_ADDR                    GFX_REG_BASE_ADDR + 0x002C
#define GFX_REG_DPR_MASK                    0x7FFFU
#define GFX_REG_DPR_OFFSET                  0U

//  Plan X Clipping Register, PXCR (+0x030)
#define GFX_REG_PXCR_ADDR                   GFX_REG_BASE_ADDR + 0x0030
#define GFX_REG_PXCR_CLIPXSTART_MASK        0x1FFFU
#define GFX_REG_PXCR_CLIPXSTART_OFFSET      16U
#define GFX_REG_PXCR_CLIPXEND_MASK          0x1FFFU
#define GFX_REG_PXCR_CLIPXEND_OFFSET        0U

//  Plan Y Clipping Register, PYCR (+0x034)
#define GFX_REG_PYCR_ADDR                   GFX_REG_BASE_ADDR + 0x0034
#define GFX_REG_PYCR_CLIPYSTART_MASK        0x1FFFU
#define GFX_REG_PYCR_CLIPYSTART_OFFSET      16U
#define GFX_REG_PYCR_CLIPYEND_MASK          0x1FFFU
#define GFX_REG_PYCR_CLIPYEND_OFFSET        0U

//  Line Draw End Coordinate Register, DSTXY (+0x038)
#define GFX_REG_LNEXY_ADDR                  GFX_REG_BASE_ADDR + 0x0038
#define GFX_REG_LNEXY_LNEX_MASK             0x1FFFU
#define GFX_REG_LNEXY_LNEX_OFFSET           16U
#define GFX_REG_LNEXY_LNEY_MASK             0x1FFFU
#define GFX_REG_LNEXY_LNEY_OFFSET           0U

//  Inverse Transform Matrix Register
#define GFX_REG_ITMR00_ADDR                 GFX_REG_BASE_ADDR + 0x0040
#define GFX_REG_ITMR01_ADDR                 GFX_REG_BASE_ADDR + 0x0044
#define GFX_REG_ITMR02_ADDR                 GFX_REG_BASE_ADDR + 0x0048
#define GFX_REG_ITMR10_ADDR                 GFX_REG_BASE_ADDR + 0x004C
#define GFX_REG_ITMR11_ADDR                 GFX_REG_BASE_ADDR + 0x0050
#define GFX_REG_ITMR12_ADDR                 GFX_REG_BASE_ADDR + 0x0054
#define GFX_REG_ITMR20_ADDR                 GFX_REG_BASE_ADDR + 0x0058
#define GFX_REG_ITMR21_ADDR                 GFX_REG_BASE_ADDR + 0x005C
#define GFX_REG_ITMR22_ADDR                 GFX_REG_BASE_ADDR + 0x0060

//  High Value of Source Color Key Register, FGCOLOR (+0x064)
#define GFX_REG_FGCOLOR_ADDR                GFX_REG_BASE_ADDR + 0x0064
#define GFX_REG_FGCOLOR_A_MASK              0xFFU
#define GFX_REG_FGCOLOR_A_OFFSET            24U
#define GFX_REG_FGCOLOR_R_MASK              0xFFU
#define GFX_REG_FGCOLOR_R_OFFSET            16U
#define GFX_REG_FGCOLOR_G_MASK              0xFFU
#define GFX_REG_FGCOLOR_G_OFFSET            8U
#define GFX_REG_FGCOLOR_B_MASK              0xFFU
#define GFX_REG_FGCOLOR_B_OFFSET            0U

//  Thick Line Parameter 1 Register, LINE_P1 (+0x068)
#define GFX_REG_LINE_P1_ADDR                GFX_REG_BASE_ADDR + 0x0068
#define GFX_REG_LINE_P1                     0U
#define GFX_REG_LINE_P1_MASK                0x1FFFFFU

//  Constant Alpha Register, CAR (+0x074)
#define GFX_REG_CAR_ADDR                    GFX_REG_BASE_ADDR + 0x0074
#define GFX_REG_CAR_CONSTALPHA_MASK         0xFFU
#define GFX_REG_CAR_CONSTALPHA_OFFSET       8U
#define GFX_REG_CAR_ROP_ROP3_MASK           0xFFU
#define GFX_REG_CAR_ROP_LINEDRAW_ROP2       0x03U
#define GFX_REG_CAR_ROP_OFFSET              0U

//  Safe Control Register (for Debugging), SAFE (+0x07C)
#define GFX_REG_SAFE_ADDR                   GFX_REG_BASE_ADDR + 0x007C
#define GFX_REG_SAFE_PROBE_MASK             0xFU
#define GFX_REG_SAFE_PROBE_OFFSET           28U
#define GFX_REG_SAFE_MANUALSCAN_EN          ((uint32_t)1 << 27)
#define GFX_REG_SAFE_SCANREXDIR_EN          ((uint32_t)1 << 26)
#define GFX_REG_SAFE_SCANREYDIR_EN          ((uint32_t)1 << 25)
#define GFX_REG_SAFE_WRREORDER_EN           ((uint32_t)1 << 19)
#define GFX_REG_SAFE_RDREORDER_EN           ((uint32_t)1 << 18)
#define GFX_REG_SAFE_BOUNDARY_INTERPOLATION ((uint32_t)1 << 13) //0 enable(default) for HUD, 1 disable
#define GFX_REG_SAFE_BUFMERGE_EN            ((uint32_t)1 << 10)
#define GFX_REG_SAFE_FORCEXFRM_EN           ((uint32_t)1 << 9)
#define GFX_REG_SAFE_PIXELIZE_EN            ((uint32_t)1 << 8)
//#define GFX_REG_SAFE_RQSIZE_MASK            0x7F
//#define GFX_REG_SAFE_RQSIZE_OFFSET          12
//#define GFX_REG_SAFE_RQSIZE_EN              (1 << 19)
#define GFX_REG_SAFE_BOUND_MASK             0x3FU
#define GFX_REG_SAFE_BOUND_OFFSET           2U
#define GFX_REG_SAFE_BOUND_EN               ((uint32_t)1 << 1)
#define GFX_REG_SAFE_AHBBLCOKMODE_EN        ((uint32_t)1 << 0)

//  Control Register 1, CR1 (+0x080)
#define GFX_REG_CR1_ADDR                    GFX_REG_BASE_ADDR + 0x0080
#define GFX_REG_CR1_RDCRC_EN                ((uint32_t)1 << 31)
#define GFX_REG_CR1_WRCRC_EN                ((uint32_t)1 << 30)
#define GFX_REG_CR1_ALUCRC_EN               ((uint32_t)1 << 29)
#define GFX_REG_CR1_RDCRC_UPD               ((uint32_t)1 << 28)
#define GFX_REG_CR1_WRCRC_UPD               ((uint32_t)1 << 27)
#define GFX_REG_CR1_ALUCRC_UPD              ((uint32_t)1 << 26)
#define GFX_REG_CR1_ALPHA_SEL_DEFAULT       ((uint32_t)0x00 << 22)
#define GFX_REG_CR1_ALPHA_SEL_SRC           ((uint32_t)0x01 << 22)
#define GFX_REG_CR1_ALPHA_SEL_DST           ((uint32_t)0x02 << 22)
#define GFX_REG_CR1_ALPHA_SEL_MAX           ((uint32_t)0x03 << 22)
#define GFX_REG_CR1_ROWMAJOR                ((uint32_t)1 << 21)
#define GFX_REG_CR1_TILESIZE                ((uint32_t)1 << 20)
#define GFX_REG_CR1_TILECRD_EN              ((uint32_t)1 << 19)
#define GFX_REG_CR1_MANUALTILEDCRD_EN       ((uint32_t)1 << 18)
#define GFX_REG_CR1_MANUALDITHER_EN         ((uint32_t)1 << 17)
#define GFX_REG_CR1_DITHER_EN               ((uint32_t)1 << 16)
#define GFX_REG_CR1_BGCOLOR_SEL             ((uint32_t)1 << 15)
#define GFX_REG_CR1_EXTEND_EN               ((uint32_t)1 << 14)
#define GFX_REG_CR1_MANUALCACHE_EN          ((uint32_t)1 << 13)
#define GFX_REG_CR1_CACHE_EN                ((uint32_t)1 << 12)
#define GFX_REG_CR1_CACHELINE               ((uint32_t)1 << 11)
#define GFX_REG_CR1_PERSPECTIVE_EN          ((uint32_t)1 << 10)
#define GFX_REG_CR1_TRANSFORM_EN            ((uint32_t)1 << 9)
#define GFX_REG_CR1_CLIP_EN                 ((uint32_t)1 << 7)
#define GFX_REG_CR1_INTERPOLATE_EN          ((uint32_t)1 << 6)
#define GFX_REG_CR1_TILEMODE_MASK           ((uint32_t)3 << 4)
#define GFX_REG_CR1_TILEMODE_OFFSET         (4U)
#define GFX_REG_CR1_TILEMODE_FILL           ((uint32_t)0x00 << 4)
#define GFX_REG_CR1_TILEMODE_PAD            ((uint32_t)0x01 << 4)
#define GFX_REG_CR1_TILEMODE_REPEAT         ((uint32_t)0x02 << 4)
#define GFX_REG_CR1_TILEMODE_REFLECT        ((uint32_t)0x03 << 4)
#define GFX_REG_CR1_TRANSPARENT_EN          ((uint32_t)1 << 3)
#define GFX_REG_CR1_MASK_EN                 ((uint32_t)1 << 2)
#define GFX_REG_CR1_ALPHABLEND_EN           ((uint32_t)1 << 1)
#define GFX_REG_CR1_CONSTALPHA_EN           ((uint32_t)1 << 0)

//  Control Register 2, CR2 (+0x084)
#define GFX_REG_CR2_ADDR                    GFX_REG_BASE_ADDR + 0x0084
#define GFX_REG_CR2_GRADMODE_H              ((uint32_t)0x00 << 20)
#define GFX_REG_CR2_GRADMODE_V              ((uint32_t)0x01 << 20)
#define GFX_REG_CR2_GRADMODE_BI             ((uint32_t)0x02 << 20)
#define GFX_REG_CR2_LSBFIRST_EN             ((uint32_t)1 << 18)
#define GFX_REG_CR2_MSKFORMAT_MASK          0x00030000U
#define GFX_REG_CR2_MSKFORMAT_OFFSET        16U
#define GFX_REG_CR2_MSKFORMAT_A8            ((uint32_t)0x00 << 16)
#define GFX_REG_CR2_MSKFORMAT_A4            ((uint32_t)0x01 << 16)
#define GFX_REG_CR2_MSKFORMAT_A2            ((uint32_t)0x02 << 16)
#define GFX_REG_CR2_MSKFORMAT_A1            ((uint32_t)0x03 << 16)
#define GFX_REG_CR2_DSTFORMAT_MASK          0x00000F00U
#define GFX_REG_CR2_DSTFORMAT_OFFSET        8U
#define GFX_REG_CR2_DSTFORMAT_RGBA8888      ((uint32_t)0x00 << 8)
#define GFX_REG_CR2_DSTFORMAT_ARGB8888      ((uint32_t)0x01 << 8)
#define GFX_REG_CR2_DSTFORMAT_BGRA8888      ((uint32_t)0x02 << 8)
#define GFX_REG_CR2_DSTFORMAT_ABGR8888      ((uint32_t)0x03 << 8)
#define GFX_REG_CR2_DSTFORMAT_RGBA5551      ((uint32_t)0x04 << 8)
#define GFX_REG_CR2_DSTFORMAT_ARGB1555      ((uint32_t)0x05 << 8)
#define GFX_REG_CR2_DSTFORMAT_BGRA5551      ((uint32_t)0x06 << 8)
#define GFX_REG_CR2_DSTFORMAT_ABGR1555      ((uint32_t)0x07 << 8)
#define GFX_REG_CR2_DSTFORMAT_RGBA4444      ((uint32_t)0x08 << 8)
#define GFX_REG_CR2_DSTFORMAT_ARGB4444      ((uint32_t)0x09 << 8)
#define GFX_REG_CR2_DSTFORMAT_BGRA4444      ((uint32_t)0x0A << 8)
#define GFX_REG_CR2_DSTFORMAT_ABGR4444      ((uint32_t)0x0B << 8)
#define GFX_REG_CR2_DSTFORMAT_RGB565        ((uint32_t)0x0C << 8)
#define GFX_REG_CR2_DSTFORMAT_BGR565        ((uint32_t)0x0D << 8)
#define GFX_REG_CR2_DSTFORMAT_A_8           ((uint32_t)0x0E << 8)
#define GFX_REG_CR2_SRCFORMAT_MASK          0x0000001FU
#define GFX_REG_CR2_SRCFORMAT_OFFSET        0U
#define GFX_REG_CR2_SRCFORMAT_RGBA8888      ((uint32_t)0x00 << 0)
#define GFX_REG_CR2_SRCFORMAT_ARGB8888      ((uint32_t)0x01 << 0)
#define GFX_REG_CR2_SRCFORMAT_BGRA8888      ((uint32_t)0x02 << 0)
#define GFX_REG_CR2_SRCFORMAT_ABGR8888      ((uint32_t)0x03 << 0)
#define GFX_REG_CR2_SRCFORMAT_RGBA5551      ((uint32_t)0x04 << 0)
#define GFX_REG_CR2_SRCFORMAT_ARGB1555      ((uint32_t)0x05 << 0)
#define GFX_REG_CR2_SRCFORMAT_BGRA5551      ((uint32_t)0x06 << 0)
#define GFX_REG_CR2_SRCFORMAT_ABGR1555      ((uint32_t)0x07 << 0)
#define GFX_REG_CR2_SRCFORMAT_RGBA4444      ((uint32_t)0x08 << 0)
#define GFX_REG_CR2_SRCFORMAT_ARGB4444      ((uint32_t)0x09 << 0)
#define GFX_REG_CR2_SRCFORMAT_BGRA4444      ((uint32_t)0x0A << 0)
#define GFX_REG_CR2_SRCFORMAT_ABGR4444      ((uint32_t)0x0B << 0)
#define GFX_REG_CR2_SRCFORMAT_RGB565        ((uint32_t)0x0C << 0)
#define GFX_REG_CR2_SRCFORMAT_BGR565        ((uint32_t)0x0D << 0)
#define GFX_REG_CR2_SRCFORMAT_A_8           ((uint32_t)0x0E << 0)
#define GFX_REG_CR2_SRCFORMAT_A_4           ((uint32_t)0x0F << 0)
#define GFX_REG_CR2_SRCFORMAT_A_2           ((uint32_t)0x10 << 0)
#define GFX_REG_CR2_SRCFORMAT_A_1           ((uint32_t)0x11 << 0)
#define GFX_REG_CR2_SRCFORMAT_XARGB         ((uint32_t)0x12 << 0)
#define GFX_REG_CR2_SRCFORMAT_XABGR         ((uint32_t)0x13 << 0)

//  Control Register 3, CR3 (+0x088)
#define GFX_REG_CR3_ADDR                    GFX_REG_BASE_ADDR + 0x0088
#define GFX_REG_CR3_LINE_P2_MASK            0xFFFFU
#define GFX_REG_CR3_LINE_P2_OFFSET          16U
#define GFX_REG_CR3_LINE_WIDTH_MASK         0xFFU
#define GFX_REG_CR3_LINE_WIDTH_OFFSET       8U
#define GFX_REG_CR3_LINEALGORITHM_0         ((uint32_t)0 << 2)
#define GFX_REG_CR3_LINEALGORITHM_1         ((uint32_t)1 << 2)
#define GFX_REG_CR3_LASTPIXEL_EN            ((uint32_t)1 << 1)
#define GFX_REG_CR3_LINE_TYPE0              ((uint32_t)0 << 0)
#define GFX_REG_CR3_LINE_TYPE1              ((uint32_t)1 << 0)

//  Command Register, CMD (+0x08C)
#define GFX_REG_CMD_ADDR                    GFX_REG_BASE_ADDR + 0x008C
#define GFX_REG_CMD_NULL_CMD                0x00U
#define GFX_REG_CMD_ROP3                    0x01U
#define GFX_REG_CMD_GRADIENT_FILL           0x02U
#define GFX_REG_CMD_LINE_DRAW               0x03U

#define GFX_REG_ICR_ADDR                    GFX_REG_BASE_ADDR + 0x0090
#define GFX_REG_ISR_ADDR                    GFX_REG_BASE_ADDR + 0x0094
#define GFX_REG_ID1_ADDR                    GFX_REG_BASE_ADDR + 0x0098
#define GFX_REG_ID2_ADDR                    GFX_REG_BASE_ADDR + 0x009C
#define GFX_REG_ID3_ADDR                    GFX_REG_BASE_ADDR + 0x00A0
#define GFX_REG_IDM1_ADDR                   GFX_REG_BASE_ADDR + 0x00A4
#define GFX_REG_IDM2_ADDR                   GFX_REG_BASE_ADDR + 0x00A8
#define GFX_REG_IDM3_ADDR                   GFX_REG_BASE_ADDR + 0x00AC
#define GFX_REG_REV_ADDR                    GFX_REG_BASE_ADDR + 0x00C0

// Status Register1, ST1(+0x0C4)
#define GFX_REG_ST1_ADDR                    GFX_REG_BASE_ADDR + 0x00C4
#define GFX_REG_ST1_LINEDRAW_IMPL           ((uint32_t)1 << 31)
#define GFX_REG_ST1_XFORM_IMPL              ((uint32_t)1 << 30)
#define GFX_REG_ST1_PERSPECTIVE_IMPL        ((uint32_t)1 << 29)
#define GFX_REG_ST1_GRADFILL_IMPL           ((uint32_t)1 << 28)
#define GFX_REG_ST1_CACHE_IMPL              ((uint32_t)1 << 27)
#define GFX_REG_ST1_MASK_IMPL               ((uint32_t)1 << 26)
#define GFX_REG_ST1_CRC32_IMPL              ((uint32_t)1 << 25)
#define GFX_REG_ST1_BUSY                    ((uint32_t)1 << 0)

#define GFX_REG_ST2_ADDR                    GFX_REG_BASE_ADDR + 0x00C8

#define GFX_REG_RDCRC_ADDR                  GFX_REG_BASE_ADDR + 0x00D0

#define GFX_REG_WRCRC_ADDR                  GFX_REG_BASE_ADDR + 0x00D4

#define GFX_REG_ALUCRC_ADDR                 GFX_REG_BASE_ADDR + 0x00D8

typedef enum _GFX_OBJ_ID
{
    GFX_ID1 = 0,
    GFX_ID2,
    GFX_ID3,
    LAST_GFX_ID
} GFX_OBJ_ID;

typedef void (*gfxBlitIdCallback)(void);

//=============================================================================
//                              Structure Definition
//=============================================================================
typedef struct _GFX_HW_REG
{
    const uint32_t  addr;
    uint32_t        data;
}GFX_HW_REG;

typedef struct _GFX_HW_REGS
{
    GFX_HW_REG  regSRC;     ///< 0x00
    GFX_HW_REG  regSRCXY;   ///< 0x04
    GFX_HW_REG  regSHWR;    ///< 0x08
    GFX_HW_REG  regSPR;     ///< 0x0C
    GFX_HW_REG  regMASK;    ///< 0x10
    GFX_HW_REG  regMASKXY;  ///< 0x14
    GFX_HW_REG  regMHWR;    ///< 0x18
    GFX_HW_REG  regMPR;     ///< 0x1C
    GFX_HW_REG  regDST;     ///< 0x20
    GFX_HW_REG  regDSTXY;   ///< 0x24
    GFX_HW_REG  regDHWR;    ///< 0x28
    GFX_HW_REG  regDPR;     ///< 0x2C
    GFX_HW_REG  regPXCR;    ///< 0x30
    GFX_HW_REG  regPYCR;    ///< 0x34
    GFX_HW_REG  regLNER;    ///< 0x38
    GFX_HW_REG  regITMR00;  ///< 0x40
    GFX_HW_REG  regITMR01;  ///< 0x44
    GFX_HW_REG  regITMR02;  ///< 0x48
    GFX_HW_REG  regITMR10;  ///< 0x4C
    GFX_HW_REG  regITMR11;  ///< 0x50
    GFX_HW_REG  regITMR12;  ///< 0x54
    GFX_HW_REG  regITMR20;  ///< 0x58
    GFX_HW_REG  regITMR21;  ///< 0x5C
    GFX_HW_REG  regITMR22;  ///< 0x60
    GFX_HW_REG  regFGCOLOR; ///< 0x64
    GFX_HW_REG  regLINEP1;  ///< 0x68
    GFX_HW_REG  regCAR;     ///< 0x74
    GFX_HW_REG  regSAFE;    ///< 0x7C
    GFX_HW_REG  regCR1;     ///< 0x80
    GFX_HW_REG  regCR2;     ///< 0x84
    GFX_HW_REG  regCR3;     ///< 0x88
    GFX_HW_REG  regCMD;     ///< 0x8C
    GFX_HW_REG  regICR;     ///< 0x90
    GFX_HW_REG  regISR;     ///< 0x94
    GFX_HW_REG  regID1;     ///< 0x98
    GFX_HW_REG  regID2;     ///< 0x9C
    GFX_HW_REG  regID3;     ///< 0xA0
    GFX_HW_REG  regIDM1;    ///< 0xA4
    GFX_HW_REG  regIDM2;    ///< 0xA8
    GFX_HW_REG  regIDM3;    ///< 0xAC
}GFX_HW_REGS;

typedef struct _GFX_HW_DEVICE
{
    GFX_HW_REGS     regs;
}GFX_HW_DEVICE;

//=============================================================================
//                              Global Data Definition
//=============================================================================

//=============================================================================
//                              Public Function Declaration
//=============================================================================
/**
 * This routine is used to initialize hardware layer.
 *
 * @return a valid pointer of hardware device register.
 */
GFX_HW_DEVICE*
gfxHwInitialize();

/**
 * This routine is used to terminate hardware layer.
 *
 * @param dev    The valid pointer of hardware device register.
 *
 * @return void.
 */
void
gfxHwTerminate(
    GFX_HW_DEVICE* dev);

/**
 * This routine is used to fire hardware engine.
 *
 * @param dev    The valid pointer of hardware device register.
 *
 * @return a bool value true if succeed, otherwise return false.
 */
bool
gfxHwEngineFire(
    GFX_HW_DEVICE* dev);

#ifdef CFG_M2D_HUD_ENABLE
bool
gfxHwEngineFireHUD(
    GFX_HW_DEVICE* dev);
#endif
/**
 * This routine is used to reset hardware engine.
 *
 * @param dev    The valid pointer of hardware device register.
 *
 * @return void.
 */
void
gfxHwReset(
    GFX_HW_DEVICE* dev);

/**
 * This routine is used to initialize hardware CRC.
 *
 * @return a bool value true if succeed, otherwise return false.
 */
bool
gfxHwCRCInitialize();

/**
 * This routine is used to get CRC value.
 *
 * @param rdcrc    The valid pointer of read CRC register value.
 * @param wrcrc    The valid pointer of write CRC register value.
 *
 * @return a bool value true if succeed, otherwise return false.
 */
bool
gfxHWCRCGetValue(
    uint32_t* rdcrc,
    uint32_t* wrcrc,
    uint32_t* alucrc);

/**
 * This routine is used to register hardware blit id interrupt callback functioin.
 *
 * @param id   related obj id.
 * @param pfCallback   corresponded callback function pointer.
 *
 * @return void.
 */
void
gfxHwRegisterBlitIdCallback(int id, gfxBlitIdCallback pfCallback);

bool
gfxSurfaceTransformGetVersion(
    GFXSurfaceDst *dstSurface,
    GFXSurfaceSrc *srcSurface);

#ifdef __cplusplus
}
#endif

#endif  // __GFX_HW_H__
