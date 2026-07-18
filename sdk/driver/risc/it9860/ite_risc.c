/**
 * Copyright (c) 2007 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * ITE RISC header File
 *
 */

// =============================================================================
//                              Include Files
// =============================================================================
#include "ite/ith.h"
#include "ite/itp.h"
#include "stdlib.h"
#include "string.h"
#include "ite/ite_risc.h"
#include "stdio.h"

// =============================================================================
//                              Structure Definition
// =============================================================================

// =============================================================================
//                              Enumeration Type Definition
// =============================================================================

// =============================================================================
//                              Constant Definition
// =============================================================================
#define ARM_LITE_BASE_ADDR              0xD8400000U
#define ARM_LITE_CTRL_REG               (ARM_LITE_BASE_ADDR + 0x0U)
#define ARM_LITE_FIRE_BIT_OFFSET        0U
#define ARM_LITE_STALL_BIT_OFFSET       8U

#define ARM_LITE_RESET_REG              0xD800000CU
#define ARM_LITE_RESET_BIT_OFFSET       29U

#define ARM_LITE_PC                     (ARM_LITE_BASE_ADDR + 0xCU)
#define ARM_LITE_INST_ADDR              (ARM_LITE_BASE_ADDR + 0x20U)
#define ARM_LITE_REMAP_ADDR             (ARM_LITE_BASE_ADDR + 0x24U)

// ALT CPU is running in SRAM
#define RISC_BASE_ADDR                  0xB0200000U
#define RISC_REMAP_ADDR                 (RISC_BASE_ADDR + 0x74U)
#define ALT_CPU_IMG_ADDRESS             0xA0000000U
#define ALT_CPU_CTRL_REG                (RISC_BASE_ADDR + 0x6CU)
#define ALT_CPU_FIRE_BIT_OFFSET         (0U)
#define ALT_CPU_STALL_BIT_OFFSET        (3U)

#define ALT_CPU_RESET_REG               0xD8000050U
#define ALT_CPU_RESET_BIT_OFFSET        27U
#define ALT_CPU_EN_CLK_BIT_OFFSET       9U
#define ALT_CPU_EN_DG_CLK_BIT_OFFSET    8U
#define ALP_CPU_PC                      (RISC_BASE_ADDR + 0x60U)

/**
 * This buffer is used to store RISC1 binary image.
 * The size of the buffer is defined as RISC1_IMAGE_SIZE + AUDIO_MESSAGE_SIZE + SHARE_MEM1_SIZE.
 * The buffer is aligned to 32 bytes and placed in the .jcr section, which is used for the relocatable read-only data.
 */
static uint8_t  g_risc_buffer_[RISC1_IMAGE_SIZE + AUDIO_MESSAGE_SIZE + SHARE_MEM1_SIZE] __attribute__ ((aligned(32), section(".jcr")));
/**
 * This variable is used to indicate if the RISC1 has been initialized.
 * Initially, it is set to false.
 */
static bool     gb_is_inited_;
/**
 * This variable is used to store the setting of ARM Lite before power down.
 * Its initial value is 0.
 */
static uint32_t g_armlite_setting_;

// =============================================================================
//                              Function Declaration
// =============================================================================
int32_t
iteRiscInit (
    void)
{
    if (gb_is_inited_ == false)
    {
        uint32_t startup_addr = (uint32_t)g_risc_buffer_;
        (void)memset(g_risc_buffer_, 0x0, sizeof(g_risc_buffer_));
        ithWriteRegA(ARM_LITE_REMAP_ADDR, startup_addr);
        ithWriteRegA(ARM_LITE_INST_ADDR, startup_addr);
        ithWriteRegA(RISC_REMAP_ADDR, startup_addr);
        gb_is_inited_ = true;
    }
    return 0;
}

int32_t
iteRiscTerminate (
    void)
{
    g_armlite_setting_ = ithReadRegA(ARM_LITE_CTRL_REG);
    ithSetRegBitA(ARM_LITE_CTRL_REG, ARM_LITE_STALL_BIT_OFFSET);
    return 0;
}

void
iteRiscWakeup (
    void)
{
    if ((g_armlite_setting_ & (((uint32_t)1U) << ARM_LITE_STALL_BIT_OFFSET)) == 0U)
    {
        ithClearRegBitA(ARM_LITE_CTRL_REG, ARM_LITE_STALL_BIT_OFFSET);
    }
}

uint8_t *
iteRiscGetTargetMemAddress (
    int32_t load_target)
{
    switch (load_target)
    {
        case RISC1_IMAGE_MEM_TARGET:
            return g_risc_buffer_;

        case RISC2_IMAGE_MEM_TARGET:
            return (uint8_t *) ALT_CPU_IMG_ADDRESS;

        case AUDIO_MESSAGE_MEM_TARGET:
            return &g_risc_buffer_[RISC1_IMAGE_SIZE];

        case SHARE_MEM1_TARGET:
            return &g_risc_buffer_[RISC1_IMAGE_SIZE + AUDIO_MESSAGE_SIZE];

        default:
            /* invalid target */
            return NULL;
    }
}

int32_t
iteRiscLoadData (
    int32_t load_target,
    uint8_t * p_data,
    int32_t data_size)
{
    switch (load_target)
    {
        case RISC1_IMAGE_MEM_TARGET:
            if (data_size > RISC1_IMAGE_SIZE)
            {
                return INVALID_LOAD_SIZE;
            }
            break;

        case RISC2_IMAGE_MEM_TARGET:
            if (data_size > RISC2_IMAGE_SIZE)
            {
                return INVALID_LOAD_SIZE;
            }
            break;

        case AUDIO_MESSAGE_MEM_TARGET:
            if (data_size > AUDIO_MESSAGE_SIZE)
            {
                return INVALID_LOAD_SIZE;
            }
            break;

        case SHARE_MEM1_TARGET:
            if (data_size > SHARE_MEM1_SIZE)
            {
                return INVALID_LOAD_SIZE;
            }
            break;

        default:
            return INVALID_MEM_TARGET;
    }

    (void)memcpy(iteRiscGetTargetMemAddress(load_target), p_data, (uint32_t)data_size);

    if (load_target != RISC2_IMAGE_MEM_TARGET)
    {
        ithFlushDCacheRange((void *)iteRiscGetTargetMemAddress(load_target), (uint32_t)data_size);
        ithFlushMemBuffer();
    }

    return ITE_RISC_OK_RESULT;
}

void
iteRiscFireCpu (
    int32_t target_cpu)
{
    switch (target_cpu)
    {
        case RISC1_CPU:
            ithWriteRegMaskA(ARM_LITE_CTRL_REG,
                (((uint32_t)1U) << ARM_LITE_FIRE_BIT_OFFSET) | (((uint32_t)0U) << ARM_LITE_STALL_BIT_OFFSET),
                (((uint32_t)1U) << ARM_LITE_FIRE_BIT_OFFSET) | (((uint32_t)1U) << ARM_LITE_STALL_BIT_OFFSET));
            break;

        case RISC2_CPU:
            ithWriteRegMaskA(ALT_CPU_CTRL_REG,
                (((uint32_t)1U) << ALT_CPU_FIRE_BIT_OFFSET) | (((uint32_t)0U) << ALT_CPU_STALL_BIT_OFFSET),
                (((uint32_t)1U) << ALT_CPU_FIRE_BIT_OFFSET) | (((uint32_t)1U) << ALT_CPU_STALL_BIT_OFFSET));
            break;

        default:
            break;
    }
}

void
iteRiscResetCpu (
    int32_t target_cpu)
{
    int32_t i;

    switch (target_cpu)
    {
        case RISC1_CPU:
            ithWriteRegMaskA(ARM_LITE_CTRL_REG,
                (((uint32_t)1U) << ARM_LITE_STALL_BIT_OFFSET),
                (((uint32_t)1U) << ARM_LITE_STALL_BIT_OFFSET));
            for (i = 0; i < 10; i++)
            {
                asm ("");
            }
            ithWriteRegMaskA(ARM_LITE_RESET_REG,
                (((uint32_t)1U) << ARM_LITE_RESET_BIT_OFFSET),
                (((uint32_t)1U) << ARM_LITE_RESET_BIT_OFFSET));
            for (i = 0; i < 10; i++)
            {
                asm ("");
            }
            ithWriteRegMaskA(ARM_LITE_RESET_REG,
                (((uint32_t)0U) << ARM_LITE_RESET_BIT_OFFSET),
                (((uint32_t)1U) << ARM_LITE_RESET_BIT_OFFSET));
            break;

        case RISC2_CPU:
            ithWriteRegMaskA(ALT_CPU_CTRL_REG,
                (((uint32_t)1U) << ALT_CPU_STALL_BIT_OFFSET),
                (((uint32_t)1U) << ALT_CPU_STALL_BIT_OFFSET));

            for (i = 0; i < 10; i++)
            {
                asm ("");
            }
            ithWriteRegMaskA(ALT_CPU_RESET_REG,
                (((uint32_t)1U) << ALT_CPU_RESET_BIT_OFFSET) | (((uint32_t)0U) << ALT_CPU_EN_CLK_BIT_OFFSET) | (((uint32_t)0U) << ALT_CPU_EN_DG_CLK_BIT_OFFSET),
                (((uint32_t)1U) << ALT_CPU_RESET_BIT_OFFSET) | (((uint32_t)1U) << ALT_CPU_EN_CLK_BIT_OFFSET) | (((uint32_t)1U) << ALT_CPU_EN_DG_CLK_BIT_OFFSET));
            for (i = 0; i < 10; i++)
            {
                asm ("");
            }
            ithWriteRegMaskA(ALT_CPU_RESET_REG,
                (((uint32_t)0U) << ALT_CPU_RESET_BIT_OFFSET) | (((uint32_t)1U) << ALT_CPU_EN_CLK_BIT_OFFSET) | (((uint32_t)1U) << ALT_CPU_EN_DG_CLK_BIT_OFFSET),
                (((uint32_t)1U) << ALT_CPU_RESET_BIT_OFFSET) | (((uint32_t)1U) << ALT_CPU_EN_CLK_BIT_OFFSET) | (((uint32_t)1U) << ALT_CPU_EN_DG_CLK_BIT_OFFSET));
            break;

        default:
            // do nothing
            break;
    }
}

uint32_t
iteRiscGetProgramCounter (
    int32_t target_cpu)
{
    uint32_t pc = 0U;
    switch (target_cpu)
    {
        case RISC1_CPU:
            pc = ithReadRegA(ARM_LITE_PC);
            break;

        case RISC2_CPU:
            pc = ithReadRegA(ALP_CPU_PC);
            break;

        default:
            // do nothing
            break;
    }
    return pc;
}
