#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "alt_cpu/freqDetect/freqDetect.h"

typedef struct
{
    pthread_t *readThread;
    unsigned long readData[FREQ_DETECT_COUNT][16];
    uint8_t readQuit;
    uint8_t run[FREQ_DETECT_COUNT];
} FREQDETECT_DEV_HANDLE;

static uint8_t gpFreqDetectImage[] =
{
#include "freqDetect.hex"
};

unsigned long lastTick[FREQ_DETECT_COUNT];
unsigned long lastSN[FREQ_DETECT_COUNT];

static pthread_mutex_t gFreqDetectMutex  = PTHREAD_MUTEX_INITIALIZER;
static FREQDETECT_DEV_HANDLE gFreqDetectDevHandle = {0};

static void freqDetectProcessCommand(int cmdId)
{
    int i = 0;
    ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, cmdId);
    while(1)
    {
        if (ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) != cmdId)
            continue;
        else
            break;
    }
    ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);
    for (i = 0; i < 1024; i++)
    {
        asm("");
    }
    ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);
}

static void *read_thread_func(void *arg)
{
    uint8_t cnt = 0 , freqDetectId;
    uint8_t* pWriteAddress = (uint8_t*) (iteRiscGetTargetMemAddress(ALT_CPU_IMAGE_MEM_TARGET) + CMD_DATA_BUFFER_OFFSET);
    while(!gFreqDetectDevHandle.readQuit)
    {
        for(freqDetectId = FREQ_DETECT0; freqDetectId < FREQ_DETECT_COUNT; freqDetectId++)
        {
            if(gFreqDetectDevHandle.run[freqDetectId] == 1)
            {
                FREQ_DETECT_READ_DATA tReadData = { 0 };
                tReadData.freqDetectId = freqDetectId;
                pthread_mutex_lock(&gFreqDetectMutex);
                memcpy(pWriteAddress, &tReadData, sizeof(FREQ_DETECT_READ_DATA));
                freqDetectProcessCommand(READ_CMD_ID);
                memcpy(&tReadData, pWriteAddress, sizeof(FREQ_DETECT_READ_DATA));
                if(tReadData.hz != 0) {
                    gFreqDetectDevHandle.readData[freqDetectId][cnt] = tReadData.hz;
                }
                pthread_mutex_unlock(&gFreqDetectMutex);
            }
        }
        ++cnt;
        cnt = cnt % 16;
        usleep(1000); //1ms
    }
}

static void freqDetectReadThreadStart()
{
    if(gFreqDetectDevHandle.readThread != NULL)
    {
        gFreqDetectDevHandle.readQuit = 1;
        pthread_join(*gFreqDetectDevHandle.readThread, NULL);
        free(gFreqDetectDevHandle.readThread);
    }
    memset(&gFreqDetectDevHandle, 0x0, sizeof(gFreqDetectDevHandle));
    gFreqDetectDevHandle.readThread = (pthread_t *)malloc(sizeof(pthread_t));
    pthread_create(gFreqDetectDevHandle.readThread, NULL, read_thread_func, NULL);
}

static void freqDetectProcessReadDataMean(FREQ_DETECT_READ_DATA* pReadData)
{
    pReadData->hz = 0;

    pthread_mutex_lock(&gFreqDetectMutex);
    for(int i = 0; i < 16; i++) pReadData->hz += gFreqDetectDevHandle.readData[pReadData->freqDetectId][i];
    pthread_mutex_unlock(&gFreqDetectMutex);

    pReadData->hz = pReadData->hz >> 4;
}

static int freqDetectIoctl(int file, unsigned long request, void *ptr, void *info)
{
    uint8_t* pWriteAddress = (uint8_t*) (iteRiscGetTargetMemAddress(ALT_CPU_IMAGE_MEM_TARGET) + CMD_DATA_BUFFER_OFFSET);
    switch (request)
    {
        case ITP_IOCTL_INIT:
        {
            //Stop ALT CPU
            iteRiscResetCpu(ALT_CPU);
            //Clear Commuication Engine and command buffer
            memset(pWriteAddress, 0x0, MAX_CMD_DATA_BUFFER_SIZE);
            ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);
            ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);

            //Load Engine First
            iteRiscLoadData(ALT_CPU_IMAGE_MEM_TARGET,gpFreqDetectImage,sizeof(gpFreqDetectImage));
            //Fire Alt CPU
            iteRiscFireCpu(ALT_CPU);
            //read thread start
            freqDetectReadThreadStart();
            break;
        }
        case ITP_IOCTL_INIT_FREQ_DETECT_PARAM:
        {
            FREQ_DETECT_INIT_DATA* ptInitData = (FREQ_DETECT_INIT_DATA*) ptr;
            //Once use HW timer, use bus clock instead of RISC CPU Clk.
            if (ptInitData->cpuClock != ithGetBusClock())
            {
                ptInitData->cpuClock = ithGetBusClock();
            }
            pthread_mutex_lock(&gFreqDetectMutex);
            memcpy(pWriteAddress, ptInitData, sizeof(FREQ_DETECT_INIT_DATA));
            freqDetectProcessCommand(INIT_CMD_ID);
            pthread_mutex_unlock(&gFreqDetectMutex);
            break;
        }
        case ITP_IOCTL_FREQ_DETECT_START:
        {
            FREQ_DETECT_START_CMD_DATA* ptStartCmd = (FREQ_DETECT_START_CMD_DATA*) ptr;
            pthread_mutex_lock(&gFreqDetectMutex);
            memcpy(pWriteAddress, ptStartCmd, sizeof(FREQ_DETECT_START_CMD_DATA));
            freqDetectProcessCommand(START_CMD_ID);
            lastTick[ptStartCmd->freqDetectId] = itpGetTickCount();
            //set read freqDetectId active
            gFreqDetectDevHandle.run[ptStartCmd->freqDetectId] = 1;
            pthread_mutex_unlock(&gFreqDetectMutex);
            break;
        }
        case ITP_IOCTL_FREQ_DETECT_READ:
        {
            unsigned long curTick;
            FREQ_DETECT_READ_DATA* ptReadCmd = (FREQ_DETECT_READ_DATA*) ptr;
            pthread_mutex_lock(&gFreqDetectMutex);
            memcpy(pWriteAddress, ptReadCmd, sizeof(FREQ_DETECT_READ_DATA));
            freqDetectProcessCommand(READ_CMD_ID);
            memcpy(ptReadCmd, pWriteAddress, sizeof(FREQ_DETECT_READ_DATA));
            pthread_mutex_unlock(&gFreqDetectMutex);
            curTick = itpGetTickCount();
            if(ptReadCmd->sn != lastSN[ptReadCmd->freqDetectId])
            {
            	lastSN[ptReadCmd->freqDetectId] = ptReadCmd->sn;
            	lastTick[ptReadCmd->freqDetectId] = curTick;
            }
            else
            {
                if (itpGetTickDuration(lastTick[ptReadCmd->freqDetectId]) > 1100)
            	{
                    //ithPrintf("_________func(#02): %s,Time out of freqDetectId %d \n", __func__, ptReadCmd->freqDetectId);
                    ptReadCmd->hz = 0;
                    ptReadCmd->sn = 0;
            	}
            }

            if(ptReadCmd->hz > 1200)
            {
                freqDetectProcessReadDataMean(ptReadCmd);
            }
            break;
        }
        case ITP_IOCTL_FREQ_DETECT_STOP:
        {
            FREQ_DETECT_STOP_CMD_DATA* ptStopCmd = (FREQ_DETECT_STOP_CMD_DATA*) ptr;
            pthread_mutex_lock(&gFreqDetectMutex);
            memcpy(pWriteAddress, ptStopCmd, sizeof(FREQ_DETECT_STOP_CMD_DATA));
            freqDetectProcessCommand(STOP_CMD_ID);
            //set read freqDetectId deactive
            gFreqDetectDevHandle.run[ptStopCmd->freqDetectId] = 0;
            pthread_mutex_unlock(&gFreqDetectMutex);
            break;
        }
        default:
            break;
    }
    return 0;
}

const ITPDevice itpDeviceFreqDetect =
{
    ":freqDetect",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    freqDetectIoctl,
    NULL
};
