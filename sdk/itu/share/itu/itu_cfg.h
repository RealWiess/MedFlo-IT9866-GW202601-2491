#ifndef ITU_CFG_H
#define ITU_CFG_H

#include <stdbool.h>
#include <stdio.h>

#define PRINTF (void)printf

/* Log fucntions definition */
#ifdef CFG_ITU_ERR
    #define ITU_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("ERR:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);      \
        } while (false)
#else
    #define ITU_LOG_ERR(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITU_WARN
    #define ITU_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("WARN:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
        } while (false)
#else
    #define ITU_LOG_WARN(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITU_INFO
    #define ITU_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
            PRINTF("INFO: " fmt, ##__VA_ARGS__);                               \
        } while (false)
#else
    #define ITU_LOG_INFO(fmt, ...)                                             \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_ITU_DBG
    #define ITU_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("DBG: " fmt, ##__VA_ARGS__);                                \
        } while (false)
    #define ituLog(fmt, ...)                                                   \
        do                                                                     \
        {                                                                      \
            PRINTF(fmt, ##__VA_ARGS__);                                        \
        } while (false)
#else
    #define ITU_LOG_DBG(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
    #define ituLog(fmt, ...)                                                   \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#define ITU_SCROLL_DELAY 3
#define ITU_STOP_DELAY   30

#endif // ITU_CFG_H
