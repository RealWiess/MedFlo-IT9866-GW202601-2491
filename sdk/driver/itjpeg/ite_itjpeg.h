/*
 * Copyright (c) 2022 ITE Tech. Inc.
 *
 */

#ifndef ITE_ITJPEG_H_
#define ITE_ITJPEG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ITE_ITJPEG_PIXEL_FORMAT_RGB565,
	ITE_ITJPEG_PIXEL_FORMAT_ARGB4444,
	ITE_ITJPEG_PIXEL_FORMAT_ARGB8888
} ite_itjpeg_pixel_format_t;

typedef struct {
	uint16_t width, height; /* Size of the input image (pixel) */
	uint16_t pitch, uv_pitch;
	ite_itjpeg_pixel_format_t format;
	uint8_t *inbuf; /* Bit stream input buffer */
	uint32_t inbuf_size;
	uint8_t *outbuf[3]; /* Bit stream output buffer */

	union {
		struct {
			uint8_t msx, msy; /* MCU size in unit of block (width, height) */
			uint8_t qtid[3]; /* Quantization table ID of each component, Y, Cb, Cr */
			uint8_t ncomp; /* Number of color components 1:grayscale, 3:color */
			uint16_t nrst; /* Restart interval */
			uint8_t *huffbits[2][2]; /* Huffman bit distribution tables [id][dcac] */
			uint8_t *huffdata[2][2]; /* Huffman decoded data tables [id][dcac] */
			uint8_t *qtdata[3]; /* Quantization tables data [id] */
			int8_t samp[3][2]; /* Sampling factor [id][h/v] */
			uint16_t yuv2rgb_color_matrix[10];
			uint8_t *alpha_buf;
			uint16_t alpha_pitch;
			uint8_t const_alpha;

			struct {
				uint8_t disable_dither : 1;
				uint8_t dither_method : 1;
				uint8_t const_alpha : 1;
				uint8_t rgb_mode : 1;
#ifdef CONFIG_ITE_ITJPEG_CMDQ_SUPPORT
				uint8_t use_cmdq : 1;
#endif
			} flags;
		} decode;
	} op;

} ite_itjpeg_context_t;

void ite_itjpeg_ctx_init(ite_itjpeg_context_t *ctx);
int ite_itjpeg_decode(ite_itjpeg_context_t *ctx);
int ite_itjpeg_wait_idle(void);

int itjpeg_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ITE_ITJPEG_H_ */
