#include "capture.h"
#include "capture_config.h"
#include "capture_hw.h"
#include "capture_reg.h"
#include "capture_util.h"
#include "math.h"
#include <string.h>


//=============================================================================
//                Constant Definition
//=============================================================================
#define CAP_DATA_BUS_WIDTH 12U
#define CAP_VDPIN_DEFAULT 100U
#define CAP_VDPIN_BASE 38U

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================
/*
Input   Output  Ratio(In/Out)
1088    1080    1.007407407
1920    1920    1
1280    1920    0.666666667
576     1080    0.533333333
480     1080    0.444444444
720     1920    0.375
640     1920    0.333333333
288     1080    0.266666667
352     1920    0.183333333
*/
#define WEIGHT_NUM 9U

static float CapScaleRatio[WEIGHT_NUM] = {
    (1088.0f / 1080), (1920.0f / 1920), (1280.0f / 1920),
    (576.0f / 1080),  (480.0f / 1080),  (720.0f / 1920),
    (640.0f / 1920),  (288.0f / 1080),  (352.0f / 1920)};

static uint32_t CapWeightMatInt[WEIGHT_NUM][CAP_SCALE_TAP_SIZE][CAP_SCALE_TAP];

//=============================================================================
//                Private Function Definition
//=============================================================================

static bool cap_uint32tobool_(uint32_t src) {
  if (src == 0x1U) {
    return true;
  } else {
    return false;
  }
}

static void cap_set_gpio_in_(uint32_t pins) {
  ithGpioSetIn(pins);

  if (pins < 64U) {
    ithGpioSetFFIEn(pins);
  }

  ithGpioSetMode(pins, ITH_GPIO_MODE2);

#if 0
    printf("GPIO_IN=%d",pins);
#endif
}
//=============================================================================
/**
 * Calculate Scale Factor
 */
//=============================================================================
static float cap_scale_factor_(uint32_t Input, uint32_t Output) {
  return (float)(((16384.0f * (float)Input) / (float)Output) / 16384.0f);
}

//=============================================================================
/**
 * Select Color Matrix
 */
//=============================================================================
static void cap_color_matrix_(CAP_CONTEXT *p_c_info) {
  if (p_c_info->color_matrix_type == RGBTOYUV_ITU601_16_235) {
    /* (Studio RGB) (16-235) to YUV SDTV ITU601 (16-235)*/
    p_c_info->rgb_to_yuv.m11 = 0x004DUL;
    p_c_info->rgb_to_yuv.m12 = 0x0096UL;
    p_c_info->rgb_to_yuv.m13 = 0x001DUL;
    p_c_info->rgb_to_yuv.m21 = 0x07D4UL;
    p_c_info->rgb_to_yuv.m22 = 0x07A9UL;
    p_c_info->rgb_to_yuv.m23 = 0x0083UL;
    p_c_info->rgb_to_yuv.m31 = 0x0083UL;
    p_c_info->rgb_to_yuv.m32 = 0x0792UL;
    p_c_info->rgb_to_yuv.m33 = 0x07EBUL;
    p_c_info->rgb_to_yuv.const_y = 0x0000UL;
    p_c_info->rgb_to_yuv.const_u = 0x0080UL;
    p_c_info->rgb_to_yuv.const_v = 0x0080UL;
  } else if (p_c_info->color_matrix_type == RGBTOYUV_ITU601_0_255) {
    /* (Computer R'G'B') (0-255) to YUV SDTV ITU601 (16-235)*/
    p_c_info->rgb_to_yuv.m11 = 0x0042UL;
    p_c_info->rgb_to_yuv.m12 = 0x0081UL;
    p_c_info->rgb_to_yuv.m13 = 0x0019UL;
    p_c_info->rgb_to_yuv.m21 = 0x07DAUL;
    p_c_info->rgb_to_yuv.m22 = 0x07B6UL;
    p_c_info->rgb_to_yuv.m23 = 0x0070UL;
    p_c_info->rgb_to_yuv.m31 = 0x0070UL;
    p_c_info->rgb_to_yuv.m32 = 0x07A2UL;
    p_c_info->rgb_to_yuv.m33 = 0x07EEUL;
    p_c_info->rgb_to_yuv.const_y = 0x0010UL;
    p_c_info->rgb_to_yuv.const_u = 0x0080UL;
    p_c_info->rgb_to_yuv.const_v = 0x0080UL;
  } else {
    p_c_info->rgb_to_yuv.m11 = 0x0100UL;
    p_c_info->rgb_to_yuv.m12 = 0x0000UL;
    p_c_info->rgb_to_yuv.m13 = 0x0000UL;
    p_c_info->rgb_to_yuv.m21 = 0x0000UL;
    p_c_info->rgb_to_yuv.m22 = 0x0100UL;
    p_c_info->rgb_to_yuv.m23 = 0x0000UL;
    p_c_info->rgb_to_yuv.m31 = 0x0000UL;
    p_c_info->rgb_to_yuv.m32 = 0x0000UL;
    p_c_info->rgb_to_yuv.m33 = 0x0100UL;
    p_c_info->rgb_to_yuv.const_y = 0x0000UL;
    p_c_info->rgb_to_yuv.const_u = 0x0000UL;
    p_c_info->rgb_to_yuv.const_v = 0x0000UL;
  }
}

//=============================================================================
/**
 * Create weighting for the matrix of scaling.
 */
//=============================================================================
static void cap_create_weight_(float scale, int32_t taps, uint8_t tapSize,
                               uint8_t adjust, uint8_t method,
                               float weightMatrix[][CAP_SCALE_TAP]) {
  uint8_t i = 0U;
  int32_t j = 0;
  float WW = 0.0f;
  float W[32] = {0.0f};
  int32_t point = 0;
  float precision = 64.0f;
  float fscale = 1.0f;
  float ftemp = 0.0f;

  if (adjust == 0U) {
    if (scale < 1.0f) {
      scale = fscale;
    } else {
      scale *= fscale;
    }
  } else if (adjust == 1U) {
    // Last update (2002/04/24) by WKLIN]
    //  For Low Pass
    if (scale < 1.0f) {
      scale = 1.2f;
    } else if (scale > 1.0f) {
      scale *= 1.1f;
    } else {
      (void)printf("Warning: scale is already equal to 1.0\n");
    }
  } else if (adjust == 2U) {
    // Last update (2003/08/17) by WKLIN in Taipei
    // For including more high frequency details
    if (scale < 1.0f) {
      scale = 0.9f;
    } else if (scale > 1.0f) {
      scale *= 0.9f;
    } else {
      (void)printf("Warning: scale is already equal to 1.0\n");
    }
  } else if (adjust == 3U) {
    // Last update (2003/09/10) by WKLIN in Hsin-Chu
    // For excluding more high frequency details
    if (scale < 1.0f) {
      scale = 1.2f;
    } else {
      /*scale >= 1.0f*/
      scale *= 1.3f;
    }
  } else {
    (void)printf(" %s() unknow error !\n", __FUNCTION__);
  }

  if (method == 10U) {
    // Sinc
    for (i = 0; i <= (tapSize >> 1); i++) {
      WW = 0.0f;
      point = (taps / 2) - 1;

      for (j = 0; j < taps; j++) {
        W[j] = cap_sinc_(((float)point + ((float)i / ((float)tapSize))) /
                       (float)scale);
        point--;
        WW += W[j];
      }
#if 0
            for (j=0; j< taps; j++)
              weight_matrix[i][j] = ((int)(W[j]/WW*precision + 0.5 ))/precision;
#endif
      // Changed: 2024/04/23
      weightMatrix[i][taps - 1] = 1.0f;
      for (j = 0; j < (taps - 1); j++) {
        ftemp = truncf(((W[j] / WW) * precision) + 0.5f);
        weightMatrix[i][j] = ftemp / precision;
        weightMatrix[i][taps - 1] -= weightMatrix[i][j];
      }
    }

    for (i = ((tapSize >> 1) + 1U); i < tapSize; i++) {
      for (j = 0; j < taps; j++) {
        weightMatrix[i][j] = weightMatrix[tapSize - i][taps - 1 - j];
      }
    }
  } else if (method == 11U) {
    // rcos
    for (i = 0; i <= (tapSize >> 1); i++) {
      WW = 0.0f;
      point = (taps / 2) - 1;

      for (j = 0; j < taps; j++) {
        W[j] = cap_rcos_(((float)point + ((float)i / ((float)tapSize))) /
                       (float)scale);
        point--;
        WW += W[j];
      }
#if 0
            for (j=0; j< taps; j++)
              weightMatrix[i][j] = ((int)(W[j]/WW*precision + 0.5 ))/precision;
#endif
      // Changed: 2024/04/23
      weightMatrix[i][(taps - 1)] = 1.0f;
      for (j = 0; j < (taps - 1); j++) {
        ftemp = truncf(((W[j] / WW) * precision) + 0.5f);
        weightMatrix[i][j] = ftemp / precision;
        weightMatrix[i][(taps - 1)] -= weightMatrix[i][j];
#if 0
                printf("ftemp = %f, W[%d][%d] = %f\n", ftemp, i, j , weightMatrix[i][j]);
#endif
      }
    }
    for (i = ((tapSize >> 1) + 1U); i < tapSize; i++) {
      for (j = 0; j < taps; j++) {
        weightMatrix[i][j] = weightMatrix[tapSize - i][(taps - 1 - j)];
      }
    }
  } else if (method == 12U) {
    // Catmull-Rom Cubic interpolation
    for (i = 0; i <= (tapSize >> 1); i++) {
      WW = 0.0f;
      point = (taps / 2) - 1;
      for (j = 0; j < taps; j++) {
        W[j] = cap_cubic01_(((float)point + ((float)i / ((float)tapSize))) /
                          (float)scale);
        point--;
        WW += W[j];
      }

      // Changed: 2024/04/23
      weightMatrix[i][(taps - 1)] = 1.0f;
      for (j = 0; j < (taps - 1); j++) {
        ftemp = truncf(((W[j] / WW) * precision) + 0.5f);
        weightMatrix[i][j] = ftemp / precision;
        weightMatrix[i][(taps - 1)] -= weightMatrix[i][j];
      }
    }
    for (i = ((tapSize >> 1) + 1U); i < tapSize; i++) {
      for (j = 0; j < taps; j++) {
        weightMatrix[i][j] = weightMatrix[tapSize - i][(taps - 1 - j)];
      }
    }
  } else {
    (void)printf(" %s() unknow error !\n", __FUNCTION__);
  }
}

//=============================================================================
//                Public Function Definition
//=============================================================================
//=============================================================================
/**
 * CAP memory Initialize.
 */
//=============================================================================

//   CAP_MEM_BUF_PITCH
//     ----------
// 720 |   Y0     |
//     ----------
// 360 |   UV0   |
//     ----------
// 720 |   Y1     |
//     ----------
// 360 |   UV1   |
//     ----------
// 720 |   Y2     |
//     ----------
// 360 |   UV2   |
//     ----------

int32_t cap_memory_init_(CAPTURE_HANDLE *ptDev, CAPTURE_SETTING info) {
  int32_t result = 0;
  CAP_CONTEXT *p_c_info = (CAP_CONTEXT *)ptDev;
  uint32_t membuffer[CAPTURE_MEM_BUF_COUNT] = {0};
  uint32_t i;
  uint32_t offset = 0;
  uint32_t y_buffer_base = 0;
  uint32_t uv_buffer_base = info.max_height;

  uint32_t size = ((info.max_height >> 1) * info.max_width) *
                  (CAPTURE_MEM_BUF_COUNT + (CAPTURE_MEM_BUF_COUNT / 2U));

  if (p_c_info->video_sys_addr != NULL) {
    (void)printf("memory already exists %s (%d) error\n", __FUNCTION__,
                 __LINE__);
    result = -1;
    goto end;
  }

  p_c_info->video_vram_addr =
      (uint32_t)itpVmemAlignedAlloc(64, size); // 64 bit aligen

  p_c_info->video_sys_addr =
      (uint8_t *)ithMapVram(p_c_info->video_vram_addr, size, ITH_VRAM_WRITE);

  if (p_c_info->video_sys_addr == NULL) {
    (void)printf("allocate fail %s (%d) error\n", __FUNCTION__, __LINE__);
    result = -1;
    goto end;
  }

  membuffer[0] = p_c_info->video_vram_addr;

  for (i = 0; i < CAPTURE_MEM_BUF_COUNT; i++) {

    if ((i % 2U) == 0U) {
      offset = y_buffer_base * info.max_width;
      y_buffer_base += (info.max_height + (info.max_height / 2U));
    } else {
      offset = uv_buffer_base * info.max_width;
      uv_buffer_base += (info.max_height + (info.max_height / 2U));
    }

    membuffer[i] = offset + membuffer[0];

    if ((i % 2U) == 0U) {
      if (i == 0U) {
        p_c_info->out_addr_y[i] = membuffer[i];
      } else {
        p_c_info->out_addr_y[i / 2U] = membuffer[i];
      }
    } else {
      p_c_info->out_addr_uv[((i + 1U) / 2U) - 1U] = membuffer[i];
    }

    (void)printf("membuffer[%d] addr = 0x%x\n", i, membuffer[i]);
  }

end:
  if (result != 0) {
    (void)printf("%s (%d) error\n", __FUNCTION__, __LINE__);
  }
  return (int32_t)result;
}
//=============================================================================
/**
 * CAP memory clear.
 */
//=============================================================================
int32_t cap_memory_deinit_(CAPTURE_HANDLE *ptDev) {
  int32_t result = 0;
  CAP_CONTEXT *p_c_info = &ptDev->cap_info;

  if (p_c_info->video_sys_addr != NULL) {
    itpVmemFree(p_c_info->video_vram_addr);
    p_c_info->video_sys_addr = NULL;
    p_c_info->video_vram_addr = 0U;

  } else {
    result = -1;
    goto end;
  }

end:
  if (result != 0) {
    (void)printf("%s (%d) error\n", __FUNCTION__, __LINE__);
  }
  return (int32_t)result;
}

//=============================================================================
/**
 * Update CAP device.
 */
//=============================================================================
int32_t cap_update_reg_(CAPTURE_HANDLE *ptDev) {
  int32_t result = 0;
  uint32_t index;
  bool b_weightfound = false;

  CAP_CONTEXT *p_c_info = &ptDev->cap_info;

  if (p_c_info == NULL) {
    result = -1;
    goto end;
  }

#if 0
    // Update Onfly or Memeory mode
    if (p_c_info->b_en_onfly_mode == true && p_c_info->b_en_interrupt == true)
    {
        ithCap_Set_ISP_HandShaking(ptDev->cap_id,MEMORY_WITH_ONFLY_MODE, &p_c_info->outinfo);
        ithCap_Set_Interrupt_Mode(ptDev->cap_id,(CAP_INT_MODE_FRAME_END | CAP_INT_MODE_SYNC_ERR), true);
    }
#endif
  if (p_c_info->b_en_onfly_mode == true) {
    if (ithCap_Set_ISP_HandShaking(ptDev->cap_id, ONFLY_MODE,
                                   &p_c_info->outinfo) != 0) {
      result = -1;
      goto end;
    }

    if (p_c_info->b_en_interrupt == true) {
      ithCap_Set_Interrupt_Mode(ptDev->cap_id, CAP_INT_MODE_SYNC_ERR, true);
    }

  } else {

    if (ithCap_Set_ISP_HandShaking(ptDev->cap_id, MEMORY_MODE,
                                   &p_c_info->outinfo) != 0) {
      result = -1;
      goto end;
    }

    if (p_c_info->b_en_interrupt == true) {
      ithCap_Set_Interrupt_Mode(
          ptDev->cap_id, (CAP_INT_MODE_FRAME_END | CAP_INT_MODE_SYNC_ERR),
          true);
    }
  }

  // Update Dither Setting
  if (p_c_info->dither_info.b_endither == true) {
    ithCap_Set_Enable_Dither(ptDev->cap_id, &p_c_info->dither_info);
  }

  cap_color_matrix_(p_c_info); // VideoMode

  // Update Capture Write Buffer Address
  ithCap_Set_Buffer_addr_Reg(ptDev->cap_id, p_c_info->out_addr_y,
                             p_c_info->out_addr_uv,
                             p_c_info->outinfo.addr_offset);

  // Update Input Pin mux
  if (ithCap_Set_Input_Pin_Mux_Reg(ptDev->cap_id, &p_c_info->inmux_info) != 0) {
    result = -1;
    goto end;
  }

  // Update IO Parameter
  if (ithCap_Set_IO_Mode_Reg(ptDev->cap_id, &p_c_info->iomode_info) != 0) {
    result = -1;
    goto end;
  }

  // Update internal Parameter
  ithCap_Set_Internal_Reg(ptDev->cap_id, &p_c_info->ininfo);

  // Non DE mode
  ithCap_Set_NonDEMode_Reg(ptDev->cap_id, &p_c_info->ininfo);

  ithCap_Set_Enable_Reg(ptDev->cap_id, &p_c_info->funen);

  // Update RGB to YUV Matrix
  if (p_c_info->funen.EnCSFun) {
    ithCap_Set_RGBtoYUVMatrix_Reg(ptDev->cap_id, &p_c_info->rgb_to_yuv);
  }
  // Update Color Format
  ithCap_Set_Color_Format_Reg(ptDev->cap_id, &p_c_info->color_info);

  // Update Scale Matrix
  p_c_info->scale_ctrl.HCI =
      cap_scale_factor_(p_c_info->ininfo.roi_width, p_c_info->outinfo.width);

  ithCap_Set_ScaleParam_Reg(ptDev->cap_id, &p_c_info->scale_ctrl);

  for (index = 0; index < WEIGHT_NUM; index++) {
    if (p_c_info->scale_ctrl.HCI >= CapScaleRatio[index]) {
      ithCap_Set_IntScaleMatrixH_Reg(ptDev->cap_id, CapWeightMatInt[index]);
      b_weightfound = true;
      break;
    }
  }

  if (b_weightfound == false) {
    ithCap_Set_IntScaleMatrixH_Reg(ptDev->cap_id,
                                   CapWeightMatInt[WEIGHT_NUM - 1U]);
  }
  // Update Skip Pattern
  ithCap_Set_Skip_Pattern_Reg(ptDev->cap_id, p_c_info->skip_pattern,
                              p_c_info->skip_period);

end:
  if (result != 0) {
    (void)printf("cap_update_reg_ error\n");
  }
  return (int32_t)result;
}

//=============================================================================
/**
 * CAP default value initialization.
 */
//=============================================================================
int32_t cap_init_(CAP_CONTEXT *p_c_info) {
  int32_t result = 0;
  int32_t i;
  uint32_t j, index;
  float weight_matrix[CAP_SCALE_TAP_SIZE][CAP_SCALE_TAP];

  if (p_c_info == NULL) {
    result = -1;
    goto end;
  }

  // default value
  p_c_info->color_info.color_depth = COLOR_DEPTH_8_BITS;

  p_c_info->ininfo.vsync_pol = 0x0UL;
  p_c_info->ininfo.hsync_pol = 0x0UL;
  p_c_info->ininfo.vsync_skip = 0x0UL;
  p_c_info->ininfo.hsync_skip = 0x2UL;
  p_c_info->ininfo.splvsync_byhsync = 0x0UL;
  p_c_info->ininfo.b_checkhs = false;
  p_c_info->ininfo.b_checkvs = false;
  p_c_info->ininfo.b_checkde = false;
  p_c_info->ininfo.hnum1 = 0UL;
  p_c_info->ininfo.linenum1 = 0UL;
  p_c_info->ininfo.linenum2 = 0UL;
  p_c_info->ininfo.linenum3 = 0UL;
  p_c_info->ininfo.linenum4 = 0UL;

  p_c_info->outinfo.wr_threshold = 0x10UL;
  p_c_info->outinfo.mem_format = SEMI_PLANAR_420;
  p_c_info->outinfo.nv12format = UV;
  p_c_info->outinfo.addr_offset = 0UL;

  p_c_info->funen.EnDEMode = false;
  p_c_info->funen.EnCSFun = false;
  p_c_info->funen.EnInBT656 = false;
  p_c_info->funen.EnHSync = false;
  p_c_info->funen.EnPort1UV2LineDS = true; // ask H.C.
  p_c_info->funen.EnAutoDetHSPol = false;
  p_c_info->funen.EnAutoDetVSPol = false;
  p_c_info->funen.EnDumpMode = false;
  p_c_info->funen.EnMemContinousDump = false;
  p_c_info->funen.EnSramNap = false;
  p_c_info->funen.EnMemLimit = false;

  p_c_info->color_matrix_type = BYPASS_MODE;

  /* Scale */
  p_c_info->scale_ctrl.HCI = 0.0f;

  /*Initial Scale Weight Matrix*/
  for (index = 0; index < WEIGHT_NUM; index++) {
    cap_create_weight_(CapScaleRatio[index], CAP_SCALE_TAP, CAP_SCALE_TAP_SIZE,
                       0U, 11U, weight_matrix);

    for (j = 0; j < CAP_SCALE_TAP_SIZE; j++) {
      for (i = 0; i < CAP_SCALE_TAP; i++) {
        CapWeightMatInt[index][j][i] =
            cap_floattofix_(weight_matrix[j][i], 1U, 6U);
      }
    }
  }

end:
  if (result != 0) {
    (void)printf("%s (%d) Capture Initialize Fail\n", __FUNCTION__, __LINE__);
  }
  return result;
}
void cap_conifg_(CAPTURE_HANDLE *ptDev, uint32_t *Cptr) {
  int32_t result = 0;
  CAP_CONTEXT *p_c_info = &ptDev->cap_info;
  uint32_t i = 0;
  uint8_t y_16pin[CAP_DATA_BUS_WIDTH];
  uint8_t u_16pin[CAP_DATA_BUS_WIDTH];
  uint8_t v_16pin[CAP_DATA_BUS_WIDTH];

  if (Cptr == NULL) {
    result = -1;
    goto end;
  }

  for (i = 0; i < CAP_DATA_BUS_WIDTH; i++) {
    if (Cptr[i] != CAP_VDPIN_DEFAULT) {
      y_16pin[i] = (uint8_t)Cptr[i];
      cap_set_gpio_in_(Cptr[i] + CAP_VDPIN_BASE);
    } else {
      y_16pin[i] = (uint8_t)0U;
    }
  }

  for (i = 0; i < CAP_DATA_BUS_WIDTH; i++) {
    if (Cptr[i + CAP_DATA_BUS_WIDTH] != CAP_VDPIN_DEFAULT) {
      u_16pin[i] = (uint8_t)Cptr[i + CAP_DATA_BUS_WIDTH];
      cap_set_gpio_in_(Cptr[i + CAP_DATA_BUS_WIDTH] + CAP_VDPIN_BASE);
    } else {
      u_16pin[i] = (uint8_t)0U;
    }
  }

  for (i = 0; i < CAP_DATA_BUS_WIDTH; i++) {
    if (Cptr[i + (CAP_DATA_BUS_WIDTH * 2U)] != CAP_VDPIN_DEFAULT) {
      v_16pin[i] = (uint8_t)Cptr[i + (CAP_DATA_BUS_WIDTH * 2U)];
      cap_set_gpio_in_(Cptr[i + (CAP_DATA_BUS_WIDTH * 2U)] + CAP_VDPIN_BASE);
    } else {
      v_16pin[i] = (uint8_t)0U;
    }
  }

  /* Input  pin mux setting */
  (void)memcpy(&p_c_info->inmux_info.y_pin_num, &y_16pin, CAP_DATA_BUS_WIDTH);
  (void)memcpy(&p_c_info->inmux_info.u_pin_num, &u_16pin, CAP_DATA_BUS_WIDTH);
  (void)memcpy(&p_c_info->inmux_info.v_pin_num, &v_16pin, CAP_DATA_BUS_WIDTH);

  for (i = 36U; i < 40U; i++) {
    if (Cptr[i] != CAP_VDPIN_DEFAULT) {
      cap_set_gpio_in_(Cptr[i] + CAP_VDPIN_BASE);
    }
  }
  /*HS Pin */
  if (Cptr[36] != CAP_VDPIN_DEFAULT) {
    p_c_info->inmux_info.hs_pin_num = (uint8_t)Cptr[36];
  } else {
    p_c_info->inmux_info.hs_pin_num = (uint8_t)0U;
  }
  /*VS Pin */
  if (Cptr[37] != CAP_VDPIN_DEFAULT) {
    p_c_info->inmux_info.vs_pin_num = (uint8_t)Cptr[37];
  } else {
    p_c_info->inmux_info.vs_pin_num = (uint8_t)0U;
  }
  /*DE Pin */
  if (Cptr[38] != CAP_VDPIN_DEFAULT) {
    p_c_info->inmux_info.de_pin_num = (uint8_t)Cptr[38];
  } else {
    p_c_info->inmux_info.de_pin_num = (uint8_t)0U;
  }
  /* Clk Pin*/
  if (Cptr[39] != CAP_VDPIN_DEFAULT) {
    p_c_info->inmux_info.uclk_pin_num = (uint8_t)Cptr[39];
  } else {
    p_c_info->inmux_info.uclk_pin_num = (uint8_t)0U;
  }

  p_c_info->inmux_info.uclk_src = (uint8_t)Cptr[40];
  p_c_info->inmux_info.b_uclk_inv = cap_uint32tobool_(Cptr[41]);
  p_c_info->inmux_info.uclk_dly = (uint8_t)Cptr[42];
  p_c_info->inmux_info.uclk_ratio = (uint8_t)Cptr[43];
  p_c_info->inmux_info.b_en_uclk = cap_uint32tobool_(Cptr[44]);
  p_c_info->inmux_info.b_autodly_dir = cap_uint32tobool_(Cptr[62]);
  p_c_info->inmux_info.b_autodly_en = cap_uint32tobool_(Cptr[63]);

  /* I/O Setting */
  p_c_info->iomode_info.io_ffen_vd_00_31 = Cptr[45];
  p_c_info->iomode_info.io_ffen_vd_35_32 = Cptr[46];

  /* Input Data Format Setting */
  switch (Cptr[47]) {
  case 0U:
    p_c_info->color_info.data_format = YUV422;
    break;

  case 1U:
    p_c_info->color_info.data_format = YUV444;
    break;

  case 2U:
    p_c_info->color_info.data_format = RGB888;
    break;

  default:
    result = -1;
    break;
  };

  p_c_info->color_info.color_order = (uint8_t)Cptr[48];

  switch (Cptr[49]) {
  case 0U:
    p_c_info->color_info.data_width = PIN_24_30_36BITS;
    break;

  case 1U:
    p_c_info->color_info.data_width = PIN_16_20_24BITS;
    break;

  case 2U:
    p_c_info->color_info.data_width = PIN_8_10_12BITS;
    break;

  default:
    result = -1;
    break;
  };

  p_c_info->funen.EnDEMode = cap_uint32tobool_(Cptr[50]);
  p_c_info->funen.EnInBT656 = cap_uint32tobool_(Cptr[51]);
  p_c_info->funen.EnHSync = cap_uint32tobool_(Cptr[52]);
  p_c_info->funen.EnAutoDetHSPol = cap_uint32tobool_(Cptr[53]);
  p_c_info->funen.EnAutoDetVSPol = cap_uint32tobool_(Cptr[54]);

  /* Input Format default Setting */
  switch (Cptr[55]) {
  case 0U:
    p_c_info->ininfo.Interleave = PROGRESSIVE;
    break;

  case 1U:
    p_c_info->ininfo.Interleave = INTERLEAVING;
    break;

  default:
    result = -1;
    break;
  };

  p_c_info->outinfo.wr_threshold = Cptr[56];
  p_c_info->ininfo.b_checkde = cap_uint32tobool_(Cptr[57]);
  p_c_info->ininfo.b_checkhs = cap_uint32tobool_(Cptr[58]);
  /*if connect to sensor, suggest to turn off. just turn on CheckHS to induce
   * frame check time. */
  p_c_info->ininfo.b_checkvs = cap_uint32tobool_(Cptr[59]);

  /* Skippattern init */
  p_c_info->skip_pattern = Cptr[60];
  p_c_info->skip_period = Cptr[61];

  /* Video source from external IO */
  ithCap_Set_VideoSignalSource(ptDev->cap_id, 0x1);

end:
  if (result != 0) {
    (void)printf("%s (%d) Setting Fail, NULL pointer\n", __FUNCTION__,
                 __LINE__);
  }
}
