/*
 * Copyright ?2020 NXP.
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

#include "ssp/mmp_spi.h"
#include "uC_Specific_Functions.h"
#include "NXP_UJA11XX_defines.h"
#include "NXP_TJA1145AFD_functions.h"


#define TJA1145_SPI_PORT  1

extern void InitScheduler(void);

/**
* \brief SPI transmission routine
* \todo implement this function with your microcontroller specific code
*	- Disable interrupts
*	- SPI_DATA_REG = tx;
*	- Wait until transition is done
*	- tx = SPI_DATA_REG
*	- Enable interrupts
* Remark: Furthermore, this function should not only send data via SPI, it should also check, if the SPI access was successful.
* Therefore, after the 1st SPI access, a 2nd SPI read access is performed to ensure that the configurations has been
* changed or rather the 1st read values have been correct.
*
* \param data A pointer to the data which shall be transmitted by the SPI interface
* \param length Number of data Bytes which shall be transmitted by the SPI interface
* \param mask Mask of Bits interesting for the consistency check
* \param type Type (write/read/interrupt) of the SPI access
* \return <b>NXP_UJA11XX_Error_Code_t</b> possible values: NXP_UJA11XX_ERROR_WRITE_FAIL = 0, NXP_UJA11XX_ERROR_READ_FAIL = 1, NXP_UJA11XX_ERROR_SPI_HW_FAIL = 2, NXP_UJA11XX_SUCCESS = 3
*/
NXP_UJA11XX_Error_Code_t SPI_Send(Byte* data, NXP_UJA11XX_SPI_Msg_Length_t length, Byte mask, NXP_UJA11XX_Access_t type) {

	uint32_t result = 1;
	unsigned char  SpiTxData[2] = {0};
	uint8_t   SpiRxData[2] = {0};
	// Insert your microcontroller specific code here
	// e.g.
	// First SPI access:
	// - Disable interrupts
	// - SPI_DATA_REG = tx;
	// - Wait until transision done
	// - tx = SPI_DATA_REG
	// - Enable interrupts
	// If no MTP register is accessed, read a second time:
	// - Disable interrupts
	// - SPI_DATA_REG = tx;
	// - Wait until transision done
	// - tx = SPI_DATA_REG
	// - Enable interrupts
	// Check of consistency:
	// - In case of a write access:
	// -- If address bytes of both SPI accesses and the data byte(s) sent in 1st and read back in 2nd SPI access are similar, SPI access was successful (NXP_UJA11XX_SUCCESS).
	// -- Else if address and data byte(s) read back are '0x00' or '0xFF', NXP_UJA11XX_ERROR_SPI_HW_FAIL is returned.
	// -- Else SPI write access was not succesful and NXP_UJA11XX_ERROR_WRITE_FAIL is returned.
	// - In case of an read access:
	// -- If address bytes and the data byte(s) of both SPI read accesses are similar, SPI access was successful (NXP_UJA11XX_SUCCESS).
	// -- Else SPI read access was not succesful and NXP_UJA11XX_ERROR_READ_FAIL is returned.
	// - In case of an access to an interrupt register:
	// -- If address bytes of both SPI accesses are similar and the according interrupt bit(s) are cleared in 2nd SPI access data, SPI access was successful.
	// -- Else SPI write access was not succesful and NXP_UJA11XX_ERROR_WRITE_FAIL is returned.
	if(length == NXP_UJA11XX_SPI_MSG_LENGTH_16)
	{
		if(type == NXP_UJA11XX_READ)
		{
		    SpiTxData[0] =  data[0];//addr
		    SpiTxData[1] =  0x0;
		    #ifdef CFG_SPI_ENABLE
		    result = mmpSpiPioTransfer(TJA1145_SPI_PORT, SpiTxData, SpiRxData, 2);
			#endif

			if(result)
				return NXP_UJA11XX_ERROR_READ_FAIL;
			else
				data[1] = SpiRxData[1];
		}

		if(type == NXP_UJA11XX_WRITE || type == NXP_UJA11XX_INTERRUPT)
		{

		    SpiTxData[0] = data[0];//addr
		    SpiTxData[1] = data[1];

		    #ifdef CFG_SPI_ENABLE
			result = mmpSpiPioTransfer(TJA1145_SPI_PORT, SpiTxData, SpiTxData, 2);
		    #endif

			if(result)
				return NXP_UJA11XX_ERROR_WRITE_FAIL;
		}

	}
	else
	{
		if(type == NXP_UJA11XX_READ)
			return NXP_UJA11XX_ERROR_READ_FAIL;

		if(type == NXP_UJA11XX_WRITE || type == NXP_UJA11XX_INTERRUPT)
			return NXP_UJA11XX_ERROR_WRITE_FAIL;
	}

	return 3;
}

/**
* \brief Init routine of the microcontroller
* \todo implement this function with your microcontroller specific code
*	- Configure SPI with MSB first, shifting on rising and sampling on falling edge
*	- Configure timer interrupt
*	- Configure external interrupt, edge sensitive, interrupt on falling edge
*	- Configure internal CAN Protocol Engine (PE)
*	- Configure Serial Communication Interfaces for LIN communication
*	- Configure internal Analog/Digital Converter (ADC)
*/
void InitMicrocontroller(void){
	// Insert your microcontroller specific code here
	// e.g.
	// Configure SPI with 16 Bit, MSB first, shifting on rising and sampling on falling edge
	// Configure timer interrupt
	// Configure external interrupt, edge sensitive, interrupt on falling edge
	// Configure CAN PE
	// Configure ADC;
    #ifdef CFG_SPI_ENABLE
    if(mmpSpiInitialize(TJA1145_SPI_PORT, SPI_OP_MASTR, CPO_0_CPH_1, SPI_CLK_10M) == 0)
    {
    	(void)printf("TJA1145 spi port(%d)inited\n", TJA1145_SPI_PORT);
    }
	#endif
}

/**
* \brief Check if a flash request is pending
* \todo implement this function with your microcontroller specific code
*	- Read dedicated register in Flash Memory Controller
* \return indicates if a flash update request is pending
*	- 1 = A Flash update is required
*	- 0 = No Flash update request pending
*/
Byte FlashProgramming(void){
	// Insert your microcontroller specific code here
	// e.g.
	// Read dedicated register in Flash Memory Controller etc.

	return 0;
}

/**
* \brief Disable microcontroller oscillator
* \todo implement this function with your microcontroller specific code
*	- stop asm command
*/
void EnterMcuStopMode(void)
{
	// Insert your microcontroller specific code here
	// e.g. stop asm command
}


/**
* \brief Stop CAN transmission in application
* \todo implement this function with your microcontroller specific code
*	- abort all tasks sending data via CAN
* \return indicates if Abort Transmission was successful
*	- 1 = All CAN transmission aborted
*	- 0 = Abort of all CAN transmissions failed
*/
Byte AbortTransmissionCAN(void){
	// Insert your microcontroller specific code here

	return 0;
}

/**
* \brief CAN PE is stopped (wake-up disabled)
* \todo implement this function with your microcontroller specific code
*	- Disable CAN PE completely
* \return indicates if Abort Transmission was successful
*	- 1 = CAN PE entered Stop Mode
*	- 0 = CAN PE has not entered Stop Mode
*/
Byte CANStopMode(void){
	// Insert your microcontroller specific code here
	// e.g. Disable CAN PE  with wake-up disabled

	return 0;
}

/**
* \brief CAN PE enters Sleep Mode (wake-up enabled)
* \todo implement this function with your microcontroller specific code
*	- Disable CAN PE completely or abort transmission
* \return indicates if Abort Transmission was successful
*	- 1 = CAN PE entered Sleep Mode
*	- 0 = CAN PE has not entered Sleep Mode
*/
Byte CANSleepMode(void){
	// Insert your microcontroller specific code here
	// e.g. Disable CAN PE  with wake-up enabled

	return 0;
}

/**
* \brief Enable transmissions of the CAN PE
* \todo implement this function with your microcontroller specific code
*	- Enable CAN PE completely or enable transmission path
* \return indicates if Enable Transmission was successful
*	- 1 = CAN PE is now able to transmit new messages
*	- 0 = Enabling of transmissions failed
*/
Byte EnableTransmissionCAN(void){
	// Insert your microcontroller specific code here
	// e.g. Enable CAN PE

	return 0;
}

/**
* \brief Disable global timebase
* \todo implement this function with your microcontroller specific code
*	- Disable timer
* \return indicates if Scheduler Disable was successful
*	- 1 = Timer disabled
*	- 0 = Timer disable failed
*/
Byte Scheduler_Disable(void){
	// Insert your microcontroller specific code here
	// e.g. disable global timer

	return 0;
}

/**
* \brief Enable global time base
* \todo implement this function with your microcontroller specific code
*	- Enable timer
* \return indicates if Scheduler Enable was successful
*	- 1 = Timer enabled
*	- 0 = Timer enable failed
*/
Byte Scheduler_Enable(void){
	// Insert your microcontroller specific code here

	return 0;
}

/**
* \brief Get RXD CAN value
* \todo implement this function with your microcontroller specific code
* \return RXD CAN value
*	- 1 = RXD CAN is high
*	- 0 = RXD CAN is low
*/
Byte RXDC_GetValue(){
	// Insert your microcontroller specific code here
	// e.g. return value of port pin connected to UJA1164 RXD pin
	return 1;
}


/**
* \brief Read Port Pin that gives the start signal for MTP programmg
* \todo implement this function with your microcontroller specific code
* \return Port pin value
*	- 1 = port pin is high (default): wait for start signal
*	- 0 = port pin is low: start programming MTP
*/
Byte MtpvnProgStart(void){
	// Insert your microcontroller specific code here

	return 1;
}


/**
* \brief Read Port Pin used for control SDMC configuration during MTP programming and watchdog triggering during normal operation
* \todo implement this function with your microcontroller specific code
* \return Port pin value
*	- 1 = port pin is high (default): During MTP programming, SDMC = 0; During Normal Operation: "Normal" WD triggering requested
*	- 0 = port pin is low: During MTP programming, SDMC = 1; During Normal Operation: "Debug" WD triggering requested (autonomous mode)
*/
Byte WdEmulation(void){
	// Insert your microcontroller specific code here

	return 1;
}

#if 0

/**
* \brief Read GPIO port configured as input port
* \todo implement this function with your microcontroller specific code
*	- Read GPIO port
* \return Port input value
*/
Byte ScanPort(void){
	// Insert your microcontroller specific code here
	// e.g. Read GPIO port and return value
	return 1;
}

/**
* \brief Set Port Pin on high level used for control a LED that shows, if SBC is in Normal Mode or not
* \todo implement this function with your microcontroller specific code
*/
void SetNormalModeLed(void) {
	// Insert your microcontroller specific code here

}


/**
* \brief Set Port Pin on low level used for control a LED that shows, if SBC is in Normal Mode or not
* \todo implement this function with your microcontroller specific code
*/
void ClearNormalModeLed(void) {
	// Insert your microcontroller specific code here

}


/**
* \brief Toggle Port Pin used for signalling a Watchdog trigger
* \todo implement this function with your microcontroller specific code
*/
void ToggleWdTriggerLed(void) {
	// Insert your microcontroller specific code here

}
#endif
