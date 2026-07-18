#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <string.h>

#include "ite/ith.h"
#include "ite/itp.h"
#include "saradc/saradc.h"
#include "saradc/saradc_hw.h"

// =============================================================================
//                Constant Definition
// =============================================================================

// =============================================================================
//                Macro Definition
// =============================================================================

// =============================================================================
//                Structure Definition
// =============================================================================
typedef struct XAIN_OBJECT_TAG
{
    SARADC_PORT                 hwPort;
    unsigned int                cfgGPIO;
    SARADC_IO_MAPPING_ENTRY     mappingEntry;
    XAIN_INFO                   xainInfo;
} XAIN_OBJECT;

typedef struct SARADC_OBJECT_TAG
{
    int                     refCount;
    pthread_mutex_t         thread_mutex;
    uint8_t                 validXAIN;
    uint8_t                 enableXAIN;
    SARADC_MODE_AVG         opModeAVG;
    SARADC_MODE_STORE       opModeStore;
    SARADC_AMPLIFY_GAIN     driving;
    SARADC_CLK_DIV          clkDivider;
    uint16_t                wbSize;
    SARADC_AVG_CAL_COUNT    avgCalCount;
    uint8_t                 trigStoreCount;
    SARADC_XAIN_SIGNAL      xainSignal;
    bool                    inUse;
    XAIN_OBJECT             XAINObjects[SARADC_XAIN_MAX_COUNT];
#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
    uint8_t                 caliXAIN;
    float                   caliVoltage[SARADC_XAIN_MAX_COUNT];
    bool                    caliUse;
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
    uint8_t                 caliXAIN;
    float                   caliVoltage[SARADC_XAIN_MAX_COUNT];
    bool                    caliUse;
    #endif

#endif
} SARADC_OBJECT;

typedef struct PORT_IO_MAPPING_ENTRY_TAG
{
    SARADC_PORT             hwPort;
    SARADC_IO_MAPPING_ENTRY mappingEntry;
} PORT_IO_MAPPING_ENTRY;

typedef struct AD_MAPPING_ENTRY_TAG
{
    float       analogInput;
    uint16_t    digitalOutput;
} AD_MAPPING_ENTRY;

// =============================================================================
//                Global Data Definition
// =============================================================================
static SARADC_OBJECT SARADCObject =
{
    0,
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
    CFG_SARADC_VALID_XAIN,
    CFG_SARADC_VALID_XAIN,
    SARADC_MODE_AVG_ENABLE,
    SARADC_MODE_STORE_AVG_ENABLE,
    SARADC_AMPLIFY_1X,
    SARADC_CLK_DIV_9,
    0,
    SARADC_AVG_CAL_COUNT_0,
    5,
    SARADC_XAIN_SIGNAL_0,
    false,
#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
    {SARADC_0},
    CFG_SARADC_CALIBRATION_XAIN,
    {CFG_SARADC_CALIBRATION_VOLTAGE},
    true
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
    {SARADC_0},
    CFG_SARADC_CALIBRATION_XAIN,
    {CFG_SARADC_CALIBRATION_OFFSET},
    true
    #endif

#else
    {0}
#endif
};

static PORT_IO_MAPPING_ENTRY tSARADCMappingTable[] =
{
    {SARADC_0, 19, ITH_GPIO_MODE0}, {SARADC_1, 20, ITH_GPIO_MODE0},
    {SARADC_2, 21, ITH_GPIO_MODE0}, {SARADC_3, 22, ITH_GPIO_MODE0},
    {SARADC_4, 23, ITH_GPIO_MODE0}, {SARADC_5, 24, ITH_GPIO_MODE0},
    {SARADC_6, 25, ITH_GPIO_MODE0}, {SARADC_7, 26, ITH_GPIO_MODE0}
};

static AD_MAPPING_ENTRY tREALMappingTable[] =
{
    {0.0F,    0U}, {0.1F,   91U}, {0.2F,  222U}, {0.3F,  350U},
    {0.4F,  481U}, {0.5F,  606U}, {0.6F,  734U}, {0.7F,  862U},
    {0.8F,  994U}, {0.9F, 1116U}, {1.0F, 1274U}, {1.1F, 1404U},
    {1.2F, 1533U}, {1.3F, 1678U}, {1.4F, 1802U}, {1.5F, 1930U},
    {1.6F, 2048U}, {1.7F, 2208U}, {1.8F, 2342U}, {1.9F, 2480U},
    {2.0F, 2612U}, {2.1F, 2754U}, {2.2F, 2869U}, {2.3F, 3003U},
    {2.4F, 3127U}, {2.5F, 3268U}, {2.6F, 3403U}, {2.7F, 3533U},
    {2.8F, 3670U}, {2.9F, 3792U}, {3.0F, 3931U}, {3.1F, 4058U},
    {3.2F, 4095U}, {3.3F, 4095U}
};

#define INTERPOLATION_SIZE 34U
static AD_MAPPING_ENTRY tINTERPOLATIONMappingTable[] =
{
    {0.0F,    0U}, {0.1F,  124U}, {0.2F,  248U}, {0.3F,  372U},
    {0.4F,  496U}, {0.5F,  620U}, {0.6F,  745U}, {0.7F,  869U},
    {0.8F,  993U}, {0.9F, 1117U}, {1.0F, 1241U}, {1.1F, 1365U},
    {1.2F, 1489U}, {1.3F, 1613U}, {1.4F, 1737U}, {1.5F, 1861U},
    {1.6F, 1985U}, {1.7F, 2110U}, {1.8F, 2234U}, {1.9F, 2358U},
    {2.0F, 2482U}, {2.1F, 2606U}, {2.2F, 2730U}, {2.3F, 2854U},
    {2.4F, 2978U}, {2.5F, 3102U}, {2.6F, 3226U}, {2.7F, 3350U},
    {2.8F, 3475U}, {2.9F, 3599U}, {3.0F, 3723U}, {3.1F, 3847U},
    {3.2F, 3971U}, {3.3F, 4095U}
};

#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
        static AD_MAPPING_ENTRY tCALIBRATIONMappingTable[SARADC_XAIN_MAX_COUNT] = {};
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
        static AD_MAPPING_ENTRY tCALIBRATIONMappingTable[SARADC_XAIN_MAX_COUNT] = {};
    #endif

#endif

// =============================================================================
//                Private Function Definition
// =============================================================================
static void
resetSARADCObject (
    SARADC_OBJECT * object)
{
    unsigned int i = 0;
#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
    float caliVoltage[SARADC_XAIN_MAX_COUNT] = {CFG_SARADC_CALIBRATION_VOLTAGE};
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
    float caliVoltage[SARADC_XAIN_MAX_COUNT] = {CFG_SARADC_CALIBRATION_OFFSET};
    #endif

#endif

    object->validXAIN       = CFG_SARADC_VALID_XAIN;
    object->enableXAIN      = CFG_SARADC_VALID_XAIN;
    object->opModeAVG       = SARADC_MODE_AVG_ENABLE;
    object->opModeStore     = SARADC_MODE_STORE_AVG_ENABLE;
    object->driving         = SARADC_AMPLIFY_1X;
    object->clkDivider      = SARADC_CLK_DIV_9;
    object->wbSize          = 0;
    object->avgCalCount     = SARADC_AVG_CAL_COUNT_0;
    object->trigStoreCount  = 5;
    object->xainSignal          = SARADC_XAIN_SIGNAL_0;
    object->inUse           = false;
#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
    object->caliXAIN    = CFG_SARADC_CALIBRATION_XAIN;
    (void)memcpy(object->caliVoltage, caliVoltage, sizeof(caliVoltage));
    object->caliUse     = true;
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
    object->caliXAIN    = CFG_SARADC_CALIBRATION_XAIN;
    (void)memcpy(object->caliVoltage, caliVoltage, sizeof(caliVoltage));
    object->caliUse     = true;
    #endif

#endif

    (void)memset(object->XAINObjects, 0, sizeof(object->XAINObjects));
    for (i = 0; i < SARADC_XAIN_MAX_COUNT; i++)
    {
        SARADC_PORT tmp = (SARADC_PORT)i;
        if (tmp == SARADC_0)
        {
            object->XAINObjects[i].hwPort   = SARADC_0;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_0_RX;
        }
        else if (tmp == SARADC_1)
        {
            object->XAINObjects[i].hwPort   = SARADC_1;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_1_RX;
        }
        else if (tmp == SARADC_2)
        {
            object->XAINObjects[i].hwPort   = SARADC_2;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_2_RX;
        }
        else if (tmp == SARADC_3)
        {
            object->XAINObjects[i].hwPort   = SARADC_3;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_3_RX;
        }
        else if (tmp == SARADC_4)
        {
            object->XAINObjects[i].hwPort   = SARADC_4;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_4_RX;
        }
        else if (tmp == SARADC_5)
        {
            object->XAINObjects[i].hwPort   = SARADC_5;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_5_RX;
        }
        else if (tmp == SARADC_6)
        {
            object->XAINObjects[i].hwPort   = SARADC_6;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_6_RX;
        }
        else
        {
            object->XAINObjects[i].hwPort   = SARADC_7;
            object->XAINObjects[i].cfgGPIO  = CFG_GPIO_XAIN_7_RX;
        }
    }
}

static bool
assignSARADCGpio(SARADC_OBJECT *object)
{
    unsigned int    i = 0U;
    unsigned int    j = 0U;
    SARADC_IO_MAPPING_ENTRY *pMappingEntry = NULL;

    for (i = 0U; i < SARADC_XAIN_MAX_COUNT; i++)
    {
        pMappingEntry = &object->XAINObjects[i].mappingEntry;

        unsigned int entryCount = sizeof(tSARADCMappingTable) / sizeof(PORT_IO_MAPPING_ENTRY);
        for (j = 0U; j < entryCount; j++)
        {
            if ((object->XAINObjects[i].hwPort == tSARADCMappingTable[j].hwPort) &&
                    (object->XAINObjects[i].cfgGPIO == tSARADCMappingTable[j].mappingEntry.gpioPin))
            {
                break;
            }
        }

        if (j >= entryCount)
        {
            return false;
        }

        pMappingEntry->gpioPin  = tSARADCMappingTable[j].mappingEntry.gpioPin;
        pMappingEntry->gpioMode = tSARADCMappingTable[j].mappingEntry.gpioMode;
    }

    return true;
}

static bool
setSARADCGPIO(SARADC_OBJECT *object)
{    
    unsigned int            setGpio = 0U;
    ITHGpioMode             setMode = ITH_GPIO_MODE0;
    uint8_t                 validXAIN = object->validXAIN;
    int                     i = 0;
    SARADC_IO_MAPPING_ENTRY *pMappingEntry = NULL;

    if ( validXAIN > ((unsigned int)pow(2.0F, (float)SARADC_XAIN_MAX_COUNT) - 1U))
    {
        (void)printf("[SARADC][%s] validXAIN is out of range: %d\n", __func__, validXAIN);
        return false;
    }

    if (!assignSARADCGpio(object))
    {
        (void)printf("[SARADC][%s] assignSARADCGpio() fail\n", __func__);
        return false;
    }

    while (validXAIN != 0x0U)
    {
        if ((validXAIN & 0x1U) == 0x1U)
        {
            pMappingEntry   = &object->XAINObjects[i].mappingEntry;

            setGpio         = pMappingEntry->gpioPin;
            setMode         = pMappingEntry->gpioMode;

            ithGpioSetDriving(setGpio, ITH_GPIO_DRIVING_3);
            ithGpioSetIn(setGpio);
            ithGpioSetMode(setGpio, setMode);
        }
        validXAIN >>= 1;
        i++;
    }

    return true;
}

static void
assignSARADCINTR(const SARADC_OBJECT *object)
{
    uint8_t enableXAIN = object->enableXAIN;
    unsigned int     i = 0U;

    for (i = 0U; i < SARADC_XAIN_MAX_COUNT; i++)
    {
        SARADC_PORT tmp = (SARADC_PORT)i;
        if ((enableXAIN & 0x1U) == 0x1U)
        {            
            if (tmp == SARADC_0)
            {    SARADC_EnableXAININTR_Reg(SARADC_0);}
            else if (tmp == SARADC_1)
            {    SARADC_EnableXAININTR_Reg(SARADC_1);}
            else if (tmp == SARADC_2)
            {    SARADC_EnableXAININTR_Reg(SARADC_2);}
            else if (tmp == SARADC_3)
            {    SARADC_EnableXAININTR_Reg(SARADC_3);}
            else if (tmp == SARADC_4)
            {    SARADC_EnableXAININTR_Reg(SARADC_4);}
            else if (tmp == SARADC_5)
            {    SARADC_EnableXAININTR_Reg(SARADC_5);}
            else if (tmp == SARADC_6)
            {    SARADC_EnableXAININTR_Reg(SARADC_6);}
            else
            {    SARADC_EnableXAININTR_Reg(SARADC_7);}
        }
        else
        {
            if (tmp == SARADC_0)
            {    SARADC_DisableXAININTR_Reg(SARADC_0);}
            else if (tmp == SARADC_1)
            {    SARADC_DisableXAININTR_Reg(SARADC_1);}
            else if (tmp == SARADC_2)
            {    SARADC_DisableXAININTR_Reg(SARADC_2);}
            else if (tmp == SARADC_3)
            {    SARADC_DisableXAININTR_Reg(SARADC_3);}
            else if (tmp == SARADC_4)
            {    SARADC_DisableXAININTR_Reg(SARADC_4);}
            else if (tmp == SARADC_5)
            {    SARADC_DisableXAININTR_Reg(SARADC_5);}
            else if (tmp == SARADC_6)
            {    SARADC_DisableXAININTR_Reg(SARADC_6);}
            else
            {    SARADC_DisableXAININTR_Reg(SARADC_7);}
        }

        enableXAIN >>= 1;
    }
}

static void
setSARADCGeneralReg (
    SARADC_OBJECT * object)
{
    SARADC_SetCLKDIV_Reg(object->clkDivider);
    SARADC_SetModeAVG_Reg(object->opModeAVG);
    SARADC_SetModeStore_Reg(object->opModeStore);
    SARADC_SetGAIN_Reg(object->driving);
    SARADC_SetValidXAIN_Reg(object->validXAIN);
    assignSARADCINTR(object);
}

static void
resetSARADCAllXAINReg (
    SARADC_OBJECT * object)
{
    unsigned int i = 0U;

    for (i = 0U; i < SARADC_XAIN_MAX_COUNT; i++)
    {
        SARADC_PORT tmp = (SARADC_PORT)i;
        if (tmp == SARADC_0)
        {
            SARADC_SetXAINRPTR_Reg(SARADC_0, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_0);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_0);
            SARADC_ResetXAINEVENT_Reg(SARADC_0);
        }
        else if (tmp == SARADC_1)
        {
            SARADC_SetXAINRPTR_Reg(SARADC_1, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_1);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_1);
            SARADC_ResetXAINEVENT_Reg(SARADC_1);
        }
        else if (tmp == SARADC_2)
        {
            SARADC_SetXAINRPTR_Reg(SARADC_2, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_2);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_2);
            SARADC_ResetXAINEVENT_Reg(SARADC_2);
        }
        else if (tmp == SARADC_3)
        {
            SARADC_SetXAINRPTR_Reg(SARADC_3, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_3);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_3);
            SARADC_ResetXAINEVENT_Reg(SARADC_3);
        }
        else if (tmp == SARADC_4)
        {
            SARADC_SetXAINRPTR_Reg(SARADC_4, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_4);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_4);
            SARADC_ResetXAINEVENT_Reg(SARADC_4);
        }
        else if (tmp == SARADC_5)
        {
            SARADC_SetXAINRPTR_Reg(SARADC_5, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_5);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_5);
            SARADC_ResetXAINEVENT_Reg(SARADC_5);
        }
        else if (tmp == SARADC_6)
        {
            SARADC_SetXAINRPTR_Reg(SARADC_6, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_6);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_6);
            SARADC_ResetXAINEVENT_Reg(SARADC_6);
        }
        else
        {
            SARADC_SetXAINRPTR_Reg(SARADC_7, object->XAINObjects[i].xainInfo.rptr);
            SARADC_ResetXAINOVERWT_Reg(SARADC_7);
            SARADC_ResetXAINAVGDetect_Reg(SARADC_7);
            SARADC_ResetXAINEVENT_Reg(SARADC_7);
        }
    }
}

#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL

static SARADC_RESULT
mmpSARDynamicallyCalibrate (
    uint16_t    input,
    uint16_t    * output)
{
    SARADC_RESULT       result          = SARADC_SUCCESS;
    uint8_t             caliXAIN        = 0x0U;
    int                 h               = 0;
    int                 i               = 0;
    int                 j               = 0;
    int                 k               = 0;
    int                 z               = 0;
    AD_MAPPING_ENTRY    start           = {0};
    AD_MAPPING_ENTRY    end             = {0};
    float               ratio_temp      = 0.0F;
    float               voltage_temp    = 0.0F;

    if ((input > 0xFFFU) || !output)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        goto end;
    }

    caliXAIN = SARADCObject.caliXAIN;
    while (caliXAIN != 0x0U)
    {
        if ((caliXAIN & 0x1U) == 0x1U)
        {
            if (tCALIBRATIONMappingTable[j].digitalOutput <= input)
            {
                h++;
            }
            i++;
        }
        j++;
        caliXAIN >>= 1;
    }

    k           = 0;
    z           = 0;
    caliXAIN    = SARADCObject.caliXAIN;
    if ((h == 0) || (h == i))
    {
        while (caliXAIN != 0x0U)
        {
            if ((caliXAIN & 0x1U) == 0x1U)
            {
                if ((h == 0) && (k == 0))
                {
                    (void)memcpy(&start, &tCALIBRATIONMappingTable[z], sizeof(start));
                }
                else if ((h == 0) && (k == 1))
                {
                    (void)memcpy(&end, &tCALIBRATIONMappingTable[z], sizeof(end));
                    break;
                }
                else if ((h == i) && (k == h - 2))
                {
                    (void)memcpy(&start, &tCALIBRATIONMappingTable[z], sizeof(start));
                }
                else if ((h == i) && (k == h - 1))
                {
                    (void)memcpy(&end, &tCALIBRATIONMappingTable[z], sizeof(end));
                    break;
                }
                k++;
            }
            z++;
            caliXAIN >>= 1;
        }

        ratio_temp = ((float)end.digitalOutput - start.digitalOutput) / (end.analogInput - start.analogInput);

        if (h == 0)
        {
            voltage_temp = start.analogInput - (((float)start.digitalOutput - input) / ratio_temp);
            if (voltage_temp < tINTERPOLATIONMappingTable[0].analogInput)
            {
                voltage_temp = 0;
            }
        }
        else if (h == i)
        {
            voltage_temp = end.analogInput + (((float)input - end.digitalOutput) / ratio_temp);
            if (voltage_temp > tINTERPOLATIONMappingTable[sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0]) - 1].analogInput)
            {
                voltage_temp = tINTERPOLATIONMappingTable[sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0]) - 1].analogInput;
            }
        }
    }
    else
    {
        while (caliXAIN != 0x0U)
        {
            if ((caliXAIN & 0x1U) == 0x1U)
            {
                if (k == h - 1)
                {
                    (void)memcpy(&start, &tCALIBRATIONMappingTable[z], sizeof(start));
                }
                else if (k == h)
                {
                    (void)memcpy(&end, &tCALIBRATIONMappingTable[z], sizeof(end));
                    break;
                }
                k++;
            }
            z++;
            caliXAIN >>= 1;
        }

        ratio_temp      = ((float)end.digitalOutput - start.digitalOutput) / (end.analogInput - start.analogInput);

        voltage_temp    = start.analogInput + (((float)input - start.digitalOutput) / ratio_temp);
        if (voltage_temp > tINTERPOLATIONMappingTable[sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0]) - 1].analogInput)
        {
            voltage_temp = tINTERPOLATIONMappingTable[sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0]) - 1].analogInput;
        }
    }

    *output = (uint16_t)(voltage_temp / tINTERPOLATIONMappingTable[sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0]) - 1].analogInput *
        tINTERPOLATIONMappingTable[sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0]) - 1].digitalOutput);

end:
    return result;
}
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL

static SARADC_RESULT
mmpSARDynamicallyCalibrate (
    uint16_t    input,
    uint16_t    * output)
{
    SARADC_RESULT       result          = SARADC_SUCCESS;
    uint8_t             caliXAIN        = 0x0U;
    int                 h               = 0;
    int                 i               = 0;
    int                 j               = 0;
    int                 k               = 0;
    int                 z               = 0;
    AD_MAPPING_ENTRY    start           = {0.0F};
    AD_MAPPING_ENTRY    end             = {0.0F};
    float               ratio_temp      = 0.0F;
    float               voltage_temp    = 0.0F;

    if ((input > 0xFFFU) || (output == NULL))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        goto end;
    }

    caliXAIN = SARADCObject.caliXAIN;
    while (caliXAIN != 0x0U)
    {
        if ((caliXAIN & 0x1U) == 0x1U)
        {
            if (tCALIBRATIONMappingTable[j].digitalOutput <= input)
            {
                h++;
            }
            i++;
        }
        j++;
        caliXAIN >>= 1;
    }

    caliXAIN = SARADCObject.caliXAIN;
    if ((h == 0) || (h == i))
    {
        while (caliXAIN != 0x0U)
        {
            if ((caliXAIN & 0x1U) == 0x1U)
            {
                if ((h == 0) && (k == 0))
                {
                    (void)memcpy(&start, &tCALIBRATIONMappingTable[z], sizeof(start));
                }
                else if ((h == 0) && (k == 1))
                {
                    (void)memcpy(&end, &tCALIBRATIONMappingTable[z], sizeof(end));
                    break;
                }
                else if ((h == i) && (k == (h - 2)))
                {
                    (void)memcpy(&start, &tCALIBRATIONMappingTable[z], sizeof(start));
                }
                else if ((h == i) && (k == (h - 1)))
                {
                    (void)memcpy(&end, &tCALIBRATIONMappingTable[z], sizeof(end));
                    break;
                }
                k++;
            }
            z++;
            caliXAIN >>= 1;
        }

        ratio_temp = ((float)end.digitalOutput - (float)start.digitalOutput) / (end.analogInput - start.analogInput);

        if (h == 0)
        {
            voltage_temp = ((float)start.analogInput - ((((float)start.digitalOutput - (float)input)) / ratio_temp));
            if (voltage_temp < tINTERPOLATIONMappingTable[0].analogInput)
            {   
                voltage_temp = 0.0F;
            }
        }
        else if (h == i)
        {
            voltage_temp = end.analogInput + (((float)input - (float)end.digitalOutput) / ratio_temp);

            unsigned int index = sizeof(tINTERPOLATIONMappingTable) / (sizeof(tINTERPOLATIONMappingTable[0]) - 1U);
            if( index < INTERPOLATION_SIZE)
            {
                if (voltage_temp > (tINTERPOLATIONMappingTable[index].analogInput))
                {
                    voltage_temp = (tINTERPOLATIONMappingTable[index].analogInput);
                }
            }
        }
    }
    else
    {
        while (caliXAIN != 0x0U)
        {
            if ((caliXAIN & 0x1U) == 0x1U)
            {
                if (k == (h - 1))
                {
                    (void)memcpy(&start, &tCALIBRATIONMappingTable[z], sizeof(start));
                }
                else if (k == h)
                {
                    (void)memcpy(&end, &tCALIBRATIONMappingTable[z], sizeof(end));
                    break;
                }
                k++;
            }
            z++;
            caliXAIN >>= 1;
        }

        ratio_temp = ((float)end.digitalOutput - (float)start.digitalOutput) / (end.analogInput - start.analogInput);

        voltage_temp = start.analogInput + (((float)input - (float)start.digitalOutput) / ratio_temp);

        unsigned int index = sizeof(tINTERPOLATIONMappingTable) / (sizeof(tINTERPOLATIONMappingTable[0]) - 1U);
        if( index < INTERPOLATION_SIZE)
        {
            if (voltage_temp > tINTERPOLATIONMappingTable[index].analogInput)
            {
                voltage_temp = tINTERPOLATIONMappingTable[index].analogInput;
            }
        }
    }

    unsigned int index = sizeof(tINTERPOLATIONMappingTable) / (sizeof(tINTERPOLATIONMappingTable[0]) - 1U);
    if( index < INTERPOLATION_SIZE)
    {
        *output = (uint16_t)((voltage_temp / tINTERPOLATIONMappingTable[index].analogInput *
                  (float)tINTERPOLATIONMappingTable[index].digitalOutput));
    }
end:
    return result;
}

#endif
#endif

// =============================================================================

//                Public Function Definition

// =============================================================================

// =============================================================================

/**
 * SARADC initialization.
 *
 * @param modeAVG            set average notification.
 * @param modeStore          set DRAM storing type.
 * @param amplifyDriving     determine XAIN driving.
 * @param divider            set clock divider of SARADC.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARInitialize (
    SARADC_MODE_AVG     modeAVG,
    SARADC_MODE_STORE   modeStore,
    SARADC_AMPLIFY_GAIN amplifyDriving,
    SARADC_CLK_DIV      divider)
{
    SARADC_RESULT result = SARADC_SUCCESS;
#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
    uint8_t validXAIN   = 0x0U;
    uint8_t caliXAIN    = 0x0U;
    int     i           = 0;
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
    uint8_t validXAIN   = 0x0U;
    uint8_t caliXAIN    = 0x0U;
    int     i           = 0;
    #endif

#endif

    if ((modeAVG > SARADC_MODE_AVG_ENABLE) ||
        (modeStore > SARADC_MODE_STORE_AVG_ENABLE) ||
        (amplifyDriving > SARADC_AMPLIFY_8X) ||
        (divider > SARADC_CLK_DIV_31))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount == 0)
    {
        ithSARADCEnableClock();
        ithSARADCResetEngine();

        resetSARADCObject(&SARADCObject);
        SARADCObject.refCount       = 1;
        SARADCObject.opModeAVG      = modeAVG;
        SARADCObject.opModeStore    = modeStore;
        SARADCObject.driving        = amplifyDriving;
        SARADCObject.clkDivider     = divider;

        if (!setSARADCGPIO(&SARADCObject))
        {
            (void)printf("[SARADC][%s] setSARADCGPIO() fail\n", __func__);
            result = SARADC_ERR_INVALID_GPIO_SETTING;
            goto end;
        }

#ifdef CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
        validXAIN   = SARADCObject.validXAIN;
        caliXAIN    = SARADCObject.caliXAIN;

        if (caliXAIN > 0xFFU)
        {
            SARADCObject.caliUse    = false;
            (void)printf("[SARADC][%s] caliXAIN is out of range: %d\n", __func__, caliXAIN);
            result                  = SARADC_ERR_INVALID_XAIN;
            goto end;
        }

        while (caliXAIN != 0x0U)
        {
            if ((caliXAIN & 0x1U) == 0x1U)
            {
                i++;
            }
            caliXAIN >>= 1;
        }

        if (i < 2)
        {
            SARADCObject.caliUse    = false;
            (void)printf("[SARADC][%s] caliXAIN is 0x%x, but it must be greater than one reference\n", __func__, caliXAIN);
            result                  = SARADC_ERR_INVALID_XAIN;
            goto end;
        }

        caliXAIN = SARADCObject.caliXAIN;
        for (i = 0; i < SARADC_XAIN_MAX_COUNT; i++)
        {
            if (!(validXAIN & 0x1U) && (caliXAIN & 0x1U))
            {
                SARADCObject.caliUse    = false;
                (void)printf("[SARADC][%s] validXAIN is 0x%x, but caliXAIN is 0x%x\n",
                        __func__, SARADCObject.validXAIN, SARADCObject.caliXAIN);
                result                  = SARADC_ERR_INVALID_XAIN;
                goto end;
            }
            validXAIN   >>= 1;
            caliXAIN    >>= 1;
        }
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
        validXAIN   = SARADCObject.validXAIN;
        caliXAIN    = SARADCObject.caliXAIN;

        if (caliXAIN > 0x1FU)
        {
            SARADCObject.caliUse    = false;
            (void)printf("[SARADC][%s] caliXAIN is out of range: %d\n", __func__, caliXAIN);
            result                  = SARADC_ERR_INVALID_XAIN;
            goto end;
        }

        while (caliXAIN != 0x0U)
        {
            if ((caliXAIN & 0x1U) == 0x1U)
            {
                i++;
            }
            caliXAIN >>= 1;
        }

        if (i < 2)
        {
            SARADCObject.caliUse    = false;
            (void)printf("[SARADC][%s] caliXAIN is 0x%x, but it must be greater than one reference\n", __func__, caliXAIN);
            result                  = SARADC_ERR_INVALID_XAIN;
            goto end;
        }

        if (validXAIN == 0x0U)
        {
            SARADCObject.caliUse    = false;
            (void)printf("[SARADC][%s] validXAIN is 0x%x, but caliXAIN is 0x%x\n",
                    __func__, SARADCObject.validXAIN, SARADCObject.caliXAIN);
            result                  = SARADC_ERR_INVALID_XAIN;
            goto end;
        }
    #endif

#endif

        setSARADCGeneralReg(&SARADCObject);
        resetSARADCAllXAINReg(&SARADCObject);
    }
    else
    {
        SARADCObject.refCount++;
        (void)printf("[SARADC][%s] already initialed. refCount = %d\n", __func__, SARADCObject.refCount);
    }

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Set Write buffer size of SARADC.
 *
 * @param    wbSize set Write buffer range for SARADC accessing.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSetWriteBufferSize (
    uint16_t wbSize)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if ((wbSize < (SARADC_WB_SIZE_ALIGN * 2U)) || ((wbSize % SARADC_WB_SIZE_ALIGN) != 0U) )
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is not idle, can not reset wbSize\n", __func__);
        result = SARADC_ERR_NOT_IDLE;
        goto end;
    }

    SARADCObject.wbSize = wbSize;
    SARADC_SetWBSize_Reg(SARADCObject.wbSize - 1U);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Set average calculation count of SARADC.
 *
 * @param avgCalCount    set average calculation count for SARADC reference
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSetAVGCalCount (
    SARADC_AVG_CAL_COUNT avgCalCount)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if (avgCalCount > SARADC_AVG_CAL_COUNT_3)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is not idle, can not reset avgCalCount\n", __func__);
        result = SARADC_ERR_NOT_IDLE;
        goto end;
    }

    SARADCObject.avgCalCount = avgCalCount;
    SARADC_SetAVGCALCount_Reg(SARADCObject.avgCalCount);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Set store average count of SARADC trigger.
 *
 * @param trigStoreCount    set store average count for SARADC reference, 0 means always
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSetTRIGStoreCount (
    uint8_t trigStoreCount)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is not idle, can not reset trigStoreCount\n", __func__);
        result = SARADC_ERR_NOT_IDLE;
        goto end;
    }

    SARADCObject.trigStoreCount = trigStoreCount;
    SARADC_SetTRIGStoreCount_Reg(SARADCObject.trigStoreCount);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Select XAIN input signal of SARADC.
 *
 * @param signal    set XAIN calibration signal for SARADC reference
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSelectXAINSignal (
    SARADC_XAIN_SIGNAL signal)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if (signal > SARADC_XAIN_SIGNAL_5)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    SARADCObject.xainSignal = signal;
    SARADC_SelectXAINSignal_Reg(SARADCObject.xainSignal);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
* SARADC termination.
*
* @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
*/

// =============================================================================
SARADC_RESULT
mmpSARTerminate (
    void)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);
    SARADCObject.refCount--;
    if (SARADCObject.refCount == 0)
    {
        ithSARADCResetEngine();
        ithSARADCDisableClock();
    }
    else if (SARADCObject.refCount == -1)
    {
        SARADCObject.refCount   = 0;
        (void)printf("[SARADC][%s] SARADC has not been initialized\n", __func__);
        result                  = SARADC_ERR_NOT_INITIALIZE;
    }
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Fire SARADC Engine.
 *
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARFire (
    void)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is not idle, can not fire SARADC\n", __func__);
        result = SARADC_ERR_NOT_IDLE;
        goto end;
    }

    SARADCObject.inUse = true;
    resetSARADCAllXAINReg(&SARADCObject);
    SARADC_DriverFire_Reg();

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Stop SARADC Engine.
 *
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARStop (
    void)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (!SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is idle, can not stop SARADC\n", __func__);
        result = SARADC_ERR_IS_IDLE;
        goto end;
    }

    SARADCObject.inUse = false;
    SARADC_DriverStop_Reg();

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Set XAIN memory base of SARADC.
 *
 * @param hwPort      indicate which XAIN will be applied.
 * @param baseAddr    determine memory base of XAIN.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSetMEMBase (
    SARADC_PORT hwPort,
    uint8_t     * baseAddr)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if ((hwPort > SARADC_7) || ((uint32_t)baseAddr >= CFG_RAM_SIZE))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    SARADCObject.XAINObjects[hwPort].xainInfo.baseAddr = baseAddr;
    SARADC_SetXAINMEMBase_Reg(hwPort, SARADCObject.XAINObjects[hwPort].xainInfo.baseAddr);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Set XAIN trigger rule of SARADC.
 *
 * @param hwPort           indicate which XAIN will be applied.
 * @param trigAVG          determine average trigger type of XAIN.
 * @param maxAVGTrigger    determine max trigger rule of XAIN.
 * @param minAVGTrigger    determine min trigger rule of XAIN.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSetAVGTriggerRule (
    SARADC_PORT     hwPort,
    SARADC_TRIG_AVG trigAVG,
    uint16_t        maxAVGTrigger,
    uint16_t        minAVGTrigger)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if ((hwPort > SARADC_7) || (trigAVG > SARADC_TRIG_AVG_LEVEL) ||
        (maxAVGTrigger > 0xFFFU) || (minAVGTrigger > 0xFFFU))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    SARADCObject.XAINObjects[hwPort].xainInfo.trigAVG       = trigAVG;
    SARADCObject.XAINObjects[hwPort].xainInfo.maxAVGTrigger = maxAVGTrigger;
    SARADCObject.XAINObjects[hwPort].xainInfo.minAVGTrigger = minAVGTrigger;
    SARADC_SetXAINTriggerRule_Reg(hwPort,
            SARADCObject.XAINObjects[hwPort].xainInfo.trigAVG,
            SARADCObject.XAINObjects[hwPort].xainInfo.maxAVGTrigger,
            SARADCObject.XAINObjects[hwPort].xainInfo.minAVGTrigger);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Set XAIN detection rule of SARADC.
 *
 * @param hwPort          indicate which XAIN will be applied.
 * @param maxAVGDetect    determine max detection rule of XAIN.
 * @param minAVGDetect    determine min detection rule of XAIN.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSetAVGDetectRule (
    SARADC_PORT hwPort,
    uint16_t    maxAVGDetect,
    uint16_t    minAVGDetect)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if ((hwPort > SARADC_7) || (maxAVGDetect > 0xFFFU) || (minAVGDetect > 0xFFFU))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    SARADCObject.XAINObjects[hwPort].xainInfo.maxAVGDetect  = maxAVGDetect;
    SARADCObject.XAINObjects[hwPort].xainInfo.minAVGDetect  = minAVGDetect;
    SARADC_SetXAINDetectRule_Reg(hwPort,
            SARADCObject.XAINObjects[hwPort].xainInfo.maxAVGDetect,
            SARADCObject.XAINObjects[hwPort].xainInfo.minAVGDetect);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Set SARADC Read pointer.
 *
 * @param hwPort    indicate which XAIN will be applied.
 * @param rptr      determine Read pointer offset of XAIN.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARSetReadPointer (
    SARADC_PORT hwPort,
    uint16_t    rptr)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if (hwPort > SARADC_7)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    SARADCObject.XAINObjects[hwPort].xainInfo.rptr = rptr;
    SARADC_SetXAINRPTR_Reg(hwPort, SARADCObject.XAINObjects[hwPort].xainInfo.rptr);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Read SARADC Write pointer.
 *
 * @param hwPort    indicate which XAIN will be applied.
 * @param wptr      return Write pointer offset that XAIN recorded.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARReadWritePointer (
    SARADC_PORT hwPort,
    uint16_t    * wptr)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if ((hwPort > SARADC_7) || (wptr == NULL))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    SARADC_ReadXAINWPTR_Reg(hwPort, wptr);

    return result;
}

// =============================================================================

/**
 * Read SARADC average register.
 *
 * @param hwPort    indicate which XAIN will be applied.
 * @param avg       return average value that XAIN recorded.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARReadAVGREG (
    SARADC_PORT hwPort,
    uint16_t    * avg)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    if (hwPort > SARADC_7)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    SARADC_ReadXAINAVGREG_Reg(hwPort, avg);

    return result;
}

// =============================================================================

/**
 * Read SARADC Write pointer.
 *
 * @param hwPort    indicate which XAIN will be checked.
 * @return true if memory base has been overwritten, false otherwise.
 */

// =============================================================================
bool
mmpSARIsOverwritingMEM (
    SARADC_PORT hwPort)
{
    if (hwPort > SARADC_7)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        return false;
    }

    return SARADC_IsXAINOVERWT_Reg(hwPort);
}

// =============================================================================

/**
 * Check average detection Interrupt of SARADC.
 *
 * @param hwPort    indicate which XAIN will be checked.
 * @return SARADC_INTR_AVG_VALID if digtal value was within valid range,
 * SARADC_INTR_AVG_ABOMAX and SARADC_INTR_AVG_UNDMIN otherwise.
 */

// =============================================================================
SARADC_INTR_AVG
mmpSARIsOutOfRange (
    SARADC_PORT hwPort)
{
    if (hwPort > SARADC_7)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        return SARADC_INTR_AVG_VALID;
    }

    return SARADC_IsXAINOutOfRange_Reg(hwPort);
}

// =============================================================================

/**
 * Check Event occurrence Interrupt of SARADC.
 *
 * @param hwPort    indicate which XAIN will be checked.
 * @return true if event has been occurred, false otherwise.
 */

// =============================================================================
bool
mmpSARIsEventOccurrence (
    SARADC_PORT hwPort)
{
    if (hwPort > SARADC_7)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        return false;
    }

    return SARADC_IsXAINEVENTOccurrence_Reg(hwPort);
}

// =============================================================================

/**
 * Reset SARADC XAIN notification.
 *
 * @param hwPort    indicate which XAIN will be checked.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARResetXAINNotification (
    SARADC_PORT hwPort)
{
    SARADC_RESULT result = SARADC_SUCCESS;
    SARADC_PORT   i = hwPort;

    if (hwPort > SARADC_7)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    SARADC_ResetXAINOVERWT_Reg(i);
    SARADC_ResetXAINAVGDetect_Reg(i);
    SARADC_ResetXAINEVENT_Reg(i);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * Reset SARADC engine.
 *
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARReset (
    void)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    ithSARADCResetEngine();

    return result;
}

// =============================================================================

/**
 * Enable SARADC XAIN set.
 *
 * @param enXAIN    the bits stand for XAIN numbers from low bit to high bit.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSAREnableXAIN (
    uint8_t enXAIN)
{
    SARADC_RESULT result = SARADC_SUCCESS;
    uint8_t       validXAIN = 0x0U;
    uint8_t       enXAINCheck = enXAIN;
    unsigned int  i = 0U;

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    validXAIN = SARADCObject.validXAIN;

    for (i = 0U; i < SARADC_XAIN_MAX_COUNT; i++)
    {
        if (((validXAIN & 0x1U) == 0U) && ((enXAINCheck & 0x1U) == 1U))
        {
            (void)printf("[SARADC][%s] SARADC validXAIN is 0x%x, can not re-enable 0x%x set\n",
                    __func__, SARADCObject.validXAIN, enXAIN);
            result = SARADC_ERR_INVALID_XAIN;
            goto end;
        }
        validXAIN >>= 1U;
        enXAINCheck >>= 1U;
    }

    SARADCObject.enableXAIN = enXAIN;
    assignSARADCINTR(&SARADCObject);
    SARADC_SetValidXAIN_Reg(SARADCObject.enableXAIN);

end:
    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
}

// =============================================================================

/**
 * SARADC conversion.
 *
 * @param hwPort    indicate which XAIN will be applied.
 * @param wbSize    set Write buffer range for SARADC accessing.
 * @param avg       return average value that XAIN recorded.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARConvert (
    SARADC_PORT hwPort,
    uint16_t    wbSize,
    uint16_t    * avg)
{
#if CFG_SARADC_CALIBRATION_ENABLE

    #ifdef CFG_SARADC_CALIBRATION_EXTERNAL
    SARADC_RESULT   result          = SARADC_SUCCESS;
    uint8_t         * writeBuffer   = NULL;
    uint16_t        read_ptr        = 0U;
    uint16_t        write_ptr       = 0U;
    uint32_t        printf_count    = 0U;
    uint8_t         caliXAIN        = 0x0U;
    int             i               = 0;
    int             k               = 0;

    if ((hwPort > SARADC_7) ||
                (wbSize < SARADC_WB_SIZE_ALIGN * 2) || (wbSize % SARADC_WB_SIZE_ALIGN) != 0 || !avg)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (!SARADCObject.caliUse)
    {
        (void)printf("[SARADC][%s] validXAIN is 0x%x, but caliXAIN is 0x%x\n",
                    __func__, SARADCObject.validXAIN, SARADCObject.caliXAIN);
        result = SARADC_ERR_INVALID_XAIN;
        goto end;
    }

    if (SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is not idle, can not set XAIN information\n", __func__);
        result = SARADC_ERR_NOT_IDLE;
        goto end;
    }

    if (result = mmpSARSetWriteBufferSize(wbSize))
    {
        goto end;
    }

    writeBuffer = (uint8_t *)itpVmemAlignedAlloc(32, wbSize);
    if (!writeBuffer)
    {
        (void)printf("[SARADC][%s] out of memory\n", __func__);
        result = SARADC_ERR_ALLOCATION_FAIL;
        goto end;
    }

    if (result = mmpSARSetMEMBase(hwPort, writeBuffer))
    {
        goto end;
    }

    if (result = mmpSARSetAVGDetectRule(hwPort, 0, 0))
    {
        goto end;
    }

    if (result = mmpSARSetAVGTriggerRule(hwPort, SARADC_TRIG_AVG_ALWAYS, 0, 0))
    {
        goto end;
    }

    if (result = mmpSAREnableXAIN(0x0 << hwPort))
    {
        goto end;
    }

    if (result = mmpSARFire())
    {
        goto end;
    }

    caliXAIN = SARADCObject.caliXAIN;

    while (caliXAIN != 0x0U)
    {
        if ((caliXAIN & 0x1U) == 0x1U)
        {
            if (result = mmpSARSetMEMBase(i, writeBuffer))
            {
                goto end;
            }

            if (result = mmpSARSetAVGDetectRule(i, 0, 0))
            {
                goto end;
            }

            if (result = mmpSARSetAVGTriggerRule(i, SARADC_TRIG_AVG_ALWAYS, 0, 0))
            {
                goto end;
            }

            if (result = mmpSARResetXAINNotification(i))
            {
                goto end;
            }

            (void)memset(writeBuffer, 0, wbSize);
            ithFlushDCacheRange(writeBuffer, wbSize);

            if (result = mmpSARSelectXAINSignal(SARADC_XAIN_SIGNAL_0))
            {
                goto end;
            }

            if (result = mmpSARReadWritePointer(i, &read_ptr))
            {
                goto end;
            }

            if (result = mmpSARSetReadPointer(i, read_ptr))
            {
                goto end;
            }

            if (result = mmpSAREnableXAIN(0x1 << i))
            {
                goto end;
            }

            // (void)printf("[SARADC] SARADC is writing memory ");
            printf_count = 0;
            while (mmpSARIsOverwritingMEM(i) != true)
            {
                if (printf_count % (100000U) == 0U)
                {
#if 0
                    (void)printf(".");
                    fflush(stdout);
#endif
                }
                printf_count++;
                usleep(1);
            }
#if 0
            (void)printf("\n");
            (void)printf("[SARADC] finish!\n");
#endif
            if (result = mmpSAREnableXAIN(0x0U << i))
            {
                goto end;
            }

            if (result = mmpSARReadWritePointer(i, &write_ptr))
            {
                goto end;
            }

            ithInvalidateDCacheRange((void *)writeBuffer, wbSize);

            if (result = mmpSARCollectOutput(writeBuffer, wbSize, read_ptr, write_ptr, avg))
            {
                goto end;
            }

            tCALIBRATIONMappingTable[i].analogInput     = SARADCObject.caliVoltage[k];
            tCALIBRATIONMappingTable[i].digitalOutput   = *avg;

            k++;
        }
        caliXAIN >>= 1;
        i++;
    }

    if (result = mmpSARResetXAINNotification(hwPort))
    {
        goto end;
    }

    (void)memset(writeBuffer, 0, wbSize);
    ithFlushDCacheRange(writeBuffer, wbSize);

    if (result = mmpSARSelectXAINSignal(SARADC_XAIN_SIGNAL_0))
    {
        goto end;
    }

    if (result = mmpSARReadWritePointer(hwPort, &read_ptr))
    {
        goto end;
    }

    if (result = mmpSARSetReadPointer(hwPort, read_ptr))
    {
        goto end;
    }

    if (result = mmpSAREnableXAIN(0x1U << hwPort))
    {
        goto end;
    }

    // (void)printf("[SARADC] SARADC is writing memory ");
    printf_count = 0;
    while (mmpSARIsOverwritingMEM(hwPort) != true)
    {
        if ((printf_count % 100000U) == 0U)
        {
#if 0
            (void)printf(".");
            fflush(stdout);
#endif
        }
        printf_count++;
        usleep(1);
    }
#if 0
    (void)printf("\n");
    (void)printf("[SARADC] finish!\n");
#endif

    if (result = mmpSARStop())
    {
        goto end;
    }

    if (result = mmpSARReadWritePointer(hwPort, &write_ptr))
    {
        goto end;
    }

    ithInvalidateDCacheRange((void *)writeBuffer, wbSize);

    if (result = mmpSARCollectOutput(writeBuffer, wbSize, read_ptr, write_ptr, avg))
    {
        goto end;
    }

    if (result = mmpSARDynamicallyCalibrate(*avg, avg))
    {
        goto end;
    }

end:
    if (writeBuffer)
    {
        itpVmemFree((uint32_t)writeBuffer);
    }

    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
    #endif

    #ifdef CFG_SARADC_CALIBRATION_INTERNAL
    SARADC_RESULT   result          = SARADC_SUCCESS;
    uint8_t         * writeBuffer   = NULL;
    uint16_t        read_ptr        = 0x0U;
    uint16_t        write_ptr       = 0x0U;
    uint32_t        printf_count    = 0x0U;
    uint8_t         caliXAIN        = 0x0U;
    int             i               = 0;
    int             k               = 0;

        if ((hwPort > SARADC_7) ||
            (wbSize < (SARADC_WB_SIZE_ALIGN * 2u)) ||
            ((wbSize % SARADC_WB_SIZE_ALIGN) != 0u) ||
            (avg == NULL))
        {
            (void)printf("[SARADC][%s] parameter fail\n", __func__);
            result = SARADC_ERR_INVALID_INPUT_PARAM;
            return result;
        }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (!SARADCObject.caliUse)
    {
        (void)printf("[SARADC][%s] validXAIN is 0x%x, but caliXAIN is 0x%x\n",
                    __func__, SARADCObject.validXAIN, SARADCObject.caliXAIN);
        result = SARADC_ERR_INVALID_XAIN;
        goto end;
    }

    if (SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is not idle, can not set XAIN information\n", __func__);
        result = SARADC_ERR_NOT_IDLE;
        goto end;
    }

    result = mmpSARSetWriteBufferSize(wbSize);
    if (result != SARADC_SUCCESS)
	{
	    goto end;
    }

    writeBuffer = (uint8_t *)itpVmemAlignedAlloc(32, wbSize);
    if (writeBuffer == NULL)
    {
        (void)printf("[SARADC][%s] out of memory\n", __func__);
        result = SARADC_ERR_ALLOCATION_FAIL;
        goto end;
    }

    result = mmpSARSetMEMBase(hwPort, writeBuffer);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARSetAVGDetectRule(hwPort, 0, 0);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARSetAVGTriggerRule(hwPort, SARADC_TRIG_AVG_ALWAYS, 0, 0);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSAREnableXAIN(0x0U << (uint8_t)hwPort);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARFire();
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    caliXAIN = SARADCObject.caliXAIN;

    while (caliXAIN != 0x0U)
    {
        if ((caliXAIN & 0x1U) == 0x1U)
        {
			result = mmpSARResetXAINNotification(hwPort);
            if (result != SARADC_SUCCESS)
            {
                goto end;
            }

            (void)memset(writeBuffer, 0, wbSize);
            ithFlushDCacheRange(writeBuffer, wbSize);

            result = mmpSARSelectXAINSignal((SARADC_XAIN_SIGNAL)(i + 1));
            if (result != SARADC_SUCCESS)
            {
                goto end;
            }

            result = mmpSARReadWritePointer(hwPort, &read_ptr);
            if (result != SARADC_SUCCESS)
            {
                goto end;
            }

            result = mmpSARSetReadPointer(hwPort, read_ptr);
            if (result != SARADC_SUCCESS)
            {
                goto end;
            }

            result = mmpSAREnableXAIN(0x1U << (uint8_t)hwPort);
            if (result != SARADC_SUCCESS)
            {
                goto end;
            }

            // (void)printf("[SARADC] SARADC is writing memory ");
            printf_count = 0;
            while (mmpSARIsOverwritingMEM(hwPort) != true)
            {
                if ((printf_count % 100000U) == 0U)
                {
#if 0
                    (void)printf(".");
                    fflush(stdout);
#endif
                }
                printf_count++;
#if 0
                usleep(1);
#endif
            }
#if 0
            (void)printf("\n");
            (void)printf("[SARADC] finish!\n");
#endif
            result = mmpSAREnableXAIN(0x0U << ((unsigned int)hwPort));
            if (result != SARADC_SUCCESS)
			{
			    goto end;
            }

            result = mmpSARReadWritePointer(hwPort, &write_ptr);
            if (result != SARADC_SUCCESS)
            {
                goto end;
            }

            ithInvalidateDCacheRange((void *)writeBuffer, wbSize);

            result = mmpSARCollectOutput(writeBuffer, wbSize, read_ptr, write_ptr, avg);
            if (result != SARADC_SUCCESS)
            {
                goto end;
            }

            tCALIBRATIONMappingTable[i].analogInput     = SARADCObject.caliVoltage[i];
            tCALIBRATIONMappingTable[i].digitalOutput   = *avg;

            k++;
        }
        caliXAIN >>= 1;
        i++;
    }

    result = mmpSARResetXAINNotification(hwPort);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    (void)memset(writeBuffer, 0, wbSize);
    ithFlushDCacheRange(writeBuffer, wbSize);

    result = mmpSARSelectXAINSignal(SARADC_XAIN_SIGNAL_0);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARReadWritePointer(hwPort, &read_ptr);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARSetReadPointer(hwPort, read_ptr);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSAREnableXAIN(0x1U << (uint8_t)hwPort);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    printf_count = 0;
    while (mmpSARIsOverwritingMEM(hwPort) != true)
    {
        if ((printf_count % 100000U) == 0U)
        {
#if 0
            (void)printf(".");
            fflush(stdout);
#endif
        }
        printf_count++;
#if 0
        usleep(1);
#endif
    }
#if 0
    (void)printf("\n");
    (void)printf("[SARADC] finish!\n");
#endif

    result = mmpSARStop();
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARReadWritePointer(hwPort, &write_ptr);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    ithInvalidateDCacheRange((void *)writeBuffer, wbSize);

    result = mmpSARCollectOutput(writeBuffer, wbSize, read_ptr, write_ptr, avg);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARDynamicallyCalibrate(*avg, avg);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

end:
    if (writeBuffer != NULL)
    {
        itpVmemFree((uint32_t)writeBuffer);
    }

    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;
    #endif

#else

    SARADC_RESULT   result          = SARADC_SUCCESS;
    uint8_t         * writeBuffer   = NULL;
    uint16_t        read_ptr        = 0;
    uint16_t        write_ptr       = 0;
    uint32_t        printf_count    = 0;

    if (hwPort < SARADC_0 || hwPort > SARADC_7 ||
            wbSize < SARADC_WB_SIZE_ALIGN * 2 || (wbSize % SARADC_WB_SIZE_ALIGN) != 0 || !avg)
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        return result;
    }

    (void)pthread_mutex_lock(&SARADCObject.thread_mutex);

    if (SARADCObject.refCount <= 0)
    {
        (void)printf("[SARADC][%s] SARADC has not been initialized, can not use API\n", __func__);
        result = SARADC_ERR_NOT_INITIALIZE;
        goto end;
    }

    if (SARADCObject.inUse)
    {
        (void)printf("[SARADC][%s] SARADC is not idle, can not set XAIN information\n", __func__);
        result = SARADC_ERR_NOT_IDLE;
        goto end;
    }

    result = mmpSAREnableXAIN(0x1U << hwPort);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    result = mmpSARSetWriteBufferSize(wbSize);
    if (result != SARADC_SUCCESS)
    {
        goto end;
    }

    writeBuffer = (uint8_t *)itpVmemAlignedAlloc(32, wbSize);
    if (!writeBuffer)
    {
        (void)printf("[SARADC][%s] out of memory\n", __func__);
        result = SARADC_ERR_ALLOCATION_FAIL;
        goto end;
    }
    else
    {
        (void)memset(writeBuffer, 0, wbSize);
        ithFlushDCacheRange(writeBuffer, wbSize);
    }

    result = mmpSARSetMEMBase(hwPort, writeBuffer);
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARResetXAINNotification(hwPort);
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARReadWritePointer(hwPort, &read_ptr);
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARSetReadPointer(hwPort, read_ptr)
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARSetAVGDetectRule(hwPort, 0, 0); 
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARSetAVGTriggerRule(hwPort, SARADC_TRIG_AVG_ALWAYS, 0, 0);
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARFire();
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    printf_count = 0;
    while (mmpSARIsOverwritingMEM(hwPort) != true)
    {
        if ((printf_count % 100000U) == 0U)
        {
#if 0
            (void)printf(".");
            fflush(stdout);
#endif
        }
        printf_count++;
#if 0
        usleep(1);
#endif
    }
#if 0
    (void)printf("\n");
    (void)printf("[SARADC] finish!\n");
#endif

    result = mmpSARStop();
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARReadWritePointer(hwPort, &write_ptr);
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    ithInvalidateDCacheRange((void *)writeBuffer, wbSize);

    result = mmpSARCollectOutput(writeBuffer, wbSize, read_ptr, write_ptr, avg);
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

    result = mmpSARTableCalibrate(*avg, avg);
    if (result != SARADC_SUCCESS)
	{
        goto end;
    }

end:
    if (writeBuffer != NULL)
    {
        itpVmemFree((uint32_t)writeBuffer);
    }

    (void)pthread_mutex_unlock(&SARADCObject.thread_mutex);

    return result;

#endif
}

// =============================================================================

/**
 * Collect SARADC output.
 *
 * @param baseAddr    indicate memory base of XAIN.
 * @param wbSize      indicate Write buffer range for SARADC accessing.
 * @param rptr        indicate Read pointer offset that XAIN recorded.
 * @param wptr        indicate Write pointer offset that XAIN recorded.
 * @param avg         return average value that XAIN recorded.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARCollectOutput (
    uint8_t     * baseAddr,
    uint16_t    wbSize,
    uint16_t    rptr,
    uint16_t    wptr,
    uint16_t    * avg)
{
    SARADC_RESULT   result  = SARADC_SUCCESS;
    uint16_t        buffer  = 0U;
    uint16_t        minimum = 0x0U;
    uint16_t        maximum = 0xFFFU;
    uint16_t        index   = 0U;
    uint32_t        average = 0x0U;

    if ((baseAddr == NULL) || (wbSize < (SARADC_WB_SIZE_ALIGN * 2U)) ||((wbSize % SARADC_WB_SIZE_ALIGN) != 0U) ||
         (avg == NULL) || (rptr == wptr))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        goto end;
    }

    if ((rptr >= wbSize) || (wptr >= wbSize))
    {
        (void)printf("[SARADC][%s][%d] RPTR or WPTR offset is out of range\n", __func__, __LINE__);
        result = SARADC_ERR_OFFSET_OUT_OF_RANGE;
        goto end;
    }

    rptr += 192U;
    if (rptr >= wbSize)
    {
        rptr -= wbSize;
    }

    while (rptr != wptr)
    {
        if ((rptr % 2U) == 0U)
        {
            buffer = *(baseAddr + rptr) & 0xFFU;
        }
        else
        {
            buffer |= (*(baseAddr + rptr) & 0xFFU) << 8;
            index++;
#if 0
            (void)printf("[%d,%d]:%x\n", index++, rptr, buffer);
#endif
            if (index == 1U)
            {
                minimum = buffer;
                maximum = buffer;
                average += buffer;
            }
            else
            {
                if (buffer < minimum)
                {
                    minimum = buffer;
                }
                else if (buffer > maximum)
                {
                    maximum = buffer;
                }
                average += buffer;
            }
        }

        rptr++;
        if (rptr == wbSize)
        {
            rptr = 0U;
        }
    }

    if (index > 0U)
    {
        *avg = (uint16_t)(average / index);
    }

    // (void)printf("[SARADC] %d outputs\n[SARADC] min:%x\n[SARADC] max:%x\n[SARADC] average:%x\n", index, minimum, maximum, *avg);

end:
    return result;
}

// =============================================================================

/**
 * SARADC table calibration.
 *
 * @param input     indicate real SARADC output.
 * @param output    return table calibration output which is calculated by input.
 * @return SARADC_SUCCESS if succeed, error codes of SARADC_ERR otherwise.
 */

// =============================================================================
SARADC_RESULT
mmpSARTableCalibrate (
    uint16_t    input,
    uint16_t    * output)
{
    SARADC_RESULT result = SARADC_SUCCESS;
    float         ratio_temp = 0.0F;
    uint16_t      real_index = 0U;
    uint16_t      trans_index = 0U;

    if ((input > 0xFFFU) || (output == NULL))
    {
        (void)printf("[SARADC][%s] parameter fail\n", __func__);
        result = SARADC_ERR_INVALID_INPUT_PARAM;
        goto end;
    }

    for (real_index = 0U; real_index < (sizeof(tREALMappingTable) / sizeof(tREALMappingTable[0])); real_index++)
    {
        if (tREALMappingTable[real_index].digitalOutput >= input)
        {
            break;
        }
    }

    if (real_index != 0x0U)
    {
        if (real_index == sizeof(tREALMappingTable) / sizeof(tREALMappingTable[0]))
        {
            (void)printf("[SARADC][%s][%d] calibration fail\n", __func__, __LINE__);
            result = SARADC_ERR_INVALID_CALIBRATION;
            goto end;
        }

        ratio_temp = ((float)input - (float)tREALMappingTable[real_index - 1u].digitalOutput) /
                ((float)tREALMappingTable[real_index].digitalOutput - (float)tREALMappingTable[real_index - 1U].digitalOutput);
    }

    for (trans_index = 0U; trans_index < (sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0])); trans_index++)
    {
        if (tINTERPOLATIONMappingTable[trans_index].analogInput == tREALMappingTable[real_index].analogInput)
        {
            if (real_index != 0U)
            {
                if(trans_index > 0U)
                {
                    *output = tINTERPOLATIONMappingTable[trans_index - 1u].digitalOutput +
                        (uint16_t)(ratio_temp * (float)(tINTERPOLATIONMappingTable[trans_index].digitalOutput - tINTERPOLATIONMappingTable[trans_index - 1U].digitalOutput));
                }
            }
            else
            {
                *output = tINTERPOLATIONMappingTable[trans_index].digitalOutput;
            }

            break;
        }
    }

    if (trans_index == (sizeof(tINTERPOLATIONMappingTable) / sizeof(tINTERPOLATIONMappingTable[0])))
    {
        (void)printf("[SARADC][%s][%d] calibration fail\n", __func__, __LINE__);
        result = SARADC_ERR_INVALID_CALIBRATION;
        goto end;
    }

end:
    return result;
}
