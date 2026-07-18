/** @file
 * PAL ARMLite CPU functions.
 *
 * @author Kevin Chen
 * @version 1.0
 * @date 2019/10/01
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
#include <errno.h>
#include <stdio.h>
#include "openrtos/FreeRTOS.h"
#include "arm_lite_dev/armlite_dev_device.h"


#ifdef CFG_ARMLITE_OPUS_CODEC
extern ITPDevice itpDeviceOpusCodec;
#endif

#ifdef CFG_ARMLITE_HUD_BG_FAST
extern ITPDevice itpDeviceHudBgFast;
#endif

#ifdef CFG_ARMLITE_CANBUS
extern ITPDevice itpDeviceCAN1;
#endif

#ifdef CFG_ARMLITE_LCD_CRC_CAL
extern ITPDevice itpDeviceLcdCrcCal;
#endif

typedef struct
{
    int         engineMode;
    ITPDevice*  ptDevice;
} ARMLITE_ENGINE;

static pthread_mutex_t gArmLiteMutex  = PTHREAD_MUTEX_INITIALIZER;
static ARMLITE_ENGINE gtCurDevice = { 0 };

static ARMLITE_ENGINE gptArmLiteEngineArray[] =
{
#ifdef CFG_ARMLITE_OPUS_CODEC
    {ARMLITE_OPUS_CODEC, &itpDeviceOpusCodec},
#endif

#ifdef CFG_ARMLITE_HUD_BG_FAST
    {ARMLITE_HUD_BG_FAST, &itpDeviceHudBgFast},
#endif

#ifdef CFG_ARMLITE_CANBUS
    {ARMLITE_CANBUS, &itpDeviceCAN1},
#endif

#ifdef CFG_ARMLITE_LCD_CRC_CAL
    {ARMLITE_LCD_CRC_CAL, &itpDeviceLcdCrcCal},
#endif

};

static int armLiteIoctl(int file, unsigned long request, void *ptr, void *info)
{
    uint32_t i = 0;
    int newEngineMode = 0;
    ITPDevice *pNewDevice = NULL;
    int ret = 0;
    
    (void)pthread_mutex_lock(&gArmLiteMutex);
    switch (request)
    {
    	case ITP_IOCTL_ARMLITE_SWITCH_ENG:
    	    //Engine is running, therefore, need to reload new engine image.
    	    newEngineMode = *(int*)ptr;
            if (gtCurDevice.engineMode == ARMLITE_UNKNOWN_DEVICE || gtCurDevice.engineMode != newEngineMode)
            {
                iteRiscResetCpu(ARMLITE_CPU);
                for (i = 0; i < sizeof(gptArmLiteEngineArray) / sizeof(ARMLITE_ENGINE); i++)
                {
                    if (newEngineMode == gptArmLiteEngineArray[i].engineMode)
                    {
                        pNewDevice = gptArmLiteEngineArray[i].ptDevice;
                        gtCurDevice.ptDevice = pNewDevice;
                        break;
                    }
                }

                if (i == sizeof(gptArmLiteEngineArray) / sizeof(ARMLITE_ENGINE))
                {
                    (void)printf("itp_armLiteDev.c(%d), requested ARMLite engine is not exited\n", __LINE__);
                }
            }
    		break;
        default:
            if (gtCurDevice.ptDevice != NULL)
            {
                ret = gtCurDevice.ptDevice->ioctl(file, request, ptr, info);
            }
            break;
    }    
    (void)pthread_mutex_unlock(&gArmLiteMutex);
    return ret;
}

static int armLiteRead(int file, char *ptr, int len, void* info)
{
    int ret = 0;
    (void)pthread_mutex_lock(&gArmLiteMutex);
    if (gtCurDevice.ptDevice != NULL)
    {
        ret =  gtCurDevice.ptDevice->read(file, ptr, len, info);
    }
    (void)pthread_mutex_unlock(&gArmLiteMutex);
    return ret;
}

static int armLiteWrite(int file, char *ptr, int len, void* info)
{
    int ret = 0;
    (void)pthread_mutex_lock(&gArmLiteMutex);
    if (gtCurDevice.ptDevice != NULL)
    {
        ret =  gtCurDevice.ptDevice->write(file, ptr, len, info);
    }
    (void)pthread_mutex_unlock(&gArmLiteMutex);
    return ret;
}

static int armLiteSeek(int file, int ptr, int dir, void *info)
{
    int ret = 0;
    (void)pthread_mutex_lock(&gArmLiteMutex);
    if (gtCurDevice.ptDevice != NULL)
    {
        ret =  gtCurDevice.ptDevice->lseek(file, ptr, dir, info);
    }
    (void)pthread_mutex_unlock(&gArmLiteMutex);
    return ret;
}

const ITPDevice itpDeviceArmLite =
{
    ":armLite",
    itpOpenDefault,
    itpCloseDefault,
    armLiteRead,
    armLiteWrite,
    armLiteSeek,
    armLiteIoctl,
    NULL
};
