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
* \version  V1.0
* \date     2020/09/18
*/
 
#ifndef __TJA1145AFD_Application_Specific_Functions_h
#define __TJA1145AFD_Application_Specific_Functions_h

#include "NXP_TJA1145AFD_Sim.h" 

// Max. number of task in ApplTask queue
#define MAXTASKNO 256

// Max. number of error entries in failure memory
#define MAXERRENTR 256

// Sleep mode configuration on input ports
#define PORTMASK_SLEEP 0x10

// Normal mode configuration on input ports
#define PORTMASK_NORMAL 0x40

// Cylic wake configuration on input ports
#define PORTMASK_STANDBY 0x20

// Supported pending tasks in application queue
typedef enum PendingTask {
	NO_OPERATION = 0,
	CHANGE_TO_SLEEP = 1,
	CHANGE_TO_STANDBY = 2,
	CHANGE_TO_NORMAL = 3,     
	EVENT_HANDLING = 4,
	POLL_TRCV_STATUS = 6,
	POLL_WAKE = 7,
	PORT_SUPERVISION = 8,
	CFG_PARTIAL_NETWORKING = 9
} PendingTask_t;

// Supported error entries in failure memory
typedef enum FailureEntries {                    // to be updated
	EMPTY = 0,
	DIAGNOSTIC_WAKEUP_SLEEP = 1,
	LEAVING_OVERLOAD = 4,
	SPI_FAILURE = 8,
	OVERTEMP_WARNING = 9,
	BUFFER_OVERFLOW = 10,
	CAN_FAILURE = 18	
} FailureEntry_t;

// Supported device types
typedef enum DeviceIdentification {
    NXP_TJA1145A = 1,
	NXP_TJA1145AFD = 2,
	NXP_UJA1164A = 3,
	NXP_UJA1167A = 4,
	NXP_UJA1167AX = 5,
	NXP_UJA1168A = 6,
	NXP_UJA1168AF = 7,
	NXP_UJA1168AXF = 8,
	NXP_UJA1168AX = 9,
	NXP_UJA1169AF = 10,
	NXP_UJA1169AXF = 11,
	NXP_UJA1169AX = 12,
	NXP_UJA1169A = 13,
	NXP_UNKNOWN = 0,                	                                                 
} Device_t;

// Supported standard return values
typedef enum {
	E_OK = 0,
	E_NOT_OK = 1
} StdReturn_t;

/**** types related to PN Autosar R4.1 ****/
typedef enum CanTrcv_TrcvWakeupReason {
	CANTRCV_WU_BY_BUS = 0,
	CANTRCV_WU_BY_PIN = 1,
	CANTRCV_WU_ERROR = 2,
	CANTRCV_WU_INTERNALLY = 3,
	CANTRCV_WU_NOT_SUPPORTED = 4,
	CANTRCV_WU_POWER_ON = 5,
	CANTRCV_WU_RESET = 6,
	CANTRCV_WU_BY_SYSERR = 7, 
	CANTRCV_WU_BY_BUS_WUF = 8,
	CANTRCV_WU_BY_BUS_WUP = 9,
} CanTrcv_TrcvWakeupReasonType;

typedef enum CanTrcv_TrcvMode {
	CANTRCV_TRCVMODE_NORMAL,
	CANTRCV_TRCVMODE_SLEEP,
	CANTRCV_TRCVMODE_STANDBY
} CanTrcv_TrcvModeType;

typedef enum CanTrcv_PNActivation {
	PN_DISABLED = 0,
	PN_ENABLED = 1,
} CanTrcv_PNActivationType;


// see TJA1145A_Application_Specific_Functions.c for documentation
void SchedulerOnTimerOverflow(void); 

// see TJA1145A_Application_Specific_Functions.c for documentation
Device_t GetDeviceType(void);            

// see TJA1145A_Application_Specific_Functions.c for documentation
void InitApplication(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t StartupOperation(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t InitTJA1145(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t CheckFsmStatus(void); 

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t ChangeToSleepOperation(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t ChangeToStandbyOperation(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t ChangeToNormalOperation(void);        

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t EventHandling(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t RxdLowHandling(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
void PollTransceiverStatus(void);
#if 0
// see TJA1145A_Application_Specific_Functions.c for documentation
void PortSupervisor(void);
#endif
// see TJA1145A_Application_Specific_Functions.c for documentation
void WakeSupervisor(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_SetPnIdentifier(Word id, TJA1145AFD_Identifier_Format_t idFormat);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_SetPnIdMask(Word mask, TJA1145AFD_Identifier_Format_t idFormat);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t ConfigurePartialNetworking(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_SPIF_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_OTW_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_PO_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_PNFDE_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_CW_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_CF_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_CBS_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_WPF_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_WPR_ServiceRoutine(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
void RXDLow_ISR(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
void Timer_ISR(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
void StopCAN_TX(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
void ResumeCAN_TX(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
Byte EnterFlashOperation(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
void AddToFailureMemory(FailureEntry_t data);          

// see TJA1145A_Application_Specific_Functions.c for documentation
void InitScheduler(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
void AddTaskToScheduler(PendingTask_t);

// see TJA1145A_Application_Specific_Functions.c for documentation
PendingTask_t GetNextTask(void);

// TJA1145A Main Function - Entry point of the application
void* TJA1145A_handle_task(void *arg);
// TJA1145A Main Function - Entry point of the application
void* TJA1145A_polling_task(void *arg);


/**** APIs related to PN Autosar R4.1 ****/

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_GetWuReason(CanTrcv_TrcvWakeupReasonType* pReason);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_ClearTrcvWufFlag(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_ReadTrcvTimeoutFlag(TJA1145AFD_CAN_Bus_Active_t* pCbs);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_ClearTrcvTimeoutFlag(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_ReadTrcvSilenceFlag(TJA1145AFD_CAN_Bus_Status_t* pCbss);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_CheckWakeFlag(void);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_SetPNActivationState(CanTrcv_PNActivationType actState);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_SetOpMode(CanTrcv_TrcvModeType);

// see TJA1145A_Application_Specific_Functions.c for documentation
StdReturn_t TJA1145A_GetOpMode(CanTrcv_TrcvModeType* pMode);

#endif

