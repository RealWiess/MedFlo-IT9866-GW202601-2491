///*****************************************
//  Copyright (C) 2009-2019
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <config.h>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2019/10/31
//   @fileversion: ITE_HDMI1.4_RXSAMPLE_1.27
//******************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define _MCU_8051_

//#ifndef _MCU_8051_
//#include "platform.h"
//#endif

#ifdef _IT66021_
#pragma message("defined _IT66021_")
#endif


#ifdef _IT6602_
#pragma message("defined _IT6602_")
#endif


//#define SUPPORT_I2C_SLAVE
#ifdef SUPPORT_I2C_SLAVE
#pragma message ("SUPPORT_I2C_SLAVE defined")
#endif

#define _EN_DUAL_PIXEL_CTRL_
#define _EN_BLOCK_PWRDN_
#define SUPPORT_OUTPUTRGB
#define SUPPORT_INPUTYUV

#endif // _CONFIG_H_
