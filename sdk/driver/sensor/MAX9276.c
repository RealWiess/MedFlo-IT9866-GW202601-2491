// =============================================================================
// =============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "iic/mmp_iic.h"
#include "MAX9276.h"

// =============================================================================
//                Constant Definition
// =============================================================================
static uint8_t              IICADDR     = 0x90 >> 1;            /* please assign IIC ADDRESS */
#ifdef CFG_SENSOR_ENABLE
static uint8_t              IICPORT     = CFG_SENSOR_IIC_PORT;  /* please assign IIC PORT      */
#else
static uint8_t              IICPORT     = IIC_PORT_2;
#endif
static int                  gMasterDev  = 0;

static MAX9276_TIMING_ID_ID gResoultion = MAX9276_HD720P_30FPS;
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

typedef struct Max9276SensorDriverStruct
{
    SensorDriverStruct base;
} Max9276SensorDriverStruct;

// =============================================================================
//                IIC API FUNCTION START
// =============================================================================
void
Max9276_I2c_Init ()
{
    char portname[16] = {0};

    sprintf(portname, ":i2c%d", IICPORT);
    gMasterDev = open(portname, 0, 0);
}

uint8_t
Max9276_ReadI2c_Byte (
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
Max9276_WriteI2c_Byte (
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
Max9276_WriteI2c_ByteMask (
    uint8_t RegAddr,
    uint8_t data,
    uint8_t mask)
{
    uint8_t     value;
    uint32_t    flag;

    value   = Max9276_ReadI2c_Byte(RegAddr);
    value   = ((value & ~mask) | (data & mask));
    flag    = Max9276_WriteI2c_Byte(RegAddr, value);

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
static void
DumpAllReg ()
{
    int i = 0;
    for (i = 0; i <= 31; i++)
    {
        (void)printf(" reg(0x%02x)= 0x%02x\n", i, Max9276_ReadI2c_Byte(i));
    }
}
// =============================================================================
//                USER IMPLEMENT FUNCTION START
// =============================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////

// X10LightDriver_t1.c
void
Max9276Initialize (
    uint16_t Mode)
{
    uint8_t REG_ID = 0;

    Max9276_I2c_Init();
    /* Please implement initial code here */

    REG_ID = Max9276_ReadI2c_Byte(MAX9276_REG1E);
    if (REG_ID == 0x22)
    {
        (void)printf("Device ID is MAX9276 \n");
    }
    else
    {
        (void)printf("Device ID error \n");
    }
}

void
Max9276Terminate (
    void)
{
    /* Please implement terminate code here */
}

void
Max9276OutputPinTriState (
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
Max9276IsSignalStable (
    uint16_t Mode)
{
    bool isStable = false;
    /* Please implement checking signal stable code here */
    /* stable return true else return false              */

    uint8_t REG04   = Max9276_ReadI2c_Byte(MAX9276_REG4);
    uint8_t REG0D   = Max9276_ReadI2c_Byte(MAX9276_REGD);

    (void)printf("REG(0x4) = %x\n", REG04);

    if ((REG04 & MAX9276_REG4_SLEEP_MASK) == 0x0)
    {
        if ((REG04 & MAX9276_REG4_LOCKED_MASK) == MAX9276_REG4_LOCKED_MASK)
        {
            isStable = true;
        }
    }

    if (REG0D != 0x00)
    {
        (void)printf("Error counter = %x\n", REG0D);
    }
    // DumpAllReg();

    return isStable;
}

uint16_t
Max9276GetProperty (
    MODULE_GETPROPERTY property)
{
    /* Please implement get information from device code here */
    switch (property)
    {
        case GetHeight:
            if ((gResoultion == MAX9276_HD720P_25FPS) || (gResoultion == MAX9276_HD720P_30FPS))
            {
                return 720;
            }
            else
            {
                return 0;
            }

        case GetWidth:
            if ((gResoultion == MAX9276_HD720P_25FPS) || (gResoultion == MAX9276_HD720P_30FPS))
            {
                return 1280;
            }
            else
            {
                return 0;
            }

            // frame rate
        case Rate:
            if (gResoultion == MAX9276_HD720P_25FPS)
            {
                return 2500;
            }
            else if (gResoultion == MAX9276_HD720P_30FPS)
            {
                return 3000;
            }
            else
            {
                return 0;
            }

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
Max9276GetStatus (
    MODULE_GETSTATUS Status)
{
    /* Please implement get status from device code here */
    switch (Status)
    {
        case IsPowerDown:
        default:
            return 0;
            break;
    }
}

void
Max9276SetProperty (
    MODULE_SETPROPERTY  Property,
    uint16_t            Value)
{
    /* Please implement set property to device code here */
}

void
Max9276PowerDown (
    uint8_t enable)
{
    /* Please implement power down code here */
}

// =============================================================================
//                Max9276 IMPLEMENT FUNCTION END
// =============================================================================
void
Max9276SensorDriver_Destory (
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
    Max9276Initialize,
    Max9276Terminate,
    Max9276OutputPinTriState,
    Max9276IsSignalStable,
    Max9276GetProperty,
    Max9276GetStatus,
    Max9276SetProperty,
    Max9276PowerDown,
    Max9276SensorDriver_Destory
};

SensorDriver
Max9276SensorDriver_Create ()
{
    Max9276SensorDriver self = calloc(1, sizeof(Max9276SensorDriverStruct));
    if (self != NULL)
    {
        self->base.vtable   = &interface;
        self->base.type     = "Max9276";
    }
    return (SensorDriver)self;
}

// end of X10LightDriver_t1.c
