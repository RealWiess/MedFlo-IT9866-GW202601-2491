/*****************************************************************************
**
**  Name:           host_constants.h
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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define TCPIP_RESULT_LIST( prefix ) \
        RESULT_ENUM( prefix, SUCCESS,             0 ),   /**< Success */               \
        RESULT_ENUM( prefix, PENDING,             1 ),   /**< Pending */               \
        RESULT_ENUM( prefix, TIMEOUT,             2 ),   /**< Timeout */               \
        RESULT_ENUM( prefix, PARTIAL_RESULTS,     3 ),   /**< Partial results */       \
        RESULT_ENUM( prefix, ERROR,               4 ),   /**< Error */                 \
        RESULT_ENUM( prefix, BADARG,              5 ),   /**< Bad Arguments */         \
        RESULT_ENUM( prefix, BADOPTION,           6 ),   /**< Mode not supported */    \
        RESULT_ENUM( prefix, UNSUPPORTED,         7 ),   /**< Unsupported function */  \
        RESULT_ENUM( prefix, INVALID_PACKET,   7008 ),   /**< Invalid packet */        \
        RESULT_ENUM( prefix, INVALID_SOCKET,   7009 ),   /**< Invalid socket */        \
        RESULT_ENUM( prefix, WAIT_ABORTED,     7010 ),   /**< Wait aborted */          \
        RESULT_ENUM( prefix, PORT_UNAVAILABLE, 7011 ),   /**< Port unavailable */      \
        RESULT_ENUM( prefix, IN_PROGRESS,             7012 ),   /**< Action in progress */    \
        RESULT_ENUM( prefix, IP_ADDRESS_IS_NOT_READY, 7013 ),   /**< IP_ADDRESS_IS_NOT_READY */ \
        RESULT_ENUM( prefix, SOCKET_CLOSED,           7014 ),   /**< Socket closed */

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    HOST_FALSE,
    HOST_TRUE,
} host_bool_t;

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
} /* extern "C" */
#endif
