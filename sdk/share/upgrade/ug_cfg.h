#ifndef UG_CFG_H
#define UG_CFG_H

#include <stdio.h>
#include <stdbool.h>

#define PRINTF printf

/* Log fucntions definition */
#ifdef CFG_UG_ERR
    #define UG_LOG_ERR(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
            PRINTF("ERR:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);      \
        } while (false)
#else
    #define UG_LOG_ERR(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_UG_WARN
    #define UG_LOG_WARN(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("WARN:%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__);     \
        } while (false)
#else
    #define UG_LOG_WARN(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_UG_INFO
    #define UG_LOG_INFO(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
            PRINTF("INFO: " fmt, ##__VA_ARGS__);                               \
        } while (false)
#else
    #define UG_LOG_INFO(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#ifdef CFG_UG_DBG
    #define UG_LOG_DBG(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
            PRINTF("DBG: " fmt, ##__VA_ARGS__);                                \
        } while (false)
#else
    #define UG_LOG_DBG(fmt, ...)                                               \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#if defined(CFG_FS_LFS)
#define UG_DEVICE ITP_DEVICE_LFS
#define UG_DEVICE_2 ITP_DEVICE_FAT
#elif defined(CFG_FS_YAFFS2)
#define UG_DEVICE ITP_DEVICE_YAFFS2
#else
#define UG_DEVICE ITP_DEVICE_FAT
#endif

#if (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
#define UG_DEVICE_2 ITP_DEVICE_FAT
#define UG_DEVICE_PAR_INFO
#endif

#endif // UG_CFG_H
