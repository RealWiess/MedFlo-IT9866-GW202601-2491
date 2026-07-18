#include <stdio.h>
#include <string.h>
#include "can_bus/it9860/can_hw.h"
#include "can_bus/it9860/can_api.h"
// =============================================================================
//                Constant Definition
// =============================================================================
// =============================================================================
//                Macro Definition
// =============================================================================
// =============================================================================
//                Structure Definition
// =============================================================================
// =============================================================================
//                Global Data Definition
// =============================================================================
// =============================================================================
//                Private Function Definition
// =============================================================================
static uint32_t alignment4byte_ (uint32_t value)
{
    uint32_t result;
    uint32_t remainder;

    result    = value >> 2U;
    remainder = value % 4U;

    if (remainder != 0U)
    {
        result += 1U;
    }

    return result;
}

static bool
_CANSetPTBWritePtr (
    CAN_HANDLE  * base,
    CAN_TXOBJ   * info)
{
    // Set TBSEL to PTB or STB
    ithWriteRegMaskA((base->ADDR | CAN_CFG_STAT_REG), 0x0 << CAN_TBSEL_SHIFT, (CAN_TBSEL | CAN_BUSOFF));

    if (ithCANGetTBStatus(base, 0x0) == BUFF_FULL)
    {
        // printf("[MSG]CAN tx ptb buffer full\n");
        return false;
    }

    return true;
}

static bool
_CANSetSTBWritePtr (
    CAN_HANDLE  * base,
    CAN_TXOBJ   * info)
{
    // Set TBSEL to STB
    ithWriteRegMaskA((base->ADDR | CAN_CFG_STAT_REG), 0x1 << CAN_TBSEL_SHIFT, (CAN_TBSEL | CAN_BUSOFF));

    if (ithCANGetTBStatus(base, 0x1) == BUFF_FULL)
    {
        // printf("[MSG]CAN tx stb buffer full\n");
        return false;
    }

    return true;
}

static void
_CANExcHandler (
    CAN_HANDLE * base)
{
    int i = 0;
    // sw Reset
    ithCANSWReset(base, true);

    for (i = 0; i < 1024; i++)
    {
        asm ("");
    }

    ithCANSWReset(base, false);

    ithCANDisableIntrAll(base);

    if (base->InterruptTable & 0x0001)
    {
        ithCANEnableIntr(base, Error_Enable);
    }

    if (base->InterruptTable & 0x0004)
    {
        ithCANEnableIntr(base, TP_Enable);
    }

    if (base->InterruptTable & 0x0002)
    {
        ithCANEnableIntr(base, TS_Enable);
    }

    if (base->InterruptTable & 0x0008)
    {
        ithCANEnableIntr(base, RB_Almost_Full_Enable);
    }

    if (base->InterruptTable & 0x0010)
    {
        ithCANEnableIntr(base, RB_Full_Enable);
    }

    if (base->InterruptTable & 0x0040)
    {
        ithCANEnableIntr(base, Receive_Enable);
    }

    if (base->InterruptTable & 0x0020)
    {
        ithCANEnableIntr(base, RB_Overrun_Enable);
    }

    if (base->InterruptTable & 0x0080)
    {
        ithCANEnableIntr(base, Bus_Error_Enable);
    }

    if (base->InterruptTable & 0x0100)
    {
        ithCANEnableIntr(base, Arbitration_Lost_Enable);
    }

    if (base->InterruptTable & 0x0200)
    {
        ithCANEnableIntr(base, Error_Passive_Enable);
    }
}

static void
_CANSetINTConfig (
    CAN_HANDLE  * base,
    uint32_t    interrupts)
{
    ithCANDisableIntrAll(base);

    if (interrupts & Error_INT)
    {
        ithCANEnableIntr(base, Error_Enable);
    }

    if (interrupts & TP_INT)
    {
        ithCANEnableIntr(base, TP_Enable);
    }

    if (interrupts & TS_INT)
    {
        ithCANEnableIntr(base, TS_Enable);
    }

    if (interrupts & RB_Almost_Full_INT)
    {
        ithCANEnableIntr(base, RB_Almost_Full_Enable);
    }

    if (interrupts & RB_Full_INT)
    {
        ithCANEnableIntr(base, RB_Full_Enable);
    }

    if (interrupts & Receive_INT)
    {
        ithCANEnableIntr(base, Receive_Enable);
    }

    if (interrupts & RB_Overrun_INT)
    {
        ithCANEnableIntr(base, RB_Overrun_Enable);
    }

    if (interrupts & Bus_Error_INT)
    {
        ithCANEnableIntr(base, Bus_Error_Enable);
    }

    if (interrupts & Arbitration_Lost_INT)
    {
        ithCANEnableIntr(base, Arbitration_Lost_Enable);
    }

    if (interrupts & Error_Passive_INT)
    {
        ithCANEnableIntr(base, Error_Passive_Enable);
    }
}

static void
_CAN_CONFIG_20M_Nominal (
    CAN_HANDLE  * base,
    CAN_BTATTR  * s_bt)
{
    switch (base->BaudRate)
    {
        case CAN_20K:
            s_bt->Prescaler = 4 - 1;
            s_bt->Bit_Time  = 250;
            s_bt->Seg_1     = 200 - 2;
            s_bt->Seg_2     = 50 - 1;
            s_bt->SJW       = 50 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_50K:
            s_bt->Prescaler = 2 - 1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_100K:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_125K_500K:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 160;
            s_bt->Seg_1     = 128 - 2;
            s_bt->Seg_2     = 32 - 1;
            s_bt->SJW       = 32 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_250K_500K:
        case CAN_250K_1M:
        case CAN_250K_1M5:
        case CAN_250K_2M:
        case CAN_250K_4M:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 80;
            s_bt->Seg_1     = 64 - 2;
            s_bt->Seg_2     = 16 - 1;
            s_bt->SJW       = 16 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_500K_1M:
        case CAN_500K_2M:
        case CAN_500K_4M:
        case CAN_500K_5M:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 40;
            s_bt->Seg_1     = 32 - 2;
            s_bt->Seg_2     = 8 - 1;
            s_bt->SJW       = 8 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_1000K_4M:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 20;
            s_bt->Seg_1     = 16 - 2;
            s_bt->Seg_2     = 4 - 1;
            s_bt->SJW       = 4 - 1;
            s_bt->SSPOFF    = 0;
            break;

        default:
            break;
    }
}

static void
_CAN_CONFIG_20M_Data (
    CAN_HANDLE  * base,
    CAN_BTATTR  * f_bt)
{
    switch (base->BaudRate)
    {
        case CAN_125K_500K:
        case CAN_250K_500K:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 40;
            f_bt->Seg_1     = 32 - 2;
            f_bt->Seg_2     = 8 - 1;
            f_bt->SJW       = 8 - 1;
            f_bt->SSPOFF    = 32;
            break;

        case CAN_250K_1M:
        case CAN_500K_1M:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 20;
            f_bt->Seg_1     = 16 - 2;
            f_bt->Seg_2     = 4 - 1;
            f_bt->SJW       = 4 - 1;
            f_bt->SSPOFF    = 16;
            break;

        case CAN_250K_1M5:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 13;
            f_bt->Seg_1     = 10 - 2;
            f_bt->Seg_2     = 3 - 1;
            f_bt->SJW       = 3 - 1;
            f_bt->SSPOFF    = 10;
            break;

        case CAN_250K_2M:
        case CAN_500K_2M:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 10;
            f_bt->Seg_1     = 8 - 2;
            f_bt->Seg_2     = 2 - 1;
            f_bt->SJW       = 2 - 1;
            f_bt->SSPOFF    = 8;
            break;

        case CAN_250K_4M:
        case CAN_500K_4M:
        case CAN_1000K_4M:
            f_bt->Prescaler = 0;    // 1-1;
            f_bt->Bit_Time  = 5;
            f_bt->Seg_1     = 4 - 2;
            f_bt->Seg_2     = 0;    // 1-1;
            f_bt->SJW       = 0;    // 1-1;
            f_bt->SSPOFF    = 4;
            break;

        case CAN_500K_5M:           // sample point 75%
            f_bt->Prescaler = 0;    // 1-1;
            f_bt->Bit_Time  = 4;
            f_bt->Seg_1     = 3 - 2;
            f_bt->Seg_2     = 0;    // 1-1;
            f_bt->SJW       = 0;    // 1-1;
            f_bt->SSPOFF    = 3;
            break;

        default:
            break;
    }
}

static void
_CAN_CONFIG_40M_Nominal (
    CAN_HANDLE  * base,
    CAN_BTATTR  * s_bt)
{
    switch (base->BaudRate)
    {
        case CAN_20K:
            s_bt->Prescaler = 8 - 1;
            s_bt->Bit_Time  = 250;
            s_bt->Seg_1     = 200 - 2;
            s_bt->Seg_2     = 50 - 1;
            s_bt->SJW       = 50 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_50K:
            s_bt->Prescaler = 4 - 1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_100K:
            s_bt->Prescaler = 2 - 1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_125K_500K:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 320;
            s_bt->Seg_1     = 256 - 2;
            s_bt->Seg_2     = 64 - 1;
            s_bt->SJW       = 64 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_250K_500K:
        case CAN_250K_1M:
        case CAN_250K_1M5:
        case CAN_250K_2M:
        case CAN_250K_4M:
            s_bt->Prescaler = 2 - 1;
            s_bt->Bit_Time  = 80;
            s_bt->Seg_1     = 64 - 2;
            s_bt->Seg_2     = 16 - 1;
            s_bt->SJW       = 16 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_500K_1M:
        case CAN_500K_2M:
        case CAN_500K_4M:
        case CAN_500K_5M:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 80;
            s_bt->Seg_1     = 64 - 2;
            s_bt->Seg_2     = 16 - 1;
            s_bt->SJW       = 16 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_1000K_4M:
            s_bt->Prescaler = 0; // 1-1;
            s_bt->Bit_Time  = 40;
            s_bt->Seg_1     = 32 - 2;
            s_bt->Seg_2     = 8 - 1;
            s_bt->SJW       = 8 - 1;
            s_bt->SSPOFF    = 0;
            break;

        default:
            break;
    }
}

static void
_CAN_CONFIG_40M_Data (
    CAN_HANDLE  * base,
    CAN_BTATTR  * f_bt)
{
    switch (base->BaudRate)
    {
        case CAN_125K_500K:
        case CAN_250K_500K:
            f_bt->Prescaler = 2 - 1;
            f_bt->Bit_Time  = 40;
            f_bt->Seg_1     = 32 - 2;
            f_bt->Seg_2     = 8 - 1;
            f_bt->SJW       = 8 - 1;
            f_bt->SSPOFF    = 32;
            break;

        case CAN_250K_1M:
        case CAN_500K_1M:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 40;
            f_bt->Seg_1     = 32 - 2;
            f_bt->Seg_2     = 8 - 1;
            f_bt->SJW       = 8 - 1;
            f_bt->SSPOFF    = 32;
            break;

        case CAN_250K_1M5:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 26;
            f_bt->Seg_1     = 20 - 2;
            f_bt->Seg_2     = 6 - 1;
            f_bt->SJW       = 6 - 1;
            f_bt->SSPOFF    = 20;
            break;

        case CAN_250K_2M:
        case CAN_500K_2M:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 20;
            f_bt->Seg_1     = 16 - 2;
            f_bt->Seg_2     = 4 - 1;
            f_bt->SJW       = 4 - 1;
            f_bt->SSPOFF    = 16;
            break;

        case CAN_250K_4M:
        case CAN_500K_4M:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 10;
            f_bt->Seg_1     = 8 - 2;
            f_bt->Seg_2     = 2 - 1;
            f_bt->SJW       = 2 - 1;
            f_bt->SSPOFF    = 8;
            break;

        case CAN_500K_5M:           // sample point 75%
            f_bt->Prescaler = 0;    // 1-1;
            f_bt->Bit_Time  = 8;
            f_bt->Seg_1     = 6 - 2;
            f_bt->Seg_2     = 2 - 1;
            f_bt->SJW       = 2 - 1;
            f_bt->SSPOFF    = 6;
            break;

        case CAN_1000K_4M:
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 10;
            f_bt->Seg_1     = 8 - 2;
            f_bt->Seg_2     = 2 - 1;
            f_bt->SJW       = 2 - 1;
            f_bt->SSPOFF    = 8;
            break;

        default:
            break;
    }
}

static void
_CAN_CONFIG_60M_Nominal (
    CAN_HANDLE  * base,
    CAN_BTATTR  * s_bt)
{
    switch (base->BaudRate)
    {
        case CAN_20K:
            s_bt->Prescaler = 12 - 1;
            s_bt->Bit_Time  = 250;
            s_bt->Seg_1     = 200 - 2;
            s_bt->Seg_2     = 50 - 1;
            s_bt->SJW       = 50 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_50K:
            s_bt->Prescaler = 6 - 1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_100K:
            s_bt->Prescaler = 3 - 1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_125K_500K:
            s_bt->Prescaler = 3 - 1;
            s_bt->Bit_Time  = 160;
            s_bt->Seg_1     = 128 - 2;
            s_bt->Seg_2     = 32 - 1;
            s_bt->SJW       = 32 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_250K_500K:
        case CAN_250K_1M:
        case CAN_250K_1M5:
        case CAN_250K_2M:
        case CAN_250K_4M:
            s_bt->Prescaler = 3 - 1;
            s_bt->Bit_Time  = 80;
            s_bt->Seg_1     = 64 - 2;
            s_bt->Seg_2     = 16 - 1;
            s_bt->SJW       = 16 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_500K_1M:
        case CAN_500K_2M:
        case CAN_500K_4M:
        case CAN_500K_5M:
            s_bt->Prescaler = 3 - 1;
            s_bt->Bit_Time  = 40;
            s_bt->Seg_1     = 32 - 2;
            s_bt->Seg_2     = 8 - 1;
            s_bt->SJW       = 8 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_1000K_4M:
            s_bt->Prescaler = 3 - 1;
            s_bt->Bit_Time  = 20;
            s_bt->Seg_1     = 16 - 2;
            s_bt->Seg_2     = 4 - 1;
            s_bt->SJW       = 4 - 1;
            s_bt->SSPOFF    = 0;
            break;

        default:
            break;
    }
}

static void
_CAN_CONFIG_80M_Nominal (
    CAN_HANDLE  * base,
    CAN_BTATTR  * s_bt)
{
    switch (base->BaudRate)
    {
        case CAN_20K:
            s_bt->Prescaler = 16 - 1;
            s_bt->Bit_Time  = 250;
            s_bt->Seg_1     = 200 - 2;
            s_bt->Seg_2     = 50 - 1;
            s_bt->SJW       = 50 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_50K:
            s_bt->Prescaler = 8 - 1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_100K:
            s_bt->Prescaler = 4 - 1;
            s_bt->Bit_Time  = 200;
            s_bt->Seg_1     = 160 - 2;
            s_bt->Seg_2     = 40 - 1;
            s_bt->SJW       = 40 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_125K_500K:
            s_bt->Prescaler = 2 - 1;
            s_bt->Bit_Time  = 320;
            s_bt->Seg_1     = 256 - 2;
            s_bt->Seg_2     = 64 - 1;
            s_bt->SJW       = 64 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_250K_500K:
        case CAN_250K_1M:
        case CAN_250K_1M5:
        case CAN_250K_2M:
        case CAN_250K_4M:
            s_bt->Prescaler = 4 - 1;
            s_bt->Bit_Time  = 80;
            s_bt->Seg_1     = 64 - 2;
            s_bt->Seg_2     = 16 - 1;
            s_bt->SJW       = 16 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_500K_1M:
        case CAN_500K_2M:
        case CAN_500K_4M:
        case CAN_500K_5M:
            s_bt->Prescaler = 2 - 1;
            s_bt->Bit_Time  = 80;
            s_bt->Seg_1     = 64 - 2;
            s_bt->Seg_2     = 16 - 1;
            s_bt->SJW       = 16 - 1;
            s_bt->SSPOFF    = 0;
            break;

        case CAN_1000K_4M:
            s_bt->Prescaler = 2 - 1;
            s_bt->Bit_Time  = 40;
            s_bt->Seg_1     = 32 - 2;
            s_bt->Seg_2     = 8 - 1;
            s_bt->SJW       = 8 - 1;
            s_bt->SSPOFF    = 0;
            break;

        default:
            break;
    }
}

static void
_CAN_CONFIG_80M_Data (
    CAN_HANDLE  * base,
    CAN_BTATTR  * f_bt)
{
    switch (base->BaudRate)
    {
        case CAN_125K_500K:
        case CAN_250K_500K:
            f_bt->Prescaler = 4 - 1;
            f_bt->Bit_Time  = 40;
            f_bt->Seg_1     = 32 - 2;
            f_bt->Seg_2     = 8 - 1;
            f_bt->SJW       = 8 - 1;
            f_bt->SSPOFF    = 32;
            break;

        case CAN_250K_1M:
        case CAN_500K_1M:
            f_bt->Prescaler = 2 - 1;
            f_bt->Bit_Time  = 40;
            f_bt->Seg_1     = 32 - 2;
            f_bt->Seg_2     = 8 - 1;
            f_bt->SJW       = 8 - 1;
            f_bt->SSPOFF    = 32;
            break;

        case CAN_250K_1M5:
            f_bt->Prescaler = 2 - 1;
            f_bt->Bit_Time  = 26;
            f_bt->Seg_1     = 20 - 2;
            f_bt->Seg_2     = 6 - 1;
            f_bt->SJW       = 6 - 1;
            f_bt->SSPOFF    = 20;
            break;

        case CAN_250K_2M:
        case CAN_500K_2M:
            f_bt->Prescaler = 2 - 1;
            f_bt->Bit_Time  = 20;
            f_bt->Seg_1     = 16 - 2;
            f_bt->Seg_2     = 4 - 1;
            f_bt->SJW       = 4 - 1;
            f_bt->SSPOFF    = 16;
            break;

        case CAN_250K_4M:
        case CAN_500K_4M:
            f_bt->Prescaler = 2 - 1;
            f_bt->Bit_Time  = 10;
            f_bt->Seg_1     = 8 - 2;
            f_bt->Seg_2     = 2 - 1;
            f_bt->SJW       = 2 - 1;
            f_bt->SSPOFF    = 8;
            break;

        case CAN_500K_5M:           // sample point 75%
            f_bt->Prescaler = 0;    // 1-1;
            f_bt->Bit_Time  = 16;
            f_bt->Seg_1     = 12 - 2;
            f_bt->Seg_2     = 4 - 1;
            f_bt->SJW       = 4 - 1;
            f_bt->SSPOFF    = 12;
            break;

        case CAN_1000K_4M:
            f_bt->Prescaler = 2 - 1;
            f_bt->Bit_Time  = 10;
            f_bt->Seg_1     = 8 - 2;
            f_bt->Seg_2     = 2 - 1;
            f_bt->SJW       = 2 - 1;
            f_bt->SSPOFF    = 8;
            break;

        default:
            break;
    }
}

static void
_CAN_Bit_Timing_Calculator (
    CAN_HANDLE  * base,
    CAN_BTATTR  * s_bt,
    CAN_BTATTR  * f_bt)
{
    switch (base->SourceClock)
    {
        case CAN_SRCCLK_20M:
            _CAN_CONFIG_20M_Nominal(base, s_bt);
            break;

        case CAN_SRCCLK_40M:
            _CAN_CONFIG_40M_Nominal(base, s_bt);
            break;

        case CAN_SRCCLK_60M:
            _CAN_CONFIG_60M_Nominal(base, s_bt);
            break;

        case CAN_SRCCLK_80M:
            _CAN_CONFIG_80M_Nominal(base, s_bt);
            break;

        default:
            break;
    }
#if CFG_CANBUS_FD_ENABLE
    switch (base->SourceClock)
    {
        case CAN_SRCCLK_20M:
            _CAN_CONFIG_20M_Data(base, f_bt);
            break;

        case CAN_SRCCLK_40M:
            _CAN_CONFIG_40M_Data(base, f_bt);
            break;

        case CAN_SRCCLK_80M:
            _CAN_CONFIG_80M_Data(base, f_bt);
            break;

        default:
            break;
    }
#endif
}

// =============================================================================
//                Public Function Definition
// =============================================================================
void
ithCANOpen (
    CAN_HANDLE * base)
{
    CAN_BTATTR  s_bt    = {0};
    CAN_BTATTR  f_bt    = {0};

    if ((base == NULL) || (base->Instance != 1))
    {
        return;
    }

    // Set Reset ON
    ithCANSWReset(base, true);

#if CFG_CANBUS_FD_ENABLE
    ithCANSetFDISO(base);
#endif

    if (base->BaudRate != CAN_USER_DEFINED)
    {
        _CAN_Bit_Timing_Calculator(base, &s_bt, &f_bt);
        // BitRate setting
        ithCANSetBitRate(base, s_bt, f_bt);
    }
    else
    {
        // BitRate setting
        ithCANSetBitRate(base, base->SlowBitRate, base->FastBitRate);
    }

    // Filter enable setting
    for (int i = 0; i < 16; i++)
    {
        ithCANACFEnable(base, i, base->TPtr[i].FilterEnable);
        ithCANSetACODE(base, i, base->TPtr[i].ACode);
        ithCANSetAMASK(base, i, base->TPtr[i].AMask,  base->TPtr[i].AIDEE,  base->TPtr[i].AIDE);
    }

    // sw reset stop
    ithCANSWReset(base, false);

    // set stb default mode
    ithCANSetSTBMode(base, 0x0);

    // LoopBack Mode Setting (for self test)
    ithCANSetLoopBack(base, base->ExternalLoopBackMode, base->InternalLoopBackMode);

    // Listen Only Mode Setting
    ithCANSetListenOnlyMode(base, base->ListenOnlyMode);

    // Trans Standby Mode Setting
    ithCANSetTransStandbyMode(base, 0x0);

    ithTTCANCrtlOFF(base);

    _CANSetINTConfig(base, base->InterruptTable);

    switch (base->SourceClock)
    {
        case CAN_SRCCLK_20M:
            ithCANSetTimeStampDiv(base, 0x4E20);
            break;

        case CAN_SRCCLK_40M:
            ithCANSetTimeStampDiv(base, 0x9C40);
            break;

        case CAN_SRCCLK_60M:
            ithCANSetTimeStampDiv(base, 0xEA60);
            break;

        case CAN_SRCCLK_80M:
            ithCANSetTimeStampDiv(base, 0x13880);
            break;

        default:
            ithCANSetTimeStampDiv(base, 0x1);
            break;
    }

    // open time stamp
    ithCANSetCIA603(base, true, 0x0);
}

int
ithCANClose (
    CAN_HANDLE * base)
{
    int i = 0;

    if ((base == NULL) || (base->Instance != 1))
    {
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    ithCANDisableIntrAll(base);
    // sw Reset
    ithCANSWReset(base, true);

    for (i = 0; i < 1024; i++)
    {
        asm ("");
    }

    ithCANSWReset(base, false);
    // ListenOnlyMode
    ithCANSetListenOnlyMode(base, true);

    return CAN_SUCCESS;
}

int
ithCANRead (
    CAN_HANDLE  * base,
    CAN_RXOBJ   * info)
{
    int result = CAN_SUCCESS;

    if ((base == NULL) || (base->Instance != 1))
    {
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    // Check RBUF status
    if (ithCANGetRBStatus(base) != BUFF_EMPTY)
    {
        CAN_RXOBJ   RX          = {0};
        uint32_t    i           = 0;
        uint32_t    lenof4byte  = 0;
        uint32_t    control     = 0;
        uint32_t    id          = 0;
        // Read control info
        control         = ithReadRegA((base->ADDR | CAN_RB_CONTROL_REG));
        RX.Control.DLC  = control & CAN_RB_DLC;
        RX.Control.BRS  = (control & CAN_RB_BRS) >> CAN_RB_BRS_SHIFT;
        RX.Control.EDL  = (control & CAN_RB_EDL) >> CAN_RB_EDL_SHIFT;
        RX.Control.RTR  = (control & CAN_RB_RTR) >> CAN_RB_RTR_SHIFT;
        RX.Control.IDE  = (control & CAN_RB_IDE) >> CAN_RB_IDE_SHIFT;
        RX.Status.TX    = (control & CAN_RB_TX) >> CAN_RB_TX_SHIFT;
        RX.Status.KOER  = (control & CAN_RB_KOER) >> CAN_RB_KOER_SHIFT;
        // Read identifier
        id              = ithReadRegA((base->ADDR | CAN_RB_IDENTIFIER_REG));

        RX.Identifier   = (RX.Control.IDE ? (id & 0x1FFFFFFF) : (id & N11_BITS_MSK));
#if CFG_CANBUS_FD_ENABLE
        RX.ESI          = (id >> 31) & N01_BITS_MSK;
#endif
        if (RX.Status.KOER == CAN_NO_ERROR)
        {
            lenof4byte = alignment4byte_(ithCANDlcToBytes(RX.Control.DLC));
            // Copy data from RBuffer reg
            for (i = 0; i < lenof4byte; i++)
            {
                unsigned int tmp = ithReadRegA(base->ADDR | (CAN_RB_DATA0_REG + i * 4));
                memcpy(&RX.RXData[i * 4], &tmp, sizeof(uint8_t) * 4);
            }
            // time stamp(CIA603)
            RX.RXRTS[0] = ithReadRegA(base->ADDR | CAN_RB_RTS0_REG);
            RX.RXRTS[1] = ithReadRegA(base->ADDR | CAN_RB_RTS1_REG);

            memcpy(info, &RX, sizeof(CAN_RXOBJ));
        }
        else
        {
            result = CAN_RX_STATUS_ERROR;
        }

        // Release this RXbuf
        ithWriteRegMaskA((base->ADDR | CAN_CFG_STAT_REG), CAN_RREL, (CAN_RREL | CAN_BUSOFF));
    }
    else
    {
        result = CAN_RX_FIFO_EMPTY;
    }

    // Return code
    return result;
}

int
ithCANWrite (
    CAN_HANDLE  * base,
    CAN_TXOBJ   * info,
    uint8_t     * dataptr)
{
    uint8_t     txbuffer[DLC_MAX]   = {0};
    uint32_t    * txptr;
    uint32_t    control             = 0;
    uint32_t    lenNumBytes         = 0;
    uint32_t    lenNumWords         = 0;
    uint8_t     i                   = 0;
    uint32_t    start_tick          = 0;

    if ((base == NULL) || (base->Instance != 1))
    {
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    if (!_CANSetPTBWritePtr(base, info))
    {
        return CAN_TX_FIFO_FULL_ERROR;
    }

    // Set One Shot mode
    if (info->SingleShot)
    {
        ithCANSetTransPTBSSMode(base, 0x1);
    }
    else
    {
        ithCANSetTransPTBSSMode(base, 0x0);
    }

    // Set TB IDENTIFIER
    if (info->Control.IDE)
    {
        ithWriteRegA((base->ADDR | CAN_TB_IDENTIFIER_REG), (info->Identifier & 0x1FFFFFFF) | info->TTSENSEL << 31);
    }
    else
    {
        ithWriteRegA((base->ADDR | CAN_TB_IDENTIFIER_REG), (info->Identifier & 0x7FF) | info->TTSENSEL << 31);
    }

    // Set TB Control
    control |= (info->Control.DLC & 0xF);   // Set payload  length(DLC)
    control |= info->Control.BRS << CAN_TB_BRS_SHIFT;
    control |= info->Control.EDL << CAN_TB_EDL_SHIFT;
    control |= info->Control.RTR << CAN_TB_RTR_SHIFT;
    control |= info->Control.IDE << CAN_TB_IDE_SHIFT;
    ithWriteRegA((base->ADDR | CAN_TB_CONTROL_REG), control);

    lenNumBytes = ithCANDlcToBytes(info->Control.DLC);
    // Copy data to txbuffer
    memcpy(&txbuffer, dataptr, sizeof(uint8_t) * lenNumBytes);

    // Bytes to Words
    lenNumWords = alignment4byte_(lenNumBytes);

    for (i = 0; i < lenNumWords && i < (DLC_MAX / 4); i++)
    {
        txptr = (uint32_t *)&txbuffer[i * 4];
        ithWriteRegA((base->ADDR | (CAN_TB_DATA0_REG + i * 4)), *txptr);
    }

    // CAN ctrl send Msg to bus
    ithCANPrimarySendCtrl(base, 1);

    // Return write success
    return CAN_SUCCESS;
}

int
ithCANFIFOUpdate (
    CAN_HANDLE  * base,
    CAN_TXOBJ   * info,
    uint8_t     * dataptr)
{
    return CAN_SUCCESS;
}

int
ithCANFIFOWrite (
    CAN_HANDLE  * base,
    CAN_TXOBJ   * info)
{
    return CAN_SUCCESS;
}

int
ithCANEnterListOnlyMode (
    CAN_HANDLE * base)
{
    return CAN_SUCCESS;
}

int
ithCANLeaveListOnlyMode (
    CAN_HANDLE * base)
{
    return CAN_SUCCESS;
}

int
ithCANGetTTS (
    CAN_HANDLE * base)
{
    return (int)ithCANGetTransTimeStamp(base);
}

void
ithCANReset (
    CAN_HANDLE * base)
{
    _CANExcHandler(base);
}

CAN_ERROR_STATE
ithCANGetErrorState (
    CAN_HANDLE * base)
{
    CAN_ERROR_STATE state = ERROR_ACTIVE;

    if (ithCANBUSState(base) == 0x1)
    {
        state = BUS_OFF;
    }
    else if (ithCANERRORState(base) == 0x1)
    {
        state = ERROR_PASSIVE;
    }
    return state;
}

uint32_t
ithCANGetREC (
    CAN_HANDLE * base)
{
    return ithCANGetReceiveErrorCouNT(base);
}

uint32_t
ithCANGetTEC (
    CAN_HANDLE * base)
{
    return ithCANGetTransmitErrorCouNT(base);
}

CAN_ERROR_TYPE
ithCANGetKOER (
    CAN_HANDLE * base)
{
    return ithCANGetKindOfError(base);
}

uint32_t
ithCANDlcToBytes (
    CAN_DLC dlc)
{
    uint32_t dataBytesInObject = 0;

    if (dlc <= CAN_DLC_8)
    {
        dataBytesInObject = dlc;
    }
#if CFG_CANBUS_FD_ENABLE
    else
    {
        switch (dlc)
        {
            case CAN_DLC_12:
                dataBytesInObject = 12;
                break;

            case CAN_DLC_16:
                dataBytesInObject = 16;
                break;

            case CAN_DLC_20:
                dataBytesInObject = 20;
                break;

            case CAN_DLC_24:
                dataBytesInObject = 24;
                break;

            case CAN_DLC_32:
                dataBytesInObject = 32;
                break;

            case CAN_DLC_48:
                dataBytesInObject = 48;
                break;

            case CAN_DLC_64:
                dataBytesInObject = 64;
                break;

            default:
                break;
        }
    }
#endif

    return dataBytesInObject;
}
