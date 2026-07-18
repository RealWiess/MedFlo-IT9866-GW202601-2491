/*
 * Copyright (c) 2022 ITE Tech. Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ITE_ITVP_H_
#define ZEPHYR_DRIVERS_MISC_ITE_ITVP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ITE_ITVP_PIXEL_FORMAT_DITHER565,
	ITE_ITVP_PIXEL_FORMAT_DITHER444,
	ITE_ITVP_PIXEL_FORMAT_RGB565,
	ITE_ITVP_PIXEL_FORMAT_RGB888,
	ITE_ITVP_PIXEL_FORMAT_YUYV,
	ITE_ITVP_PIXEL_FORMAT_YVYU,
	ITE_ITVP_PIXEL_FORMAT_UYVY,
	ITE_ITVP_PIXEL_FORMAT_VYUY,
	ITE_ITVP_PIXEL_FORMAT_YUV422,
	ITE_ITVP_PIXEL_FORMAT_YUV420,
	ITE_ITVP_PIXEL_FORMAT_YUV444,
	ITE_ITVP_PIXEL_FORMAT_YUV422R,
	ITE_ITVP_PIXEL_FORMAT_NV12,
	ITE_ITVP_PIXEL_FORMAT_NV21,
} ite_itvp_pixel_format_t;

typedef enum {
	ITE_ITVP_ROT_NONE,
	ITE_ITVP_ROT_90,
	ITE_ITVP_ROT_270,
	ITE_ITVP_ROT_180,
	ITE_ITVP_ROT_MIRROR,
	ITE_ITVP_ROT_FLIP,
} ite_itvp_rot_t;

typedef enum {
	ITE_ITVP_MODE_DEFAULT,
	ITE_ITVP_MODE_VIDEO,
	ITE_ITVP_MODE_BYPASS,
} ite_itvp_mode_t;

typedef void (*ite_itvp_callback_t)(void *data);

typedef struct {
	struct {
		uint16_t width;
		uint16_t height;
		uint16_t pitch;
		uint16_t uv_pitch;
		ite_itvp_pixel_format_t format;
		uint8_t *data[3];
	} input;

	struct {
		uint16_t width;
		uint16_t height;
		uint16_t pitch;
		uint16_t uv_pitch;
		ite_itvp_pixel_format_t format;
		uint8_t *data[5][3];
		uint8_t frame_count;
		uint8_t const_alpha;
	} output;

	struct {
		uint8_t dither_method_b: 1;
		uint8_t const_alpha: 1;
		uint8_t capture_onfly: 1;
		uint8_t flush_input_cache: 1;
#ifdef CONFIG_ITE_ITVP_CMDQ_SUPPORT
		uint8_t use_cmdq: 1;
#endif
	} flags;

	float hci, vci;
	uint32_t hci_fixed, vci_fixed;
	uint8_t h_weight_matrix[8][4];
	uint8_t v_weight_matrix[8][4];
	uint16_t yuv2rgb_color_matrix[12];
	uint16_t rgb2yuv_color_matrix[12];
	ite_itvp_rot_t rot;
	ite_itvp_mode_t mode;
	ite_itvp_callback_t callback;
	void *callback_data;
} ite_itvp_context_t;

int ite_itvp_ctx_init(ite_itvp_context_t *ctx);
int ite_itvp_process(ite_itvp_context_t *ctx);
int ite_itvp_wait_idle(void);

int itvp_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_ITE_ITVP_H_ */
