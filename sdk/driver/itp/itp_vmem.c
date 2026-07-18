/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Video Memory Management functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <malloc.h>
#include "itp_cfg.h"

#ifdef _WIN32

uint32_t itpVmemAlloc(size_t size)
{
    return ithVmemAlloc(ITH_ALIGN_UP(size, 32));
}

uint32_t itpVmemAlignedAlloc(size_t alignment, size_t size)
{
    return ithVmemAlignedAlloc(ITH_ALIGN_UP(alignment, 32), ITH_ALIGN_UP(size, 32));
}

void itpVmemFree(uint32_t addr)
{
    ithVmemFree(addr);
}

void itpVmemStats(void)
{
    ithVmemStats();
}

uint32_t itpWTAlloc(size_t size)
{
    return ithVmemAlloc(size);
}

void itpWTFree(uint32_t addr)
{
    ithVmemFree(addr);
}

void itpWTStats(void)
{
    ithVmemStats();
}

#else

uint32_t itpVmemAlloc(size_t size)
{
    return (uint32_t) memalign(64, ITH_ALIGN_UP(size, 32));
}

uint32_t itpVmemAlignedAlloc(size_t alignment, size_t size)
{
    return (uint32_t) memalign(ITH_ALIGN_UP(alignment, 32), ITH_ALIGN_UP(size, 32));
}

void itpVmemFree(uint32_t addr)
{
    free((void*) addr);
}

void itpVmemStats(void)
{
    malloc_stats();
}

extern uint32_t __wt_base;
#define ITP_WT_BASE (uint32_t)&__wt_base

uint32_t itpWTAlloc(size_t size)
{
#if (CFG_WT_SIZE > 0)
    #if (CFG_CHIP_FAMILY == 9860)
    uint32_t addr = ithVmemAlloc(size);
    if (addr)
    {
        return ITP_WT_BASE + addr;
    }
    else
    {
        return 0;
    }
    #else
    return ithVmemAlloc(size);
    #endif
#else
    return (uint32_t) memalign(64, size);
#endif
}

void itpWTFree(uint32_t addr)
{
#if (CFG_WT_SIZE > 0)
    #if (CFG_CHIP_FAMILY == 9860)
    ithVmemFree(addr - ITP_WT_BASE);
    #else
    ithVmemFree(addr);
    #endif
#else
    free((void*) addr);
#endif
}

void itpWTStats(void)
{
#if (CFG_WT_SIZE > 0)
    ithVmemStats();
#else
    malloc_stats();
#endif
}

#ifdef CFG_M2D_RESERVE_MEM_ENABLE
extern uint32_t __m2d_base;
#define ITP_M2D_BASE (uint32_t)&__m2d_base

uint32_t itpWTM2DAlloc(size_t size)
{
	uint32_t addr = ithVmemM2DAlloc(size);
	if(!addr)
		return 0;
	else
    	return ITP_M2D_BASE + addr;
}

void itpWTM2DFree(uint32_t addr)
{
    ithVmemM2DFree(addr - ITP_M2D_BASE);
}

void itpWTM2DStats(void)
{
    ithVmemM2DStats();
}
#endif
#endif // _WIN32
