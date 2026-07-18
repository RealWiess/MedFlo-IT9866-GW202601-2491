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
                                              
#ifndef __uC_Specific_Functions_h
#define __uC_Specific_Functions_h  
#include <stdio.h>
#include "NXP_UJA11XX_defines.h"

/**
*	\brief Enable interrupts 
*	\todo implement this function with your microcontroller specific code
*	- Enable Interrupts (Assemby command)
*/
#define __EI() { }

/**
*	\brief Disable interrupts 
*	\todo implement this function with your microcontroller specific code
*	- Disable Interrupts (Assemby command)
*/
#define __DI() {}    

// see uC_Specific_Functions.c for documentation
NXP_UJA11XX_Error_Code_t SPI_Send(Byte* data, NXP_UJA11XX_SPI_Msg_Length_t length, Byte mask ,NXP_UJA11XX_Access_t type);

// see uC_Specific_Functions.c for documentation
void InitMicrocontroller(void); 

// see uC_Specific_Functions.c for documentation
void EnterMcuStopMode(void);

// see uC_Specific_Functions.c for documentation
Byte FlashProgramming(void);   

// see uC_Specific_Functions.c for documentation
Byte AbortTransmissionCAN(void);

// see uC_Specific_Functions.c for documentation
Byte CANStopMode(void);

// see uC_Specific_Functions.c for documentation
Byte CANSleepMode(void);

// see uC_Specific_Functions.c for documentation
Byte EnableTransmissionCAN(void);

// see uC_Specific_Functions.c for documentation
Byte Scheduler_Disable(void);

// see uC_Specific_Functions.c for documentation
Byte Scheduler_Enable(void);

// see uC_Specific_Functions.c for documentation
Byte RXDC_GetValue(void);

// see uC_Specific_Functions.c for documentation
Byte MtpvnProgStart(void);

// see uC_Specific_Functions.c for documentation
Byte WdEmulation(void);   

#if 0
// see uC_Specific_Functions.c for documentation
Byte ScanPort(void);

// see uC_Specific_Functions.c for documentation
void SetNormalModeLed(void);

// see uC_Specific_Functions.c for documentation
void ClearNormalModeLed(void);

// see uC_Specific_Functions.c for documentation
void ToggleWdTriggerLed(void);
#endif
#endif