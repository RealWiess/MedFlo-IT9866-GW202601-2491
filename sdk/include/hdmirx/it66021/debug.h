///*****************************************
//  Copyright (C) 2009-2019
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <debug.h>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2019/10/31
//   @fileversion: ITE_HDMI1.4_RXSAMPLE_1.27
//******************************************/

#ifndef _DEBUG_H_
#define _DEBUG_H_


#define Debug_message 1

//#ifndef Debug_message
//#define Debug_message 1
//#endif


#if Debug_message
    #include <stdio.h>
    #define EQ_DEBUG_PRINTF(x) // printf  x
    #define EQ_PORT0_PRINTF(x) // printf  x
    #define EQ_PORT1_PRINTF(x) // printf  x
    #define VIDEOTIMNG_DEBUG_PRINTF(x)   printf  x
    #define IT6602_DEBUG_INT_PRINTF(x)   printf  x
    #define IT6602_DEBUG_INT_PRINTF1(x)   //printf  x
    #define IT6602_DEBUG_INT_PRINTF2(x)   //printf  x
    #define IT6602_DEBUG_INT_PRINTF3(x)   // printf  x
    #define HDMIRX_VIDEO_PRINTF(x)   printf  x
    #define HDMIRX_AUDIO_PRINTF(x)  //printf  x
    #define HDMIRX_DEBUG_PRINT(x) //printf x
    #define CEC_DEBUG_PRINTF(x) //printf x
    #define EDID_DEBUG_PRINTF(x) //printf  x
    #define it660x_DEBUG_PRINTF(x)  printf  x
    #define VSDB_DEBUG_PRINTF(x) //printf  x
    #define RXHDCP_DEBUG_PRINT(x) printf x

    #define HDCP_DEBUG_PRINTF(x) //printf x
    #define HDCP_DEBUG_PRINTF1(x) //printf x
    #define HDCP_DEBUG_PRINTF2(x) //printf x

#else
    #define EQ_DEBUG_PRINTF(x)
    #define EQ_PORT0_PRINTF(x)
    #define EQ_PORT1_PRINTF(x)
    #define VIDEOTIMNG_DEBUG_PRINTF(x)
    #define IT6602_DEBUG_INT_PRINTF(x)
    #define IT6602_DEBUG_INT_PRINTF1(x)
    #define IT6602_DEBUG_INT_PRINTF2(x)
    #define IT6602_DEBUG_INT_PRINTF3(x)
    #define HDMIRX_VIDEO_PRINTF(x)
    #define HDMIRX_AUDIO_PRINTF(x)
    #define HDMIRX_DEBUG_PRINT(x)
    #define CEC_DEBUG_PRINTF(x)
    #define EDID_DEBUG_PRINTF(x)
    #define it660x_DEBUG_PRINTF(x)
    #define VSDB_DEBUG_PRINTF(x)
    #define RXHDCP_DEBUG_PRINT(x)
#endif




#endif
