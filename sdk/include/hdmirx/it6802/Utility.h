///*****************************************
//  Copyright (C) 2009-2018
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <Utility.h>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2018/05/10
//   @fileversion: ITE_MHLRX_SAMPLE_V1.27
//******************************************/

#ifndef _Utility_h_
#define _Utility_h_

#include "ite/itp.h"

void delay1ms(unsigned int ms);
int TimeOutCheck(unsigned int timer, unsigned int x);
BOOL IsTimeOut(unsigned int x);
unsigned int GetCurrentVirtualTime();

#endif
