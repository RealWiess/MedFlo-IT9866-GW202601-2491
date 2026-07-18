/** @file
 * PAL RISCV2 CPU functions.
 *
 * @author Steven Hsiao
 * @version 1.0
 * @date 2018/09/26
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
#include <errno.h>
#include "openrtos/FreeRTOS.h"
#include "riscv2/riscv2_device.h"


#ifdef CFG_RISCV2_TEST_DEVICE
extern ITPDevice itpDeviceRiscv2Test;
#endif

typedef struct
{
    int         engineMode;
    ITPDevice*  ptDevice;
} RISCV2_ENGINE;

static pthread_mutex_t gRiscv2Mutex  = PTHREAD_MUTEX_INITIALIZER;
static RISCV2_ENGINE gtCurDevice = { 0 };

static RISCV2_ENGINE gptRiscv2EngineArray[] =
{
#ifdef CFG_RISCV2_TEST_DEVICE
    {RISCV2_TEST_DEVICE, &itpDeviceRiscv2Test},
#endif

};

static int riscv2Ioctl(int file, unsigned long request, void *ptr, void *info)
{
    int i = 0;
    int newEngineMode = 0;
    ITPDevice *pNewDevice = NULL;
    int ret = 0;

    pthread_mutex_lock(&gRiscv2Mutex);
    switch (request)
    {
    	case ITP_IOCTL_RISCV2_SWITCH_ENG:
    	    //Engine is running, therefore, need to reload new engine image.
    	    newEngineMode = *(int*)ptr;
            if (gtCurDevice.engineMode == RISCV2_UNKNOWN_DEVICE || gtCurDevice.engineMode != newEngineMode)
            {
                iteRiscResetCpu(RISCV_2);
                for (i = 0; i < sizeof(gptRiscv2EngineArray) / sizeof(RISCV2_ENGINE); i++)
                {
                    if (newEngineMode == gptRiscv2EngineArray[i].engineMode)
                    {
                        pNewDevice = gptRiscv2EngineArray[i].ptDevice;
                        gtCurDevice.ptDevice = pNewDevice;
                        break;
                    }
                }

                if (i == sizeof(gptRiscv2EngineArray) / sizeof(RISCV2_ENGINE))
                {
                    (void)printf("itp_riscv2.c(%d), requested RISCV2 engine is not exited\n", __LINE__);
                }
            }
    		break;
        default:
            ret = gtCurDevice.ptDevice->ioctl(file, request, ptr, info);
            break;
    }
    pthread_mutex_unlock(&gRiscv2Mutex);
    return ret;
}

static int riscv2Read(int file, char *ptr, int len, void* info)
{
    int ret = 0;
    if (gtCurDevice.ptDevice)
    {
        pthread_mutex_lock(&gRiscv2Mutex);
        ret =  gtCurDevice.ptDevice->read(file, ptr, len, info);
        pthread_mutex_unlock(&gRiscv2Mutex);
    }
    return ret;
}

static int riscv2Write(int file, char *ptr, int len, void* info)
{
    int ret = 0;
    if (gtCurDevice.ptDevice)
    {
        pthread_mutex_lock(&gRiscv2Mutex);
        ret =  gtCurDevice.ptDevice->write(file, ptr, len, info);
        pthread_mutex_unlock(&gRiscv2Mutex);
    }
    return ret;
}

static int riscv2Seek(int file, int ptr, int dir, void *info)
{
    int ret = 0;

    if (gtCurDevice.ptDevice)
    {
        pthread_mutex_lock(&gRiscv2Mutex);
        ret =  gtCurDevice.ptDevice->lseek(file, ptr, dir, info);
        pthread_mutex_unlock(&gRiscv2Mutex);
    }
    return ret;
}

const ITPDevice itpDeviceRiscv2 =
{
    ":riscv2",
    itpOpenDefault,
    itpCloseDefault,
    riscv2Read,
    riscv2Write,
    riscv2Seek,
    riscv2Ioctl,
    NULL
};
