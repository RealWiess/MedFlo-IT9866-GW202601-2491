#include "stl_cpu.h"
#include "stl_cpu_.h"

stl_status stl_cpu_runtime_test_(void);

stl_status stl_cpu_runtime_test(void)
{
    uint32_t   irq_state = 0;
    stl_status status;
    fa626_irq_local_disable_save(&irq_state);
    status = stl_cpu_runtime_test_();
    fa626_irq_local_restore(irq_state);

    return status;
}