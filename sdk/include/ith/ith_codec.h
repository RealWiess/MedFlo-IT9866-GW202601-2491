#ifndef ITH_CODEC_H
#define ITH_CODEC_H

#include "ith/ith_utility.h"

/** @addtogroup ith ITE Hardware Library
 *  @{
 */
/** @addtogroup ith_codec Codec
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command for codec on risc.
 */
bool ithCodecCommand(int32_t command, int32_t parameter0, int32_t parameter1, int32_t parameter2);

/**
 * Open engine for codec on risc.
 */
//void ithCodecOpenEngine(void);

/**
 * Read card id from wiegand.
 */
int32_t ithCodecWiegandReadCard(int32_t index, unsigned long long* card_id);

/**
 * Redirect printf message to codec of sw uart.
 */
void ithCodecPrintfWrite(char* string, int32_t length);

void ithCodecCtrlBoardWrite(uint8_t* data, int32_t length);

void ithCodecCtrlBoardRead(uint8_t* data, int32_t length);

void ithCodecHeartBeatRead(uint8_t* data,int32_t length);

#ifdef __cplusplus
}
#endif

#endif // ITH_CODEC_H
/** @} */ // end of ith_codec
/** @} */ // end of ith
