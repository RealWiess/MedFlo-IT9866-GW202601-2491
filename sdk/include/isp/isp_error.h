#ifndef __ISP_ERROR_H_9QF78O71_NM87_4UCK_V470_VNEL6GRKEC5L__
#define __ISP_ERROR_H_9QF78O71_NM87_4UCK_V470_VNEL6GRKEC5L__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                              Constant Definition
//=============================================================================
/**
 *  ISP error code
 */
#define  ISP_ERR_BASE 0x50000000UL

typedef uint32_t ISP_RESULT;

#define ISP_SUCCESS                     0UL
#define ISP_ERR_NOT_IDLE                (ISP_ERR_BASE + 0x0002UL)
#define ISP_ERR_INPUT_SIZE_ERROR        (ISP_ERR_BASE + 0x0006UL)
#define ISP_ERR_PORT1_SIZE_ERROR        (ISP_ERR_BASE + 0x0007UL)
#define ISP_ERR_CONTEXT_ALLOC_FAIL      (ISP_ERR_BASE + 0x0009UL)
#define ISP_ERR_NOT_INITIALIZE          (ISP_ERR_BASE + 0x000AUL)
#define ISP_ERR_NO_MATCH_ENABLE_TYPE    (ISP_ERR_BASE + 0x000EUL)
#define ISP_ERR_NO_MATCH_DISABLE_TYPE   (ISP_ERR_BASE + 0x000FUL)
#define ISP_ERR_COLOR_CTRL_OUT_OF_RANGE (ISP_ERR_BASE + 0x0012UL)
#define ISP_ERR_UNSUPPORTED_COLOR_CTRL  (ISP_ERR_BASE + 0x0013UL)
#define ISP_ERR_INVALID_GAIN_VALUE      (ISP_ERR_BASE + 0x0014UL)
#define ISP_ERR_INVALID_DISPLAY_WINDOW  (ISP_ERR_BASE + 0x0016UL)
#define ISP_ERR_INVALID_INPUT_PARAM     (ISP_ERR_BASE + 0x0017UL)
#define ISP_ERR_NO_MATCH_OUTPUT_FORMAT  (ISP_ERR_BASE + 0x0018UL)
#define ISP_ERR_NO_MATCH_INPUT_FORMAT   (ISP_ERR_BASE + 0x0019UL)
#define ISP_ERR_QUEUE_FIRE_NOT_IDLE     (ISP_ERR_BASE + 0x0020UL)
#define ISP_ERR_INVALID_PARAM           (ISP_ERR_BASE + 0x0021UL)
#define ISP_ERR_NULL_POINTER            (ISP_ERR_BASE + 0x0221UL)


//=============================================================================
//				  Macro Definition
//=============================================================================

//=============================================================================
//				  Structure Definition
//=============================================================================

//=============================================================================
//				  Global Data Definition
//=============================================================================

//=============================================================================
//				  Private Function Definition
//=============================================================================

//=============================================================================
//				  Public Function Definition
//=============================================================================

#ifdef __cplusplus
}
#endif

#endif
