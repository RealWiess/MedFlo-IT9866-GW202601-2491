/**
 * @file stl_ram.h
 * @author I-Chun Lai (ichun.lai@ite.com.tw)
 * @brief Safety Test Library for RAM Test
 * @version 0.1
 * @date 2021-12-08
 *
 * @copyright
 *
 * Copyright (c) 2022 ITE Tech.
 * All rights reserved.
 *
 * This software component is licensed by ITE Tech. under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                         opensource.org/licenses/BSD-3-Clause
 */

#ifndef STL_FA626_INC_STL_RAM_H_
#define STL_FA626_INC_STL_RAM_H_

#include <stdint.h>
#include "stl.h"

/**
 * @brief RAM MarchC test
 *        WARNING > If the tested region contains important data (
 *        ex. code, data, stack...), the interrupt must be disabled
 *        when this function is called.
 *
 *        The @arg backup region should not in the tested region.
 *
 * @param start     the start address of the memory to be tested.
 * @param end       the end address of the memory to be tested.
 *                  The end address will not be tested.
 *                  If you want to test the memory range from 0x0 ~ 0x10,
 *                  but you don't want to include 0x10, just specifies
 *                  the end address as 0x10.
 * @param pattern   background pattern.
 * @param backup    start address of backup region. The size of the backup
 *                  region is at least 280 bytes.
 *
 * @return stl_status
 */
stl_status stl_ram_test(
    uint8_t  *start,
    uint8_t  *end,
    uint32_t pattern,
    uint32_t *backup);

/**
 * @brief RAM MarchC test
 *        WARNING > All the RAM area to be tested is destroyed during this test.
 *
 *        The @arg backup region should not in the tested region.
 *
 * @param start     the start address of the memory to be tested.
 * @param end       the end address of the memory to be tested.
 *                  The end address will not be tested.
 *                  If you want to test the memory range from 0x0 ~ 0x10,
 *                  but you don't want to include 0x10, just specifies
 *                  the end address as 0x10.
 * @param pattern   background pattern.
 * @param backup    start address of backup region. The size of the backup
 *                  region is at least 16 bytes.
 *
 * @return stl_status
 */
stl_status stl_ram_test_without_backup(
    uint8_t  *start,
    uint8_t  *end,
    uint32_t pattern,
    uint32_t *backup);

/**
 * @brief Do the RAM runtime test
 *        This test will test all the memory in the class_b section.
 *        It tests 64 bytes of memory each time when it is called until
 *        all memory in the class_b section has been tested.
 *
 *        The class_b section is located at the end of RAM, see the
 *        <sdk_root>/openrtos/boot/fa626/default.lds
 *
 * @return stl_status
 */
stl_status stl_ram_runtime_test();

#endif  // STL_FA626_INC_STL_RAM_H_
