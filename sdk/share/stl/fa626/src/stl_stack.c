#include "stl_stack.h"

#define MAGIC_NUMBER (('I' << 24ul) | ('T' << 16ul) | ('E' << 8ul))

extern char __stack_start__[];
extern char __irq_stack_top__[];
extern char __fiq_stack_top__[];
extern char __svc_stack_top__[];
extern char __abt_stack_top__[];
extern char __und_stack_top__[];

void stl_stack_test_init(void)
{
    *(volatile uint32_t *)__stack_start__   = MAGIC_NUMBER;  // for irq stack
    *(volatile uint32_t *)__irq_stack_top__ = MAGIC_NUMBER;  // for fiq stack
    *(volatile uint32_t *)__fiq_stack_top__ = MAGIC_NUMBER;  // for svc stack
    *(volatile uint32_t *)__svc_stack_top__ = MAGIC_NUMBER;  // for abt stack
    *(volatile uint32_t *)__abt_stack_top__ = MAGIC_NUMBER;  // for und stack
    *(volatile uint32_t *)__und_stack_top__ = MAGIC_NUMBER;  // for sys stack
}

stl_status stl_stack_runtime_test(void)
{
    if ((MAGIC_NUMBER != *(volatile uint32_t *)__stack_start__  )
     || (MAGIC_NUMBER != *(volatile uint32_t *)__irq_stack_top__)
     || (MAGIC_NUMBER != *(volatile uint32_t *)__fiq_stack_top__)
     || (MAGIC_NUMBER != *(volatile uint32_t *)__svc_stack_top__)
     || (MAGIC_NUMBER != *(volatile uint32_t *)__abt_stack_top__)
     || (MAGIC_NUMBER != *(volatile uint32_t *)__und_stack_top__))
    {
        return stl_error;
    }
    return stl_success;
}