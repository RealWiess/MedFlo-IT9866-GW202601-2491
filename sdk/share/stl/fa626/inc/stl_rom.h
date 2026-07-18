/**
 * @file stl_rom.h
 * @author I-Chun Lai (ichun.lai@ite.com.tw)
 * @brief Safety Test Library for ROM Test
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
 *
 * The content of the Norflash is structured as follows:
 *  _______________
 * |               | <- the beginning address is 0x0
 * |  bootloader   |
 * |_______________|
 * |               |
 * | 0xFF padding  |
 * |_______________|
 * |               |
 * |  MAC address  |
 * |_______________|
 * |               |
 * | 0xFF padding  |
 * |_______________|
 * |               | <- the beginning address is defined by
 * | Fimware image |    CFG_UPGRADE_IMAGE_POS
 * |_______________|
 * |               |
 * | 0xFF padding  |
 * |_______________|
 * |               |
 * |  File system  |
 * |  Partitions   |
 * |_______________|
 *
 * The bootloader may not exist, depending on whether
 * CFG_BOOTLOADER_ENABLE is defined.
 *
 * This test is only for the bootloader and firmware areas.
 */

#ifndef STL_FA626_INC_STL_ROM_H_
#define STL_FA626_INC_STL_ROM_H_

#include <stdint.h>
#include "stl.h"


/**
 * @brief Initial the rom test
 *
 * Get the size of the bootloader and firmware image regions.
 *
 * Calculate the crc value of these two regions and record the crc
 * values in the internal static variables.
 *
 * @return stl_status stl_success if the size is reasonable, otherwise
 *                    return stl_error.
 */
stl_status stl_rom_test_init(void);

/**
 * @brief Do the rom test
 *
 * @return stl_status - stl_success if the crc calculation for both regions
 *                      is complete and the crc matches the value calculated
 *                      when the system was just started.
 *                    - stl_testing if crc calculations for both regions are
 *                      in progress.
 *                    - stl_fail if the crc calculation for both regions
 *                      is complete and the crc does not matche the value
 *                      calculated when the system was just started.
 */
stl_status stl_rom_runtime_test(void);


#endif  // STL_FA626_INC_STL_ROM_H_
