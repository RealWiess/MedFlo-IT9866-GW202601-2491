/*
 * ATA_API.c
 * High-Speed CAN Transceiver ATA6570
 * Standard CAN Data Rate up to 1 Mbit/s
 *
 */
#include "ATA6570/sw_spi.h"
#include "ATA6570/ata_api.h"

void ithATA6570Init(uint8_t port, ATA6570_CAN_BAUDRATE datarate)
{
	uint8_t SESR_REG = 0;
	uint8_t TRXESR_REG = 0;
    uint8_t WKESR_REG = 0;

	spi_init_ata6570(port);

    if(_ATA6570_ADDR ==  spi_read_ata6570(port, _ATA6570_DIDR))
    {
        (void)printf("ATA6570 online\n");
		//Transceiver Event Status
		TRXESR_REG = spi_read_ata6570(port, _ATA6570_TRXESR);
		if(TRXESR_REG & 0x1)
		{
			spi_write_ata6570(port,_ATA6570_TRXESR, TRXESR_REG); //clear CAN bus wake-up event status
			TRXESR_REG = spi_read_ata6570(port, _ATA6570_TRXESR);
			(void)printf("ATA6570_TRXESR_REG = 0x%x\n",TRXESR_REG);

		}

		WKESR_REG = spi_read_ata6570(port, _ATA6570_WKESR);
		if(WKESR_REG & 0x3)
		{
			spi_write_ata6570(port,_ATA6570_WKESR,  WKESR_REG); //clear wake pin event status
			WKESR_REG = spi_read_ata6570(port, _ATA6570_WKESR);
			(void)printf("ATA6570_WKESR_REG  = 0x%x\n",WKESR_REG);
		}

	    spi_write_ata6570(port,_ATA6570_SECR,   0x04); //enable overtemperature prewarning event capture

		//System Event Status
	    SESR_REG = spi_read_ata6570(port, _ATA6570_SESR);//bit4:Power on status
	    (void)printf("ATA6570_SESR_REG = 0x%x\n",SESR_REG);
	    spi_write_ata6570(port,_ATA6570_SESR,   SESR_REG); //clear all status.

		//Set Data Rate
	    spi_write_ata6570(port,_ATA6570_DRCR,   datarate); //can data rate

	    switch(datarate)
	    {
	        case ATA6570_CAN_50K:
	    		(void)printf("Set ATA6570 Data Rate 50K\n");
				break;
			case ATA6570_CAN_100K:
				(void)printf("Set ATA6570 Data Rate 100K\n");
				break;
			case ATA6570_CAN_125K:
				(void)printf("Set ATA6570 Data Rate 125K\n");
				break;
			case ATA6570_CAN_250K:
				(void)printf("Set ATA6570 Data Rate 250K\n");
				break;
			case ATA6570_CAN_500K:
				(void)printf("Set ATA6570 Data Rate 500K\n");
				break;
			case ATA6570_CAN_1000K:
				(void)printf("Set ATA6570 Data Rate 1000K\n");
				break;

	    }
		//CAN Frame Config Reg
	    spi_write_ata6570(port,_ATA6570_CFCR,   0xC8); //IDE = 1 ,PNDM = 1, DLC = 8
	    //CAN ID Reg
	    spi_write_ata6570(port,_ATA6570_CIDR0,  0x00);//bit[7:0] (ID07 to ID00 of the extened frame format)
	    spi_write_ata6570(port,_ATA6570_CIDR1,  0x00);//bit[7:0] (ID15 to ID08 of the extened frame format)

	    spi_write_ata6570(port,_ATA6570_CIDR2,  0x00); //bit[7:0] (ID23 to ID16 of the extened frame format)
	    											   //bit[7:2] (ID05 to ID00 of the standard frame format)

	    spi_write_ata6570(port,_ATA6570_CIDR3,  0x00); //bit[4:0] (ID28 to ID24 of the standard frame format)
	                                                   //bit[4:0] (ID10 to ID06 of the standard frame format)
	    //CAN ID Mask Reg
	    spi_write_ata6570(port,_ATA6570_CIDMR0, 0x00);
	    spi_write_ata6570(port,_ATA6570_CIDMR1, 0x00);
	    spi_write_ata6570(port,_ATA6570_CIDMR2, 0x00);
	    spi_write_ata6570(port,_ATA6570_CIDMR3, 0x00);
		//CAN Data Mask Reg
	    spi_write_ata6570(port,_ATA6570_CDMR0,  0xFF);
	    spi_write_ata6570(port,_ATA6570_CDMR1,  0xFF);
	    spi_write_ata6570(port,_ATA6570_CDMR2,  0xFF);
	    spi_write_ata6570(port,_ATA6570_CDMR3,  0xFF);
	    spi_write_ata6570(port,_ATA6570_CDMR4,  0xFF);
	    spi_write_ata6570(port,_ATA6570_CDMR5,  0xFF);
	    spi_write_ata6570(port,_ATA6570_CDMR6,  0xFF);
	    spi_write_ata6570(port,_ATA6570_CDMR7,  0xFF);

		//CAN Transceiver Control Reg
	    spi_write_ata6570(port,_ATA6570_TRXCR,  0x71); // bit5:PNCFOK = 1,bit4: CPNE = 1, bit[1:0] COPM = 1 ,TRX NORMAL MODE = 0x1

    }
	else
	{
		(void)printf("ATA6570 offline\n");
	}

}

ATA6570_CAN_OPMODE ithATA6570GetOPMode(uint8_t port)
{
    uint8_t  DMCR_REG = 0;
    DMCR_REG = spi_read_ata6570(port, _ATA6570_DMCR);
    return DMCR_REG & 0x7;
}

uint8_t ithATA6570GetModeStatus(uint8_t port)
{
    uint8_t  DMSR_REG = 0;
    DMSR_REG = spi_read_ata6570(port, _ATA6570_DMSR);
    return DMSR_REG;
}

ATA6570_CAN_COMODE ithATA6570GetCOMode(uint8_t port)
{
    uint8_t  TRXCR_REG = 0;
    TRXCR_REG = spi_read_ata6570(port, _ATA6570_TRXCR);
    return TRXCR_REG & 0x3;
}

void ithATA6570SetOPMode(uint8_t port, ATA6570_CAN_OPMODE mode)
{
	uint8_t TRXESR_REG = 0;
    uint8_t WKESR_REG  = 0;

	switch(mode)
	{
		case ATA6570_OPMODE_SLEEP:
		//Transceiver Event Status
		TRXESR_REG = spi_read_ata6570(port, _ATA6570_TRXESR);
		if(TRXESR_REG & 0x1)
		{
			spi_write_ata6570(port,_ATA6570_TRXESR, TRXESR_REG); //clear CAN bus wake-up event status
			TRXESR_REG = spi_read_ata6570(port, _ATA6570_TRXESR);
			(void)printf("ATA6570_TRXESR_REG = 0x%x\n",TRXESR_REG);

		}

		WKESR_REG = spi_read_ata6570(port, _ATA6570_WKESR);
		if(WKESR_REG & 0x3)
		{
			spi_write_ata6570(port,_ATA6570_WKESR,	WKESR_REG); //clear wake pin event status
			WKESR_REG = spi_read_ata6570(port, _ATA6570_WKESR);
			(void)printf("ATA6570_WKESR_REG  = 0x%x\n",WKESR_REG);
		}

		//Wakeup Event Setting
		//1.CANBUS wakeup
		spi_write_ata6570(port,_ATA6570_TRXECR, 0x01); //CAN bus wake-up detection enable(CWUE = 1)
		//2.Wakup pin
		spi_write_ata6570(port,_ATA6570_WKECR, 0x02); //enable wake pin rising edge detection interrupt
		//spi_write_ata6570(port,_ATA6570_WKECR, 0x01); //enable wake pin falling edge detection interrupt
		break;
	}
	//Device Mode Control Reg
    spi_write_ata6570(port,_ATA6570_DMCR,  mode); //mode change
}
