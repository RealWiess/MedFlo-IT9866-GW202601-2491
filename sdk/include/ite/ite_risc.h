/*
 * Copyright (c) 2007 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * ITE RISC header File
 *
 */

#ifndef ITE_RISC_H
#define ITE_RISC_H

// =============================================================================
//                              Include Files
// =============================================================================
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "ith/ith_risc.h"

// =============================================================================
//                              Structure Definition
// =============================================================================

// =============================================================================
//                              Enumeration Type Definition
// =============================================================================

typedef enum ITE_RISC_ENGINE_TAG
{
    ITE_SW_PERIPHERAL_ENGINE = 1, /**< SW peripheral engine */
    ITE_RISC_ENGINE_RESERVED
} ITE_RISC_ENGINE;

// =============================================================================
//                              Constant Definition
// =============================================================================

#define ITE_RISC_OK_RESULT  0
#define INVALID_MEM_TARGET  -1
#define INVALID_LOAD_SIZE   -2

// =============================================================================
//                              Function Declaration
// =============================================================================
int32_t
iteRiscInit (
    void);

int32_t
iteRiscTerminate (
    void);

void
iteRiscWakeup (
    void);

int32_t
iteRiscLoadData (
    int32_t load_target,
    uint8_t * p_data,
    int32_t data_size);

void
iteRiscFireCpu (
    int32_t target_cpu);

void
iteRiscResetCpu (
    int32_t target_cpu);

uint32_t
iteRiscGetProgramCounter (
    int32_t target_cpu);

uint8_t *
iteRiscGetTargetMemAddress (
    int32_t load_target);

int32_t
iteRiscOpenEngine (
    ITE_RISC_ENGINE engine_type,
    uint32_t        boot_mode);

int32_t
iteRiscTerminateEngine (
    void);

#ifdef __cplusplus
}
#endif

#endif
