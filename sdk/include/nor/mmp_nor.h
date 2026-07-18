/*
 * Copyright (c) 2007 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * ITE NOR Driver API header file.
 *
 */

#ifndef MMP_NOR_H
#define MMP_NOR_H

//=============================================================================
//                              Include Files
//=============================================================================
#include "stdint.h"
#include "ssp/mmp_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DLL export API declaration for Win32.
 */

#if 0
#if defined(WIN32)
#if defined(NOR_EXPORTS)
#define NOR_API __declspec(dllexport)
#else
#define NOR_API __declspec(dllimport)
#endif
#else
#define NOR_API extern
#endif
#endif

#define NOR_API

#define BUILD_VERSION   0xA

//=============================================================================
//                              Structure Definition
//=============================================================================
typedef struct
{
    uint32_t    protectStartAddr;
    uint32_t    protectSize;
} NOR_PROTECT_ITEM;

//=============================================================================
//                              Enumeration Type Definition
//=============================================================================
typedef enum
{
    NOR_ATTITUDE_ERASE_UNIT,
    NOR_ATTITUDE_DEVICE_COUNT,
    NOR_ATTITUDE_CURRENT_DEVICE_ID,
    NOR_ATTITUDE_PAGE_SIZE,
    NOR_ATTITUDE_PAGE_PER_SECTOR,
    NOR_ATTITUDE_SECTOR_PER_BLOCK,
    NOR_ATTITUDE_BLOCK_SIZE,
    NOR_ATTITUDE_TOTAL_SIZE,
    NOR_ATTITUDE_UN_DEFINED
}NOR_ATTITUDE;

//=============================================================================
//                              Constant Definition
//=============================================================================
#define NOR_RESULT                uint32_t
#if defined (CFG_NOR_CHIP_SEL_ID)
    #define NOR_CSN                 CFG_NOR_CHIP_SEL_ID
#else
    #define NOR_CSN                 SPI_CSN_0
#endif
/*Error Code*/
#define NOR_ERROR_DEVICE_UNKNOW		1
#define NOR_ERROR_DEVICE_TIMEOUT    2
#define NOR_ERROR_STATUS_BUSY	    3
#define NOR_ERROR_STATUS_PROTECT	4
#define NOR_ERROR_ADDR_UN_ALIGNED   5

//=============================================================================
//                              Global Data Definition
//=============================================================================
#if defined (CFG_NOR_USE_AXISPI)
    extern pthread_mutex_t NOR_XIP_MUTEX;
#endif

//=============================================================================
//                              Function Declaration
//=============================================================================

/** @defgroup group18 ITE NOR Driver API
 *  The NOR module API.
 *  @{
 */

/**
 * Initialize NOR
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorTerminate()
 */
NOR_API NOR_RESULT
NorInitial(
	SPI_PORT port,
	SPI_CSN  chipSelectPin);

/**
 * Terminate NOR
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorInitial()
 */
NOR_API NOR_RESULT
NorTerminate(
	SPI_PORT port,
	SPI_CSN  chipSelectPin);

/**
 * Erase blocks
 *
 * @param address   	Start address to erase.
 * @param eraseLength	The length of erase.
 *
 * @return true if succeed, error codes of false otherwise.
 */
NOR_API bool
NorErase(
	SPI_PORT	port,
	SPI_CSN		chipSelectPin,
	uint32_t	address,
	uint32_t    eraseLength);

/**
 * Erase all blocks
 *
 *
 * @return true if succeed, error codes of false otherwise.
 */
NOR_API NOR_RESULT
NorEraseAll(
	SPI_PORT	port,
	SPI_CSN		chipSelectPin);

/**
 * Read Nor Data
 *
 * @param pdes        destination data buffer.
 * @param addr        read address.
 * @param size        read size.
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorWrite()
 */
NOR_API NOR_RESULT
NorRead(
	SPI_PORT	port,
	SPI_CSN		chipSelectPin,
	uint32_t	address,
    uint8_t*	outData,
    uint32_t	outDataSize);

/**
 * Protect NOR Data through SW driver
 *
 * @param totalCount        protection item count.
 * @param protectInfo       protection range.
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorWrite(), NorWriteWithoutErase();
 */
NOR_API NOR_RESULT
NorSWProtect(
    SPI_PORT            port,
    SPI_CSN             chipSelectPin,
    uint32_t            totalCount,
    NOR_PROTECT_ITEM    *protectInfo);

/**
 * Write NOR Data
 *
 * @param psrc        destination data buffer.
 * @param addr        write address.
 * @param size        write size.
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorRead()
 */
NOR_API NOR_RESULT
NorWrite(
	SPI_PORT	port,
	SPI_CSN		chipSelectPin,
	uint32_t	address,
    uint8_t*	inData,
    uint32_t	inDataSize);

/**
 * Write NOR Data Without Erase
 *
 * @param psrc        destination data buffer.
 * @param addr        write address.
 * @param size        write size.
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorRead()
 */
NOR_API NOR_RESULT
NorWriteWithoutErase(
	SPI_PORT	port,
	SPI_CSN		chipSelectPin,
	uint32_t	address,
    uint8_t*	inData,
    uint32_t	inDataSize);

/**
 * Write NOR Data Without Erase and ignore alignment check
 *
 * @param psrc        destination data buffer.
 * @param addr        write address.
 * @param size        write size.
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorRead()
 */
NOR_RESULT
NorWriteWithoutEraseMisalign (
    SPI_PORT    port,
    SPI_CSN     chipSelectPin,
    uint32_t    address,
    uint8_t     * inData,
    uint32_t    inDataSize);

/**
 * Erase whole NOR flash chip
 *
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorRead()
 */
#if 0
NOR_API NOR_RESULT
NorBulkErase(
    void);
#endif

/**
 * Get NOR Capacity
 *
 *
 * @return NOR Capacity size.
 * @see NorGetAttitude()
 */
NOR_API uint32_t
NorGetCapacity(
    SPI_PORT port,
    SPI_CSN chipSelectPin);

/**
 * Get NOR ERASE_UNIT,DEVICE_COUNT,CURRENT_DEVICE_ID information
 *
 *
 * @param atti        NOR attitude.
 *
 * @return NOR_RESULT_SUCCESS if succeed, error codes of NOR_RESULT_ERROR otherwise.
 * @see NorGetDeviceName()
 */
NOR_API uint32_t
NorGetAttitude(
	SPI_PORT port,
	SPI_CSN  chipSelectPin,
    NOR_ATTITUDE atti);

/**
 * Get NOR Device Name
 *
 *
 * @param num        NOR Support ID num.
 *
 * @return Device Name.
 * @see NorGetAttitude()
 */
#if 0
NOR_API uint8_t *
NorGetDeviceName(
    uint8_t num);

uint32_t
NorGetDeviceCount(void);

uint32_t
NorGetDeviceInfo(
    uint32_t deviceIndex,
    uint32_t *pId1,
    uint32_t *pId2,
    uint32_t *pId3,
    char     **deviceName);
#endif

/**
 * Get NOR Driver Build Info
 *
 *
 * @param version     NOR Driver Build Version.
 * @param date        NOR Driver Build Data.
 *
 * @see NorGetDeviceName()
 */
#if 0
NOR_API void
NorGetBuildInfo(
    uint8_t *version,
    uint8_t *date);
#endif

#if defined(CFG_NOR_USE_AXISPI)
NOR_API NOR_RESULT
NorReadUID(
    SPI_PORT port,
    SPI_CSN  chipSelectPin,
    uint8_t  *outData); // first 16 bytes are the UID

NOR_API NOR_RESULT
NorWriteOTP(
    SPI_PORT port,
    SPI_CSN  chipSelectPin,
    uint8_t  *inData); // first 512 bytes are the OTP

NOR_API NOR_RESULT
NorReadOTP(
    SPI_PORT port,
    SPI_CSN  chipSelectPin,
    uint8_t  *outData); // first 512 bytes are the OTP
#endif

#ifdef CFG_NOR_USE_DPUAES
NOR_API NOR_RESULT
NorSetAESKey(
    SPI_PORT port,
    SPI_CSN  chipSelectPin,
    uint8_t  *inData); // first 16 bytes are the AES key

NOR_API NOR_RESULT
NorWriteAES(
    SPI_PORT port,
    SPI_CSN  chipSelectPin,
    uint32_t address,
    uint8_t  *inData,
    uint32_t inDataSize);

NOR_API NOR_RESULT
NorReadAES(
    SPI_PORT port,
    SPI_CSN  chipSelectPin,
    uint32_t address,
    uint8_t  *outData,
    uint32_t outDataSize);

NOR_API NOR_RESULT
NorWriteWithoutEraseAES(
    SPI_PORT port,
    SPI_CSN  chipSelectPin,
    uint32_t address,
    uint8_t  *inData,
    uint32_t inDataSize);
#endif

//@}
#ifdef __cplusplus
}
#endif

#endif