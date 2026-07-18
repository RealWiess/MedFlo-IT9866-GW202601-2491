/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Castor hardware abstraction layer configurations.
 *
 * @author Jim Tan
 * @version 1.0
 */
#ifndef ITH_CFG_H
#define ITH_CFG_H

// include paths
#if defined(ANDROID) // Android
    #include "ite/ith.h"
    #include <stdio.h>
    #define PRINTF (void)printf

#elif defined(__LINUX_ARM_ARCH__) // Linux kernel
    #include "mach/ith.h"
    #include <linux/kernel.h>
    #define PRINTF (void)printk

#elif defined(CONFIG_ARM) // U-Boot
    #include "asm/arch/ith.h"
    #include "exports.h"
    #define PRINTF (void)printf

#elif defined(_WIN32) // Win32
    #include "ite/ith.h"
    #include <stdio.h>
    #include <stdbool.h>
    #define PRINTF (void)printf

#else
    #include <stdbool.h>
    #include "ite/ith.h"
    #define PRINTF (void)ithPrintf

#endif // defined(__LINUX_ARM_ARCH__)

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
#ifdef CFG_ITH_ERR
    #define ITH_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("ERR:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);      \
        } while (false)
#else
    #define ITH_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITH_WARN
    #define ITH_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("WARN:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
        } while (false)
#else
    #define ITH_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITH_INFO
    #define ITH_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("INFO: " fmt, ##__VA_ARGS__);                               \
        } while (false)
#else
    #define ITH_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITH_DBG
    #define ITH_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("DBG: " fmt, ##__VA_ARGS__);                                \
        } while (false)
#else
    #define ITH_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

// Command queue definitions
#define ITH_CMDQ_LOOP_TIMEOUT 3000 // 30000

// Video memory definitions
#define ITH_VMEM_BESTFIT

#endif // ITH_CFG_H
