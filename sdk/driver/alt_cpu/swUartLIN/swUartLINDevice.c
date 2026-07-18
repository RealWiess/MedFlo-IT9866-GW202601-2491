#include <string.h>
#include "alt_cpu/swUartLIN/swUartLIN.h"

static uint8_t gpswUartLINImage[] =
{
#include "swUartLIN.hex"
};

#if 0
static void
swUartLINDebug (
    int reg_num)
{
    int RegBase = 0x16E2;
    int i;
    int tmp     = 0;

    while (i <= (reg_num - 1) * 2)   // reg_num max: 7
    {
        tmp = ALT_CPU_COMMAND_REG_READ(RegBase + i);
        (void)printf("0x%x: %x\n", RegBase + i, tmp);
        i   = i + 2;
    }
}
#endif

static void
swUartLINProcessCommand (
    int cmdId)
{
    int i = 0;
    ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, cmdId);
    for (;;)
    {
        if (ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) != cmdId) //這邊相等代表alt_cpu事情做完了
        {
            // Waiting ALT CPU response
            continue;
        }
        else
        {
            break;
        }
    }
    ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0); //完成任務後，將REQUEST_CMD_REG設為0
    for (i = 0; i < 1024; i++)
    {
        asm ("");
    }
    ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0); //將RESPONSE_CMD_REG也設為0，回到原始狀態
}

static int
swUartLINRead (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    uint8_t             * pReadAddress  = (uint8_t *) (iteRiscGetTargetMemAddress(ALT_CPU_IMAGE_MEM_TARGET) + CMD_DATA_BUFFER_OFFSET);
    SW_UART_READ_DATA   ptReadData      = {0};
    SW_UART_READ_DATA   *data_info = (SW_UART_READ_DATA *)ptr;
    ptReadData.pReadDataBuffer  = data_info->pReadDataBuffer; 
    ptReadData.len              = len;
    if (ptReadData.len >= MAX_UART_BUFFER_SIZE)
    {
        ithPrintf("swUartLIN read data is too big\n");
        return 0;
    }
    memcpy(pReadAddress, &ptReadData, sizeof(SW_UART_READ_DATA));
    swUartLINProcessCommand(READ_DATA_CMD_ID);
    memcpy(&ptReadData, pReadAddress, sizeof(SW_UART_READ_DATA));
    memcpy(ptReadData.pReadDataBuffer, ptReadData.pReadBuffer, ptReadData.len);

    // ithInvalidateDCacheRange(ptReadData.pReadDataBuffer, ptReadData.len); //不能加這行 不然他會把資料都清空。

    memcpy(&(data_info->rx_breakfield_dectect), &(ptReadData.rx_breakfield_dectect), sizeof(bool));
    
    (void)usleep(100);
    return ptReadData.len;
}

static int
swUartLINWrite (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    uint8_t             * pWriteAddress = (uint8_t *) (iteRiscGetTargetMemAddress(ALT_CPU_IMAGE_MEM_TARGET) + CMD_DATA_BUFFER_OFFSET);
    SW_UART_WRITE_DATA  *data_info = (SW_UART_WRITE_DATA *)ptr;
    SW_UART_WRITE_DATA  ptWriteData     = {0};
    ptWriteData.len                 = len;
    ptWriteData.pWriteDataBuffer    = data_info->pWriteDataBuffer;
    ptWriteData.isbreakfield = data_info->isbreakfield;

    // printf("value : 0x%x! ptWriteData.isbreakfield : %d\n", ptWriteData.pWriteDataBuffer[0], data_info->isbreakfield);
    // printf("Entering swUartLINWrite!\n");

    if (ptWriteData.len >= MAX_UART_BUFFER_SIZE) // 要加上 || ptWriteData.len == 0，否則data長度為0有可能會出問題
    {
        ithPrintf("swUartLIN write data is too big\n");
        return 0;
    }

    memcpy(ptWriteData.pWriteBuffer, ptWriteData.pWriteDataBuffer, ptWriteData.len);
    do
    {
        memcpy(pWriteAddress, &ptWriteData, sizeof(SW_UART_WRITE_DATA)); //放到altcpu上的databuffer位址
        swUartLINProcessCommand(WRITE_DATA_CMD_ID);
        (void)usleep(100);
    } while (ptWriteData.len != ((SW_UART_WRITE_DATA *) pWriteAddress)->len);

    return ptWriteData.len;
}

// Just for SW Uart Debug
static int
swUartLINDbgPutchar (
    int c)
{
    static unsigned char    pBuffer[64] = {0};
    static int              curIndex    = 0;

    pBuffer[curIndex++] = (unsigned char) c;

    if (curIndex >= 64)
    {
        itpWrite(ITP_DEVICE_ALT_CPU, pBuffer, curIndex);
        curIndex = 0;
    }
    return c;
}

static int
swUartLINIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    uint8_t * pWriteAddress = (uint8_t *) (iteRiscGetTargetMemAddress(ALT_CPU_IMAGE_MEM_TARGET) + CMD_DATA_BUFFER_OFFSET);
    switch (request)
    {
        case ITP_IOCTL_INIT:
        {
            
            // Stop ALT CPU
            iteRiscResetCpu(ALT_CPU);

            // Clear Commuication Engine and command buffer
            memset(pWriteAddress, 0x0, MAX_CMD_DATA_BUFFER_SIZE);
            ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);
            ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);

            // Load Engine First
            iteRiscLoadData(ALT_CPU_IMAGE_MEM_TARGET, gpswUartLINImage, sizeof(gpswUartLINImage)); //這邊決定cpu會做什麼

            // Fire Alt CPU
            iteRiscFireCpu(ALT_CPU);
            break;
        }

        case ITP_IOCTL_INIT_DBG_UART:
        {
            // For Software Uart Debug
            ithPutcharFunc = swUartLINDbgPutchar;
            break;
        }

        case ITP_IOCTL_INIT_UART_PARAM:
        {
            SW_UART_INIT_DATA * ptInitData = (SW_UART_INIT_DATA *) ptr;
            // Once use HW timer, use bus clock instead of RISC CPU Clk.
            if (ptInitData->cpuClock != ithGetBusClock())
            {
                ptInitData->cpuClock = ithGetBusClock();
            }

            if (ptInitData->uartTxGpio == 0xFFFFFFFF)   // if TxGpio = -1
            {
                ptInitData->uartTxGpio = 0;
            }

            if (ptInitData->uartRxGpio == 0xFFFFFFFF)   // if RxGpio = -1
            {
                ptInitData->uartRxGpio = 0;
            }

            memcpy(pWriteAddress, ptInitData, sizeof(SW_UART_INIT_DATA));
            swUartLINProcessCommand(INIT_CMD_ID);
            break;
        }

        default:
            break;
    }
    return 0;
}

const ITPDevice itpDeviceSwUartLIN =
{
    ":swUartLIN",
    itpOpenDefault,
    itpCloseDefault,
    swUartLINRead,
    swUartLINWrite,
    itpLseekDefault,
    swUartLINIoctl,
    NULL
};
