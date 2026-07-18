#include <stdint.h>
#include <cpu_irq.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"

typedef struct {
    union {
        struct {
            long x1;
            long x4;
            long x5;
            long x6;
            long x7;
            long x10;
            long x11;
            long x12;
            long x13;
            long x14;
            long x15;
            long x16;
            long x17;
            long x28;
            long x29;
            long x30;
            long x31;
        };
        long caller_regs[17];
    };
    long      mepc;
    long      mstatus;
    long      mxstatus;
    #ifdef __riscv_flen
    #if __riscv_flen == 64
    long long fp_caller_regs[20];
    #else
    long      fp_caller_regs[20];
    #endif
    int       fcsr;
    #endif
} SAVED_CONTEXT;

extern void freertos_tick_handler(void);

 __attribute__((always_inline))
 static inline uint32_t r_fp()
{
    uint32_t x;
    asm volatile("mv %0, s0" : "=r" (x) );
    return x;
}

extern int ithPrintf(const char *fmt, ...);

// See https://pdos.csail.mit.edu/6.828/2020/labs/traps.html
void backtrace()
{
    uint32_t fp = r_fp();

    ithPrintf("backtrace: ");
    while (fp != 0)
    {
        uint32_t ra = *((uint32_t*)(fp - 4));
        ithPrintf("0x%X ", ra);
        if (ra == 0) break;
        fp = *((uint32_t*)(fp - 8));
    }
    ithPrintf("\n");
}

__attribute__((weak)) void freertos_mswi_handler(void)
{
    clear_csr(NDS_MIE, MIP_MSIP);
}

extern __attribute__((aligned(16))) StackType_t xISRStack[configISR_STACK_SIZE_WORDS];

// This is a temporary solution.
__attribute__((weak))
void freertos_except_handler(
    unsigned long mcause,
    unsigned long mepc)
{
    uint32_t mdcause = read_csr(NDS_MDCAUSE);

#ifndef CFG_BUILD_MINSIZEREL
    ithPrintf("=================== Exception ==================\n");
#endif
    backtrace();
    ithPrintf("Unhandled Trap : mcause = 0x%x, mepc = 0x%x\n", mcause, mepc);
#ifndef CFG_BUILD_MINSIZEREL
    switch (mcause)
    {
    case 0:
        ithPrintf("Instruction address misaligned\n");
        break;

    case 1:
        ithPrintf("Instruction access fault\n");
        switch (mdcause)
        {
        case 1:
            ithPrintf("ECC/Parity error\n");
            break;

        case 2:
            ithPrintf("PMP instruction access violation\n");
            break;

        case 3:
            ithPrintf("Bus error\n");
            break;

        case 4:
            ithPrintf("PMA empty hole access\n");
            break;
        }
        break;

    case 2:
        ithPrintf("Illegal instruction\n");
        break;

    case 3:
        ithPrintf("Breakpoint\n");
        break;

    case 4:
        ithPrintf("Load address misaligned\n");
        break;

    case 5:
        ithPrintf("Load access fault\n");
        switch (mdcause)
        {
        case 1:
            ithPrintf("ECC/Parity error\n");
            break;

        case 2:
            ithPrintf("PMP load access violation\n");
            break;

        case 3:
            ithPrintf("Bus error\n");
            break;

        case 4:
            ithPrintf("Misaligned address\n");
            break;

        case 5:
            ithPrintf("PMA empty hole access\n");
            break;

        case 6:
            ithPrintf("PMA attribute inconsistency\n");
            break;

        case 7:
            ithPrintf("PMA NAMO exception\n");
            break;
        }
        break;

    case 6:
        ithPrintf("Store/AMO address misaligned\n");
        break;

    case 7:
        ithPrintf("Store/AMO access fault\n");
        switch (mdcause)
        {
        case 1:
            ithPrintf("ECC/Parity error\n");
            break;

        case 2:
            ithPrintf("PMP store access violation\n");
            break;

        case 3:
            ithPrintf("Bus error\n");
            break;

        case 4:
            ithPrintf("Misaligned address\n");
            break;

        case 5:
            ithPrintf("PMA empty hole access\n");
            break;

        case 6:
            ithPrintf("PMA attribute inconsistency\n");
            break;

        case 7:
            ithPrintf("PMA NAMO exception\n");
            break;
        }
        break;

    case 11:
        ithPrintf("Environment call from M-mode\n");
        break;

    case 12:
        ithPrintf("Instruction page fault\n");
        break;

    case 13:
        ithPrintf("Load page fault\n");
        break;

    case 15:
        ithPrintf("Store/AMO page fault\n");
        break;

    case 32:
        if (read_csr(NDS_MSP_BOUND) == (uint32_t)xISRStack)
            ithPrintf("\n\n@@ ISR stack overflow!!! \n");
        else
            ithPrintf("\n\n@@ Task:[%s] stack overflow!!! \n", pcTaskGetName(NULL));
        break;

    case 33:
        ithPrintf("Stack underflow exception\n");
        break;
    }
    ithPrintf("================================================\n");
#endif
    while (1)
        ;
}

__attribute__((weak))
void ithIntrDoIrq(void)
{}

void freertos_irq_handler(unsigned long mcause)
{
    __cpu_preempt_count_add(1);

    /* Do your trap handling */
    if ((mcause & MCAUSE_CAUSE) == IRQ_M_TIMER)
    {
        /* Machine timer interrupt */
        //mtime_handler();
        freertos_tick_handler();
    }
    else if ((mcause & MCAUSE_CAUSE) == IRQ_M_EXT)
    {
        /* Machine-level interrupt from PLIC */
        ithIntrDoIrq();
    }
    else if ((mcause & MCAUSE_CAUSE) == IRQ_M_SOFT)
    {
        /* Machine SWI interrupt */
        freertos_mswi_handler();
    }

    __cpu_preempt_count_sub(1);
}

__attribute__ ((weak, alias ("freertos_except_handler")))
void baremetal_except_handler(
    unsigned long mcause,
    unsigned long mepc);

void baremetal_trap_handler(unsigned long mcause, SAVED_CONTEXT *context)
{
    /* Do your trap handling */
    if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == IRQ_M_EXT))
    {
        /* Machine-level interrupt from PLIC */
        ithIntrDoIrq();
    }
    else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == IRQ_M_TIMER))
    {
        /* Machine timer interrupt */
    }
    else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == IRQ_M_SOFT))
    {
        /* Machine SWI interrupt */
    }
    else if (!(mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == TRAP_M_ECALL))
    {
        /* Machine Syscal call */
        context->mepc += 4;
    }
    else
    {
        /* Unhandled Trap */
        //context->mepc = except_handler(mcause, context->mepc, context->caller_regs);
        baremetal_except_handler(mcause, context->mepc);
    }
}