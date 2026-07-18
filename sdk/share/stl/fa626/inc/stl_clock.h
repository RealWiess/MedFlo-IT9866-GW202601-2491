/**
 * @file stl_clock.h
 * @author I-Chun Lai (ichun.lai@ite.com.tw)
 * @brief Safety Test Library for Clock Test
 * @version 0.1
 * @date 2022-08-25
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
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STL_FA626_INC_STL_CLOCK_H_
#define STL_FA626_INC_STL_CLOCK_H_

#include <stdint.h>
#include "stl.h"

/**
 * @brief Clock test
 *
 *        Do clock test at system startup time.
 *
 *        WARNING > This function must be called after the vTaskStartScheduler()
 *        is called. The CFG_RTC_ENABLE option must be turned on.
 *
 *        This function uses the xTaskGetTickCount() function of freertos to
 *        get the current time. It assumes that the operating system's tick
 *        rate is 1000Hz (this value is determined by configTICK_RATE_HZ, see
 *        FreeRTOSConfig.h).
 *
 * @return stl_status
 */
stl_status stl_clock_runtime_test(void);

#endif  // STL_FA626_INC_STL_CLOCK_H_