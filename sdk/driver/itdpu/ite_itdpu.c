/*
 * Copyright (c) 2022 ITE Tech. Inc.
 */

#include "ite_itdpu.h"

#include <assert.h>
#include <sched.h>
#include <string.h>
#include <sys/errno.h>

#include "ite/ith.h"

#define BIT(n)            (1UL << (n))
#define BIT_MASK(n)       (BIT(n) - 1UL)

#define LOG_ERR           ithPrintf

#if (CFG_CHIP_FAMILY == 9860)
    #define REG_DPU_BASE   0xD0900000
    #define BUFFER_SIZE    (512 * 1024)
#else
    #define REG_DPU_BASE   0xB0900000
#endif

#define REG_DPU_DCR      0x00
#define REG_DPU_SAR      0x04
#define REG_DPU_DAR      0x08
#define REG_DPU_SRC_SIZE 0x0C
#define REG_DPU_INTCT    0x10
#define REG_DPU_DSR      0x14
#define REG_DPU_CRC0     0x18
#define REG_DPU_CRC1     0x1C
#define REG_DPU_KEY0     0x2C
#define REG_DPU_IVR0     0x44

#define WAIT_BUSY_TIMEOUT     (500000U)

struct itdpu_config {
	uint32_t base;
};

#if (CFG_CHIP_FAMILY == 9860)
static uint8_t itdpu_buf[BUFFER_SIZE] __attribute__((aligned(32)));
#endif

static const struct itdpu_config itdpu_config = {
	.base = REG_DPU_BASE,
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

static inline void sys_clear_bit(uint32_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp & ~(1 << bit);
}

static void itdpu_reset(void)
{
	/* reset DCR except true random function enable */
	sys_set_mask(itdpu_config.base + REG_DPU_DCR, ~BIT(17), 0x0);
}

static int itdpu_wait_finish(void)
{
  uint32_t timeout = WAIT_BUSY_TIMEOUT;
	uint32_t status;

	for (;;) {
		status = sys_read32(itdpu_config.base + REG_DPU_DSR);
		if (status & BIT(0)) {
			break;
		}
		if (timeout == 0UL) {
      /* clear DPU fire bit */
      sys_clear_bit(itdpu_config.base + REG_DPU_DCR, 1);

			LOG_ERR("wait idle timeout");
			return -EINVAL;
		}
		timeout--;
    sched_yield();
	}

	/* clear DPU fire bit */
	sys_clear_bit(itdpu_config.base + REG_DPU_DCR, 1);

	return 0;
}

static int itdpu_do_crc32(const uint8_t *data, size_t len, bool first_pkt,
			  uint32_t *crc)
{
#if (CFG_CHIP_FAMILY == 9860)
	const uint32_t mask = (BIT_MASK(3) << 24) | BIT(19) | (BIT_MASK(2) << 16) | BIT(11) |
			      BIT(7) | (BIT_MASK(3) << 4) | BIT(3) | BIT(2) | BIT(1);
	const uint32_t val = (0x2 << 24) | BIT(7) | BIT(2) | BIT(1);
#else
	const uint32_t mask = (BIT_MASK(3) << 24) | BIT(19) | (BIT_MASK(2) << 16) | BIT(11) |
			      BIT(7) | (BIT_MASK(3) << 4) | BIT(1);
	const uint32_t val = (0x2 << 24) | BIT(7) | BIT(1);
#endif
	int ret;

	if (first_pkt) {
		/* init crc */
		sys_write32(0xFFFFFFFF, itdpu_config.base + REG_DPU_CRC0);
#if (CFG_CHIP_FAMILY == 9860)
		sys_write32(0xFFFFFFFF, itdpu_config.base + REG_DPU_CRC1);
#endif
	} else if (crc) {
		/* continue crc */
		uint32_t crc_val = ~(*crc);
		sys_write32(crc_val, itdpu_config.base + REG_DPU_CRC0);
#if (CFG_CHIP_FAMILY == 9860)
		sys_write32(crc_val, itdpu_config.base + REG_DPU_CRC1);
#endif
	}

	sys_write32((uint32_t)data, itdpu_config.base + REG_DPU_SAR);
	sys_write32(len, itdpu_config.base + REG_DPU_SRC_SIZE);

	/* do crc32 */
	sys_set_mask(itdpu_config.base + REG_DPU_DCR, mask, val);

	ret = itdpu_wait_finish();

	if (ret == 0 && crc) {
		*crc = sys_read32(itdpu_config.base + REG_DPU_CRC0) ^ 0xFFFFFFFF;
	}

	return ret;
}

int ite_itdpu_crc32(const uint8_t *data, size_t len, uint32_t *crc)
{
	bool size_aligned = ((uint32_t)len & 0x3) == 0;
	int ret = 0;

#if (CFG_CHIP_FAMILY == 9860)
	bool data_aligned = ((uint32_t)data & 0x3) == 0;
	if (data_aligned && size_aligned) {
		ithFlushDCacheRange((void *)data, len);
		ret = itdpu_do_crc32(data, len, *crc == 0, crc);
	} else if (size_aligned) {
		uint8_t *ptr = (uint8_t *)data;
		while (len > 0) {
			size_t size = ITH_MIN(len, BUFFER_SIZE);
			memcpy(itdpu_buf, ptr, size);
      ithFlushDCacheRange(itdpu_buf, size);
			ret = itdpu_do_crc32(itdpu_buf, size, *crc == 0, crc);
			if (ret < 0) {
				goto end;
			}
			ptr += size;
			len -= size;
		}
#else
	if (size_aligned) {
		ithFlushDCacheRange((void *)data, len);
		ret = itdpu_do_crc32(data, len, *crc == 0, crc);
#endif
	} else {
		LOG_ERR("Unsupported parameters");
		ret = -EINVAL;
		goto end;
	}

end:
	return ret;
}

int itdpu_init(void)
{
  ithDpuEnableClock();
	itdpu_reset();

	return 0;
}
