#include <string.h>
#include <stdlib.h>
#include "arm_lite_dev/lcdCrcCal/lcdCrcCal.h"

#define REMAP(x) ((uint8_t *)x - iteRiscGetTargetMemAddress(ARMLITE_CPU_IMAGE_MEM_TARGET))

static uint8_t gplcdCrcCalImage[] =
{
    #include "lcdCrcCal.hex"
};

static void lcdCrcCalProcessCommand(int cmdId)
{
    int i = 0;
    ARMLITE_COMMAND_REG_WRITE(REQUEST_CMD_REG, cmdId);
    while(1)
    {
        usleep(1000);

        if (ARMLITE_COMMAND_REG_READ(RESPONSE_CMD_REG) != cmdId)
            continue;
        else
            break;
    }
    ARMLITE_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);

    for (i = 0; i < 1024; i++)
    {
        asm("");
    }
    ARMLITE_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);
}

static int lcdCrcCalIoctl(int file, unsigned long request, void *ptr, void *info)
{
    uint8_t* pExchangeAddress = (uint8_t*) (iteRiscGetTargetMemAddress(ARMLITE_CPU_IMAGE_MEM_TARGET) + CMD_DATA_BUFFER_OFFSET);
    switch (request)
    {
        case ITP_IOCTL_INIT:
        {
            //Stop CPU
            iteRiscResetCpu(ARMLITE_CPU);

            //Clear Commuication Engine and command buffer
            memset(pExchangeAddress, 0x0, MAX_CMD_DATA_BUFFER_SIZE);
            ARMLITE_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);
            ARMLITE_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);

            //Load Engine First
            iteRiscLoadData(ARMLITE_CPU_IMAGE_MEM_TARGET,gplcdCrcCalImage,sizeof(gplcdCrcCalImage));

            //Fire CPU
            iteRiscFireCpu(ARMLITE_CPU);
            break;
        }

        case ITP_IOCTL_LCD_CRC_CAL:
        {
            LCD_CRC_CAL_CMD_DATA* ptCrcData = (LCD_CRC_CAL_CMD_DATA*) ptr;
            ptCrcData->buffAddr = (ptCrcData->buffAddr ? REMAP(ptCrcData->buffAddr):0);
            memcpy(pExchangeAddress, ptCrcData, sizeof(LCD_CRC_CAL_CMD_DATA));
#ifdef CFG_CPU_WB
            ithFlushDCacheRange((void*)pExchangeAddress, sizeof(LCD_CRC_CAL_CMD_DATA));
            ithFlushMemBuffer();
#endif
            lcdCrcCalProcessCommand(LCD_CRC_CAL_CMD_ID);
            ithInvalidateDCacheRange(pExchangeAddress, sizeof(LCD_CRC_CAL_CMD_DATA));
            memcpy(ptCrcData, pExchangeAddress, sizeof(LCD_CRC_CAL_CMD_DATA));
            break;
        }
        
        default:
            break;
    }
    return 0;
}

const ITPDevice itpDeviceLcdCrcCal =
{
    ":lcdCrcCal",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    lcdCrcCalIoctl,
    NULL
};
