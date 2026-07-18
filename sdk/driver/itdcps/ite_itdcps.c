/*
 * Copyright (c) 2022 ITE Tech. Inc.
 */

#include "ite_itdcps.h"

#include <assert.h>
#include <sched.h>
#include <string.h>
#include <sys/errno.h>

#include "ite/ith.h"
#include "ite/itp.h"

#define BIT(n)            (1UL << (n))
#define BIT_MASK(n)       (BIT(n) - 1UL)

#define LOG_ERR           ithPrintf

#if (CFG_CHIP_FAMILY == 9860)
    #define REG_DCPS_BASE   0xD0900100
#else
    #define REG_DCPS_BASE   0xB0900100
#endif

#define REG_DCPS_DCR0     0x00
#define REG_DCPS_DCR1     0x04
#define REG_DCPS_SAR      0x08
#define REG_DCPS_DAR      0x0C
#define REG_DCPS_SRC_SIZE 0x10
#define REG_DCPS_DST_SIZE 0x14
#define REG_DCPS_INTCT    0x18
#define REG_DCPS_DSR      0x1C
#define REG_DCPS_DDCR     0x20

#define WAIT_BUSY_TIMEOUT      (500000U)

static const uint8_t itdcps_ucl_magic[8] = {0x00, 0xe9, 0x55, 0x43, 0x4c, 0xff, 0x01, 0x1a};
static const uint8_t itdcps_speedy_magic[3] = {'I', 'T', 'E'};

struct itdcps_config {
	uint32_t base;
};

struct itdcps_data {
	int32_t last_id;
#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT
	bool last_use_cmdq;
#endif
};

static const struct itdcps_config itdcps_config = {
	.base = REG_DCPS_BASE,
};

static struct itdcps_data itdcps_data;

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

static void itdcps_reset(void)
{
	sys_write32(0x0, itdcps_config.base + REG_DCPS_DCR1);
	itdcps_data.last_id = (int32_t)sys_read32(itdcps_config.base + REG_DCPS_DDCR);
}

static int itdcps_wait_finish(void)
{
  uint32_t timeout = WAIT_BUSY_TIMEOUT;
  uint32_t status;
    
	for (;;) {
		status = sys_read32(itdcps_config.base + REG_DCPS_DSR);
		if (status & BIT(0)) {
			break;
		}
		if (timeout == 0UL) {
			LOG_ERR("wait idle timeout");
			return -EINVAL;
		}
		timeout--;
    sched_yield();
	}

	if (status & BIT(2)) {
		LOG_ERR("decompress fail");
		return -EINVAL;
	}
  
	return 0;
}

static int itdcps_decompress_ucl(ite_itdcps_dsc_t *dsc)
{
	uint8_t *dst = dsc->dst;
	uint8_t *src = (uint8_t *)dsc->src;
	uint32_t block_size, out_size, in_size;

	if (memcmp(src, itdcps_ucl_magic, 8)) {
		LOG_ERR("UCL magic incorrect");
		return -EINVAL;
	}
	src += 12;

	if (*src != 0x2e) {
		LOG_ERR("unsupport UCL method: 0x%X", *src);
		return -ENOTSUP;
	}
	src += 2;

	memcpy(&block_size, src, 4);
	block_size = be32_to_cpu(block_size);
	if (block_size < 1024) {
		LOG_ERR("invalid block size: %u", block_size);
		return -EINVAL;
	}
	src += 4;

	out_size = in_size = 0;

	while ((out_size < dsc->dst_size) && (in_size < dsc->src_size)) {
		uint32_t out_len, in_len;

		memcpy(&out_len, src, 4);
		out_len = be32_to_cpu(out_len);
		if (out_len == 0) {
			break;
		}
		src += 4;
		memcpy(&in_len, src, 4);
		in_len = be32_to_cpu(in_len);
		if ((in_len > block_size) || (out_len > block_size) || (in_len == 0) ||
		    (in_len > out_len)) {
			LOG_ERR("invalid block size");
			return -EINVAL;
		}
		src += 4;

		sys_write32((uint32_t)src, itdcps_config.base + REG_DCPS_SAR);
		sys_write32((uint32_t)dst, itdcps_config.base + REG_DCPS_DAR);

		/* do decompress */
		sys_write32(in_len, itdcps_config.base + REG_DCPS_SRC_SIZE);
		sys_write32(out_len, itdcps_config.base + REG_DCPS_DST_SIZE);

		/* burst size 8, ucl decompress mode, DCPS fire */
		sys_set_mask(itdcps_config.base + REG_DCPS_DCR0, BIT_MASK(7),
			     (0x2 << 4) | (0x0 << 2) | BIT(1));

		dsc->id = ++itdcps_data.last_id;

		int ret = itdcps_wait_finish();
		if (ret < 0) {
			return ret;
		}

		dst += out_len;
		src += in_len;
		out_size += out_len;
		in_size += 8 + in_len;
	}

	return 0;
}

static int itdcps_decompress_speedy(ite_itdcps_dsc_t *dsc)
{
	uint8_t *src = (uint8_t *)dsc->src;
	uint32_t out_size, in_size;

	if (memcmp(src, itdcps_speedy_magic, 3)) {
		LOG_ERR("speedy magic incorrect");
		return -EINVAL;
	}
	src += 6;

	memcpy(&out_size, src, 4);
	out_size = le32_to_cpu(out_size);
	if (out_size > dsc->dst_size) {
		LOG_ERR("invalid dst size: %u > %u", out_size, dsc->dst_size);
		return -EINVAL;
	}
	src += 4;

	memcpy(&in_size, src, 4);
	in_size = le32_to_cpu(in_size);
	in_size += 14;
	if (in_size > dsc->src_size) {
		LOG_ERR("invalid src size: %u > %u", in_size, dsc->src_size);
		return -EINVAL;
	}

	/* do decompress */
	sys_write32((uint32_t)dsc->src, itdcps_config.base + REG_DCPS_SAR);
	sys_write32((uint32_t)dsc->dst, itdcps_config.base + REG_DCPS_DAR);
	sys_write32(in_size, itdcps_config.base + REG_DCPS_SRC_SIZE);
	sys_write32(out_size, itdcps_config.base + REG_DCPS_DST_SIZE);

	/* burst size 8, speedy decompress mode, DCPS fire */
	sys_set_mask(itdcps_config.base + REG_DCPS_DCR0, BIT_MASK(7),
		     (0x2 << 4) | (0x2 << 2) | BIT(1));

	dsc->id = ++itdcps_data.last_id;

	int ret = itdcps_wait_finish();
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int itdcps_compress_speedy(ite_itdcps_dsc_t *dsc)
{
	uint8_t *dst = (uint8_t *)dsc->dst;
	uint32_t out_size, in_size;

	out_size = cpu_to_le32(dsc->dst_size);
	in_size = cpu_to_le32(dsc->src_size);

	/* do compress */
	sys_write32((uint32_t)dsc->src, itdcps_config.base + REG_DCPS_SAR);
	sys_write32((uint32_t)dst, itdcps_config.base + REG_DCPS_DAR);
	sys_write32(in_size, itdcps_config.base + REG_DCPS_SRC_SIZE);
	sys_write32(out_size, itdcps_config.base + REG_DCPS_DST_SIZE);

	/* 4 byte mode, burst size 8, speedy compress mode, DCPS fire */
	sys_set_mask(itdcps_config.base + REG_DCPS_DCR0, BIT_MASK(10),
		     (0x4 << 8) | (0x2 << 4) | (0x3 << 2) | BIT(1));

	dsc->id = ++itdcps_data.last_id;

	int ret = itdcps_wait_finish();
	if (ret < 0) {
		return ret;
	}

	dst += 10;
	memcpy(&dsc->out_size, dst, 4);
	dsc->out_size = le32_to_cpu(dsc->out_size) + 14;
	if (dsc->out_size > dsc->dst_size) {
		LOG_ERR("invalid dst size: %u > %u", dsc->out_size, dsc->dst_size);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT

static uint32_t itdcps_calc_cmd_size(int cmd_count)
{
	uint32_t cmd_size = 0;

	for (int i = 0; i < cmd_count; i++) {
		/* SAR, DAR, SRC_SIZE, DST_SIZE */
		cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);

		/* DCR0 */
		cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;
	}

	return cmd_size;
}

static int itdcps_decompress_ucl_cmdq(ite_itdcps_dsc_t *dsc)
{
	uint8_t *dst = dsc->dst;
	uint8_t *src = (uint8_t *)dsc->src;
	uint32_t block_size, out_size, in_size, cmd_size, val;
	int cmd_count;

	if (memcmp(src, itdcps_ucl_magic, 8)) {
		LOG_ERR("UCL magic incorrect");
		return -EINVAL;
	}
	src += 12;

	if (*src != 0x2e) {
		LOG_ERR("unsupport UCL method: 0x%X", *src);
		return -ENOTSUP;
	}
	src += 2;

	memcpy(&block_size, src, 4);
	block_size = be32_to_cpu(block_size);
	if (block_size < 1024) {
		LOG_ERR("invalid block size: %u", block_size);
		return -EINVAL;
	}
	src += 4;

	out_size = in_size = cmd_count = 0;

	while ((out_size < dsc->dst_size) && (in_size < dsc->src_size)) {
		uint32_t out_len, in_len;

		memcpy(&out_len, src, 4);
		out_len = be32_to_cpu(out_len);
		if (out_len == 0) {
			break;
		}
		out_size += out_len;
		src += 4;

		memcpy(&in_len, src, 4);
		in_len = be32_to_cpu(in_len);
		if ((in_len > block_size) || (out_len > block_size) || (in_len == 0) ||
		    (in_len > out_len)) {
			LOG_ERR("invalid block size");
			return -EINVAL;
		}
		in_size += 8 + in_len;
		src += 4 + in_len;
		cmd_count++;
	}
	src = (uint8_t *)dsc->src + 18;

	val = sys_read32(itdcps_config.base + REG_DCPS_DCR0) & ~BIT_MASK(7);
	/* burst size 8, ucl decompress mode, DCPS fire */
	val |= (0x2 << 4) | (0x0 << 2) | BIT(1);

	cmd_size = itdcps_calc_cmd_size(cmd_count);
  ithCmdQLock(ITH_CMDQ0_OFFSET);
  uint32_t *ptr = ithCmdQWaitSize(cmd_size, ITH_CMDQ0_OFFSET);
	if (!ptr) {
		return -EBUSY;
	}

	for (int i = 0; i < cmd_count; i++) {
		uint32_t out_len, in_len;

		memcpy(&out_len, src, 4);
		out_len = be32_to_cpu(out_len);
		assert(out_len > 0);
		src += 4;

		memcpy(&in_len, src, 4);
		in_len = be32_to_cpu(in_len);
		src += 4;

		/* do decompress */
		ITH_CMDQ_BURST_CMD(ptr, itdcps_config.base + REG_DCPS_SAR, 4);
		/* SAR */
		*ptr++ = (uint32_t)src;
		/* DAR */
		*ptr++ = (uint32_t)dst;
		/* SRC_SIZE */
		*ptr++ = (uint32_t)in_len;
		/* DST_SIZE */
		*ptr++ = (uint32_t)out_len;

		/* DCR0 */
		ITH_CMDQ_SINGLE_CMD(ptr, itdcps_config.base + REG_DCPS_DCR0, val);

		dsc->id = ++itdcps_data.last_id;

		src += in_len;
		dst += out_len;
	}

  ithCmdQFlush(ptr, ITH_CMDQ0_OFFSET);
  ithCmdQUnlock(ITH_CMDQ0_OFFSET);

	return 0;
}

static int itdcps_decompress_speedy_cmdq(ite_itdcps_dsc_t *dsc)
{
	uint8_t *src = (uint8_t *)dsc->src;
	uint32_t out_size, in_size, cmd_size, val;

	if (memcmp(src, itdcps_speedy_magic, 3)) {
		LOG_ERR("speedy magic incorrect");
		return -EINVAL;
	}
	src += 6;

	memcpy(&out_size, src, 4);
	out_size = le32_to_cpu(out_size);
	if (out_size > dsc->dst_size) {
		LOG_ERR("invalid dst size: %u > %u", out_size, dsc->dst_size);
		return -EINVAL;
	}
	src += 4;

	memcpy(&in_size, src, 4);
	in_size = le32_to_cpu(in_size);
	in_size += 14;
	if (in_size > dsc->src_size) {
		LOG_ERR("invalid src size: %u > %u", in_size, dsc->src_size);
		return -EINVAL;
	}

	val = sys_read32(itdcps_config.base + REG_DCPS_DCR0) & ~BIT_MASK(7);
	/* burst size 8, speedy decompress mode, DCPS fire */
	val |= (0x2 << 4) | (0x2 << 2) | BIT(1);

	cmd_size = itdcps_calc_cmd_size(1);
  ithCmdQLock(ITH_CMDQ0_OFFSET);
  uint32_t *ptr = ithCmdQWaitSize(cmd_size, ITH_CMDQ0_OFFSET);
	if (!ptr) {
		return -EBUSY;
	}

	/* do decompress */
	ITH_CMDQ_BURST_CMD(ptr, itdcps_config.base + REG_DCPS_SAR, 4);
	/* SAR */
	*ptr++ = (uint32_t)dsc->src;
	/* DAR */
	*ptr++ = (uint32_t)dsc->dst;
	/* SRC_SIZE */
	*ptr++ = (uint32_t)in_size;
	/* DST_SIZE */
	*ptr++ = (uint32_t)out_size;

	/* DCR0 */
	ITH_CMDQ_SINGLE_CMD(ptr, itdcps_config.base + REG_DCPS_DCR0, val);

  ithCmdQFlush(ptr, ITH_CMDQ0_OFFSET);
  ithCmdQUnlock(ITH_CMDQ0_OFFSET);
  
	dsc->id = ++itdcps_data.last_id;

	return 0;
}

#endif /* CONFIG_ITE_ITDCPS_CMDQ_SUPPORT */

int ite_itdcps_decompress(ite_itdcps_dsc_t *dsc)
{
	assert(dsc);
	assert(dsc->dst_size > 0);
	assert(dsc->dst);
	assert(dsc->src_size > 0);
	assert(dsc->src);
	int ret = 0;

	ithFlushDCacheRange((void *)dsc->src, dsc->src_size);
	ithInvalidateDCacheRange((void *)dsc->dst, dsc->dst_size);

	if (dsc->flags.mode == ITE_ITDCPS_MODE_UCL_DEC) {
#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT
		if (dsc->flags.use_cmdq) {
			ret = itdcps_decompress_ucl_cmdq(dsc);
			itdcps_data.last_use_cmdq = true;
		} else
#endif
		{
#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT
			if (itdcps_data.last_use_cmdq) {
				ite_itdcps_wait_finish(itdcps_data.last_id);
				itdcps_data.last_use_cmdq = false;
			}
#endif
			ret = itdcps_decompress_ucl(dsc);
		}
	} else if (dsc->flags.mode == ITE_ITDCPS_MODE_SPEEDY_DEC) {
#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT
		if (dsc->flags.use_cmdq) {
			ret = itdcps_decompress_speedy_cmdq(dsc);
			itdcps_data.last_use_cmdq = true;
		} else
#endif
		{
#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT
			if (itdcps_data.last_use_cmdq) {
				ite_itdcps_wait_finish(itdcps_data.last_id);
				itdcps_data.last_use_cmdq = false;
			}
#endif
			ret = itdcps_decompress_speedy(dsc);
		}
	} else {
		LOG_ERR("unspport mode: %u", dsc->flags.mode);
		return -EINVAL;
	}

	return ret;
}

int ite_itdcps_compress(ite_itdcps_dsc_t *dsc)
{
	assert(dsc);
	assert(dsc->dst_size > 0);
	assert(dsc->dst);
	assert(dsc->src_size > 0);
	assert(dsc->src);
	int ret = 0;

	ithFlushDCacheRange((void *)dsc->src, dsc->src_size);
	ithInvalidateDCacheRange((void *)dsc->dst, dsc->dst_size);

	if (dsc->flags.mode == ITE_ITDCPS_MODE_SPEEDY_COMP) {
		ret = itdcps_compress_speedy(dsc);
	} else {
		LOG_ERR("unspport mode: %u", dsc->flags.mode);
		return -EINVAL;
	}

	return ret;
}

int ite_itdcps_stream_decompress(ite_itdcps_context_t *ctx)
{
	assert(ctx);
	assert(ctx->dcps_callback);
	assert(ctx->dsc.flags.mode == ITE_ITDCPS_MODE_UCL_DEC);
	ite_itdcps_dsc_t *dsc = &ctx->dsc;
	uint8_t *dst, *src;
	uint32_t block_size;
	int ret;

	ctx->status = ITE_ITDCPS_START;
	ctx->dsc.dst_size = 0;
	ctx->dsc.src_size = 26;
	ret = ctx->dcps_callback(ctx);
	if (ret < 0) {
		LOG_ERR("dcps callback fail: %d\n", ret);
		return ret;
	}

	src = (uint8_t *)dsc->src;

	if (memcmp(src, itdcps_ucl_magic, 8)) {
		LOG_ERR("UCL magic incorrect");
		return -EINVAL;
	}
	src += 12;

	if (*src != 0x2e) {
		LOG_ERR("unsupport UCL method: 0x%X", *src);
		return -ENOTSUP;
	}
	src += 2;

	memcpy(&block_size, src, 4);
	block_size = be32_to_cpu(block_size);
	if (block_size < 1024) {
		LOG_ERR("invalid block size: %u", block_size);
		return -EINVAL;
	}
	src += 4;

	while (src) {
		uint32_t out_len, in_len;

		memcpy(&out_len, src, 4);
		out_len = be32_to_cpu(out_len);
		if (out_len == 0) {
			break;
		}
		src += 4;
		memcpy(&in_len, src, 4);
		in_len = be32_to_cpu(in_len);
		if ((in_len > block_size) || (out_len > block_size) || (in_len == 0) ||
		    (in_len > out_len)) {
			LOG_ERR("invalid block size");
			return -EINVAL;
		}

		ctx->status = ITE_ITDCPS_BLOCK_START;
		dsc->dst_size = out_len;
		dsc->src_size = in_len;
		ret = ctx->dcps_callback(ctx);
		if (ret < 0) {
			LOG_ERR("dcps callback fail: %d\n", ret);
			return ret;
		}

		dst = dsc->dst;
		src = (uint8_t *)dsc->src;

		sys_write32((uint32_t)src, itdcps_config.base + REG_DCPS_SAR);
		sys_write32((uint32_t)dst, itdcps_config.base + REG_DCPS_DAR);

		/* do decompress */
		sys_write32(in_len, itdcps_config.base + REG_DCPS_SRC_SIZE);
		sys_write32(out_len, itdcps_config.base + REG_DCPS_DST_SIZE);

		/* enable interrupt */
		sys_set_bit(itdcps_config.base + REG_DCPS_INTCT, 0);

		/* burst size 8, ucl decompress mode, DCPS fire */
		sys_set_mask(itdcps_config.base + REG_DCPS_DCR0, BIT_MASK(7),
			     (0x2 << 4) | (0x0 << 2) | BIT(1));

		dsc->id = ++itdcps_data.last_id;

		ret = itdcps_wait_finish();
		if (ret < 0) {
			return ret;
		}

		ctx->status = ITE_ITDCPS_BLOCK_END;
		dsc->dst_size = 0;
		ctx->dsc.src_size = 8;
		ret = ctx->dcps_callback(ctx);
		if (ret < 0) {
			LOG_ERR("dcps callback fail: %d\n", ret);
			return ret;
		}

		src = (uint8_t *)dsc->src;
	}

	return 0;
}

int ite_itdcps_wait_idle(void)
{
	uint32_t timeout = WAIT_BUSY_TIMEOUT;

#ifdef CONFIG_ITE_ITDCPS_CMDQ_SUPPORT
	int ret = ite_itcmdq_wait_empty(itdcps_config.cmdq_dev, false);
	if (ret < 0) {
		return ret;
	}
#endif

	for (;;) {
		uint32_t val = sys_read32(itdcps_config.base + REG_DCPS_DSR);
		if ((val & BIT_MASK(2)) == 0x1) {
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

int ite_itdcps_wait_finish(int32_t id)
{
	int32_t curr_id = (int32_t)sys_read32(itdcps_config.base + REG_DCPS_DDCR);
	int ret = 0;

	if ((curr_id >= 0) && (id < 0)) {
		ret = ite_itdcps_wait_idle();
	} else if (curr_id < id) {
		uint32_t timeout = WAIT_BUSY_TIMEOUT;
		for (;;) {
			curr_id = (int32_t)sys_read32(itdcps_config.base + REG_DCPS_DDCR);
			if (curr_id >= id) {
				break;
			}
			if (timeout == 0UL) {
				LOG_ERR("wait ID timeout: %d < %d", curr_id, id);
				ret = -EBUSY;
				break;
			}
			timeout--;
			sched_yield();
		}
	}

	return ret;
}

int itdcps_init(void)
{
  ithDcpsEnableClock();
  ithDcpsResetEngine();
	itdcps_reset();

	return 0;
}
