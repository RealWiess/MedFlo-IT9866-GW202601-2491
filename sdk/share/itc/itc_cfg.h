#ifndef ITC_CFG_H
#define ITC_CFG_H

#include <stdio.h>

#define PRINTF printf

/* Log fucntions definition */
#ifdef CFG_ITC_ERR
    #define ITC_LOG_ERR(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
            PRINTF("ERR:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);      \
        } while (false)
#else
    #define ITC_LOG_ERR(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITC_WARN
    #define ITC_LOG_WARN(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("WARN:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
        } while (false)
#else
    #define ITC_LOG_WARN(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITC_INFO
    #define ITC_LOG_INFO(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("INFO: " fmt, ##__VA_ARGS__);                               \
        } while (false)
#else
    #define ITC_LOG_INFO(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITC_DBG
    #define ITC_LOG_DBG(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
            PRINTF("DBG: " fmt, ##__VA_ARGS__);                                \
        } while (false)
#else
    #define ITC_LOG_DBG(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#endif // ITC_CFG_H
