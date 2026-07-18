// =============================================================================
// =============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "iic/mmp_iic.h"
#include "DS90UB926Q.h"

// =============================================================================
//                Constant Definition
// =============================================================================
static uint8_t  IICADDR     = 0x58 >> 1;            /* please assign IIC ADDRESS */
#ifdef CFG_SENSOR_ENABLE
static uint8_t  IICPORT     = CFG_SENSOR_IIC_PORT;  /* please assign IIC PORT      */
#else
static uint8_t  IICPORT     = IIC_PORT_2;
#endif
static int      gMasterDev  = 0;
// =============================================================================
//                Macro Definition
// =============================================================================
#define GPIO_PASS_PIN       34
#define GPIO_LOCK_PIN       35
#define GPIO_OSSSEL_PIN     32
#define GPIO_OEN_PIN        36

#define PATTERN_GEN_TEST    0
// =============================================================================
//                Structure Definition
// =============================================================================
typedef struct _REGPAIR
{
    uint8_t     addr;
    uint16_t    value;
} REGPAIR;

typedef struct DS90UB926QSensorDriverStruct
{
    SensorDriverStruct base;
} DS90UB926QSensorDriverStruct;

// =============================================================================
//                IIC API FUNCTION START
// =============================================================================
void
DS90UB926Q_I2c_Init ()
{
    char portname[16] = {0};

    sprintf(portname, ":i2c%d", IICPORT);
    gMasterDev = open(portname, 0, 0);
}

uint8_t
DS90UB926Q_ReadI2c_Byte (
    uint8_t RegAddr)
{
    uint8_t     dbuf[2] = {0};
    int         result  = 0;
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt     = {0};

    dbuf[0]             = (uint8_t)(RegAddr);
    pdbuf++;

    evt.slaveAddress    = IICADDR;
    evt.cmdBuffer       = dbuf;
    evt.cmdBufferSize   = 1;
    evt.dataBuffer      = pdbuf;
    evt.dataBufferSize  = 1;

    result              = read(gMasterDev, (char *)&evt, 1);
    if (result != 0)
    {
        ithPrintf("ReadI2c_Byte read address 0x%02x error!\n", RegAddr);
    }
    // (void)printf("Reg = %x, value[0] = %x ,value[1] = %x\n",RegAddr, dbuf[0],dbuf[1]);
    return dbuf[1];
}

uint32_t
DS90UB926Q_WriteI2c_Byte (
    uint8_t RegAddr,
    uint8_t data)
{
    uint8_t     dbuf[2] = {0};
    int         result  = 0;
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt     = {0};

    *pdbuf++            = (uint8_t)(RegAddr & 0xff);
    *pdbuf              = (uint8_t)(data);

    evt.slaveAddress    = IICADDR;
    evt.cmdBuffer       = dbuf;
    evt.cmdBufferSize   = 2;

    if (0 != (result = write(gMasterDev, (char *)&evt, 1)))
    {
        (void)printf("WriteI2c_Byte Write Error, reg=%02x val=%02x\n", RegAddr, data);
    }
    return result;
}

uint32_t
DS90UB926Q_WriteI2c_ByteMask (
    uint8_t RegAddr,
    uint8_t data,
    uint8_t mask)
{
    uint8_t     value;
    uint32_t    flag;

    value   = DS90UB926Q_ReadI2c_Byte(RegAddr);
    value   = ((value & ~mask) | (data & mask));
    flag    = DS90UB926Q_WriteI2c_Byte(RegAddr, value);

    return flag;
}

// =============================================================================
//                IIC API FUNCTION END
// =============================================================================

// =============================================================================
//                Global Data Definition
// =============================================================================

// =============================================================================
//                Private Function Definition
// =============================================================================
static int
DS90UB926Q_ResetHW ()
{
    uint8_t reset_reg   = 0;
    uint8_t timeout     = 5;

    DS90UB926Q_WriteI2c_ByteMask(DS90UB926Q_RESET, (0x1 << DS90UB926Q_RESET_RESET1_SHIFT), DS90UB926Q_RESET_RESET1_MASK);
    while (timeout > 0)
    {
        reset_reg = DS90UB926Q_ReadI2c_Byte(DS90UB926Q_RESET);
        if ((reset_reg & DS90UB926Q_RESET_RESET1_MASK) != DS90UB926Q_RESET_RESET1_MASK)
        {
            // (void)printf("[926Q] Reset Done \n");
            return 0;
        }

        timeout--;
        usleep(1000);
    }

    return 1;
}

static void
DS90UB926Q_800_480_Pattern ()
{
    // Active horizontal 800, Active vertical 480, 60.3 HZ, internal clk
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_CFG, 0x05);
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN, 0x11);
}

static void
DS90UB926Q_1280_720_Pattern ()
{
    // Active horizontal 1280, Active vertical 720, 32Hz , internal clk
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0x5); // CLK Div
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x3);
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0x4); // total hor LSB 8
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x72);
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0x5); // total ver LSB 4 + hor MSB 4
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0xE6);
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0x6); // total ver MSB 8
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x2E);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0x7); // active hor LSB 8
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x00);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0x8); // active ver LSB 4 + hor MSB 4
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x05);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0x9); // active ver MSB 8
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x2D);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0xA); // hor sync width
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x28);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0xB); // ver sync width
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x5);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0xC); // hor back proch
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0xDC);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_ADDR, 0xD); // ver back proch
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_DATA, 0x14);

    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN_CFG, 0x05);
    DS90UB926Q_WriteI2c_Byte(DS90UB926Q_PATTERN_GEN, 0x11);
}

// =============================================================================
//                USER IMPLEMENT FUNCTION START
// =============================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////

// X10LightDriver_t1.c
void
DS90UB926QInitialize (
    uint16_t Mode)
{
    uint8_t i2c_deviceid = 0;

    DS90UB926Q_I2c_Init();

    /* read i2c device id */
    i2c_deviceid = DS90UB926Q_ReadI2c_Byte(DS90UB926Q_I2CID);
    (void)printf("[926Q] I2C id = 0x%x \n", i2c_deviceid);

    /* output gpio set */
    ithGpioSetMode(GPIO_OSSSEL_PIN, ITH_GPIO_MODE0);
    ithGpioSetOut(GPIO_OSSSEL_PIN);
    ithGpioSet(GPIO_OSSSEL_PIN);

    ithGpioSetMode(GPIO_OEN_PIN, ITH_GPIO_MODE0);
    ithGpioSetOut(GPIO_OEN_PIN);
    ithGpioSet(GPIO_OEN_PIN);

    /* input gpio set */
    ithGpioSetMode(GPIO_LOCK_PIN, ITH_GPIO_MODE0);
    ithGpioSetIn(GPIO_LOCK_PIN);

    ithGpioSetMode(GPIO_PASS_PIN, ITH_GPIO_MODE0);
    ithGpioSetIn(GPIO_PASS_PIN);

    /* digital reset */
    if (DS90UB926Q_ResetHW())
    {
        (void)printf("[926Q] Reset Timeout \n");
    }

#if PATTERN_GEN_TEST
    DS90UB926Q_1280_720_Pattern();
    // DS90UB926Q_800_480_Pattern();
#endif
}

void
DS90UB926QTerminate (
    void)
{
    ithGpioSetMode(GPIO_OSSSEL_PIN, ITH_GPIO_MODE0);
    ithGpioSetOut(GPIO_OSSSEL_PIN);
    ithGpioClear(GPIO_OSSSEL_PIN);

    ithGpioSetMode(GPIO_OEN_PIN, ITH_GPIO_MODE0);
    ithGpioSetOut(GPIO_OEN_PIN);
    ithGpioClear(GPIO_OEN_PIN);
}

void
DS90UB926QOutputPinTriState (
    uint8_t flag)
{
    if (flag == true)
    {
        /* Please implement outputpintristate code here */
    }
    else
    {
        /* Please implement disable outputpintristate code here */
    }
}

uint8_t
DS90UB926QIsSignalStable (
    uint16_t Mode)
{
    bool    isStable    = false;
    uint8_t lock_status = 0;
    /* stable return true else return false              */
    (void)printf("[926Q] Lock pin = %d \n", ithGpioGet(GPIO_LOCK_PIN));
    (void)printf("[926Q] Pass pin = %d \n", ithGpioGet(GPIO_PASS_PIN));

    lock_status = DS90UB926Q_ReadI2c_Byte(DS90UB926Q_GENSTATUS);
    (void)printf("[926Q] Lockreg = 0x%x \n", lock_status);
    if ((lock_status & DS90UB926Q_GENSTATUS_LOCK_MASK) == 0x1)
    {
        isStable = true;
    }

    return isStable;
}

uint16_t
DS90UB926QGetProperty (
    MODULE_GETPROPERTY property)
{
    /* Please implement get information from device code here */
    switch (property)
    {
    // case GetTopFieldPolarity:
        case GetHeight:
            return 720;

        case GetWidth:
            return 1280;

            // frame rate
        case Rate:
            return 3000;

        case GetModuleIsInterlace:
            return 0;

        case matchResolution:
            return 1;

        case GetIsAnalogDecoder:
            return 1;

        default:
            return 0;
            break;
    }
}

uint8_t
DS90UB926QGetStatus (
    MODULE_GETSTATUS Status)
{
    /* Please implement get status from device code here */
    switch (Status)
    {
        case IsPowerDown:
        case IsSVideoInput:
        default:
            return 0;
            break;
    }
}

void
DS90UB926QSetProperty (
    MODULE_SETPROPERTY  Property,
    uint16_t            Value)
{
    /* Please implement set property to device code here */
}

void
DS90UB926QPowerDown (
    uint8_t enable)
{
    /* Please implement power down code here */
}

// =============================================================================
//                DS90UB926Q IMPLEMENT FUNCTION END
// =============================================================================
void
DS90UB926QSensorDriver_Destory (
    SensorDriver base)
{
    SensorDriver self = (SensorDriver)base;
    if (self)
    {
        free(self);
    }
}

/* assign callback funciton */
static SensorDriverInterfaceStruct interface =
{
    DS90UB926QInitialize,
    DS90UB926QTerminate,
    DS90UB926QOutputPinTriState,
    DS90UB926QIsSignalStable,
    DS90UB926QGetProperty,
    DS90UB926QGetStatus,
    DS90UB926QSetProperty,
    DS90UB926QPowerDown,
    DS90UB926QSensorDriver_Destory
};

SensorDriver
DS90UB926QSersorDriver_Create ()
{
    DS90UB926QSensorDriver self = calloc(1, sizeof(DS90UB926QSensorDriverStruct));
    if (self != NULL)
    {
        self->base.vtable   = &interface;
        self->base.type     = "DS90UB926Q";
    }
    return (SensorDriver)self;
}

// end of X10LightDriver_t1.c
