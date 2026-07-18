#if (CFG_CHIP_FAMILY == 9860)
    #include "it9860/mmp_isp.h"
#elif (CFG_CHIP_FAMILY == 9830)
    #include "it9830/mmp_isp.h"
#else
    #error "not defined"
#endif