#ifndef __SPI_MSG_H__
#define __SPI_MSG_H__

#include <stdbool.h>
#include <stdio.h>

// #define SPI_USE_VERBOSE_LOG
#define SPI_USE_ERROR_LOG

#ifdef SPI_USE_ERROR_LOG
    #define SPI_ERROR_MSG(fmt, ...)                                            \
        do                                                                     \
        {                                                                      \
            (void)printf("[SPI]%s(%d)[ERROR]: " fmt, __FUNCTION__, __LINE__,   \
                         ##__VA_ARGS__);                                       \
        } while (false)
#else
    #define SPI_ERROR_MSG(fmt, ...)                                            \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#define SPI_LOG_MSG(fmt, ...)                                                  \
    do                                                                         \
    {                                                                          \
        (void)printf("[SPI]%s(%d) " fmt, __FUNCTION__, __LINE__,               \
                     ##__VA_ARGS__);                                           \
    } while (false)

#ifdef SPI_USE_VERBOSE_LOG
    #define SPI_VERBOSE_LOG(fmt, ...)                                          \
        do                                                                     \
        {                                                                      \
            (void)printf("[SPI]%s(%d) " fmt, __FUNCTION__, __LINE__,           \
                         ##__VA_ARGS__)                                        \
        } while (false)
#else
    #define SPI_VERBOSE_LOG(fmt, ...)                                          \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#define SPI_FUNC_ENTRY (void)printf("%s(%d) ENTER\n", __FUNCTION__, __LINE__)
#define SPI_FUNC_LEAVE (void)printf("%s(%d) LEAVE\n", __FUNCTION__, __LINE__)
#define SPI_HERE       (void)printf("%s()[%d]\n", __FUNCTION__, __LINE__)

#endif // __SPI_MSG_H__
