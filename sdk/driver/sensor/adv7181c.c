#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "iic/mmp_iic.h"
#include "adv7181c.h"

// =============================================================================
//                Constant Definition
// =============================================================================
static uint8_t ADV7181_ADDR = 0x42;
#ifdef CFG_SENSOR_ENABLE
    #define ADV7181_IIC_PORT    CFG_SENSOR_IIC_PORT
#else
    #define ADV7181_IIC_PORT    IIC_PORT_2
#endif
// =============================================================================
//                Macro Definition
// =============================================================================

// =============================================================================
//                Structure Definition
// =============================================================================
typedef struct _REGPAIR
{
    uint8_t     addr;
    uint16_t    value;
} REGPAIR;

typedef struct ADV7181SensorDriverStruct
{
    SensorDriverStruct base;
} ADV7181SensorDriverStruct;

// =============================================================================
//                Global Data Definition
// =============================================================================
static int      gMasterDev          = 0;
static uint16_t ADV7181_InWidth     = 640;
static uint16_t ADV7181_InHeight    = 480;
static uint16_t ADV7181_InFrameRate = 6000;
static bool     ADV7181_Initialed   = 0;

// =============================================================================
//                Private Function Definition
// =============================================================================
// =============================================================================
//                IIC API FUNCTION START
// =============================================================================
void
DRV_ADV7181C_I2c_init ()
{
    char portname[16] = {0};

    sprintf(portname, ":i2c%d", ADV7181_IIC_PORT);
    gMasterDev = open(portname, 0, 0);
}

uint8_t
DRV_ADV7181C_I2CRead (
    uint8_t Addr,
    uint8_t RegAddr)
{
    uint8_t     dbuf[2] = {0};
    int         result  = 0;
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt;

    dbuf[0]             = (uint8_t)(RegAddr);
    pdbuf++;

    evt.slaveAddress    = (Addr >> 1);
    evt.cmdBuffer       = dbuf;
    evt.cmdBufferSize   = 1;
    evt.dataBuffer      = pdbuf;
    evt.dataBufferSize  = 1;

    result              = read(gMasterDev, (char *)&evt, 1);

    if (result != 0)
    {
        (void)printf("_ADV7181_ReadI2c_Byte read address 0x%02x error!\n", RegAddr);
    }

    return dbuf[1];
}

uint32_t
DRV_ADV7181C_I2CWrite (
    uint8_t Addr,
    uint8_t RegAddr,
    uint8_t data)
{
    uint8_t     dbuf[2] = {0};
    int         result  = 0;
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt     = {0};

    *pdbuf++            = (uint8_t)(RegAddr & 0xff);
    *pdbuf              = (uint8_t)(data);

    evt.slaveAddress    = (Addr >> 1);
    evt.cmdBuffer       = dbuf;
    evt.cmdBufferSize   = 2;

    while (1)
    {
        result = write(gMasterDev, (char *)&evt, 1);

        if (result != 0)
        {
            (void)printf("_ADV7181_WriteI2c_Byte Write Error, reg=%02x val=%02x\n", RegAddr, data);
        }
        else
        {
            break;
        }
    }

    return result;
}

// =============================================================================
//                IIC API FUNCTION END
// =============================================================================

REGPAIR VGA60Hz[] = {
    {0x05, 0x02},   // Prim_Mode =010b for GR
    {0xC3, 0x46},   // ADC1 to Ain4, ADC0 to Ain6,
    {0xC4, 0xB5},   // ADC2 to Ain5 and enables manual override of mux
    {0x06, 0x08},   // VID_STD=00008 for 640x480 _@60
    {0x1D, 0x47},   // Enable 28MHz Crystal
    {0x3A, 0x11},
    {0x3B, 0x81},
    {0x3C, 0x5C},   // PLL_QPUMP
    {0x6A, 0x00},   // DLL Phase Adjust
    {0x52, 0x00},   // Colour Space Conversion from RGB->YCrCb
    {0x53, 0x00},   // CSC
    {0x54, 0x12},   // CSC
    {0x55, 0x90},   // CSC
    {0x56, 0x38},   // CSC
    {0x57, 0x69},   // CSC
    {0x58, 0x48},   // CSC
    {0x59, 0x08},   // CSC
    {0x5A, 0x00},   // CSC
    {0x5B, 0x75},   // CSC
    {0x5C, 0x21},   // CSC
    {0x5D, 0x00},   // CSC
    {0x5E, 0x1A},   // CSC
    {0x5F, 0xB8},   // CSC
    {0x60, 0x08},   // CSC
    {0x61, 0x00},   // CSC
    {0x62, 0x20},   // CSC
    {0x63, 0x03},   // CSC
    {0x64, 0xD7},   // CSC
    {0x65, 0x19},   // CSC
    {0x66, 0x48},   // CSC last
    {0x67, 0x05},
    {0x6B, 0x83},   // DE ON,16 bit output
    {0x73, 0xD0},   // Set man_gain
    {0x74, 0x04},
    {0x75, 0x01},
    {0x76, 0x00},
    {0x77, 0x04},
    {0x78, 0x08},
    {0x79, 0x02},
    {0x7A, 0x00},
    {0x7B, 0x1D},// TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
    {0x7C, 0xC0},
    {0x7D, 0x00},
    {0x7E, 0x00},
    {0xC9, 0x00},
    {0x85, 0x03},
    {0x86, 0x0B},
    {0xF4, 0x2A},   // Medium High Drive Strength
    {0x0E, 0x80},   // User Sub Map Enable
    {0x52, 0x46},
    {0x54, 0x00},
    {0x0E, 0x00},   // User Map Enable
    {0x68, 0x01},
};
REGPAIR SVGA56Hz[] = {
    {0x05, 0x02},   // Prim_Mode =010b for GR
    {0xC3, 0x46},   // ADC1 to Ain4, ADC0 to Ain6,
    {0xC4, 0xB5},   // ADC2 to Ain5 and enables manual override of mux
    {0x06, 0x00},   // VID_STD=00000 for 800x600_@56
    {0x1D, 0x47},   // Enable 28MHz Crystal
    {0x3A, 0x11},
    {0x3B, 0x81},
    {0x3C, 0x5D},   // PLL_QPUMP to 101b
    {0x6A, 0x00},   // DLL Phase Adjust
    {0x52, 0x00},   // Colour Space Conversion from RGB->YCrCb
    {0x53, 0x00},   // CSC
    {0x54, 0x12},   // CSC
    {0x55, 0x90},   // CSC
    {0x56, 0x38},   // CSC
    {0x57, 0x69},   // CSC
    {0x58, 0x48},   // CSC
    {0x59, 0x08},   // CSC
    {0x5A, 0x00},   // CSC
    {0x5B, 0x75},   // CSC
    {0x5C, 0x21},   // CSC
    {0x5D, 0x00},   // CSC
    {0x5E, 0x1A},   // CSC
    {0x5F, 0xB8},   // CSC
    {0x60, 0x08},   // CSC
    {0x61, 0x00},   // CSC
    {0x62, 0x20},   // CSC
    {0x63, 0x03},   // CSC
    {0x64, 0xD7},   // CSC
    {0x65, 0x19},   // CSC
    {0x66, 0x48},   // CSC last
    {0x67, 0x05},
    {0x6B, 0x83},   // DE ON,16 bit output
    {0x73, 0xD0},   // Set man_gain
    {0x74, 0x04},
    {0x75, 0x01},
    {0x76, 0x00},
    {0x77, 0x04},
    {0x78, 0x08},
    {0x79, 0x02},
    {0x7A, 0x00},
    {0x7B, 0x1D},// TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
    {0x7C, 0xC0},
    {0x7D, 0x00},
    {0x7E, 0x00},
    {0xC9, 0x00},
    {0x85, 0x03},
    {0x86, 0x0B},
    {0xF4, 0x2A},   // Medium High Drive Strength
    {0x0E, 0x80},   // User Sub Map Enable
    {0x52, 0x46},
    {0x54, 0x00},
    {0x0E, 0x00},   // User Map Enable
    {0x68, 0x01},
};

REGPAIR SVGA60Hz[] = {
    {0x05, 0x02},   // Prim_Mode =010b for GR
    {0xC3, 0x46},   // ADC1 to Ain4, ADC0 to Ain6,
    {0xC4, 0xB5},   // ADC2 to Ain5 and enables manual override of mux
    {0x06, 0x01},   // VID_STD=00001 for 800x600_@60
    {0x1D, 0x47},   // Enable 28MHz Crystal
    {0x3A, 0x11},
    {0x3B, 0x81},
    {0x3C, 0x5D},   // PLL_QPUMP to 101b
    {0x6A, 0x00},   // DLL Phase Adjust
    {0x52, 0x00},   // Colour Space Conversion from RGB->YCrCb
    {0x53, 0x00},   // CSC
    {0x54, 0x12},   // CSC
    {0x55, 0x90},   // CSC
    {0x56, 0x38},   // CSC
    {0x57, 0x69},   // CSC
    {0x58, 0x48},   // CSC
    {0x59, 0x08},   // CSC
    {0x5A, 0x00},   // CSC
    {0x5B, 0x75},   // CSC
    {0x5C, 0x21},   // CSC
    {0x5D, 0x00},   // CSC
    {0x5E, 0x1A},   // CSC
    {0x5F, 0xB8},   // CSC
    {0x60, 0x08},   // CSC
    {0x61, 0x00},   // CSC
    {0x62, 0x20},   // CSC
    {0x63, 0x03},   // CSC
    {0x64, 0xD7},   // CSC
    {0x65, 0x19},   // CSC
    {0x66, 0x48},   // CSC last
    {0x67, 0x05},
    {0x6B, 0x83},   // DE ON,16 bit output
    {0x73, 0xD0},   // Set man_gain
    {0x74, 0x04},
    {0x75, 0x01},
    {0x76, 0x00},
    {0x77, 0x04},
    {0x78, 0x08},
    {0x79, 0x02},
    {0x7A, 0x00},
    {0x7B, 0x1D},// TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
    {0x7C, 0xC0},
    {0x7D, 0x00},
    {0x7E, 0x00},
    {0xC9, 0x00},
    {0x85, 0x03},
    {0x86, 0x0B},
    {0xF4, 0x2A},   // Medium High Drive Strength
    {0x0E, 0x80},   // User Sub Map Enable
    {0x52, 0x46},
    {0x54, 0x00},
    {0x0E, 0x00},   // User Map Enable
    {0x68, 0x01},
};

REGPAIR XVGA60Hz[] = {
    {0x05, 0x02},   // Prim_Mode =010b for GR
    {0xC3, 0x46},   // ADC1 to Ain4, ADC0 to Ain6,
    {0xC4, 0xB5},   // ADC2 to Ain5 and enables manual override of mux
    {0x06, 0x0C},   // VID_STD=0000C for 1024x768 _@60
    {0x1D, 0x47},   // Enable 28MHz Crystal
    {0x3A, 0x11},
    {0x3B, 0x81},   // Enable External Bias
    {0x3C, 0x5D},   // PLL_QPUMP to 101b
    {0x6A, 0x00},   // DLL Phase Adjust
    {0x52, 0x00},   // Colour Space Conversion from RGB->YCrCb
    {0x53, 0x00},   // CSC
    {0x54, 0x12},   // CSC
    {0x55, 0x90},   // CSC
    {0x56, 0x38},   // CSC
    {0x57, 0x69},   // CSC
    {0x58, 0x48},   // CSC
    {0x59, 0x08},   // CSC
    {0x5A, 0x00},   // CSC
    {0x5B, 0x75},   // CSC
    {0x5C, 0x21},   // CSC
    {0x5D, 0x00},   // CSC
    {0x5E, 0x1A},   // CSC
    {0x5F, 0xB8},   // CSC
    {0x60, 0x08},   // CSC
    {0x61, 0x00},   // CSC
    {0x62, 0x20},   // CSC
    {0x63, 0x03},   // CSC
    {0x64, 0xD7},   // CSC
    {0x65, 0x19},   // CSC
    {0x66, 0x48},   // CSC last
    {0x67, 0x05},
    {0x6B, 0x83},   // DE ON,16 bit output
    {0x73, 0xD0},   // Set man_gain
    {0x74, 0x04},
    {0x75, 0x01},
    {0x76, 0x00},
    {0x77, 0x04},
    {0x78, 0x08},
    {0x79, 0x02},
    {0x7A, 0x00},
    {0x7B, 0x1D},// TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
    {0x7C, 0xC0},
    {0x7D, 0x00},
    {0x7E, 0x00},
    {0xC9, 0x00},
    {0x85, 0x03},
    {0x86, 0x0B},
    {0xF4, 0x2A},   // Medium High Drive Strength
    {0x0E, 0x80},   // User Sub Map Enable
    {0x52, 0x46},
    {0x54, 0x00},
    {0x0E, 0x00},   // User Map Enable
    {0x68, 0x01},
};

REGPAIR SXGA60Hz[] = {
    {0x05, 0x02},   // Prim_Mode =010b for GR
    {0x06, 0x05},   // VID_STD=00005 for 1280x1024 _@60
    {0x37, 0x00},   //
    {0xC3, 0x46},   // ADC1 to Ain4, ADC0 to Ain6,
    {0xC4, 0xB5},   // ADC2 to Ain5 and enables manual override of mux
    {0x1D, 0x47},   // Enable 28MHz Crystal
    {0x3A, 0x21},   //
    {0x3B, 0x61},   //
    {0x3C, 0x5D},   // PLL_QPUMP to 101b
    {0x6A, 0x00},   // DLL Phase Adjust
    {0x52, 0x00},   // Colour Space Conversion from RGB->YCrCb
    {0x53, 0x00},   // CSC
    {0x54, 0x12},   // CSC
    {0x55, 0x90},   // CSC
    {0x56, 0x38},   // CSC
    {0x57, 0x69},   // CSC
    {0x58, 0x48},   // CSC
    {0x59, 0x08},   // CSC
    {0x5A, 0x00},   // CSC
    {0x5B, 0x75},   // CSC
    {0x5C, 0x21},   // CSC
    {0x5D, 0x00},   // CSC
    {0x5E, 0x1A},   // CSC
    {0x5F, 0xB8},   // CSC
    {0x60, 0x08},   // CSC
    {0x61, 0x00},   // CSC
    {0x62, 0x20},   // CSC
    {0x63, 0x03},   // CSC
    {0x64, 0xD7},   // CSC
    {0x65, 0x19},   // CSC
    {0x66, 0x48},   // CSC last
    {0x67, 0x05},   //
    {0x6B, 0x83},   // DE ON,16 bit output
    {0x73, 0x90},   // Set man_gain
    {0x74, 0x04},   //
    {0x75, 0x01},   //
    {0x76, 0x00},   //
    {0x77, 0x04},   //
    {0x78, 0x08},   //
    {0x79, 0x02},   //
    {0x7A, 0x00},   //
    {0x7B, 0x1D},   // TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
    {0x7C, 0xC0},   //
    {0x7D, 0x00},   //
    {0x7E, 0x00},   //
    {0xC9, 0x00},   //
    {0x85, 0x03},   //
    {0x86, 0x0B},   //
    {0x87, 0xE6},   // PLL_DIV
    {0x88, 0x98},   // PLL_DIV
    {0x8A, 0xF0},   //
    {0x8F, 0x01},   // FR_LL
    {0x90, 0xBF},   // FR_LL
    {0xB3, 0xFE},   //
    {0xBF, 0x06},   //
    {0xF4, 0x2A},   // Medium High Drive Strength
    {0x0E, 0x80},   // User Sub Map Enable
    {0x52, 0x46},   //
    {0x54, 0x00},   //
    {0x0E, 0x00},   // User Map Enable
    {0x68, 0x01},   //
};

bool
_ADV7181GetLock (
    void)
{
    bool result = false;
#if 1
    if (ADV7181_Initialed)
    {
        // (void)printf("reg0x10 = %x \n",DRV_ADV7181C_I2CRead(ADV7181_ADDR, 0x10));//status 1
        // (void)printf("reg0x12 = %x \n",DRV_ADV7181C_I2CRead(ADV7181_ADDR, 0x12));//status 2
        // (void)printf("reg0x13 = %x \n",DRV_ADV7181C_I2CRead(ADV7181_ADDR, 0x13));//status 3
        if (DRV_ADV7181C_I2CRead(ADV7181_ADDR, 0x12) == 0x80)
        {
            result = true;
        }
        else
        {
            result = false;
        }
    }
#else
    result = true;
#endif
    usleep(1000);
    return result;
}

void
_ADV7181SetSTD (
    ADV7181_INPUT_STANDARD sd)
{
    int i = 0;

    if (sd == XVGA_1024x768x60)
    {
        // Set width height
        ADV7181_InWidth     = 1024;
        ADV7181_InHeight    = 768;
        ADV7181_InFrameRate = 6000;

        for (i = 0; i < sizeof(XVGA60Hz) / sizeof(REGPAIR); i++)
        {
            DRV_ADV7181C_I2CWrite(ADV7181_ADDR, XVGA60Hz[i].addr, XVGA60Hz[i].value);
        }
        (void)printf("XVGA 1024x768@60\n");
    }
    else if (sd == SVGA_800x600x60)
    {
        // Set width height
        ADV7181_InWidth     = 800;
        ADV7181_InHeight    = 600;
        ADV7181_InFrameRate = 6000;

        for (i = 0; i < sizeof(SVGA60Hz) / sizeof(REGPAIR); i++)
        {
            DRV_ADV7181C_I2CWrite(ADV7181_ADDR, SVGA60Hz[i].addr, SVGA60Hz[i].value);
        }

        (void)printf("SVGA 800x600@60\n");
    }
    else if (sd == SVGA_800x600x56)
    {
        // Set width height
        ADV7181_InWidth     = 800;
        ADV7181_InHeight    = 600;
        ADV7181_InFrameRate = 5600;
        for (i = 0; i < sizeof(SVGA56Hz) / sizeof(REGPAIR); i++)
        {
            DRV_ADV7181C_I2CWrite(ADV7181_ADDR, SVGA56Hz[i].addr, SVGA56Hz[i].value);
        }

        (void)printf("SVGA 800x600@56\n");
    }
    else if (sd == VGA_640x480x60)
    {
        // Set width height
        ADV7181_InWidth     = 640;
        ADV7181_InHeight    = 480;
        ADV7181_InFrameRate = 6000;

        for (i = 0; i < sizeof(VGA60Hz) / sizeof(REGPAIR); i++)
        {
            DRV_ADV7181C_I2CWrite(ADV7181_ADDR, VGA60Hz[i].addr, VGA60Hz[i].value);
        }
        (void)printf("VGA 640x480@60\n");
    }
    else
    {
        // Set width height
        ADV7181_InWidth     = 1280;
        ADV7181_InHeight    = 1024;
        ADV7181_InFrameRate = 6000;

        for (i = 0; i < sizeof(SXGA60Hz) / sizeof(REGPAIR); i++)
        {
            DRV_ADV7181C_I2CWrite(ADV7181_ADDR, SXGA60Hz[i].addr, SXGA60Hz[i].value);
        }
        (void)printf("No supported timing\n");
    }

#if 0
    // DRV_ADV7181C_I2CWrite(ADV7181_ADDR, 0xBF, 0x06);//0x6 Force user define color
    // DRV_ADV7181C_I2CWrite(ADV7181_ADDR, 0xC0, 0x0);//G
    // DRV_ADV7181C_I2CWrite(ADV7181_ADDR, 0xC1, 0x0);//R
    // DRV_ADV7181C_I2CWrite(ADV7181_ADDR, 0xC2, 0x0);//B
#endif
}

// =============================================================================
//                Public Function Definition
// =============================================================================

void
ADV7181Initialize (
    uint16_t Mode)
{
#ifdef CFG_SENSOR_RESETPIN_ENABLE
    ithGpioClear(CFG_SN1_GPIO_RST);
    ithGpioSetOut(CFG_SN1_GPIO_RST);
    ithGpioSetMode(CFG_SN1_GPIO_RST, ITH_GPIO_MODE0);
    usleep(100 * 1000);
    ithGpioSet(CFG_SN1_GPIO_RST);
    usleep(100 * 1000);

    (void)printf("Reset adv7181c done\n");
#endif

    if (!ADV7181_Initialed)
    {
        DRV_ADV7181C_I2c_init();
        // _ADV7181SetSTD(VGA_640x480x60);
        // _ADV7181SetSTD(SVGA_800x600x60);
        _ADV7181SetSTD(XVGA_1024x768x60);
        // _ADV7181SetSTD(SXGA_1280x1024x60);
        ADV7181_Initialed = 1;
    }
    (void)printf("ADV7181c init done\n");
    usleep(1000 * 100);
}

void
ADV7181Terminate (
    void)
{
    ADV7181_Initialed = 0;
}

void
ADV7181OutputPinTriState (
    uint8_t flag)
{
}

uint8_t
ADV7181IsSignalStable (
    uint16_t Mode)
{
    if (_ADV7181GetLock())
    {
        return true;
    }

    return false;
}

uint16_t
ADV7181GetProperty (
    MODULE_GETPROPERTY property)
{
    switch (property)
    {
        case GetTopFieldPolarity:
            return 1;
            break;

        case GetHeight:
            return ADV7181_InHeight;
            break;

        case GetWidth:
            return ADV7181_InWidth;
            break;

        case Rate:
            return ADV7181_InFrameRate;
            break;

        case GetModuleIsInterlace:
            return 0;
            break;

        case GetIsAnalogDecoder:
            return 1;
            break;

        default:
            (void)printf("error property id =%d\n", property);
            return 0;
    }
}

uint8_t
ADV7181GetStatus (
    MODULE_GETSTATUS Status)
{
    switch (Status)
    {
        default:
            (void)printf("error status id =%d \n", Status);
            return 0;
    }
}

void
ADV7181SetProperty (
    MODULE_SETPROPERTY  Property,
    uint16_t            Value)
{
    switch (Property)
    {
        default:
            break;
    }
}

void
ADV7181PowerDown (
    uint8_t enable)
{
    if (enable)
    {
    }
    else
    {
    }
}

void
ADV7181SensorDriver_Destory (
    SensorDriver base)
{
    ADV7181SensorDriver self = (ADV7181SensorDriver)base;
    if (self)
    {
        free(self);
    }
}

static SensorDriverInterfaceStruct interface =
{
    ADV7181Initialize,
    ADV7181Terminate,
    ADV7181OutputPinTriState,
    ADV7181IsSignalStable,
    ADV7181GetProperty,
    ADV7181GetStatus,
    ADV7181SetProperty,
    ADV7181PowerDown,
    ADV7181SensorDriver_Destory
};

SensorDriver
ADV7181SensorDriver_Create ()
{
    ADV7181SensorDriver self = calloc(1, sizeof(ADV7181SensorDriverStruct));
    if (self != NULL)
    {
        self->base.vtable   = &interface;
        self->base.type     = "ADV7181";
    }
    return (SensorDriver)self;
}
