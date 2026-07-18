/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Castor platform abstraction layer configurations.
 *
 * @author Jim Tan
 * @version 1.0
 */
#ifndef ITP_CFG_H
#define ITP_CFG_H

// include paths
#include "ite/ith.h"
#include "ite/itp.h"
#include <stdio.h>
#include <stdbool.h>
#define PRINTF printf

// Debug definition
#if !defined(NDEBUG) && !defined(DEBUG)
    #define DEBUG
#endif

#ifdef DEBUG
    #define ASSERT(e) ((e) ? (void) 0 : ithAssertFail(#e, __FILE__, __LINE__, __FUNCTION__))
#else
    #define ASSERT(e) ((void) 0)
#endif

/* Log fucntions definition */
#ifdef CFG_ITP_ERR
    #define ITP_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("ERR:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);      \
        } while (false)
#else
    #define ITP_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITP_WARN
    #define ITP_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("WARN:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
        } while (false)
#else
    #define ITP_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITP_INFO
    #define ITP_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("INFO: " fmt, ##__VA_ARGS__);                               \
        } while (false)
#else
    #define ITP_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITP_DBG
    #define ITP_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("DBG: " fmt, ##__VA_ARGS__);                                \
        } while (false)
#else
    #define ITP_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

// UART
//#define ITP_UART_DMA
//#define ITP_RS485_DMA

// FAT
#define ITP_FAT_UTF8

// OSD CONSOLE
#define ITP_OSD_TIMEOUT_COUNT 10000

#endif // ITP_CFG_H
