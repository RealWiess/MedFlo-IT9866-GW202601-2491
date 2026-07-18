/*
 * Copyright © 2020 NXP.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** 
* \author   XML Parser V1.2
* \version  V1.1
* \date     2020/09/18
*/

//--------------------------------------------------------------------------------
// pre-processor directive
#ifndef NXP_TJA1145AFD_SIM_H
#define NXP_TJA1145AFD_SIM_H

//--------------------------------------------------------------------------------
// function header include
#include "NXP_TJA1145AFD_Functions.h"

//--------------------------------------------------------------------------------
// register depending structures
//--------------------------------------------------------------------------------
// register 0x01
typedef struct
{
	TJA1145AFD_Mode_Control_t	TJA1145AFD_MC;
}TJA1145AFD_Mode_Control_Reg_t;
//---------------------------------------------------
// register 0x03
typedef struct
{
	TJA1145AFD_Forced_Sleep_Mode_Status_t	TJA1145AFD_FSMS;
	TJA1145AFD_Over_Temperature_Warning_Status_t	TJA1145AFD_OTWS;
	TJA1145AFD_Normal_Mode_Status_t	TJA1145AFD_NMS;
}TJA1145AFD_Main_Status_Reg_t;
//---------------------------------------------------
// register 0x04
typedef struct
{
	TJA1145AFD_Over_Temperature_Warning_Enable_t	TJA1145AFD_OTWE;
	TJA1145AFD_SPI_Failure_Detect_Enable_t	TJA1145AFD_SPIFE;
}TJA1145AFD_System_Event_Enable_Reg_t;
//---------------------------------------------------
// register 0x06
typedef struct
{
	TJA1145AFD_RAM_Memory_0700_t	TJA1145AFD_GPM0;
}TJA1145AFD_Memory_0_Reg_t;
//---------------------------------------------------
// register 0x07
typedef struct
{
	TJA1145AFD_RAM_Memory_0815_t	TJA1145AFD_GPM1;
}TJA1145AFD_Memory_1_Reg_t;
//---------------------------------------------------
// register 0x08
typedef struct
{
	TJA1145AFD_RAM_Memory_1623_t	TJA1145AFD_GPM2;
}TJA1145AFD_Memory_2_Reg_t;
//---------------------------------------------------
// register 0x09
typedef struct
{
	TJA1145AFD_RAM_Memory_2431_t	TJA1145AFD_GPM3;
}TJA1145AFD_Memory_3_Reg_t;
//---------------------------------------------------
// register 0x0A
typedef struct
{
	TJA1145AFD_Lock_Control_6_t	TJA1145AFD_LK6C;
	TJA1145AFD_Lock_Control_5_t	TJA1145AFD_LK5C;
	TJA1145AFD_Lock_Control_4_t	TJA1145AFD_LK4C;
	TJA1145AFD_Lock_Control_3_t	TJA1145AFD_LK3C;
	TJA1145AFD_Lock_Control_2_t	TJA1145AFD_LK2C;
	TJA1145AFD_Lock_Control_1_t	TJA1145AFD_LK1C;
	TJA1145AFD_Lock_Control_0_t	TJA1145AFD_LK0C;
}TJA1145AFD_Lock_Control_Reg_t;
//---------------------------------------------------
// register 0x20
typedef struct
{
	TJA1145AFD_CAN_FD_tolerance_t	TJA1145AFD_CFDC;
	TJA1145AFD_Partial_Network_Config_t	TJA1145AFD_PNCOK;
	TJA1145AFD_CAN_Selective_WakeUp_t	TJA1145AFD_CPNC;
	TJA1145AFD_CAN_Mode_Selection_t	TJA1145AFD_CMC;
}TJA1145AFD_CAN_Control_Reg_t;
//---------------------------------------------------
// register 0x22
typedef struct
{
	TJA1145AFD_CAN_Transmitter_Status_t	TJA1145AFD_CTS;
	TJA1145AFD_Partial_Network_Error_t	TJA1145AFD_CPNERR;
	TJA1145AFD_Partial_Network_Config_Error_t	TJA1145AFD_CPNS;
	TJA1145AFD_Partial_Network_Osc_t	TJA1145AFD_COSCS;
	TJA1145AFD_CAN_Bus_Status_t	TJA1145AFD_CBSS;
	TJA1145AFD_VCAN_Status_t	TJA1145AFD_VCS;
	TJA1145AFD_Dominant_Timeout_Event_t	TJA1145AFD_CFS;
}TJA1145AFD_Transceiver_Status_Reg_t;
//---------------------------------------------------
// register 0x23
typedef struct
{
	TJA1145AFD_CAN_Bus_Silence_Detect_t	TJA1145AFD_CBSE;
	TJA1145AFD_CAN_Failure_Detect_t	TJA1145AFD_CFE;
	TJA1145AFD_CAN_WakeUp_Detect_t	TJA1145AFD_CWE;
}TJA1145AFD_Transceiver_Event_Enable_Reg_t;
//---------------------------------------------------
// register 0x26
typedef struct
{
	TJA1145AFD_CAN_Data_Rate_t	TJA1145AFD_CDR;
}TJA1145AFD_Data_Rate_Reg_t;
//---------------------------------------------------
// register 0x27
typedef struct
{
	TJA1145AFD_CAN_ID0700_t	TJA1145AFD_ID0700;
}TJA1145AFD_CAN_Identifier_0_Reg_t;
//---------------------------------------------------
// register 0x28
typedef struct
{
	TJA1145AFD_CAN_ID1508_t	TJA1145AFD_ID1508;
}TJA1145AFD_CAN_Identifier_1_Reg_t;
//---------------------------------------------------
// register 0x29
typedef struct
{
	TJA1145AFD_CAN_ID2318_t	TJA1145AFD_ID2318;
	TJA1145AFD_CAN_ID1716_t	TJA1145AFD_ID1716;
}TJA1145AFD_CAN_Identifier_2_Reg_t;
//---------------------------------------------------
// register 0x2A
typedef struct
{
	TJA1145AFD_CAN_ID2824_t	TJA1145AFD_ID2824;
}TJA1145AFD_CAN_Identifier_3_Reg_t;
//---------------------------------------------------
// register 0x2B
typedef struct
{
	TJA1145AFD_CAN_M0700_t	TJA1145AFD_M0700;
}TJA1145AFD_CAN_Mask_0_Reg_t;
//---------------------------------------------------
// register 0x2C
typedef struct
{
	TJA1145AFD_CAN_M1508_t	TJA1145AFD_M1508;
}TJA1145AFD_CAN_Mask_1_Reg_t;
//---------------------------------------------------
// register 0x2D
typedef struct
{
	TJA1145AFD_CAN_M2318_t	TJA1145AFD_M2318;
	TJA1145AFD_CAN_M1716_t	TJA1145AFD_M1716;
}TJA1145AFD_CAN_Mask_2_Reg_t;
//---------------------------------------------------
// register 0x2E
typedef struct
{
	TJA1145AFD_CAN_M2824_t	TJA1145AFD_M2824;
}TJA1145AFD_CAN_Mask_3_Reg_t;
//---------------------------------------------------
// register 0x2F
typedef struct
{
	TJA1145AFD_Identifier_Format_t	TJA1145AFD_IDE;
	TJA1145AFD_Partial_Network_Data_Mask_t	TJA1145AFD_PNDM;
	TJA1145AFD_CAN_Data_Length_t	TJA1145AFD_DLC;
}TJA1145AFD_Frame_Control_Reg_t;
//---------------------------------------------------
// register 0x4B
typedef struct
{
	TJA1145AFD_Wake_Pin_Status_t	TJA1145AFD_WPVS;
}TJA1145AFD_Wake_Status_Reg_t;
//---------------------------------------------------
// register 0x4C
typedef struct
{
	TJA1145AFD_Wake_Pin_Rising_Edge_Detect_t	TJA1145AFD_WPRE;
	TJA1145AFD_Wake_Pin_Falling_Edge_Detect_t	TJA1145AFD_WPFE;
}TJA1145AFD_Wake_Pin_Enable_Reg_t;
//---------------------------------------------------
// register 0x60
typedef struct
{
	TJA1145AFD_Wake_Pin_Event_t	TJA1145AFD_WPE;
	TJA1145AFD_Transceiver_Event_t	TJA1145AFD_TRXE;
	TJA1145AFD_System_Event_t	TJA1145AFD_SYSE;
}TJA1145AFD_Global_Event_Status_Reg_t;
//---------------------------------------------------
// register 0x61
typedef struct
{
	TJA1145AFD_Power_On_t	TJA1145AFD_PO;
	TJA1145AFD_Over_Temperature_Warning_t	TJA1145AFD_OTW;
	TJA1145AFD_SPI_Failure_t	TJA1145AFD_SPIF;
}TJA1145AFD_System_Event_Status_Reg_t;
//---------------------------------------------------
// register 0x63
typedef struct
{
	TJA1145AFD_Partial_Network_Frame_Detect_t	TJA1145AFD_PNFDE;
	TJA1145AFD_CAN_Bus_Active_t	TJA1145AFD_CBS;
	TJA1145AFD_CAN_Failure_t	TJA1145AFD_CF;
	TJA1145AFD_CAN_WakeUp_t	TJA1145AFD_CW;
}TJA1145AFD_Transceiver_Event_Status_Reg_t;
//---------------------------------------------------
// register 0x64
typedef struct
{
	TJA1145AFD_Rising_Wake_Pin_Event_t	TJA1145AFD_WPR;
	TJA1145AFD_Falling_Wake_Pin_Event_t	TJA1145AFD_WPF;
}TJA1145AFD_Wake_Pin_Event_Reg_t;
//---------------------------------------------------
// register 0x68
typedef struct
{
	TJA1145AFD_Data_Mask_0_Config_t	TJA1145AFD_DM0;
}TJA1145AFD_Data_Mask_0_Reg_t;
//---------------------------------------------------
// register 0x69
typedef struct
{
	TJA1145AFD_Data_Mask_1_Config_t	TJA1145AFD_DM1;
}TJA1145AFD_Data_Mask_1_Reg_t;
//---------------------------------------------------
// register 0x6A
typedef struct
{
	TJA1145AFD_Data_Mask_2_Config_t	TJA1145AFD_DM2;
}TJA1145AFD_Data_Mask_2_Reg_t;
//---------------------------------------------------
// register 0x6B
typedef struct
{
	TJA1145AFD_Data_Mask_3_Config_t	TJA1145AFD_DM3;
}TJA1145AFD_Data_Mask_3_Reg_t;
//---------------------------------------------------
// register 0x6C
typedef struct
{
	TJA1145AFD_Data_Mask_4_Config_t	TJA1145AFD_DM4;
}TJA1145AFD_Data_Mask_4_Reg_t;
//---------------------------------------------------
// register 0x6D
typedef struct
{
	TJA1145AFD_Data_Mask_5_Config_t	TJA1145AFD_DM5;
}TJA1145AFD_Data_Mask_5_Reg_t;
//---------------------------------------------------
// register 0x6E
typedef struct
{
	TJA1145AFD_Data_Mask_6_Config_t	TJA1145AFD_DM6;
}TJA1145AFD_Data_Mask_6_Reg_t;
//---------------------------------------------------
// register 0x6F
typedef struct
{
	TJA1145AFD_Data_Mask_7_Config_t	TJA1145AFD_DM7;
}TJA1145AFD_Data_Mask_7_Reg_t;
//---------------------------------------------------
// register 0x7E
typedef struct
{
	TJA1145AFD_Device_ID_t	TJA1145AFD_IDS;
}TJA1145AFD_Identification_Reg_t;
//---------------------------------------------------
#endif

