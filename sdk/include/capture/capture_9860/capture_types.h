#ifndef CAPTURE_TYPES_H
#define CAPTURE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ite/ith.h"
#include "ite/itp.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//=============================================================================
//                              Constant Definition
//=============================================================================

#define CAP_SCALE_TAP 4
#define CAP_SCALE_TAP_SIZE 8U

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Type Definition
//=============================================================================

typedef enum CAP_INPUT_VIDEO_FORMAT_TAG {
  PROGRESSIVE = 0,
  INTERLEAVING = 1,
} CAP_INPUT_VIDEO_FORMAT;

typedef enum CAP_INPUT_COLORDEPTH_TAG {
  COLOR_DEPTH_8_BITS = 0,
  COLOR_DEPTH_10_BITS = 1,
  COLOR_DEPTH_12_BITS = 2
} CAP_INPUT_COLORDEPTH;

typedef enum CAP_INPUT_YUV_DATA_FORMAT_TAG {
  YUV422 = 0,
  YUV444 = 1,
  RGB888 = 2,
} CAP_INPUT_YUV_DATA_FORMAT;

typedef enum CAP_INPUT_VIDEO_SYNC_MODE_TAG {
  BT_601 = 0,
  BT_656 = 1
} CAP_INPUT_VIDEO_SYNC_MODE;

typedef enum CAP_ISP_HANDSHAKING_MODE_TAG {
  MEMORY_MODE,
  ONFLY_MODE,
  MEMORY_WITH_ONFLY_MODE
} CAP_ISP_HANDSHAKING_MODE;

typedef enum CAP_MEM_BUF_TAG {
  CAP_MEM_Y0 = 0,
  CAP_MEM_UV0 = 1,
  CAP_MEM_Y1 = 2,
  CAP_MEM_UV1 = 3,
  CAP_MEM_Y2 = 4,
  CAP_MEM_UV2 = 5,
  CAP_MEM_Y3 = 6,
  CAP_MEM_UV3 = 7,
  CAP_MEM_Y4 = 8,
  CAP_MEM_UV4 = 9,
} CAP_MEM_BUF;

typedef enum CAP_LANE_STATUS_TAG {
  CAP_LANE0_STATUS,
} CAP_LANE_STATUS;

typedef enum CAP_INPUT_NV12FORMAT_TAG { UV = 0, VU = 1 } CAP_INPUT_NV12FORMAT;

typedef enum CAP_INPUT_DITHER_MODE_TAG {
  DITHER_OL = 0,
  DITHER_1L = 1,
  DITHER_2L = 2,
  DITHER_3L = 3
} CAP_INPUT_DITHER_MODE;

typedef enum CAP_INPUT_DATA_WIDTH_TAG {
  PIN_24_30_36BITS = 0,
  PIN_16_20_24BITS = 1,
  PIN_8_10_12BITS = 2
} CAP_INPUT_DATA_WIDTH;

typedef enum CAP_OUTPUT_MEMORY_FOTMAT_TAG {
  SEMI_PLANAR_420 = 0,
  SEMI_PLANAR_422 = 1,
  PACKET_MODE_422 = 2
} CAP_OUTPUT_MEMORY_FOTMAT;

typedef enum CAP_COLOR_MATRIX_TAG {
  RGBTOYUV_ITU601_0_255 = 0,
  RGBTOYUV_ITU601_16_235 = 1,
  BYPASS_MODE = 2,
} CAP_COLOR_MATRIX;

//=============================================================================
//                              Structure Definition
//=============================================================================
typedef struct CAP_INPUT_DITHER_INFO_TAG {
  bool b_endither;
  CAP_INPUT_DITHER_MODE dithermode;
} CAP_INPUT_DITHER_INFO;

/* Input Format Info */
typedef struct CAP_INPUT_INFO_TAG {
  uint32_t vsync_pol;
  uint32_t hsync_pol;
  uint32_t vsync_skip;
  uint32_t hsync_skip;
  uint32_t splvsync_byhsync;
  /* begin capture after HSYNC stable */
  bool b_checkhs;
  /* begin capture after VSYNC stable */
  bool b_checkvs;
  /* begin capture after DE stable */
  bool b_checkde;
  /* Active Region  Info */
  uint32_t act_width;
  uint32_t act_height;
  /* 0: Progressive , 1 :interleaving */
  CAP_INPUT_VIDEO_FORMAT Interleave;

  /*For no Data enable Use */
  /* Input HSync active area start numner [12:0] */
  uint32_t hnum1;
  /* Input active area start line number[11:0] of top field */
  uint32_t linenum1;
  /* Input active area line number[11:0] of top field */
  uint32_t linenum2;
  /* Input active area start line number[11:0] of bottom field */
  uint32_t linenum3;
  /* Input active area line number[11:0] of bottom field */
  uint32_t linenum4;
  /* ROI :region of interest Info */
  uint32_t roi_x;
  uint32_t roi_y;
  uint32_t roi_width;
  uint32_t roi_height;
} CAP_INPUT_INFO;

typedef struct CAP_ENFUN_INFO_TAG {
  bool EnDEMode;
  bool EnCSFun;
  bool EnInBT656;

  bool EnHSync;
  bool EnAutoDetHSPol;
  bool EnAutoDetVSPol;
  bool EnDumpMode;
  bool EnMemContinousDump;
  bool EnSramNap;
  bool EnMemLimit;

  bool EnProgressiveToField;
  bool EnCrossLineDE;
  bool EnYPbPrTopVSMode;
  bool EnDlyVS;
  bool EnHSPosEdge;
  bool EnPort1UV2LineDS;
} CAP_ENFUN_INFO;

typedef struct CAP_YUV_INFO_TAG {
  /* 0:YUYV 1:YVYU 2: UYVY 3 :VYUY */
  uint8_t color_order;
  /* 0: 8-bit 1: 10 -bit 2 :12-bit */
  CAP_INPUT_COLORDEPTH color_depth;
  CAP_INPUT_DATA_WIDTH data_width;
  /* ColorFormat 0:YUV422 1:YUV444 2:RGB888 */
  CAP_INPUT_YUV_DATA_FORMAT data_format;
} CAP_YUV_INFO;

/* Pin I/O  Related Define */
typedef struct CAP_INPUT_MUX_INFO_TAG {
  uint8_t y_pin_num[12];
  uint8_t u_pin_num[12];
  uint8_t v_pin_num[12];

  /* enable capture clock */
  bool b_en_uclk;
  /* reserved */
  uint8_t uclk_ratio;
  /* use uclk to sample input video signal with delay. */
  uint8_t uclk_dly;
  /* set 1 to sample input video signal with inverted uclk */
  bool b_uclk_inv;
  /* uclk source :  1:external IO 2: internal colorbar 3: internal LCD */
  uint8_t uclk_src;
  /* VD number of input clock */
  uint8_t uclk_pin_num;
  /* 0: bi-direction 1: uni-direction*/
  bool b_autodly_dir;
  /* enable auto delay for clock */
  bool b_autodly_en;
  /* VD number of HSYNC */
  uint8_t hs_pin_num;
  /* VD number of VSYNC */
  uint8_t vs_pin_num;
  /* VD number of DE */
  uint8_t de_pin_num;
} CAP_INPUT_MUX_INFO;

typedef struct CAP_IO_MODE_INFO_TAG {
  uint32_t io_ffen_vd_00_31;
  uint32_t io_ffen_vd_35_32;
} CAP_IO_MODE_INFO;

typedef struct CAP_OUTPUT_INFO_TAG {
  /* scaled width */
  uint32_t width;
  /* scaled height  = input height*/
  uint32_t height;
  /* memory mode output pitch */
  uint32_t pitch_y;
  uint32_t pitch_uv;
  uint32_t addr_offset;
  CAP_OUTPUT_MEMORY_FOTMAT mem_format;
  /* yuv420 color format */
  CAP_INPUT_NV12FORMAT nv12format;
  /* threshold to begin mem write range:(0-31) */
  uint32_t wr_threshold;
} CAP_OUTPUT_INFO;

typedef struct CAP_COLOR_BAR_CONFIG_TAG {
  bool b_en_colorbar;
  bool b_vsync_pol;
  bool b_hsync_pol;
  uint32_t v_act_start_line;
  uint32_t v_act_line;
  uint32_t act_line;
  uint32_t blank_line1;
  uint32_t blank_line2;
  uint32_t hs_act;
  uint32_t blank_pix1;
  uint32_t blank_pix2;
  uint32_t act_pix;
} CAP_COLOR_BAR_CONFIG;

/* Transfer Matrix RGB to YUV */
typedef struct CAP_RGB_TO_YUV_TAG {
  uint32_t m11;
  uint32_t m12;
  uint32_t m13;
  uint32_t m21;
  uint32_t m22;
  uint32_t m23;
  uint32_t m31;
  uint32_t m32;
  uint32_t m33;
  uint32_t const_y;
  uint32_t const_u;
  uint32_t const_v;
} CAP_RGB_TO_YUV;

/* Capture Scale Function */
typedef struct CAP_SCALE_CTRL_TAG {
  float HCI;
  float WeightMatX[CAP_SCALE_TAP_SIZE][CAP_SCALE_TAP];
} CAP_SCALE_CTRL;

typedef struct CAP_CONTEXT_TAG {

  /* OnflyMode option 1: onfly mode 0: memory mode */
  bool b_en_onfly_mode;

  /* Interrupt option 1: on, 0: off */
  bool b_en_interrupt;

  /* Resolution match flag */
  bool b_match_resolution;

  /* allocate memory buffer information */
  uint8_t *video_sys_addr;
  uint32_t video_vram_addr;
  uint32_t out_addr_y[5];
  uint32_t out_addr_uv[5];

  /* Skip fun setting */
  uint32_t skip_pattern;
  uint32_t skip_period;

  /* Input format Info */
  CAP_INPUT_INFO ininfo;

  /* Input color format Info */
  CAP_YUV_INFO color_info;

  /* Input data format Info */
  CAP_INPUT_DITHER_INFO dither_info;

  /* I/O pin Info */
  CAP_INPUT_MUX_INFO inmux_info;

  /* I/O FIFO mode */
  CAP_IO_MODE_INFO iomode_info;

  /* Output info */
  CAP_OUTPUT_INFO outinfo;

  /* Enable function */
  CAP_ENFUN_INFO funen;

  /* Scale Fun. */
  CAP_SCALE_CTRL scale_ctrl;

  /* RGB to YUV Fun. */
  CAP_RGB_TO_YUV rgb_to_yuv;

  /* RGB to YUV Matrix */
  CAP_COLOR_MATRIX color_matrix_type;

} CAP_CONTEXT;

#ifdef __cplusplus
}
#endif

#endif
