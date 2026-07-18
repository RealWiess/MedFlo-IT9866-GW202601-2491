#include "stl_ram.h"
#include <stdint.h>
#include "stl_cpu_.h"

//#define DEBUG
#ifdef DEBUG
#include <stdio.h>
#endif

extern uint8_t __class_b_start[];
extern uint8_t __class_b_end[];

stl_status stl_ram_runtime_test()
{
    static uint8_t * start = __class_b_start;
    static uint8_t  backup[280];

    stl_status status;
    uint8_t * end;
    uint32_t   irq_state = 0;

    #define STEP_SIZE   (64)    // test 64 bytes one time

    fa626_irq_local_disable_save(&irq_state);
    end = start + STEP_SIZE;
    if ((uint32_t)end >= (uint32_t)__class_b_end)
        end = __class_b_end;

#ifdef DEBUG
    printf("test start(0x%08X) end(0x%08x) class_b start(0x%08X) end(0x%08x)\n",
        (uint32_t)start, (uint32_t)end, (uint32_t)__class_b_start, (uint32_t)__class_b_end);
#endif
    status = stl_ram_test(start, end, 0x0, (uint32_t*)backup);

    start += STEP_SIZE;
    if ((uint32_t)start >= (uint32_t)__class_b_end)
        start = __class_b_start;
    fa626_irq_local_restore(irq_state);

    return status;
}