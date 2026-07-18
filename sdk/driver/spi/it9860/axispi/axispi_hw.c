#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "axispi_hw.h"
//#include "axispi_msg.h"
#include "ite/ith.h"

static uint32_t gLastAxiCtrlRegVal = 0U;
static uint32_t gLastAxiIntrRegVal = 0U;

#if 0
void SPIRunRDX1()
{
    ithWriteRegMaskA(AXISPI_BASE+0x00U, address);//SPI Flash Address.
    ithWriteRegMaskA(AXISPI_BASE+0x04U, 0x01000003U);//Instruction length, address length(3 byte).
    ithWriteRegMaskA(AXISPI_BASE+0x08U, datasize);//Data Counter. (1 byte = 8bit)                                               //
    ithWriteRegMaskA(AXISPI_BASE+0x0cU, 0x00000000U+(cmd << 24U) );//SPI command(02h, X1 PP)
}
#endif

uint32_t AxiSPIReadAddr(void)
{
    return ithReadRegA(SPI_REG_CMD_W0);
}

void AxiSpiWriteAddr(uint32_t data)
{
#if 0
    (void)printf("WriteAddr 0x%02" PRIx32 "\n", data);
#endif
    ithWriteRegA(SPI_REG_CMD_W0, data);
}

void AxiSpiWriteInstLength(uint32_t data)
{
#if 0
    (void)printf("InstLength 0x%02" PRIx32 "\n", data);
#endif
    ithWriteRegMaskA(SPI_REG_CMD_W1, (data << SPI_SHT_INST_LENGTH), SPI_MSK_INST_LENGTH);
}

void AxiSpiWriteAddrLength(uint32_t data)
{
#if 0
    (void)printf("AddrLength 0x%02" PRIx32 "\n", data);
#endif
    ithWriteRegMaskA(SPI_REG_CMD_W1, (data << SPI_SHT_ADDR_LENGTH), SPI_MSK_ADDR_LENGTH);
}

void AxiSpiWriteDummy(uint32_t data)
{
    ithWriteRegMaskA(SPI_REG_CMD_W1, (data << SPI_SHT_DUMMY_CYCLE), SPI_MSK_DUMMY_CYCLE);
}

void AxiSpiWriteDataCounter(uint32_t data)
{
#if 0
    (void)printf("DataCounter 0x%02" PRIx32 "\n", data);
#endif
    ithWriteRegA(SPI_REG_CMD_W2, data);
}

void AxiSpiWriteData(uint32_t data)
{
#if 0
    (void)printf("WData 0x%02" PRIx32 "\n", data);
#endif
    ithWriteRegA(SPI_REG_DATA_PORT, data);
}

uint32_t AxiSpiReadData(void)
{
    return ithReadRegA(SPI_REG_DATA_PORT);
}

uint32_t AxiSpiReadDataCounter(void)
{
    return ithReadRegA(SPI_REG_CMD_W2);
}

uint32_t AxiSpiReadTXFIFOStatus(void)
{
    uint32_t value;
    value = ithReadRegA(SPI_REG_STATUS) & SPI_MSK_TXFIFO_READY;
    if (0U == value)
    {
        (void)printf("TX FIFO FULL!\n");
    }

    return value;
}

#if 0
uint32_t AxiSpiReadRXFIFOStatus(void)
{
    uint32_t value;
    value = ithReadRegA(SPI_REG_STATUS) & SPI_MSK_RXFIFO_READY;
    if (0U == value)
    {
        (void)printf("RX FIFO FULL!\n");
    }

    return value;
}
#endif

uint32_t AxiSpiReadTXFIFOValidSpace(void)
{
    uint32_t value;
    value = (ithReadRegA(SPI_REG_STATUS) & SPI_MSK_TXFIFO_VALID_SPACE) >> SPI_SHT_TXFIFO_VALID_SPACE;
    if (0U == value)
    {
        (void)printf("TX FIFO is full!!\n");
    }
    return value;
}

uint32_t AxiSpiReadRXFIFODataCount(void)
{
    uint32_t value;
    value = (ithReadRegA(SPI_REG_STATUS) & SPI_MSK_RXFIFO_DATA_COUNT) >> SPI_SHT_RXFIFO_DATA_COUNT;

    return value;
}

uint32_t AxiSpiReadIntrStatus(void)
{
    uint32_t value = 0U;
    value = (ithReadRegA(SPI_REG_INTR_STATUS) & SPI_MSK_CMD_CMPLT_STATUS);
#if 0
    (void)printf("ReadIntr=0x%x\n",value);
#endif
    return value;
}

void AxiSpiSetIntrStatus(void)
{
    ithWriteRegMaskA(SPI_REG_INTR_STATUS, 0x1U, SPI_MSK_CMD_CMPLT_STATUS);
}

void AxiSpiWriteInstCodeRead(uint32_t data)
{
    uint32_t value = 0U;

    value |= (data << SPI_SHT_INST_CODE);
#if 0
    (void)printf("WriteInstCode data=0x%02" PRIx32 ",value=0x%x\n", data,value);
#endif
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiWriteInstCode4bitRead(uint32_t data, SPI_IOMODE iomode, SPI_DTRMODE dtrmode)
{
    uint32_t value = 0U;

    value   |= (data << SPI_SHT_INST_CODE);
    value   |= (iomode << SPI_SHT_OPERATE_MODE);
    value   |= (dtrmode << SPI_SHT_EN_DTR_MODE);
#if 0
    (void)printf("WriteInstCode data=0x%02" PRIx32 ",value=0x%x\n", data,value);
#endif
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiWriteInstCodeReadStatusEnable(uint32_t data)
{
    uint32_t value = 0U;

    value   |= (data << SPI_SHT_INST_CODE);
    value   |= (0x1UL << SPI_SHT_EN_READ_STATUS);
#if 0
    (void)printf("AxiSpiWriteInstCodeReadStatusEnable data=0x%02" PRIx32 ",value=0x%x\n", data,value);
#endif
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiWriteInstCodeReadStatus(uint32_t data)
{
    uint32_t value = 0U;

    value   |= (data << SPI_SHT_INST_CODE);
    value   |= (0x1UL << SPI_SHT_READ_STATUS);
#if 0
    (void)printf("AxiSpiWriteInstCodeReadStatus data=0x%02" PRIx32 ",value=0x%x\n", data,value);
#endif
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiWriteInstCodeWriteEnable(uint32_t data)
{
    uint32_t value = 0U;

    value   |= (data << SPI_SHT_INST_CODE);
    value   |= (0x1UL << SPI_SHT_EN_WRITE);
#if 0
    (void)printf("AxiSpiWriteInstCodeWriteEnable data=0x%02" PRIx32 ",value=0x%x\n", data,value);
#endif
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiWriteInstCodeWriteDisable(uint32_t data)
{
    uint32_t value = 0U;

    value   |= (data << SPI_SHT_INST_CODE);
    value   |= (0x0UL << SPI_SHT_EN_WRITE);
#if 0
    (void)printf("AxiSpiWriteInstCodeWriteDisable data=0x%02" PRIx32 ",value=0x%x\n", data,value);
#endif
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiWriteInstCode4bitWriteEnable(uint32_t data, SPI_IOMODE iomode)
{
    uint32_t value = 0U;

    value   |= (data << SPI_SHT_INST_CODE);
    value   |= (0x1UL << SPI_SHT_EN_WRITE);
    value   |= (iomode << SPI_SHT_OPERATE_MODE);
#if 0
    (void)printf("AxiSpiWriteInstCode4bitWriteEnable data=0x%02" PRIx32 ",value=0x%x\n", data,value);
#endif
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiWriteInstCode4bitWriteDisable(uint32_t data)
{
    uint32_t value = 0U;

    value   |= (data << SPI_SHT_INST_CODE);
    value   |= (0x0UL << SPI_SHT_EN_WRITE);
    value   |= (0x4UL << SPI_SHT_OPERATE_MODE);
    (void)printf("AxiSpiWriteInstCodeWriteDisable data=0x%02" PRIx32 ",value=0x%" PRIx32 "\n", data, value);
    ithWriteRegA(SPI_REG_CMD_W3, value);
}

void AxiSpiSelectChipSelect(uint32_t data)
{
    (void)printf("AxiSpiSelectChipSelect 0x%02" PRIx32 "\n", data);
    ithWriteRegMaskA(SPI_REG_CMD_W3, (data << SPI_SHT_START_CS), SPI_MSK_START_CS);
}

void AxiSpiSetOperateMode(uint32_t data)
{
    (void)printf("AxiSpiSetOperateMode 0x%02" PRIx32 "\n", data);
    ithWriteRegMaskA(SPI_REG_CMD_W3, (data << SPI_SHT_OPERATE_MODE), SPI_MSK_OPERATE_MODE);
}

void AxiSpiDTRModeEnable(void)
{
    (void)printf("AxiSpiDTRModeEnable\n");
    ithWriteRegMaskA(SPI_REG_CMD_W3, (0x1UL << SPI_SHT_EN_DTR_MODE), SPI_MSK_EN_DTR_MODE);
}

void AxiSpiSetClkDiv(uint32_t data)
{
    (void)printf("AxiSpiSetClkDivider 0x%02" PRIx32 "\n", data);
    ithWriteRegMaskA(SPI_REG_CTL, (data << SPI_SHT_CLK_DIV), SPI_MSK_CLK_DIV);
    gLastAxiCtrlRegVal = ithReadRegA(SPI_REG_CTL);
}

void AxiSpiSetBusyBit(uint32_t data)
{
    (void)printf("AxiSpiSetBusyBit 0x%02" PRIx32 "\n", data);
    ithWriteRegMaskA(SPI_REG_CTL, (data << SPI_SHT_RDY_LOC), SPI_MSK_RDY_LOC);
    gLastAxiCtrlRegVal = ithReadRegA(SPI_REG_CTL);
}

#if 0
void AxiSpiWriteEnable(void)
{
    (void)printf("AxiSpiWriteEnable\n");
    ithWriteRegMaskA(SPI_REG_CMD_W3, (0x1UL << SPI_SHT_EN_WRITE), SPI_MSK_EN_WRITE);
}

void AxiSpiWriteDisable(void)
{
    (void)printf("AxiSpiWriteDisable\n");
    ithWriteRegMaskA(SPI_REG_CMD_W3, (0x0UL << SPI_SHT_EN_WRITE), SPI_MSK_EN_WRITE);
}

void AxiSpiReadStatusEnable(void)
{
    (void)printf("AxiSpiReadStatusEnable\n");
    ithWriteRegMaskA(SPI_REG_CMD_W3, (0x1UL << SPI_SHT_EN_READ_STATUS), SPI_MSK_EN_READ_STATUS);
}

#endif

void AxiSpiDMAEnable(void)
{
#if 0
    (void)printf("AxiSpiDMAEnable\n");
#endif
    ithWriteRegMaskA(SPI_REG_INTR_CTL, 0x1U, N01_BITS_MSK);
    gLastAxiIntrRegVal = ithReadRegA(SPI_REG_INTR_CTL);
}

void AxiSpiDMADisable(void)
{
#if 0
    (void)printf("AxiSpiDMADisable\n");
#endif
    ithWriteRegMaskA(SPI_REG_INTR_CTL, 0x0U, N01_BITS_MSK);
    gLastAxiIntrRegVal = ithReadRegA(SPI_REG_INTR_CTL);
}

void AxiSpiSetRxFifoThod(uint32_t value)
{
    ithWriteRegMaskA(SPI_REG_INTR_CTL, value << SPI_SHT_RXFIFO_THOD, SPI_MSK_RXFIFO_THOD);
    gLastAxiIntrRegVal = ithReadRegA(SPI_REG_INTR_CTL);
}

void AxiSpiCmdCompletIntrEnable(void)
{
    (void)printf("AxiSpiCmdCompletIntrEnable\n");
    ithWriteRegMaskA(SPI_REG_INTR_CTL, (0x1UL << SPI_SHT_EN_CMD_CMPLT_INTR), SPI_MSK_EN_CMD_CMPLT_INTR);
    gLastAxiIntrRegVal = ithReadRegA(SPI_REG_INTR_CTL);
}

void AxiSpiRestoreLastSetting(void)
{
    ithWriteRegA(SPI_REG_CTL, gLastAxiCtrlRegVal);
    ithWriteRegA(SPI_REG_INTR_CTL, gLastAxiIntrRegVal);
}
