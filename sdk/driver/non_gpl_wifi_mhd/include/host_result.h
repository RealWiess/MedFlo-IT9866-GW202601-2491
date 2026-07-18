/*****************************************************************************
**
**  Name:           host_result.h
**
**  Description:    This is the wrapper file for Hardware Abstraction 
**                  layer (HAL) and OS Abstraction Layer (OSAL) of MHD, 
**                  Broadcom's WiFi host driver.
**
**  Copyright (c) 2018, Broadcom Inc., All Rights Reserved.
**  Broadcom WCC. Proprietary and confidential.
**
*****************************************************************************/

#pragma once

#include "platform_constants.h"
#include "host_resource.h"
#include "host_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef RESULT_ENUM
#define RESULT_ENUM( prefix, name, value )  prefix ## name = (value)
#endif /* ifndef RESULT_ENUM */

/*
 * Values: 0 - 999
 */
#define HOST_RESULT_LIST( prefix ) \
    RESULT_ENUM( prefix, SUCCESS,                        0 ),   /**< Success */                        \
    RESULT_ENUM( prefix, PENDING,                        1 ),   /**< Pending */                        \
    RESULT_ENUM( prefix, TIMEOUT,                        2 ),   /**< Timeout */                        \
    RESULT_ENUM( prefix, PARTIAL_RESULTS,                3 ),   /**< Partial results */                \
    RESULT_ENUM( prefix, ERROR,                          4 ),   /**< Error */                          \
    RESULT_ENUM( prefix, BADARG,                         5 ),   /**< Bad Arguments */                  \
    RESULT_ENUM( prefix, BADOPTION,                      6 ),   /**< Mode not supported */             \
    RESULT_ENUM( prefix, UNSUPPORTED,                    7 ),   /**< Unsupported function */           \
    RESULT_ENUM( prefix, OUT_OF_HEAP_SPACE,              8 ),   /**< Dynamic memory space exhausted */ \
    RESULT_ENUM( prefix, NOTUP,                          9 ),   /**< Interface is not currently Up */  \
    RESULT_ENUM( prefix, UNFINISHED,                    10 ),   /**< Operation not finished yet */     \
    RESULT_ENUM( prefix, CONNECTION_LOST,               11 ),   /**< Connection to server lost */      \
    RESULT_ENUM( prefix, NOT_FOUND,                     12 ),   /**< Item not found */                 \
    RESULT_ENUM( prefix, PACKET_BUFFER_CORRUPT,         13 ),   /**< Packet buffer corrupted */        \
    RESULT_ENUM( prefix, ROUTING_ERROR,                 14 ),   /**< Routing error */                  \
    RESULT_ENUM( prefix, BADVALUE,                      15 ),   /**< Bad value */                      \
    RESULT_ENUM( prefix, WOULD_BLOCK,                   16 ),   /**< Function would block */           \
    RESULT_ENUM( prefix, ABORTED,                       17 ),   /**< Operation aborted */              \
    RESULT_ENUM( prefix, CONNECTION_RESET,              18 ),   /**< Connection has been reset */      \
    RESULT_ENUM( prefix, CONNECTION_CLOSED,             19 ),   /**< Connection is closed */           \
    RESULT_ENUM( prefix, NOT_CONNECTED,                 20 ),   /**< Connection is not connected */    \
    RESULT_ENUM( prefix, ADDRESS_IN_USE,                21 ),   /**< Address is in use */              \
    RESULT_ENUM( prefix, NETWORK_INTERFACE_ERROR,       22 ),   /**< Network interface error */        \
    RESULT_ENUM( prefix, ALREADY_CONNECTED,             23 ),   /**< Socket is already connected */    \
    RESULT_ENUM( prefix, INVALID_INTERFACE,             24 ),   /**< Interface specified in invalid */ \
    RESULT_ENUM( prefix, SOCKET_CREATE_FAIL,            25 ),   /**< Socket creation failed */         \
    RESULT_ENUM( prefix, INVALID_SOCKET,                26 ),   /**< Socket is invalid */              \
    RESULT_ENUM( prefix, CORRUPT_PACKET_BUFFER,         27 ),   /**< Packet buffer is corrupted */     \
    RESULT_ENUM( prefix, UNKNOWN_NETWORK_STACK_ERROR,   28 ),   /**< Unknown network stack error */    \
    RESULT_ENUM( prefix, NO_STORED_AP_IN_DCT,           29 ),   /**< DCT contains no AP credentials */ \
    RESULT_ENUM( prefix, STA_JOIN_FAILED,               30 ),   /**< Join failed */

/* These Enum result values are for BUFFER errors
 */

#define BUFFER_RESULT_LIST( prefix )  \
    RESULT_ENUM( prefix, UNAVAILABLE_TEMPORARY, 1009 ),   /**< Buffer unavailable temporarily */    \
    RESULT_ENUM( prefix, UNAVAILABLE_PERMANENT, 1010 ),   /**< Buffer unavailable permanently */    \
    RESULT_ENUM( prefix, POINTER_MOVE_ERROR,    1033 ),   /**< Error moving the current pointer of a packet buffer  */   \
    RESULT_ENUM( prefix, SIZE_SET_ERROR,        1034 ),   /**< Error setting size of packet buffer */   \
    RESULT_ENUM( prefix, ALLOC_FAIL,            1038 ),   /**< Failed to allocate a packet buffer */

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * HOST Result Type
 */
typedef enum
{
    HOST_RESULT_LIST   (  HOST_           )  /*    0 -  999 */
    BUFFER_RESULT_LIST (  HOST_BUFFER_    )  /* 1000 - 1999 */
    RESOURCE_RESULT_LIST(  HOST_RESOURCE_ )  /* 4000 - 4999 */
    PLATFORM_RESULT_LIST(  HOST_PLATFORM_ )  /* 6000 - 6999 */
    TCPIP_RESULT_LIST   (  HOST_TCPIP_    )  /* 7000 - 7999 */
} host_result_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
