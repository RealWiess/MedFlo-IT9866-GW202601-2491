#include <stdarg.h>
#include <string.h>
#include "mmio.h"
#include "ite/itp.h"

extern int stl_cpu_startup_test();

// This is the code which executes on the 2nd CPU.
int main(int argc, char **argv)
{
    // Do CPU startup test on the 2nd CPU (arm_lite), and
    // write the test result to USER_DEFINE_REG0 register.
    //
    // The 1st CPU can get the 2nd CPU test result via
    // reading USER_DEFINE_REG0 register.
    //
    // USER_DEFINE_REG0:
    // - 0: test fail
    // - 1: test success
    // - other: testing
    //
    // @note Remember to add `set ARMLITECODEC=1` command in
    //       the <sdk_root>/build/xxx.cmd if you want to do
    //       the 2nd CPU test.
    ithWriteRegA(USER_DEFINE_REG0, stl_cpu_startup_test());

    return 0;
}
