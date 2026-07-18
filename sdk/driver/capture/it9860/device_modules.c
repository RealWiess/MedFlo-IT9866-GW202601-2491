#include "capture/video_device_table.h"
#include "capture_config.h"
#include "ite_capture.h"
#include "sensor/mmp_sensor.h"
#include <string.h>

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
static SensorDriver md;
static uint32_t g_sensor_res = 0xFF;
/* devices mutex protect */
pthread_mutex_t g_device_mutex = PTHREAD_MUTEX_INITIALIZER;

//=============================================================================
//                Public Function Definition
//=============================================================================
//=============================================================================
/**
 * @brief Sets all device outputs to Tri-State (high impedance).
 * @param  none.
 * @return  none.
 */
//=============================================================================

void iteCapDeviceAllDeviceTriState(void) {
  (void)pthread_mutex_lock(&g_device_mutex);
#ifdef CFG_SENSOR_ENABLE
  // Tri-State Device
  ithSensorOutputPinTriState(md, true);
#endif
  (void)pthread_mutex_unlock(&g_device_mutex);
}

//=============================================================================
/**
 * @brief   Set up the sensor, turn on the power & initialize.
 * @param   none.
 * @return  int32_t,init success or fail
 */
//=============================================================================

int32_t iteCapDeviceInitialize(void) {

  (void)pthread_mutex_lock(&g_device_mutex);

  int32_t result = 0;
  #ifdef CFG_SENSOR_ENABLE
  unsigned char name[] = {CFG_CAPTURE_SENSOR_NAME};
  #if 0
  (void)printf("device name = %s \n",name);
  #endif
  md = ithSensorCreate(name);
  if(md != NULL)
  {
      ithSensorPowerDown(md, false);
      ithSensorInit(md, 0);
      g_sensor_res = 0xFF;
  }
  else
  {
      result = -1;
  }
  #endif
  if (result != 0) {
      (void)printf("%s error \n", __FUNCTION__);
  }
  
  (void)pthread_mutex_unlock(&g_device_mutex);
  return result;
}

//=============================================================================
/**
 * @brief   Deinitialize the sensor and turn off the power.
 * @param   none.
 * @return  none.
 */
//=============================================================================

void iteCapDeviceTerminate(void) {
  (void)pthread_mutex_lock(&g_device_mutex);
#ifdef CFG_SENSOR_ENABLE
  ithSensorOutputPinTriState(md, true);
  ithSensorDeInit(md);
  ithSensorPowerDown(md, true);
  ithSensorDestroy(md);
  md = (SensorDriver)NULL;
#endif
  (void)pthread_mutex_unlock(&g_device_mutex);
}

//=============================================================================
/**
 * @brief   Get the sensor signal stable or not.
 * @param   none.
 * @return  bool, true is stable.
 */
//=============================================================================

bool iteCapDeviceIsSignalStable(void) {
  bool b_signal_stable = false;
  (void)pthread_mutex_lock(&g_device_mutex);
#ifdef CFG_SENSOR_ENABLE
  b_signal_stable = (bool)ithSensorIsSignalStable(md, 0);
#endif
  (void)pthread_mutex_unlock(&g_device_mutex);
  return b_signal_stable;
}

//=============================================================================
/**
 * @brief   Allow the capture engine to obtain sensor-related information.
 * @param   *ptDev,points to capture_handle structure.
 * @return  none.
 */
//=============================================================================

void iteCapGetDeviceInfo(CAPTURE_HANDLE *ptDev) {
  (void)pthread_mutex_lock(&g_device_mutex);
#ifdef CFG_SENSOR_ENABLE
  uint32_t i = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t framerate = 0;
  uint32_t interlanced = 0;

  width = (uint32_t)ithSensorGetProperty(md, GetWidth);
  height = (uint32_t)ithSensorGetProperty(md, GetHeight);
  framerate = (uint32_t)ithSensorGetProperty(md, Rate);
  interlanced = (uint32_t)ithSensorGetProperty(md, GetModuleIsInterlace);

#if 0 /*if source is RGB, open Color Convert!*/
  iteCapSetColorConver(ptDev, true, RGBTOYUV_ITU601_0_255);
#endif

  for (i = 0; i < ITH_COUNT_OF(VIDEO_TABLE); i++) {
    if ((width == VIDEO_TABLE[i].HActive) &&
        (height == VIDEO_TABLE[i].VActive) &&
        (framerate == VIDEO_TABLE[i].FrameRate)) {

      if (iteCapSetInputSize(ptDev, width, height) != 0) {
        (void)printf("error\n");
      }
      if (iteCapSetInterleave(ptDev, interlanced) != 0) {
        (void)printf("error\n");
      }
      /* Set input ROI info */
      if (iteCapSetRoi(ptDev, VIDEO_TABLE[i].ROIPosX, VIDEO_TABLE[i].ROIPosY,
                       VIDEO_TABLE[i].ROIWidth,
                       VIDEO_TABLE[i].ROIHeight) != 0) {
        (void)printf("error\n");
      }

      /* Set output width*/
      if (iteCapSetOutputSize(ptDev, VIDEO_TABLE[i].ROIWidth) != 0) {
        (void)printf("error\n");
      }
      /* Set output pitch*/
      if (iteCapSetOutputPitch(ptDev, width, width) != 0) {
        (void)printf("error\n");
      }
      
#if 0
      /*no drop frame*/
      iteCapSetDrop(ptDev, 0xF, 0x3);
#endif 

      (void)printf("W/H(%u/%u), ROI(%u/%u)\n", width, height,
                   VIDEO_TABLE[i].ROIWidth, VIDEO_TABLE[i].ROIHeight);

      g_sensor_res = i;
      break;
    }
  }

#endif
  (void)pthread_mutex_unlock(&g_device_mutex);
}

//=============================================================================
/**
 * @brief   Allow the user to obtain sensor-related information.
 * @param   option, Information type.
 * @return  uint32_t, Information value.
 */
//=============================================================================

uint32_t iteCapDeviceGetProperty(DEVICESPROPERTYS option) {
  uint32_t info = 0;
  (void)pthread_mutex_lock(&g_device_mutex);
  switch (option) {
  case DEVICES_TABLEINDEX:
    info = g_sensor_res;
    break;
#ifdef CFG_SENSOR_ENABLE
  case DEVICES_WIDTH:
    info = (uint32_t)ithSensorGetProperty(md, GetWidth);
    break;
  case DEVICES_HEIGHT:
    info = (uint32_t)ithSensorGetProperty(md, GetHeight);
    break;
  case DEVICES_ISINTERLANCED:
    info = (uint32_t)ithSensorGetProperty(md, GetModuleIsInterlace);
    break;
  case DEVICES_FRAMETRATE:
    info = (uint32_t)ithSensorGetProperty(md, Rate);
    break;
  case DEVICES_IS_DECODER:
    info = (uint32_t)ithSensorGetProperty(md, GetIsAnalogDecoder);
    break;
#endif
  default:
    break;
  }
  (void)pthread_mutex_unlock(&g_device_mutex);

  return info;
}

//=============================================================================
/**
 * @brief   Control the LED light that comes with the sensor.
 * @param   enable, trun on led or not.
 * @return  none.
 */
//=============================================================================

void iteCapDeviceLEDON(bool enable) {
  (void)pthread_mutex_lock(&g_device_mutex);
#ifdef CFG_SENSOR_ENABLE
  if (enable) {
    ithSensorSetProperty(md, LEDON, 0);
  } else {
    ithSensorSetProperty(md, LEDOFF, 0);
  }
#endif
  (void)pthread_mutex_unlock(&g_device_mutex);
}

//=============================================================================
/**
 * @brief   Control the specified channel state of multi-channel sensor hardware.
 * @param   ch, channel id.
 * @return  none.
 */
//=============================================================================

void iteCapDeviceCHSwitch(uint32_t ch) {
  (void)pthread_mutex_lock(&g_device_mutex);
#ifdef CFG_SENSOR_ENABLE
  ithSensorSetProperty(md, VideoInCH, ch);
#endif
  (void)pthread_mutex_unlock(&g_device_mutex);
}
