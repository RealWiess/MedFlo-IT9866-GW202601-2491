#include <memory.h>
#include "stl_cpu.h"
#include "stl_cpu2.h"
#include "stl_ram.h"
#include "stl_stack.h"
#include "stl_cpu_.h"
#include "ite/itp.h"

#define __FA626_CP15_C1_ENABLE_MMU      (1u << 0)
#define __FA626_CP15_C1_ENABLE_D_CACHE  (1u << 2)
#define __FA626_CP15_C1_ENABLE_W_BUFFER (1u << 3)
#define __FA626_CP15_C1_ENABLE_I_CACHE  (1u << 12)

#define __SRAM_START                    (0xa0000000)
#define __SRAM_SIZE                     (32 * 1024)
#define __SRAM_END                      (__SRAM_START + __SRAM_SIZE)
#define __CPU_DCACHE_SIZE               (32 * 1024) // The 1st CPU's data cache size is 32K.

extern uint8_t __class_b_start[];
extern uint8_t __class_b_end[];

// This is the code which executes on the 1st CPU.
void stl_startup(void)
{
    extern uint8_t __heap_start__[];
    extern uint8_t __heap_end__[];

    uint32_t    irq_state = 0;
    uint32_t    backup_regs[4];

    fa626_irq_local_disable_save(&irq_state);

    // Test the 1st CPU
    if (stl_cpu_startup_test() != stl_success)
    {
        // test fail
        goto fail;
    }

    // Test the 2nd CPU
    if (stl_cpu2_startup_test() != stl_success)
    {
        // test fail
        goto fail;
    }

    // Test the SRAM region.
    if (stl_ram_test_without_backup(
        (uint8_t *)__SRAM_START,
        (uint8_t *)__SRAM_END,
        0, backup_regs) != stl_success)
    {
        // test fail
        goto fail;
    }

    //
    // Test the FULL DRAM region.
    //

    // First, the heap and class_b sections are tested with the
    // destructive test.
    if (stl_ram_test_without_backup(
        (uint8_t *)__heap_start__,  // from __heap_start__
        (uint8_t *)__class_b_end,   // to __class_b_end
        0, backup_regs) != stl_success)
    {
        // test fail
        goto fail;
    }

    // Then the code, data, bss, stack sections are tested with the
    // non-destructive test.

    // Before testing the code region, the memory test code should
    // be moved to the SRAM, or the code will be destroyed during the test.
    // This is why we test the SRAM region before testing the DRAM region.

    // the start address of the stl_ram_test function
    extern uint8_t stl_ram_test_start[];
    // the end address of the stl_ram_test function
    extern uint8_t stl_ram_test_end[];
    // the size of the stl_ram_test function
    uint32_t    stl_ram_test_size = (uint32_t)stl_ram_test_end
                                    - (uint32_t)stl_ram_test_start;

    // copy the stl_ram_test function to the end of SRAM region.
    stl_status (*pf_stl_ram_test)(
        uint8_t  *start,
        uint8_t  *end,
        uint32_t pattern,
        uint32_t *backup) = (void *)(__SRAM_END - stl_ram_test_size);
    memcpy(
        pf_stl_ram_test,
        stl_ram_test_start,
        stl_ram_test_size);

    // Call the stl_ram_test function which is already in the
    // end of heap section.
    // The memory to be tested will be backuped in the start
    // of CLASS B region.
    if (pf_stl_ram_test(
        (uint8_t *)0,
        (uint8_t *)__heap_start__,
        0, (uint32_t *)(__class_b_start)) != stl_success)
    {
        // test fail
        goto fail;
    }

    stl_stack_test_init();

    fa626_irq_local_restore(irq_state);
    return;

fail:
    fa626_irq_local_restore(irq_state);
    // DO error handling....
    return;
}
