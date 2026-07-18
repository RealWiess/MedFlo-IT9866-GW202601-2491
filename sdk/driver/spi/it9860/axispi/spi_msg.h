#ifndef __SPI_MSG_H__
#define __SPI_MSG_H__

#include <stdio.h>

//#define SPI_USE_VERBOSE_LOG
#define SPI_USE_ERROR_LOG

#ifdef _WIN32

    #ifdef SPI_USE_ERROR_LOG
        #define SPI_ERROR_MSG(format, ...)                                     \
            (void)printf("[SPI]%s(%d)[ERROR]: " format, __FUNCTION__,          \
                         __LINE__, __VA_ARGS__)
    #else
        #define SPI_ERROR_MSG(format, args...)
    #endif

    #define SPI_LOG_MSG(format, ...)                                           \
        (void)printf("[SPI]%s(%d) " format, __FUNCTION__, __LINE__, __VA_ARGS__)

    #ifdef SPI_USE_VERBOSE_LOG
        #define SPI_VERBOSE_LOG(format, ...)                                   \
            (void)printf("[SPI]%s(%d) " format, __FUNCTION__, __LINE__,        \
                         __VA_ARGS__args)
    #else
        #define SPI_VERBOSE_LOG(format, ...)
    #endif

    #define SPI_FUNC_ENTRY (void)printf("%s() ENTER\n", __FUNCTION__, __LINE__)
    #define SPI_FUNC_LEAVE (void)printf("%s() LEAVE\n", __FUNCTION__, __LINE__)
    #define SPI_HERE       (void)printf("%s()[%d]\n", __FUNCTION__, __LINE__)
#else

    #ifdef SPI_USE_ERROR_LOG
        #define SPI_ERROR_MSG(format, args...)                                 \
            (void)printf("[SPI]%s(%d)[ERROR]: " format, __FUNCTION__,          \
                         __LINE__, ##args)
    #else
        #define SPI_ERROR_MSG(format, args...)
    #endif

    #define SPI_LOG_MSG(format, args...)                                       \
        (void)printf("[SPI]%s(%d) " format, __FUNCTION__, __LINE__, ##args)

    #ifdef SPI_USE_VERBOSE_LOG
        #define SPI_VERBOSE_LOG(format, args...)                               \
            (void)printf("[SPI]%s(%d) " format, __FUNCTION__, __LINE__, ##args)
    #else
        #define SPI_VERBOSE_LOG(format, args...)
    #endif

    #define SPI_FUNC_ENTRY                                                     \
        (void)printf("%s(%d) ENTER\n", __FUNCTION__, __LINE__)
    #define SPI_FUNC_LEAVE                                                     \
        (void)printf("%s(%d) LEAVE\n", __FUNCTION__, __LINE__)
    #define SPI_HERE (void)printf("%s()[%d]\n", __FUNCTION__, __LINE__)

#endif

#endif	// __SPI_MSG_H__
