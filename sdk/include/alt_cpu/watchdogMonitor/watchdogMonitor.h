#ifndef WATCHDOG_MONITOR_H
#define WATCHDOG_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "alt_cpu/alt_cpu_device.h"

//=============================================================================
//                Constant Definition
//=============================================================================
#define MAX_CMD_DATA_BUFFER_SIZE    256
#define CMD_DATA_BUFFER_OFFSET      (16 * 1024 - MAX_CMD_DATA_BUFFER_SIZE)

#define RUN_CMD_ID                  1
#define STOP_CMD_ID                 2

#define ITP_IOCTL_WATCHDOG_MONITOR_RUN      ITP_IOCTL_CUSTOM_CTL_ID1
#define ITP_IOCTL_WATCHDOG_MONITOR_STOP     ITP_IOCTL_CUSTOM_CTL_ID2

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct
{
    unsigned long dummyData;
} WATCHDOG_MONITOR_RUN_CMD_DATA;

typedef struct
{
    unsigned long dummyData;
} WATCHDOG_MONITOR_STOP_CMD_DATA;

//=============================================================================
//                Global Data Definition
//=============================================================================


//=============================================================================
//                Private Function Definition
//=============================================================================


//=============================================================================
//                Public Function Definition
//=============================================================================

#ifdef __cplusplus
}
#endif

#endif





