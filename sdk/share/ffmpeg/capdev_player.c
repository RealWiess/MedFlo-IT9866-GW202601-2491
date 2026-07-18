/*
 * iTE castor3 media player for camera capture
 *
 * @file:capdev_player.c
 * @version 6.0.0
 * @How to use:
 *
 *
 *  1.set play window size 
 *     itv_set_video_window(x, y, width, height);
 *  2.init
 *     mtal_pb_init(EventHandler);
 *     mtal_spec.camera_in = CAPTURE_IN;
 *     mtal_pb_select_file(&mtal_spec);
 *  3.start play
 *     mtal_pb_play();
 *  4.stop
 *     mtal_pb_stop();
 *  5.destory
 *     mtal_pb_exit();
 *
 *  6.deine setting
 *     CAPDEV_SENSOR_MAX_WIDTH , according sensor max Image resolution
 *     CAPDEV_SENSOR_MAX_HEIGHT, according sensor max Image resolution
 *     CAPDEV_MS_PER_FRAME         ,  capture thread update frame pre ms. ex: 60fps => 16ms
 */
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "ite/itp.h"
#include "ith/ith_video.h"
#include "ite/itv.h"

#ifdef __OPENRTOS__
    #include "openrtos/FreeRTOS.h"
    #include "openrtos/queue.h"
#endif
#include "file_player.h"
#include "isp/mmp_isp.h"
#include "capture/capture_9860/ite_capture.h"
#ifdef CFG_SENSOR_ENABLE
    #include "sensor/mmp_sensor.h"
#endif
///////////////////////////////////////////////////////////////////////////////////////////
// Definitions and type
///////////////////////////////////////////////////////////////////////////////////////////

#define CAPDEVQUEUESIZE          10
#define CAPDEV_SENSOR_MAX_WIDTH  1920U
#define CAPDEV_SENSOR_MAX_HEIGHT 1080U
#define CAPDEV_MS_PER_FRAME      16
#define CAPDEVTIMEOUT            100//100 * 10ms
#define SENSORTIMEOUT            100//100 * 10ms

#ifdef CFG_CAPTURE_ONFLY_MODE_ENABLE
#define CAP_MEM_MODE_ENABLE      false
#else
#define CAP_MEM_MODE_ENABLE      true
#endif

typedef struct SPlayerInstance {
    int             abort_request;
} SPlayerInstance;

typedef struct SPlayerProps {
    pthread_t          read_tid;
    cb_handler_t       callback;
    int                instCnt;
    bool               is_thread_create;
    bool               capdev_error;    /*capture error status*/
    bool               is_memory_mode; /*memory control*/
    uint32_t           channel_id;     /*sensor channel control*/
    MMP_CAP_DEVICE_ID  devices_type;   /*input source type*/
    SPlayerInstance    *inst;
} SPlayerProps;

typedef struct ITE_CAP_VIDEO_INFO_TAG {
  uint32_t OutWidth;
  uint32_t OutHeight;
  uint8_t IsInterlaced;
  MMP_CAP_FRAMERATE FrameRate;
  uint8_t *DisplayAddrY;
  uint8_t *DisplayAddrU;
  uint8_t *DisplayAddrV;
  uint8_t *DisplayAddrOldY;
  uint32_t PitchY;
  uint32_t PitchUV;
  uint32_t OutMemFormat;
} ITE_CAP_VIDEO_INFO;

void *cap_handle_thread(void *arg);
static void cap_isr0(void *arg);
///////////////////////////////////////////////////////////////////////////////////////////
// Global Value
//
///////////////////////////////////////////////////////////////////////////////////////////
static pthread_mutex_t gcapdev_mutex;
static SPlayerProps     *gcapdev_prop = NULL;
static CAPTURE_HANDLE  gCapDev0;

#ifdef __OPENRTOS__
QueueHandle_t          gCapInfoQueue; // Queue use to interrupt
#endif
///////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//
///////////////////////////////////////////////////////////////////////////////////////////
static void cap_isr0(void *arg)
{

    uint32_t       capture0state = 0, interrupt_status = 0;
#ifdef __OPENRTOS__    
    BaseType_t     gHigherPriorityTaskWoken = (BaseType_t) 0;
#endif
    capture0state = iteCapGetEngineErrorStatus(&gCapDev0, MMP_CAP_LANE0_STATUS);

    if ((capture0state >> 31) == 0x1U)
    {
        if (((capture0state >> 8) & 0xFU) != 0x0U)
        {
            #if 0
            (void)ithPrintf("cap0_isr err\n");
            #endif
            (void)ithPrintf("ErrorCode = 0x%x\n",(capture0state >> 8) & 0xFU);
            interrupt_status = 1;
#ifdef __OPENRTOS__            
            if(xQueueSendToBackFromISR(gCapInfoQueue, (void *)&interrupt_status, &gHigherPriorityTaskWoken) == 0)
            {
                (void)ithPrintf("xQueue is full \n");
            }
#endif
            //clear cap0 interrupt and reset error status
            (void)iteCapClearInterrupt(&gCapDev0, true);
        }
        else
        {   
            #if 0
            (void)ithPrintf("cap0_isr frame end\n");
            #endif
            interrupt_status = 0;
#ifdef __OPENRTOS__             
            if(xQueueSendToBackFromISR(gCapInfoQueue, (void *)&interrupt_status, &gHigherPriorityTaskWoken) == 0)
            {
                (void)ithPrintf("xQueue is full \n");
            }
#endif
            //clear cap0 interrupt
            (void)iteCapClearInterrupt(&gCapDev0, false);
        }
    }

#ifdef __OPENRTOS__      
    portYIELD_FROM_ISR(gHigherPriorityTaskWoken);
#endif

    return;
}

static int Signal_Check(CAPTURE_HANDLE *ptDev)
{
    int         timeout  = 0;

    while ((iteCapGetEngineErrorStatus(ptDev, MMP_CAP_LANE0_STATUS) & 0xEU) != 0xEU)
    {
        if (++timeout > CAPDEVTIMEOUT)
        {
            return 1;
        }
        (void)printf("Hsync or Vsync not stable!\n");
        (void)usleep(10 * 1000);
    }

    iteCapFire(ptDev, true);
    (void)printf("Capture Fire! (%d)\n", ptDev->cap_id);

    return 0;
}

static int SensorSignal_Check(void)
{
    int timeout = 0;
    while (iteCapDeviceIsSignalStable() != true)
    {
        if (++timeout > SENSORTIMEOUT)
        {
            (void)printf("Wait Sensor stable timeout\n");
            return 1;
        }
        (void)usleep(1000 * 10);
    } 
    return 0;
}
static void CaptureGetNewFrame(CAPTURE_HANDLE *ptDev, ITE_CAP_VIDEO_INFO *p_outdata)
{
    uint32_t cap_idx  = 0;
    CAP_CONTEXT *p_captxt  = &ptDev->cap_info;

    p_outdata->OutHeight    = p_captxt->outinfo.height;
    p_outdata->OutWidth     = p_captxt->outinfo.width;
    p_outdata->PitchY       = p_captxt->outinfo.pitch_y;
    p_outdata->PitchUV      = p_captxt->outinfo.pitch_uv;
    if(p_captxt->ininfo.Interleave == PROGRESSIVE)
    {
        p_outdata->IsInterlaced = 0x0U;
    }
    else
    {
        p_outdata->IsInterlaced = 0x1U;
    }

    cap_idx               = iteCapReturnWrBufIndex(ptDev);

    switch (cap_idx)
    {
    case 0:
        p_outdata->DisplayAddrY = (uint8_t *)p_captxt->out_addr_y[0];
        p_outdata->DisplayAddrU = (uint8_t *)p_captxt->out_addr_uv[0];
        p_outdata->DisplayAddrV = (uint8_t *)p_captxt->out_addr_uv[0];
        break;

    case 1:
        p_outdata->DisplayAddrY = (uint8_t *)p_captxt->out_addr_y[1];
        p_outdata->DisplayAddrU = (uint8_t *)p_captxt->out_addr_uv[1];
        p_outdata->DisplayAddrV = (uint8_t *)p_captxt->out_addr_uv[1];
        break;

    case 2:
        p_outdata->DisplayAddrY = (uint8_t *)p_captxt->out_addr_y[2];
        p_outdata->DisplayAddrU = (uint8_t *)p_captxt->out_addr_uv[2];
        p_outdata->DisplayAddrV = (uint8_t *)p_captxt->out_addr_uv[2];
        break;
    default:
        (void)printf("index is error \n");
        break;
    }
}

static void ithCapdevPlayer_FlipLCD(CAPTURE_HANDLE *ptDev)
{
    ITE_CAP_VIDEO_INFO video_info  = {0};
    uint8_t            *dbuf    = NULL;
#ifdef CFG_BUILD_ITV    
    ITV_DBUF_PROPERTY  dbufprop = {0};
#endif


#ifdef CFG_BUILD_ITV
    dbuf = itv_get_dbuf_anchor();
#endif
    if (dbuf == NULL)
    {
        (void)printf("itv buffer full \n");
        return;
    }

    CaptureGetNewFrame(ptDev, &video_info);

#ifdef CFG_BUILD_ITV
    if (video_info.IsInterlaced == 0x1U)
    {
        (void)itv_enable_isp_feature(MMP_ISP_DEINTERLACE);
    }
#endif
    dbufprop.src_w    = video_info.OutWidth;
    dbufprop.src_h    = video_info.OutHeight;
    dbufprop.pitch_y  = video_info.PitchY;
    dbufprop.pitch_uv = video_info.PitchUV;

    dbufprop.format   = MMP_ISP_IN_NV12;
    dbufprop.ya       = video_info.DisplayAddrY;
    dbufprop.ua       = video_info.DisplayAddrU;
    dbufprop.va       = video_info.DisplayAddrV;
#ifdef CFG_BUILD_ITV
    if(itv_update_dbuf_anchor(&dbufprop) != 0)
    {
        (void)printf("itv buffer full \n");
    }
#endif
}

void *cap_handle_thread(void *arg)
{
    SPlayerProps    *pprop           = (SPlayerProps *) arg;
    SPlayerInstance *is;
    int            ret              = 0;
    int            interrupt_status = 0;
    uint32_t       first_tick       = 0;
    int            diff_tick        = 0;
    int            delay_time       = 0;
    uint32_t       capturestate     = 0;
    MMP_ISP_SHARE  isp_share     = {0};

    if (!pprop)
    {
        (void)printf("Player not exist\n");
        ret = -1;
        goto fail;
    }

    is = pprop->inst;

    /*capture init*/
    if(iteCapInitialize() != 0)
    {
        (void)printf("ithCapInitialize fail\n");
        ret = -1;
        goto fail;
    }

    /*Sensor init */
    if(iteCapDeviceInitialize() != 0)
    {
        (void)printf("ithCapDeviceInitialize fail\n");
        ret = -1;
        goto fail;
    }

    iteCapDeviceCHSwitch(pprop->channel_id);

    (void)SensorSignal_Check();    

    
    /* Get decoder type */
    if(iteCapDeviceGetProperty(DEVICES_IS_DECODER) == 0x1U)
    {
        pprop->devices_type = MMP_CAP_DEV_ANALOG_DECODER;
    }
    else
    {
        pprop->devices_type = MMP_CAP_DEV_SENSOR;
    }
    /* Memory Mode Configuration */
    if(pprop->is_memory_mode)
    {
        (void)printf("CAPTURE MEM MODE type = %d\n", pprop->devices_type);
        CAPTURE_SETTING mem_modeset = {pprop->devices_type, false,  true, CAPDEV_SENSOR_MAX_WIDTH, CAPDEV_SENSOR_MAX_HEIGHT};
        iteCapConnect(&gCapDev0, mem_modeset);
        iteCapRegisterIRQ(&cap_isr0, &gCapDev0);
        iteCapGetDeviceInfo(&gCapDev0);
    }
    else /* Onfly Mode Configuration (with Isp) */
    {
        CAPTURE_SETTING onfly_modeset = {pprop->devices_type, true,  false, CAPDEV_SENSOR_MAX_WIDTH, CAPDEV_SENSOR_MAX_HEIGHT};
        iteCapConnect(&gCapDev0, onfly_modeset);
        iteCapDisableIRQ();
        iteCapGetDeviceInfo(&gCapDev0);
        itv_set_pb_mode(0);
        (void)printf("CAPTURE ONFLY MODE type = %d\n", pprop->devices_type);
        isp_share.addrY      = 0UL;
        isp_share.addrU      = 0UL;
        isp_share.addrV      = 0UL;
        isp_share.width      = (MMP_UINT16)gCapDev0.cap_info.outinfo.width;
        isp_share.height     = (MMP_UINT16)gCapDev0.cap_info.outinfo.height;
        isp_share.pitchY     = (MMP_UINT16)gCapDev0.cap_info.outinfo.pitch_y;
        isp_share.pitchUv    = (MMP_UINT16)gCapDev0.cap_info.outinfo.pitch_uv;
        isp_share.format     = MMP_ISP_IN_YUV422;
        itv_set_isp_onfly(isp_share, false);
        itv_set_pb_mode(1);
    }


    for (;;)
    {
        if (is->abort_request == 1)
        {
            (void)printf("cap_handle_thread break!\n");
            break;
        }

        first_tick = itpGetTickCount();
#ifdef __OPENRTOS__
        if(pprop->is_memory_mode)
        {
            if (xQueueReceive(gCapInfoQueue, &interrupt_status, 0) == 1)
            {
                if (interrupt_status == 0)
                {
                    ithCapdevPlayer_FlipLCD(&gCapDev0);
                }
                else
                {
                    pprop->capdev_error = true;
                    break;
                }
            }
        }
#endif
        if (iteCapIsFire(&gCapDev0) == false)
        {
            if (iteCapDeviceIsSignalStable())
            {
                iteCapGetDeviceInfo(&gCapDev0);
                if(iteCapParameterSetting(&gCapDev0) == 0)
                {
                    (void)Signal_Check(&gCapDev0);
                    first_tick = itpGetTickCount();
                }
            }
        }
        
        if(!pprop->is_memory_mode)
        {
            capturestate = iteCapGetEngineErrorStatus(&gCapDev0, MMP_CAP_LANE0_STATUS);
            if((capturestate & 0x0F00U) != 0x0U)
            {
                (void)printf("ErrorCode = 0x%x\n",(capturestate >> 8) & 0xFU);
                pprop->capdev_error = true;
                break;
            }
        }

        diff_tick = itpGetTickDuration(first_tick);
        delay_time = CAPDEV_MS_PER_FRAME - diff_tick;
        if(delay_time < 1)
        { 
            delay_time = 1;
        }
        (void)usleep(delay_time * 1000);
    }

    if(pprop->capdev_error)
    {
         pprop->callback(PLAYER_EVENT_CAPTURE_DEV, (void*)NULL);
         pprop->capdev_error = false;
    }
    
fail:
    (void)printf("terminate video\n");
        
    if (ret != 0)
    {
        (void)printf("read_thread failed, ret=%d\n", ret);
    }
    else
    {
        (void)printf("read thread %x done\n", pprop->read_tid);
    }
    pthread_exit(NULL);
    return NULL;
}

static int stream_close(SPlayerProps *pprop)
{
    void           *status;

    (void)printf("stream close\n");

    if ((pprop == NULL) || (pprop->inst == NULL))
    {
        return -1;
    }

    pprop->inst->abort_request = 1;

    if(pthread_join(pprop->read_tid, &status) != 0)
    {
        return -1;
    }

    free(pprop->inst);
    pprop->inst    = NULL;
    pprop->instCnt = 0;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
//
///////////////////////////////////////////////////////////////////////////////////////////

/*Capture init & sensor init*/
static int ithCapdevPlayer_init(cb_handler_t callback)
{
    SPlayerProps    *pprop        = NULL;
    SPlayerInstance *inst         = NULL;
    MMP_ISP_SHARE  isp_share      = {0};
    (void)printf("call = %s\n", __FUNCTION__);

    if (gcapdev_prop != NULL)
    {
        (void)printf("Initialize player failed\n");
        return -1;
    }
    pprop = (SPlayerProps *) calloc(sizeof(char), sizeof(SPlayerProps));
    if (pprop == NULL)
    {
        (void)printf("Initialize player failed\n");
        return -1;
    }

    pprop->callback      = callback;

    inst                 = (SPlayerInstance *) calloc(1, sizeof(SPlayerInstance));
    if (inst == NULL)
    {
        (void)printf("Initialize player failed\n");
        free(pprop);
        return -1;
    }

    if (pprop->inst != NULL)
    {
        free(pprop->inst);
    }
    pprop->inst             = inst;
    pprop->instCnt          = 1;
    pprop->is_thread_create = false;
    pprop->capdev_error     = false;
    /*IT986x default use memory mode*/
    pprop->is_memory_mode   = CAP_MEM_MODE_ENABLE;

    pthread_mutex_init(&gcapdev_mutex, NULL);

#ifdef __OPENRTOS__
    gCapInfoQueue      = xQueueCreate(CAPDEVQUEUESIZE, (unsigned portBASE_TYPE) sizeof(int));
#endif

    /*power on*/
    iteCapPowerUp();
    
    gcapdev_prop = pprop;

    return 0;
}

/*Set sensor channel ,wait sensor ready*/
static int ithCapdevPlayer_select_file(const char *filename, int level)
{
    (void)printf("call = %s\n", __FUNCTION__);
    SPlayerProps    *pprop = gcapdev_prop;
    
    if(strcmp(filename, "CH0") == 0)
    {
        pprop->channel_id = 0U;
    }
    else if(strcmp(filename, "CH1") == 0)
    {
        pprop->channel_id = 1U;
    }
    else if(strcmp(filename, "CH2") == 0)
    {
        pprop->channel_id = 2U;
    }
    else if(strcmp(filename, "CH3") == 0)
    {
        pprop->channel_id = 3U;
    }
    else
    {
        (void)printf("unknown channel\n");
    }
    
    return 0;
}

/*Run capture play thread*/
static int ithCapdevPlayer_play(void)
{
    SPlayerProps    *pprop = gcapdev_prop;
    int            rc;
    (void)printf("call = %s\n", __FUNCTION__);
    pthread_mutex_lock(&gcapdev_mutex);

    if (pprop == NULL)
    {
        (void)printf("pprop not exist\n");
        pthread_mutex_unlock(&gcapdev_mutex);
        return -1;
    }

    if (pprop->inst == NULL)
    {
        (void)printf("No assigned stream in player\n");
        pthread_mutex_unlock(&gcapdev_mutex);
        return -1;
    }

    if (pprop->is_thread_create == false)
    {
        rc  = pthread_create(&pprop->read_tid, NULL, &cap_handle_thread, (void *)pprop);

        if (rc > 0)
        {
            (void)printf("create thread failed %d\n", rc);
            pthread_mutex_unlock(&gcapdev_mutex);
            return -1;
        }
    }

    pprop->is_thread_create = true;
    pthread_mutex_unlock(&gcapdev_mutex);
    return 0;
}

/*Capture stop & sensor stop*/
static int ithCapdevPlayer_stop(void)
{
    SPlayerProps    *pprop = gcapdev_prop;

    (void)printf("call = %s\n", __FUNCTION__);
    pthread_mutex_lock(&gcapdev_mutex);

    if (!pprop)
    {
        (void)printf("pprop not exist\n");
        pthread_mutex_unlock(&gcapdev_mutex);
        return -1;
    }

    if(stream_close(pprop) != 0)
    {
        pthread_mutex_unlock(&gcapdev_mutex);
        return -1;
    }
    if(iteCapTerminate() != 0)
    {
        (void)printf("ithCapTerminate fail\n");
    }

    iteCapDeviceTerminate();

    pprop->is_thread_create = false;

#ifdef CFG_BUILD_ITV
    itv_flush_dbuf();
#endif

    pthread_mutex_unlock(&gcapdev_mutex);
    return 0;
}

/*Capture deinit*/
static int ithCapdevPlayer_deinit(void)
{
    (void)printf("call = %s\n", __FUNCTION__);

    if(gcapdev_prop == NULL)
    {
        (void)printf("pprop not exist\n");
        return -1;       
    }

    /* Release PlayerProps if any */
    if (gcapdev_prop != NULL)
    {
        if (gcapdev_prop->inst != NULL)
        {
            free(gcapdev_prop->inst);
        }
        free(gcapdev_prop);
        gcapdev_prop = NULL;
    }

    /* Release video request memory buffers */

    if(iteCapTerminate() != 0)
    {
        (void)printf("ithCapTerminate fail\n");
    }

    if(iteCapDisConnect(&gCapDev0) != 0)
    {
        (void)printf("ithCapDisConnect fail\n");
    }
#ifdef __OPENRTOS__
    vQueueDelete(gCapInfoQueue);
#endif
    pthread_mutex_destroy(&gcapdev_mutex);
    return 0;
}

/*None*/
static int ithCapdevPlayer_pause(void)
{
    (void)printf("call = %s\n", __FUNCTION__);
    return 0;
}

/*None*/
static int ithCapdevPlayer_play_videoloop(void)
{
    (void)printf("call = %s\n", __FUNCTION__);
    return 0;
}


/*Get current video resolution*/
static int ithCapdevPlayer_get_video_resolution_ext(int *width, int *height, char *filepath)
{
     *width  = (int)iteCapDeviceGetProperty(DEVICES_WIDTH);
     *height = (int)iteCapDeviceGetProperty(DEVICES_HEIGHT);
     return 0;
}


#if defined(_MSC_VER)
ithMediaPlayer captureplayer = {
    ithCapdevPlayer_init,
    ithCapdevPlayer_select_file,
    ithCapdevPlayer_play,
    ithCapdevPlayer_pause,
    ithCapdevPlayer_stop,
    ithCapdevPlayer_play_videoloop,
    ithCapdevPlayer_deinit,
    ithCapdevPlayer_get_video_resolution_ext,
};
#else // no defined _MSC_VER
ithMediaPlayer captureplayer = {
    .init                   = &ithCapdevPlayer_init,
    .select                 = &ithCapdevPlayer_select_file,
    .play                   = &ithCapdevPlayer_play,
    .pause                  = &ithCapdevPlayer_pause,
    .stop                   = &ithCapdevPlayer_stop,
    .play_videoloop         = &ithCapdevPlayer_play_videoloop,
    .deinit                 = &ithCapdevPlayer_deinit,
    .getVideoResolution_ext = &ithCapdevPlayer_get_video_resolution_ext,
};
#endif

