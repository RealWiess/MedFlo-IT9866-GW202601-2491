/*
 * SW_SPI.c
 * High-Speed CAN Transceiver ATA6570 SPI protocol
 * Bit sampling is performed on the falling edge of the clock
 * Data is shifted in/out on the rising edge
 * Via SPI the ATA6570 can be configured and operated.
 */
#include "ssp/mmp_spi.h"
#include "ATA6570/sw_spi.h"
static int LOCAL_SPI_PORT = 0;

/*Init SPI only for ATA6570*/
void spi_init_ata6570(uint8_t port)
{
    #ifdef CFG_SPI_ENABLE
    mmpSpiInitialize(port, SPI_OP_MASTR, CPO_0_CPH_1, SPI_CLK_4M);
    (void)printf("ATA6570 spi inited\n");
    #endif
}

/*Just writes 8-Bit Data to the Chip*/
void  spi_write_ata6570(uint8_t port, uint8_t addr, uint8_t value)
{
    int32_t result = 0;
    uint8_t  SpiTxData[2] = {0};

    SpiTxData[0] = (addr << 1);
    SpiTxData[1] =  value;

    #ifdef CFG_SPI_ENABLE
	result = mmpSpiPioTransfer(port, SpiTxData, SpiTxData, 2);
    #endif
}

/*Ths reads Data and transmitts simply 0x00 to the device, data will be read back and returnd as 8-Bit Transfer*/
uint8_t spi_read_ata6570(uint8_t port, uint8_t addr)
{
    uint8_t  SpiTxData[2] = {0};
    uint8_t   SpiRxData[2] = {0};
    SpiTxData[0] =  (addr << 1) | 0x1;
    SpiTxData[1] =  0x0;
    #ifdef CFG_SPI_ENABLE
    if (mmpSpiPioTransfer(port, SpiTxData, SpiRxData, 2) == 0)
    {
        return SpiRxData[1];
    }
    #endif
    return 0xff;
}
