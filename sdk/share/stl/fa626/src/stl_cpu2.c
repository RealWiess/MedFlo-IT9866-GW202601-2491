#include "stl_cpu2.h"
#include "ite/itp.h"
#include "ith/ith_risc.h"
#include "ite/ite_risc.h"
#include "mmio.h"

static uint8_t gInitTestImgBuf[] =
{
    #include "stl_init.hex"
};

static uint8_t gRuntimeTestImgBuf[] =
{
    #include "stl_runtime.hex"
};

static stl_status stl_cpu2_test_(
    uint8_t*    pImgBuf,
    int         imgSize)
{
    int        run = 0, i = 0;
    volatile   stl_status result = stl_testing;

    iteRiscInit();
    // The 1st CPU and the 2nd CPU use the USER_DEFINE_REG0 register
    // to exchange the 2nd CPU's test result.
    //
    // USER_DEFINE_REG0:
    // - 0: test fail
    // - 1: test success
    // - other: testing
    iteRiscResetCpu(ARMLITE_CPU);
    ithWriteRegA(USER_DEFINE_REG0, stl_testing);
    iteRiscLoadData(ARMLITE_CPU_IMAGE_MEM_TARGET, pImgBuf, imgSize);
    iteRiscFireCpu(ARMLITE_CPU);
    for (i = 0; i < 1024; i++)
    {
        asm ("");
    }
    do
    {
        if ((result = ithReadRegA(USER_DEFINE_REG0)) != stl_testing)
        {
            break;
        }
        for (i = 0; i < 1024; i++)
        {
            asm("");
        }
        run++;
    } while (run < 10);

    return result;
}

stl_status stl_cpu2_startup_test()
{
    return stl_cpu2_test_(gInitTestImgBuf, sizeof(gInitTestImgBuf));
}

stl_status stl_cpu2_runtime_test()
{
    return stl_cpu2_test_(gRuntimeTestImgBuf, sizeof(gRuntimeTestImgBuf));
}