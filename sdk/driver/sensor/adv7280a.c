#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "iic/mmp_iic.h"
#include "adv7280a.h"

// =============================================================================
//                Constant Definition
// =============================================================================
static uint8_t ADV7280_ADDR = 0x40;
#ifdef CFG_SENSOR_ENABLE
    #define ADV7280_IIC_PORT    CFG_SENSOR_IIC_PORT
#else
    #define ADV7280_IIC_PORT    IIC_PORT_2
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

typedef struct ADV7280SensorDriverStruct
{
    SensorDriverStruct base;
} ADV7280SensorDriverStruct;

// =============================================================================
//                Global Data Definition
// =============================================================================
static int      gMasterDev          = 0;
static uint16_t gADV7280CurMode     = 0xFF;
static uint16_t gADV7280CurSTD      = 0;
static uint16_t ADV7280_InWidth     = 0;
static uint16_t ADV7280_InHeight    = 0;
static uint16_t ADV7280_InFrameRate = 0;
static uint16_t ADV7280_Interlaced  = 1;
static uint16_t ADV7280_MatchRes    = 0;
static uint16_t ADV7280_Initialed   = 0;

// =============================================================================
//                Private Function Definition
// =============================================================================
// =============================================================================
//                IIC API FUNCTION START
// =============================================================================
void
DRV_ADV7280A_I2c_init ()
{
    char portname[16] = {0};

    sprintf(portname, ":i2c%d", ADV7280_IIC_PORT);
    gMasterDev = open(portname, 0, 0);
}

uint8_t
DRV_ADV7280A_I2CRead (
    uint8_t Addr,
    uint8_t RegAddr)
{
    uint8_t     dbuf[2] = {0};
    int         result  = 0;
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt     = {0};

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
        (void)printf("_ADV7280_ReadI2c_Byte read address 0x%02x error!\n", RegAddr);
    }
    // (void)printf("Reg = %x, value[0] = %x ,value[1] = %x\n",RegAddr, dbuf[0],dbuf[1]);
    return dbuf[1];
}

uint32_t
DRV_ADV7280A_I2CWrite (
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

    if (0 != (result = write(gMasterDev, (char *)&evt, 1)))
    {
        (void)printf("_ADV7280_WriteI2c_Byte Write Error, reg=%02x val=%02x\n", RegAddr, data);
    }
    return result;
}

// =============================================================================
//                IIC API FUNCTION END
// =============================================================================
void
_ADV7280Power_Up_Sequence ()
{
    /* ADV7280A Power-Up Sequence */
    (void)printf("power up Sequence \n");
    /* 1. Set nPWRDWN to H level */

#ifdef CFG_SENSOR_POWERDOWNPIN_ENABLE
    ithGpioSet(CFG_SN1_GPIO_PWN);
    ithGpioSetOut(CFG_SN1_GPIO_PWN);
    ithGpioSetMode(CFG_SN1_GPIO_PWN, ITH_GPIO_MODE0);
#endif
    /* 2. wait 5 ms for RESET operation */
    usleep(10 * 1000);

    /* 3. Set nRESET to H level */
#ifdef CFG_SENSOR_RESETPIN_ENABLE
    ithGpioSet(CFG_SN1_GPIO_RST);
    ithGpioSetOut(CFG_SN1_GPIO_RST);
    ithGpioSetMode(CFG_SN1_GPIO_RST, ITH_GPIO_MODE0);
#endif

    /* 4. Wait 5 ms then start normal operation */
    usleep(10 * 1000);

    (void)printf("power up Sequence done\n");
}

uint8_t
_ADV7280GetInitStatus (
    void)
{
    return ADV7280_Initialed;
}

void
_ADV7280SetChannel (
    ADV7280_SEL_CHANNEL ch)
{
    if (ADV7280_Initialed)
    {
        DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x00, ch);
    }
}

ADV7280_INPUT_STANDARD
_ADV7280GetSTD (
    void)
{
    uint8_t tmpdata = 0;
    uint8_t result  = 0;
    if (ADV7280_Initialed)
    {
        tmpdata = DRV_ADV7280A_I2CRead(ADV7280_ADDR, 0x10);
        result  = tmpdata >> 4;
    }

    switch (result)
    {
        case ADV7280_NTSC_M_J:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 487;
            ADV7280_InFrameRate = 5994;
            gADV7280CurSTD      = ADV7280_NTSC_M_J;
            ADV7280_MatchRes    = 1;
            (void)printf("NTSC M/J\n");
            break;

        case ADV7280_NTSC_4_43:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 487;
            ADV7280_InFrameRate = 5994;
            gADV7280CurSTD      = ADV7280_NTSC_4_43;
            ADV7280_MatchRes    = 1;
            (void)printf("NTSC 4.43\n");
            break;

#if 0
        case ADV7280_PAL_M:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 480;
            ADV7280_InFrameRate = 5994;
            gADV7280CurSTD      = ADV7280_PAL_M;
            ADV7280_MatchRes    = 1;
            (void)printf("PAL_M\n");
            break;

        case ADV7280_PAL_60:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 480;
            ADV7280_InFrameRate = 6000;
            gADV7280CurSTD      = ADV7280_PAL_60;
            ADV7280_MatchRes    = 1;
            (void)printf("PAL_60\n");
            break;

#endif
        case ADV7280_PAL_B_G_H_I_D:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 576;
            ADV7280_InFrameRate = 5000;
            gADV7280CurSTD      = ADV7280_PAL_B_G_H_I_D;
            ADV7280_MatchRes    = 1;
            (void)printf("PAL B/G/H/I/D\n");
            break;

#if 0
        case ADV7280_SECAM:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 576;
            ADV7280_InFrameRate = 5000;
            gADV7280CurSTD      = ADV7280_SECAM;
            ADV7280_MatchRes    = 1;
            (void)printf("SECAM\n");
            break;

        case ADV7280_PAL_COMBINATION_N:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 576;
            ADV7280_InFrameRate = 5000;
            gADV7280CurSTD      = ADV7280_PAL_COMBINATION_N;
            ADV7280_MatchRes    = 1;
            (void)printf("PAL Combination N\n");
            break;

        case ADV7280_SECAM_525:
            ADV7280_InWidth     = 720;
            ADV7280_InHeight    = 480;
            ADV7280_InFrameRate = 5994;
            gADV7280CurSTD      = ADV7280_SECAM_525;
            ADV7280_MatchRes    = 1;
            (void)printf("SECAM 525\n");
            break;

#endif
        default:
            ADV7280_MatchRes = 0;
            (void)printf("[ADV7280]can not recognize\n");
            break;
    }
    return result;
}

bool
_ADV7280GetLock (
    void)
{
    bool    result  = false;
    uint8_t TmpData = 0;
    if (ADV7280_Initialed)
    {
        TmpData = DRV_ADV7280A_I2CRead(ADV7280_ADDR, 0x10);
        // (void)printf("[status 1]reg 0x10 = 0x%x\n", TmpData);
        if (TmpData & 0x1)      // IN_LOCK
        {
            if (TmpData & 0x4)  // FSC LOCK
            {
                (void)printf("FSC LOCK\n");
                result = true;
            }
        }
    }

    return result;
}

void
_ADV7280ColorBar (
    void)
{
    /*
    :Free-run, Color Bars 480p YPbPr Out:
    delay 10 ; Wait 10ms After Hardware Reset To Start I2C
    42 0F 80 ; Reset ADV7280A
    56 17 02 ; Reset Encoder
    delay 10 ; Wait 10ms
    42 0F 00 ; Exit Power Down Mode [ADV7280A writes begin]
    42 52 CD ; AFE IBIAS
    42 00 07 ; ADI Required Write [INSEL set to unconnected input]
    42 0C 37 ; Force Free run mode
    42 02 54 ; Force standard to NTSC-M
    42 14 11 ; Set Free-run pattern to 100% color bars
    42 80 51 ; ADI Required Write
    42 81 51 ; ADI Required Write
    42 82 68 ; ADI Required Write
    42 17 41 ; Enable SH1
    42 03 0C ; Enable Pixel & Sync output drivers
    42 04 07 ; Power-up INTRQ, HS & VS pads
    42 13 00 ; Enable ADV7280A for 28_63636MHz crystal
    42 1D 40 ; Enable LLC output driver
    42 FD 84 ; Set VPP Map
    84 A3 00 ; ADI Required Write [ADV7280A VPP writes begin]
    84 5B 00 ; Enable Advanced Timing Mode
    84 55 80 ; Enable the Deinterlacer for I2P [All ADV7280A writes finished]
    56 17 02 ; Software Reset [Encoder writes begin]
    56 00 9C ; Power up DAC's and PLL
    56 01 70 ; ED at 54MHz input
    56 30 04 ; 525p at 59.94 Hz with Embedded Timing
    56 31 01 ; Pixel Data Valid [Encoder Writes finished]
    End
    */
    // usleep(20*1000);
    // DRV_ADV7280A_I2CWrite(0x42, 0x0F, 0x80); /* remove RESET command due to cause no ACK */
    // DRV_ADV7280A_I2CWrite(0x56, 0x17, 0x02);
    usleep(10 * 1000);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x0F, 0x00);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x52, 0xCD);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x00, 0x07);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x0C, 0x37);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x02, 0x54);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x14, 0x11);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x80, 0x51);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x81, 0x51);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x82, 0x68);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x17, 0x41);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x03, 0x0C);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x04, 0x87);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x13, 0x00);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x1D, 0x40);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0xFD, 0x84);

    // Set I2P
    DRV_ADV7280A_I2CWrite(0x84, 0xA3, 0x00);        /* **ADI Required Write** */
    DRV_ADV7280A_I2CWrite(0x84, 0x5B, 0x00);        /* Enable Advanced Timing Mode */
    DRV_ADV7280A_I2CWrite(0x84, 0x55, 0x80);        /* Enable the Deinterlacer for 12P */
}

// CVBS

void
_ADV7280SetCVBS_AUTO (
    void)
{
    /* Set CVBS input on AIN1 (ADV7280A) */
    /*
    :Autodetect CVBS Single Ended In Ain 1, YPbPr Out:
    delay 10 ; Wait 10ms After Hardware Reset To Start I2C
    42 0F 80 ; Reset ADV7280A
    56 17 02 ; Reset Encoder
    delay 10 ; Wait 10ms
    42 0F 00 ; Exit Power Down Mode [ADV7280A writes begin]
    42 52 CD ; AFE IBIAS
    42 00 00 ; CVBS in on AIN1
    42 0E 80 ; ADI Required Write
    42 9C 00 ; Reset Current Clamp Circuitry [step1]
    42 9C FF ; Reset Current Clamp Circuitry [step2]
    42 0E 00 ; Enter User Sub Map
    42 80 51 ; ADI Required Write
    42 81 51 ; ADI Required Write
    42 82 68 ; ADI Required Write
    42 17 41 ; Enable SH1
    42 03 0C ; Enable Pixel & Sync output drivers
    42 04 07 ; Power-up INTRQ, HS & VS pads
    42 13 00 ; Enable ADV7280A for 28_63636MHz crystal
    42 1D 40 ; Enable LLC output driver [ADV7280A writes finished]
    End
    */
    // usleep(10*1000);
    // DRV_ADV7280A_I2CWrite(0x42, 0x0F, 0x80);  /* remove RESET command due to cause no ACK */
    // DRV_ADV7280A_I2CWrite(0x56, 0x17, 0x02);
    usleep(10 * 1000);
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x0F, 0x00);    /* Exit Power Down Mode */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x52, 0xCD);    /* AFE IBIAS */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x00, 0x00);    /* CVBS in on AIN1 */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x02, 0x04);    /* Autodetects  */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x0E, 0x80);    /* **ADI Required Write** */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x9C, 0x00);    /* Reset Current Clamp Circuitry [step1] */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x9C, 0xFF);    /* Reset Current Clamp Circuitry [step2] */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x0E, 0x00);    /* Enter User Sub Map */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x80, 0x51);    /* **ADI Required Write** */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x81, 0x51);    /* **ADI Required Write** */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x82, 0x68);    /* **ADI Required Write** */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x17, 0x41);    /* Enable SH1 */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x03, 0x0C);    /* Enable Pixel & Sync output drivers */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x04, 0x87);    /* Power-up INTRQ, HS & VS pads */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x13, 0x00);    /* Enable ADV7280A for 28.63636MHz crystal */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x1D, 0x40);    /* Enable LLC output driver */
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0xFD, 0x84);    /* Set VPP Map */
#ifdef ENABLE_I2P_FUNCTION
    // Set I2P
    DRV_ADV7280A_I2CWrite(0x84, 0xA3, 0x00);            /* **ADI Required Write** */
    DRV_ADV7280A_I2CWrite(0x84, 0x5B, 0x00);            /* Enable Advanced Timing Mode */
    DRV_ADV7280A_I2CWrite(0x84, 0x55, 0x80);            /* Enable the Deinterlacer for I2P */
#endif
}

// =============================================================================
//                Public Function Definition
// =============================================================================

void
ADV7280Initialize (
    uint16_t Mode)
{
    uint8_t TmpData = 0;
    (void)printf("adv7280 init\n");

    DRV_ADV7280A_I2c_init();

    gADV7280CurMode = Mode;

#ifdef ENABLE_DEBUG_VIDEO_COLORBAR
    _ADV7280ColorBar();
#else
    switch (gADV7280CurMode)
    {
        case ADV7280_INPUT_CVBS:
            _ADV7280SetCVBS_AUTO();
            break;

        default:
            break;
    }
#endif

    // For Reduce noise
    // Step1 : 0x1D[7]  set to ��0��, b'0xxxx xxxx'
    TmpData = DRV_ADV7280A_I2CRead(ADV7280_ADDR, 0x1D);
    TmpData &= 0x7F;
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x1D, TmpData);

#if 0
    TmpData             = 0;
    // Step2 : 0xF4[3:2] set to ��0000�� , decrease clock output driving strength.  b'xxxx 00xx'
    TmpData             = DRV_ADV7280A_I2CRead(ADV7280_ADDR, 0xF4);
    TmpData             &= 0xC3;
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0xF4, TmpData);

    TmpData             = 0;
    // Step2 : 0x37[0] set to ��0�� , decrease clock output driving strength.  b'xxxx xxx0'
    TmpData             = DRV_ADV7280A_I2CRead(ADV7280_ADDR, 0x37);
    TmpData             &= 0xFE;
    DRV_ADV7280A_I2CWrite(ADV7280_ADDR, 0x37, TmpData);
#endif
    ADV7280_Initialed   = 1;
    (void)printf("adv7280 init end\n");
}

void
ADV7280Terminate (
    void)
{
    ADV7280_Initialed = 0;
}

void
ADV7280OutputPinTriState (
    uint8_t flag)
{
}

uint8_t
ADV7280IsSignalStable (
    uint16_t Mode)
{
#ifdef ENABLE_DEBUG_VIDEO_COLORBAR
    ADV7280_InWidth     = 720;
    ADV7280_InHeight    = 487;
    ADV7280_InFrameRate = 5994;
    return true;
#else

    if (_ADV7280GetLock())
    {
        if (gADV7280CurMode == ADV7280_INPUT_CVBS)
        {
            _ADV7280GetSTD();
        }
        return true;
    }
    return false;
#endif
}

uint16_t
ADV7280GetProperty (
    MODULE_GETPROPERTY property)
{
    switch (property)
    {
        case GetTopFieldPolarity:
            return 1;
            break;

        case GetHeight:
            return ADV7280_InHeight;
            break;

        case GetWidth:
            return ADV7280_InWidth;
            break;

        case Rate:
            return ADV7280_InFrameRate;
            break;

        case GetModuleIsInterlace:
#ifdef ENABLE_I2P_FUNCTION
            ADV7280_Interlaced  = 0;
#else
            ADV7280_Interlaced  = 1;
#endif

            return ADV7280_Interlaced;
            break;

        case GetIsAnalogDecoder:
            return 1;
            break;

        case matchResolution:
            return ADV7280_MatchRes;
            break;

        default:
            (void)printf("error property id =%d\n", property);
            return 0;
            break;
    }
}

uint8_t
ADV7280GetStatus (
    MODULE_GETSTATUS Status)
{
    switch (Status)
    {
        case IsPowerDown:
            return _ADV7280GetInitStatus();
            break;

        default:
            (void)printf("error status id =%d \n", Status);
            return 0;
            break;
    }
}

void
ADV7280SetProperty (
    MODULE_SETPROPERTY  Property,
    uint16_t            Value)
{
    switch (Property)
    {
        case VideoInCH:
            _ADV7280SetChannel(Value);
            break;

        default:
            break;
    }
}

void
ADV7280PowerDown (
    uint8_t enable)
{
    if (enable)
    {
#ifdef CFG_SENSOR_POWERDOWNPIN_ENABLE
        ithGpioClear(CFG_SN1_GPIO_PWN);
        ithGpioSetOut(CFG_SN1_GPIO_PWN);
        ithGpioSetMode(CFG_SN1_GPIO_PWN, ITH_GPIO_MODE0);
#endif

#ifdef CFG_SENSOR_RESETPIN_ENABLE
        ithGpioClear(CFG_SN1_GPIO_RST);
        ithGpioSetOut(CFG_SN1_GPIO_RST);
        ithGpioSetMode(CFG_SN1_GPIO_RST, ITH_GPIO_MODE0);
#endif
    }
    else
    {
        _ADV7280Power_Up_Sequence();
    }
}

void
ADV7280SensorDriver_Destory (
    SensorDriver base)
{
    ADV7280SensorDriver self = (ADV7280SensorDriver)base;
    if (self)
    {
        free(self);
    }
}

static SensorDriverInterfaceStruct interface =
{
    ADV7280Initialize,
    ADV7280Terminate,
    ADV7280OutputPinTriState,
    ADV7280IsSignalStable,
    ADV7280GetProperty,
    ADV7280GetStatus,
    ADV7280SetProperty,
    ADV7280PowerDown,
    ADV7280SensorDriver_Destory
};

SensorDriver
ADV7280SensorDriver_Create ()
{
    ADV7280SensorDriver self = calloc(1, sizeof(ADV7280SensorDriverStruct));
    if (self != NULL)
    {
        self->base.vtable   = &interface;
        self->base.type     = "ADV7280";
    }
    return (SensorDriver)self;
}
