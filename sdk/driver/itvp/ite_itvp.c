/*
 * Copyright (c) 2022 ITE Tech. Inc.
 */

#include "ite_itvp.h"

#include <assert.h>
#include <sched.h>
#include <string.h>
#include <sys/errno.h>

#include "ite/ith.h"

#define BIT(n)      (1UL << (n))
#define BIT_MASK(n) (BIT(n) - 1UL)
#define LOG_ERR     ithPrintf

#define REG_VP_BASE 0xD0300000

#define REG_VP_FIRE              0x0000
#define REG_VP_DEINTERLACE       0x0004
#define REG_VP_EN4               0x0008
#define REG_VP_EN6               0x000C
#define REG_VP_FRAME_Y_ADDR      0x0010
#define REG_VP_FRAME_U_ADDR      0x0014
#define REG_VP_FRAME_V_ADDR      0x0018
#define REG_VP_PRE_FRAME_Y_ADDR  0x001C
#define REG_VP_PRE_FRAME_U_ADDR  0x0020
#define REG_VP_PRE_FRAME_V_ADDR  0x0024
#define REG_VP_Y_PITCH           0x0028
#define REG_VP_SRC_HEIGHT        0x002C
#define REG_VP_SCALE_HCIP        0x0030
#define REG_VP_SCALE_VCIP        0x0034
#define REG_VP_SCALE_HCI         0x0038
#define REG_VP_SCALE_VCI         0x003C
#define REG_VP_DENOISE           0x0040
#define REG_VP_CI_YUV_12         0x0044
#define REG_VP_CI_YUV_21         0x0048
#define REG_VP_CI_YUV_23         0x004C
#define REG_VP_CI_YUV_32         0x0050
#define REG_VP_CI_YUV_CONST_R    0x0054
#define REG_VP_CI_YUV_CONST_G    0x0056
#define REG_VP_CI_YUV_CONST_B    0x0058
#define REG_VP_COLOR_CONV2       0x0044
#define REG_VP_COLOR_CONV4       0x0048
#define REG_VP_COLOR_CONV6       0x004C
#define REG_VP_COLOR_CONV8       0x0050
#define REG_VP_COLOR_CONV10      0x0054
#define REG_VP_COLOR_CONV12      0x0058
#define REG_VP_COLOR_CORR2       0x005C
#define REG_VP_COLOR_CORR4       0x0060
#define REG_VP_COLOR_CORR6       0x0064
#define REG_VP_COLOR_CORR8       0x0068
#define REG_VP_COLOR_CORR10      0x006C
#define REG_VP_COLOR_CORR12      0x0070
#define REG_VP_OUTPUT_1Y         0x0074
#define REG_VP_OUTPUT_1U         0x0078
#define REG_VP_OUTPUT_1V         0x007C
#define REG_VP_OUTPUT_2Y         0x0080
#define REG_VP_OUTPUT_2U         0x0084
#define REG_VP_OUTPUT_2V         0x0088
#define REG_VP_OUTPUT_3Y         0x008C
#define REG_VP_OUTPUT_3U         0x0090
#define REG_VP_OUTPUT_3V         0x0094
#define REG_VP_OUTPUT_4Y         0x0098
#define REG_VP_OUTPUT_4U         0x009C
#define REG_VP_OUTPUT_4V_L       0x00A0
#define REG_VP_OUTPUT_0Y_H       0x00A4
#define REG_VP_OUTPUT_0U_H       0x00A8
#define REG_VP_OUTPUT_0V_H       0x00AC
#define REG_VP_OUTPUT_HEIGHT     0x00B0
#define REG_VP_OUTPUT_PITCH      0x00B4
#define REG_VP_OUTPUT_4V_H       0x00BC
#define REG_VP_BG_COLOR          0x00C0
#define REG_VP_SCALE_DST_X       0x00C4
#define REG_VP_SCALE_DST_WIDTH   0x00C8
#define REG_VP_SRC_POS_Y         0x0100
#define REG_VP_SRC_PANEL_HEIGHT  0x0104
#define REG_VP_SRC_PANEL_COLOR_V 0x0108
#define REG_VP_SCALE_WX00        0x0114
#define REG_VP_SCALE_WX10        0x0118
#define REG_VP_SCALE_WX20        0x011C
#define REG_VP_SCALE_WX30        0x0120
#define REG_VP_SCALE_WX40        0x0124
#define REG_VP_SCALE_WY00        0x0128
#define REG_VP_SCALE_WY10        0x012C
#define REG_VP_SCALE_WY20        0x0130
#define REG_VP_SCALE_WY30        0x0134
#define REG_VP_SCALE_WY40        0x0138
#define REG_VP_PRESCALE_HCI      0x013C
#define REG_VP_PRESCALE_WIDTH    0x0140
#define REG_VP_PRESCALE_WX02     0x0144
#define REG_VP_PRESCALE_WX12     0x0148
#define REG_VP_PRESCALE_WX22     0x014C
#define REG_VP_PRESCALE_WX32     0x0150
#define REG_VP_PRESCALE_WX42     0x0154
#define REG_VP_OUT_YUV_11        0x0164
#define REG_VP_OUT_YUV_13        0x0168
#define REG_VP_OUT_YUV_22        0x016C
#define REG_VP_OUT_YUV_31        0x0170
#define REG_VP_OUT_YUV_33        0x0174
#define REG_VP_OUT_YUV_CONST_U   0x0178
#define REG_VP_STATUS            0x01FC

#define VP_WEIGHT_COUNT    9
#define VP_MAX_WIDTH       1920
#define VP_MAX_HEIGHT      2048
#define WAIT_BUSY_TIMEOUT  (500000U)

struct itvp_config {
	uint32_t base;
};

struct itvp_data {
	ite_itvp_callback_t callback;
	void *callback_data;
#ifdef CONFIG_ITE_ITVP_CMDQ_SUPPORT
	bool last_use_cmdq;
#endif
};

static const struct itvp_config itvp_config = {
	.base = REG_VP_BASE,
};

static struct itvp_data itvp_data;

static const uint16_t yuv2rgb_color_matrix_default[12] = {
	0x0100, 0x0000, 0x015F, 0x0100, 0x07AA, 0x074D,
	0x0100, 0x01BB, 0x0000, 0x0351, 0x0084, 0x0322,
};

static const uint16_t yuv2rgb_color_matrix_video[12] = {
	0x012A, 0x0000, 0x0199, 0x012A, 0x079C, 0x0730,
	0x012A, 0x0205, 0x0000, 0x0321, 0x0087, 0x02EB,
};

static const uint16_t yuv2rgb_color_matrix_bypass[12] = {
	0x0100, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000,
	0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000,
};

static const float scale_ratio_table[VP_WEIGHT_COUNT] = {
	(1088.0f / 1080), (1920.0f / 1920), (1280.0f / 1920), (576.0f / 1080), (480.0f / 1080),
	(720.0f / 1920),  (640.0f / 1920),  (288.0f / 1080),  (352.0f / 1920)};

static const uint8_t weight_matrix_table[VP_WEIGHT_COUNT][5][4] = {
	{{0x0, 0x3f, 0x0, 0x1},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3a, 0x12, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
	{{0x0, 0x40, 0x0, 0x0},
	 {0xfc, 0x3f, 0x8, 0xfd},
	 {0xf9, 0x3b, 0x11, 0xfb},
	 {0xf8, 0x32, 0x1d, 0xf9},
	 {0xf9, 0x28, 0x28, 0xf7}},
};

static const uint16_t rgb2yuv_color_matrix_default[12] = {
	0x0042, 0x0081, 0x0019, 0x03DA, 0x03B6, 0x0070,
	0x0070, 0x03A2, 0x03EE, 0x0010, 0x0080, 0x0080,
};

static const uint16_t rgb2yuv_color_matrix_video[12] = {
	0x004D, 0x0096, 0x001D, 0x03D4, 0x03A9, 0x0083,
	0x0083, 0x0392, 0x03EB, 0x0000, 0x0080, 0x0080,
};

static const uint16_t rgb2yuv_color_matrix_bypass[12] = {
	0x0100, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000,
	0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000,
};

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

static inline float scale_factor(uint16_t in, uint16_t out)
{
	return (float)(((int)(16384.0f * in / (float)out)) / 16384.0f);
}

static uint32_t float_to_fixed(float f, int e, int m)
{
	uint32_t val;
	int i;

	memcpy(&val, &f, sizeof(uint32_t));

	if (f < 0.0f) {
		f *= -1.0f;
		for (i = 0; i < m; i++) {
			f *= 2.0f;
		}
		val = (uint32_t)f;
		val = ~val + 1;
	} else {
		for (i = 0; i < m; i++) {
			f *= 2.0f;
		}
		val = (uint32_t)f;
	}

	return val;
}

static int itvp_wait_finish(void)
{
  uint32_t timeout = WAIT_BUSY_TIMEOUT;
  for (;;) {
    uint32_t status = sys_read32(itvp_config.base + REG_VP_STATUS);
    if (!(status & BIT(5)) && !(status & BIT(10))) {
      break;
    }
    if (timeout == 0UL) {
      LOG_ERR("process timeout");
      return -EINVAL;
    }
    timeout--;
    sched_yield();
  }

	return 0;
}

int ite_itvp_wait_idle(void)
{
#ifdef CONFIG_ITE_ITVP_CMDQ_SUPPORT
	if (itvp_data.last_use_cmdq) {
        ithCmdQWaitEmpty(ITH_CMDQ0_OFFSET);
		itvp_data.last_use_cmdq = false;
	}
#endif
	return itvp_wait_finish();
}

int ite_itvp_ctx_init(ite_itvp_context_t *ctx)
{
	assert(ctx);
	int i, index;

	if ((ctx->input.width % 2) || (ctx->input.height % 2)) {
		LOG_ERR("invalid input width/height: %u/%u", ctx->input.width, ctx->input.height);
		return -EINVAL;
	}

	if ((ctx->output.width >= VP_MAX_WIDTH) || (ctx->output.height >= VP_MAX_HEIGHT)) {
		LOG_ERR("invalid output width/height: %u/%u", ctx->output.width,
			ctx->output.height);
		return -EINVAL;
	}

	ctx->hci = scale_factor(ctx->input.width, ctx->output.width);
	ctx->vci = scale_factor(ctx->input.height, ctx->output.height);
	ctx->hci_fixed = float_to_fixed(ctx->hci, 6, 14);
	ctx->vci_fixed = float_to_fixed(ctx->vci, 6, 14);

	index = VP_WEIGHT_COUNT - 1;
	for (i = 0; i < VP_WEIGHT_COUNT; i++) {
		if (ctx->hci >= scale_ratio_table[i]) {
			index = i;
			break;
		}
	}
	memcpy(ctx->h_weight_matrix, &weight_matrix_table[index], sizeof(ctx->h_weight_matrix));

	index = VP_WEIGHT_COUNT - 1;
	for (i = 0; i < VP_WEIGHT_COUNT; i++) {
		if (ctx->vci >= scale_ratio_table[i]) {
			index = i;
			break;
		}
	}
	memcpy(ctx->v_weight_matrix, &weight_matrix_table[index], sizeof(ctx->v_weight_matrix));

	switch (ctx->mode) {
	case ITE_ITVP_MODE_DEFAULT:
		memcpy(ctx->yuv2rgb_color_matrix, yuv2rgb_color_matrix_default,
		       sizeof(ctx->yuv2rgb_color_matrix));
		memcpy(ctx->rgb2yuv_color_matrix, rgb2yuv_color_matrix_default,
		       sizeof(ctx->rgb2yuv_color_matrix));
		break;

	case ITE_ITVP_MODE_VIDEO:
		memcpy(ctx->yuv2rgb_color_matrix, yuv2rgb_color_matrix_video,
		       sizeof(ctx->yuv2rgb_color_matrix));
		memcpy(ctx->rgb2yuv_color_matrix, rgb2yuv_color_matrix_video,
		       sizeof(ctx->rgb2yuv_color_matrix));
		break;

	case ITE_ITVP_MODE_BYPASS:
		memcpy(ctx->yuv2rgb_color_matrix, yuv2rgb_color_matrix_bypass,
		       sizeof(ctx->yuv2rgb_color_matrix));
		memcpy(ctx->rgb2yuv_color_matrix, rgb2yuv_color_matrix_bypass,
		       sizeof(ctx->rgb2yuv_color_matrix));
		break;

	default:
		LOG_ERR("invalid mode: %d", ctx->mode);
		return -EINVAL;
	}

	return 0;
}

static int ite_itvp_process_default(ite_itvp_context_t *ctx)
{
	uint32_t val;
	int ret, i, j;
	bool in_rgb, in_yuv, out_rgb, out_yuv, color_space_conv_en;

	in_rgb = in_yuv = out_rgb = out_yuv = color_space_conv_en = false;

	switch (ctx->input.format) {
	case ITE_ITVP_PIXEL_FORMAT_RGB565:
		/* enable RGB565 packet mode input */
		val = BIT(4);
		in_rgb = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_RGB888:
		/* enable RGB888 packet mode input */
		val = BIT(5);
		in_rgb = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUYV:
	case ITE_ITVP_PIXEL_FORMAT_YVYU:
	case ITE_ITVP_PIXEL_FORMAT_UYVY:
	case ITE_ITVP_PIXEL_FORMAT_VYUY:
		/* YUV packet input mode */
		val = (ctx->input.format - ITE_ITVP_PIXEL_FORMAT_YUYV) << 10;
		/* enable packet mode input */
		val |= BIT(3);
		in_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUV422:
	case ITE_ITVP_PIXEL_FORMAT_YUV420:
	case ITE_ITVP_PIXEL_FORMAT_YUV444:
	case ITE_ITVP_PIXEL_FORMAT_YUV422R:
		/* YUV plane input mode */
		val = (ctx->input.format - ITE_ITVP_PIXEL_FORMAT_YUV422) << 8;
		/* enable plane mode input */
		val |= BIT(2);
		in_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_NV12:
	case ITE_ITVP_PIXEL_FORMAT_NV21:
		/* NV input mode */
		val = (ctx->input.format - ITE_ITVP_PIXEL_FORMAT_NV12) << 1;
		/* enable NV mode input */
		val |= BIT(0);
		in_yuv = true;
		break;

	default:
		LOG_ERR("invalid input pixel format: %d", ctx->input.format);
		return -EINVAL;
	}

	/* input mode */
	sys_set_mask(itvp_config.base + REG_VP_DEINTERLACE, BIT_MASK(16) << 16, val << 16);

	if (ctx->flags.capture_onfly) {
		/* capture onfly mode */
		val = BIT(2);
	} else {
		/* read memory mode */
		val = BIT(1);
	}

	switch (ctx->output.format) {
	case ITE_ITVP_PIXEL_FORMAT_DITHER565:
	case ITE_ITVP_PIXEL_FORMAT_DITHER444:
	case ITE_ITVP_PIXEL_FORMAT_RGB565:
	case ITE_ITVP_PIXEL_FORMAT_RGB888:
		/* RGB packet mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_DITHER565) << (16 + 14);
		/* dither method */
		val |= ctx->flags.dither_method_b << (16 + 7);
		out_rgb = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUYV:
	case ITE_ITVP_PIXEL_FORMAT_YVYU:
	case ITE_ITVP_PIXEL_FORMAT_UYVY:
	case ITE_ITVP_PIXEL_FORMAT_VYUY:
		/* YUV packet mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_YUYV) << (16 + 12);
		/* YUV packet output mode */
		val |= 0x1 << (16 + 8);
		out_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUV422:
	case ITE_ITVP_PIXEL_FORMAT_YUV420:
	case ITE_ITVP_PIXEL_FORMAT_YUV444:
	case ITE_ITVP_PIXEL_FORMAT_YUV422R:
		/* YUV plane mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_YUV422) << (16 + 10);
		/* YUV plane output mode */
		val |= 0x2 << (16 + 8);
		out_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_NV12:
	case ITE_ITVP_PIXEL_FORMAT_NV21:
		/* NV output mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_NV12) << 5;
		/* enable NV mode output */
		val |= BIT(4);
		out_yuv = true;
		break;

	default:
		LOG_ERR("invalid output pixel format: %d", ctx->input.format);
		return -EINVAL;
	}

	/* output mode */
	sys_write32(val, itvp_config.base + REG_VP_EN4);

	if (!ctx->flags.capture_onfly) {
		/* input address Y */
		sys_write32((uint32_t)ctx->input.data[0], itvp_config.base + REG_VP_FRAME_Y_ADDR);

		/* input address V */
		sys_write32((uint32_t)ctx->input.data[1], itvp_config.base + REG_VP_FRAME_U_ADDR);

		/* input address U */
		sys_write32((uint32_t)ctx->input.data[2], itvp_config.base + REG_VP_FRAME_V_ADDR);
	}
	/* input pitch Y/UV */
	val = (ctx->input.uv_pitch) << 16 | ctx->input.pitch;
	sys_write32(val, itvp_config.base + REG_VP_Y_PITCH);

	/* input width/height */
	val = (ctx->input.width) << 16 | ctx->input.height;
	sys_write32(val, itvp_config.base + REG_VP_SRC_HEIGHT);

	/* scaling factor in H direction (6.14) */
	sys_write32(ctx->hci_fixed, itvp_config.base + REG_VP_SCALE_HCI);

	/* scaling factor in V direction (6.14) */
	sys_write32(ctx->vci_fixed, itvp_config.base + REG_VP_SCALE_VCI);

	if (in_yuv || out_rgb) {
		/* YUV to RGB color space conversion matrix */
		val = ctx->yuv2rgb_color_matrix[0];
		sys_set_mask(itvp_config.base + REG_VP_DENOISE, BIT_MASK(11) << 16, val << 16);

		for (i = 0; i < 5; i++) {
			val = ctx->yuv2rgb_color_matrix[i * 2 + 1];
			val |= (uint32_t)ctx->yuv2rgb_color_matrix[i * 2 + 2] << 16;
			sys_write32(val, itvp_config.base + REG_VP_CI_YUV_12 + (4 * i));
		}

		val = ctx->yuv2rgb_color_matrix[11];
		sys_set_mask(itvp_config.base + REG_VP_COLOR_CONV12, BIT_MASK(10), val);

		color_space_conv_en = true;
	}

	if (ctx->flags.capture_onfly) {
		if (ctx->output.frame_count > 1) {
			sys_write32((uint32_t)ctx->output.data[1][0],
				    itvp_config.base + REG_VP_OUTPUT_1Y);
			sys_write32((uint32_t)ctx->output.data[1][1],
				    itvp_config.base + REG_VP_OUTPUT_1U);
			sys_write32((uint32_t)ctx->output.data[1][2],
				    itvp_config.base + REG_VP_OUTPUT_1V);
		}
		if (ctx->output.frame_count > 2) {
			sys_write32((uint32_t)ctx->output.data[2][0],
				    itvp_config.base + REG_VP_OUTPUT_2Y);
			sys_write32((uint32_t)ctx->output.data[2][1],
				    itvp_config.base + REG_VP_OUTPUT_2U);
			sys_write32((uint32_t)ctx->output.data[2][2],
				    itvp_config.base + REG_VP_OUTPUT_2V);
		}
		if (ctx->output.frame_count > 3) {
			sys_write32((uint32_t)ctx->output.data[3][0],
				    itvp_config.base + REG_VP_OUTPUT_3Y);
			sys_write32((uint32_t)ctx->output.data[3][1],
				    itvp_config.base + REG_VP_OUTPUT_3U);
			sys_write32((uint32_t)ctx->output.data[3][2],
				    itvp_config.base + REG_VP_OUTPUT_3V);
		}
		if (ctx->output.frame_count > 4) {
			sys_write32((uint32_t)ctx->output.data[4][0],
				    itvp_config.base + REG_VP_OUTPUT_4Y);
			sys_write32((uint32_t)ctx->output.data[4][1],
				    itvp_config.base + REG_VP_OUTPUT_4U);
			sys_set_mask(itvp_config.base + REG_VP_OUTPUT_4V_L, BIT_MASK(16),
				     (uint32_t)ctx->output.data[4][0]);
			sys_set_mask(itvp_config.base + REG_VP_OUTPUT_4V_H, BIT_MASK(16) << 16,
				     (uint32_t)ctx->output.data[4][0]);
		}
	}

	/* output address Y */
	sys_set_mask(itvp_config.base + REG_VP_OUTPUT_4V_L, BIT_MASK(16) << 16,
		     (uint32_t)ctx->output.data[0][0] << 16);

	/* output address U/Y */
	val = ((uint32_t)ctx->output.data[0][1] << 16) | ((uint32_t)ctx->output.data[0][0] >> 16);
	sys_write32(val, itvp_config.base + REG_VP_OUTPUT_0Y_H);

	/* output address V/U */
	val = ((uint32_t)ctx->output.data[0][2] << 16) | ((uint32_t)ctx->output.data[0][1] >> 16);
	sys_write32(val, itvp_config.base + REG_VP_OUTPUT_0U_H);

	/* output width, output address */
	val = ((uint32_t)ctx->output.width << 16) | ((uint32_t)ctx->output.data[0][2] >> 16);
	sys_write32(val, itvp_config.base + REG_VP_OUTPUT_0V_H);

	/* output pitch Y, output height */
	val = ((uint32_t)ctx->output.pitch << 16) | (ctx->output.height);
	sys_write32(val, itvp_config.base + REG_VP_OUTPUT_HEIGHT);

	/* output pitch UV */
	sys_set_mask(itvp_config.base + REG_VP_OUTPUT_PITCH, BIT_MASK(16), ctx->output.uv_pitch);

	/* scaling height/width */
	val = ((uint32_t)ctx->output.height << 16) | (ctx->output.width);
	sys_write32(val, itvp_config.base + REG_VP_SCALE_DST_WIDTH);

	/* source panel width */
	val = (uint32_t)ctx->input.width << 16;
	sys_write32(val, itvp_config.base + REG_VP_SRC_POS_Y);

	/* source panel height */
	sys_write32(ctx->input.height, itvp_config.base + REG_VP_SRC_PANEL_HEIGHT);

#if (CFG_CHIP_FAMILY != 9860)
	if (out_rgb) {
		if (ctx->flags.const_alpha) {
			/* const alpha enable, const alpha */
			val = BIT(9) | ctx->output.const_alpha;
			sys_set_mask(itvp_config.base + REG_VP_SRC_PANEL_COLOR_V, BIT_MASK(9) << 16,
				     val << 16);
		} else {
			sys_clear_bit(itvp_config.base + REG_VP_SRC_PANEL_COLOR_V, 25);
		}
	}
#endif

	/* scale factors in H direction */
	for (i = 0; i < 5; i++) {
		val = 0;
		for (j = 0; j < 4; j++) {
			val |= (uint32_t)ctx->h_weight_matrix[i][j] << (8 * j);
		}
		sys_write32(val, itvp_config.base + REG_VP_SCALE_WX00 + (4 * i));
	}

	/* scale factors in V direction */
	for (i = 0; i < 5; i++) {
		val = 0;
		for (j = 0; j < 4; j++) {
			val |= (uint32_t)ctx->v_weight_matrix[i][j] << (8 * j);
		}
		sys_write32(val, itvp_config.base + REG_VP_SCALE_WY00 + (4 * i));
	}

	/* pre-scale hci, 1.0 on fixed point */
	sys_write32(0x4000, itvp_config.base + REG_VP_PRESCALE_HCI);

	/* pre-scale parameter/width */
	val = (uint32_t)ctx->h_weight_matrix[0][1] << 24;
	val |= (uint32_t)ctx->h_weight_matrix[0][0] << 16;
	val |= ctx->input.width;
	sys_write32(val, itvp_config.base + REG_VP_PRESCALE_WIDTH);

	/* pre-scale parameters */
	for (i = 0; i < 4; i++) {
		val = (uint32_t)ctx->h_weight_matrix[i][2];
		val |= (uint32_t)ctx->h_weight_matrix[i][3] << 8;
		val |= (uint32_t)ctx->h_weight_matrix[i + 1][0] << 16;
		val |= (uint32_t)ctx->h_weight_matrix[i + 1][1] << 24;
		sys_write32(val, itvp_config.base + REG_VP_PRESCALE_WX02 + (4 * i));
	}

	val = (uint32_t)ctx->h_weight_matrix[4][2];
	val |= (uint32_t)ctx->h_weight_matrix[4][3] << 8;
	sys_set_mask(itvp_config.base + REG_VP_PRESCALE_WX42, BIT_MASK(16), val);

	if (out_yuv) {
		/* RGB to YUV color space conversion matrix */
		for (i = 0; i < 6; i++) {
			val = ctx->rgb2yuv_color_matrix[i * 2];
			val |= (uint32_t)ctx->rgb2yuv_color_matrix[i * 2 + 1] << 16;
			sys_write32(val, itvp_config.base + REG_VP_OUT_YUV_11 + (4 * i));
		}
		color_space_conv_en = true;
	}

	if (ctx->flags.flush_input_cache) {
  	if (in_rgb) {
  		ithFlushDCacheRange(ctx->input.data[0],
  					   ctx->input.pitch * ctx->input.height);
  	} else if (in_yuv) {
  		ithFlushDCacheRange(ctx->input.data[0],
  					   ctx->input.pitch * ctx->input.height);
  		ithFlushDCacheRange(ctx->input.data[1],
  					   ctx->input.uv_pitch * ctx->input.height);
			ithFlushDCacheRange(ctx->input.data[2],
						   ctx->input.uv_pitch * ctx->input.height);
		}
	}

	/* fire engine */
	val = BIT(0);

	/* rotate direction */
	val |= ctx->rot << 26;

	if (ctx->flags.capture_onfly) {
		/* output buffer count */
		val |= (ctx->output.frame_count - 1) << 23;
	}

	/* enable color space conv */
	if (color_space_conv_en) {
		val |= BIT(17);
	}

	sys_write32(val, itvp_config.base + REG_VP_FIRE);

	if (ctx->flags.capture_onfly) {
		itvp_data.callback = ctx->callback;
		itvp_data.callback_data = ctx->callback_data;
	} else {
		ret = itvp_wait_finish();
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#ifdef CONFIG_ITE_ITVP_CMDQ_SUPPORT

static uint32_t itvp_calc_cmd_size(bool in_rgb, bool in_yuv, bool out_rgb, bool out_yuv)
{
	uint32_t cmd_size = 0;

	/* REG_VP_DEINTERLACE ~ REG_VP_FRAME_V_ADDR */
	cmd_size += ITE_ITCMDQ_BURST_CMD_HEADER_SIZE + REG_VP_FRAME_V_ADDR - REG_VP_DEINTERLACE + 4;

	/* REG_VP_Y_PITCH, REG_VP_SRC_HEIGHT */
	cmd_size += ITE_ITCMDQ_SINGLE_CMD_SIZE * 2;

	/* REG_VP_SCALE_HCI, REG_VP_SCALE_VCI */
	cmd_size += ITE_ITCMDQ_SINGLE_CMD_SIZE * 2;

	if (in_yuv || out_rgb) {
		/* REG_VP_DENOISE ~ REG_VP_CI_YUV_CONST_R */
		cmd_size += ITE_ITCMDQ_BURST_CMD_HEADER_SIZE + REG_VP_CI_YUV_CONST_R -
			    REG_VP_DENOISE + 4;

		/* REG_VP_COLOR_CONV12 */
		cmd_size += ITE_ITCMDQ_SINGLE_CMD_SIZE * 1;
	}

	/* REG_VP_OUTPUT_4V_L ~ REG_VP_OUTPUT_PITCH */
	cmd_size += ITE_ITCMDQ_BURST_CMD_HEADER_SIZE + REG_VP_OUTPUT_PITCH - REG_VP_OUTPUT_4V_L + 4;

	/* REG_VP_SCALE_DST_WIDTH, REG_VP_SRC_POS_Y, REG_VP_SRC_PANEL_HEIGHT */
	cmd_size += ITE_ITCMDQ_SINGLE_CMD_SIZE * 3;

#if (CFG_CHIP_FAMILY != 9860)
	if (out_rgb) {
		/* REG_VP_SRC_PANEL_COLOR_V */
		cmd_size += ITE_ITCMDQ_SINGLE_CMD_SIZE * 1;
	}
#endif

	/* REG_VP_SCALE_WX00 ~ REG_VP_PRESCALE_WX42 */
	cmd_size +=
		ITE_ITCMDQ_BURST_CMD_HEADER_SIZE + REG_VP_PRESCALE_WX42 - REG_VP_SCALE_WX00 + 4 + 4;

	if (out_yuv) {
		/* REG_VP_OUT_YUV_11 ~ REG_VP_OUT_YUV_CONST_U */
		cmd_size += ITE_ITCMDQ_BURST_CMD_HEADER_SIZE + REG_VP_OUT_YUV_CONST_U -
			    REG_VP_OUT_YUV_11 + 4;
	}

	/* REG_VP_FIRE */
	cmd_size += ITE_ITCMDQ_SINGLE_CMD_SIZE * 1;

	return cmd_size;
}

static int ite_itvp_process_cmdq(ite_itvp_context_t *ctx)
{
	bool in_rgb, in_yuv, out_rgb, out_yuv, color_space_conv_en;
	uint32_t val, cmd_size, *cmd_ptr, input_mode_val;
	int i, j;

	in_rgb = in_yuv = out_rgb = out_yuv = color_space_conv_en = false;

	switch (ctx->input.format) {
	case ITE_ITVP_PIXEL_FORMAT_RGB565:
		/* enable RGB565 packet mode input */
		input_mode_val = BIT(4);
		in_rgb = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_RGB888:
		/* enable RGB888 packet mode input */
		input_mode_val = BIT(5);
		in_rgb = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUYV:
	case ITE_ITVP_PIXEL_FORMAT_YVYU:
	case ITE_ITVP_PIXEL_FORMAT_UYVY:
	case ITE_ITVP_PIXEL_FORMAT_VYUY:
		/* YUV packet input mode */
		input_mode_val = (ctx->input.format - ITE_ITVP_PIXEL_FORMAT_YUYV) << 10;
		/* enable packet mode input */
		input_mode_val |= BIT(3);
		in_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUV422:
	case ITE_ITVP_PIXEL_FORMAT_YUV420:
	case ITE_ITVP_PIXEL_FORMAT_YUV444:
	case ITE_ITVP_PIXEL_FORMAT_YUV422R:
		/* YUV plane input mode */
		input_mode_val = (ctx->input.format - ITE_ITVP_PIXEL_FORMAT_YUV422) << 8;
		/* enable plane mode input */
		input_mode_val |= BIT(2);
		in_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_NV12:
	case ITE_ITVP_PIXEL_FORMAT_NV21:
		/* NV input mode */
		input_mode_val = (ctx->input.format - ITE_ITVP_PIXEL_FORMAT_NV12) << 1;
		/* enable NV mode input */
		input_mode_val |= BIT(0);
		in_yuv = true;
		break;

	default:
		LOG_ERR("invalid input pixel format: %d", ctx->input.format);
		return -EINVAL;
	}

	/* read memory mode */
	val = BIT(1);

	switch (ctx->output.format) {
	case ITE_ITVP_PIXEL_FORMAT_DITHER565:
	case ITE_ITVP_PIXEL_FORMAT_DITHER444:
	case ITE_ITVP_PIXEL_FORMAT_RGB565:
	case ITE_ITVP_PIXEL_FORMAT_RGB888:
		/* RGB packet mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_DITHER565) << (16 + 14);
		/* dither method */
		val |= ctx->flags.dither_method_b << (16 + 7);
		out_rgb = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUYV:
	case ITE_ITVP_PIXEL_FORMAT_YVYU:
	case ITE_ITVP_PIXEL_FORMAT_UYVY:
	case ITE_ITVP_PIXEL_FORMAT_VYUY:
		/* YUV packet mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_YUYV) << (16 + 12);
		/* YUV packet output mode */
		val |= 0x1 << (16 + 8);
		out_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_YUV422:
	case ITE_ITVP_PIXEL_FORMAT_YUV420:
	case ITE_ITVP_PIXEL_FORMAT_YUV444:
	case ITE_ITVP_PIXEL_FORMAT_YUV422R:
		/* YUV plane mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_YUV422) << (16 + 10);
		/* YUV plane output mode */
		val |= 0x2 << (16 + 8);
		out_yuv = true;
		break;

	case ITE_ITVP_PIXEL_FORMAT_NV12:
	case ITE_ITVP_PIXEL_FORMAT_NV21:
		/* NV output mode */
		val |= (ctx->output.format - ITE_ITVP_PIXEL_FORMAT_NV12) << 5;
		/* enable NV mode output */
		val |= BIT(4);
		out_yuv = true;
		break;

	default:
		LOG_ERR("invalid output pixel format: %d", ctx->input.format);
		return -EINVAL;
	}

	cmd_size = itvp_calc_cmd_size(in_rgb, in_yuv, out_rgb, out_yuv);
	cmd_ptr = ite_itcmdq_lock(itvp_config.cmdq_dev, cmd_size);
	if (!cmd_ptr) {
		return -EBUSY;
	}

	ITH_CMDQ_BURST_CMD(cmd_ptr, itvp_config.base + REG_VP_DEINTERLACE,
			     (REG_VP_FRAME_V_ADDR - REG_VP_DEINTERLACE + 4) / 4);

	/* input mode */
	*cmd_ptr++ = input_mode_val << 16;

	/* output mode */
	*cmd_ptr++ = val;

	val = 0;
	*cmd_ptr++ = val;

	/* input address Y */
	val = (uint32_t)ctx->input.data[0];
	*cmd_ptr++ = val;

	/* input address V */
	val = (uint32_t)ctx->input.data[1];
	*cmd_ptr++ = val;

	/* input address U */
	val = (uint32_t)ctx->input.data[2];
	*cmd_ptr++ = val;

	/* input pitch Y/UV */
	val = (ctx->input.uv_pitch) << 16 | ctx->input.pitch;
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_Y_PITCH, val);

	/* input width/height */
	val = (ctx->input.width) << 16 | ctx->input.height;
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_SRC_HEIGHT, val);

	/* scaling factor in H direction (6.14) */
	val = ctx->hci_fixed;
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_SCALE_HCI, val);

	/* scaling factor in V direction (6.14) */
	val = ctx->vci_fixed;
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_SCALE_VCI, val);

	if (in_yuv || out_rgb) {
		ITH_CMDQ_BURST_CMD(cmd_ptr, itvp_config.base + REG_VP_DENOISE,
				     (REG_VP_CI_YUV_CONST_R - REG_VP_DENOISE + 4) / 4);

		/* YUV to RGB color space conversion matrix */
		val = ctx->yuv2rgb_color_matrix[0] << 16;
		*cmd_ptr++ = val;

		for (i = 0; i < 5; i++) {
			val = ctx->yuv2rgb_color_matrix[i * 2 + 1];
			val |= (uint32_t)ctx->yuv2rgb_color_matrix[i * 2 + 2] << 16;
			*cmd_ptr++ = val;
		}

		val = ctx->yuv2rgb_color_matrix[11];
		ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_COLOR_CONV12, val);

		color_space_conv_en = true;
	}

	ITH_CMDQ_BURST_CMD(cmd_ptr, itvp_config.base + REG_VP_OUTPUT_4V_L,
			     (REG_VP_OUTPUT_PITCH - REG_VP_OUTPUT_4V_L + 4) / 4);

	/* output address Y */
	val = (uint32_t)ctx->output.data[0][0] << 16;
	*cmd_ptr++ = val;

	/* output address U/Y */
	val = ((uint32_t)ctx->output.data[0][1] << 16) | ((uint32_t)ctx->output.data[0][0] >> 16);
	*cmd_ptr++ = val;

	/* output address V/U */
	val = ((uint32_t)ctx->output.data[0][2] << 16) | ((uint32_t)ctx->output.data[0][1] >> 16);
	*cmd_ptr++ = val;

	/* output width, output address */
	val = ((uint32_t)ctx->output.width << 16) | ((uint32_t)ctx->output.data[0][2] >> 16);
	*cmd_ptr++ = val;

	/* output pitch Y, output height */
	val = ((uint32_t)ctx->output.pitch << 16) | (ctx->output.height);
	*cmd_ptr++ = val;

	/* output pitch UV */
	val = ctx->output.uv_pitch;
	*cmd_ptr++ = val;

	/* scaling height/width */
	val = ((uint32_t)ctx->output.height << 16) | (ctx->output.width);
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_SCALE_DST_WIDTH, val);

	/* source panel width */
	val = (uint32_t)ctx->input.width << 16;
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_SRC_POS_Y, val);

	/* source panel height */
	val = ctx->input.height;
	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_SRC_PANEL_HEIGHT, val);

#if (CFG_CHIP_FAMILY != 9860)
	if (out_rgb) {
		if (ctx->flags.const_alpha) {
			/* const alpha enable, const alpha */
			val = (BIT(9) | ctx->output.const_alpha) << 16;
		} else {
			val = 0;
		}
		ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_SRC_PANEL_COLOR_V, val);
	}
#endif

	ITH_CMDQ_BURST_CMD(cmd_ptr, itvp_config.base + REG_VP_SCALE_WX00,
			     (REG_VP_PRESCALE_WX42 - REG_VP_SCALE_WX00 + 4 + 4) / 4);

	/* scale factors in H direction */
	for (i = 0; i < 5; i++) {
		val = 0;
		for (j = 0; j < 4; j++) {
			val |= (uint32_t)ctx->h_weight_matrix[i][j] << (8 * j);
		}
		*cmd_ptr++ = val;
	}

	/* scale factors in V direction */
	for (i = 0; i < 5; i++) {
		val = 0;
		for (j = 0; j < 4; j++) {
			val |= (uint32_t)ctx->v_weight_matrix[i][j] << (8 * j);
		}
		*cmd_ptr++ = val;
	}

	/* pre-scale hci, 1.0 on fixed point */
	val = 0x4000;
	*cmd_ptr++ = val;

	/* pre-scale parameter/width */
	val = (uint32_t)ctx->h_weight_matrix[0][1] << 24;
	val |= (uint32_t)ctx->h_weight_matrix[0][0] << 16;
	val |= ctx->input.width;
	*cmd_ptr++ = val;

	/* pre-scale parameters */
	for (i = 0; i < 4; i++) {
		val = (uint32_t)ctx->h_weight_matrix[i][2];
		val |= (uint32_t)ctx->h_weight_matrix[i][3] << 8;
		val |= (uint32_t)ctx->h_weight_matrix[i + 1][0] << 16;
		val |= (uint32_t)ctx->h_weight_matrix[i + 1][1] << 24;
		*cmd_ptr++ = val;
	}

	val = (uint32_t)ctx->h_weight_matrix[4][2];
	val |= (uint32_t)ctx->h_weight_matrix[4][3] << 8;
	*cmd_ptr++ = val;

	/* NULL CMD */
	*cmd_ptr++ = 0;

	if (out_yuv) {
		ITH_CMDQ_BURST_CMD(cmd_ptr, itvp_config.base + REG_VP_OUT_YUV_11,
				     (REG_VP_OUT_YUV_CONST_U - REG_VP_OUT_YUV_11 + 4) / 4);

		/* RGB to YUV color space conversion matrix */
		for (i = 0; i < 6; i++) {
			val = ctx->rgb2yuv_color_matrix[i * 2];
			val |= (uint32_t)ctx->rgb2yuv_color_matrix[i * 2 + 1] << 16;
			*cmd_ptr++ = val;
		}
		color_space_conv_en = true;
	}

	if (ctx->flags.flush_input_cache) {
	if (in_rgb) {
		ithFlushDCacheRange(ctx->input.data[0],
					   ctx->input.pitch * ctx->input.height);

	} else if (in_yuv) {
    	ithFlushDCacheRange(ctx->input.data[0],
   				   ctx->input.pitch * ctx->input.height);
    	ithFlushDCacheRange(ctx->input.data[1],
					   ctx->input.uv_pitch * ctx->input.height);
			ithFlushDCacheRange(ctx->input.data[2],
					   ctx->input.uv_pitch * ctx->input.height);
		}
	}

	/* fire engine */
	val = BIT(0);

	/* rotate direction */
	val |= ctx->output.rot << 26;

	/* enable color space conv */
	if (color_space_conv_en) {
		val |= BIT(17);
	}

	ITH_CMDQ_SINGLE_CMD(cmd_ptr, itvp_config.base + REG_VP_FIRE, val);

  ithCmdQFlush(cmd_ptr, ITH_CMDQ0_OFFSET);
  ithCmdQUnlock(ITH_CMDQ0_OFFSET);

	return 0;
}

#endif /* CONFIG_ITE_ITVP_CMDQ_SUPPORT */

int ite_itvp_process(ite_itvp_context_t *ctx)
{
	assert(ctx);
	int ret = 0;

#ifdef CONFIG_ITE_ITVP_CMDQ_SUPPORT
	if (ctx->flags.use_cmdq) {
		ret = ite_itvp_process_cmdq(ctx);
		itvp_data.last_use_cmdq = true;
	} else
#endif
	{
#ifdef CONFIG_ITE_ITVP_CMDQ_SUPPORT

		if (itvp_data.last_use_cmdq) {
			ite_itcmdq_wait_empty(itvp_config.cmdq_dev, false);
			itvp_data.last_use_cmdq = false;
		}
#endif
		ret = ite_itvp_process_default(ctx);
	}

	return ret;
}

int itvp_init(void)
{
	ithIspResetEngine(ITH_ISP_CORE0);
  ithIspResetAllReg();
  ithIspEnableClock();

	return 0;
}
