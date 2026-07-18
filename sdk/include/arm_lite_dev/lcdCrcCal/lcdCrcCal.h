#ifndef LCDCRCCAL_H
#define LCDCRCCAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_lite_dev/armlite_dev_device.h"

//=============================================================================
//                Constant Definition
//=============================================================================
#define LCD_CRC_CAL_CMD_ID          1

#define ITP_IOCTL_LCD_CRC_CAL       ITP_IOCTL_CUSTOM_CTL_ID1

#define MAX_CMD_DATA_BUFFER_SIZE    1024
#define CMD_DATA_BUFFER_OFFSET      (450 * 1024)

#define MAX_BLOCK_DATA_COUNT        32

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} LCD_BLOCK_DATA;

typedef struct
{
    uint32_t buffAddr;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t blockCount;
    LCD_BLOCK_DATA blockData[MAX_BLOCK_DATA_COUNT];
    uint32_t  crcVal[MAX_BLOCK_DATA_COUNT];
} LCD_CRC_CAL_CMD_DATA;

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





