/*
 * Copyright (c) 2012-2017 Andes Technology Corporation
 * All rights reserved.
 *
 */

#include "nds_core_v5.h"
#include <cpu_cache.h>
#include <cpu_pma.h>
#include <cpu_pmp.h>
#include "ite/ith.h"
#if (CFG_WT_SIZE > 0)
#include "openrtos/FreeRTOS.h"
#include "openrtos/semphr.h"
#endif

extern void reset_vector(void);
extern void BootInit(void);

/* This must be a leaf function, no child function */
void __platform_init(void) __attribute__((naked));
void __platform_init(void)
{
    /* Do your platform low-level initial */
    CPU_IDCACHE_DISABLE();

    __asm("ret");
}

void c_startup(void)
{
#define MEMCPY(des, src, n) __builtin_memcpy((des), (src), (n))
#define MEMSET(s, c, n)     __builtin_memset((s), (c), (n))
    /* Data section initialization */
    extern char  __bss_start__, __bss_end__;
    unsigned int size;

    /* Clear bss section */
    size = &__bss_end__ - &__bss_start__;
    MEMSET(&__bss_start__, 0, size);
}

#if (CFG_WT_SIZE > 0)
static uint8_t wt_buf[CFG_WT_SIZE] __attribute__ ((aligned(CFG_WT_SIZE))) = {0};
#endif

void system_init(void)
{
    /*
     * Do your system reset handling here
     */
    /* Enable PLIC features */
    //if (read_csr(NDS_MMISC_CTL) & (1 << 1)) {
    /* External PLIC interrupt is vectored */
    //__nds__plic_set_feature(NDS_PLIC_FEATURE_PREEMPT | NDS_PLIC_FEATURE_VECTORED);
    //} else {
    /* External PLIC interrupt is NOT vectored */
    //__nds__plic_set_feature(NDS_PLIC_FEATURE_PREEMPT);
    //}

    /* Enable Misaligned access and non-blocking load */
    set_csr(NDS_MMISC_CTL, (1 << 6) | (1 << 8));

    if (cpu_support_pma())
    {
        #if (CFG_WT_SIZE > 0)
        static ITHVmem vmem;

        vmem.startAddr    = (uint32_t)wt_buf;
        vmem.totalSize    = CFG_WT_SIZE;
        vmem.mutex        = (void*)xSemaphoreCreateMutex();
        vmem.usedMcbCount = vmem.freeSize = 0;

        ithVmemInit(&vmem);

        cpu_pma_config_t config_params_0 = {
            cpu_pma_0,
            (void *)wt_buf,
            CFG_WT_SIZE,
            cpu_pma_etyp_napot,
            #if defined(CFG_USBHCC)
            cpu_pma_mtyp_noncacheable,
            #else
            cpu_pma_mtyp_memory_wt_ra,
            #endif
        };
        cpu_pma_napot_config(CPU_PMA, &config_params_0);
        #endif // #if defined(CFG_CPU_WB) && (CFG_WT_SIZE > 0)
        #ifndef CFG_CPU_WB
        // SRAM
        cpu_pma_config_t config_params_14 = {
            cpu_pma_14,
            (void *)ITH_AXI_SRAM0_BASE,
            ITH_AXI_SRAM0_SIZE,
            cpu_pma_etyp_napot,
            cpu_pma_mtyp_memory_wt_ra,
        };
        cpu_pma_napot_config(CPU_PMA, &config_params_14);
        // DRAM
        cpu_pma_config_t config_params_15 = {
            cpu_pma_15,
            (void *)0x0,
            CFG_RAM_SIZE,
            cpu_pma_etyp_napot,
            cpu_pma_mtyp_memory_wt_ra,
        };
        cpu_pma_napot_config(CPU_PMA, &config_params_15);
        #endif
    }

    #if 1 //def CFG_CACHE_ENABLE
    CPU_ICACHE_INVALIDATE();
    CPU_DCACHE_INVALIDATE();

    #if 1 //def CFG_CPU_WB
          /* Enable I/D cache */
    CPU_IDCACHE_ENABLE();
    #else
    CPU_ICACHE_ENABLE();
    #endif
    #endif

    BootInit();
}
