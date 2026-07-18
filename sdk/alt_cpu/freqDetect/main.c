#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "alt_cpu/freqDetect/freqDetect.h"
#include "alt_cpu/alt_cpu_utility.h"

#define ENDIAN_SWAP16(x) \
        (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8))

#define ENDIAN_SWAP32(x) \
        (((x & 0x000000FF) << 24) | \
        ((x & 0x0000FF00) <<  8) | \
        ((x & 0x00FF0000) >>  8) | \
        ((x & 0xFF000000) >> 24))

#define FREQ_DETECT_STATE_NULL               0
#define FREQ_DETECT_STATE_MONITOR            1

typedef struct
{
    uint32_t                bValid;
    uint32_t                bRun;
    uint32_t                freqDetectGpio;
    int                     prevGpioVal;
    uint32_t                startTime;
    uint32_t                checkTime;
    uint32_t                checkTicks;
    uint32_t                checkCount;
    uint32_t                transitionCount;
    uint32_t                duration;
    uint32_t                hz;
    uint32_t                state;
    uint32_t                cpuClock;
	uint32_t                sn;
} FREQ_DETECT_HANDLE;

static FREQ_DETECT_HANDLE gptFreqDetectHandle[FREQ_DETECT_COUNT] = { 0 };
static uint32_t           gSn[FREQ_DETECT_COUNT] = { 0 };

static void freqDetectProcessInitCmd(void)
{
    FREQ_DETECT_INIT_DATA* ptInitData = (FREQ_DETECT_INIT_DATA*) CMD_DATA_BUFFER_OFFSET;
    uint32_t freqDetectId = ENDIAN_SWAP32(ptInitData->freqDetectId);

    FREQ_DETECT_HANDLE *ptFreqDetectHandle = 0;
    if (freqDetectId >= FREQ_DETECT0 && freqDetectId < FREQ_DETECT_COUNT)
    {
        ptFreqDetectHandle = &gptFreqDetectHandle[freqDetectId];
        ptFreqDetectHandle->freqDetectGpio = ENDIAN_SWAP32(ptInitData->freqDetectGpio);
        ptFreqDetectHandle->cpuClock = ENDIAN_SWAP32(ptInitData->cpuClock);
        ptFreqDetectHandle->checkTicks = ptFreqDetectHandle->cpuClock >> 2;
        ptFreqDetectHandle->bValid = 1;
        ptFreqDetectHandle->bRun = 0;

        //set pin to gpio mode
        setGpioMode(ptFreqDetectHandle->freqDetectGpio, 0);
        //Set GPIO to input mode
        setGpioDir(ptFreqDetectHandle->freqDetectGpio, 1);
    }
}

static void freqDetectProcessReadCmd(void)
{
    FREQ_DETECT_READ_DATA* ptReadCmd = (FREQ_DETECT_READ_DATA*) CMD_DATA_BUFFER_OFFSET;
    uint32_t freqDetectId = ENDIAN_SWAP32(ptReadCmd->freqDetectId);

    if (freqDetectId >= FREQ_DETECT0 && freqDetectId < FREQ_DETECT_COUNT)
    {
        FREQ_DETECT_HANDLE *ptFreqDetectHandle = &gptFreqDetectHandle[freqDetectId];
        if (!ptFreqDetectHandle->bRun || !ptFreqDetectHandle->bValid)
        {
            ptReadCmd->hz = 0;
            ptReadCmd->sn = 0;
        }
        else
        {
            ptReadCmd->hz = ENDIAN_SWAP32(ptFreqDetectHandle->hz);
            ptReadCmd->sn = ENDIAN_SWAP32(ptFreqDetectHandle->sn);
        }
    }
}

static void freqDetectProcessStartCmd(void)
{
    FREQ_DETECT_START_CMD_DATA* ptStartCmd = (FREQ_DETECT_START_CMD_DATA*) CMD_DATA_BUFFER_OFFSET;
    uint32_t freqDetectId = ENDIAN_SWAP32(ptStartCmd->freqDetectId);  
    
    if (freqDetectId >= FREQ_DETECT0 && freqDetectId < FREQ_DETECT_COUNT)
    {
        FREQ_DETECT_HANDLE *ptFreqDetectHandle = &gptFreqDetectHandle[freqDetectId];
        if (ptFreqDetectHandle->bValid)
        {
            ptFreqDetectHandle->bRun = 1;
            ptFreqDetectHandle->state = FREQ_DETECT_STATE_NULL;
            ptFreqDetectHandle->startTime = 0;
            ptFreqDetectHandle->prevGpioVal = -1;
            ptFreqDetectHandle->duration = 0;
            ptFreqDetectHandle->transitionCount = 0;
        }
    }
}

static void freqDetectProcessStopCmd(void)
{
    FREQ_DETECT_STOP_CMD_DATA* ptStopCmd = (FREQ_DETECT_STOP_CMD_DATA*) CMD_DATA_BUFFER_OFFSET;
    uint32_t freqDetectId = ENDIAN_SWAP32(ptStopCmd->freqDetectId);  
    
    if (freqDetectId >= FREQ_DETECT0 && freqDetectId < FREQ_DETECT_COUNT)
    {
        FREQ_DETECT_HANDLE *ptFreqDetectHandle = &gptFreqDetectHandle[freqDetectId];
        if (ptFreqDetectHandle->bValid && ptFreqDetectHandle->bRun)
        {
            ptFreqDetectHandle->bRun = 0;
        }
    }
}

static void freqDetectMonitor(void)
{
    uint32_t freqDetectId = 0;

    FREQ_DETECT_HANDLE *ptFreqDetectHandle = 0;

    for (freqDetectId = FREQ_DETECT0; freqDetectId < FREQ_DETECT_COUNT; freqDetectId++)
    {
        ptFreqDetectHandle = &gptFreqDetectHandle[freqDetectId];
        if (ptFreqDetectHandle->bRun)
        {
            switch (ptFreqDetectHandle->state)
            {
                case FREQ_DETECT_STATE_NULL:
                {
                    ptFreqDetectHandle->prevGpioVal = getGpioValue(ptFreqDetectHandle->freqDetectGpio, 1);
                    ptFreqDetectHandle->state = FREQ_DETECT_STATE_MONITOR;
                    break;
                }
                case FREQ_DETECT_STATE_MONITOR:
                {
                    uint32_t gpioVal = getGpioValue(ptFreqDetectHandle->freqDetectGpio, 1);
                    if (gpioVal != ptFreqDetectHandle->prevGpioVal)
                    {
                        ptFreqDetectHandle->prevGpioVal = gpioVal;
                        if (ptFreqDetectHandle->transitionCount == 0)
                        {
                            ptFreqDetectHandle->startTime = ptFreqDetectHandle->checkTime = getCurTimer(0);
                        }
                        ptFreqDetectHandle->prevGpioVal = gpioVal;
                        if (++ptFreqDetectHandle->transitionCount == 3)
                        {
                            float hz0 = 0.0;
                            uint32_t hz = 0;
                            uint32_t newStartTime = getCurTimer(0);
                            ptFreqDetectHandle->duration = getDuration(0, ptFreqDetectHandle->startTime);
                            hz0 = ((float)ptFreqDetectHandle->cpuClock / ptFreqDetectHandle->duration) + 0.4;
                            if (hz0 > 2000) ;
                            else if (hz0 > 1985 && hz0 < 1994)
                                hz0 += 6;
                            else if (hz0 > 1800)
                                hz0 += 5;
                            else if (hz0 > 1700)
                                hz0 += 4;
                            else if (hz0 > 1500)
                                hz0 += 3;
                            else if (hz0 > 1000)
                                hz0 += 2;
                            else if (hz0 > 500)
                                hz0 += 1;
                            hz = (uint32_t)hz0;

                            gSn[freqDetectId]++;
                            ptFreqDetectHandle->hz = hz;
                            ptFreqDetectHandle->sn = gSn[freqDetectId];
                            ptFreqDetectHandle->startTime = ptFreqDetectHandle->checkTime = newStartTime;
                            ptFreqDetectHandle->transitionCount = 1;
                            ptFreqDetectHandle->checkCount = 0;
                        }
                    }
                    break;
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    //Set GPIO and Clock Setting
    int cnt = 0;

    //Start Timer
    memset(gptFreqDetectHandle, 0x0, sizeof(gptFreqDetectHandle));
    startTimer(0);
    while(1)
    {
        if(cnt == 200)
        {
            int inputCmd = ALT_CPU_COMMAND_REG_READ(REQUEST_CMD_REG);
            if (inputCmd && ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) == 0)
            {
                switch(inputCmd)
                {
                    case INIT_CMD_ID:
                        freqDetectProcessInitCmd();
                        break;
                    case READ_CMD_ID:
                        freqDetectProcessReadCmd();
                        break;
                    case START_CMD_ID:
                        freqDetectProcessStartCmd();
                        break;
                    case STOP_CMD_ID:
                        freqDetectProcessStopCmd();
                        break;
                    default:
                        break;
                }
                ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, (uint16_t) inputCmd);
            }
            cnt = 0;
        }
        freqDetectMonitor();
        cnt++;
    }
}
