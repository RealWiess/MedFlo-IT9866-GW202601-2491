///*****************************************
//  Copyright (C) 2009-2019
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <Utility.h>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2019/10/31
//   @fileversion: ITE_HDMI1.4_RXSAMPLE_1.27
//******************************************/

#ifndef _Utility_h_
#define _Utility_h_

#include "ite/itp.h"


void delay1ms(unsigned short ms);
int TimeOutCheck(unsigned int timer, unsigned int x);
BOOL IsTimeOut(unsigned int x);
unsigned int GetCurrentVirtualTime();

#endif
