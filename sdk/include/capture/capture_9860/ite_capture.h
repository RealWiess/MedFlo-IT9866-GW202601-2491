#ifndef ITE_CAPTURE_H
#define ITE_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "capture_types.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define CAP_API extern

//=============================================================================
//                              Constant Definition
//=============================================================================
/**
 *  Device Select
 */
typedef enum MMP_CAP_DEVICE_ID_TAG {
  MMP_CAP_UNKNOW_DEVICE = 0,
  MMP_CAP_DEV_SENSOR,
  MMP_CAP_DEV_ANALOG_DECODER,
  MMP_CAP_DEV_MAX,
} MMP_CAP_DEVICE_ID;

typedef enum DEVICESPROPERTYS_TAG {
  DEVICES_WIDTH = 0,
  DEVICES_HEIGHT,
  DEVICES_FRAMETRATE,
  DEVICES_ISINTERLANCED,
  DEVICES_TABLEINDEX,
  DEVICES_IS_DECODER,
} DEVICESPROPERTYS;

typedef enum MMP_CAP_LANE_STATUS_TAG {
  MMP_CAP_LANE0_STATUS,
} MMP_CAP_LANE_STATUS;

typedef enum MMP_CAP_FRAMERATE_TAG {
  MMP_CAP_FRAMERATE_UNKNOW = 0,
  MMP_CAP_FRAMERATE_25HZ,
  MMP_CAP_FRAMERATE_50HZ,
  MMP_CAP_FRAMERATE_30HZ,
  MMP_CAP_FRAMERATE_60HZ,
  MMP_CAP_FRAMERATE_29_97HZ,
  MMP_CAP_FRAMERATE_59_94HZ,
  MMP_CAP_FRAMERATE_23_97HZ,
  MMP_CAP_FRAMERATE_24HZ,
  MMP_CAP_FRAMERATE_56HZ,
  MMP_CAP_FRAMERATE_70HZ,
  MMP_CAP_FRAMERATE_72HZ,
  MMP_CAP_FRAMERATE_75HZ,
  MMP_CAP_FRAMERATE_85HZ,
} MMP_CAP_FRAMERATE;

typedef enum MMP_CAP_INPUT_INFO_TAG {
  MMP_CAP_INPUT_INFO_640X480_60P = 0,
  MMP_CAP_INPUT_INFO_720X480_59I = 1,
  MMP_CAP_INPUT_INFO_720X480_59P = 2,
  MMP_CAP_INPUT_INFO_720X480_60I = 3,
  MMP_CAP_INPUT_INFO_720X480_60P = 4,
  MMP_CAP_INPUT_INFO_720X576_50I = 5,
  MMP_CAP_INPUT_INFO_720X576_50P = 6,
  MMP_CAP_INPUT_INFO_1280X720_50P = 7,
  MMP_CAP_INPUT_INFO_1280X720_59P = 8,
  MMP_CAP_INPUT_INFO_1280X720_60P = 9,
  MMP_CAP_INPUT_INFO_1920X1080_23P = 10,
  MMP_CAP_INPUT_INFO_1920X1080_24P = 11,
  MMP_CAP_INPUT_INFO_1920X1080_25P = 12,
  MMP_CAP_INPUT_INFO_1920X1080_29P = 13,
  MMP_CAP_INPUT_INFO_1920X1080_30P = 14,
  MMP_CAP_INPUT_INFO_1920X1080_50I = 15,
  MMP_CAP_INPUT_INFO_1920X1080_50P = 16,
  MMP_CAP_INPUT_INFO_1920X1080_59I = 17,
  MMP_CAP_INPUT_INFO_1920X1080_59P = 18,
  MMP_CAP_INPUT_INFO_1920X1080_60I = 19,
  MMP_CAP_INPUT_INFO_1920X1080_60P = 20,
  MMP_CAP_INPUT_INFO_800X600_60P = 21,
  MMP_CAP_INPUT_INFO_1024X768_60P = 22,
  MMP_CAP_INPUT_INFO_1280X768_60P = 23,
  MMP_CAP_INPUT_INFO_1280X800_60P = 24,
  MMP_CAP_INPUT_INFO_1280X960_60P = 25,
  MMP_CAP_INPUT_INFO_1280X1024_60P = 26,
  MMP_CAP_INPUT_INFO_1360X768_60P = 27,
  MMP_CAP_INPUT_INFO_1366X768_60P = 28,
  MMP_CAP_INPUT_INFO_1440X900_60P = 29,
  MMP_CAP_INPUT_INFO_1400X1050_60P = 30,
  MMP_CAP_INPUT_INFO_1440X1050_60P = 31,
  MMP_CAP_INPUT_INFO_1600X900_60P = 32,
  MMP_CAP_INPUT_INFO_1600X1200_60P = 33,
  MMP_CAP_INPUT_INFO_1680X1050_60P = 34,
  MMP_CAP_INPUT_INFO_ALL = 35,
  MMP_CAP_INPUT_INFO_NUM = 36,
  MMP_CAP_INPUT_INFO_UNKNOWN = 37,
  MMP_CAP_INPUT_INFO_CAMERA = 38,
} MMP_CAP_INPUT_INFO;

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef enum CAPTURE_DEV_ID_TAG {
  CAP_DEV_ID0 = 0,
} CAPTURE_DEV_ID;

typedef struct CAPTURE_HANDLE_TAG {
  CAP_CONTEXT cap_info;        // cap device info
  CAPTURE_DEV_ID cap_id;       // cap 0
  MMP_CAP_DEVICE_ID source_id; // front source id
} CAPTURE_HANDLE;

typedef struct CAPTURE_SETTING_TAG {
  MMP_CAP_DEVICE_ID inputsource;
  bool b_onflymode_en;
  bool b_Interrupt_en;
  uint32_t max_width;
  uint32_t max_height;
} CAPTURE_SETTING;

//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================
CAP_API void iteCapDeviceAllDeviceTriState(void);
CAP_API int32_t iteCapDeviceInitialize(void);
CAP_API void iteCapDeviceTerminate(void);
CAP_API bool iteCapDeviceIsSignalStable(void);
CAP_API void iteCapGetDeviceInfo(CAPTURE_HANDLE *ptDev);
CAP_API uint32_t iteCapDeviceGetProperty(DEVICESPROPERTYS option);
CAP_API void iteCapDeviceLEDON(bool enable);
CAP_API void iteCapDeviceCHSwitch(uint32_t ch);

CAP_API int32_t iteCapInitialize(void);
CAP_API int32_t iteCapConnect(CAPTURE_HANDLE *cap_handle, CAPTURE_SETTING info);
CAP_API int32_t iteCapDisConnect(CAPTURE_HANDLE *ptDev);
CAP_API int32_t iteCapTerminate(void);
CAP_API bool iteCapIsFire(CAPTURE_HANDLE *ptDev);
CAP_API uint32_t iteCapGetEngineErrorStatus(CAPTURE_HANDLE *ptDev,
                                            MMP_CAP_LANE_STATUS lanenum);
CAP_API int32_t iteCapParameterSetting(CAPTURE_HANDLE *ptDev);
CAP_API void iteCapFire(CAPTURE_HANDLE *ptDev, bool enable);
CAP_API void iteCapRegisterIRQ(ITHIntrHandler caphandler,
                               CAPTURE_HANDLE *ptDev);
CAP_API void iteCapDisableIRQ(void);
CAP_API uint32_t iteCapClearInterrupt(CAPTURE_HANDLE *ptDev, bool get_err);
CAP_API int32_t iteCapSetInputSize(CAPTURE_HANDLE *ptDev, uint32_t width,
                           uint32_t height);
CAP_API int32_t iteCapSetInterleave(CAPTURE_HANDLE *ptDev, uint32_t interleave);
CAP_API int32_t iteCapSetRoi(CAPTURE_HANDLE *ptDev, uint32_t x, uint32_t y,
                     uint32_t width, uint32_t height);
CAP_API int32_t iteCapSetOutputSize(CAPTURE_HANDLE *ptDev, uint32_t width);
CAP_API int32_t iteCapSetOutputPitch(CAPTURE_HANDLE *ptDev, uint32_t y, uint32_t uv);
CAP_API int32_t iteCapSetColorConver(CAPTURE_HANDLE *ptDev, bool cs_en,
                             CAP_COLOR_MATRIX type);
CAP_API int32_t iteCapSetDrop(CAPTURE_HANDLE *ptDev, uint32_t drop_control,
                             uint32_t drop_period);
CAP_API uint32_t iteCapReturnWrBufIndex(CAPTURE_HANDLE *ptDev);
CAP_API MMP_CAP_FRAMERATE iteCapGetInputFrameRate(CAPTURE_HANDLE *ptDev);
CAP_API void iteAVSyncCounterCtrl(CAPTURE_HANDLE *ptDev, uint32_t mode,
                                  uint32_t divider);

CAP_API void iteAVSyncCounterReset(CAPTURE_HANDLE *ptDev, uint32_t mode);
CAP_API uint32_t iteAVSyncCounterRead(CAPTURE_HANDLE *ptDev, uint32_t mode);
CAP_API bool iteAVSyncMuteDetect(CAPTURE_HANDLE *ptDev);
CAP_API void iteCapPowerUp(void);
CAP_API void iteCapPowerDown(void);
CAP_API uint32_t iteCapGetDetectedWidth(CAPTURE_HANDLE *ptDev);
CAP_API uint32_t iteCapGetDetectedHeight(CAPTURE_HANDLE *ptDev);
CAP_API uint32_t iteCapGetDetectedInterleave(CAPTURE_HANDLE *ptDev);

#ifdef __cplusplus
}
#endif

#endif