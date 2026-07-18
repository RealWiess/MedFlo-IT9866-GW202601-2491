///*****************************************
//  Copyright (C) 2009-2019
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IO.h>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2019/10/31
//   @fileversion: ITE_HDMI1.4_RXSAMPLE_1.27
//******************************************/

#ifndef _IO_h_
#define _IO_h_
#include "typedef.h"


#ifdef SUPPORT_UART_CMD
void UartCommand();
#endif



/****************************************************************************/
//Copy from Mcu.h

/****************************************************************************/

#define IT6602A0_HDMI_ADDR 0x94	//Hardware Fixed I2C address of IT6602 HDMI
#define IT6602B0_HDMI_ADDR 0x92	//Hardware Fixed I2C address of IT6602 HDMI
#define EDID_ADDR 0xA8	//Software programmable I2C address of IT6602 EDID RAM
#define CEC_ADDR 0xC8	//Software programmable I2C address of IT6602 CEC


#define DP_ADDR 0x90
#define ADC_ADDR 0x90



#define HDMI_DEV  0
#define DP_DEV	  0

#define RXDEV           0
#define MAXRXDEV        1
/****************************************************************************/

/****************************************************************************/
#define HDMI_DEV		0
#define SWADR	 	0x96
#define EDID_HEAD	0xA0

#define DELAY_TIME        1
#define IDLE_TIME        100

#define HIGH            1
#define LOW                0
#define ACTIVE          1
#define DISABLE         0

/****************************************************************************/
/****************************************************************************/


//BYTE HDMI_IIC_Read( BYTE RegAddr);
//BOOL HDMI_IIC_Write( BYTE RegAddr,BYTE DataIn);
//BYTE HDMI_IIC_SET( BYTE offset, BYTE mask, BYTE datain );
//BOOL i2c_write_byte( BYTE address,BYTE offset,BYTE byteno,BYTE *p_data,BYTE device );
//BOOL i2c_read_byte( BYTE address,BYTE offset,BYTE byteno,BYTE *p_data,BYTE device );

//###ADD API###
SYS_STATUS HDMIRX_ReadI2C_ByteN(unsigned char iicaddress, unsigned char RegAddr,unsigned char *pData,int N);

SYS_STATUS HDMIRX_WriteI2C_ByteN(unsigned char iicaddress, unsigned char RegAddr,unsigned char *pData,int N);
//###ADD API###


#ifdef Enable_IT6602_CEC
#include "debug.h"
unsigned char IT6602_CEC_ReadI2C_Byte(BYTE RegAddr);
SYS_STATUS IT6602_CEC_WriteI2C_Byte(BYTE offset,BYTE buffer );
#endif


//#define hdmirxrd(x) HDMI_IIC_Read(x)
//#define hdmirxwr(x,y) HDMI_IIC_Write(x,y)



void SetEDIDWp(BOOL ENA);


void HotPlug(unsigned char Enable);

void init_printf(void);

void SetintActive(BOOL bactive);


#endif
