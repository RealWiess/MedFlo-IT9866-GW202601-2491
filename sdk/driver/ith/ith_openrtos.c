/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL FreeRTOS functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <sys/param.h>
#include "ith_cfg.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/semphr.h"
#include "openrtos/task.h"
#ifdef __riscv
    #include <cpu_cache.h>
    #include <cpu_power.h>
    #include <cpu_mtimer.h>
#endif

#ifdef __SM32__
    #include "openrtos/sm32/port_spr_defs.h"
#endif

#define CACHE_LINESZ   32

extern uint32_t __bootimage_func_start;
extern int      _text_end;
extern uint32_t __cmdq_base;
extern uint32_t __cmdq_base_1;

#ifdef CFG_CMDQ_ENABLE
static ITHCmdQ  cmdQ;
static ITHCmdQ  cmdQ_1;
#endif

void            *ithStorMutex = NULL;

extern uint32_t CacheID926();

static void GpioIntrHandler(void *arg)
{
    ithGpioDoIntr();
}

void ithInit(void)
{
    static void *dmaMutex;

#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)
    const ITHCardConfig cardCfg =
    {
        {
            // card detect pin of sd0
#if defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC) && !defined(CFG_SDIO0_STATIC)
            CFG_GPIO_SD0_CARD_DETECT,
#elif defined(CFG_SD0_STATIC) || defined(CFG_SDIO0_STATIC)
            (uint8_t)-2,     // always insert
#else
            (uint8_t)-1,
#endif

            // card detect pin of sd1
#if defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC) && !defined(CFG_SDIO1_STATIC)
            CFG_GPIO_SD1_CARD_DETECT,
#elif defined(CFG_SD1_STATIC) || defined(CFG_SDIO1_STATIC)
            (uint8_t)-2,     // always insert
#else
            (uint8_t)-1,
#endif
        },

        {
            // power enable pin of sd0
#if defined(CFG_SD0_ENABLE)
            CFG_GPIO_SD0_POWER_ENABLE,
#else
            (uint8_t)-1,
#endif

            // power enable pin of sd1
#if defined(CFG_SD1_ENABLE)
            CFG_GPIO_SD1_POWER_ENABLE,
#else
            (uint8_t)-1,
#endif
        },

        {
            // write protect pin of sd0
#if defined(CFG_SD0_ENABLE)
            CFG_GPIO_SD0_WRITE_PROTECT,
#else
            (uint8_t)-1,
#endif

            // write protect pin of sd1
#if defined(CFG_SD1_ENABLE)
            CFG_GPIO_SD1_WRITE_PROTECT,
#else
            (uint8_t)-1,
#endif
        },

        {
#if defined(CFG_GPIO_SD0_IO)
            CFG_GPIO_SD0_IO,
#else
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
#endif
        },

        {
#if defined(CFG_GPIO_SD1_IO)
            CFG_GPIO_SD1_IO,
#else
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
#endif
        },
    };
#endif // #if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)

    // init memory debug
#ifdef CFG_MEMDBG_ENABLE
    #if 0
    ithMemDbgSetPassId0(ITH_MEMDBG0, ITH_MEMDBG_PASSID_AXI0_NONE, ITH_MEMDBG_PASSID_AXI1_NONE, ITH_MEMDGB_RISC, ITH_MEMDBG_PASSID_AXI3_NONE); /* as reference code */
    #endif
    ithMemDbgSetMode(ITH_MEMDBG0, ITH_MEMDGB_WRITEONLY);
    ithMemDbgSetRange(ITH_MEMDBG0, 0x0, 0x1000 - 64 - 4);   // watch writing null pointer error
    ithMemDbgEnable(ITH_MEMDBG0);

    ithMemDbgSetMode(ITH_MEMDBG1, ITH_MEMDGB_WRITEONLY);
    ithMemDbgSetRange(ITH_MEMDBG1, (uint32_t)&__bootimage_func_start, (uint32_t)&_text_end);   // watch writing .text section error
    ithMemDbgEnable(ITH_MEMDBG1);

    ithMemDbgEnableIntr();
#endif // CFG_MEMDBG_ENABLE

#if (CFG_CHIP_FAMILY == 9860)
    #if !defined(CFG_JPEG_HW_ENABLE) && !defined(CFG_VIDEO_ENABLE)
    // disable isp clock for power saving
    ithIspDisableClock();
    #endif
#endif

#if (CFG_CHIP_FAMILY == 9860)
    #if !defined(CFG_USB0_ENABLE) && !defined(CFG_USBHCC)
    ithUsbPhyDisableClock(ITH_USB0);
    #endif
#endif

    // init mpu
    //ithGetRevisionId();
    //ithGetPackageId(); // for power saving
    ithIntrReset();
    ithIntrInit();

    // recover AHB0 control register value
    ithAhb0SetCtrlReg();

#ifndef __riscv
    // init timer for delay counting
    ithTimerReset(portTIMER);
    ithTimerCtrlEnable(portTIMER, ITH_TIMER_UPCOUNT);
    ithTimerCtrlEnable(portTIMER, ITH_TIMER_PERIODIC);
    ithTimerSetCounter(portTIMER, 0);
    ithTimerSetMatch(portTIMER, configCPU_CLOCK_HZ / configTICK_RATE_HZ);
    ithTimerEnable(portTIMER);
#endif

    // init gpio
    ithGpioInit();
    ithGpioSetDebounceClock(800);
    ithIntrDisableIrq(ITH_INTR_GPIO);
    ithIntrClearIrq(ITH_INTR_GPIO);
    ithIntrRegisterHandlerIrq(ITH_INTR_GPIO, GpioIntrHandler, NULL);
    ithGpioEnableClock();

    // init dma controller
    dmaMutex = xSemaphoreCreateMutex();
    ithDmaInit(dmaMutex);

    // init card
#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)
    ithCardInit(&cardCfg);
    // storage need always power on for pin share issue
    ithCardPowerOn(ITH_CARDPIN_SD0);
    ithCardPowerOn(ITH_CARDPIN_SD1);
#endif // defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)

    // create mutex for storage pin share
    ithStorMutex = xSemaphoreCreateMutex();

    // init command queue
#ifdef CFG_CMDQ_ENABLE
    #if (CFG_CHIP_FAMILY == 970)
    cmdQ.size  = CFG_CMDQ_SIZE;
    cmdQ.addr  = (uint32_t)&__cmdq_base;
    cmdQ.mutex = xSemaphoreCreateMutex();

    ithCmdQInit(&cmdQ, ITH_CMDQ0_OFFSET);
    ithCmdQReset(ITH_CMDQ0_OFFSET);
        #if BYTE_ORDER == BIG_ENDIAN
    ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, ITH_CMDQ0_OFFSET);
        #endif

    cmdQ_1.size  = CFG_CMDQ_SIZE;
    cmdQ_1.addr  = (uint32_t)&__cmdq_base_1;
    cmdQ_1.mutex = xSemaphoreCreateMutex();

    ithCmdQInit(&cmdQ_1, ITH_CMDQ1_OFFSET);
    ithCmdQReset(ITH_CMDQ1_OFFSET);
        #if BYTE_ORDER == BIG_ENDIAN
    ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, ITH_CMDQ1_OFFSET);
        #endif

    ithWriteRegA(0xb0600010, 0x0FFF0173);
    ithWriteRegA(0xb0608010, 0x0FFF0174);
    #else // 9860 and 9830 only one cmdQ
    cmdQ.size  = CFG_CMDQ_SIZE;
    cmdQ.addr  = (uint32_t)&__cmdq_base;
    cmdQ.mutex = xSemaphoreCreateMutex();

    ithCmdQInit(&cmdQ, ITH_CMDQ0_OFFSET);
    ithCmdQReset(ITH_CMDQ0_OFFSET);
        #if BYTE_ORDER == BIG_ENDIAN
    ithCmdQCtrlEnable(ITH_CMDQ_BIGENDIAN, ITH_CMDQ0_OFFSET);
        #endif
    #endif //(CFG_CHIP_FAMILY == 970)
#endif // CFG_CMDQ_ENABLE

    // init stc
#if ((CFG_CHIP_FAMILY == 9070) || (CFG_CHIP_FAMILY == 9910))
    #ifdef CFG_VIDEO_ENABLE
    ithFpcReset();
    #endif
#endif
}

void ithAssertFail(const char *exp, const char *file, int line, const char *func)
{
    (void)ithPrintf("\r\n*** ASSERTION FAILED in %s:%d/%s():\r\n%s\r\n", file, line, func, exp);

    for (;;)
    {
        taskYIELD();
    }
}

void ithDelay(unsigned long us)
{
#if 0
    TickType_t t = (TickType_t)((uint64_t) us / (1000 * portTICK_PERIOD_MS));

    if (t > 0)
    {
        vTaskDelay(t);
        us = us % (1000 * portTICK_PERIOD_MS);
    }
#endif

#ifndef __riscv
    if (us > 0U)
    {
        long          tmo = (uint64_t)ithGetBusClock() * us / 1000000;
        unsigned long now, last = ithTimerGetCounter(portTIMER);

        while (tmo > 0)
        {
            now = ithTimerGetCounter(portTIMER);
            if (now > last)
            {
                tmo -= now - last;
            }
            else
            {
                tmo -= configCPU_CLOCK_HZ / configTICK_RATE_HZ - last + now; /* count up timer overflow */
            }

            last = now;
        }
    }
#else
    if (us > 0U)
    {
        uint64_t tmo = cpu_get_mtime() + ((uint64_t)configCPU_CLOCK_HZ * us / 1000000U);

        while (cpu_get_mtime() < tmo) { }
    }
#endif
}

void ithLockMutex(void *mutex)
{
    xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY);
}

void ithUnlockMutex(void *mutex)
{
    xSemaphoreGive((SemaphoreHandle_t)mutex);
}

void ithFlushDCache(void)
{
#ifdef CFG_CPU_WB
    #ifdef __arm__
    ithEnterCritical();

    __asm__ __volatile__ (
        "mov r3,#0\n"
        "mcr p15,0,r3,c7,c10,0\n"   /* flush d-cache all */
        "mcr p15,0,r3,c7,c10,4\n"   /* flush d-cache write buffer */
        :
        :
        : "r3"  /* clobber list */
    );

    ithExitCritical();

    #elif defined(__riscv)
    CPU_DCACHE_FLUSH();
    #endif //   __arm__
#endif // CFG_CPU_WB
}

__attribute__((used)) void ithInvalidateDCache(void)
{
#ifdef __arm__
    ithEnterCritical();

    __asm__ __volatile__ (
        "mov r3,#0\n"
        "mcr p15,0,r3,c7,c6,0\n"    /* invalidate d-cache all */
        :
        :
        : "r3"  /* clobber list */
    );

    ithExitCritical();

#elif defined(__SM32__)
    int ncs, bs;
    int cache_size, cache_line_size;
    int i = 0;

    // Number of cache sets
    ncs             = ((mfspr(SPR_DCCFGR) >> 3) & 0xf);

    // Number of cache block size
    bs              = ((mfspr(SPR_DCCFGR) >> 7) & 0x1) + 4;

    // Calc Cache size
    cache_line_size = 1 << bs;
    cache_size      = 1 << (ncs + bs);

    ithEnterCritical();

    // Disable DC
    mtspr(SPR_SR, mfspr(SPR_SR) & ~SPR_SR_DCE);

    // Flush DC
    do
    {
        mtspr(SPR_DCBIR, i);
        i += cache_line_size;
    } while (i < cache_size);

    // Enable DC
    mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_DCE);
    asm volatile ("l.nop 0x0\nl.nop 0x0\nl.nop 0x0\nl.nop 0x0\n");

    ithExitCritical();

#elif defined(__riscv)
    CPU_DCACHE_INVALIDATE();
#endif //   __arm__
}

__attribute__((used)) void ithFlushDCacheRange(void *addr, uint32_t size)
{
#ifdef CFG_CPU_WB
    #ifdef __arm__
    {
        unsigned int  linesz = CACHE_LINESZ;
        unsigned long start  = (uint32_t)addr;

        // aligned to cache line
        size  += (0U != (start % linesz)) ? linesz : 0U;
        start &= ~(linesz - 1U);

        ithEnterCritical();

        // do it!
        __asm__ __volatile__ (
            "mov r0,%0\n"
            "mov r1,%1\n"
            "add r1,r0,r1\n"
            "1:\n"
            "mcr p15,0,r0,c7,c10,1\n"
            "add r0,r0,%2\n"
            "cmp r0,r1\n"
            "blo 1b\n"
            "mov r0,#0\n"
            "mcr p15,0,r0,c7,c10,4\n"
            :
            : "r" (start), "r" (size), "r" (linesz) /* input */
            : "r0", "r1", "cc"                      /* clobber list */
        );

        ithExitCritical();
    }

    #elif defined(__riscv)
    CPU_DCACHE_FLUSH_RANGE(addr, size);
    #endif //   __arm__
#endif // CFG_CPU_WB
}

__attribute__((used)) void ithInvalidateDCacheRange(void *addr, uint32_t size)
{
#ifdef __arm__
    unsigned int  linesz = CACHE_LINESZ;
    unsigned long start  = (uint32_t)addr;

    // aligned to cache line
    size  += (0U != (start % linesz)) ? linesz : 0U;
    start &= ~(linesz - 1U);

    ithEnterCritical();

    // do it!
    __asm__ __volatile__ (
        "mov r0,%0\n"
        "mov r1,%1\n"
        "add r1,r0,r1\n"
        "1:\n"
        "mcr p15,0,r0,c7,c6,1\n"
        "add r0,r0,%2\n"
        "cmp r0,r1\n"
        "blo 1b\n"
        :
        : "r" (start), "r" (size), "r" (linesz) /* input */
        : "r0", "r1", "cc"                      /* clobber list */
    );

    ithExitCritical();

#elif defined(__SM32__)

    unsigned int line_size;
    unsigned int cache_size;
    unsigned int start;
    unsigned int end;
    int          ncs, bs;

    // Number of cache sets
    ncs        = ((mfspr(SPR_DCCFGR) >> 3) & 0xf);

    // Number of cache block size
    bs         = ((mfspr(SPR_DCCFGR) >> 7) & 0x1) + 4;

    cache_size = (1 << (ncs + bs));
    line_size  = (1 << bs);

    start      = ((unsigned int)addr) & ~(line_size - 1);
    end        = ((unsigned int)addr) + size - 1;
    end        = ((unsigned int)end) & ~(line_size - 1);

    if (end > start + cache_size - line_size)
        end = start + cache_size - line_size;

    ithEnterCritical();

    do
    {
        mtspr(SPR_DCBIR, start);
        start += line_size;
    } while (start <= end);

    ithExitCritical();

#elif defined(__riscv)
    CPU_DCACHE_INVALIDATE_RANGE(addr, size);
#endif //   __arm__
}

void ithInvalidateICache(void)
{
#ifdef __arm__
    ithEnterCritical();

    __asm__ __volatile__ (
        "mcr p15,0,%0,c7,c14,0\n"
        "mcr p15,0,%0,c7,c5,0\n"    /* invalidate i-cache all */
        :
        :
        "r" (0)  /* clobber list */
    );

    ithExitCritical();

#elif defined(__SM32__)
    int ncs, bs;
    int cache_size, cache_line_size;
    int i = 0;

    // Number of cache sets
    ncs             = (mfspr(SPR_ICCFGR) >> 3) & 0xf;

    // Number of cache block size
    bs              = ((mfspr(SPR_ICCFGR) >> 7) & 0x1) + 4;

    // Calc Cache size
    cache_line_size = 1 << bs;
    cache_size      = 1 << (ncs + bs);

    ithEnterCritical();

    // Disable IC, DC
    mtspr(SPR_SR, mfspr(SPR_SR) & ~SPR_SR_ICE);
    mtspr(SPR_SR, mfspr(SPR_SR) & ~SPR_SR_DCE);

    // Flush IC, DC
    do
    {
        mtspr(SPR_ICBIR, i);
        mtspr(SPR_DCBIR, i);
        i += cache_line_size;
    } while (i < cache_size);

    // Enable IC, DC
    mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_ICE);
    mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_DCE);
    asm volatile ("l.nop 0x0\nl.nop 0x0\nl.nop 0x0\nl.nop 0x0\n");

    ithExitCritical();

#elif defined(__riscv)
    CPU_ICACHE_INVALIDATE();
#endif //   __arm__
}

void ithCpuDoze(void)
{
    vTaskSuspendAll();

#if ((CFG_CHIP_FAMILY == 970) || (CFG_CHIP_FAMILY == 9860))
    uint32_t arm_base = (CacheID926() == ITH_ARM_CACHE_TYPE) ? 0xD8300000U : 0xD8400000U;
    //workaround 970 CPU power saving mode problem
    ithWriteRegMaskA(arm_base, 0x00010000U, 0x00010000U);
#endif

#ifdef __arm__
    __asm__ __volatile__ (
        "mov r3,#0\n"
        "mcr p15,0,r3,c7,c10,0\n"   /* Clean D-Cache All */
        "mcr p15,0,r3,c7,c10,4\n"   /* Sync (drain write buffer) */
        "mcr p15,0,r3,c7,c0,4\n"    /* execute the IDLE instruction but it only executes when it is not user mode */
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        :
        :
        : "r3"  /* clobber list */
    );
#elif defined(__SM32__)
    asm volatile ("l.mtspr\t\t%0,%1,0" : : "r" (SPR_PMR), "r" (SPR_PMR_DME));
    asm volatile ("l.nop\t\t0x3388"); // One delay slot to enter doze mode
    asm volatile ("l.nop\t\t0x3388"); // Another delay slot to avoid the
                                      // delay interrupt

#elif defined(__riscv)
    CPU_DOZE();
#endif //   __arm__

#if ((CFG_CHIP_FAMILY == 970) || (CFG_CHIP_FAMILY == 9860))
    // workaround 970 CPU power saving mode problem
    if (CacheID926() == ITH_ARMLITE_CACHE_TYPE)
    {
        ithWriteRegA(arm_base, 0x1U);
    }
    else
    {
        ithWriteRegMaskA(arm_base, 0x00000000U, 0x00010000U);
    }
#endif

    (void) xTaskResumeAll();
}

__attribute__((used)) void ithFlushMemBuffer(void)
{
    ithEnterCritical();

#ifdef __arm__
    __asm__ __volatile__ (
        "mov r3,#0\n"
        "mcr p15,0,r3,c7,c10,4\n"   /* flush d-cache write buffer */
        :
        :
        : "r3"  /* clobber list */
    );

    /* memory wrap flush only apply on tiling mode, and AXI0 will not enable tiling mode. */
    #if 0
    ithMemWrapFlush(ITH_MEM_AXI0WRAP);
    #endif

#elif defined(__SM32__)

    {
        int i;
        while (mfspr(SPR_SBSTATUS) != 1) { }
        ithWriteRegMaskA(0xB0200000U + 0x78U, (0x1U << 13U), (0x1U << 13U));
        for (i = 0; i < 100; i++)
        {
            asm volatile(""); // TODO
        }
        ithWriteRegMaskA(0xB0200000U + 0x78U, 0U,            (0x1U << 13U));
        ithMemWrapFlush(ITH_MEM_AXI2WRAP);
    }

#endif //   __arm__

    ithExitCritical();
}

void ithFlushAhbWrap(void)
{
    ithEnterCritical();

    ithWriteRegMaskH(0x3DAU, (uint16_t)(0x1UL << 11U), (uint16_t)(0x1UL << 11U));
    ithWriteRegMaskH(0x3F8U, 0x2U, 0x2U);

    // wait AHB Wrap flush finish!
    while (0U !=(ithReadRegH(0x3F8U) & 0x2U)) { }

    ithWriteRegMaskH(0x3DAU, 0x0U, (uint16_t)(0x1UL << 11U));

    ithExitCritical();
}

void ithYieldFromISR(bool yield)
{
    portYIELD_FROM_ISR(yield ? pdTRUE : pdFALSE);
}

void ithTaskYield(void)
{
    taskYIELD();
}

void ithEnterCritical(void)
{
    if (!ithCpuInInterrupt())
    {
        portENTER_CRITICAL();
    }
}

void ithExitCritical(void)
{
    if (!ithCpuInInterrupt())
    {
        portEXIT_CRITICAL();
    }
}

ITHCpuMode ithGetCpuMode(void)
{
#ifdef __arm__
    uint32_t mode = 0U;

    __asm__ __volatile__ (
        "mrs %0, CPSR\n"
        : "=r" (mode)
    );
    return (mode & 0x1FU);

#elif defined(__SM32__)

    return (mfspr(SPR_SR) & SPR_SR_SM) ? ITH_CPU_SVC : ITH_CPU_SYS;

#elif defined(__riscv)
    return ITH_CPU_SYS;

#endif //   __arm__
}
