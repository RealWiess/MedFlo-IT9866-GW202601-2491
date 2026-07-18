#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include "can_api.h"
#include "can_hw.h"

//=============================================================================
//                Constant Definition
//=============================================================================
#define SW_WORKAROUND_ENABLE 1
//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================
static pthread_mutex_t internal_mutex[2] = {PTHREAD_MUTEX_INITIALIZER,
                                            PTHREAD_MUTEX_INITIALIZER};
static pthread_mutex_t can_general_mutex = PTHREAD_MUTEX_INITIALIZER;

static void (*can_user_cbfunction[2])(void);
#if SW_WORKAROUND_ENABLE
sem_t can_ptb_sem[2];
sem_t can_stb_sem[2];
sem_t can_aif_sem[2];
#endif
static bool     can_clk_on    = false;   // can clk enable or disable
static bool     can_opened[2] = {false}; // can opened or closed
//=============================================================================
//                Private Function Definition
//=============================================================================
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

static bool can_set_ptb_wrptr_ (CAN_HANDLE * base, CAN_TXOBJ * info)
{

    /*Set TBSEL to PTB or STB*/
    ithWriteRegMaskA((base->ADDR | CAN_CFG_STAT_REG), 0x0UL << CAN_TBSEL_SHIFT,
                     (CAN_TBSEL | CAN_BUSOFF));

    if (ithCANGetTBStatus(base, false) == BUFF_FULL)
    {
#if 0
        (void)printf("[MSG]CAN tx ptb buffer full\n");
#endif
        return false;
    }

    return true;
}
static bool can_set_stb_wrptr_ (CAN_HANDLE * base, CAN_TXOBJ * info)
{

    /*Set TBSEL to STB*/
    ithWriteRegMaskA((base->ADDR | CAN_CFG_STAT_REG), 0x1UL << CAN_TBSEL_SHIFT,
                     (CAN_TBSEL | CAN_BUSOFF));

    if (ithCANGetTBStatus(base, true) == BUFF_FULL)
    {
#if 0
        (void)printf("[MSG]CAN tx stb buffer full\n");
#endif
        return false;
    }

    return true;
}

static void can_intr_handler_ (void * arg)
{
    CAN_HANDLE * base       = (CAN_HANDLE *)arg;
    uint32_t     flagstatus = ithCANGetIntrFlag(base);

#if SW_WORKAROUND_ENABLE
    if (flagstatus & CAN_TPIF) /*Transmission Primary Interrupt Flag*/
    {
        sem_post(&can_ptb_sem[base->Instance]);
    }

    if (flagstatus & CAN_TSIF) /*Transmission Secondary Interrupt Flag*/
    {
        sem_post(&can_stb_sem[base->Instance]);
    }

    if (flagstatus & CAN_AIF) /*Abort Interrupt Flag*/
    {
        sem_post(&can_aif_sem[base->Instance]);
    }
#endif

    /*Call user callback.*/
    if (can_user_cbfunction[base->Instance] != NULL)
    {
        (*can_user_cbfunction[base->Instance])();
    }
}
#if 0
static void TTCANIntrHandler_(void *arg)
{
    CAN_HANDLE *base      = (CAN_HANDLE *) arg;
    uint32_t   flagstatus = ithTTCANGetIntrFlag(base);

    if (flagstatus & CAN_TTIF)//Time Trigger interrupt Flag
    {
        ithCANClearIntrFlag(base, Time_Trigger_Interrupt_Flag);
    }

    if (flagstatus & CAN_TEIF)
    {
        ithCANClearIntrFlag(base, Trigger_Error_Interrupt_Flag);
    }
}
#endif

static void can_exc_handler_ (CAN_HANDLE * base)
{
    ithCANSWReset(base, true);  /*sw reset on*/
    ithDelay(10);
    ithCANSWReset(base, false); /*sw reset off*/

    ithCANDisableIntrAll(base);

    if (base->InterruptTable & 0x0001U)
    {
        ithCANEnableIntr(base, Error_Enable);
    }

    if (base->InterruptTable & 0x0004U)
    {
        ithCANEnableIntr(base, TP_Enable);
    }

    if (base->InterruptTable & 0x0002U)
    {
        ithCANEnableIntr(base, TS_Enable);
    }

    if (base->InterruptTable & 0x0008U)
    {
        ithCANEnableIntr(base, RB_Almost_Full_Enable);
    }

    if (base->InterruptTable & 0x0010U)
    {
        ithCANEnableIntr(base, RB_Full_Enable);
    }

    if (base->InterruptTable & 0x0040U)
    {
        ithCANEnableIntr(base, Receive_Enable);
    }

    if (base->InterruptTable & 0x0020U)
    {
        ithCANEnableIntr(base, RB_Overrun_Enable);
    }

    if (base->InterruptTable & 0x0080U)
    {
        ithCANEnableIntr(base, Bus_Error_Enable);
    }

    if (base->InterruptTable & 0x0100U)
    {
        ithCANEnableIntr(base, Arbitration_Lost_Enable);
    }

    if (base->InterruptTable & 0x0200U)
    {
        ithCANEnableIntr(base, Error_Passive_Enable);
    }
}

static void can_set_int_config_ (CAN_HANDLE * base, uint32_t interrupts)
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
static void can_enable_irq_ (CAN_HANDLE * base, bool OnOff,
                             ITHIntrHandler handler)
{
    if (base->Instance == 0U)
    {
        ithIntrDisableIrq(ITH_INTR_CANBUS0);
        ithIntrClearIrq(ITH_INTR_CANBUS0);
        if (OnOff)
        {
            ithIntrSetTriggerModeIrq(ITH_INTR_CANBUS0, ITH_INTR_LEVEL);
            ithIntrRegisterHandlerIrq(ITH_INTR_CANBUS0, handler, (void *)base);
            /*Enable can0 the interrupts.*/
            ithIntrEnableIrq(ITH_INTR_CANBUS0);
        }
    }
    else
    {
        ithIntrDisableIrq(ITH_INTR_CANBUS1);
        ithIntrClearIrq(ITH_INTR_CANBUS1);
        if (OnOff)
        {
            ithIntrSetTriggerModeIrq(ITH_INTR_CANBUS1, ITH_INTR_LEVEL);
            ithIntrRegisterHandlerIrq(ITH_INTR_CANBUS1, handler, (void *)base);
            /*Enable can1 the interrupts.*/
            ithIntrEnableIrq(ITH_INTR_CANBUS1);
        }
    }
}

static void can_config_20M_nominal_ (CAN_HANDLE * base, CAN_BTATTR * s_bt)
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

#if CFG_CANBUS_FD_ENABLE
static void can_config_20M_data_ (CAN_HANDLE * base, CAN_BTATTR * f_bt)
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
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 5;
            f_bt->Seg_1     = 4 - 2;
            f_bt->Seg_2     = 0; // 1-1;
            f_bt->SJW       = 0; // 1-1;
            f_bt->SSPOFF    = 4;
            break;
        case CAN_500K_5M:        // sample point 75%
            f_bt->Prescaler = 0; // 1-1;
            f_bt->Bit_Time  = 4;
            f_bt->Seg_1     = 3 - 2;
            f_bt->Seg_2     = 0; // 1-1;
            f_bt->SJW       = 0; // 1-1;
            f_bt->SSPOFF    = 3;
            break;
        default:
            CANPRINT("[MSG]CAN FD no support\n");
            break;
    }
}
#endif

static void can_config_40M_nominal_ (CAN_HANDLE * base, CAN_BTATTR * s_bt)
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

#if CFG_CANBUS_FD_ENABLE
static void can_config_40M_data_ (CAN_HANDLE * base, CAN_BTATTR * f_bt)
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
        case CAN_500K_5M:        // sample point 75%
            f_bt->Prescaler = 0; // 1-1;
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
            CANPRINT("[MSG]CAN FD no support\n");
            break;
    }
}
#endif

static void can_config_60M_nominal_ (CAN_HANDLE * base, CAN_BTATTR * s_bt)
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

static void can_config_80M_nominal_ (CAN_HANDLE * base, CAN_BTATTR * s_bt)
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

#if CFG_CANBUS_FD_ENABLE
static void can_config_80M_data_ (CAN_HANDLE * base, CAN_BTATTR * f_bt)
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
        case CAN_500K_5M:        // sample point 75%
            f_bt->Prescaler = 0; // 1-1;
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
            CANPRINT("[MSG]CAN FD no support\n");
            break;
    }
}
#endif

static void can_bit_timing_calculator_ (CAN_HANDLE * base, CAN_BTATTR * s_bt,
                                        CAN_BTATTR * f_bt)
{
    switch (base->SourceClock)
    {
        case CAN_SRCCLK_20M:
            can_config_20M_nominal_(base, s_bt);
            break;
        case CAN_SRCCLK_40M:
            can_config_40M_nominal_(base, s_bt);
            break;
        case CAN_SRCCLK_60M:
            can_config_60M_nominal_(base, s_bt);
            break;
        case CAN_SRCCLK_80M:
            can_config_80M_nominal_(base, s_bt);
            break;
        default:
            CANPRINT("[MSG]CAN no support this SRCCLK!!!!\n");
            break;
    }
    (void)printf(
        "[MSG]CAN [S]Prescaler = %" PRIu32 ", Bit_Time = %" PRIu32
        ", Seg1 = %" PRIu32 ", Seg2 = %" PRIu32 ", SJW = %" PRIu32 "\n",
        s_bt->Prescaler, s_bt->Bit_Time, s_bt->Seg_1, s_bt->Seg_2, s_bt->SJW);

#if CFG_CANBUS_FD_ENABLE
    switch (base->SourceClock)
    {
        case CAN_SRCCLK_20M:
            can_config_20M_data_(base, f_bt);
            (void)printf("[MSG]CAN [F]Prescaler = %d, Bit_Time = %d, Seg1 = "
                         "%d, Seg2 = %d, SJW = %d, SSPOFF=%d\n",
                         f_bt->Prescaler, f_bt->Bit_Time, f_bt->Seg_1,
                         f_bt->Seg_2, f_bt->SJW, f_bt->SSPOFF);
            break;

        case CAN_SRCCLK_40M:
            can_config_40M_data_(base, f_bt);
            (void)printf("[MSG]CAN [F]Prescaler = %d, Bit_Time = %d, Seg1 = "
                         "%d, Seg2 = %d, SJW = %d, SSPOFF=%d\n",
                         f_bt->Prescaler, f_bt->Bit_Time, f_bt->Seg_1,
                         f_bt->Seg_2, f_bt->SJW, f_bt->SSPOFF);
            break;
        case CAN_SRCCLK_80M:
            can_config_80M_data_(base, f_bt);
            (void)printf("[MSG]CAN [F]Prescaler = %d, Bit_Time = %d, Seg1 = "
                         "%d, Seg2 = %d, SJW = %d, SSPOFF=%d\n",
                         f_bt->Prescaler, f_bt->Bit_Time, f_bt->Seg_1,
                         f_bt->Seg_2, f_bt->SJW, f_bt->SSPOFF);
            break;
        default:
            CANPRINT("[MSG]CAN FD no support this SRCCLK!!!\n");
            break;
    }
#endif
}

//=============================================================================
//                Public Function Definition
//=============================================================================
void ithCANOpen (CAN_HANDLE * base)
{

    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE))
    {
        CANPRINT("[MSG]CAN handle parameter incorrect\n");
        return;
    }

    /*check clk ready*/
    if (can_clk_on == false)
    {
        ithCANCLKEnable(1U);
    }
    pthread_mutex_lock(&internal_mutex[base->Instance]);

    CAN_BTATTR s_bt = {0};
    CAN_BTATTR f_bt = {0};

    if ((base->InterruptHD != NULL) && (base->Instance < CAN_MAX_INSTANCE))
    {
        can_user_cbfunction[base->Instance] = base->InterruptHD;
    }
    else
    {
        can_user_cbfunction[base->Instance] = NULL;
    }
    /*Set Reset ON*/
    ithCANSWReset(base, true);

#if CFG_CANBUS_FD_ENABLE
    ithCANSetFDISO(base);
#endif

    if (base->BaudRate != CAN_USER_DEFINED)
    {
        can_bit_timing_calculator_(base, &s_bt, &f_bt);
        /*BitRate setting*/
        ithCANSetBitRate(base, s_bt, f_bt);
    }
    else
    {
        (void)printf("[MSG]CAN [S]Prescaler = %" PRIu32 ", Bit_Time = %" PRIu32
                     ", Seg1 = %" PRIu32 ", Seg2 = %" PRIu32 ", SJW = %" PRIu32
                     "\n",
                     base->SlowBitRate.Prescaler, base->SlowBitRate.Bit_Time,
                     base->SlowBitRate.Seg_1, base->SlowBitRate.Seg_2,
                     base->SlowBitRate.SJW);
#if CFG_CANBUS_FD_ENABLE
        (void)printf("[MSG]CAN [S]Prescaler = %" PRIu32 ", Bit_Time = %" PRIu32
                     ", Seg1 = %" PRIu32 ", Seg2 = %" PRIu32 ", SJW = %" PRIu32
                     ", SSPOFF=%" PRIu32 "\n",
                     base->FastBitRate.Prescaler, base->FastBitRate.Bit_Time,
                     base->FastBitRate.Seg_1, base->FastBitRate.Seg_2,
                     base->FastBitRate.SJW, base->FastBitRate.SSPOFF);
#endif
        /*BitRate setting, user define*/
        ithCANSetBitRate(base, base->SlowBitRate, base->FastBitRate);
    }

    /*Filter enable setting*/
    for (int i = 0; i < 16; i++)
    {
        ithCANACFEnable(base, i, base->TPtr[i].FilterEnable);
        ithCANSetACODE(base, i, base->TPtr[i].ACode);
        ithCANSetAMASK(base, i, base->TPtr[i].AMask, base->TPtr[i].AIDEE,
                       base->TPtr[i].AIDE);
    }

    /*sw reset stop*/
    ithCANSWReset(base, false);

    /*set stb default mode*/
    ithCANSetSTBMode(base, false);

    /*LoopBack Mode Setting (for self test)*/
    ithCANSetLoopBack(base, base->ExternalLoopBackMode,
                      base->InternalLoopBackMode);

    /*Listen Only Mode Setting*/
    ithCANSetListenOnlyMode(base, base->ListenOnlyMode);

    /*Trans Standby Mode Setting*/
    ithCANSetTransStandbyMode(base, false);

    ithTTCANCrtlOFF(base);
    /*Resister irq  and enable irq*/
    can_enable_irq_(base, true, can_intr_handler_);
    can_set_int_config_(base, base->InterruptTable);

    switch (base->SourceClock)
    {
        case CAN_SRCCLK_20M:
            ithCANSetTimeStampDiv(base, 0x4E20UL);
            break;

        case CAN_SRCCLK_40M:
            ithCANSetTimeStampDiv(base, 0x9C40UL);
            break;

        case CAN_SRCCLK_60M:
            ithCANSetTimeStampDiv(base, 0xEA60UL);
            break;

        case CAN_SRCCLK_80M:
            ithCANSetTimeStampDiv(base, 0x13880UL);
            break;

        default:
            ithCANSetTimeStampDiv(base, 0x1UL);
            break;
    }

    /*open time stamp*/
    ithCANSetCIA603(base, true, false);
#if 0
    (void)printf("0xbc = %x \n", ithReadRegA((base->ADDR | CAN_VER_0_REG)));
    (void)printf("0xa0 = %x \n", ithReadRegA((base->ADDR | CAN_CFG_STAT_REG)));
#endif

#if SW_WORKAROUND_ENABLE
    sem_init(&can_ptb_sem[base->Instance], 0, 0);
    sem_init(&can_stb_sem[base->Instance], 0, 0);
    sem_init(&can_aif_sem[base->Instance], 0, 0);
#endif
    can_opened[base->Instance] = true;

    pthread_mutex_unlock(&internal_mutex[base->Instance]);
}

int ithCANClose (CAN_HANDLE * base)
{
    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE) ||
        (can_clk_on == false))
    {
        CANPRINT("[MSG]CAN handle parameter incorrect\n");
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    pthread_mutex_lock(&internal_mutex[base->Instance]);

    if (!can_opened[base->Instance])
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine status is closed\n");
        return CAN_NOT_OPEN_ERROR;
    }

    can_enable_irq_(base, false, NULL);
    ithCANDisableIntrAll(base);

    ithCANSWReset(base, true);  /*sw Reset on*/
    ithDelay(10);
    ithCANSWReset(base, false); /*sw Reset off*/
    /*ListenOnlyMode*/
    ithCANSetListenOnlyMode(base, true);

#if SW_WORKAROUND_ENABLE
    sem_destroy(&can_ptb_sem[base->Instance]);
    sem_destroy(&can_stb_sem[base->Instance]);
    sem_destroy(&can_aif_sem[base->Instance]);
#endif
    can_opened[base->Instance] = false;

    pthread_mutex_unlock(&internal_mutex[base->Instance]);

    return CAN_SUCCESS;
}

int ithCANRead (CAN_HANDLE * base, CAN_RXOBJ * info)
{
    int result = CAN_SUCCESS;

    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE))
    {
        ithPrintf("[MSG]CAN handle parameter incorrect\n");
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    /*check RBUF status*/
    if (ithCANGetRBStatus(base) != BUFF_EMPTY)
    {
        CAN_RXOBJ RX         = {0};
        uint32_t  i          = 0U;
        uint32_t  lenof4byte = 0U;
        uint32_t  control    = 0U;
        uint32_t  id         = 0U;
        /*Read control info*/
        control              = ithReadRegA((base->ADDR | CAN_RB_CONTROL_REG));
        RX.Control.DLC       = control & CAN_RB_DLC;
        RX.Control.BRS       = (control & CAN_RB_BRS) >> CAN_RB_BRS_SHIFT;
        RX.Control.EDL       = (control & CAN_RB_EDL) >> CAN_RB_EDL_SHIFT;
        RX.Control.RTR       = (control & CAN_RB_RTR) >> CAN_RB_RTR_SHIFT;
        RX.Control.IDE       = (control & CAN_RB_IDE) >> CAN_RB_IDE_SHIFT;
        RX.Status.TX         = (control & CAN_RB_TX) >> CAN_RB_TX_SHIFT;
        RX.Status.KOER       = (control & CAN_RB_KOER) >> CAN_RB_KOER_SHIFT;
        /*Read identifier*/
        id = ithReadRegA((base->ADDR | CAN_RB_IDENTIFIER_REG));

        /*identifier assign*/
        if (RX.Control.IDE == 0x1)
        {
            RX.Identifier = (id & 0x1FFFFFFFUL);
        }
        else
        {
            RX.Identifier = (id & N11_BITS_MSK);
        }
#if CFG_CANBUS_FD_ENABLE
        RX.ESI = (id >> 31U) & N01_BITS_MSK;
#endif
        if (RX.Status.KOER == CAN_NO_ERROR)
        {
            lenof4byte = alignment4byte_(ithCANDlcToBytes(RX.Control.DLC));
            /*copy data from RBuffer reg*/
            for (i = 0; i < lenof4byte; i++)
            {
                uint32_t tmp =
                    ithReadRegA(base->ADDR | (CAN_RB_DATA0_REG + i * 4));
                (void)memcpy(&RX.RXData[i * 4], &tmp, sizeof(uint8_t) * 4);
            }
            /*time stamp(CIA603)*/
            RX.RXRTS[0] = ithReadRegA(base->ADDR | CAN_RB_RTS0_REG);
            RX.RXRTS[1] = ithReadRegA(base->ADDR | CAN_RB_RTS1_REG);

            (void)memcpy(info, &RX, sizeof(CAN_RXOBJ));
        }
        else
        {
            ithPrintf("[MSG]CAN KOER = %d\n", RX.Status.KOER);
            result = CAN_RX_STATUS_ERROR;
        }

        /*Release this RXbuf*/
        ithWriteRegMaskA((base->ADDR | CAN_CFG_STAT_REG), CAN_RREL,
                         (CAN_RREL | CAN_BUSOFF));
    }
    else
    {
        result = CAN_RX_FIFO_EMPTY;
    }

    /*return code*/
    return result;
}

int ithCANWrite (CAN_HANDLE * base, CAN_TXOBJ * info, uint8_t * dataptr)
{
    uint8_t    txbuffer[DLC_MAX] = {0};
    uint32_t * txptr;
    uint32_t   control     = 0U;
    uint32_t   lenNumBytes = 0U;
    uint32_t   lenNumWords = 0U;
    uint8_t    i           = 0U;
    uint32_t   start_tick  = 0U;

    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE) ||
        (can_clk_on == false))
    {
        CANPRINT("[MSG]CAN handle parameter incorrect\n");
        return CAN_PARAMETER_INCORRECT_ERROR;
    }
    pthread_mutex_lock(&internal_mutex[base->Instance]);

    if (!can_opened[base->Instance])
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine status is closed\n");
        return CAN_NOT_OPEN_ERROR;
    }

    if (!can_set_ptb_wrptr_(base, info))
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine tx ptb fifo is full\n");
        return CAN_TX_FIFO_FULL_ERROR;
    }

#if SW_WORKAROUND_ENABLE
    start_tick = itpGetTickCount();
#endif

    /*set One Shot mode*/
    if (info->SingleShot)
    {
        ithCANSetTransPTBSSMode(base, true);
    }
    else
    {
        ithCANSetTransPTBSSMode(base, false);
    }

    /*Set TB IDENTIFIER*/
    if (info->Control.IDE == 0x1)
    {
        ithWriteRegA((base->ADDR | CAN_TB_IDENTIFIER_REG),
                     (info->Identifier & 0x1FFFFFFFU));
    }
    else
    {
        ithWriteRegA((base->ADDR | CAN_TB_IDENTIFIER_REG),
                     (info->Identifier & 0x7FFU));
    }

    /*Set TTSEN */
    if (info->TTSENSEL)
    {
        ithSetRegBitA((base->ADDR | CAN_TB_IDENTIFIER_REG), 31);
    }
    else
    {
        ithClearRegBitA((base->ADDR | CAN_TB_IDENTIFIER_REG), 31);
    }

    /*set TB control*/
    control |= (info->Control.DLC & 0xF); /* Set payload  length(DLC)*/
    control |= info->Control.BRS << CAN_TB_BRS_SHIFT;
    control |= info->Control.EDL << CAN_TB_EDL_SHIFT;
    control |= info->Control.RTR << CAN_TB_RTR_SHIFT;
    control |= info->Control.IDE << CAN_TB_IDE_SHIFT;
    ithWriteRegA((base->ADDR | CAN_TB_CONTROL_REG), control);

    lenNumBytes = ithCANDlcToBytes(info->Control.DLC);
    /*Copy data to txbuffer*/
    (void)memcpy(&txbuffer, dataptr, sizeof(uint8_t) * lenNumBytes);

    /*Bytes to Words*/
    lenNumWords = alignment4byte_(lenNumBytes);

    for (i = 0; i < lenNumWords && i < (DLC_MAX / 4); i++)
    {
        txptr = (uint32_t *)&txbuffer[i * 4];
        ithWriteRegA((base->ADDR | (CAN_TB_DATA0_REG + i * 4)), *txptr);
    }

    /*CAN ctrl send Msg to bus*/
    ithCANPrimarySendCtrl(base, 1);

    pthread_mutex_unlock(&internal_mutex[base->Instance]);

#if SW_WORKAROUND_ENABLE
    if (base->InterruptTable & TP_INT)
    {
        while (sem_trywait(&can_ptb_sem[base->Instance]) != 0)
        {
            /*The user has actively stopped transmission, so timeout checks are skipped.*/
            if (sem_trywait(&can_aif_sem[base->Instance]) == 0)
            {
                CANPRINT("[MSG]CAN engine tx abort\n");
                return CAN_TX_USER_ABORT_ERROR;
            }

            /*The CAN is currently in a BUS OFF state, so timeout checks are skipped.*/
            if(ithCANBUSState(base) == 0x1U)
            {
                return CAN_BUS_OFF_ERROR;
            }

            if (itpGetTickDuration(start_tick) > info->Timeout)
            {
                pthread_mutex_lock(&internal_mutex[base->Instance]);
                can_exc_handler_(base); /*time out exception*/
                pthread_mutex_unlock(&internal_mutex[base->Instance]);
                CANPRINT("[MSG]CAN engine tx wait timeout\n");
                return CAN_TX_WAIT_TIMEOUT_ERROR;
            }
            (void)usleep(1000);
        }
    }
#endif

    /*return write success*/
    return CAN_SUCCESS;
}

int ithCANFIFOUpdate (CAN_HANDLE * base, CAN_TXOBJ * info, uint8_t * dataptr)
{
    uint8_t    txbuffer[DLC_MAX] = {0};
    uint32_t * txptr;
    uint32_t   control     = 0U;
    uint32_t   lenNumBytes = 0U;
    uint32_t   lenNumWords = 0U;
    uint8_t    i           = 0U;

    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE) ||
        (can_clk_on == false))
    {
        CANPRINT("[MSG]CAN handle parameter incorrect\n");
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    pthread_mutex_lock(&internal_mutex[base->Instance]);

    if (!can_opened[base->Instance])
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine status is closed\n");
        return CAN_NOT_OPEN_ERROR;
    }

    if (!can_set_stb_wrptr_(base, info))
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine tx stb fifo is full\n");
        return CAN_TX_FIFO_FULL_ERROR;
    }
    /*Set TB IDENTIFIER*/
    if (info->Control.IDE == 0x1)
    {
        ithWriteRegA((base->ADDR | CAN_TB_IDENTIFIER_REG),
                     (info->Identifier & 0x1FFFFFFFU));
    }
    else
    {
        ithWriteRegA((base->ADDR | CAN_TB_IDENTIFIER_REG),
                     (info->Identifier & 0x7FFU));
    }

     /*Set TTSEN */
    if (info->TTSENSEL)
    {
        ithSetRegBitA((base->ADDR | CAN_TB_IDENTIFIER_REG), 31);
    }
    else
    {
        ithClearRegBitA((base->ADDR | CAN_TB_IDENTIFIER_REG), 31);
    }

    /*Set TB Control*/
    control |= (info->Control.DLC & 0xF); /*Set payload  length(DLC)*/
    control |= info->Control.BRS << CAN_TB_BRS_SHIFT;
    control |= info->Control.EDL << CAN_TB_EDL_SHIFT;
    control |= info->Control.RTR << CAN_TB_RTR_SHIFT;
    control |= info->Control.IDE << CAN_TB_IDE_SHIFT;
    ithWriteRegA((base->ADDR | CAN_TB_CONTROL_REG), control);

    lenNumBytes = ithCANDlcToBytes(info->Control.DLC);
    /*Copy data to txbuffer*/
    (void)memcpy(&txbuffer, dataptr, sizeof(uint8_t) * lenNumBytes);

    /*Bytes to Words*/
    lenNumWords = alignment4byte_(lenNumBytes);

    for (i = 0; i < lenNumWords && i < (DLC_MAX / 4); i++)
    {
        txptr = (uint32_t *)&txbuffer[i * 4];
        ithWriteRegA((base->ADDR | (CAN_TB_DATA0_REG + i * 4)), *txptr);
    }
    /*STB BUFFER pointer next BUFFER*/
    ithWriteRegMaskA((base->ADDR | CAN_CFG_STAT_REG), CAN_TSNEXT,
                     (CAN_TSNEXT | CAN_BUSOFF));

    pthread_mutex_unlock(&internal_mutex[base->Instance]);
    /*return write success*/
    return CAN_SUCCESS;
}

int ithCANFIFOWrite (CAN_HANDLE * base, CAN_TXOBJ * info)
{
    uint32_t start_tick = 0U;

    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE) ||
        (can_clk_on == false))
    {
        CANPRINT("[MSG]CAN handle parameter incorrect\n");
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    pthread_mutex_lock(&internal_mutex[base->Instance]);

    if (!can_opened[base->Instance])
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine status is closed\n");
        return CAN_NOT_OPEN_ERROR;
    }

#if SW_WORKAROUND_ENABLE
    if (ithCANGetSecondarySendIdle(base))
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine tx stb send is busy\n");
        return CAN_TX_BUSY_ERROR;
    }
    start_tick = itpGetTickCount();
#endif
    /*check single shot mode */
    if (info->SingleShot)
    {
        ithCANSetTransSTBSSMode(base, true);
    }
    else
    {
        ithCANSetTransSTBSSMode(base, false);
    }

    if (ithCANGetTBStatus(base, true) != BUFF_EMPTY)
    {
        /*STB SEND*/
        ithCANSecondarySendCtrl(base, true);

        pthread_mutex_unlock(&internal_mutex[base->Instance]);

#if SW_WORKAROUND_ENABLE
        if (base->InterruptTable & TS_INT)
        {
            while (sem_trywait(&can_stb_sem[base->Instance]) != 0)
            {
                /*The user has actively stopped transmission, so timeout checks are skipped.*/
                if (sem_trywait(&can_aif_sem[base->Instance]) == 0)
                {
                    CANPRINT("[MSG]CAN engine tx abort\n");
                    return CAN_TX_USER_ABORT_ERROR;
                }

                /*The CAN is currently in a BUS OFF state, so timeout checks are skipped.*/
                if(ithCANBUSState(base) == 0x1U)
                {
                    return CAN_BUS_OFF_ERROR;
                }

                if (itpGetTickDuration(start_tick) > info->Timeout)
                {
                    pthread_mutex_lock(&internal_mutex[base->Instance]);
                    can_exc_handler_(base); /*time out exception*/
                    pthread_mutex_unlock(&internal_mutex[base->Instance]);
                    CANPRINT("[MSG]CAN engine tx wait timeout\n");
                    return CAN_TX_WAIT_TIMEOUT_ERROR;
                }
                (void)usleep(1000);
            }
        }
#endif
    }
    else
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
    }

    /*return write success*/
    return CAN_SUCCESS;
}

int ithCANEnterListOnlyMode (CAN_HANDLE * base)
{
    int      result     = CAN_ENTERLOM_TIMEOUT_ERROR;
    uint32_t stat_data  = 0U;
    uint32_t timercount = 0U;

    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE) ||
        (can_clk_on == false))
    {
        CANPRINT("[MSG]CAN handle parameter incorrect\n");
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    pthread_mutex_lock(&internal_mutex[base->Instance]);

    if (!can_opened[base->Instance])
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine status is closed\n");
        return CAN_NOT_OPEN_ERROR;
    }

    pthread_mutex_unlock(&internal_mutex[base->Instance]);

    while (timercount < 1000UL)
    {
        stat_data = ithReadRegA((base->ADDR | CAN_CFG_STAT_REG));

        if ((stat_data & CAN_TPE) == 0x0UL)
        {
            if ((stat_data & CAN_TSONE) == 0x0UL &&
                (stat_data & CAN_TSALL) == 0x0UL)
            {
                pthread_mutex_lock(&internal_mutex[base->Instance]);
                ithCANSetListenOnlyMode(base, true);
                pthread_mutex_unlock(&internal_mutex[base->Instance]);
                result = CAN_SUCCESS;
                break;
            }
            else
            {
                pthread_mutex_lock(&internal_mutex[base->Instance]);
                ithCANSecondarySendCtrl(base, false); /*Stop STB Tx*/
                pthread_mutex_unlock(&internal_mutex[base->Instance]);
            }
        }
        else
        {
            pthread_mutex_lock(&internal_mutex[base->Instance]);
            ithCANPrimarySendCtrl(base, false); /*Stop PTB Tx*/
            pthread_mutex_unlock(&internal_mutex[base->Instance]);
        }

        timercount++;
        (void)usleep(100); /*busy wait*/
    }

    /*return code*/
    return result;
}

int ithCANLeaveListOnlyMode (CAN_HANDLE * base)
{
    if ((base == NULL) || (base->Instance >= CAN_MAX_INSTANCE) ||
        (can_clk_on == false))
    {
        CANPRINT("[MSG]CAN handle parameter incorrect\n");
        return CAN_PARAMETER_INCORRECT_ERROR;
    }

    pthread_mutex_lock(&internal_mutex[base->Instance]);

    if (!can_opened[base->Instance])
    {
        pthread_mutex_unlock(&internal_mutex[base->Instance]);
        CANPRINT("[MSG]CAN engine status is closed\n");
        return CAN_NOT_OPEN_ERROR;
    }

    ithCANSetListenOnlyMode(base, false);

    pthread_mutex_unlock(&internal_mutex[base->Instance]);

    return CAN_SUCCESS;
}

uint32_t ithCANDlcToBytes (CAN_DLC dlc)
{
    uint32_t dataBytesInObject = 0U;

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
                dataBytesInObject = 12U;
                break;
            case CAN_DLC_16:
                dataBytesInObject = 16U;
                break;
            case CAN_DLC_20:
                dataBytesInObject = 20U;
                break;
            case CAN_DLC_24:
                dataBytesInObject = 24U;
                break;
            case CAN_DLC_32:
                dataBytesInObject = 32U;
                break;
            case CAN_DLC_48:
                dataBytesInObject = 48U;
                break;
            case CAN_DLC_64:
                dataBytesInObject = 64U;
                break;
            default:
                break;
        }
    }
#endif
    return dataBytesInObject;
}

int ithCANGetTTS (CAN_HANDLE * base)
{
    if (can_clk_on == false)
    {
        return -1;
    }
    else
    {
        return (int)ithCANGetTransTimeStamp(base);
    }
}

void ithCANReset (CAN_HANDLE * base)
{
    if (can_clk_on == false)
    {
        return;
    }

    pthread_mutex_lock(&internal_mutex[base->Instance]);

    ithCANSWReset(base, true);
    ithDelay(10);
    ithCANSWReset(base, false);

    pthread_mutex_unlock(&internal_mutex[base->Instance]);
}

CAN_ERROR_STATE ithCANGetErrorState (CAN_HANDLE * base)
{
    CAN_ERROR_STATE state = ERROR_ACTIVE;

    if (can_clk_on == false)
    {
        return BUS_OFF;
    }

    if (ithCANBUSState(base) == 0x1U)
    {
        state = BUS_OFF;
    }
    else if (ithCANERRORState(base) == 0x1U)
    {
        state = ERROR_PASSIVE;
    }
    return state;
}

uint32_t ithCANGetREC (CAN_HANDLE * base)
{
    if (can_clk_on == false)
    {
        return 255UL;
    }
    else
    {
        return ithCANGetReceiveErrorCouNT(base);
    }
}

uint32_t ithCANGetTEC (CAN_HANDLE * base)
{
    if (can_clk_on == false)
    {
        return 255UL;
    }
    else
    {
        return ithCANGetTransmitErrorCouNT(base);
    }
}

CAN_ERROR_TYPE ithCANGetKOER (CAN_HANDLE * base)
{
    if (can_clk_on == false)
    {
        return CAN_OTHER_ERROR;
    }
    else
    {
        return ithCANGetKindOfError(base);
    }
}

void ithCANSetGPIO (uint32_t Instance, uint32_t rxpin, uint32_t txpin)
{
    uint32_t WREG = 0x80008000UL;
    pthread_mutex_lock(&can_general_mutex);

    // for IT986x ECO
    if ((rxpin >= 21U) && (rxpin <= 24U))
    {
        (void)printf("[MSG]CAN RXPin GPIO(%" PRIu32 ") NO SUPPORT!!\n", rxpin);
        rxpin = rxpin - 21U;
    }

    if ((txpin >= 21U) && (txpin <= 24U))
    {
        txpin = txpin - 21U;
    }
    // for IT986x ECO end

    WREG |= rxpin << 16U;
    WREG |= txpin;

    if (Instance == 0UL)
    {
        ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO_CANBUS0_REG, WREG);
    }
    else if (Instance == 1UL)
    {
        ithWriteRegA(ITH_GPIO_BASE + ITH_GPIO_CANBUS1_REG, WREG);
    }
    else
    {
        CANPRINT("[MSG]CAN no this instance\n");
    }

    pthread_mutex_unlock(&can_general_mutex);
}

void ithCANCLKEnable (uint8_t ctl)
{
    uint32_t tmp  = 0U;
    uint32_t mask = 0U;

    if (ctl == 0x1U) // Enable CLK
    {
        pthread_mutex_lock(&can_general_mutex);
        tmp = ithReadRegA(APB2CLK_REG);
        tmp |= (0x1UL << APB2CLK_RESET);
        tmp |= (0x1UL << APB2CLK_CAN1_RESET);
        tmp |= (0x1UL << APB2CLK_CAN0_RESET);
        tmp |= (0x1UL << APB2CLK_EN);
        tmp |= (0x1UL << APB2CLK_EN_DIV_CLK);
        tmp |= (0x1UL << APB2CLK_UPDATE);
        ithWriteRegA(APB2CLK_REG, tmp);

        tmp = ithReadRegA(APB2CLK_REG);
        mask |= (0x1UL << APB2CLK_RESET);
        mask |= (0x1UL << APB2CLK_CAN1_RESET);
        mask |= (0x1UL << APB2CLK_CAN0_RESET);
        tmp = tmp & ~mask;
        ithWriteRegA(APB2CLK_REG, tmp);
        can_clk_on = true;
        pthread_mutex_unlock(&can_general_mutex);

        (void)usleep(250);
    }
    else // Disable CLK
    {
        pthread_mutex_lock(&can_general_mutex);
        ithClearRegBitA(APB2CLK_REG, APB2CLK_EN);
        ithClearRegBitA(APB2CLK_REG, APB2CLK_EN_DIV_CLK);
        can_clk_on = false;
        pthread_mutex_unlock(&can_general_mutex);
    }
}
