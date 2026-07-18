/*
 * Copyright (c) 2022 ITE Tech. Inc.
 *
 */

#include "ite_itjpeg.h"

#include <assert.h>
#include <sched.h>
#include <string.h>
#include <sys/errno.h>

#include "ite/ith.h"

#define BIT(n)            (1UL << (n))
#define BIT_MASK(n)       (BIT(n) - 1UL)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define ROUND_UP(x, align)                                   \
	((((unsigned long)(x) + ((unsigned long)(align) - 1)) / \
	  (unsigned long)(align)) * (unsigned long)(align))

#define LOG_ERR           ithPrintf

#if (CFG_CHIP_FAMILY == 9860)
    #define REG_JPEG_BASE   0xD0200000
#else
    #define REG_JPEG_BASE   0xB0200000
#endif

#define REG_JPEG_CODEC_CTRL                  0x0000
#define REG_JPEG_TABLE_SPECIFY               0x0004
#define REG_JPEG_DISPLAY_MCU_HEIGHT_Y        0x0008
#define REG_JPEG_LINE_BUF_ADDR_A_COMPONENT_H 0x000C
#define REG_JPEG_LINE_BUF_ADDR_B_COMPONENT_H 0x0010
#define REG_JPEG_LINE_BUF_ADDR_C_COMPONENT_H 0x0014
#define REG_JPEG_Y_PITCH                     0x0018
#define REG_JPEG_BITSTREAM_BUF_ADDR_L        0x001C
#define REG_JPEG_BITSTREAM_BUF_ADDR_H        0x0020
#define REG_JPEG_BITSTREAM_BUF_SIZE_H        0x0024
#define REG_JPEG_BITSTREAM_RW_SIZE_H         0x0028
#define REG_JPEG_SAMPLING_FACTOR_CD          0x002C
#define REG_JPEG_INDEX1_QTABLE               0x006C
#define REG_JPEG_INDEX2_QTABLE               0x00AC
#define REG_JPEG_DROP_DUPLICATE              0x00EC
#define REG_JPEG_ORIGINAL_MCU_WIDTH          0x00F0
#define REG_JPEG_LEFT_MCU_OFFSET             0x00F4
#define REG_JPEG_UP_MCU_OFFSET               0x00F8
#define REG_JPEG_CODEC_FIRE                  0x00FC
#define REG_JPEG_INTR_STATUS                 0x0100
#define REG_JPEG_LINE_BUF_VALID_SLICE        0x0104
#define REG_JPEG_BITSTREAM_VALID_SIZE_L      0x0108
#define REG_JPEG_HUFFMAN_CTRL                0x010C
#define REG_JPEG_HUFFMAN_AC_LUMINANCE_CTRL   0x0110
#define REG_JPEG_RGB_MODE_CTRL               0x1000
#define REG_JPEG_APLHA_PLANE_ADDR_H          0x1004
#define REG_JPEG_YUV_TO_RGB_11               0x1008
#define REG_JPEG_YUV_TO_RGB_21               0x100C
#define REG_JPEG_YUV_TO_RGB_23               0x1010
#define REG_JPEG_YUV_TO_RGB_32               0x1014
#define REG_JPEG_YUV_TO_RGB_CONST_G          0x1018
#define REG_JPEG_DISABLE_DITHER_KEY          0x101C
#define REG_JPEG_EN_CONST_ALPHA              0x1020

#define JPEG_MAX_WIDTH       4096
#define JPEG_MAX_HEIGHT      4096
#define JPEG_MAX_DECODE_SIZE 36000000

#define WAIT_BUSY_TIMEOUT    (500000U)

static const uint16_t yuv2rgb_color_matrix[10] = {
	0x0100, 0x015f, 0x0100, 0x07aa, 0x074d, 0x0100, 0x01bb, 0x0351, 0x0084, 0x0322,
};

struct itjpeg_config {
	uint32_t base;
};

struct itjpeg_data {
#ifdef CONFIG_ITE_ITJPEG_CMDQ_SUPPORT
	bool last_use_cmdq;
#endif
};

static const struct itjpeg_config itjpeg_config = {
	.base = REG_JPEG_BASE,
};

static struct itjpeg_data itjpeg_data;

static inline uint32_t sys_read32(uint32_t addr)
{
    uint32_t value;
    value = *(volatile uint32_t *)addr;
    return value;
}

static inline void sys_write32(uint32_t data, uint32_t addr)
{
    *(volatile uint32_t *)addr = data;
}

static inline void sys_set_mask(uint32_t addr, uint32_t mask, uint32_t value)
{
	uint32_t temp = sys_read32(addr);

	temp &= ~(mask);
	temp |= value;

	sys_write32(temp, addr);
}

static inline void sys_set_bit(uint32_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp | (1 << bit);
}

static inline void sys_clear_bit(uint32_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp & ~(1 << bit);
}

static int itjpeg_wait_finish(void)
{
  uint32_t timeout = WAIT_BUSY_TIMEOUT;
    
	for (;;) {
		uint32_t status =
			sys_read32(itjpeg_config.base + REG_JPEG_INTR_STATUS) & BIT_MASK(16);
		if (status & BIT(0)) {
			break;
		}
		if (status & BIT(2)) {
			LOG_ERR("decode error");
			return -EINVAL;
		}
		if (timeout == 0UL) {
			LOG_ERR("wait finish timeout");
			return -EINVAL;
		}
		timeout--;
    sched_yield();
	}

	return 0;
}

int ite_itjpeg_wait_idle(void)
{
	uint32_t timeout = WAIT_BUSY_TIMEOUT;

#ifdef CONFIG_ITE_ITJPEG_CMDQ_SUPPORT
	if (itjpeg_data.last_use_cmdq) {
		ithCmdQWaitEmpty(ITH_CMDQ0_OFFSET);
		itjpeg_data.last_use_cmdq = false;
	}
#endif

	for (;;) {
		uint32_t status = sys_read32(itjpeg_config.base + REG_JPEG_CODEC_FIRE) >> 16;
		if ((status & 0xF800) != 0x3000) {
			break;
		}
		if (timeout == 0UL) {
			LOG_ERR("wait idle timeout");
			return -EINVAL;
		}
		timeout--;
    sched_yield();
	}

	return 0;
}

void ite_itjpeg_ctx_init(ite_itjpeg_context_t *ctx)
{
	assert(ctx);
	memset(ctx, 0, sizeof(ite_itjpeg_context_t));

	memcpy(ctx->op.decode.yuv2rgb_color_matrix, yuv2rgb_color_matrix,
	       sizeof(yuv2rgb_color_matrix));
	ctx->op.decode.const_alpha = 255;
#if (CFG_CHIP_FAMILY != 9860)
	ctx->op.decode.flags.rgb_mode = 1;
#endif
}

static int ite_itjpeg_decode_default(ite_itjpeg_context_t *ctx)
{
	uint8_t width_unit = ctx->op.decode.msx * 8;
	uint8_t height_unit = ctx->op.decode.msy * 8;
	uint16_t align_width = ROUND_UP(ctx->width, width_unit);
	uint16_t align_height = ROUND_UP(ctx->height, height_unit);
	uint16_t mcu_width = align_width / width_unit;
	uint16_t mcu_height = align_height / height_unit;
	uint32_t bstream_readpos = ((uint32_t)ctx->inbuf) & BIT_MASK(2);
	uint32_t bstream_size = ROUND_UP(ctx->inbuf_size + bstream_readpos, 4);
	uint32_t val, addr, mask, shift;
	int ret, i, j, k;

	/* codec control register */
	/* restart interval, bitstream read position, command mode, JPEG decode output */
	val = (ctx->op.decode.nrst << 16) | (bstream_readpos << 12) | BIT(11);

	if (ctx->op.decode.flags.rgb_mode) {
		/* RGB mode */
		val |= 0x5;
	} else {
		val |= 0x1;
	}

	if (ctx->op.decode.ncomp == 3) {
		/* 1st/2nd/3rd component line buffer valid, component A/B/C valid to decode
		 */
		val |= BIT(9) | BIT(8) | BIT(7) | BIT(5) | BIT(4) | BIT(3);
	} else if (ctx->op.decode.ncomp == 1) {
		/* 1st component line buffer valid, component A valid to decode */
		val |= BIT(7) | BIT(3);
	} else {
		LOG_ERR("invalid components number: %d", ctx->op.decode.ncomp);
		return -EINVAL;
	}

	sys_write32(val, itjpeg_config.base + REG_JPEG_CODEC_CTRL);

	/* table specify register */
	/* huffman table index for decoding component B/C AC/DC */
	val = BIT(9) | BIT(8) | BIT(5) | BIT(4);

	/* quantization table index for component C/B/A */
	val |= (ctx->op.decode.qtid[2] << 10) | (ctx->op.decode.qtid[1] << 6) |
	       (ctx->op.decode.qtid[0] << 2);

	/* display MCU width of Y component */
	val |= mcu_width << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_TABLE_SPECIFY);

	/* display MCU height of Y component register */
	val = mcu_height;

	/* base address of Y component 1 */
	addr = ((uint32_t)ctx->outbuf[0]) >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_DISPLAY_MCU_HEIGHT_Y);

	/* base address of Y component register 2 */
	val = addr >> 16;

	/* base address of U component register 1 */
	addr = ((uint32_t)ctx->outbuf[1]) >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_LINE_BUF_ADDR_A_COMPONENT_H);

	/* base address of U component register 2 */
	val = addr >> 16;

	/* base address of V component register 1 */
	addr = ((uint32_t)ctx->outbuf[2]) >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_LINE_BUF_ADDR_B_COMPONENT_H);

	/* base address of V component register 2 */
	val = addr >> 16;

	/* line buffer number register */
	val |= mcu_height << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_LINE_BUF_ADDR_C_COMPONENT_H);

	/* Y pitch register */
	val = ctx->pitch >> 2;

	/* U,V pitch register */
	val |= (((uint32_t)ctx->uv_pitch) >> 2) << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_Y_PITCH);

	/* base address of bit stream register 1 */
	addr = ((uint32_t)ctx->inbuf) >> 2;
	val = (addr & BIT_MASK(16)) << 16;
	sys_write32(val, itjpeg_config.base + REG_JPEG_BITSTREAM_BUF_ADDR_L);

	/* base address of bit stream register 2 */
	val = addr >> 16;

	/* bitstream buffer size 1 */
	addr = bstream_size >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_BITSTREAM_BUF_ADDR_H);

	/* bit stream size register 2 */
	val = addr >> 16;
	sys_write32(val, itjpeg_config.base + REG_JPEG_BITSTREAM_BUF_SIZE_H);

	/* horizontal, vertical sampling factor 1 */
	val = (ctx->op.decode.samp[1][1] << 12) | (ctx->op.decode.samp[1][0] << 8) |
	      (ctx->op.decode.samp[0][1] << 4) | ctx->op.decode.samp[0][0];
	sys_set_mask(itjpeg_config.base + REG_JPEG_BITSTREAM_RW_SIZE_H, BIT_MASK(16) << 16,
		     val << 16);

	/* horizontal, vertical sampling factor 2 */
	val = (ctx->op.decode.samp[2][1] << 4) | ctx->op.decode.samp[2][0];

	sys_set_mask(itjpeg_config.base + REG_JPEG_SAMPLING_FACTOR_CD, BIT_MASK(16), val);

	/* quantization table of index 1-3 registers */
	for (i = 0; i < 3; i++) {
		const uint8_t *table = ctx->op.decode.qtdata[i];
		if (!table) {
			continue;
		}

		addr = itjpeg_config.base + REG_JPEG_SAMPLING_FACTOR_CD + (0x40 * i);
		val = (((uint32_t)table[1] << 8) | table[0]) << 16;
		sys_set_mask(addr, BIT_MASK(16) << 16, val);

		j = 1;
		for (k = 2; k < 62; k += 4) {
			val = ((uint32_t)table[k + 3] << 24) | (table[k + 2] << 16) |
			      (table[k + 1] << 8) | table[k];
			sys_write32(val, addr + 0x4 * j);
			j++;
		}

		val = ((uint32_t)table[k + 1] << 8) | table[k];
		sys_set_mask(addr + 0x4 * j, BIT_MASK(16), val);
	}

	/* component drop and duplicate specify register */
	val = 0;
	if (ctx->op.decode.ncomp == 3) {
		if ((ctx->op.decode.msx == 1) && (ctx->op.decode.msy == 1)) {
			val = BIT(8) | BIT(4);
		} else if ((ctx->op.decode.msx == 2) && (ctx->op.decode.msy == 2)) {
			if ((ctx->op.decode.samp[1][0] != 1) || (ctx->op.decode.samp[1][1] != 2)) {
				val = BIT(11) | BIT(7);
			}
		} else if ((ctx->op.decode.msx == 4) && (ctx->op.decode.msy == 1)) {
			val = BIT(10) | BIT(6);
		}
	}
	sys_set_mask(itjpeg_config.base + REG_JPEG_DROP_DUPLICATE, BIT_MASK(16) << 16, val << 16);

	/* original image width in MCU unit register */
	val = (mcu_height << 16) | mcu_width;
	sys_write32(val, itjpeg_config.base + REG_JPEG_ORIGINAL_MCU_WIDTH);

	/* partial display image left/right region offset in MCU unit register */
	val = (mcu_width << 16) | 0x1;

	/* number of block of MCU height */
	val |= ctx->op.decode.msy << 11;

	/* number of block of MCU */
	addr = ctx->op.decode.samp[0][0] * ctx->op.decode.samp[0][1] - 1;
	if (ctx->op.decode.ncomp == 3) {
		addr += ctx->op.decode.samp[1][0] * ctx->op.decode.samp[1][1] +
			ctx->op.decode.samp[2][0] * ctx->op.decode.samp[2][1];
	}
	val |= (addr << 11) << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_LEFT_MCU_OFFSET);

	/* partial display image up/down region offset in MCU unit register */
	val = (mcu_height << 16) | 0x1;
	sys_write32(val, itjpeg_config.base + REG_JPEG_UP_MCU_OFFSET);

	/* number of Huffman codes of each length */
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			const uint8_t *huffbits = ctx->op.decode.huffbits[i][j];

			/* start input */
			sys_set_mask(itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, 0x3 << 30,
				     0x3 << 30);

			/* tables selection, reset */
			val = (j << 13) | (i << 12);
			sys_set_mask(itjpeg_config.base + REG_JPEG_HUFFMAN_CTRL, BIT_MASK(16), val);

			/* input codes */
			for (k = 0; k < 15; k++) {
				val = (j << 13) | (i << 12) | ((k + 1) << 8) | huffbits[k];
				sys_set_mask(itjpeg_config.base + REG_JPEG_HUFFMAN_CTRL,
					     BIT_MASK(16), val);
			}

			/* input ended */
			sys_set_mask(itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, 0x3 << 30, 0x0);
		}
	}

	/* DC value associated with each Huffman code */
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			const uint8_t *huffdata = ctx->op.decode.huffdata[i][j];
			int hufflen = 0;

			for (k = 0; k < 16; k++) {
				hufflen += ctx->op.decode.huffbits[i][j][k];
			}

			if (j > 0) {
				/* AC input data */
				/* input start */
				if (i > 0) {
					mask = 0x1 << 30;
					shift = 16;
				} else {
					mask = 0x3 << 30;
					shift = 0;
				}

				sys_set_mask(itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, mask,
					     mask);

				/* input data */
				for (k = 0; k < hufflen; k++) {
					val = (k << 8) | huffdata[k];
					sys_set_mask(itjpeg_config.base +
							     REG_JPEG_HUFFMAN_AC_LUMINANCE_CTRL,
						     BIT_MASK(16) << shift, val << shift);
				}
				/* input ended */
				sys_set_mask(itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, mask,
					     0x0);
			} else {
				/* DC input data */
				const int len = (hufflen + 1) / 2;

				for (k = 0; k < len; k++) {
					/* input start */
					sys_set_bit(itjpeg_config.base + REG_JPEG_TABLE_SPECIFY,
						    30);

					/* input data */
					if ((hufflen & 0x1) && (k == (len - 1))) {
						val = (i << 11) | (k << 8) | huffdata[k * 2];
					} else {
						val = (i << 11) | (k << 8) |
						      (huffdata[k * 2 + 1] << 4) | huffdata[k * 2];
					}
					sys_set_mask(itjpeg_config.base + REG_JPEG_HUFFMAN_CTRL,
						     BIT_MASK(16) << 16, val << 16);

					/* input ended */
					sys_clear_bit(itjpeg_config.base + REG_JPEG_TABLE_SPECIFY,
						      30);
				}
			}
		}
	}

	/* RGB mode register */
	val = ctx->format;

	/* alpha plane base register 1 */
	addr = (uint32_t)ctx->op.decode.alpha_buf >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_RGB_MODE_CTRL);

	/* alpha plane base register 2 */
	val = (addr >> 16) & BIT_MASK(8);

	/* alpha plane pitch register */
	addr = ctx->op.decode.alpha_pitch >> 3;
	val |= addr << 16;

	sys_write32(val, itjpeg_config.base + REG_JPEG_APLHA_PLANE_ADDR_H);

	/* YUV to RGB registers, YUV to RGB constant R/G/B registers */
	addr = itjpeg_config.base + REG_JPEG_YUV_TO_RGB_11;
	for (i = 0; i < ARRAY_SIZE(ctx->op.decode.yuv2rgb_color_matrix); i += 2) {
		val = (ctx->op.decode.yuv2rgb_color_matrix[i + 1] << 16) |
		      ctx->op.decode.yuv2rgb_color_matrix[i];
		sys_write32(val, addr);
		addr += 0x4;
	}

	/* dither disable register */
	val = (ctx->op.decode.flags.dither_method << 16) | ctx->op.decode.flags.disable_dither;
	sys_write32(val, itjpeg_config.base + REG_JPEG_DISABLE_DITHER_KEY);

	/* enable constant alpha register */
	val = (ctx->op.decode.const_alpha << 16) | ctx->op.decode.flags.const_alpha;
	sys_write32(val, itjpeg_config.base + REG_JPEG_EN_CONST_ALPHA);

	/* JPEG codec fire register */
	sys_set_bit(itjpeg_config.base + REG_JPEG_CODEC_FIRE, 0);

	/* bitstream r/w data size register */
	val = bstream_size >> 2;
	sys_set_mask(itjpeg_config.base + REG_JPEG_BITSTREAM_BUF_SIZE_H, BIT_MASK(16) << 16,
		     (val & BIT_MASK(16)) << 16);
	sys_set_mask(itjpeg_config.base + REG_JPEG_BITSTREAM_RW_SIZE_H, BIT_MASK(8),
		     (val >> 16) & BIT_MASK(8));

	/* bitstream buffer control register */
	/* last bitstream data flag, bitstream buffer write end flag */
	val = BIT(2) | BIT(1);
	sys_set_mask(itjpeg_config.base + REG_JPEG_LINE_BUF_VALID_SLICE, BIT_MASK(3) << 16,
		     val << 16);

	ret = itjpeg_wait_finish();

	return ret;
}

#ifdef CONFIG_ITE_ITJPEG_CMDQ_SUPPORT

static uint32_t itjpeg_calc_cmd_size(ite_itjpeg_context_t *ctx)
{
	uint32_t cmd_size = 0;
	int i, j, k;

	/* CODEC_CTRL ~ FACTOR_CD */
	cmd_size += ITH_CMDQ_BURST_CMD_SIZE + REG_JPEG_SAMPLING_FACTOR_CD -
		    REG_JPEG_CODEC_CTRL + 4;

	/* quantization table of index 1-3 registers */
	for (int i = 0; i < 3; i++) {
		const uint8_t *table = ctx->op.decode.qtdata[i];
		if (table) {
			cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 0x40 + 4 + 4;
		}
	}

	/* DROP_DUPLICATE ~ UP_MCU_OFFSET */
	cmd_size += ITH_CMDQ_BURST_CMD_SIZE + REG_JPEG_UP_MCU_OFFSET -
		    REG_JPEG_DROP_DUPLICATE + 4;

	/* number of Huffman codes of each length */
	cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE * 2 * 2 * (1 + 1 + 15 + 1);

	/* DC value associated with each Huffman code */
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			int hufflen = 0;

			for (k = 0; k < 16; k++) {
				hufflen += ctx->op.decode.huffbits[i][j][k];
			}

			if (j > 0) {
				cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE * (1 + hufflen + 1);
			} else {
				const int len = (hufflen + 1) / 2;
				cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE * len * (1 + 1 + 1);
			}
		}
	}

	/* RGB_MODE_CTRL ~ EN_CONST_ALPHA */
	cmd_size += ITH_CMDQ_BURST_CMD_SIZE + REG_JPEG_EN_CONST_ALPHA -
		    REG_JPEG_RGB_MODE_CTRL + 4 + 4;

	/* CODEC_FIRE, BITSTREAM_BUF_SIZE_H, BITSTREAM_RW_SIZE_H, LINE_BUF_VALID_SLICE */
	cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE * 4;

	return cmd_size;
}

static int ite_itjpeg_decode_cmdq(ite_itjpeg_context_t *ctx)
{
	uint8_t width_unit = ctx->op.decode.msx * 8;
	uint8_t height_unit = ctx->op.decode.msy * 8;
	uint16_t align_width = ROUND_UP(ctx->width, width_unit);
	uint16_t align_height = ROUND_UP(ctx->height, height_unit);
	uint16_t mcu_width = align_width / width_unit;
	uint16_t mcu_height = align_height / height_unit;
	uint32_t bstream_readpos = ((uint32_t)ctx->inbuf) & BIT_MASK(2);
	uint32_t bstream_size = ROUND_UP(ctx->inbuf_size + bstream_readpos, 4);
	uint32_t val, addr, mask, shift, cmd_size, *cmd_ptr, table_specify_val, bs_buf_size_h_val,
		bs_rw_size_h_val;
	int i, j, k;

	/* codec control register */
	/* restart interval, bitstream read position, command mode, JPEG decode output */
	val = (ctx->op.decode.nrst << 16) | (bstream_readpos << 12) | BIT(11);

	if (ctx->op.decode.flags.rgb_mode) {
		/* RGB mode */
		val |= 0x5;
	} else {
		val |= 0x1;
	}

	if (ctx->op.decode.ncomp == 3) {
		/* 1st/2nd/3rd component line buffer valid, component A/B/C valid to decode
		 */
		val |= BIT(9) | BIT(8) | BIT(7) | BIT(5) | BIT(4) | BIT(3);
	} else if (ctx->op.decode.ncomp == 1) {
		/* 1st component line buffer valid, component A valid to decode */
		val |= BIT(7) | BIT(3);
	} else {
		LOG_ERR("invalid components number: %d", ctx->op.decode.ncomp);
		return -EINVAL;
	}

	cmd_size = itjpeg_calc_cmd_size(ctx);
  ithCmdQLock(ITH_CMDQ0_OFFSET);
  cmd_ptr = ithCmdQWaitSize(cmd_size, ITH_CMDQ0_OFFSET);
	if (!cmd_ptr) {
		return -EBUSY;
	}

	ITH_CMDQ_BURST_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_CODEC_CTRL,
			     (REG_JPEG_SAMPLING_FACTOR_CD - REG_JPEG_CODEC_CTRL + 4) / 4);

	*cmd_ptr++ = val;

	/* table specify register */
	/* huffman table index for decoding component B/C AC/DC */
	table_specify_val = BIT(9) | BIT(8) | BIT(5) | BIT(4);

	/* quantization table index for component C/B/A */
	table_specify_val |= (ctx->op.decode.qtid[2] << 10) | (ctx->op.decode.qtid[1] << 6) |
			     (ctx->op.decode.qtid[0] << 2);

	/* display MCU width of Y component */
	table_specify_val |= mcu_width << 16;

	*cmd_ptr++ = table_specify_val;

	/* display MCU height of Y component register */
	val = mcu_height;

	/* base address of Y component 1 */
	addr = ((uint32_t)ctx->outbuf[0]) >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	*cmd_ptr++ = val;

	/* base address of Y component register 2 */
	val = addr >> 16;

	/* base address of U component register 1 */
	addr = ((uint32_t)ctx->outbuf[1]) >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	*cmd_ptr++ = val;

	/* base address of U component register 2 */
	val = addr >> 16;

	/* base address of V component register 1 */
	addr = ((uint32_t)ctx->outbuf[2]) >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	*cmd_ptr++ = val;

	/* base address of V component register 2 */
	val = addr >> 16;

	/* line buffer number register */
	val |= mcu_height << 16;

	/* U,V pitch register */
	val |= (((uint32_t)ctx->uv_pitch) >> 2) << 16;

	*cmd_ptr++ = val;

	/* Y pitch register */
	val = ctx->pitch >> 2;
	*cmd_ptr++ = val;

	/* base address of bit stream register 1 */
	addr = ((uint32_t)ctx->inbuf) >> 2;
	val = (addr & BIT_MASK(16)) << 16;
	*cmd_ptr++ = val;

	/* base address of bit stream register 2 */
	val = addr >> 16;

	/* bitstream buffer size 1 */
	addr = bstream_size >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	*cmd_ptr++ = val;

	/* bit stream size register 2 */
	bs_buf_size_h_val = addr >> 16;

	/* bitstream r/w data size register 1 */
	bs_buf_size_h_val |= (addr & BIT_MASK(16)) << 16;

	*cmd_ptr++ = bs_buf_size_h_val;

	/* bitstream r/w data size register 2 */
	bs_rw_size_h_val = addr >> 16;

	/* horizontal, vertical sampling factor 1 */
	bs_rw_size_h_val |= ((ctx->op.decode.samp[1][1] << 12) | (ctx->op.decode.samp[1][0] << 8) |
			     (ctx->op.decode.samp[0][1] << 4) | ctx->op.decode.samp[0][0])
			    << 16;

	*cmd_ptr++ = bs_rw_size_h_val;

	/* horizontal, vertical sampling factor 2 */
	val = (ctx->op.decode.samp[2][1] << 4) | ctx->op.decode.samp[2][0];

	*cmd_ptr++ = val;

	/* quantization table of index 1-3 registers */
	for (i = 0; i < 3; i++) {
		const uint8_t *table = ctx->op.decode.qtdata[i];
		if (!table) {
			continue;
		}

		ITH_CMDQ_BURST_CMD(cmd_ptr,
				     itjpeg_config.base + REG_JPEG_SAMPLING_FACTOR_CD + (0x40 * i),
				     (0x40 + 4) / 4);

		val |= (((uint32_t)table[1] << 8) | table[0]) << 16;
		*cmd_ptr++ = val;

		j = 1;
		for (k = 2; k < 62; k += 4) {
			val = ((uint32_t)table[k + 3] << 24) | (table[k + 2] << 16) |
			      (table[k + 1] << 8) | table[k];
			*cmd_ptr++ = val;

			j++;
		}

		val = ((uint32_t)table[k + 1] << 8) | table[k];
		*cmd_ptr++ = val;

		/* NULL CMD */
		*cmd_ptr++ = 0;
	}

	ITH_CMDQ_BURST_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_DROP_DUPLICATE,
			     (REG_JPEG_UP_MCU_OFFSET - REG_JPEG_DROP_DUPLICATE + 4) / 4);

	/* component drop and duplicate specify register */
	if (ctx->op.decode.ncomp == 3) {
		if ((ctx->op.decode.msx == 1) && (ctx->op.decode.msy == 1)) {
			val |= (BIT(8) | BIT(4)) << 16;
		} else if ((ctx->op.decode.msx == 2) && (ctx->op.decode.msy == 2)) {
			if ((ctx->op.decode.samp[1][0] != 1) || (ctx->op.decode.samp[1][1] != 2)) {
				val |= (BIT(11) | BIT(7)) << 16;
			}
		} else if ((ctx->op.decode.msx == 4) && (ctx->op.decode.msy == 1)) {
			val |= (BIT(10) | BIT(6)) << 16;
		}
	}
	*cmd_ptr++ = val;

	/* original image width in MCU unit register */
	val = (mcu_height << 16) | mcu_width;
	*cmd_ptr++ = val;

	/* partial display image left/right region offset in MCU unit register */
	val = (mcu_width << 16) | 0x1;

	/* number of block of MCU height */
	val |= ctx->op.decode.msy << 11;

	/* number of block of MCU */
	addr = ctx->op.decode.samp[0][0] * ctx->op.decode.samp[0][1] - 1;
	if (ctx->op.decode.ncomp == 3) {
		addr += ctx->op.decode.samp[1][0] * ctx->op.decode.samp[1][1] +
			ctx->op.decode.samp[2][0] * ctx->op.decode.samp[2][1];
	}
	val |= (addr << 11) << 16;

	*cmd_ptr++ = val;

	/* partial display image up/down region offset in MCU unit register */
	val = (mcu_height << 16) | 0x1;
	*cmd_ptr++ = val;

	/* number of Huffman codes of each length */
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			const uint8_t *huffbits = ctx->op.decode.huffbits[i][j];

			/* start input */
			val = table_specify_val | (0x3 << 30);
			ITH_CMDQ_SINGLE_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_TABLE_SPECIFY,
					      val);

			/* tables selection, reset */
			val = (j << 13) | (i << 12);
			ITH_CMDQ_SINGLE_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_HUFFMAN_CTRL,
					      val);

			/* input codes */
			for (k = 0; k < 15; k++) {
				val = (j << 13) | (i << 12) | ((k + 1) << 8) | huffbits[k];
				ITH_CMDQ_SINGLE_CMD(
					cmd_ptr, itjpeg_config.base + REG_JPEG_HUFFMAN_CTRL, val);
			}

			/* input ended */
			ITH_CMDQ_SINGLE_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_TABLE_SPECIFY,
					      table_specify_val);
		}
	}

	/* DC value associated with each Huffman code */
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			const uint8_t *huffdata = ctx->op.decode.huffdata[i][j];
			int hufflen = 0;

			for (k = 0; k < 16; k++) {
				hufflen += ctx->op.decode.huffbits[i][j][k];
			}

			if (j > 0) {
				/* AC input data */
				/* input start */
				if (i > 0) {
					mask = 0x1 << 30;
					shift = 16;
				} else {
					mask = 0x3 << 30;
					shift = 0;
				}

				val = (table_specify_val & ~mask) | mask;
				ITH_CMDQ_SINGLE_CMD(
					cmd_ptr, itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, val);

				/* input data */
				for (k = 0; k < hufflen; k++) {
					val = (k << 8) | huffdata[k];
					val <<= shift;
					ITH_CMDQ_SINGLE_CMD(
						cmd_ptr,
						itjpeg_config.base +
							REG_JPEG_HUFFMAN_AC_LUMINANCE_CTRL,
						val);
				}
				/* input ended */
				val = table_specify_val & ~mask;
				ITH_CMDQ_SINGLE_CMD(
					cmd_ptr, itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, val);
			} else {
				/* DC input data */
				const int len = (hufflen + 1) / 2;

				for (k = 0; k < len; k++) {
					/* input start */
					val = table_specify_val | BIT(30);
					ITH_CMDQ_SINGLE_CMD(
						cmd_ptr,
						itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, val);

					/* input data */
					if ((hufflen & 0x1) && (k == (len - 1))) {
						val = (i << 11) | (k << 8) | huffdata[k * 2];
					} else {
						val = (i << 11) | (k << 8) |
						      (huffdata[k * 2 + 1] << 4) | huffdata[k * 2];
					}

					val <<= 16;
					ITH_CMDQ_SINGLE_CMD(
						cmd_ptr, itjpeg_config.base + REG_JPEG_HUFFMAN_CTRL,
						val);

					/* input ended */
					val = table_specify_val & ~BIT(30);
					ITH_CMDQ_SINGLE_CMD(
						cmd_ptr,
						itjpeg_config.base + REG_JPEG_TABLE_SPECIFY, val);
				}
			}
		}
	}

	ITH_CMDQ_BURST_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_RGB_MODE_CTRL,
			     (REG_JPEG_EN_CONST_ALPHA - REG_JPEG_RGB_MODE_CTRL + 4) / 4);

	/* RGB mode register */
	val = ctx->format;

	/* alpha plane base register 1 */
	addr = (uint32_t)ctx->op.decode.alpha_buf >> 2;
	val |= (addr & BIT_MASK(16)) << 16;

	*cmd_ptr++ = val;

	/* alpha plane base register 2 */
	val = (addr >> 16) & BIT_MASK(8);

	/* alpha plane pitch register */
	addr = ctx->op.decode.alpha_pitch >> 3;
	val |= addr << 16;

	*cmd_ptr++ = val;

	/* YUV to RGB registers, YUV to RGB constant R/G/B registers */
	for (i = 0; i < ARRAY_SIZE(ctx->op.decode.yuv2rgb_color_matrix); i += 2) {
		val = (ctx->op.decode.yuv2rgb_color_matrix[i + 1] << 16) |
		      ctx->op.decode.yuv2rgb_color_matrix[i];
		*cmd_ptr++ = val;
	}

	/* dither disable register */
	val = (ctx->op.decode.flags.dither_method << 16) | ctx->op.decode.flags.disable_dither;
	*cmd_ptr++ = val;

	/* enable constant alpha register */
	val = (ctx->op.decode.const_alpha << 16) | ctx->op.decode.flags.const_alpha;
	*cmd_ptr++ = val;

	/* NULL CMD */
	*cmd_ptr++ = 0;

	/* JPEG codec fire register */
	val = BIT(0);
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_CODEC_FIRE, val);

	/* bitstream r/w data size register */
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_BITSTREAM_BUF_SIZE_H,
			      bs_buf_size_h_val);
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_BITSTREAM_RW_SIZE_H,
			      bs_rw_size_h_val);

	/* bitstream buffer control register */
	/* last bitstream data flag, bitstream buffer write end flag */
	val = (BIT(2) | BIT(1)) << 16;
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itjpeg_config.base + REG_JPEG_LINE_BUF_VALID_SLICE, val);

  ithCmdQFlush(cmd_ptr, ITH_CMDQ0_OFFSET);
  ithCmdQUnlock(ITH_CMDQ0_OFFSET);

	return 0;
}

#endif /* CONFIG_ITE_ITJPEG_CMDQ_SUPPORT */

int ite_itjpeg_decode(ite_itjpeg_context_t *ctx)
{
	assert(ctx);
	int ret = 0;

	if ((ctx->width >= JPEG_MAX_WIDTH) || (ctx->height >= JPEG_MAX_HEIGHT) ||
	    (ctx->width * ctx->height >= JPEG_MAX_DECODE_SIZE) || (ctx->width % 8) ||
	    (ctx->height % 8)) {
		LOG_ERR("invalid width/height: %u/%u", ctx->width, ctx->height);
		return -EINVAL;
	}

	if ((uint32_t)ctx->outbuf[0] % 16) {
		LOG_ERR("output buffer not aligned: 0x%X", (uint32_t)ctx->outbuf[0]);
		return -EINVAL;
	}

	if (!ctx->op.decode.flags.rgb_mode &&
	    ((uint32_t)ctx->outbuf[1] % 16 || (uint32_t)ctx->outbuf[2] % 16)) {
		LOG_ERR("U/V output buffer not aligned: 0x%X 0x%X", (uint32_t)ctx->outbuf[1],
			(uint32_t)ctx->outbuf[2]);
		return -EINVAL;
	}

	if (ctx->op.decode.qtid[0] > 2 || ctx->op.decode.qtid[1] > 2 ||
	    ctx->op.decode.qtid[2] > 2) {
		LOG_ERR("invalid qtid: %d %d %d", ctx->op.decode.qtid[0], ctx->op.decode.qtid[1],
			ctx->op.decode.qtid[2]);
		return -EINVAL;
	}

	if (ctx->op.decode.flags.rgb_mode && (ctx->format != ITE_ITJPEG_PIXEL_FORMAT_RGB565)) {
		if (ctx->op.decode.alpha_pitch % 8) {
			LOG_ERR("invalid alpha pitch: %u", ctx->op.decode.alpha_pitch);
			return -EINVAL;
		}
		if ((uint32_t)ctx->op.decode.alpha_buf % 8) {
			LOG_ERR("alpha buffer not aligned: 0x%X",
				(uint32_t)ctx->op.decode.alpha_buf);
			return -EINVAL;
		}
	}

  ithFlushDCacheRange(ctx->inbuf, ctx->inbuf_size);
	ithInvalidateDCacheRange(ctx->outbuf[0], ctx->pitch * ctx->height);

	if (!ctx->op.decode.flags.rgb_mode) {
		ithInvalidateDCacheRange(ctx->outbuf[1], ctx->uv_pitch * ctx->height);
		ithInvalidateDCacheRange(ctx->outbuf[2], ctx->uv_pitch * ctx->height);
	}

#ifdef CONFIG_ITE_ITJPEG_CMDQ_SUPPORT
	if (ctx->op.decode.flags.use_cmdq) {
		ret = ite_itjpeg_decode_cmdq(ctx);
		itjpeg_data.last_use_cmdq = true;
	} else
#endif
	{
#ifdef CONFIG_ITE_ITJPEG_CMDQ_SUPPORT

		if (itjpeg_data.last_use_cmdq) {
      ithCmdQWaitEmpty(ITH_CMDQ0_OFFSET);
			itjpeg_data.last_use_cmdq = false;
		}
#endif
		ret = ite_itjpeg_decode_default(ctx);
	}

	return ret;
}

int itjpeg_init(void)
{
	ithJpegEnableClock();
    ithJpegResetReg();
    ithJpegResetEngine();

	return 0;
}
