/**
 * @file stl_stack.h
 * @author I-Chun Lai (ichun.lai@ite.com.tw)
 * @brief Safety Test Library for Stack Overflow Test
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
 */

#ifndef STL_FA626_INC_STL_STACK_H_
#define STL_FA626_INC_STL_STACK_H_

#include <stdint.h>
#include "stl.h"

void stl_stack_test_init(void);

/**
 * @brief STACK Overflow test
 *
 *        This function only test fixed stack area:
 *        - ISR stack
 *        - FIQ stack
 *        - SVC stack
 *        - DATA Abort stack
 *        - Undefined Instruction stack
 *        - SYS stack
 *        See <sdk_root>/openrtos/boot/fa626/default.lds
 *
 * @return stl_status
 */
stl_status stl_stack_runtime_test(void);


#endif  // STL_FA626_INC_STL_STACK_H_
