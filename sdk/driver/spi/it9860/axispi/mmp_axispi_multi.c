//=============================================================================
//                              Include Files
//=============================================================================
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "ite/ith.h"
#include "ite/itp.h"
#include "ssp/mmp_axispi.h"
#include "ssp/ssp_error.h"
#if defined (__OPENRTOS__)
#elif defined (__FREERTOS__)
    #include "or32.h"
#endif
#include "axispi_hw.h"
#include "axispi_reg.h"
#include "spi_msg.h"

#ifdef CFG_WATCHDOG_ENABLE
extern bool WatchDogGetIsChipInfo(void);
#endif
//=============================================================================
//                              Compile Option
//=============================================================================
//#define ENABLE_SET_SPI_CLOCK_AS_40MHZ
//#define ENABLE_SET_SPI_CLOCK_AS_10MHZ

//=============================================================================
//                              Macro
//=============================================================================
#define TO_SPI_HW_PORT(port) SPI_HW_PORT_0

#define SWAP_ENDIAN32(value) \
    ((((value) >> 24) & 0x000000FF) | \
     (((value) >> 8) & 0x0000FF00) | \
     (((value) << 8) & 0x00FF0000) | \
     (((value) << 24) & 0xFF000000))

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Global Data Definition
//=============================================================================
typedef struct SPI_OBJECT_TAG
{
    int                         refCount;
    pthread_mutex_t             mutex;
    SPI_HW_PORT                 hwPort;
    SPI_OP_MODE                 opMode;
    SPI_CONFIG_MAPPING_ENTRY    tMappingEntry;
    int                         dma_ch;
    int                         dma_slave_ch;
    char                        *ch_name;
    char                        *slave_ch_name;
    // Slave Mode
    bool                        slaveReadThreadTerminate;
    pthread_t                   slaveReadThread;
    SpiSlaveCallbackFunc        slaveCallBackFunc;
    float                       refclk;
} SPI_OBJECT;

typedef struct PORT_IO_MAPPING_ENTRY_TAG
{
    SPI_PORT             port;
    SPI_IO_MAPPING_ENTRY mapping_entry;
} PORT_IO_MAPPING_ENTRY;

static SPI_OBJECT SpiObjects[1] =
{
    {
        0,                             // refCount
        PTHREAD_MUTEX_INITIALIZER      // mutex
    }
};

static PORT_IO_MAPPING_ENTRY tSpiMisoMappingTable[] =
{
    {SPI_0, {7, 1}}
};

static PORT_IO_MAPPING_ENTRY tSpiMosiMappingTable[] =
{
    {SPI_0, {6, 1}}
};

static PORT_IO_MAPPING_ENTRY tSpiClockMappingTable[] =
{
    {SPI_0, {10, 1}}
};

static PORT_IO_MAPPING_ENTRY tSpiChipSelMappingTable[] =
{
    {SPI_0, {5, 1}}, {SPI_0, {11, 1}}
};

static PORT_IO_MAPPING_ENTRY tSpiWpMappingTable[] =
{
    {SPI_0, {8, 1}}
};

static PORT_IO_MAPPING_ENTRY tSpiHoldMappingTable[] =
{
    {SPI_0, {9, 1}}
};

#ifdef SPI_USE_DMA
static int              DmaSpiChannel = 0;
#endif
#if defined(SPI_USE_DMA) && defined(SPI_USE_DMA_INTR)
static sem_t            SpiDmaMutex;
#endif

static unsigned int     gSpiCtrlMethod                      = SPI_CONTROL_SLAVE;
static unsigned int     gShareGpioTable[SPI_SHARE_GPIO_MAX] = {0U};
#if 0
static SPI_CONTROL_MODE ctrl_mode   = SPI_CONTROL_NOR;
#endif
static pthread_mutex_t  SwitchMutex;
static unsigned int     gDummyByte[SPI_PORT_MAX][SPI_CSN_MAX]  = {0U};

//=============================================================================
//                              Private Function Declaration
//=============================================================================

//=============================================================================
//                              Private Function Definition
//=============================================================================
#if defined(SPI_USE_DMA) && defined(SPI_USE_DMA_INTR)
static void
SpiDmaIntr(
    int         ch,
    void        *arg,
    uint32_t    int_status)
{
    itpSemPostFromISR(&SpiDmaMutex);
}
#endif

#ifdef SPI_USE_DMA
static bool
spiWaitDmaIdle_(
    int dmachannel)
{
    #ifdef SPI_USE_DMA_INTR
    int result = 0;

    result = itpSemWaitTimeout(&SpiDmaMutex, 5000);
    if (0 != result)
    {
        /* Time out, fail! */
        return false;
    }
    else
    {
        return true;
    }
    #else
    uint32_t timeout_ms = 3000U;

    while (ithDmaIsBusy(dmachannel) && --timeout_ms)
    {
        (void)usleep(1000);
    }

    if (0U == timeout_ms)
    {
        /* DMA fail */
        return false;
    }
    return true;
    #endif
}
#endif

#ifdef SPI_USE_DMA
static bool
spiInitDma_(SPI_PORT port)
{
    bool                result      = true;
    #ifdef SPI_USE_DMA_INTR
    ITHDmaIntrHandler   spiDmaIntr  = SpiDmaIntr;
    #else
    ITHDmaIntrHandler   spiDmaIntr  = NULL;
    #endif

    if (port == SPI_0)
    {
        SpiObjects[port].ch_name        = "dma_spi0";
        SpiObjects[port].slave_ch_name  = "dma_slave_spi0";
    }
    else
    {
        return false;
    }

#if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    SpiObjects[port].dma_ch = ithDmaRequestCh(SpiObjects[port].ch_name, ITH_DMA_CH_PRIO_HIGH_3, spiDmaIntr, NULL);
#else
    SpiObjects[port].dma_ch = ithDmaRequestCh(SpiObjects[port].ch_name, ITH_DMA_CH_PRIO_HIGH_3, NULL, NULL);
#endif

    if (SpiObjects[port].dma_ch >= 0)
    {
        ithDmaReset(SpiObjects[port].dma_ch);
    }
    else
    {
        SPI_ERROR_MSG("Request DMA fail.\n");
        result = false;
    }

#if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    SpiObjects[port].dma_slave_ch = ithDmaRequestCh(SpiObjects[port].slave_ch_name, ITH_DMA_CH_PRIO_HIGH_3, spiDmaIntr, NULL);
#else
    SpiObjects[port].dma_slave_ch = ithDmaRequestCh(SpiObjects[port].slave_ch_name, ITH_DMA_CH_PRIO_HIGH_3, NULL, NULL);
#endif

    if (SpiObjects[port].dma_slave_ch >= 0)
    {
        ithDmaReset(SpiObjects[port].dma_slave_ch);
    }
    else
    {
        SPI_ERROR_MSG("Request DMA fail.\n");
        result = false;
    }
    return result;
}
#endif

#if 0
static float
_GetWclkClock()
{
    uint16_t    PLL1_numerator = ithReadRegH(0x00A4) & 0x3FF;
    uint16_t    reg_1C = ithReadRegH(0x001C);
    uint16_t    wclkRatio = reg_1C & 0x3FF;
    uint16_t    clockSrc = (reg_1C & 0x3800) >> 11;
    uint16_t    regValue = 0;
    uint16_t    pllDivider = 1;
    float       wclk_value = 0;

    switch (clockSrc)
    {
    case 0: // From PLL1 output1
        regValue    = ithReadRegH(0x00A2);
        pllDivider  = regValue & 0x007F;
        break;

    case 1: // From PLL1 output2
        regValue    = ithReadRegH(0x00A2);
        pllDivider  = (regValue & 0x7F00) >> 8;
        break;

    case 2: // From PLL2 output1
    case 3: // From PLL2 output2
    case 4: // From PLL3 output1
    case 5: // From PLL3 output2
    case 6: // From CKSYS (12MHz)
    case 7: // From Ring OSC (200KHz)
        SPI_ERROR_MSG("Unknown clock source %d\n", clockSrc);
        assert(0);
        break;

    default:
        break;
    }

    wclk_value = (float)PLL1_numerator / (wclkRatio + 1) / pllDivider;
    return wclk_value;
}

static void
_spiResetSpiObject(
    SPI_OBJECT *object)
{
    object->hwPort                      = SPI_HW_PORT_0;
    object->opMode                      = SPI_OP_MASTR;
    object->slaveReadThreadTerminate    = false;
    #if 0
    object->slaveReadThread = NULL;
    #endif
    object->slaveCallBackFunc           = NULL;
}
#endif

static bool
spiAssign970Gpio_(
    SPI_PORT port,
    SPI_CSN cs,
    ITHGpioDriving driving)
{
    int                         i           = 0;
    int                         entryCount  = 0;
    int                         mosiGpio    = 0;
    int                         misoGpio    = 0;
    int                         clockGpio   = 0;
    int                         chipSelGpio = 0;
    int                         wpGpio      = 0;
    int                         holdGpio    = 0;

    SPI_CONFIG_MAPPING_ENTRY    *ptMappingEntry;

    ptMappingEntry = &SpiObjects[port].tMappingEntry;
    if (port == SPI_0)
    {
#if defined (CFG_AXISPI_ENABLE)
        mosiGpio    = CFG_AXISPI_MOSI_GPIO;
        misoGpio    = CFG_AXISPI_MISO_GPIO;
        clockGpio   = CFG_AXISPI_CLOCK_GPIO;
        chipSelGpio = CFG_AXISPI_CHIP_SEL_GPIO;
        wpGpio      = CFG_AXISPI_WP_GPIO;
        holdGpio    = CFG_AXISPI_HOLD_GPIO;
#else
        return false;
#endif
    }
    else
    {
        (void)printf(" UnKnown SPI Port !!!");
        return false;
    }

    //MOSI
    entryCount = sizeof(tSpiMosiMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
    for (i = 0; i < entryCount; i++)
    {
        if ((port == tSpiMosiMappingTable[i].port)
            && (mosiGpio == tSpiMosiMappingTable[i].mapping_entry.gpioPin))
        {
            break;
        }
    }
    if (i >= entryCount)
    {
        return false;
    }
    ptMappingEntry->spiMosi.gpioPin     = tSpiMosiMappingTable[i].mapping_entry.gpioPin;
    ptMappingEntry->spiMosi.gpioMode    = tSpiMosiMappingTable[i].mapping_entry.gpioMode;

    //MISO
    entryCount                          = sizeof(tSpiMisoMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
    for (i = 0; i < entryCount; i++)
    {
        if ((port == tSpiMisoMappingTable[i].port)
            && (misoGpio == tSpiMisoMappingTable[i].mapping_entry.gpioPin))
        {
            break;
        }
    }
    if (i >= entryCount)
    {
        return false;
    }
    ptMappingEntry->spiMiso.gpioPin     = tSpiMisoMappingTable[i].mapping_entry.gpioPin;
    ptMappingEntry->spiMiso.gpioMode    = tSpiMisoMappingTable[i].mapping_entry.gpioMode;

    //CLOCK
    entryCount                          = sizeof(tSpiClockMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
    for (i = 0; i < entryCount; i++)
    {
        if ((port == tSpiClockMappingTable[i].port)
            && (clockGpio == tSpiClockMappingTable[i].mapping_entry.gpioPin))
        {
            break;
        }
    }
    if (i >= entryCount)
    {
        return false;
    }
    ptMappingEntry->spiClock.gpioPin    = tSpiClockMappingTable[i].mapping_entry.gpioPin;
    ptMappingEntry->spiClock.gpioMode   = tSpiClockMappingTable[i].mapping_entry.gpioMode;

    //CHIP SEL
    entryCount                          = sizeof(tSpiChipSelMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
    if (cs >= entryCount)
    {
        return false;
    }
    ptMappingEntry->spiChipSel.gpioPin  = tSpiChipSelMappingTable[cs].mapping_entry.gpioPin;
    ptMappingEntry->spiChipSel.gpioMode = tSpiChipSelMappingTable[cs].mapping_entry.gpioMode;

    //WP
    entryCount                          = sizeof(tSpiWpMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
    for (i = 0; i < entryCount; i++)
    {
        if ((port == tSpiWpMappingTable[i].port)
            && (wpGpio == tSpiWpMappingTable[i].mapping_entry.gpioPin))
        {
            break;
        }
    }
    if (i >= entryCount)
    {
        return false;
    }
    ptMappingEntry->spiWp.gpioPin   = tSpiWpMappingTable[i].mapping_entry.gpioPin;
    ptMappingEntry->spiWp.gpioMode  = tSpiWpMappingTable[i].mapping_entry.gpioMode;

    //HOLD
    entryCount                      = sizeof(tSpiHoldMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
    for (i = 0; i < entryCount; i++)
    {
        if ((port == tSpiHoldMappingTable[i].port)
            && (holdGpio == tSpiHoldMappingTable[i].mapping_entry.gpioPin))
        {
            break;
        }
    }
    if (i >= entryCount)
    {
        return false;
    }
    ptMappingEntry->spiHold.gpioPin     = tSpiHoldMappingTable[i].mapping_entry.gpioPin;
    ptMappingEntry->spiHold.gpioMode    = tSpiHoldMappingTable[i].mapping_entry.gpioMode;
    ithGpioSetDriving(mosiGpio, driving);
    ithGpioSetDriving(misoGpio, driving);
    ithGpioSetDriving(clockGpio, driving);
    ithGpioSetDriving(ptMappingEntry->spiChipSel.gpioPin, driving);
    ithGpioSetDriving(wpGpio, driving);
    ithGpioSetDriving(holdGpio, driving);

    return true;
}

static bool
spiSetGPIO_(
    SPI_PORT port,
    SPI_CSN  cs)
{
    int                         setGpio = 0;
    int                         setMode = 0;
    int                         entryCount  = 0;
    int                         i = 0;
    SPI_CONFIG_MAPPING_ENTRY    *ptMappingEntry;

#if 0
    if (ithGetDeviceId() == 0x970)
#endif
    {
        if (!spiAssign970Gpio_(port, cs, ITH_GPIO_DRIVING_1))
        {
            return false;
        }
    }
#if 0
    else
    {
        return false;
    }
#endif

    ptMappingEntry  = &SpiObjects[port].tMappingEntry;

    // Set MOSI
    setGpio         = ptMappingEntry->spiMosi.gpioPin;
    setMode         = ptMappingEntry->spiMosi.gpioMode;
    ithGpioSetMode(setGpio, setMode);

    // Set MISO
    setGpio = ptMappingEntry->spiMiso.gpioPin;
    setMode = ptMappingEntry->spiMiso.gpioMode;
    ithGpioSetMode(setGpio, setMode);

    // Set CLOCK
    setGpio = ptMappingEntry->spiClock.gpioPin;
    setMode = ptMappingEntry->spiClock.gpioMode;
    ithGpioSetMode(setGpio, setMode);

    // Set CHIP SEL    
    setGpio = ptMappingEntry->spiChipSel.gpioPin;
    setMode = ptMappingEntry->spiChipSel.gpioMode;
    ithGpioSetMode(setGpio, setMode);

    // Reset other CHIP SEL
    entryCount                          = sizeof(tSpiChipSelMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
    for (i = 0; i < entryCount; i++)
    {
        if ((port == tSpiChipSelMappingTable[i].port)
            && (setGpio != tSpiChipSelMappingTable[i].mapping_entry.gpioPin))
        {
            ithGpioSetMode(tSpiChipSelMappingTable[i].mapping_entry.gpioPin, ITH_GPIO_MODE0);
        }
    }
    
    // Set WP
    setGpio = ptMappingEntry->spiWp.gpioPin;
    setMode = ptMappingEntry->spiWp.gpioMode;
    ithGpioSetMode(setGpio, setMode);

    // Set HOLD
    setGpio = ptMappingEntry->spiHold.gpioPin;
    setMode = ptMappingEntry->spiHold.gpioMode;
    ithGpioSetMode(setGpio, setMode);

    return true;
}

static bool AXISPIWriteData(uint8_t *data, uint32_t dataSize)
{
    //the Max FIFO count is 32, so we check the TX FIFO space ,
    //if it is valid ,TX FIFO Valid count should not be zero. (push one data, TX FIFO -1)
    uint32_t    writeCount  = dataSize;
    uint32_t    i           = 0U;
    int32_t     timeOut     = 10000000;
    uint32_t    value       = 0U;

    while (writeCount > 0U)
    {
        if ((AxiSpiReadTXFIFOValidSpace()) > 0U)
        {
            if (writeCount >= 4U)
            {
                value       = ((((uint32_t)data[i]) << 24UL) | (((uint32_t)data[i + 1]) << 16UL) | (((uint32_t)data[i + 2]) << 8UL) | ((uint32_t)data[i + 3]));
                AxiSpiWriteData(value);
                writeCount  -= 4U;
                i           += 4U;
            }
            else if (writeCount == 3U)
            {
                value       = ((((uint32_t)data[i]) << 24UL) | (((uint32_t)data[i + 1]) << 16UL) | (((uint32_t)data[i + 2]) << 8UL));
                AxiSpiWriteData(value);
                writeCount  -= 3U;
                i           += 3U;
            }
            else if (writeCount == 2U)
            {
                value       = ((((uint32_t)data[i]) << 24UL) | (((uint32_t)data[i + 1]) << 16UL));
                AxiSpiWriteData(value);
                writeCount  -= 2U;
                i           += 2U;
            }
            else
            {
                value       = ((((uint32_t)data[i]) << 24UL));
                AxiSpiWriteData(value);
                writeCount  -= 1U;
                i++;
            }
        }
        else
        {
            (void)usleep(1);
            if ((--timeOut) < 0)
            {
                break;
            }
        }
    }

    return (timeOut >= 0);
}

static bool AXISPIReadData(uint8_t *outData, uint32_t outDataSize)
{
#if 0
    (void)printf("AXISPIReadData\n");
#endif
    uint32_t    readCount   = outDataSize;
    uint32_t    i           = 0U;
    int32_t     timeOut     = 10000000;

    while (readCount)
    {
        uint32_t rxFifoCount = AxiSpiReadRXFIFODataCount();
        if (rxFifoCount > 0U)
        {
            uint32_t val = AxiSpiReadData();

            if (readCount >= 4U)
            {
                outData[i]     = (uint8_t)((val & 0x000000FFUL) >> 0UL);
                outData[i + 1] = (uint8_t)((val & 0x0000FF00UL) >> 8UL);
                outData[i + 2] = (uint8_t)((val & 0x00FF0000UL) >> 16UL);
                outData[i + 3] = (uint8_t)((val & 0xFF000000UL) >> 24UL);
#if 0
                (void)printf("[%x], [%x], [%x], [%x]\n", outData[i], outData[i+1], outData[i+2], outData[i+3]);
#endif
                readCount       -= 4U;
                i               += 4U;
            }
            else
            {
                if (readCount == 3U)
                {
                    outData[i]     = (uint8_t)((val & 0x000000FFUL) >> 0UL);
                    outData[i + 1] = (uint8_t)((val & 0x0000FF00UL) >> 8UL);
                    outData[i + 2] = (uint8_t)((val & 0x00FF0000UL) >> 16UL);
#if 0
                    (void)printf("out[%d]= [%x], out[%d]= [%x], out[%d]= [%x]\n", i, outData[i], i+1, outData[i+1], i+2, outData[i+2]);
#endif
                    readCount   -= 3U;
                    i           += 3U;
                }
                else if (readCount == 2U)
                {
                    outData[i]     = (uint8_t)((val & 0x000000FFUL) >> 0UL);
                    outData[i + 1] = (uint8_t)((val & 0x0000FF00UL) >> 8UL);
#if 0
                    (void)printf("out[%d]=[%x], out[%d]= [%x]\n", i, outData[i], i+1, outData[i+1]);
#endif
                    readCount   -= 2U;
                    i           += 2U;
                }
                else
                {
                    outData[i] = (uint8_t)((val & 0x000000FFUL) >> 0UL);
#if 0
                    (void)printf("out[%d]= [%x]\n", i, outData[i]);
#endif
                    readCount   -= 1U;
                    i           += 1U;
                }
            }
        }
        else
        {
            (void)usleep(1);
            if ((--timeOut) < 0)
            {
                break;
            }
        }
    }

    return (timeOut >= 0);
}

#ifdef SPI_USE_DMA
static bool
spiDmaWriteData_(
    uint8_t     *data,
    uint32_t    dataSize)
{
    bool result = true;
    #if !defined(CFG_NOR_USE_AXISPI) && defined(CFG_NAND_ENABLE)
    uint32_t timeout_ms = 3000000U;
    #endif
    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE
    ITHDmaBurst burstlen = 0;
        #endif
    #endif

    DmaSpiChannel = SpiObjects[0].dma_ch;
    #if 0
    (void)printf("spiDmaWriteData_ data=0x%x,DmaSpiChannel=%d\n", data,
                 DmaSpiChannel);

    ithFlushDCacheRange((void *)data, dataSize);
    ithFlushMemBuffer();
    #endif

    ithWriteRegMaskA((ITH_DMA_BASE + 0x4CU), 0x0UL << ((uint32_t)DmaSpiChannel & 0x7U),
                     0x1UL << ((uint32_t)DmaSpiChannel & 0x7U)); // Indian change

    /* bit 4: RX fifo endian , bit 5 : TX Fifo*/
    ithWriteRegMaskA(SPI_REG_CMD_W1, (0UL << 5UL), (1UL << 5UL));
    #if 0
    (void)printf("write dma channel = %d\n", DmaSpiChannel);
    #endif
    ithDmaSetSrcAddr(DmaSpiChannel, (uint32_t)data);
    ithDmaSetSrcParamsA(DmaSpiChannel, ITH_DMA_WIDTH_32, ITH_DMA_CTRL_INC);
    ithDmaSetDstParamsA(DmaSpiChannel, ITH_DMA_WIDTH_32, ITH_DMA_CTRL_FIX);
    #if 0
    (void)printf("--- write datasize = 0x%x ---\n", dataSize);
    #endif
    ithDmaSetTxSize(DmaSpiChannel, dataSize);

    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE
    if (0U != (dataSize & 0x004U))
    {
        burstlen = ITH_DMA_BURST_1;
    }
    else if (0U != (dataSize & 0x008U))
    {
        burstlen = ITH_DMA_BURST_2;
    }
    else if (0U != (dataSize & 0x010U))
    {
        burstlen = ITH_DMA_BURST_4;
    }
    else if (0U != (dataSize & 0x020U))
    {
        burstlen = ITH_DMA_BURST_8;
    }
    else
    {
        burstlen = ITH_DMA_BURST_8;
    }

    ithDmaSetBurst(DmaSpiChannel, burstlen);
        #else
    ithDmaSetBurst(DmaSpiChannel, ITH_DMA_BURST_8);
        #endif
    #else
    ithDmaSetBurst(DmaSpiChannel, ITH_DMA_BURST_8);
    #endif

    ithDmaSetDstAddr(DmaSpiChannel, SPI_REG_DATA_PORT);
    ithDmaSetRequest(DmaSpiChannel, ITH_DMA_NORMAL_MODE, ITH_DMA_AXISPI_TX_RX, ITH_DMA_HW_HANDSHAKE_MODE, ITH_DMA_AXISPI_TX_RX);

    ithDmaStart(DmaSpiChannel); // Fire DMA

    AxiSpiDMAEnable();

    // Wait idle
    #if 1
        #if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    if (spiWaitDmaIdle_(DmaSpiChannel) == false)
    {
        (void)printf("Wait DMA idle timeout.\n");
        result = false;
    }
        #else
    while (ithDmaIsBusy(DmaSpiChannel) && --timeout_ms)
    {
        (void)usleep(1);
    }

    if (!timeout_ms)
    {
        (void)printf("Wait DMA idle timeout.\n");
        result = false;
    }
        #endif
    #endif

    AxiSpiDMADisable();

    return result;
}

static bool
spiDmaReadData_(
    uint8_t     *outData,
    uint32_t    outDataSize)
{
    bool        result      = true;
    #if !defined(CFG_NOR_USE_AXISPI) && defined(CFG_NAND_ENABLE)
    uint32_t timeout_ms = 3000000U;
    #endif
    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE
    ITHDmaBurst burstlen = 0;
        #endif
    #endif

    DmaSpiChannel = SpiObjects[0].dma_slave_ch;

    #if 0
    (void)printf("spiDmaReadData_ outData=0x%x,DmaSpiChannel=%d\n",outData,DmaSpiChannel);
    #endif
    AxiSpiSetRxFifoThod(1U);

    ithWriteRegMaskA((ITH_DMA_BASE + 0x4CU), 0x0UL << ((uint32_t)DmaSpiChannel & 0x7U), 0x1UL << ((uint32_t)DmaSpiChannel & 0x7U)); //Indian change

    ithWriteRegMaskA(SPI_REG_CMD_W1, (0UL << 4UL), (1UL << 4UL)); /* bit 4: RX fifo endian , bit 5 : TX Fifo*/

    ithDmaSetDstAddr(DmaSpiChannel, (uint32_t)outData);
    ithDmaSetSrcParamsA(DmaSpiChannel, ITH_DMA_WIDTH_32, ITH_DMA_CTRL_FIX);
    ithDmaSetDstParamsA(DmaSpiChannel, ITH_DMA_WIDTH_32, ITH_DMA_CTRL_INC);

    ithDmaSetTxSize(DmaSpiChannel, outDataSize);

    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE
    if (0U != (outDataSize & 0x004U))
    {
        burstlen = ITH_DMA_BURST_1;
    }
    else if (0U != (outDataSize & 0x008U))
    {
        burstlen = ITH_DMA_BURST_2;
    }
    else if (0U != (outDataSize & 0x010U))
    {
        burstlen = ITH_DMA_BURST_4;
    }
    else if (0U != (outDataSize & 0x020U))
    {
        burstlen = ITH_DMA_BURST_8;
    }
    else if (0U != (outDataSize & 0x040U))
    {
        burstlen = ITH_DMA_BURST_16;
    }
    else
    {
        burstlen = ITH_DMA_BURST_16;
    }

    ithDmaSetBurst(DmaSpiChannel, burstlen);
        #else
    ithDmaSetBurst(DmaSpiChannel, ITH_DMA_BURST_16);
        #endif
    #else
    ithDmaSetBurst(DmaSpiChannel, ITH_DMA_BURST_16);
    #endif

    ithDmaSetSrcAddr(DmaSpiChannel, SPI_REG_DATA_PORT);
    ithDmaSetRequest(DmaSpiChannel, ITH_DMA_HW_HANDSHAKE_MODE, ITH_DMA_AXISPI_TX_RX, ITH_DMA_NORMAL_MODE, ITH_DMA_AXISPI_TX_RX);
    ithDmaStart(DmaSpiChannel); // Fire DMA
    AxiSpiDMAEnable();
    ithInvalidateDCacheRange(outData, outDataSize);

    // Wait ilde
    #if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    if (spiWaitDmaIdle_(DmaSpiChannel) == false)
    {
        (void)printf("Wait DMA idle timeout.\n");
        result = false;
    }
    #else
    while (ithDmaIsBusy(DmaSpiChannel) && --timeout_ms)
    {
        (void)usleep(1);
    }

    if (!timeout_ms)
    {
        (void)printf("Wait DMA idle timeout.\n");
        result = false;
    }
    #endif
    AxiSpiDMADisable();

    return result;
}

#endif

void
mmpAxiSpiSetDummyByteMulti(SPI_PORT port, SPI_CSN cs, unsigned int dummybyte)
{
    pthread_mutex_lock(&SpiObjects[port].mutex);
    gDummyByte[port][cs] = dummybyte;
    pthread_mutex_unlock(&SpiObjects[port].mutex);
}

int32_t
mmpAxiSpiInitializeMulti(
    SPI_PORT    port,
    SPI_CSN     cs,
    SPI_OP_MODE mode,
    SPI_FORMAT  format,
    SPI_CLK_LAB spiclk,
    SPI_CLK_DIV divider)
{
    int32_t result = 0;

    SPI_FUNC_ENTRY;

    if (port >= SPI_PORT_MAX)
    {
        result = 1;
        goto end;
    }

    pthread_mutex_lock(&SpiObjects[port].mutex);

    if (SpiObjects[port].refCount == 0)
    {
#if 0
        int      entrycnt     = 0;
        int      i            = 0;
        float    wclk         = _GetWclkClock();
        uint32_t sclk_divider = 0U;

        _spiResetSpiObject(&SpiObjects[port]);
        SpiObjects[port].refCount = 1;
        SpiObjects[port].hwPort   = TO_SPI_HW_PORT(port);
        SpiObjects[port].opMode   = mode;
        SpiObjects[port].refclk   = 20.0f;

        entrycnt                  = sizeof(spi_clk_tab) / sizeof(SPI_REF_CLK);
        for (i = 0; i < entrycnt; i++)
        {
            if (spiclk == spi_clk_tab[i].spi_clk_lab)
            {
                SpiObjects[port].refclk = spi_clk_tab[i].refclock;
                break;
            }
        }

        for (sclk_divider = 0; sclk_divider < 80; sclk_divider++)
        {
            float tmpclk = 0.0f;
            tmpclk       = (wclk / (((float)sclk_divider + 1) * 2));

            if (tmpclk <= SpiObjects[port].refclk)
            {
                break;
            }
        }
#endif
        AxiSpiSetClkDiv(divider);

        if (spiSetGPIO_(port, cs) == false)
        {
            SPI_ERROR_MSG("spiSetGPIO_() fail.\n");
            result = 1;
            goto end;
        }

#if 0
        _spiSetRegisters(port, mode, format, sclk_divider);

        (void)printf("\n========================================\n");
        (void)printf("               SPI %d init\n", port);
        (void)printf("========================================\n");
        (void)printf("Version       : %d.%d.%d\n",
                     spiGetMajorVersion(TO_SPI_HW_PORT(port)),
                     spiGetMinorVersion(TO_SPI_HW_PORT(port)),
                     spiGetReleaseVersion(TO_SPI_HW_PORT(port)));
        (void)printf("SClock Divider : %d\n", sclk_divider);
        (void)printf("WCLK          : %.2f MHz\n", wclk);
        (void)printf("SPI Clock     : %.2f MHz\n\n",
                     wclk / (((float)sclk_divider + 1) * 2));
#endif

#ifdef SPI_USE_DMA
        if (spiInitDma_(port) == false)
        {
            SPI_ERROR_MSG("spiInitDma_() fail.\n");
            result = 1;
            goto end;
        }
    #ifdef SPI_USE_DMA_INTR
        sem_init(&SpiDmaMutex, 0, 0);
    #endif
#endif

#if 0
        if (mode == SPI_OP_SLAVE)
        {
            pthread_create(&SpiObjects[port].slaveReadThread, NULL,
                           _SpiSlaveReadThread, &SpiObjects[port]);
        }
#endif
    }
    else
    {
        SpiObjects[port].refCount++;
        (void)printf("SPI already initialed. refCount = %d\n", SpiObjects[port].refCount);
    }

end:
    pthread_mutex_unlock(&SpiObjects[port].mutex);
    SPI_FUNC_LEAVE;

#if 0
    (void)printf("loopback mode start\n");
    ithWriteRegMaskA(TO_SPI_HW_PORT(port) + REG_SPI_CONTROL_0, (1UL << 7UL), (1UL << 7UL));
    (void)printf("loopback mode end\n");
#endif
    return result;
}

int32_t
mmpAxiSpiTerminateMulti(
    SPI_PORT port)
{
    int32_t result = 0;
    pthread_mutex_destroy(&SwitchMutex);
    pthread_mutex_lock(&SpiObjects[port].mutex);

    SpiObjects[port].refCount--;
    if (SpiObjects[port].refCount == 0)
    {
#ifdef SPI_USE_DMA
        ithDmaFreeCh(SpiObjects[port].dma_ch);
        ithDmaFreeCh(SpiObjects[port].dma_slave_ch);
    #ifdef SPI_USE_DMA_INTR
        sem_destroy(&SpiDmaMutex);
    #endif
#endif

        if (SpiObjects[port].opMode == SPI_OP_SLAVE)
        {
            SpiObjects[port].slaveReadThreadTerminate = true;
        }

        (void)printf( "========================================\n");
        (void)printf( "             SPI %d terminated.\n", port);
        (void)printf( "========================================\n");
    }

    pthread_mutex_unlock(&SpiObjects[port].mutex);

    return result;
}

#ifdef SPI_USE_DMA
int32_t
mmpAxiSpiDmaWriteMulti(
    SPI_PORT        port,
    SPI_CSN         cs,
    SPI_IOMODE      iomode,
    uint8_t         *inCommand,
    SPI_CMD_LEN     inCommandLen,
    uint32_t        *slaveaddr,
    SPI_ADDR_LEN    slaveaddrLen,
    uint8_t         *inData,
    uint32_t        inDataSize)
{
    int32_t     result      = true;
#if 0
    (void)printf("++ DMA Write ++\n");
    int32_t  currentsize = inDataSize;;
#endif
    int32_t     timeOut     = 1000000;

    pthread_mutex_lock(&SpiObjects[port].mutex);
    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE

            #ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
    {
        // Turn off watchdog hw reset mode, then use sw interrupt isr to handle
        // inappropriate system unknown hanged issue inside axispi access scope.
        ithWatchDogCtrlDisable(ITH_WD_RESET);
    }
            #else
    ithWatchDogCtrlDisable(ITH_WD_RESET);
            #endif

        #endif
    #endif

    #if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    if (spiSetGPIO_(port, cs) == false)
    {
        pthread_mutex_unlock(&SpiObjects[port].mutex);
        return false;
    }
#endif

    if (slaveaddr == NULL)
    {
        AxiSpiWriteAddr(0);
    }
    else
    {
        AxiSpiWriteAddr(*slaveaddr);
    }
    AxiSpiWriteAddrLength(slaveaddrLen);
    AxiSpiWriteDataCounter(inDataSize); //Data Counter. (4 byte = 32bit)
    AxiSpiWriteInstLength(inCommandLen);
    AxiSpiWriteDummy(0);

    #if 1
    AxiSpiWriteInstCode4bitWriteEnable((uint32_t)*inCommand, iomode);
    #else
    //AxiSpiWriteInstCodeWriteEnable((uint32_t)*inCommand);
    if ((uint32_t)*inCommand == 0x38 || (uint32_t)*inCommand == 0x3E || (uint32_t)*inCommand == 0x32)
    {
        #if 0
        (void)printf("--dma write cmd = 0x%x --\n", (uint32_t)*inCommand);
        #endif
        AxiSpiWriteInstCode4bitWriteEnable((uint32_t)*inCommand);
    }
    else
    {
        AxiSpiWriteInstCodeWriteEnable((uint32_t)*inCommand);
    }
    #endif

    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE
    if ((inDataSize % 4) != 0)
    {
        (void)printf(
            "[AXISPI_DW error] mmpAxiSpiDmaWrite is not 4 alignment!!!(%d)\n",
            inDataSize);
        while (true)
        {
            sleep(1);
        }
    }
        #endif
    #endif

    do
    {
        if (spiDmaWriteData_(inData, inDataSize) == 0)
        {
            (void)printf("write fail!\n");
        }

        // Wait ilde
        while (0U == AxiSpiReadIntrStatus())
        {
            if (--timeOut < 0)
            {
                (void)printf("AxiSpiReadIntrStatus fail!\n");
                result = false;
                break;
            }
            else
            {
                (void)usleep(1);
            }
        }
        AxiSpiSetIntrStatus();
    } while (false);

    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE

            #ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
    {
                #if 0
        ithWriteRegMaskA(0xD800006C, 1UL << 31UL, 1UL << 31UL);
        (void)usleep(10);
        ithWriteRegMaskA(0xD800006C, 0UL << 31UL, 1UL << 31UL);
        AxiSpiRestoreLastSetting();
                #endif
        // Turn on watchdog hw reset mode. Theoretically, hw reset suppose to
        // cover all cases out of axi spi access scope.
        ithWatchDogCtrlEnable(ITH_WD_RESET);
    }
            #else
                #if 0
    ithWriteRegMaskA(0xD800006C, 1UL << 31UL, 1UL << 31UL);
    (void)usleep(10);
    ithWriteRegMaskA(0xD800006C, 0UL << 31UL, 1UL << 31UL);
    AxiSpiRestoreLastSetting();
                #endif
    ithWatchDogCtrlEnable(ITH_WD_RESET);
            #endif

        #endif
    #endif

    pthread_mutex_unlock(&SpiObjects[port].mutex);

    return result;
}
#else
int32_t
mmpAxiSpiDmaWrite(
    SPI_PORT        port,
    SPI_CSN         cs,
    SPI_IOMODE      iomode,
    uint8_t         *inCommand,
    SPI_CMD_LEN     inCommandLen,
    uint32_t        *slaveaddr,
    SPI_ADDR_LEN    slaveaddrLen,
    uint8_t         *inData,
    uint32_t        inDataSize)
{
    return mmpAxiSpiPioWriteMulti(port, cs, iomode, inCommand, inCommandLen, slaveaddr, slaveaddrLen, inData, inDataSize);
}
#endif

#ifdef SPI_USE_DMA
int32_t
mmpAxiSpiDmaReadMulti(
    SPI_PORT        port,
    SPI_CSN         cs,
    SPI_IOMODE      iomode,
    SPI_DTRMODE     dtrmode,
    uint8_t         *inCommand,
    SPI_CMD_LEN     inCommandLen,
    uint32_t        *slaveaddr,
    SPI_ADDR_LEN    slaveaddrLen,
    uint8_t         *outData,
    uint32_t        outDataSize)
{
    int32_t result = true;
#if 0
    (void)printf("++ DMA Read ++\n");
#endif
    int32_t     remainsize  = outDataSize;
    uint32_t    rDMASize    = 0U;
    uint8_t     *rpaddr     = outData;
    int32_t     timeOut     = 10000000;

    pthread_mutex_lock(&SpiObjects[port].mutex);
    
    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE

            #ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
    {
        // Turn off watchdog hw reset mode, then use sw interrupt isr to handle
        // inappropriate system unknown hanged issue inside axispi access scope.
        ithWatchDogCtrlDisable(ITH_WD_RESET);
    }
            #else
    ithWatchDogCtrlDisable(ITH_WD_RESET);
            #endif

        #endif
    #endif

    #if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    if (spiSetGPIO_(port, cs) == false)
    {
        pthread_mutex_unlock(&SpiObjects[port].mutex);
        return false;
    }
    #endif
    #if 0
    ithWriteRegMaskA(0xd800006cU, (1UL << 31UL), (1UL << 31UL));
    #endif
    while (AxiSpiReadRXFIFODataCount())
    {
        AxiSpiReadData();
    }

    if (slaveaddr == NULL)
    {
        AxiSpiWriteAddr(0);
    }
    else
    {
        AxiSpiWriteAddr(*slaveaddr);
    }
    AxiSpiWriteAddrLength(slaveaddrLen);
    AxiSpiWriteDataCounter(outDataSize);//Data Counter. (4 byte = 32bit)
    AxiSpiWriteInstLength(inCommandLen);
    AxiSpiWriteDummy(0U);

    #if 1
    if ((iomode == SPI_IOMODE_5) || (iomode == SPI_IOMODE_4) ||
        (iomode == SPI_IOMODE_2))
    {
        AxiSpiWriteDummy(gDummyByte[port][cs]);
    }

    AxiSpiWriteInstCode4bitRead((uint32_t)*inCommand, (iomode == SPI_IOMODE_5) ? SPI_IOMODE_0 : iomode, dtrmode);
    #else
    if ((uint32_t)*inCommand == 0xEBU || (uint32_t)*inCommand == 0xECU)
    {
        AxiSpiWriteDummy(6U);
        #if 0
        (void)printf("-- dma read cmd 0x%x --\n", (uint32_t)*inCommand);
        #endif
        AxiSpiWriteInstCode4bitRead((uint32_t)*inCommand);
    }
    else
    {
        AxiSpiWriteInstCodeRead((uint32_t)*inCommand);
    }
    #endif

    if ((outDataSize % 4U) == 0U)
    {
        rDMASize    = outDataSize;
        remainsize  -= outDataSize;
    }
    else
    {
    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE
        (void)printf(
            "[AXISPI_DR error] mmpAxiSpiDmaRead is not 4 alignment!!!(%d)\n",
            outDataSize);
        while (true)
        {
            sleep(1);
        }
        #endif
    #endif
        outDataSize = (outDataSize >> 2U) << 2U;
        rDMASize    = outDataSize;
        remainsize  -= outDataSize;
    }

#if 0
    (void)printf("rDMASize = %d\n", rDMASize);
    (void)usleep(2);    //It's nesscessary for NAND to avoid from DMA fail.
#endif

    do
    {
        if (spiDmaReadData_(rpaddr, rDMASize) == false)
        {
            (void)printf("Read fail!\n");
            while (true);
        }
        // Wait ilde
        while (0U == AxiSpiReadIntrStatus())
        {
            if (--timeOut < 0)
            {
                (void)printf("AxiSpiReadIntrStatus fail!\n");
                result = false;
                break;
            }
            else
            {
                (void)usleep(1);
            }
        }
        AxiSpiSetIntrStatus();
    } while (false);

    rpaddr += rDMASize;

#if 0
    (void)printf("remainsize = %d\n", remainsize);
#endif
    if (0U != remainsize)
    {
        AXISPIReadData(rpaddr, remainsize);
    }

    #ifndef CFG_NOR_USE_AXISPI
        #ifdef CFG_NAND_ENABLE

            #ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
    {
                #if 0
        ithWriteRegMaskA(0xD800006CUL, 1UL << 31UL, 1UL << 31UL);
        (void)usleep(10);
        ithWriteRegMaskA(0xD800006CUL, 0UL << 31UL, 1UL << 31UL);
        AxiSpiRestoreLastSetting();
                #endif
        // Turn on watchdog hw reset mode. Theoretically, hw reset suppose to
        // cover all cases out of axi spi access scope.
        ithWatchDogCtrlEnable(ITH_WD_RESET);
    }
            #else
                #if 0
    ithWriteRegMaskA(0xD800006CUL, 1UL << 31UL, 1UL << 31UL);
    (void)usleep(10);
    ithWriteRegMaskA(0xD800006CUL, 0UL << 31UL, 1UL << 31UL);
    AxiSpiRestoreLastSetting();
                #endif
    ithWatchDogCtrlEnable(ITH_WD_RESET);
            #endif

        #endif
    #endif

    pthread_mutex_unlock(&SpiObjects[port].mutex);
    #if 0
    (void)printf("--  DMA Read result=%d--\n", result);
    #endif
    return result;
}
#else
int32_t
mmpAxiSpiDmaReadMulti(
    SPI_PORT        port,
    SPI_CSN         cs,
    SPI_IOMODE      iomode,
    SPI_DTRMODE     dtrmode,
    uint8_t         *inCommand,
    SPI_CMD_LEN     inCommandLen,
    uint32_t        *slaveaddr,
    SPI_ADDR_LEN    slaveaddrLen,
    uint8_t         *outData,
    uint32_t        outDataSize)
{
    return mmpAxiSpiPioReadMulti(port, cs, iomode, dtrmode, inCommand, inCommandLen, slaveaddr, slaveaddrLen, outData, outDataSize);
}
#endif

int32_t
mmpAxiSpiPioWriteMulti(
    SPI_PORT        port,
    SPI_CSN         cs,
    SPI_IOMODE      iomode,
    uint8_t         *inCommand,
    SPI_CMD_LEN     inCommandLen,
    uint32_t        *slaveaddr,
    SPI_ADDR_LEN    slaveaddrLen,
    uint32_t        *inData,
    uint32_t        inDataSize)
{
    int32_t     result  = true;
    int32_t     timeOut = 100000;

    pthread_mutex_lock(&SpiObjects[port].mutex);

#ifndef CFG_NOR_USE_AXISPI
#ifdef CFG_NAND_ENABLE

#ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
        //Turn off watchdog hw reset mode, then use sw interrupt isr to handle inappropriate system unknown hanged issue inside
        //axispi access scope.
        ithWatchDogCtrlDisable(ITH_WD_RESET);
#else
    ithWatchDogCtrlDisable(ITH_WD_RESET);
#endif

#endif
#endif

#if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    if (spiSetGPIO_(port, cs) == false)
    {
        pthread_mutex_unlock(&SpiObjects[port].mutex);
        return false;
    }
#endif
#if 0
    (void)printf("++ pio Write ++\n");
#endif
    if (slaveaddr == NULL)
    {
        AxiSpiWriteAddr(0U);
    }
    else
    {
        AxiSpiWriteAddr(*slaveaddr);
    }
    AxiSpiWriteAddrLength(slaveaddrLen);
    AxiSpiWriteDataCounter(inDataSize);//Data Counter. (4 byte = 32bit)
    AxiSpiWriteInstLength(inCommandLen);
    AxiSpiWriteDummy(0);
    ithWriteRegMaskA(SPI_REG_CMD_W1, (1UL << 5UL), (1UL << 5UL)); //big endian

#if 1
    AxiSpiWriteInstCode4bitWriteEnable((uint32_t)*inCommand, iomode);
#else
    if ((uint32_t)*inCommand == 0x38 || (uint32_t)*inCommand == 0x3E)
    {
        AxiSpiWriteInstCode4bitWriteEnable((uint32_t)*inCommand);
    }
    else
    {
        AxiSpiWriteInstCodeWriteEnable((uint32_t)*inCommand);
    }
#endif

    do
    {
        if (AXISPIWriteData((uint8_t *)inData, inDataSize) == false)
        {
            (void)printf("-- AxiSpi Write Fail !!! ---\n");
        }

        while (0U == AxiSpiReadIntrStatus())
        {
            if (--timeOut < 0)
            {
                (void)printf("AxiSpiReadIntrStatus fail!\n");
                result = false;
                break;
            }
            else
            {
                (void)usleep(1);
            }
        }
        AxiSpiSetIntrStatus();
    } while (false);

#ifndef CFG_NOR_USE_AXISPI
#ifdef CFG_NAND_ENABLE

#ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
    {
        #if 0
        ithWriteRegMaskA(0xD800006C, 1UL << 31UL, 1UL << 31UL);
        (void)usleep(10);
        ithWriteRegMaskA(0xD800006C, 0UL << 31UL, 1UL << 31UL);
        AxiSpiRestoreLastSetting();
        #endif
        //Turn on watchdog hw reset mode. Theoretically, hw reset suppose to cover all cases out of axi spi access scope.
        ithWatchDogCtrlEnable(ITH_WD_RESET);
    }
#else
            #if 0
    ithWriteRegMaskA(0xD800006C, 1UL << 31UL, 1UL << 31UL);
    (void)usleep(10);
    ithWriteRegMaskA(0xD800006C, 0UL << 31UL, 1UL << 31UL);
    AxiSpiRestoreLastSetting();
            #endif
    ithWatchDogCtrlEnable(ITH_WD_RESET);
        #endif

    #endif
#endif

    pthread_mutex_unlock(&SpiObjects[port].mutex);

    return result;
}

int32_t
mmpAxiSpiPioWriteInISRMulti(
    SPI_PORT        port,
    SPI_CSN         cs,
    SPI_IOMODE      iomode,
    uint8_t         *inCommand,
    SPI_CMD_LEN     inCommandLen,
    uint32_t        *slaveaddr,
    SPI_ADDR_LEN    slaveaddrLen,
    uint32_t        *inData,
    uint32_t        inDataSize)
{
    int32_t     result  = true;
    uint32_t    value   = 0;
    int32_t     timeOut = 1000;

    if (slaveaddr == 0)
        AxiSpiWriteAddr(0);
    else
        AxiSpiWriteAddr(*slaveaddr);

    AxiSpiWriteAddrLength(slaveaddrLen);
    AxiSpiWriteDataCounter(inDataSize);     // Data Counter. (4 byte = 32bit)
    AxiSpiWriteInstLength(inCommandLen);
    AxiSpiWriteDummy(0);
    ithWriteRegMaskA(SPI_REG_CMD_W1, (1 << 5), (1 << 5)); //big endian

    AxiSpiWriteInstCode4bitWriteEnable((uint32_t)*inCommand, iomode);

    // iclai: For the command to switch nandflash to die2,
    // the length of the data field is 0, so there is no need
    // to call the AXISPIWriteData() function.
    //if (AXISPIWriteData((uint8_t *)inData, inDataSize) == false)
    //{
    //    printf("-- AxiSpi Write Fail !!! ---\n");
    //}

    while (0x00 == AxiSpiReadIntrStatus())
    {
        if (--timeOut < 0)
        {
            result = false;
            break;
        }
        else
        {
            ithDelay(1);
        }
    }
    AxiSpiSetIntrStatus();

    ithWriteRegMaskA(0xD800006C, 1 << 31, 1 << 31);
    ithDelay(10);
    ithWriteRegMaskA(0xD800006C, 0 << 31, 1 << 31);
    return result;
}

int32_t
mmpAxiSpiPioReadMulti(
    SPI_PORT        port,
    SPI_CSN         cs,
    SPI_IOMODE      iomode,
    SPI_DTRMODE     dtrmode,
    uint8_t         *inCommand,
    SPI_CMD_LEN     inCommandLen,
    uint32_t        *slaveaddr,
    SPI_ADDR_LEN    slaveaddrLen,
    uint32_t        *outData,
    uint32_t        outDataSize)
{
    int32_t result  = true;
    int32_t timeOut = 1000000;

    if ((inCommandLen == NONE_BYTE) || (inCommandLen == RESERVE_BYTE))
    {
        (void)printf("Error Instruction Length !!!\n");
        result = false;
        return result;
    }

#ifdef CFG_CPU_WB
    uint8_t *command    = (uint8_t *)itpVmemAlloc(inCommandLen);
#else
    uint8_t *command    = inCommand;
#endif

    if (NULL == command)
    {
        (void)printf("No enough memory\n");
        return false;
    }
#if 0
    if (outData == NULL || outDataSize == 0)
    {
        (void)printf("Parameter error! outData = 0x%08X, outDataSize = %d\n",
                     outData, outDataSize);
        return false;
    }
#endif

#ifdef CFG_CPU_WB
    (void)memcpy(command, inCommand, inCommandLen);
#endif

    pthread_mutex_lock(&SpiObjects[port].mutex);

#ifndef CFG_NOR_USE_AXISPI
    #ifdef CFG_NAND_ENABLE

        #ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
    {
        // Turn off watchdog hw reset mode, then use sw interrupt isr to handle
        // inappropriate system unknown hanged issue inside axispi access scope.
        ithWatchDogCtrlDisable(ITH_WD_RESET);
    }
        #else
    ithWatchDogCtrlDisable(ITH_WD_RESET);
        #endif

    #endif
#endif

#if defined(CFG_NOR_USE_AXISPI) || !defined(CFG_NAND_ENABLE)
    if (spiSetGPIO_(port, cs) == false)
    {
        pthread_mutex_unlock(&SpiObjects[port].mutex);
    #ifdef CFG_CPU_WB
        itpVmemFree((uint32_t)command);
    #endif
        return false;
    }
#endif

    if (slaveaddr == NULL)
    {
        AxiSpiWriteAddr(0U);
    }
    else
    {
        AxiSpiWriteAddr(*slaveaddr);
    }
    AxiSpiWriteAddrLength(slaveaddrLen);
    AxiSpiWriteDataCounter(outDataSize);//Data Counter. (4 byte = 32bit)
    AxiSpiWriteInstLength(inCommandLen);
    AxiSpiWriteDummy(0U);
#if 0
    ithWriteRegMaskA(SPI_REG_CMD_W1, (1UL << 4UL), (1UL << 4UL)); /* bit 4: RX fifo endian , bit 5 : TX Fifo*/
#endif
                                                                            //if ((uint32_t)*inCommand == 0x05) // Read Nor Status cmd
    if (((uint32_t)*inCommand == 0x05U) ||
        (((uint32_t)*inCommand == 0x0FU) &&
         (*slaveaddr == 0xC0U))) // Read Nor/Nand Status cmd
    {
        if (0U != outDataSize)
        {
            AxiSpiWriteInstCodeReadStatus((uint32_t)*inCommand);
        }
        else
        {
            AxiSpiWriteInstCodeReadStatusEnable((uint32_t)*inCommand);
        }
    }
    else
    {
#if 1
        if ((iomode == SPI_IOMODE_5) || (iomode == SPI_IOMODE_4) ||
            (iomode == SPI_IOMODE_2))
        {
            AxiSpiWriteDummy(gDummyByte[port][cs]);
        }

        AxiSpiWriteInstCode4bitRead(
            (uint32_t)*inCommand,
            (iomode == SPI_IOMODE_5) ? SPI_IOMODE_0 : iomode, dtrmode);
#else
        if ((uint32_t)*inCommand == 0xEB || (uint32_t)*inCommand == 0xEC)
        {
            AxiSpiWriteDummy(6);
            AxiSpiWriteInstCode4bitRead((uint32_t)*inCommand);
        }
        else
            AxiSpiWriteInstCodeRead((uint32_t)*inCommand);
#endif
    }

    do
    {
        if (AXISPIReadData((uint8_t *)outData, outDataSize) == false)
        {
            (void)printf("-- AxiSpi Read Fail !!! ---\n");
            while (true);
        }

        while (0U == AxiSpiReadIntrStatus())
        {
            if (--timeOut < 0)
            {
                (void)printf("AxiSpiReadIntrStatus fail!\n");
                result = false;
                break;
            }
            else
            {
                (void)usleep(1);
            }
        }

        //Read Status register(only for IT9860 & IT970)
        if (((uint32_t)*inCommand == 0x0FU) && (*slaveaddr==0xC0U) && (0U == outDataSize))
        {
            *outData = (uint8_t)ithReadRegA(SPI_REG_READ_STATUS);
        }

        AxiSpiSetIntrStatus();
#if 0
        (void)printf("## cmd =%x , 0x18=0x%08x\n", (uint32_t)*inCommand, ithReadRegA(REG_SPI_BASE+0x18U));
#endif
    } while (false);

#ifndef CFG_NOR_USE_AXISPI
    #ifdef CFG_NAND_ENABLE

        #ifdef CFG_WATCHDOG_ENABLE
    if (!WatchDogGetIsChipInfo())
    {
            #if 0
        ithWriteRegMaskA(0xD800006CUL, 1UL << 31UL, 1UL << 31UL);
        (void)usleep(10);
        ithWriteRegMaskA(0xD800006CUL, 0UL << 31UL, 1UL << 31UL);
        AxiSpiRestoreLastSetting();
            #endif
        // Turn on watchdog hw reset mode. Theoretically, hw reset suppose to
        // cover all cases out of axi spi access scope.
        ithWatchDogCtrlEnable(ITH_WD_RESET);
    }
        #else
            #if 0
    ithWriteRegMaskA(0xD800006C, 1UL << 31UL, 1UL << 31UL);
    (void)usleep(10);
    ithWriteRegMaskA(0xD800006C, 0UL << 31UL, 1UL << 31UL);
    AxiSpiRestoreLastSetting();
            #endif
    ithWatchDogCtrlEnable(ITH_WD_RESET);
        #endif

    #endif
#endif

    pthread_mutex_unlock(&SpiObjects[port].mutex);

#ifdef CFG_CPU_WB
    itpVmemFree((uint32_t)command);
#endif
    return result;
}

void
mmpAxiSpiResetControl(
    void)
{
#if 0
    pthread_mutex_unlock(&SwitchMutex);
    #if 0
    nor_using = false;
    #endif
#else
    return;
#endif
}

#if 0
static void
_SwitchSpiGpioPin(
    SPI_CONTROL_MODE controlMode)
{
    int pin_ori = gShareGpioTable[ctrl_mode];
    int pin_dst = gShareGpioTable[controlMode];

    if (pin_ori && pin_dst)
    {
        if (gSpiCtrlMethod == SPI_CONTROL_SLAVE)
        {
            ithGpioSet(pin_ori);
            ithGpioSetOut(pin_ori);

            ithGpioClear(pin_dst);
            ithGpioSetIn(pin_dst);
            (void)usleep(10);
        }
        else
        {
            ithGpioSet(pin_ori);
            ithGpioSetOut(pin_ori);

            ithGpioClear(pin_dst);
            ithGpioSetOut(pin_dst);
            (void)usleep(10);

            while (true)
            {
                if (!ithGpioGet(pin_dst) && ithGpioGet(pin_ori)) break;
            }
        }
    }
}
#endif

void
mmpAxiSpiInitShareGpioPin(
    unsigned int *pins)
{
    unsigned int i;

    (void)printf("init SPI share pin:[%x]\n", ITH_COUNT_OF(pins));
#if 0
    ithWriteRegMaskA( REG_SSP_FREERUN, (0x1UL << 2UL), (0x1UL << 2UL));
#endif
    pthread_mutex_init(&SwitchMutex, NULL);

    for (i = 0; i < SPI_SHARE_GPIO_MAX; i++)
    {
        if ((pins[i] < 64) && (pins[i] > 0))
        {
            gShareGpioTable[i] = pins[i];
        }
        else
        {
            gShareGpioTable[i] = 0;
        }

        (void)printf("pin[%x]=[%d,%d]\n", i, pins[i], gShareGpioTable[i]);
    }

    if (gShareGpioTable[SPI_CONTROL_NAND]) gSpiCtrlMethod = SPI_CONTROL_NAND;
    (void)printf("gSpiCtrlMethod = %x %d\n", gSpiCtrlMethod, SPI_CONTROL_NAND);
}

void
mmpAxiSpiSetControlMode(
    SPI_CONTROL_MODE controlMode)
{
#if 0
    uint32_t    mask    = 0U;
    uint32_t    value   = 0U;

    pthread_mutex_lock(&SwitchMutex);
    #if 0
    (void)printf("SetSpiGpio:[%x,%x]\n", controlMode, ctrl_mode);
    #endif

    if (controlMode != ctrl_mode)
    {
        _SwitchSpiGpioPin(controlMode);
        ctrl_mode = controlMode;
    }
#else
    return;
#endif
}

int32_t
mmpAxiSpiSetDrivingMulti(
    SPI_PORT    port,
    SPI_CSN     cs,
    ITHGpioDriving driving)
{
    pthread_mutex_lock(&SpiObjects[port].mutex);

    if (!spiAssign970Gpio_(port, cs, driving))
    {
        pthread_mutex_unlock(&SpiObjects[port].mutex);
        return 1;
    }

    pthread_mutex_unlock(&SpiObjects[port].mutex);

    return 0;
}
