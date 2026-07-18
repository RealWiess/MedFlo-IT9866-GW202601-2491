#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "capture/capture_9860/ite_capture.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define SIGNALCHECK_TIMEOUT 100 // 100*10ms
#define SENSORCHECK_TIMEOUT 300 // 300*10ms

CAPTURE_HANDLE        gCapDev0    = {0};
/*memory mode max : 1080P*/
static int            g_maxwidth  = 1920;
static int            g_maxheight = 1080;
static unsigned short gFramerate  = 0;
static unsigned short ginterlaced = 0;
static QueueHandle_t  gCapQueue;
static int            gInputCH = 0;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void           cap_isr0 (void * arg)
{
    uint32_t   capture0state            = 0;
    uint32_t   capwrindex               = 0;
    BaseType_t gHigherPriorityTaskWoken = (BaseType_t)0;

    capture0state = iteCapGetEngineErrorStatus(&gCapDev0, MMP_CAP_LANE0_STATUS);

    if (capture0state >> 31)
    {
        if ((capture0state >> 8) & 0xF)
        {
            (void)ithPrintf("[Error]ErrorCode = 0x%x\n",
                            (capture0state >> 8) & 0xF);
            // clear cap0 interrupt and reset error status
            iteCapClearInterrupt(&gCapDev0, true);
        }
        else
        {
            // clear cap0 interrupt
            iteCapClearInterrupt(&gCapDev0, false);
            capwrindex = iteCapReturnWrBufIndex(&gCapDev0);
            xQueueSendToBackFromISR(gCapQueue, &capwrindex,
                                    &gHigherPriorityTaskWoken);
        }
    }
    portYIELD_FROM_ISR(gHigherPriorityTaskWoken);
}

static int Signal_Check (CAPTURE_HANDLE * ptDev)
{
    int timeout = 0;
    while ((iteCapGetEngineErrorStatus(ptDev, MMP_CAP_LANE0_STATUS) & 0xe) !=
           0xe)
    {
        if (++timeout > SIGNALCHECK_TIMEOUT)
        {
            (void)printf("Wait Capture Lock timeout\n");
            return 1;
        }
        DEBUG_PRINT("Hsync or Vsync not stable!\n");
        (void)usleep(10000);
    }

    iteCapFire(ptDev, true);
    DEBUG_PRINT("Capture Fire! (%d)\n", ptDev->cap_id);

    return 0;
}

static int SensorSignal_Check (void)
{
    int timeout = 0;
    while (iteCapDeviceIsSignalStable() != true)
    {
        // printf("Sensor not stable!\n");
        if (++timeout > SENSORCHECK_TIMEOUT)
        {
            printf("Wait Sensor stable timeout\n");
            return 1;
        }
        usleep(10000);
    }
    return 0;
}

static void f_capture_init (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    /*capture clk on*/
    iteCapPowerUp();
    /*capture init*/
    iteCapInitialize();

    gCapQueue = xQueueCreate(20, (unsigned portBASE_TYPE)sizeof(uint32_t));
}

static void f_capture_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    iteCapPowerDown();
#if defined(CFG_SENSOR_ENABLE)
    iteCapDeviceLEDON(0);
    iteCapDeviceTerminate();
#endif
    vQueueDelete(gCapQueue);
}

static void f_capture_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    IteQueueblk       blk            = {0};
    IteSensorStream * output         = NULL;
    uint32_t          capturestate   = 0;
    uint32_t          rec_wrindex    = 0;

    CAPTURE_SETTING   mem_analog_set = {MMP_CAP_DEV_ANALOG_DECODER, false, true,
                                        g_maxwidth, g_maxheight};
    CAPTURE_SETTING   mem_digital_set = {MMP_CAP_DEV_SENSOR, false, true,
                                         g_maxwidth, g_maxheight};

    if (g_maxwidth != 0 && g_maxheight != 0)
    {
        if (iteCapDeviceGetProperty(DEVICES_IS_DECODER) == 1)
        {
            iteCapConnect(&gCapDev0, mem_analog_set);
        }
        else
        {
            iteCapConnect(&gCapDev0, mem_digital_set);
        }

        iteCapRegisterIRQ(cap_isr0, &gCapDev0);
    }
    else
    {
        DEBUG_PRINT("[%s] Filter(%d) Width or Height Error \n", __FUNCTION__,
                    f->filterDes.id);
    }

    while (f->run)
    {

        if (iteCapDeviceIsSignalStable())
        {
            if (iteCapIsFire(&gCapDev0) == false)
            {
                iteCapGetDeviceInfo(&gCapDev0);
                iteCapParameterSetting(&gCapDev0);
                Signal_Check(&gCapDev0);
            }
        }

        if (xQueueReceive(gCapQueue, &rec_wrindex, 0))
        {
            // printf("rec_wrindex = %d\n",rec_wrindex);
            output              = malloc(sizeof(IteSensorStream));
            output->Width       = gCapDev0.cap_info.outinfo.width;
            output->Height      = gCapDev0.cap_info.outinfo.height;
            output->Interlanced = gCapDev0.cap_info.ininfo.Interleave;
            output->DataAddrY   = gCapDev0.cap_info.out_addr_y[rec_wrindex];
            output->DataAddrU   = gCapDev0.cap_info.out_addr_uv[rec_wrindex];
            output->DataAddrV   = gCapDev0.cap_info.out_addr_uv[rec_wrindex];
            output->PitchY      = gCapDev0.cap_info.outinfo.pitch_y;
            output->PitchUV     = gCapDev0.cap_info.outinfo.pitch_uv;

            blk.private1        = (void *)output;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        usleep(16 * 1000);
    }
    iteCapTerminate();
    iteCapDisConnect(&gCapDev0);
}

static void f_cap_getframerate (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SensorSignal_Check();

    gFramerate  = iteCapDeviceGetProperty(DEVICES_FRAMETRATE);
    ginterlaced = iteCapDeviceGetProperty(DEVICES_ISINTERLANCED);

    switch (gFramerate)
    {
        case 2500:
            gFramerate = MMP_CAP_FRAMERATE_25HZ;
            break;
        case 3000:
            gFramerate = MMP_CAP_FRAMERATE_30HZ;
            break;
        case 5000:
            if (ginterlaced)
            {
                gFramerate = MMP_CAP_FRAMERATE_25HZ;
            }
            else
            {
                gFramerate = MMP_CAP_FRAMERATE_50HZ;
            }
            break;
        case 5994:
        case 6000:
            if (ginterlaced)
            {
                gFramerate = MMP_CAP_FRAMERATE_30HZ;
            }
            else
            {
                gFramerate = MMP_CAP_FRAMERATE_60HZ;
            }
            break;
        default:
            gFramerate = MMP_CAP_FRAMERATE_25HZ;
            (void)printf("[Error]unknow frame rate!\n");
            break;
    }

    *(unsigned short *)arg = gFramerate;
#if 0
	printf("gFramerate = %x  addr = %x\n", gFramerate , *(unsigned short*)arg);
#endif
}

static void f_cap_getinterlanced (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SensorSignal_Check();

    ginterlaced            = iteCapDeviceGetProperty(DEVICES_ISINTERLANCED);

    *(unsigned short *)arg = ginterlaced;
}

static void f_cap_sensor_init (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    gInputCH = (int)arg;
#if defined(CFG_SENSOR_ENABLE)
    /*Sensor init*/
    iteCapDeviceInitialize();
    iteCapDeviceCHSwitch((uint32_t)gInputCH);
    iteCapDeviceLEDON(1);
#endif
}

static void f_cap_getsensorstable (IteFilter * f, void * arg)
{
    if (iteCapDeviceIsSignalStable())
    {
        *(bool *)arg = true;
    }
    else
    {
        *(bool *)arg = false;
    }
}

static IteMethodDes Filter_CAP_methods[] = {
    {   ITE_FILTER_CAP_GETFRAMERATE,    f_cap_getframerate},
    { ITE_FILTER_CAP_GETINTERLANCED,  f_cap_getinterlanced},
    {  ITE_FILTER_CAP_SETSENSORINIT,     f_cap_sensor_init},
    {ITE_FILTER_CAP_GETSENSORSTABLE, f_cap_getsensorstable},
};

// clang-format off
IteFilterDes        FilterCapture = {
    ITE_FILTER_CAP_ID,
    f_capture_init,
    f_capture_uninit,
    f_capture_process,
    Filter_CAP_methods
};
// clang-format on
