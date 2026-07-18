#ifndef CAPTURE_HW_H
#define CAPTURE_HW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "capture_types.h"
#include "ite_capture.h"
#include "ith/ith_defs.h"


//=============================================================================
//                Constant Definition
//=============================================================================
#define B_CAP_MEM_MODE (0x1UL)
#define B_CAP_ONFLY_MODE (0x1UL << 1U)
#define B_CAP_ERR_RST_STOP_ENGINE (0x1UL << 16U)

#define B_CAP_GET_INTR (0x1UL << 31U)
#define B_CAP_GET_INTR_FROM_DEV1 (0x1UL << 28U)

/* color bar */
#define B_CAP_COLOR_BAR_ROLLING_EN (0x1UL << 31U)
#define B_CAP_COLOR_BAR_VSPOL (0x1UL << 17U)
#define B_CAP_COLOR_BAR_HSPOL (0x1UL << 16U)

/* input data format */
#define CAP_IN_DATA_FORMAT_YUV422 (0x1UL << 12U)

#define CAP_PROGRESSIVE 0
#define CAP_INTERLEVING 1
#define CAP_BT_601 0
#define CAP_BT_656 1

#define CAP_ENCSFUN (0x1UL << 24U)

/* Capture interrupt mode */
#define CAP_INT_MODE_FRAME_END 0x0001UL
#define CAP_INT_MODE_SYNC_ERR 0x0010UL
#define CAP_INT_MODE_DCLK_PHASE_DRIFTED 0x0100UL
#define CAP_INT_MODE_MUTE_DETECT 0x1000UL

#define CAP_BIT_SCALEWEIGHT 0x00FFUL // 0000 0000 1111 1111

#define CAP_SHT_SCALEWEIGHT_H 8U
#define CAP_SHT_SCALEWEIGHT_L 0U

#define CAP_BIT_RGB_TO_YUV 0x07FFUL       // 0000 0111 1111 1111
#define CAP_BIT_RGB_TO_YUV_CONST 0x03FFUL // 0000 0011 1111 1111

#define AUDIO_COUNTER_CLEAR 0x0001U
#define VIDEO_COUNTER_CLEAR 0x0002U
#define MUTE_COUNTER_CLEAR 0x0004U
#define AUDIO_COUNTER_LATCH 0x0008U
#define VIDEO_COUNTER_LATCH 0x0008U
#define MUTE_COUNTER_LATCH 0x0010U
#define AUDIO_COUNTER_SEL 0x0020U
#define VIDEO_COUNTER_SEL 0x0040U
#define MUTE_COUNTER_SEL 0x0080U
#define MUTEPRE_COUNTER_SEL 0x0400U

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
//                Public Function Definition
//=============================================================================
void ithCapEnableClock(void);

void ithCapDisableClock(void);

void ithCap_Set_Fire(CAPTURE_DEV_ID DEV_ID);

void ithCap_Set_UnFire(CAPTURE_DEV_ID DEV_ID);

void ithCap_Set_Reset(void);

void ithCap_Set_Register_Reset(void);

void ithCap_Set_AutoDelayHWFlow(CAPTURE_DEV_ID DEV_ID);

void ithCap_Set_ErrReset(CAPTURE_DEV_ID DEV_ID);

void ithCap_Set_TurnOnClock_Reg(CAPTURE_DEV_ID DEV_ID, bool flag);

int32_t ithCap_Set_Input_Pin_Mux_Reg(CAPTURE_DEV_ID DEV_ID,
                                     CAP_INPUT_MUX_INFO *pininfo);

void ithCap_Set_Color_Format_Reg(CAPTURE_DEV_ID DEV_ID, CAP_YUV_INFO *pYUVinfo);

int32_t ithCap_Set_IO_Mode_Reg(CAPTURE_DEV_ID DEV_ID,
                               CAP_IO_MODE_INFO *io_config);

void ithCap_Set_Internal_Reg(CAPTURE_DEV_ID DEV_ID, CAP_INPUT_INFO *pIninfo);

void ithCap_Set_NonDEMode_Reg(CAPTURE_DEV_ID DEV_ID, CAP_INPUT_INFO *pIninfo);

void ithCap_Set_Output_Width_Reg(CAPTURE_DEV_ID DEV_ID,
                                 CAP_OUTPUT_INFO *pOutInfo);

void ithCap_Set_Output_Reg(CAPTURE_DEV_ID DEV_ID, CAP_OUTPUT_INFO *pOutInfo);

void ithCap_Set_Skip_Pattern_Reg(CAPTURE_DEV_ID DEV_ID, uint32_t pattern,
                                 uint32_t period);
void ithCap_Set_Hsync_Polarity(CAPTURE_DEV_ID DEV_ID, uint32_t Hsync);
void ithCap_Set_Vsync_Polarity(CAPTURE_DEV_ID DEV_ID, uint32_t Vsync);

void ithCap_Set_Interrupt_Mode(CAPTURE_DEV_ID DEV_ID, uint32_t Intr_Mode,
                               bool flag);

void ithCap_Set_Color_Bar(CAPTURE_DEV_ID DEV_ID,
                          CAP_COLOR_BAR_CONFIG color_info);

void ithCap_Set_Interleave(CAPTURE_DEV_ID DEV_ID, uint32_t Interleave);

void ithCap_Set_Width_Height(CAPTURE_DEV_ID DEV_ID, uint32_t width,
                             uint32_t height);

void ithCap_Set_ROI_Size(CAPTURE_DEV_ID DEV_ID, uint32_t x, uint32_t y,
                         uint32_t width, uint32_t height);

void ithCap_Set_Clean_Intr(CAPTURE_DEV_ID DEV_ID);

void ithCap_Set_Enable_Reg(CAPTURE_DEV_ID DEV_ID, CAP_ENFUN_INFO *pFunEn);

void ithCap_Set_Enable_Dither(CAPTURE_DEV_ID DEV_ID,
                              CAP_INPUT_DITHER_INFO *pDitherinfo);

int32_t ithCap_Set_ISP_HandShaking(CAPTURE_DEV_ID DEV_ID,
                                   CAP_ISP_HANDSHAKING_MODE handshakmode,
                                   CAP_OUTPUT_INFO *pOutInfo);

void ithCap_Set_Error_Handleing(CAPTURE_DEV_ID DEV_ID, uint32_t errDetectEn);

void ithCap_Set_Wait_Error_Reset(CAPTURE_DEV_ID DEV_ID);

void ithCap_Set_Memory_AddressLimit_Reg(CAPTURE_DEV_ID DEV_ID,
                                        uint32_t memUpBound,
                                        uint32_t memLoBound);

void ithCap_Set_Buffer_addr_Reg(CAPTURE_DEV_ID DEV_ID, uint32_t *pYAddr,
                                uint32_t *pUVAddr, uint32_t addrOffset);

void ithCap_Set_ScaleParam_Reg(CAPTURE_DEV_ID DEV_ID,
                               const CAP_SCALE_CTRL *pScaleFun);

void ithCap_Set_IntScaleMatrixH_Reg(CAPTURE_DEV_ID DEV_ID,
                                    uint32_t WeightMatX[][CAP_SCALE_TAP]);

void ithCap_Set_RGBtoYUVMatrix_Reg(CAPTURE_DEV_ID DEV_ID,
                                   const CAP_RGB_TO_YUV *pRGBtoYUV);

void ithCap_Set_MemThreshold(CAPTURE_DEV_ID DEV_ID, uint32_t threshold);

void ithCap_Set_VideoSignalSource(CAPTURE_DEV_ID DEV_ID, uint8_t source);

//=============================================================================
/**
 * Audio/Video/Mute Counter control function
 */
//=============================================================================
void ithAVSync_CounterCtrl(CAPTURE_DEV_ID DEV_ID, uint32_t ctlmode,
                           uint32_t divider);

void ithAVSync_CounterReset(CAPTURE_DEV_ID DEV_ID, uint32_t resetmode);

uint32_t ithAVSync_CounterRead(CAPTURE_DEV_ID DEV_ID, uint32_t readmode);

bool ithAVSync_MuteDetect(CAPTURE_DEV_ID DEV_ID);

//=============================================================================
/**
 *                Public Get Function Definition
 */
//=============================================================================

bool ithCap_Get_IsFire(CAPTURE_DEV_ID DEV_ID);

int32_t ithCap_Get_WaitEngineIdle(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Lane_status(CAPTURE_DEV_ID DEV_ID, CAP_LANE_STATUS lanenum);

uint32_t ithCap_Get_Hsync_Polarity(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Vsync_Polarity(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Hsync_Polarity_Status(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Vsync_Polarity_Status(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Revision(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_MRawVTotal(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Detected_Width(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Detected_Height(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Detected_Interleave(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_Error_Status(CAPTURE_DEV_ID DEV_ID);

uint32_t ithCap_Get_FIFOMAX(CAPTURE_DEV_ID DEV_ID, bool ISFIFO_Y);

void ithCap_Dump_Reg(CAPTURE_DEV_ID DEV_ID);

#ifdef __cplusplus
}
#endif

#endif
