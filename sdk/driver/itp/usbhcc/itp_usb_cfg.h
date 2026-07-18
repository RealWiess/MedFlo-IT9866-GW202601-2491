/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Castor platform abstraction layer configurations.
 *
 * @author Jim Tan
 * @version 1.0
 */
#ifndef ITP_USB_CFG_H
#define ITP_USB_CFG_H

// include paths
#include "ite/ith.h"
#include "ite/itp.h"
#include <stdio.h>
#define PRINTF (void)printf

// Debug definition
#if !defined(NDEBUG) && !defined(DEBUG)
    #define DEBUG
#endif

#ifdef DEBUG
    #define ASSERT(e)                                                          \
        ((e) ? (void)0 : ithAssertFail(#e, __FILE__, __LINE__, __FUNCTION__))
#else
    #define ASSERT(e) ((void)0)
#endif

/* Log fucntions definition */
#ifdef CFG_ITP_ERR
    #define ITP_USB_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("ERR:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);      \
        } while (false)
#else
    #define ITP_USB_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITP_WARN
    #define ITP_USB_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("WARN:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
        } while (false)
#else
    #define ITP_USB_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITP_INFO
    #define ITP_USB_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("INFO: " fmt, ##__VA_ARGS__);                               \
        } while (false)
#else
    #define ITP_USB_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITP_DBG
    #define ITP_USB_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("DBG: " fmt, ##__VA_ARGS__);                                \
        } while (false)
#else
    #define ITP_USB_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif


#endif // ITP_USB_CFG_H
