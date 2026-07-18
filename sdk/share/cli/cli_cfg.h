/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * CLI configurations.
 *
 * @author Jim Tan
 * @version 1.0
 */
#ifndef CLI_CFG_H
#define CLI_CFG_H

// include paths
#include "ite/ith.h"
#include "ite/itp.h"
#include <stdio.h>

#define PRINTF (void)printf

/* Log fucntions definition */
#ifdef CFG_CLI_ERR
    #define CLI_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("ERR:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);      \
        } while (false)
#else
    #define CLI_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_CLI_WARN
    #define CLI_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("WARN:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
        } while (false)
#else
    #define CLI_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_CLI_INFO
    #define CLI_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("INFO: " fmt, ##__VA_ARGS__);                               \
        } while (false)
#else
    #define CLI_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_CLI_DBG
    #define CLI_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("DBG: " fmt, ##__VA_ARGS__);                                \
        } while (false)
#else
    #define CLI_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#endif // CLI_CFG_H
