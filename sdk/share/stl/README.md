# How to use the STL library

The STL library is designed for obtaining the UL/CSA/IEC 60730-1/60335-1. The library includes some test code examples used to test the cpu, memory, rom, stack, and clock skewing (The functions for testing if there is clock skew are developing and not included in this release).

1. Put the library (the whole `stl` directory) in the `<sdk_root>/sdk/share/`。
2. Edit the `<sdk_root>/sdk/share/Kconfig` file and append the `source "$CMAKE_SOURCE_DIR/sdk/share/stl/Kconfig"` statement to the file.

   ```
   #
   # For a description of the syntax of this configuration file,
   # see Documentation/kbuild/kconfig-language.txt.
   #

   source "$CMAKE_SOURCE_DIR/sdk/share/cli/Kconfig"
   ...
   source "$CMAKE_SOURCE_DIR/sdk/share/stl/Kconfig"
   ```

3. Edit the `<sdk_root>/openrtos/boot/CMakeLists.txt` file. Add the following statements related with `CFG_BUILD_STL`. These statements are used to let the `boot` library link the `stl` library.

   ```cmake
   enable_language(C ASM)

   add_library(boot STATIC)

   target_sources(boot
   PRIVATE
       init.c
   )

   ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BUILD_STL)
   if (DEFINED CFG_BUILD_STL)
       target_link_libraries(boot
       PRIVATE
           stl
       )
   endif()
   ```

4. Edit the `<sdk_root>/openrtos/boot/init.c` file. Include the `stl.h` in this file. Call `stl_startup();` in the beginning of the BootInit() function.

   ```c
   ...
   #ifdef CFG_BUILD_STL
       #include "stl.h"
   #endif
   ...

   void __init BootInit(void)
   {
   #if (CFG_CHIP_FAMILY == 9860)
       #if (CFG_NOR_DTRMODE_FREQ > 0)
       float sclk = _SetSCLKClock();
       #endif
   #endif

   #if (CFG_CHIP_FAMILY == 9860)
       static ITHVmem vmem;
   #endif

   #if (CFG_CHIP_FAMILY == 970)
       // workaround SD/NOR co-bus issue
       #if !defined(CFG_SD0_ENABLE) && !defined(CFG_SD1_ENABLE)
       ithWriteRegMaskA(0xD800006C, 0x00000005, 0x000001FF);
       #endif
       // workaround AX1 JPEG bug
       if (ithGetRevisionId() == 0)
           ithWriteRegMaskA(0xD8000038, 0x0000080A, 0x0000080F);
   #endif

   #ifdef CFG_BUILD_STL
       stl_startup();
   #endif
       ...
   ```

 5.  Edit the `<sdk_root>/sdk/def.cmake`。Add the following statements. These statements are used to build the code for the 2nd CPU.

     ```cmake
     ...
     if (DEFINED CFG_BUILD_STL)
         set(CFG_BUILD_RISC 1)
         add_definitions(
             -DCFG_RISC_ENABLE
         )
     endif()
     ...

     if (DEFINED CFG_UPGRADE_BOOTLOADER
      OR DEFINED CFG_UPGRADE_IMAGE
      OR DEFINED CFG_UPGRADE_DATA
      OR DEFINED CFG_BUILD_STL)
         set(CFG_BUILD_UPGRADE 1)
     endif()
     ```

6. Edit the `<sdk_root>/project/arm_lite_codec/CMakeLists.txt`。Add `enable_language(C CXX ASM)` statement to the beginning of the file. Besides, add `add_subdirectory(${CMAKE_SOURCE_DIR}/sdk/share/stl/fa626/cpu2 ${CMAKE_BINARY_DIR}/sdk/share/stl/fa626/cpu2)` statement to the end of the file.

   ```cmake
   enable_language(C CXX ASM)

   file(MAKE_DIRECTORY
       ${CMAKE_BINARY_DIR}/lib/${CFG_CPU_NAME}
   )

   ...

   add_subdirectory(${CMAKE_SOURCE_DIR}/sdk/codec ${CMAKE_BINARY_DIR}/sdk/codec)
   add_subdirectory(${CMAKE_SOURCE_DIR}/sdk/share/stl/fa626/cpu2 ${CMAKE_BINARY_DIR}/sdk/share/stl/fa626/cpu2)
   ```

7. Edit `<sdk_root>/build.cmake`。Add `ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(stl)` statement to the beginning of the file.

   ```cmake
   ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(stl)
   ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qrdecode)
   ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qrencode)
   ...
   ```

8. Edit `<sdk_root>/project/xxx/Kconfig`。Add the following statements according to the norflash size (in bytes) on the board.

   ```
   config UPGRADE_NOR_IMAGE_SIZE
       default "0x1000000"

   config BUILD_UPGRADE
       def_bool y
   ```

9. Replace `<sdk_root>/openrtos/boot/fa626/default.lds` a with `<sdk_root>/sdk/share/stl/fa626/src/default.lds`.

10. If you want to test the 2nd CPU, remember to add the
      `set CODEC=1` (for IT97x) or `set ARMLITECODEC=1` command in the `<sdk_root>/build/openrtos/xxx.cmd` file.。

11. Double click `<sdk_root>/build/openrtos/xxx.cmd` file. Check the "Libraries --> stl --> Enable IEC-60730 Class B Test" option in qconf window.

12. Check the "Storage --> NOR Device" option in qconf window.


## API Decriptions

- `void stl_startup(void)`

  This function shows how to test the correctness of the registers for the first cpu and the second cpu. It also tests the correctness of the SRAM (located in the range of 0xa0000000~0xa0008000) and the entire dram during system startup, and tests the correctness of the cpu data cache. Finally it initialize the stack test.

  This function should be called as early as possible and must be executed after the bss section and stack have been initialized.

  ATTENTATION: You need to do error handling in this function if the test is failed.

- `stl_status stl_cpu_startup_test()`

   For FA626 (A ARM926ejs based CPU), this function test the flags of CPSR, use the values of 0x55555555 and 0xaaaaaaaa to test if there are bit stuck for register r0~12. It also tests r13 (stack pointer register) and r14 (link register) of all CPU modes.

   When this function is called, the global interrupt of the cpu must be disabled.

- `stl_status stl_cpu_runtime_test()`
  This function only  use the values of 0x55555555 and 0xaaaaaaaa to test if there are bit stuck for register r0~12. It is designed to be call in the runtime to test if the cpu is broken.

  Call this function in the infinite loop in a thread or in the timer interrupt handler.

  This function is interrupt-safed.

  ```c
  #include <unistd.h>
  #include <stdint.h>
  #include <ite/itp.h>
  #include <stl_cpu.h>
  #include <stl_stack.h>
  #include <stl_ram.h>
  #include <stl_rom.h>

  void *TestFunc(void *arg)
  {
      stl_status status;

      itpInit();

      while (1)
      {
          status = stl_rom_runtime_test();
          if (status == stl_error)
          {
              goto fail;
          }
          status = stl_stack_runtime_test();
          if (status == stl_error)
          {
              goto fail;
          }

          status = stl_cpu_runtime_test();
          if (status == stl_error)
          {
              goto fail;
          }

          status = stl_ram_runtime_test();
          if (status == stl_error)
          {
              goto fail;
          }

          status = stl_clock_runtime_test();
          if (status == stl_error)
          {
              goto fail;
          }

          usleep(1000000);
      }
  fail:
      // Do your error handling

      return 0;
  }
  ```

- `void stl_stack_test_init(void)`

  This function will put a flag 'ITE\0' in the bottom of the stack for different cpu modes. This function and the `stl_stack_runtime_test()` function are used to test the overflow of the fixed stacks. If you want to test the stacks of the threads dynamically allocated at runtime, please see the FreeRTOS manual and search the keywork `vApplicationStackOverflowHook`。

- `stl_status stl_stack_runtime_test(void)`

   This function test if the flags put in the bottom of the stack for different cpu modes are overwritten.

   Call this function in the infinite loop in a thread

- `stl_status stl_ram_test(uint8_t *start, uint8_t *end, uint32_t pattern, uint32_t *backup);`

   This function do the RAM test using the MarchC algorithm. It will test 256 bytes of memory in a loop until the whole memory region specified by the `start` and `end` parameter are tested. It backup 256 bytes data of the memory to be tested, use the MarchC algorithm to test the 256 bytes memory, and restore the data of this region. Then it does the same process for the next 256 bytes memory region.

   When this function is called, the global interrupt of the cpu must be disabled.

- `stl_status stl_ram_test_without_backup(uint8_t *start, uint8_t *end, uint32_t pattern, uint32_t *backup);`

   This function is similar to the `stl_ram_test()` functions, but it doesn't backup the data of the memory region to be tested and only  the RAM test using the MarchC algorithm. Thus the data of the memory region to be tested will be destroyed. It is faster than the the `stl_ram_test()` functions.

- `stl_rom_test_init()`

   This function will calculate the size and the crc of the bootloader (if `CFG_BOOTLOADER_ENABLE` is defined) and the fireware image stored on the norflash.

   This function must be call after `ioctl(ITP_DEVICE_NOR, ITP_IOCTL_INIT, NULL);`。

   This function is not thread-safe.

- `stl_status stl_rom_runtime_test(void)`

   This function implement a state machine which will read 64 Kbytes of data in the bootloader or the firmware image region from the norflash, then calculate the crc value of 1024 bytes of data each time when it is called. Until it finish calculating the crc of the 64 Kbytes of data, it will read the next 64 Kbytes of data and do the crc calculation again. The read-and-crc-caculation process will be done over and over again until it finish calculating the crc of all the bootloader or fimware image region. Once the crc value of the whole bootloader or firmware image region is calculated, this function will compare the crc value which are just calculated with  the crc value calculated and stored by the `stl_rom_test_init()` function. If two values are different, it means the data in the read-only bootloader or firmware image region is modified and the function will return `stl_error`, otherwise it will return `stl_success`.

   This function must be call after `ioctl(ITP_DEVICE_NOR, ITP_IOCTL_INIT, NULL);`。

   This function is not thread-safe. Call this function in the infinite loop in a thread.

- `stl_status stl_clock_runtime_test(void)`

   This function detects whether the system has clock skew problems.

   This function must be called after the vTaskStartScheduler() is called. Beside, the CFG_RTC_ENABLE option must be turned on.

   This function is not thread-safe. Call this function in the infinite loop in a thread.
