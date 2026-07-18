#ifndef HUDBGFAST_H
#define HUDBGFAST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_lite_dev/armlite_dev_device.h"

//=============================================================================
//                Constant Definition
//=============================================================================
#define INIT_CMD_ID                 1
#define INIT_CMD_CMDQ_ID            2
#define INIT_CMD_HW_ID              3
#define SKIP_BG_FAST_CMD_ID         4

#define ITP_IOCTL_HUD_INIT_PARAM            ITP_IOCTL_CUSTOM_CTL_ID1
#define ITP_IOCTL_HUD_INIT_CMDQ_PARAM       ITP_IOCTL_CUSTOM_CTL_ID2
#define ITP_IOCTL_HUD_INIT_HW_PARAM         ITP_IOCTL_CUSTOM_CTL_ID3
#define ITP_IOCTL_HUD_SKIP_BG_FAST          ITP_IOCTL_CUSTOM_CTL_ID4

#define MAX_CMD_DATA_BUFFER_SIZE    1024
#define CMD_DATA_BUFFER_OFFSET      (450 * 1024)

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct
{
    uint32_t  ithCmdQ;
    uint32_t  ithCmdQ1;
    uint32_t  cmdQBase;
    uint32_t  currPtr;
    uint32_t  cmdQBase1;
    uint32_t  currPtr1;
    uint32_t  gTableID;
    uint32_t  gHudTableIndex;
    uint32_t  gpHudTableData;
    uint32_t  gpHudTableData1;
    uint32_t  gpHudTableData2;
    uint32_t  gpHudTableData3;
    uint32_t  gBlock_W;
    uint32_t  gBlock_H;
    uint32_t  gW_num;
    uint32_t  gH_num;
} HUD_INIT_DATA;

typedef struct
{
    uint32_t  dst;
} HUD_CONTEXT;

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





