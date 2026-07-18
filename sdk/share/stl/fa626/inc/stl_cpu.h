/**
 * @file stl_cpu.h
 * @author I-Chun Lai (ichun.lai@ite.com.tw)
 * @brief Safety Test Library for CPU Test
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

#ifndef STL_FA626_INC_STL_CPU_H_
#define STL_FA626_INC_STL_CPU_H_

#include <stdint.h>
#include "stl.h"

/**
 * @brief CPU test
 *
 *        Do CPU test at system startup time.
 *
 *        WARNING > The interrupt must be disabled when this function is
 *        called.
 *
 * @return stl_status
 */
stl_status stl_cpu_startup_test(void);

/**
 * @brief CPU test
 *
 *        Do CPU test at system startup time.
 *
 *        WARNING > The interrupt must be disabled when this function is
 *        called.
 *
 * @return stl_status
 */
stl_status stl_cpu_runtime_test(void);

#endif  // STL_FA626_INC_STL_CPU_H_
