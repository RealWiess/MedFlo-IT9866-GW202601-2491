#include <stdlib.h>
#include <string.h>

#define NEC_START_HIGH   9000
#define NEC_START_LOW    4500
#define NEC_BIT_HIGH     560
#define NEC_BIT0_LOW     560
#define NEC_BIT1_LOW     1680
#define NEC_RPT_HIGH     9000
#define NEC_RPT_LOW      2250
#define NEC_TOTAL_BIT    32U
#define ERR_RANGE        15

uint32_t ir_NEC_decoder(uint32_t *data, int, uint8_t*, uint8_t*);
