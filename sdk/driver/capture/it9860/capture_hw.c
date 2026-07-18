#include "capture_hw.h"
#include "capture_config.h"
#include "capture_reg.h"
#include "capture_util.h"

//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================
static __inline uint32_t CapAddr_(CAPTURE_DEV_ID id, uint32_t offset) {
  if ((uint32_t)id < DEV_ID_MAX) {
    return ((REG_CAP0_BASE | offset) | ((uint32_t)id << 12U));
  } else {
    (void)ithPrintf("[Error]device id error.\n");
  }

  return REG_CAP0_BASE;
}
//=============================================================================
//                Public Function Definition
//=============================================================================
void ithCapEnableClock(void) {
  /* enable clock */
  ithSetRegBitA(ITH_HOST_BASE + ITH_CAP_CLK_REG, ITH_EN_M12CLK_BIT);
  ithSetRegBitA(ITH_HOST_BASE + ITH_CAP_CLK_REG, ITH_EN_W18CLK_BIT);
  ithSetRegBitA(ITH_HOST_BASE + ITH_CAP_CLK_REG, ITH_EN_DIV_CAPCLK_BIT);
}

void ithCapDisableClock(void) {
  /* disable clock */
  ithClearRegBitA(ITH_HOST_BASE + ITH_CAP_CLK_REG, ITH_EN_M12CLK_BIT);
  ithClearRegBitA(ITH_HOST_BASE + ITH_CAP_CLK_REG, ITH_EN_W18CLK_BIT);
  ithClearRegBitA(ITH_HOST_BASE + ITH_CAP_CLK_REG, ITH_EN_DIV_CAPCLK_BIT);
}

void ithCap_Set_Reset(void) {
  ithWriteRegMaskA(ITH_HOST_BASE + ITH_CAP_CLK_REG,
                   (0x1UL << ITH_EN_CAPC_RST_BIT),
                   (0x1UL << ITH_EN_CAPC_RST_BIT));
  (void)usleep(1000);
  ithWriteRegMaskA(ITH_HOST_BASE + ITH_CAP_CLK_REG, 0x0U,
                   (0x1UL << ITH_EN_CAPC_RST_BIT));
}

void ithCap_Set_Register_Reset(void) {
  ithWriteRegMaskA(ITH_HOST_BASE + ITH_CAP_CLK_REG,
                   (0x1UL << ITH_EN_CAP_REG_RST_BIT),
                   (0x1UL << ITH_EN_CAP_REG_RST_BIT));
  (void)usleep(1000);
  ithWriteRegMaskA(ITH_HOST_BASE + ITH_CAP_CLK_REG, 0x0U,
                   (0x1UL << ITH_EN_CAP_REG_RST_BIT));
  (void)usleep(1000);
}

void ithCap_Set_AutoDelayHWFlow(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data = 0;
  /* disable Auto Delay */
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), 0x0U,
                   CAP_MSK_UCLK_AUTO_DLYEN);
  /* Disable uclken */
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), 0x0U,
                   CAP_MSK_UCLKEN);

  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
  data = data & (0x0FUL << 16);
  data = data >> 8;
  /* write Dlystage to UCLKDly */
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), data,
                   CAP_MSK_UCLK_DLY);
  /* Enable uclken */
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), BIT3,
                   CAP_MSK_UCLKEN);
}

void ithCap_Set_Fire(CAPTURE_DEV_ID DEV_ID) {
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER39), (BIT0 | BIT1));
}

void ithCap_Set_UnFire(CAPTURE_DEV_ID DEV_ID) {
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER39), 0x0U);
  /*flush fifo data*/
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER8), 0x0U,
                   CAP_MSK_WR_MERGE_TH);
}

void ithCap_Set_ErrReset(CAPTURE_DEV_ID DEV_ID) {
  /*disable cap*/
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER39), 0x0U,
                   (CAP_MSK_CAP_RESET | CAP_MSK_CAP_ENABLE));

  /*switch to internal colorbar*/
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), BIT1,
                   CAP_MSK_UCLKSRC);

  /*reset*/
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER39), BIT0,
                   CAP_MSK_CAP_RESET);
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER39), 0x0U,
                   CAP_MSK_CAP_RESET);

  /*switch to external IO*/
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), BIT0,
                   CAP_MSK_UCLKSRC);
}

/* Input clk delay enable */
void ithCap_Set_TurnOnClock_Reg(CAPTURE_DEV_ID DEV_ID, bool flag) {

  if (flag == true) {
    ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), BIT3,
                     CAP_MSK_UCLKEN);
  } else {
    ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), 0x0U,
                     CAP_MSK_UCLKEN);
  }
}

/* Input Related Reg Setting */
int32_t ithCap_Set_Input_Pin_Mux_Reg(CAPTURE_DEV_ID DEV_ID,
                                     CAP_INPUT_MUX_INFO *pininfo) {
  int32_t result = 0;
  uint32_t data = 0, mask = 0;

  /* Disable uclken */
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), 0x0U,
                   CAP_MSK_UCLKEN);

  /* Setting Y Pin mux */
  data = (uint32_t)pininfo->y_pin_num[0];
  data |= ((uint32_t)pininfo->y_pin_num[1] << 8U);
  data |= ((uint32_t)pininfo->y_pin_num[2] << 16U);
  data |= ((uint32_t)pininfo->y_pin_num[3] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_Y_DATA_PIN_MUX0), data);

  data = (uint32_t)pininfo->y_pin_num[4];
  data |= ((uint32_t)pininfo->y_pin_num[5] << 8U);
  data |= ((uint32_t)pininfo->y_pin_num[6] << 16U);
  data |= ((uint32_t)pininfo->y_pin_num[7] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_Y_DATA_PIN_MUX1), data);

  data = (uint32_t)pininfo->y_pin_num[8];
  data |= ((uint32_t)pininfo->y_pin_num[9] << 8U);
  data |= ((uint32_t)pininfo->y_pin_num[10] << 16U);
  data |= ((uint32_t)pininfo->y_pin_num[11] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_Y_DATA_PIN_MUX2), data);

  /*Setting U Pin mux*/
  data = (uint32_t)pininfo->u_pin_num[0];
  data |= ((uint32_t)pininfo->u_pin_num[1] << 8U);
  data |= ((uint32_t)pininfo->u_pin_num[2] << 16U);
  data |= ((uint32_t)pininfo->u_pin_num[3] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_U_DATA_PIN_MUX0), data);

  data = (uint32_t)pininfo->u_pin_num[4];
  data |= ((uint32_t)pininfo->u_pin_num[5] << 8U);
  data |= ((uint32_t)pininfo->u_pin_num[6] << 16U);
  data |= ((uint32_t)pininfo->u_pin_num[7] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_U_DATA_PIN_MUX1), data);

  data = (uint32_t)pininfo->u_pin_num[8];
  data |= ((uint32_t)pininfo->u_pin_num[9] << 8U);
  data |= ((uint32_t)pininfo->u_pin_num[10] << 16U);
  data |= ((uint32_t)pininfo->u_pin_num[11] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_U_DATA_PIN_MUX2), data);

  /*Setting V Pin mux*/
  data = (uint32_t)pininfo->v_pin_num[0];
  data |= ((uint32_t)pininfo->v_pin_num[1] << 8U);
  data |= ((uint32_t)pininfo->v_pin_num[2] << 16U);
  data |= ((uint32_t)pininfo->v_pin_num[3] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_V_DATA_PIN_MUX0), data);

  data = (uint32_t)pininfo->v_pin_num[4];
  data |= ((uint32_t)pininfo->v_pin_num[5] << 8U);
  data |= ((uint32_t)pininfo->v_pin_num[6] << 16U);
  data |= ((uint32_t)pininfo->v_pin_num[7] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_V_DATA_PIN_MUX1), data);

  data = (uint32_t)pininfo->v_pin_num[8];
  data |= ((uint32_t)pininfo->v_pin_num[9] << 8U);
  data |= ((uint32_t)pininfo->v_pin_num[10] << 16U);
  data |= ((uint32_t)pininfo->v_pin_num[11] << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_V_DATA_PIN_MUX2), data);

  /*Set HS,VS,DE, Pin mux*/
  data = (uint32_t)pininfo->hs_pin_num;
  data |= ((uint32_t)pininfo->vs_pin_num << 8U);
  data |= ((uint32_t)pininfo->de_pin_num << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER6), data);

  /*Set Input clk mode*/
  data = (uint32_t)(pininfo->uclk_src & 0x07UL) |
         ((uint32_t)(pininfo->uclk_ratio & 0x0FUL) << 4U) |
         ((uint32_t)(pininfo->uclk_dly & 0x0FUL) << 8U) |
         ((uint32_t)(pininfo->uclk_pin_num & 0x3FUL) << 16U) |
         ((uint32_t)(0x1UL) << 29U) | ((uint32_t)(0x0UL) << 30U);

  if (pininfo->b_en_uclk == true) {
    data = (data | BIT3);
  }

  if (pininfo->b_uclk_inv == true) {
    data = (data | BIT12);
  }

  if (pininfo->b_autodly_dir == true) {
    data = (data | BIT28);
  }

  if (pininfo->b_autodly_en == true) {
    data = (data | BIT31);
  }

  mask = (CAP_MSK_UCLKSRC | CAP_MSK_UCLKEN | CAP_MSK_UCLK_RATIO |
          CAP_MSK_UCLK_DLY | CAP_MSK_UCLK_INV | CAP_MSK_UCLK_PINNUM |
          CAP_MSK_UCLK_AUTO_DLY_RULE | CAP_MSK_UCLK_AUTO_DLY_SMOOTH |
          CAP_MSK_AUTO_DLYEN_FREQ | CAP_MSK_UCLK_AUTO_DLYEN);

  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), data, mask);

  return result;
}

void ithCap_Set_Color_Format_Reg(CAPTURE_DEV_ID DEV_ID,
                                 CAP_YUV_INFO *pYUVinfo) {
  uint32_t data;

  switch (pYUVinfo->data_format) {
  /* ColorFormat 0:YUV422 */
  case YUV422:
    data = 0U;
    break;
    /* ColorFormat 1:YUV444 */
  case YUV444:
    data = 1U;
    break;
  /* ColorFormat 2:RGB888 */
  case RGB888:
    data = 2U;
    break;

  default:
    data = 0U;
    break;
  }

  /* 0: 8-bit 1: 10 -bit 2 :12-bit */
  data = (data | (((uint32_t)pYUVinfo->color_order & 0x3UL) << 2U));

  switch (pYUVinfo->color_depth) {
  /* 8-bit */
  case COLOR_DEPTH_8_BITS:
    data = (data | (0x0UL << 4U));
    break;
  /* 10 -bit */
  case COLOR_DEPTH_10_BITS:
    data = (data | (0x1UL << 4U));
    break;
  /* 12-bit */
  case COLOR_DEPTH_12_BITS:
    data = (data | (0x2UL << 4U));
    break;

  default:
    data = (data | (0x0UL << 4U));
    break;
  }

  switch (pYUVinfo->data_width) {
  /* 24_30_36BITS */
  case PIN_24_30_36BITS:
    data = (data | (0x0UL << 6U));
    break;
  /* 16_20_24BITS */
  case PIN_16_20_24BITS:
    data = (data | (0x1UL << 6U));
    break;
  /* 8_10_12BITS */
  case PIN_8_10_12BITS:
    data = (data | (0x2UL << 6U));
    break;

  default:
    data = (data | (0x2UL << 6U));
    break;
  }

  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER2), data);
}

int32_t ithCap_Set_IO_Mode_Reg(CAPTURE_DEV_ID DEV_ID,
                               CAP_IO_MODE_INFO *io_config) {
  int32_t result = 0;
  /*Enable IO sampling Flip-Flop*/
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER4),
               io_config->io_ffen_vd_00_31);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER5),
               (io_config->io_ffen_vd_35_32 & 0xFUL));
  return result;
}

void ithCap_Set_Internal_Reg(CAPTURE_DEV_ID DEV_ID, CAP_INPUT_INFO *pIninfo) {
  uint32_t data = 0, mask = 0;

  /* Set Hsync & Vsync Porlarity */
  data = (pIninfo->hsync_pol & 0x1UL);
  data |= ((pIninfo->vsync_pol & 0x1UL) << 1U);

  /* Set VsyncSkip & HsyncSkip */
  data |= ((pIninfo->vsync_skip & 0x7UL) << 8U);
  data |= ((pIninfo->hsync_skip & 0x3FUL) << 12U);

  /* Set sample Vsync by Hsync */
  data |= ((pIninfo->splvsync_byhsync & 0x1UL) << 27U);

  /* Set CheckHsync &  CheckVsync & CheckDE enable */
  if(pIninfo->b_checkhs == true)
  {
    data |= (0x1UL << 28U);
  }

  if(pIninfo->b_checkvs == true)
  {
    data |= (0x1UL << 29U);
  }

  if(pIninfo->b_checkde == true)
  {
    data |= (0x1UL << 30U);
  }

  mask = (CAP_MSK_PHS | CAP_MSK_PVS | CAP_MSK_VSYNC_SKIP | CAP_MSK_HSYNC_SKIP |
          CAP_MSK_SAMPLE_VS | CAP_MSK_CHECK_HS | CAP_MSK_CHECK_VS |
          CAP_MSK_CHECK_DE);

  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING), data, mask);
}

void ithCap_Set_NonDEMode_Reg(CAPTURE_DEV_ID DEV_ID, CAP_INPUT_INFO *pIninfo) {

  uint32_t data = 0;

  ithWriteRegA(CapAddr_(DEV_ID, CAP_ACTIVE_REGION_SETTING_REGISTER0),
               (pIninfo->hnum1 & 0x1FFFUL));

  data = (pIninfo->linenum1 & 0xFFFUL);
  data |= ((pIninfo->linenum2 & 0xFFFUL) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_ACTIVE_REGION_SETTING_REGISTER1), data);

  data = (pIninfo->linenum3 & 0xFFFUL);
  data |= ((pIninfo->linenum4 & 0xFFFUL) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_ACTIVE_REGION_SETTING_REGISTER2), data);
}

void ithCap_Set_Output_Width_Reg(CAPTURE_DEV_ID DEV_ID,
                                 CAP_OUTPUT_INFO *pOutInfo) {

  /*Set scale Width*/
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_SCALING_SETTING_REGISTER0),
                   (uint32_t)(pOutInfo->width), CAP_MSK_HOR_SCALE_WIDTH);
}

void ithCap_Set_Output_Reg(CAPTURE_DEV_ID DEV_ID, CAP_OUTPUT_INFO *pOutInfo) {
  uint32_t data = 0;

  /* Set output pitch */
  data = (((pOutInfo->pitch_y >> 3U) & N10_BITS_MSK) << 3U);
  data |= (((pOutInfo->pitch_uv >> 3U) & N10_BITS_MSK) << 19U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER21), data);

  /* Set memory write threshould & NV12Format */
  if(pOutInfo->nv12format == UV)
  {
    data = 0x0U;
  }
  else
  {
    data = (N01_BITS_MSK << 5U);
  }

  data |= ((pOutInfo->wr_threshold & N05_BITS_MSK) << 8U);
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER8), data,
                   (CAP_MSK_WR_MERGE_TH | CAP_MSK_NV12_FORMAT));
}

void ithCap_Set_Skip_Pattern_Reg(CAPTURE_DEV_ID DEV_ID, uint32_t pattern,
                                 uint32_t period) {
  /* Skip frame control */
  uint32_t data = 0;
  data = (pattern & N16_BITS_MSK);
  data |= ((period & N04_BITS_MSK) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_FRAMERATE_SETTING_REGISTER), data);
}

void ithCap_Set_Hsync_Polarity(CAPTURE_DEV_ID DEV_ID, uint32_t Hsync) {
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING), Hsync,
                   CAP_MSK_PHS);
}

void ithCap_Set_Vsync_Polarity(CAPTURE_DEV_ID DEV_ID, uint32_t Vsync) {
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING), (Vsync << 1),
                   CAP_MSK_PVS);
}

void ithCap_Set_Interrupt_Mode(CAPTURE_DEV_ID DEV_ID, uint32_t Intr_Mode,
                               bool flag) {
  uint32_t data = 0;

  if (flag == true) {
    if ((Intr_Mode & CAP_INT_MODE_FRAME_END) != 0U) {
      data |= 0x1UL;
    }
    if ((Intr_Mode & CAP_INT_MODE_SYNC_ERR) != 0U) {
      data |= (0x1UL << 1U);
    }
    if ((Intr_Mode & CAP_INT_MODE_DCLK_PHASE_DRIFTED) != 0U) {
      data |= (0x1UL << 2U);
    }
    if ((Intr_Mode & CAP_INT_MODE_MUTE_DETECT) != 0U) {
      data |= (0x1UL << 3U);
    }
    ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_INTERRUPT_SETTING_REGISTER), data,
                     CAP_MSK_INT_EN);
  }
}

void ithCap_Set_Color_Bar(CAPTURE_DEV_ID DEV_ID,
                          CAP_COLOR_BAR_CONFIG color_info) {
  uint32_t data = 0;

  if (color_info.b_en_colorbar) {
    data |= B_CAP_COLOR_BAR_ROLLING_EN;
  }
  data |= (color_info.v_act_start_line & 0x0FFFUL);
  data |= ((color_info.v_act_line & 0x0FFFUL) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER22), data);

  data = (color_info.blank_line1 & 0x0FFFUL);
  data |= ((color_info.act_line & 0x0FFFUL) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER23), data);

  data = (color_info.blank_line2 & 0x0FFFUL);
  data |= ((color_info.hs_act & 0x0FFFUL) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER24), data);

  data = (color_info.blank_pix1 & 0x0FFFUL);
  data |= ((color_info.act_pix & 0x0FFFUL) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER25), data);

  data = (color_info.blank_pix2 & 0x0FFFUL);

  if (color_info.b_hsync_pol) {
    data |= B_CAP_COLOR_BAR_HSPOL;
  }
  if (color_info.b_vsync_pol) {
    data |= B_CAP_COLOR_BAR_VSPOL;
  }
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER26), data);
}

void ithCap_Set_Interleave(CAPTURE_DEV_ID DEV_ID, uint32_t Interleave) {
  /* Set Interleave or Progressive */
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER0),
                   (Interleave << 1U), CAP_MSK_INTERLEAVE);
}

void ithCap_Set_Width_Height(CAPTURE_DEV_ID DEV_ID, uint32_t width,
                             uint32_t height) {
  uint32_t data = 0;

  /* Set Active Region  Set CapWidth & Cap Height */
  data = (width & N12_BITS_MSK);
  data |= ((height & N13_BITS_MSK) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER1), data);
}

void ithCap_Set_ROI_Size(CAPTURE_DEV_ID DEV_ID, uint32_t x, uint32_t y,
                         uint32_t width, uint32_t height) {
  /* The width & height size of ROI image */
  uint32_t data = 0;

  /* Set ROI */ /* The source frame start X position and start Y position */
  data = (x & N13_BITS_MSK);
  data |= ((y & N12_BITS_MSK) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_ROI_SETTING_REGISTER0), data);

  /* The width size and height size of ROI image */
  data = (width & N13_BITS_MSK);
  data |= ((height & N12_BITS_MSK) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_ROI_SETTING_REGISTER1), data);
}

void ithCap_Set_Clean_Intr(CAPTURE_DEV_ID DEV_ID) {
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_INTERRUPT_SETTING_REGISTER), BIT4,
                   CAP_MSK_INT_CLEAR);
#if 0
    ithPrintf("Clear (%d) intr\n", DEV_ID);
#endif
}

void ithCap_Set_Enable_Reg(CAPTURE_DEV_ID DEV_ID, CAP_ENFUN_INFO *pFunEn) {
  uint32_t data = 0;

  /* Null pointer check to prevent crashes */
  if (pFunEn == NULL)
  {
    return;
  }
  /* BT601 or BT656 */
  if(pFunEn->EnInBT656 == true)
  {
    data = 0x1U;
  }
  else
  {
    data = 0x0U;
  }

  /* Enable Hsync or not */
  if(pFunEn->EnHSync == true)
  {
    data |= CAP_MSK_HSEN;
  }

  /* Enable Data Enable or not */
  if(pFunEn->EnDEMode == true)
  {
    data |= CAP_MSK_DEEN;
  }
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER0), data,
                   (CAP_MSK_DEEN | CAP_MSK_HSEN | CAP_MSK_EMBEDDED));

  /* Enable CS fun */
  if(pFunEn->EnCSFun == true)
  {
    ithWriteRegMaskA(
        CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER0),
        CAP_MSK_EN_CSFUN, CAP_MSK_EN_CSFUN);
  }

  /* AutoDected Hsync & AutoDected Vsync or not */
  if(pFunEn->EnAutoDetHSPol == true)
  {
    data = CAP_MSK_HSPOL_DET;
  }
  else
  {
    data = 0U;
  }

  if(pFunEn->EnAutoDetVSPol == true)
  {
    data |= CAP_MSK_VSPOL_DET;
  }

  if(pFunEn->EnDumpMode == true)
  {
    data |= CAP_MSK_DUMPMODE;
  }
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING), data,
                   (CAP_MSK_HSPOL_DET | CAP_MSK_VSPOL_DET | CAP_MSK_DUMPMODE));

  if(pFunEn->EnMemContinousDump == true)
  {
    data = CAP_MSK_MEM_CONT_DUMP;
  }
  else
  {
    data = 0U;
  }

  if(pFunEn->EnSramNap == true)
  {
    data |= CAP_MSK_SRAM_NAP_EN;
  }

  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CAPTURE_OUTPUT_SETTING), data,
                   (CAP_MSK_MEM_CONT_DUMP | CAP_MSK_SRAM_NAP_EN));

  if(pFunEn->EnPort1UV2LineDS == true)
  {
    data = CAP_MSK_EN_UV_2LINE;
  }
  else
  {
    data = 0x0U;
  }

  if(pFunEn->EnMemLimit == true)
  {
    data |= CAP_MSK_MEM_LIMIT_EN;
  }

  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER8), data,
                   (CAP_MSK_EN_UV_2LINE | CAP_MSK_MEM_LIMIT_EN));
}

void ithCap_Set_Enable_Dither(CAPTURE_DEV_ID DEV_ID,
                              CAP_INPUT_DITHER_INFO *pDitherinfo) {
  uint32_t data = 0;

  /* Null pointer check to prevent crashes */
  if (pDitherinfo == NULL)
  {
    return;
  }

  switch(pDitherinfo->dithermode)
  {
    case DITHER_OL:
      data = 0x0U;
    break;
    case DITHER_1L:
      data = 0x1U;
    break;
    case DITHER_2L:
      data = 0x2U;
    break;
    case DITHER_3L:
      data = 0x3U;
    break;
    default:
      data = 0x0U;
    break;
  }

  if(pDitherinfo->b_endither == true)
  {
    data |= (N01_BITS_MSK << 3U);
  }
  ithWriteRegA(CapAddr_(DEV_ID, CAP_DITHERING_SETTING), data);
}

int32_t ithCap_Set_ISP_HandShaking(CAPTURE_DEV_ID DEV_ID,
                                   CAP_ISP_HANDSHAKING_MODE handshakmode,
                                   CAP_OUTPUT_INFO *pOutInfo) {
  uint32_t data = 0;
  int32_t result = -1;

  /* Null pointer check to prevent crashes */
  if (pOutInfo == NULL)
  {
    return result;
  }
  /*onfly mode*/
  if (handshakmode == ONFLY_MODE) {
    data |= B_CAP_ONFLY_MODE;
    ithWriteRegA(CapAddr_(DEV_ID, CAP_CAPTURE_OUTPUT_SETTING), data);

    data = ithReadRegA(CapAddr_(DEV_ID, CAP_CAPTURE_OUTPUT_SETTING));
    return ((data & B_CAP_ONFLY_MODE) == B_CAP_ONFLY_MODE) ? 0 : result;
  } else if (handshakmode == MEMORY_MODE) /*memory mode*/
  {
    data |= B_CAP_MEM_MODE;
    switch(pOutInfo->mem_format)
    {
        case SEMI_PLANAR_420:
         data = (data & 0xFFFFFFEFUL);
        break;

        case SEMI_PLANAR_422:
         data |= (0x1UL << 4U);
        break;

        case PACKET_MODE_422:
         data |= (0x2UL << 4U);
        break;

        default:
         data = (data & 0xFFFFFFEFUL);
        break;
    }
    ithWriteRegA(CapAddr_(DEV_ID, CAP_CAPTURE_OUTPUT_SETTING), data);

    data = ithReadRegA(CapAddr_(DEV_ID, CAP_CAPTURE_OUTPUT_SETTING));
    return ((data & B_CAP_MEM_MODE) == B_CAP_MEM_MODE) ? 0 : result;
  } else if (handshakmode == MEMORY_WITH_ONFLY_MODE) /*memory with onfly mode*/
  {
    data |= (B_CAP_ONFLY_MODE | B_CAP_MEM_MODE);

    switch(pOutInfo->mem_format)
    {
        case SEMI_PLANAR_420:
         data = (data & 0xFFFFFFEFUL);
        break;

        case SEMI_PLANAR_422:
         data |= (0x1UL << 4U);
        break;

        case PACKET_MODE_422:
         data |= (0x2UL << 4U);
        break;

        default:
         data = (data & 0xFFFFFFEFUL);
        break;
    }

    ithWriteRegA(CapAddr_(DEV_ID, CAP_CAPTURE_OUTPUT_SETTING), data);

    data = ithReadRegA(CapAddr_(DEV_ID, CAP_CAPTURE_OUTPUT_SETTING));
    return ((data & (B_CAP_ONFLY_MODE | B_CAP_MEM_MODE)) ==
                    (B_CAP_ONFLY_MODE | B_CAP_MEM_MODE))
               ? 0
               : result;
  } else {
    (void)printf("ISP HandShaking error\n");
    return result;
  }
}

void ithCap_Set_Error_Handleing(CAPTURE_DEV_ID DEV_ID,
                                   uint32_t errDetectEn) {

  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_ERROR_DETECT_SETTING_REGISTER),
                   errDetectEn, CAP_MSK_ERR_DETECT);
}

void ithCap_Set_Wait_Error_Reset(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = 0xFFFF0000UL;
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_ERROR_DETECT_SETTING_REGISTER), data,
                   CAP_MSK_ERR_RESET);

}

void ithCap_Set_Memory_AddressLimit_Reg(CAPTURE_DEV_ID DEV_ID,
                                        uint32_t memUpBound,
                                        uint32_t memLoBound) {
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER9), memUpBound);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER10), memLoBound);
}

void ithCap_Set_Buffer_addr_Reg(CAPTURE_DEV_ID DEV_ID, uint32_t *pYAddr,
                                uint32_t *pUVAddr, uint32_t addrOffset) {
  uint32_t vram_addr = 0;

  // Y0
  vram_addr = pYAddr[0];
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER11),
               (vram_addr + addrOffset));

  // UV0
  vram_addr = pUVAddr[0];
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER12),
               (vram_addr + addrOffset));

  // Y1
  vram_addr = pYAddr[1];
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER13),
               (vram_addr + addrOffset));

  // UV1
  vram_addr = pUVAddr[1];
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER14),
               (vram_addr + addrOffset));

  // Y2
  vram_addr = pYAddr[2];
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER15),
               (vram_addr + addrOffset));

  // UV2
  vram_addr = pUVAddr[2];
  ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER16),
               (vram_addr + addrOffset));

#if(CAPTURE_MEM_BUF_COUNT == 6U)
    // Y3
    vram_addr = pYAddr[0];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER17),
                 (vram_addr + addrOffset));

    // UV3
    vram_addr = pUVAddr[0];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER18),
                 (vram_addr + addrOffset));

    // Y4
    vram_addr = pYAddr[0];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER19),
                 (vram_addr + addrOffset));

    // UV4
    vram_addr = pUVAddr[0];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER20),
                 (vram_addr + addrOffset));
#elif(CAPTURE_MEM_BUF_COUNT == 8U)
    // Y3
    vram_addr = pYAddr[3];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER17),
                 (vram_addr + addrOffset));

    // UV3
    vram_addr = pUVAddr[3];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER18),
                 (vram_addr + addrOffset));

    // Y4
    vram_addr = pYAddr[0];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER19),
                 (vram_addr + addrOffset));

    // UV4
    vram_addr = pUVAddr[0];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER20),
                 (vram_addr + addrOffset));
#else
    // Y3
    vram_addr = pYAddr[3];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER17),
                 (vram_addr + addrOffset));

    // UV3
    vram_addr = pUVAddr[3];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER18),
                 (vram_addr + addrOffset));

    // Y4
    vram_addr = pYAddr[4];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER19),
                 (vram_addr + addrOffset));

    // UV4
    vram_addr = pUVAddr[4];
    ithWriteRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER20),
                 (vram_addr + addrOffset));
#endif

  /*Set frame buffer num , memory mode*/
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER8),
                   (CAPTURE_MEM_BUF_COUNT >> 1) - 1U, CAP_MSK_WR_BUFFER_NUM);
}

void ithCap_Set_ScaleParam_Reg(CAPTURE_DEV_ID DEV_ID,
                               const CAP_SCALE_CTRL *pScaleFun) {
  uint32_t HCI;

  HCI = cap_floattofix_(pScaleFun->HCI, 6, 14);

  ithWriteRegA(CapAddr_(DEV_ID, CAP_SCALING_SETTING_REGISTER1),
               (uint32_t)(HCI & CAP_MSK_HOR_SCALE_HCI));
}

void ithCap_Set_IntScaleMatrixH_Reg(CAPTURE_DEV_ID DEV_ID,
                                    uint32_t WeightMatX[][CAP_SCALE_TAP]) {
  uint32_t Value;

  Value = (uint32_t)((WeightMatX[0][0] & CAP_BIT_SCALEWEIGHT)
                     << CAP_SHT_SCALEWEIGHT_L) |
          ((WeightMatX[0][1] & CAP_BIT_SCALEWEIGHT) << CAP_SHT_SCALEWEIGHT_H) |
          ((WeightMatX[0][2] & CAP_BIT_SCALEWEIGHT) << 16U) |
          ((WeightMatX[0][3] & CAP_BIT_SCALEWEIGHT) << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_SCALING_SETTING_REGISTER2),
               (uint32_t)Value);

  Value = (uint32_t)((WeightMatX[1][0] & CAP_BIT_SCALEWEIGHT)
                     << CAP_SHT_SCALEWEIGHT_L) |
          ((WeightMatX[1][1] & CAP_BIT_SCALEWEIGHT) << CAP_SHT_SCALEWEIGHT_H) |
          ((WeightMatX[1][2] & CAP_BIT_SCALEWEIGHT) << 16U) |
          ((WeightMatX[1][3] & CAP_BIT_SCALEWEIGHT) << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_SCALING_SETTING_REGISTER3),
               (uint32_t)Value);

  Value = (uint32_t)((WeightMatX[2][0] & CAP_BIT_SCALEWEIGHT)
                     << CAP_SHT_SCALEWEIGHT_L) |
          ((WeightMatX[2][1] & CAP_BIT_SCALEWEIGHT) << CAP_SHT_SCALEWEIGHT_H) |
          ((WeightMatX[2][2] & CAP_BIT_SCALEWEIGHT) << 16U) |
          ((WeightMatX[2][3] & CAP_BIT_SCALEWEIGHT) << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_SCALING_SETTING_REGISTER4),
               (uint32_t)Value);

  Value = (uint32_t)((WeightMatX[3][0] & CAP_BIT_SCALEWEIGHT)
                     << CAP_SHT_SCALEWEIGHT_L) |
          ((WeightMatX[3][1] & CAP_BIT_SCALEWEIGHT) << CAP_SHT_SCALEWEIGHT_H) |
          ((WeightMatX[3][2] & CAP_BIT_SCALEWEIGHT) << 16U) |
          ((WeightMatX[3][3] & CAP_BIT_SCALEWEIGHT) << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_SCALING_SETTING_REGISTER5),
               (uint32_t)Value);

  Value = (uint32_t)((WeightMatX[4][0] & CAP_BIT_SCALEWEIGHT)
                     << CAP_SHT_SCALEWEIGHT_L) |
          ((WeightMatX[4][1] & CAP_BIT_SCALEWEIGHT) << CAP_SHT_SCALEWEIGHT_H) |
          ((WeightMatX[4][2] & CAP_BIT_SCALEWEIGHT) << 16U) |
          ((WeightMatX[4][3] & CAP_BIT_SCALEWEIGHT) << 24U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_SCALING_SETTING_REGISTER6),
               (uint32_t)Value);
}

//=============================================================================
/**
 * @brief This function sets the RGB to YUV color conversion matrix
 * @param DEV_ID engine id
 * @param pRGBtoYUV Transfer matrix
 * @return none.
 */
//=============================================================================
void ithCap_Set_RGBtoYUVMatrix_Reg(CAPTURE_DEV_ID DEV_ID,
                                   const CAP_RGB_TO_YUV *pRGBtoYUV) {
  uint32_t data = 0;

  /*CSOffset 1~3 all setting to zero*/
  ithWriteRegMaskA(
      CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER0), 0x0U,
      0x00FFFFFFUL);

  data = (pRGBtoYUV->m11 & CAP_BIT_RGB_TO_YUV);
  data |= ((pRGBtoYUV->m12 & CAP_BIT_RGB_TO_YUV) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER1),
               data);

  data = (pRGBtoYUV->m13 & CAP_BIT_RGB_TO_YUV);
  data |= ((pRGBtoYUV->m21 & CAP_BIT_RGB_TO_YUV) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER2),
               data);

  data = (pRGBtoYUV->m22 & CAP_BIT_RGB_TO_YUV);
  data |= ((pRGBtoYUV->m23 & CAP_BIT_RGB_TO_YUV) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER3),
               data);

  data = (pRGBtoYUV->m31 & CAP_BIT_RGB_TO_YUV);
  data |= ((pRGBtoYUV->m32 & CAP_BIT_RGB_TO_YUV) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER4),
               data);

  data = (pRGBtoYUV->m33 & CAP_BIT_RGB_TO_YUV);
  data |= ((pRGBtoYUV->const_y & CAP_BIT_RGB_TO_YUV_CONST) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER5),
               data);

  data = (pRGBtoYUV->const_u & CAP_BIT_RGB_TO_YUV_CONST);
  data |= ((pRGBtoYUV->const_v & CAP_BIT_RGB_TO_YUV_CONST) << 16U);
  ithWriteRegA(CapAddr_(DEV_ID, CAP_COLOR_SPACE_CONVERSION_SETTING_REGISTER6),
               data);
}

void ithCap_Set_MemThreshold(CAPTURE_DEV_ID DEV_ID, uint32_t threshold) {
  uint32_t data = 0;
  data = ((threshold & N05_BITS_MSK) << 8U);
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER8), data,
                   CAP_MSK_WR_MERGE_TH);
}

//=============================================================================
/**
 * @brief This function sets the image source.
 * @param DEV_ID engine id
 * @param source 1:external IO ,2:internal IO ,4:internal LCD
 * @return none.
 */
//=============================================================================
void ithCap_Set_VideoSignalSource(CAPTURE_DEV_ID DEV_ID, uint8_t source) {
  uint8_t data = 0;

  data = source & 0x7U;

  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_CLOCK_SETTING_REGISTER), data,
                   CAP_MSK_UCLKSRC);
}

//=============================================================================
//
// Audio/Video/Mute Counter control function
//
//=============================================================================
void ithAVSync_CounterCtrl(CAPTURE_DEV_ID DEV_ID, uint32_t ctlmode,
                           uint32_t divider) {
  if (ctlmode == AUDIO_COUNTER_SEL) {
    ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_AV_SYNC_SETTING_REGISTER1), divider,
                     0x000007FFUL);
  } else if (ctlmode == VIDEO_COUNTER_SEL) {
    ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_AV_SYNC_SETTING_REGISTER1),
                     ((divider & 0x000007FFUL) << 16U), 0x000007FFUL << 16U);
  } else if (ctlmode == MUTE_COUNTER_SEL) {
    ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_AV_SYNC_SETTING_REGISTER0),
                     ((divider & 0x000007FFUL) << 16U), 0x000007FFUL << 16U);
  }
  else  {
    (void)printf("ithAVSync_CounterCtrl mode error\n");
  }
}

void ithAVSync_CounterReset(CAPTURE_DEV_ID DEV_ID, uint32_t resetmode) {
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_AV_SYNC_SETTING_REGISTER0),
                   (resetmode & 0x001FUL), 0x001FUL);
}

uint32_t ithAVSync_CounterRead(CAPTURE_DEV_ID DEV_ID, uint32_t readmode) {
  uint32_t value = 0;

  if (readmode == AUDIO_COUNTER_SEL) {
    value = ithReadRegA(CapAddr_(DEV_ID, CAP_AV_SYNC_STATUS_REGISTER0));
  } else if (readmode == VIDEO_COUNTER_SEL) {
    value = ithReadRegA(CapAddr_(DEV_ID, CAP_AV_SYNC_STATUS_REGISTER1));
  } else if (readmode == MUTE_COUNTER_SEL) {
    value = ithReadRegA(CapAddr_(DEV_ID, CAP_AV_SYNC_STATUS_REGISTER2));
  } else if (readmode == MUTEPRE_COUNTER_SEL) {
    value = ithReadRegA(CapAddr_(DEV_ID, CAP_AV_SYNC_STATUS_REGISTER3));
  }
  else {
    (void)printf("ithAVSync_CounterRead mode error\n");
  }

  return value;
}

bool ithAVSync_MuteDetect(CAPTURE_DEV_ID DEV_ID) {
  uint32_t value;

  value = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));

  if ((value & 0x01000000UL) == 0x01000000UL) {
    return true;
  } else {
    return false;
  }
}

//=============================================================================
//                Public Get Function Definition
//=============================================================================

bool ithCap_Get_IsFire(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER39));

  if ((data & 0x0003UL) == 0x0003UL) {
    return true;
  } else {
    return false;
  }
}

int32_t ithCap_Get_WaitEngineIdle(CAPTURE_DEV_ID DEV_ID) {
  int32_t result = 0;
  uint32_t status = 0;
  uint32_t timeOut = 0;

  /*change to look the engine busy bit.*/
  status = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
  while (!(status & 0x1000UL)) {
    (void)usleep(1000);
    if (++timeOut > 2000U) {
      (void)printf("Capture still busy !!!!!\n");
      result = -1;
      goto end;
    }
    status = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
  }

end:
  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!!\n", __FUNCTION__, __LINE__);
  }

  return (int32_t)result;
}

/* Get Capture Lane error status */
uint32_t ithCap_Get_Lane_status(CAPTURE_DEV_ID DEV_ID,
                                CAP_LANE_STATUS lanenum) {
  uint32_t data = 0;

  switch (lanenum) {
  case CAP_LANE0_STATUS:
    data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
    break;
  default:
    (void)printf("%s (%d) ERROR\n", __FUNCTION__, __LINE__);
    break;
  }

  return (uint32_t)data;
}

uint32_t ithCap_Get_Hsync_Polarity(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING));
  data = (data & 0x01UL);
  return data;
}

uint32_t ithCap_Get_Vsync_Polarity(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING));
  data = ((data & 0x02UL) >> 1U);
  return data;
}

uint32_t ithCap_Get_Hsync_Polarity_Status(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
  data &= 0x01UL;
  (void)printf("%s data = 0x%x\n", __FUNCTION__, data);
  return data;
}

uint32_t ithCap_Get_Vsync_Polarity_Status(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
  data &= 0x02UL;
  (void)printf("%s data = 0x%x\n", __FUNCTION__, data);
  return data;
}

uint32_t ithCap_Get_Revision(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER3));
  data &= CAP_MSK_REV_NUM;
  (void)printf("%s data = 0x%x\n", __FUNCTION__, data);
  return data;
}

uint32_t ithCap_Get_MRawVTotal(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER3));
  return ((data & 0xFFFF0000UL) >> 16U);
}

uint32_t ithCap_Get_Detectd_Hsync_Polarity(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING));
  data = ((data & 0x00000040UL) >> 6U);
  return data;
}

uint32_t ithCap_Get_Detectd_Vsync_Polarity(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_CAPTURE_INTERNAL_SETTING));
  data = ((data & 0x00000080UL) >> 7U);
  return data;
}

uint32_t ithCap_Get_Detected_Width(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data, InputBitWidth;

  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER2));
  InputBitWidth = ithReadRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER2));
  InputBitWidth = (InputBitWidth & 0xC0UL) >> 6U;

  if (InputBitWidth > 0) {
    data /= InputBitWidth;
  }

  return ((data & 0x0FFFUL));
}

uint32_t ithCap_Get_Detected_Height(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data = 0;

  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER2));
  return ((data & 0x1FFF0000UL) >> 16U);
}

uint32_t ithCap_Get_Detected_Interleave(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data = 0;

  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
  data = (data & 0x08000000UL) >> 27U;
  return data;
}

uint32_t ithCap_Get_Error_Status(CAPTURE_DEV_ID DEV_ID) {
  uint32_t data = 0;
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_ENGINE_STATUS_REGISTER0));
  if (data & 0xF00UL) {
    return (data & 0xF00UL) >> 8U;
  } else {
    return 0;
  }
}

uint32_t ithCap_Get_FIFOMAX(CAPTURE_DEV_ID DEV_ID, bool ISFIFO_Y) {
  uint32_t data = 0;

  /* Debug Sel = 0 */
  ithWriteRegMaskA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER40), 0x0UL,
                   0x3FUL);

  /* Read FIFO MAX Y & UV */
  data = ithReadRegA(CapAddr_(DEV_ID, CAP_DEBUG_STATUS_REGISTER));

  if (ISFIFO_Y) {
    data = data & 0x1FFUL; // Y FIFO MAX
  } else {
    data = (data & 0x1FF0000UL) >> 16U; // UV FIFO MAX
  }

  return data;
}

//=============================================================================
/**
 * @brief Read all registers; this function is used for debugging.
 * @param DEV_ID engine id
 * @return none.
 */
//=============================================================================
void ithCap_Dump_Reg(CAPTURE_DEV_ID DEV_ID) {
  uint32_t i, data;

  for (i = 0; i <= 544; i += 4) {
    data = ithReadRegA(CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER0 + i));
    (void)ithPrintf("reg=0x%x ,val=0x%08x\n",
                    CapAddr_(DEV_ID, CAP_GENERAL_SETTING_REGISTER0 + i), data);
  }
  (void)ithPrintf("\n");
}
