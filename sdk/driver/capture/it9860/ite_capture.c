#include "ite_capture.h"
#include "capture.h"
#include "capture_config.h"
#include "capture_hw.h"
#include <string.h>

//=============================================================================
//                Constant Definition
//=============================================================================
#define CAPCONFIGNUM 64
#define TIMING_RANG 2
//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================
uint32_t CAPConfigTable[] = {
#include "defcap_config.h"
};

/* record how many device opened */
uint32_t g_cap_num;
/* for g_cap_num mutex protect*/
pthread_mutex_t g_cap_mutex = PTHREAD_MUTEX_INITIALIZER;

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================
//=============================================================================
/**
 * @brief Cap context initialization.
 * @param  none.
 * @return  int32_t,init success or fail
 */
//=============================================================================

int32_t iteCapInitialize(void) {
  int32_t result = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);

  ithCap_Set_Reset();
  g_cap_num = 0;

  (void)pthread_mutex_unlock(&g_cap_mutex);
  return result;
}

//=============================================================================
/**
 * @brief Cap connect source and capinfo default init.
 * @param *ptDev,points to capture_handle structure.
 * @param info, onfly mode ,memory mode, max width, max height param.
 * @return int32_t,connect success or fail.
 */
//=============================================================================
int32_t iteCapConnect(CAPTURE_HANDLE *ptDev, CAPTURE_SETTING info) {
  int32_t result = 0;
  /* cap0 config */
  uint32_t *gp_cap_config = CAPConfigTable;

  (void)pthread_mutex_lock(&g_cap_mutex);

  if (ptDev == NULL || g_cap_num >= CAP_DEVICE_ID_MAX) {
    result = -1;
    goto end;
  }

  /* reset caphandle mem */
  (void)memset((void *)ptDev, 0, sizeof(CAPTURE_HANDLE));

  /* count capture be init num */
  g_cap_num++;

  /* cap id setting */
  if (g_cap_num == 1U) {
    ptDev->cap_id = CAP_DEV_ID0;
  }

  /* default capinfo setting */
  if(cap_init_(&ptDev->cap_info) != 0) {
    result = -1;
    goto end;
  }

  /* capture set user config*/
  if (ptDev->cap_id == CAP_DEV_ID0) {
    cap_conifg_(ptDev, gp_cap_config);
  }
  /* frontend source id setting */
  ptDev->source_id = info.inputsource;

  /* capture onflymode flag setting */
  if (info.b_onflymode_en) {
    ptDev->cap_info.b_en_onfly_mode = true; // onfly mode
  } else {
    ptDev->cap_info.b_en_onfly_mode = false;
    cap_memory_init_(ptDev, info); // memory mode
  }

  /* capture interrupt flag setting */
  if (info.b_Interrupt_en) {
    ptDev->cap_info.b_en_interrupt = true;
  } else {
    ptDev->cap_info.b_en_interrupt = false;
  }

  /* setting mem pitchY pitchUV */
  ptDev->cap_info.outinfo.pitch_y = info.max_width;
  ptDev->cap_info.outinfo.pitch_uv = info.max_width;

end:
  (void)pthread_mutex_unlock(&g_cap_mutex);
  if (result != 0) {
    (void)printf("%s error %d", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief free memory ,and reset cap_handle .
 * @param *ptDev,points to capture_handle structure.
 * @return  int32_t,disconnect success or fail.
 */
//=============================================================================

int32_t iteCapDisConnect(CAPTURE_HANDLE *ptDev) {
  int32_t result = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);
  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  /* engine is running? */
  if (ithCap_Get_IsFire(ptDev->cap_id)) {
    result = -1;
    goto end;
  }
  /* memory mode */
  if (!ptDev->cap_info.b_en_onfly_mode && ptDev->cap_info.b_en_interrupt) {
    cap_memory_deinit_(ptDev);
  }

  /* reset caphandle mem */
  (void)memset((void *)ptDev, 0, sizeof(CAPTURE_HANDLE));

  if (g_cap_num > 0U) {
    g_cap_num--;
  }
end:
  (void)pthread_mutex_unlock(&g_cap_mutex);
  if (result != 0) {
    (void)printf("%s error %d", __FUNCTION__, __LINE__);
  }

  return result;
}

//=============================================================================
/**
 * @brief Cap terminate all engine,include disable engine(unfire)  and reset
 * engine.
 * @param  none.
 * @return  int32_t, -1 => fail 0 => success
 */
//=============================================================================

int32_t iteCapTerminate(void) {
  int32_t result = 0;
  uint32_t index = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);
  /* Disable Cap all engine */
  for (index = 0; index < CAP_DEVICE_ID_MAX; index++) {
    if (ithCap_Get_IsFire((CAPTURE_DEV_ID)index)) {
      ithCap_Set_UnFire((CAPTURE_DEV_ID)index);
      result = ithCap_Get_WaitEngineIdle((CAPTURE_DEV_ID)index);
      if (result) {
        (void)printf(" err 0x%x !\n", result);
        goto end;
      }
    }
    ithCap_Set_Clean_Intr((CAPTURE_DEV_ID)index);
  }

  ithCap_Set_Reset();
end:
  (void)pthread_mutex_unlock(&g_cap_mutex);
  if (result != 0) {
    (void)printf(" %s() err 0x%x !\n", __FUNCTION__, result);
  }

  return result;
}

//=============================================================================
/**
 * @brief Cap current fire status.
 * @param *ptDev,points to capture_handle structure.
 * @return true => fire(running) , false => unfire(stop)
 */
//=============================================================================
bool iteCapIsFire(CAPTURE_HANDLE *ptDev) {
  bool b_status = false;
  (void)pthread_mutex_lock(&g_cap_mutex);
  b_status = ithCap_Get_IsFire(ptDev->cap_id);
  (void)pthread_mutex_unlock(&g_cap_mutex);
  return b_status;
}

//=============================================================================
/**
 * @brief Cap Get EngineErrorStatus [this function can be called in ISR].
 * @param *ptDev,points to capture_handle structure.
 * @param lanenum,LANE0 => reg[0x200].
 * @return
 * bit[3:0]Stable status:
 * [0]:Hsync stable
 * [1]:Vsync stable
 * [2]:DE stable(X)
 * [3]:DE stable(Y)
 * bit[11:8] Error status:
 * [1]:Hsync loss
 * [2]:Vsync loss
 * [3]:DE loss
 * [4]:frame end error
 * [5]:capture overflow
 * [7]:frame rate change
 * [8]:time out
 */
//=============================================================================
uint32_t iteCapGetEngineErrorStatus(CAPTURE_HANDLE *ptDev,
                                    MMP_CAP_LANE_STATUS lanenum) {
  uint32_t err_status = 0;

  err_status = ithCap_Get_Lane_status(ptDev->cap_id, lanenum);

  return err_status;
}

//=============================================================================
/**
 * @brief Cap Set parameter to hw registers.
 * @param *ptDev,points to capture_handle structure.
 * @return int32_t, -1 => fail 0 => success.
 */
//=============================================================================
int32_t iteCapParameterSetting(CAPTURE_HANDLE *ptDev) {
  int32_t result = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);
  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  /* Update parameter */
  result = cap_update_reg_(ptDev);
#if 0
    ithCap_Dump_Reg(ptDev->cap_id);
#endif
end:
  (void)pthread_mutex_unlock(&g_cap_mutex);
  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief Set Cap fire or not.
 * @param *ptDev,points to capture_handle structure.
 * @param enable,true => fire, false => unfire.
 * @return none.
 */
//=============================================================================
void iteCapFire(CAPTURE_HANDLE *ptDev, bool enable) {
  (void)pthread_mutex_lock(&g_cap_mutex);
  if (enable) {

    /* Update Error Handle mode */
    if (ptDev->source_id == MMP_CAP_DEV_ANALOG_DECODER) {
      ithCap_Set_Error_Handleing(ptDev->cap_id, 0xFF7CUL);
    } else {
      ithCap_Set_Error_Handleing(ptDev->cap_id, 0xFF7FUL);
    }

    ithCap_Set_Wait_Error_Reset(ptDev->cap_id);

    ithCap_Set_Fire(ptDev->cap_id);
  } else {
    ithCap_Set_UnFire(ptDev->cap_id);
  }
  (void)pthread_mutex_unlock(&g_cap_mutex);
}
//=============================================================================
/**
 * @brief Register  an interrupt handler[Only OPENRTOS]
 * @param caphandler,user define caphandler
 * @param *ptDev,points to capture_handle structure.
 * @return none.
 */
//=============================================================================
void iteCapRegisterIRQ(ITHIntrHandler caphandler, CAPTURE_HANDLE *ptDev) {
  /* Initialize Capture IRQ */
  ithIntrDisableIrq(ITH_INTR_CAPTURE);
  ithIntrClearIrq(ITH_INTR_CAPTURE);
#if defined(__OPENRTOS__)
  /* register NAND Handler to IRQ */
  ithIntrRegisterHandlerIrq(ITH_INTR_CAPTURE, caphandler, (void *)ptDev);
#endif // defined (__OPENRTOS__)

  /* set IRQ to edge trigger */
  ithIntrSetTriggerModeIrq(ITH_INTR_CAPTURE, ITH_INTR_EDGE);

  /* set IRQ to detect rising edge */
  ithIntrSetTriggerLevelIrq(ITH_INTR_CAPTURE, ITH_INTR_HIGH_RISING);

  /* Enable IRQ */
  ithIntrEnableIrq(ITH_INTR_CAPTURE);
}

//=============================================================================
/**
 * @brief Cap disable IRQ[Only OPENRTOS]
 * @param none.
 * @return none.
 */
//=============================================================================
void iteCapDisableIRQ(void) {
  /* Initialize Capture IRQ */
  ithIntrDisableIrq(ITH_INTR_CAPTURE);
  ithIntrClearIrq(ITH_INTR_CAPTURE);
}

//=============================================================================
/**
 * @brief Clear Interrupt [this function can be called in ISR]
 * @param *ptDev,points to capture_handle structure.
 * @return 0.
 */
//=============================================================================

uint32_t iteCapClearInterrupt(CAPTURE_HANDLE *ptDev, bool get_err) {
  /* error reset flow */
  if (get_err) {
    ithCap_Set_ErrReset(ptDev->cap_id);
#if 0
        ithCap_Set_Error_Handleing(ptDev->cap_id, 0x0);
#endif
  }

  ithCap_Set_Clean_Intr(ptDev->cap_id);
  return 0;
}

//=============================================================================
/**
 * @brief This function is used to set the active region for capturing images.
 * @param width active width
 * @param height active height
 * @return int32_t 0 : success
 */
//=============================================================================

int32_t iteCapSetInputSize(CAPTURE_HANDLE *ptDev, uint32_t width,
                           uint32_t height) {

  int32_t result = 0;

  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  (void)pthread_mutex_lock(&g_cap_mutex);

  ptDev->cap_info.ininfo.act_width = width;
  ptDev->cap_info.ininfo.act_height = height;

  ithCap_Set_Width_Height(ptDev->cap_id, width, height);

  (void)pthread_mutex_unlock(&g_cap_mutex);

end:

  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief Set capture interleave mode.
 * @param *ptDev,points to capture_handle structure.
 * @return int32_t 0 : success
 */
//=============================================================================
int32_t iteCapSetInterleave(CAPTURE_HANDLE *ptDev, uint32_t interleave) {

  int32_t result = 0;

  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  (void)pthread_mutex_lock(&g_cap_mutex);

  if (interleave == 0U) {
    ptDev->cap_info.ininfo.Interleave = PROGRESSIVE;
  } else {
    ptDev->cap_info.ininfo.Interleave = INTERLEAVING;
  }

  ithCap_Set_Interleave(ptDev->cap_id, interleave);
  (void)pthread_mutex_unlock(&g_cap_mutex);

end:
  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief This function sets the region of interest for the image.
 * @param *ptDev,points to capture_handle structure
 * @param  x  (x, y) is the coordinate of the top-left corner.
 * @param  y  (x, y) is the coordinate of the top-left corner.
 * @param  width width and height are the respective dimensions of the range.
 * @param  height  width and height are the respective dimensions of the range.
 * @return int32_t 0 : success
 */
//=============================================================================

int32_t iteCapSetRoi(CAPTURE_HANDLE *ptDev, uint32_t x, uint32_t y,
                     uint32_t width, uint32_t height) {
  int32_t result = 0;

  (void)pthread_mutex_lock(&g_cap_mutex);

  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  if ((x + width) > ptDev->cap_info.ininfo.act_width) {
    (void)printf("roi over size\n");
    result = -1;
    goto end;
  }

  if ((y + height) > ptDev->cap_info.ininfo.act_height) {
    (void)printf("roi over size\n");
    result = -1;
    goto end;
  }

  ptDev->cap_info.ininfo.roi_x = x;
  ptDev->cap_info.ininfo.roi_y = y;
  ptDev->cap_info.ininfo.roi_width = width;
  ptDev->cap_info.ininfo.roi_height = height;

  ithCap_Set_ROI_Size(ptDev->cap_id, x, y, width, height);

end:
  (void)pthread_mutex_unlock(&g_cap_mutex);

  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief This function sets the width value in memory mode.
 * @param *ptDev,points to capture_handle structure.
 * @param  width sets the output video width in memory mode.
 * @return int32_t 0 : success
 */
//=============================================================================

int32_t iteCapSetOutputSize(CAPTURE_HANDLE *ptDev, uint32_t width) {

  int32_t result = 0;

  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  (void)pthread_mutex_lock(&g_cap_mutex);

  ptDev->cap_info.outinfo.width = width;
  ptDev->cap_info.outinfo.height = ptDev->cap_info.ininfo.roi_height;

  /* update scale width */
  ithCap_Set_Output_Width_Reg(ptDev->cap_id, &ptDev->cap_info.outinfo);

  (void)pthread_mutex_unlock(&g_cap_mutex);

end:

  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief This function sets the pitch value in memory mode.
 * @param *ptDev,points to capture_handle structure.
 * @param y y pitch value.
 * @param uv uv pitch value.
 * @return int32_t 0 : success
 */
//=============================================================================

int32_t iteCapSetOutputPitch(CAPTURE_HANDLE *ptDev, uint32_t y, uint32_t uv) {

  int32_t result = 0;

  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  (void)pthread_mutex_lock(&g_cap_mutex);

  ptDev->cap_info.outinfo.pitch_y = y;
  ptDev->cap_info.outinfo.pitch_uv = uv;

  /* memory pitch & format*/
  ithCap_Set_Output_Reg(ptDev->cap_id, &ptDev->cap_info.outinfo);

  (void)pthread_mutex_unlock(&g_cap_mutex);

end:

  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief This function sets the parameters related to color transformation.
 * @param *ptDev,points to capture_handle structure.
 * @param cs_en the color transformation feature is enabled.
 * @param type Set the color transformation matrix.
 * @return int32_t 0 : success
 */
//=============================================================================

int32_t iteCapSetColorConver(CAPTURE_HANDLE *ptDev, bool cs_en,
                             CAP_COLOR_MATRIX type) {

  int32_t result = 0;

  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  (void)pthread_mutex_lock(&g_cap_mutex);

  ptDev->cap_info.funen.EnCSFun = cs_en;
  ptDev->cap_info.color_matrix_type = type;

  (void)pthread_mutex_unlock(&g_cap_mutex);

end:

  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}

//=============================================================================
/**
 * @brief This function controls the frame dropping mode of the capture engine.
 * @param *ptDev,points to capture_handle structure.
 * @param drop_control control i-th frame drop or not, 0:drop frame 1: capture frame(value range: 0xFFFF)
 * @param drop_period  Set the evaluation period.(value range: 0xF)
 * @return int32_t 0 : success
 * 
 *    example:
 *    (drop_control : 0xF , drop_period : 0x3)  all capture
 *    (drop_control : 0x3B , drop_period : 0x5)  Drop one frame every six frames.
 *
 */
//=============================================================================
int32_t iteCapSetDrop(CAPTURE_HANDLE *ptDev, uint32_t drop_control,
                             uint32_t drop_period) {

  int32_t result = 0;

  if (ptDev == NULL) {
    result = -1;
    goto end;
  }

  (void)pthread_mutex_lock(&g_cap_mutex);

  ptDev->cap_info.skip_period  = drop_period;
  ptDev->cap_info.skip_pattern = drop_control;


  (void)pthread_mutex_unlock(&g_cap_mutex);

end:

  if (result != 0) {
    (void)printf("%s (%d) ERROR !!!!\n", __FUNCTION__, __LINE__);
  }
  return result;
}
//=============================================================================
/**
 * @brief Get current number of memory ring buffer [this function can be called
 * in ISR]
 * @param *ptDev,points to capture_handle structure.
 * @return current buffer number.
 */
//=============================================================================

uint32_t iteCapReturnWrBufIndex(CAPTURE_HANDLE *ptDev) {
  uint32_t wr_index = 0;
  wr_index =
      ((ithCap_Get_Lane_status(ptDev->cap_id, CAP_LANE0_STATUS) & 0x70UL) >>
       4U);
  return wr_index;
}

//=============================================================================
/**
 * @brief Get current input Source Frame Rate(Note: need wait capture 1 frame )
 * @param *ptDev,points to capture_handle structure.
 * @return MMP_CAP_FRAMERATE, hw dectected current frame rate.
 */
//=============================================================================
MMP_CAP_FRAMERATE
iteCapGetInputFrameRate(CAPTURE_HANDLE *ptDev) {
  uint32_t raw_vtotal = 0;
  uint32_t mclk_freq = 0;
  uint32_t framerate = 0;
  uint32_t interlaced = 0;
  float temp_framerate;
  MMP_CAP_FRAMERATE fr_mode = MMP_CAP_FRAMERATE_UNKNOW;
  (void)pthread_mutex_lock(&g_cap_mutex);
  raw_vtotal = ithCap_Get_MRawVTotal(ptDev->cap_id);
  interlaced = ithCap_Get_Detected_Interleave(ptDev->cap_id);
  if (raw_vtotal > 0U) {
    if (interlaced == 1U) {
      raw_vtotal = raw_vtotal / 2U;
    }

    mclk_freq = ithGetMclk() / 1000U;
    temp_framerate = (3906.25f * mclk_freq) / (float)raw_vtotal;
    framerate = (uint32_t)temp_framerate;
#if 0
        (void)printf("raw_vtotal = %d mclk_freq = %d framerate = %d\n", raw_vtotal, mclk_freq, framerate);
#endif
  }

  if ((23988UL > framerate) && (framerate > 23946UL)) // 23.976fps
  {
    fr_mode = MMP_CAP_FRAMERATE_23_97HZ;
  } else if ((24030UL > framerate) && (framerate > 23987UL)) // 24fps
  {
    fr_mode = MMP_CAP_FRAMERATE_24HZ;
  } else if ((25030UL > framerate) && (framerate > 24970UL)) // 25fps
  {
    fr_mode = MMP_CAP_FRAMERATE_25HZ;
  } else if ((29985UL > framerate) && (framerate > 29940UL)) // 29.97fps
  {
    fr_mode = MMP_CAP_FRAMERATE_29_97HZ;
  } else if ((30030UL > framerate) && (framerate > 29984UL)) // 30fps
  {
    fr_mode = MMP_CAP_FRAMERATE_30HZ;
  } else if ((51000UL > framerate) && (framerate > 49000UL)) // 50fps
  {
    fr_mode = MMP_CAP_FRAMERATE_50HZ;
  } else if ((57000UL > framerate) && (framerate > 55000UL)) // 56fps
  {
    fr_mode = MMP_CAP_FRAMERATE_56HZ;
  } else if ((59970UL > framerate) && (framerate > 57001UL)) // 59.94fps
  {
    fr_mode = MMP_CAP_FRAMERATE_59_94HZ;
  } else if ((62030UL > framerate) && (framerate > 59969UL)) // 60fps
  {
    fr_mode = MMP_CAP_FRAMERATE_60HZ;
  } else if ((70999UL > framerate) && (framerate > 69000UL)) // 70fps
  {
    fr_mode = MMP_CAP_FRAMERATE_70HZ;
  } else if ((73000UL > framerate) && (framerate > 71000UL)) // 72fps
  {
    fr_mode = MMP_CAP_FRAMERATE_72HZ;
  } else if ((76000UL > framerate) && (framerate > 74000UL)) // 75fps
  {
    fr_mode = MMP_CAP_FRAMERATE_75HZ;
  } else if ((86000UL > framerate) && (framerate > 84000UL)) // 85fps
  {
    fr_mode = MMP_CAP_FRAMERATE_85HZ;
  } else {
    fr_mode = MMP_CAP_FRAMERATE_UNKNOW;
  }
#if 0
    (void)printf("frame rate mode = %d \n",fr_mode);
#endif
  (void)pthread_mutex_unlock(&g_cap_mutex);
  return fr_mode;
}

//=============================================================================
/**
 * @brief iteAVSyncCounterInit
 */
//=============================================================================
void iteAVSyncCounterCtrl(CAPTURE_HANDLE *ptDev, uint32_t mode,
                          uint32_t divider) {
  (void)pthread_mutex_lock(&g_cap_mutex);
  ithAVSync_CounterCtrl(ptDev->cap_id, mode, divider);
  (void)pthread_mutex_unlock(&g_cap_mutex);
}

//=============================================================================
/**
 * @brief iteAVSyncCounterReset
 */
//=============================================================================
void iteAVSyncCounterReset(CAPTURE_HANDLE *ptDev, uint32_t mode) {
  (void)pthread_mutex_lock(&g_cap_mutex);
  ithAVSync_CounterReset(ptDev->cap_id, mode);
  (void)pthread_mutex_unlock(&g_cap_mutex);
}

//=============================================================================
/**
 * @brief iteAVSyncCounterRead
 */
//=============================================================================
uint32_t iteAVSyncCounterRead(CAPTURE_HANDLE *ptDev, uint32_t mode) {
  uint32_t counter_read = 0;

  (void)pthread_mutex_lock(&g_cap_mutex);
  counter_read = ithAVSync_CounterRead(ptDev->cap_id, mode);
  (void)pthread_mutex_unlock(&g_cap_mutex);

  return counter_read;
}

//=============================================================================
/**
 * @brief iteAVSyncMuteDetect
 */
//=============================================================================
bool iteAVSyncMuteDetect(CAPTURE_HANDLE *ptDev) {
  bool b_mute = false;
  (void)pthread_mutex_lock(&g_cap_mutex);
  b_mute = ithAVSync_MuteDetect(ptDev->cap_id);
  (void)pthread_mutex_unlock(&g_cap_mutex);
  return b_mute;
}

//=============================================================================
/**
 * @brief power up cap controler.
 * @param none.
 * @return none.
 */
//=============================================================================
void iteCapPowerUp(void) {
  (void)pthread_mutex_lock(&g_cap_mutex);
  ithCap_Set_Reset();
  ithCapEnableClock();
  (void)pthread_mutex_unlock(&g_cap_mutex);
}

//=============================================================================
/**
 * @brief power down cap controler.
 * @param none.
 * @return none.
 */
//=============================================================================
void iteCapPowerDown(void) {
  uint32_t index = 0;
  int32_t result = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);
  for (index = 0; index < CAP_DEVICE_ID_MAX; index++) {
    if (ithCap_Get_IsFire((CAPTURE_DEV_ID)index)) {
      ithCap_Set_UnFire((CAPTURE_DEV_ID)index);
      result = ithCap_Get_WaitEngineIdle((CAPTURE_DEV_ID)index);
      if (result != 0) {
        (void)printf(" err 0x%x !\n", result);
        goto end;
      }
    }
    ithCap_Set_Clean_Intr((CAPTURE_DEV_ID)index);
  }
  ithCapDisableClock();
end:
  (void)pthread_mutex_unlock(&g_cap_mutex);
  if (result != 0) {
    (void)printf(" %s() err 0x%x !\n", __FUNCTION__, result);
  }
}

//=============================================================================
/**
 * @brief Cap Get Detected Width
 * @param *ptDev,points to capture_handle structure.
 * @return Detected Width.
 */
//=============================================================================
uint32_t iteCapGetDetectedWidth(CAPTURE_HANDLE *ptDev) {
  uint32_t width = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);
  width = ithCap_Get_Detected_Width(ptDev->cap_id);
  (void)pthread_mutex_unlock(&g_cap_mutex);
  return width;
}

//=============================================================================
/**
 * @brief Cap Get Detected Height
 * @param *ptDev,points to capture_handle structure.
 * @return Detected Height.
 */
//=============================================================================
uint32_t iteCapGetDetectedHeight(CAPTURE_HANDLE *ptDev) {
  uint32_t height = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);
  height = ithCap_Get_Detected_Height(ptDev->cap_id);
  (void)pthread_mutex_unlock(&g_cap_mutex);
  return height;
}

//=============================================================================
/**
 * @brief Cap Get Detected Interleave
 * @param *ptDev,points to capture_handle structure.
 * @return Detected Interleave.
 */
//=============================================================================
uint32_t iteCapGetDetectedInterleave(CAPTURE_HANDLE *ptDev) {
  uint32_t interlaced = 0;
  (void)pthread_mutex_lock(&g_cap_mutex);
  interlaced = ithCap_Get_Detected_Interleave(ptDev->cap_id);
  (void)pthread_mutex_unlock(&g_cap_mutex);
  return interlaced;
}
