#ifndef FREQ_DETECT_H
#define FREQ_DETECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "alt_cpu/alt_cpu_device.h"

//=============================================================================
//                Constant Definition
//=============================================================================
#define MAX_CMD_DATA_BUFFER_SIZE    640
#define CMD_DATA_BUFFER_OFFSET      (16 * 1024 - MAX_CMD_DATA_BUFFER_SIZE)

#define INIT_CMD_ID                 1
#define READ_CMD_ID                 2
#define START_CMD_ID                3
#define STOP_CMD_ID                 4

#define ITP_IOCTL_INIT_FREQ_DETECT_PARAM            ITP_IOCTL_CUSTOM_CTL_ID1
#define ITP_IOCTL_FREQ_DETECT_START                 ITP_IOCTL_CUSTOM_CTL_ID2
#define ITP_IOCTL_FREQ_DETECT_READ                  ITP_IOCTL_CUSTOM_CTL_ID3
#define ITP_IOCTL_FREQ_DETECT_STOP                  ITP_IOCTL_CUSTOM_CTL_ID4

typedef enum
{
    FREQ_DETECT0 = 0,
    FREQ_DETECT1,
    FREQ_DETECT2,
    FREQ_DETECT3,
    FREQ_DETECT_COUNT, //Must be the last entry;
} FREQ_DETECT_ID;

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct
{
    unsigned long freqDetectId;
    unsigned long cpuClock;
    unsigned long freqDetectGpio;
} FREQ_DETECT_INIT_DATA;

typedef struct
{
    unsigned long freqDetectId;
    unsigned long hz;
    unsigned long sn;
} FREQ_DETECT_READ_DATA;

typedef struct
{
    unsigned long freqDetectId;
} FREQ_DETECT_START_CMD_DATA;

typedef struct
{
    unsigned long freqDetectId;
} FREQ_DETECT_STOP_CMD_DATA;

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





