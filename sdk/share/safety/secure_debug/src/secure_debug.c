#include "ith/ith_defs.h"
#include "ith/ith_reg.h"

void secure_debug_off(void)
{
    ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_MISC_SET_REG,
                    ITH_GPIO_HOSTSEL_GPIO << ITH_GPIO_HOST_SEL_POS,
                    ITH_GPIO_HOST_SEL_MSK);
}

void secure_debug_on(void)
{
    ithWriteRegMaskA(ITH_GPIO_BASE + ITH_GPIO_MISC_SET_REG,
                    ITH_GPIO_HOSTSEL_HOSTCFG << ITH_GPIO_HOST_SEL_POS,
                    ITH_GPIO_HOST_SEL_MSK);
}
