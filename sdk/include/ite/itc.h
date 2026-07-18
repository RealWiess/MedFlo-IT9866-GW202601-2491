/*
 * Copyright (c) 2009 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * ITE Common Library.
 *
 * @author Jim Tan
 * @version 1.0
 * @date 2013
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @defgroup itc ITE Common Library
 *  @{
 */
#ifndef ITE_ITC_H
#define ITE_ITC_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Definitions
#define ITC_LOCK_FAIL   (-1)    ///< Locks fail error

#include "itc_tree.h"
#include "itc_stream.h"
#include "itc_array_stream.h"
#include "itc_buffer_stream.h"
#include "itc_file_stream.h"
#include "itc_list_stream.h"
#include "itc_block_stream.h"
#include "itc_cli_stream.h"

/** @} */ // end of itc_stream

/** @defgroup itc_url URL
*  @{
*/
/**
* Returns a url-encoded version of str.
* IMPORTANT: be sure to free() the returned string after use
*
* @param str url to encode.
* @return a url-encoded version of str.
*/
char *itcUrlEncode(char *str);

/**
* Returns a url-decoded version of str.
* IMPORTANT: be sure to free() the returned string after use
*
* @param str url to encode.
* @return a url-decoded version of str.
*/
char *itcUrlDecode(char *str);

/** @} */ // end of itc_url

/** @defgroup itc_crc CRC
*  @{
*/
/**
* Calculates CRC-16 code.
*
* @param data the data to calculate.
* @param size the data size.
* @return the crc-16 code.
*/
uint16_t itcCrc16(const uint8_t* data, uint16_t size);

/** @} */ // end of itc_crc

#ifdef __cplusplus
}
#endif

#endif // ITE_ITC_H
/** @} */ // end of itc
