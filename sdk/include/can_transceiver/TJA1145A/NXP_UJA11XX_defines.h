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
#ifndef NXP_UJA11XX_DEFINES_H
#define NXP_UJA11XX_DEFINES_H

#define TRUE 1
#define FALSE 0

//--------------------------------------------------------------------------------
// general enumerations
//--------------------------------------------------------------------------------
// Data type which enables exchangeability of code with simulation environment
#ifndef Byte
typedef unsigned char Byte;
#endif
#ifndef Word
typedef unsigned int Word;
#endif
#ifndef bool
typedef unsigned char bool;
#endif

//---------------------------------------------------
// access identificator (used for address generation and SPI function)
typedef enum
{
	NXP_UJA11XX_WRITE     = 0,
	NXP_UJA11XX_READ      = 1,
	NXP_UJA11XX_INTERRUPT = 2
}NXP_UJA11XX_Access_t;
//---------------------------------------------------
// function error code return value
typedef enum
{
	NXP_UJA11XX_ERROR_WRITE_FAIL,
	NXP_UJA11XX_ERROR_READ_FAIL,
	NXP_UJA11XX_ERROR_SPI_HW_FAIL,
	NXP_UJA11XX_SUCCESS
}NXP_UJA11XX_Error_Code_t;
//---------------------------------------------------
// SPI transmission length
typedef enum
{
	NXP_UJA11XX_SPI_MSG_LENGTH_16 = 2,
	NXP_UJA11XX_SPI_MSG_LENGTH_24 = 3,
	NXP_UJA11XX_SPI_MSG_LENGTH_32 = 4,
}NXP_UJA11XX_SPI_Msg_Length_t;

//--------------------------------------------------------------------------------
#endif

