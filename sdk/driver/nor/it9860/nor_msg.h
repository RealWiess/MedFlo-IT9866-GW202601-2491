#ifndef NOR_MSG_H__
#define NOR_MSG_H__

#include <stdio.h>

#if 0
#define ENABLE_NOR_LOG_MSG
#endif

#ifdef _WIN32

    #define NOR_ERROR_MSG(format, ...)                                         \
        (void)printf("[NOR]%s(%d)[ERROR]: " format, __FUNCTION__, __LINE__,    \
                     __VA_ARGS__)

    #ifdef ENABLE_NOR_LOG_MSG
        #define NOR_LOG_MSG(format, ...)                                       \
            (void)printf("[NOR]%s(%d) " format, __FUNCTION__, __LINE__,        \
                         __VA_ARGS__)
    #else
        #define NOR_LOG_MSG(format, ...)
    #endif

    #define NOR_FUNC_ENTRY (void)printf("%s() ENTER\n", __FUNCTION__, __LINE__)
    #define NOR_FUNC_LEAVE (void)printf("%s() LEAVE\n", __FUNCTION__, __LINE__)
    #define NOR_HERE       (void)printf("%s()[%d]\n", __FUNCTION__, __LINE__);

#else

    #define NOR_ERROR_MSG(format, args...)                                     \
        (void)printf("[NOR]%s(%d)[ERROR]: " format, __FUNCTION__, __LINE__,    \
                     ##args)

    #ifdef ENABLE_NOR_LOG_MSG
        #define NOR_LOG_MSG(format, args...)                                   \
            (void)printf("[NOR]%s(%d) " format, __FUNCTION__, __LINE__, ##args)
    #else
        #define NOR_LOG_MSG(format, args...)
    #endif

    #define NOR_FUNC_ENTRY (void)printf("%s() ENTER\n", __FUNCTION__, __LINE__)
    #define NOR_FUNC_LEAVE (void)printf("%s() LEAVE\n", __FUNCTION__, __LINE__)
    #define NOR_HERE       (void)printf("%s()[%d]\n", __FUNCTION__, __LINE__);

#endif

#endif  // NOR_MSG_H__
