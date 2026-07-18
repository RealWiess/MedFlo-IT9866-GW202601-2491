#include <string.h>
#include <stdlib.h>
#include "arm_lite_dev/hudBgFast/hudBgFast.h"

#define REMAP(x) ((uint8_t *)x - iteRiscGetTargetMemAddress(ARMLITE_CPU_IMAGE_MEM_TARGET))

static uint8_t gpHudBgFastImage[] =
{
    #include "hudBgFast.hex"
};

static void hudBgFastProcessCommand(int cmdId)
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

static int hudBgFastIoctl(int file, unsigned long request, void *ptr, void *info)
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
            iteRiscLoadData(ARMLITE_CPU_IMAGE_MEM_TARGET,gpHudBgFastImage,sizeof(gpHudBgFastImage));

            //Fire CPU
            iteRiscFireCpu(ARMLITE_CPU);
            break;
        }
        case ITP_IOCTL_HUD_INIT_PARAM:
        {
            HUD_INIT_DATA* ptInitData = (HUD_INIT_DATA*) ptr;
            ptInitData->ithCmdQ = (ptInitData->ithCmdQ ? REMAP(ptInitData->ithCmdQ):0);
            ptInitData->ithCmdQ1 = (ptInitData->ithCmdQ1 ? REMAP(ptInitData->ithCmdQ1):0);
            ptInitData->cmdQBase = (ptInitData->cmdQBase ? REMAP(ptInitData->cmdQBase):0);
            ptInitData->currPtr = (ptInitData->currPtr ? REMAP(ptInitData->currPtr):0);
            ptInitData->cmdQBase1 = (ptInitData->cmdQBase1 ? REMAP(ptInitData->cmdQBase1):0);
            ptInitData->currPtr1 = (ptInitData->currPtr1 ? REMAP(ptInitData->currPtr1):0);
            ptInitData->gTableID = (ptInitData->gTableID ? REMAP(ptInitData->gTableID):0);
            ptInitData->gHudTableIndex = (ptInitData->gHudTableIndex ? REMAP(ptInitData->gHudTableIndex):0);
            ptInitData->gpHudTableData = (ptInitData->gpHudTableData ? REMAP(ptInitData->gpHudTableData):0);
            ptInitData->gpHudTableData1 = (ptInitData->gpHudTableData1 ? REMAP(ptInitData->gpHudTableData1):0);
            ptInitData->gpHudTableData2 = (ptInitData->gpHudTableData2 ? REMAP(ptInitData->gpHudTableData2):0);
            ptInitData->gpHudTableData3 = (ptInitData->gpHudTableData3 ? REMAP(ptInitData->gpHudTableData3):0);
            memcpy(pExchangeAddress, ptInitData, sizeof(HUD_INIT_DATA));

            ithFlushDCacheRange((void*)pExchangeAddress, sizeof(HUD_INIT_DATA));
            ithFlushMemBuffer();

            hudBgFastProcessCommand(INIT_CMD_ID);
            break;
        }
        case ITP_IOCTL_HUD_INIT_CMDQ_PARAM:
        {
            HUD_INIT_DATA* ptInitData = (HUD_INIT_DATA*) ptr;
            ptInitData->ithCmdQ = (ptInitData->ithCmdQ ? REMAP(ptInitData->ithCmdQ):0);
            ptInitData->ithCmdQ1 = (ptInitData->ithCmdQ1 ? REMAP(ptInitData->ithCmdQ1):0);
            ptInitData->cmdQBase = (ptInitData->cmdQBase ? REMAP(ptInitData->cmdQBase):0);
            ptInitData->currPtr = (ptInitData->currPtr ? REMAP(ptInitData->currPtr):0);
            ptInitData->cmdQBase1 = (ptInitData->cmdQBase1 ? REMAP(ptInitData->cmdQBase1):0);
            ptInitData->currPtr1 = (ptInitData->currPtr1 ? REMAP(ptInitData->currPtr1):0);
            memcpy(pExchangeAddress, ptInitData, sizeof(HUD_INIT_DATA));

            ithFlushDCacheRange((void*)pExchangeAddress, sizeof(HUD_INIT_DATA));
            ithFlushMemBuffer();

            hudBgFastProcessCommand(INIT_CMD_CMDQ_ID);
            break;
        }
        case ITP_IOCTL_HUD_INIT_HW_PARAM:
        {
            HUD_INIT_DATA* ptInitData = (HUD_INIT_DATA*) ptr;
            ptInitData->gTableID = (ptInitData->gTableID ? REMAP(ptInitData->gTableID):0);
            ptInitData->gHudTableIndex = (ptInitData->gHudTableIndex ? REMAP(ptInitData->gHudTableIndex):0);
            ptInitData->gpHudTableData = (ptInitData->gpHudTableData ? REMAP(ptInitData->gpHudTableData):0);
            ptInitData->gpHudTableData1 = (ptInitData->gpHudTableData1 ? REMAP(ptInitData->gpHudTableData1):0);
            ptInitData->gpHudTableData2 = (ptInitData->gpHudTableData2 ? REMAP(ptInitData->gpHudTableData2):0);
            ptInitData->gpHudTableData3 = (ptInitData->gpHudTableData3 ? REMAP(ptInitData->gpHudTableData3):0);
            memcpy(pExchangeAddress, ptInitData, sizeof(HUD_INIT_DATA));

            ithFlushDCacheRange((void*)pExchangeAddress, sizeof(HUD_INIT_DATA));
            ithFlushMemBuffer();

            hudBgFastProcessCommand(INIT_CMD_HW_ID);
            break;
        }
        case ITP_IOCTL_HUD_SKIP_BG_FAST:
        {
            HUD_CONTEXT* ptInitData = (HUD_CONTEXT*) ptr;
            ptInitData->dst = REMAP(ptInitData->dst);
            memcpy(pExchangeAddress, ptInitData, sizeof(HUD_CONTEXT));

            ithFlushDCacheRange((void*)pExchangeAddress, sizeof(HUD_CONTEXT));
            ithFlushMemBuffer();

            hudBgFastProcessCommand(SKIP_BG_FAST_CMD_ID);
            break;
        }
        
        default:
            break;
    }
    return 0;
}

const ITPDevice itpDeviceHudBgFast =
{
    ":hudBgFast",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    hudBgFastIoctl,
    NULL
};
