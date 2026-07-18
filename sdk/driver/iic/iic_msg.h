#ifndef __IIC_MSG_H__
#define __IIC_MSG_H__

#include <stdio.h>

#ifdef _WIN32

    #define IIC_FUNC_ENTRY  (void)printf("%s() ENTER\n", __FUNCTION__, __LINE__)
    #define IIC_FUNC_LEAVE  (void)printf("%s() LEAVE\n", __FUNCTION__, __LINE__)
    #define IIC_HERE        (void)printf("%s()[%d]\n", __FUNCTION__, __LINE__)

#else

    #define IIC_FUNC_ENTRY  (void)printf("%s() ENTER\n", __FUNCTION__, __LINE__)
    #define IIC_FUNC_LEAVE  (void)printf("%s() LEAVE\n", __FUNCTION__, __LINE__)
    #define IIC_HERE        (void)printf("%s()[%d]\n", __FUNCTION__, __LINE__)

#endif

#endif  // __IIC_MSG_H__
