/*
 * Copyright (c) 2022 ITE Tech. Inc.
 */

#ifndef ITE_ITDCPS_H_
#define ITE_ITDCPS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ITE_ITDCPS_MODE_UCL_DEC = 0x0,
	ITE_ITDCPS_MODE_SPEEDY_DEC = 0x2,
	ITE_ITDCPS_MODE_SPEEDY_COMP = 0x3
} ite_itdcps_mode_t;

typedef struct {
	uint32_t dst_size;
	uint8_t *dst;
	uint32_t src_size;
	const uint8_t *src;
	uint32_t out_size;
	int32_t id;
	struct {
		uint8_t mode: 2;
#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT
		uint8_t use_cmdq: 1;
#endif
	} flags;
} ite_itdcps_dsc_t;

typedef enum {
	ITE_ITDCPS_START,
	ITE_ITDCPS_BLOCK_START,
	ITE_ITDCPS_BLOCK_END,
} ite_itdcps_status_t;

typedef struct ite_itdcps_context ite_itdcps_context_t;

typedef int (*ite_itdcps_callback_t)(ite_itdcps_context_t *ctx);

typedef struct ite_itdcps_context {
	ite_itdcps_callback_t dcps_callback;
	ite_itdcps_dsc_t dsc;
	ite_itdcps_status_t status;
} ite_itdcps_context_t;

int ite_itdcps_decompress(ite_itdcps_dsc_t *dsc);
int ite_itdcps_compress(ite_itdcps_dsc_t *dsc);
int ite_itdcps_stream_decompress(ite_itdcps_context_t *ctx);
int ite_itdcps_wait_idle(void);
int ite_itdcps_wait_finish(int32_t id);

int itdcps_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ITE_ITDCPS_H_ */
