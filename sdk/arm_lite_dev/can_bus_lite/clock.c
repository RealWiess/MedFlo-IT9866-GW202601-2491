#include "arm_lite_dev/can_bus_lite/clock.h"

static unsigned int clkCpu = 0, clkMem = 0, clkBus = 0, riscCpu = 0;
static uint32_t     ClkApb2Val, ClkAhbVal, ClkAxiVal, ClkMem2Val, ClkRiscVal;
static uint32_t     ClkCapVal, ClkDPVal;

#define PLL1_1_SETTING1            (ITH_HOST_BASE + 0x0100)
#define PLL1_1_SETTING2            (ITH_HOST_BASE + 0x0104)
#define PLL_MODE_OFFSET            31
#define PLL_MODE_MASK              0x1
#define PLL_MUX_OFFSET             30
#define PLL_MUX_MASK               0x1
#define PLL_NUMERATOR_OFFSET       16
#define PLL_NUMERATOR_MASK         0x7F
#define PLL_POST_DIV_OFFSET        8
#define PLL_POST_DIV_MASK          0x7F
#define PLL_PRE_DIV_OFFSET         0
#define PLL_PRE_DIV_MASK           0x1F
#define PLL_SDM_FRACTION_OFFSET    0
#define PLL_SDM_FRACTION_MASK      0x7FFFF
#define PLL_SDK_FRACTION_SIGN_MASK 0x80000

#define APBCLK_REG                 (ITH_HOST_BASE + 0x001C)
#define APBCLK_SRC_SEL_OFFSET      12
#define APBCLK_SRC_SEL_MASK        0x7
#define APBCLK_RATIO_OFFSET        0
#define APBCLK_RATIO_MASK          0x3FF

#define DEFAULT_XIN_CLK            (12000000)
#define DEFAULT_RING_CLK           (200000)

typedef enum
{
    CLK_PLL1_OUTPUT1 = 0, ///< From PLL1 output1
    CLK_PLL1_OUTPUT2 = 1, ///< From PLL1 output2
    CLK_PLL2_OUTPUT1 = 2, ///< From PLL2 output1
    CLK_PLL2_OUTPUT2 = 3, ///< From PLL2 output2
    CLK_PLL3_OUTPUT1 = 4, ///< From PLL3 output1
    CLK_PLL3_OUTPUT2 = 5, ///< From PLL3 output2
    CLK_PLL3_OUTPUT3 = 6, ///< From PLL3 output3
    CLK_CKSYS        = 7, ///< From CKSYS (12MHz)
} OUTPUTCLK_SRC;


static uint32_t ARMLite_GetOutputPllClk(uint32_t outputPll)
{
    bool     done   = false;
    uint64_t srcClk = DEFAULT_XIN_CLK;

    while (!done)
    {
        uint32_t regSetting1Val = ithReadRegA(((outputPll - CLK_PLL1_OUTPUT1) << 3) + PLL1_1_SETTING1);
        uint32_t regSetting2Val = ithReadRegA(((outputPll - CLK_PLL1_OUTPUT1) << 3) + PLL1_1_SETTING2);
        int32_t  nMode          = ((regSetting1Val >> PLL_MODE_OFFSET) & PLL_MODE_MASK);
        int32_t  clkFromPll     = ((regSetting1Val >> PLL_MUX_OFFSET) & PLL_MUX_MASK);
        int32_t  preDiv         = ((regSetting1Val >> PLL_PRE_DIV_OFFSET) & PLL_PRE_DIV_MASK);
        int32_t  postDiv        = ((regSetting1Val >> PLL_POST_DIV_OFFSET) & PLL_POST_DIV_MASK);
        int32_t  numerator      = ((regSetting1Val >> PLL_NUMERATOR_OFFSET) & PLL_NUMERATOR_MASK);
        uint32_t newNumerator;

        if (nMode)
        {
            int32_t fractionalPart;
            int32_t signBit = ((regSetting2Val >> PLL_SDM_FRACTION_OFFSET) & PLL_SDK_FRACTION_SIGN_MASK);
            fractionalPart = ((regSetting2Val >> PLL_SDM_FRACTION_OFFSET) & PLL_SDM_FRACTION_MASK) / 1024;

            if (signBit)
            {
                fractionalPart = -(1024 - fractionalPart);
            }
            newNumerator = numerator * 1024 + fractionalPart;
        }

        switch (outputPll)
        {
        case CLK_PLL1_OUTPUT1:
            if (clkFromPll)
            {
                outputPll = CLK_PLL1_OUTPUT2;
            }
            else
            {
                done = true;
            }
            break;
        case CLK_PLL2_OUTPUT1:
            if (clkFromPll)
            {
                outputPll = CLK_PLL2_OUTPUT2;
            }
            else
            {
                done = true;
            }
            break;
        case CLK_PLL3_OUTPUT1:
            if (clkFromPll)
            {
                outputPll = CLK_PLL3_OUTPUT2;
            }
            else
            {
                done = true;
            }
            break;
        case CLK_PLL3_OUTPUT2:
            if (clkFromPll)
            {
                outputPll = CLK_PLL3_OUTPUT3;
            }
            else
            {
                done = true;
            }
            break;
        default:
            done = true;
            break;
        }

        if (preDiv == 0)
        {
            preDiv = 1;
        }

        if (postDiv == 0)
        {
            postDiv = 1;
        }

        if (nMode)
        {
            srcClk = ((srcClk / preDiv * newNumerator / postDiv) / 1024);
        }
        else
        {
            srcClk = ((srcClk / preDiv * numerator / postDiv));
        }
    }

    return (uint32_t)srcClk;
}

uint32_t ARMLite_GetSrcClk(uint32_t clkSrc)
{
    if (clkSrc == CLK_CKSYS)
    {
        return DEFAULT_XIN_CLK;
    }
    else
    {
        return ARMLite_GetOutputPllClk(clkSrc);
    }
}

uint32_t ARMLite_GetBusClock(void)
{
    uint32_t src = ((ithReadRegA(APBCLK_REG) >> APBCLK_SRC_SEL_OFFSET) & APBCLK_SRC_SEL_MASK);
    uint32_t div = ((ithReadRegA(APBCLK_REG) >> APBCLK_RATIO_OFFSET) & APBCLK_RATIO_MASK);
    //000: From PLL1 output1 (default)
    //001: From PLL1 output2
    //010: From PLL2 output1
    //011: From PLL2 output2
    //100: From PLL3 output1
    //101: From PLL3 output2
    //111: From PLL3 output3
    //111: From CKSYS (12MHz)
    if (div == 0)
    {
        div = 1;
    }
    return ARMLite_GetSrcClk(src) / div;
}