// =============================================================================
// =============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "iic/mmp_iic.h"
#include "TP2825B.h"

// =============================================================================
//                Constant Definition
// =============================================================================
/* Note:
TP2825B i2c Thd minimum is 250 ns.
I2c driver need set SDA delay time.
file path: itp_i2c.c
function:mmpIicInitialize(.....,uint32_t delay)
*/
static uint8_t                  IICADDR     = (0x88 >> 1);          /* please assign IIC ADDRESS */
#ifdef CFG_SENSOR_ENABLE
static uint8_t                  IICPORT     = CFG_SENSOR_IIC_PORT;  /* please assign IIC PORT      */
#else
static uint8_t                  IICPORT     = IIC_PORT_2;
#endif
static int                      gMasterDev  = 0;

uint8_t                         state[1];
uint8_t                         count[1];
uint8_t                         mode[1];
uint8_t                         std[1];
uint8_t                         egain[1][4];

const TP2825B_RESOLUTIONINFO    cap_table[TP2825B_RES_TABLE_INDEX] =
{
    {TP2802_NTSC,     720,  480,  5994, 1},
    {TP2802_PAL,      720,  576,  5000, 1},
    {TP2802_720P25,   1280, 720,  2500, 0},
    {TP2802_720P30,   1280, 720,  3000, 0},
    {TP2802_720P25V2, 1280, 720,  2500, 0},
    {TP2802_720P30V2, 1280, 720,  3000, 0},
    {TP2802_1080P25,  1920, 1080, 2500, 0},
    {TP2802_1080P30,  1920, 1080, 3000, 0},
    {0xFF,            720,  480,  6000, 1},
};
// =============================================================================
//                Structure Definition
// =============================================================================
typedef struct _REGPAIR
{
    uint8_t addr;
    uint8_t value;
} REGPAIR;

typedef struct TP2825BSensorDriverStruct
{
    SensorDriverStruct base;
} TP2825BSensorDriverStruct;

// =============================================================================
//                IIC API FUNCTION START
// =============================================================================
void
tp28xx_I2c_Init ()
{
    char portname[16] = {0};

    sprintf(portname, ":i2c%d", IICPORT);
    gMasterDev = open(portname, 0, 0);
}

uint8_t
tp28xx_byte_read (
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
        (void)printf("ReadI2c_uint8_t read address 0x%02x error!\n", RegAddr);
    }
    // (void)printf("Reg = %x, value[0] = %x ,value[1] = %x\n",RegAddr, dbuf[0],dbuf[1]);
    return dbuf[1];
}

uint32_t
tp28xx_byte_write (
    uint8_t RegAddr,
    uint8_t data)
{
    // rintf("w reg (0x%02x, 0x%02x) \n ",RegAddr, data);
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
        (void)printf("WriteI2c_uint8_t Write Error, reg=%02x val=%02x\n", RegAddr, data);
    }
    return result;
}

// =============================================================================
//                IIC API FUNCTION END
// =============================================================================

// =============================================================================
//                Global Data Definition
// =============================================================================
REGPAIR TP2825B_DataSet[] =
{
    {0x40, 0x00},
    {0x35, 0x25},
    {0x4c, 0x43},   //
    {0x4e, 0x05},   // 0x1d
    {0xf5, 0x10},
    {0xfd, 0x80},
    {0xf6, 0x00},   //
    {0xf7, 0x44},
    {0xfa, 0x03},
    {0x1b, 0x01},   //
};
uint8_t TP2825B_FORMAT_DataSet[][32] =
{
// addr
    {0x02, 0x0c, 0x0d, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x20, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27, 0x28, 0x2b, 0x2c, 0x2d, 0x2e, 0x30, 0x31, 0x32, 0x33, 0x35, 0x36, 0x00, 0x39, 0x3a},
// 1NTSC 960H
    {0xcf, 0x13, 0x50, 0x13, 0x60, 0xbc, 0x12, 0xf0, 0x07, 0x09, 0x38, 0x40, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x70, 0x2a, 0x68, 0x57, 0x62, 0xbb, 0x96, 0xc0, 0x65, 0xdc, 0x00, 0x04, 0x32},
    // {0xcf,0x13,0x50, 0x03,0xd6,0xa0,0x12,0xf0,0x05,0x06,0xb4, 0x40,0x84,0x36,0x3c, 0xff,0x05,0x2d,0x00, 0x70,0x2a,0x4b,0x57, 0x62,0xbb,0x96,0xcb, 0x65,0xdc,0x00,0x04,0x32},
// 2PAL 960H
    {0xcf, 0x13, 0x51, 0x13, 0x76, 0xbc, 0x17, 0x20, 0x17, 0x09, 0x48, 0x48, 0x84, 0x37, 0x3f, 0xff, 0x05, 0x2d, 0x00, 0x70, 0x2a, 0x64, 0x56, 0x7a, 0x4a, 0x4d, 0xf0, 0x65, 0xdc, 0x00, 0x04, 0x32},
    // {0xcf,0x13,0x51, 0x03,0xf0,0xa0,0x17,0x20,0x15,0x06,0xc0, 0x48,0x84,0x37,0x3f, 0xff,0x05,0x2d,0x00, 0x70,0x2a,0x4b,0x56, 0x7a,0x4a,0x4d,0xfb, 0x65,0xdc,0x00,0x04,0x32},
// 3TVI 720P30V2
    {0xca, 0x13, 0x50, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x06, 0x72, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x25, 0xdc, 0x00, 0x18, 0x32},
// 4TVI 720P25V2
    {0xca, 0x13, 0x50, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x07, 0xbc, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x25, 0xdc, 0x00, 0x18, 0x32},
// 5TVI 1080P30
    {0xc8, 0x03, 0x50, 0x03, 0xd2, 0x80, 0x29, 0x38, 0x47, 0x08, 0x98, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 6TVI 1080P25
    {0xc8, 0x03, 0x50, 0x03, 0xd2, 0x80, 0x29, 0x38, 0x47, 0x0a, 0x50, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 7HDA 720P30
    {0xce, 0x13, 0x70, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x06, 0x72, 0x40, 0x46, 0x36, 0x3c, 0xfe, 0x01, 0x2d, 0x00, 0x60, 0x3a, 0x5a, 0x40, 0x9d, 0xca, 0x01, 0xd0, 0x25, 0xdc, 0x00, 0x18, 0x32},
// 8HDA 720P25
    {0xce, 0x13, 0x71, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x07, 0xbc, 0x40, 0x46, 0x36, 0x3c, 0xfe, 0x01, 0x2d, 0x00, 0x60, 0x3a, 0x5a, 0x40, 0x9e, 0x20, 0x01, 0x90, 0x25, 0xdc, 0x00, 0x18, 0x32},
// 9HDA 1080P30
    {0xcc, 0x03, 0x72, 0x01, 0xf0, 0x80, 0x29, 0x38, 0x47, 0x08, 0x98, 0x38, 0x46, 0x36, 0x3c, 0xfe, 0x0d, 0x2d, 0x00, 0x60, 0x3a, 0x54, 0x40, 0xa5, 0x95, 0xe0, 0x60, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 10HDA 1080P25
    {0xcc, 0x03, 0x73, 0x01, 0xf0, 0x80, 0x29, 0x38, 0x47, 0x0a, 0x50, 0x3c, 0x46, 0x36, 0x3c, 0xfe, 0x0d, 0x2d, 0x00, 0x60, 0x3a, 0x54, 0x40, 0xa5, 0x86, 0xfb, 0x60, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 11TVI 720P60
    {0xca, 0x03, 0x50, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x06, 0x72, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 12TVI 720P50
    {0xca, 0x03, 0x50, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x07, 0xbc, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 13TVI QHD30
    {0xc8, 0x03, 0x50, 0x23, 0x1b, 0x04, 0x38, 0xa0, 0x5a, 0x0c, 0xe2, 0x50, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x0a, 0x58, 0x70, 0x74, 0x58, 0x9f, 0x60, 0x15, 0xdc, 0x40, 0x40, 0x12},
// 14TVI QHD25
    {0xc8, 0x03, 0x50, 0x23, 0x1b, 0x04, 0x38, 0xa0, 0x5a, 0x0f, 0x76, 0x50, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x0a, 0x58, 0x70, 0x74, 0x58, 0x9f, 0x60, 0x15, 0xdc, 0x40, 0x40, 0x12},
// 15TVI 720P30V1
    {0xca, 0x03, 0x50, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x0c, 0xe4, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 16TVI 720P25V1
    {0xca, 0x03, 0x50, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x0f, 0x78, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 17TVI 8MP15
    {0xc8, 0x03, 0x50, 0x13, 0xbd, 0x04, 0x50, 0x70, 0x8f, 0x11, 0x2e, 0x50, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x0a, 0x58, 0x70, 0x74, 0x59, 0xbd, 0x60, 0x18, 0xca, 0x40, 0x40, 0x12},
// 18TVI 8MP12
    {0xc8, 0x03, 0x50, 0x13, 0xbd, 0x04, 0x50, 0x70, 0x8f, 0x14, 0x9e, 0x50, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x0a, 0x58, 0x70, 0x74, 0x59, 0xbd, 0x60, 0x18, 0xca, 0x40, 0x40, 0x12},
// 19TVI 5MP20
    {0xc8, 0x03, 0x50, 0x23, 0x36, 0x24, 0x1a, 0x98, 0x7a, 0x0e, 0xa2, 0x50, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x0a, 0x54, 0x70, 0x74, 0xa7, 0x18, 0x50, 0x17, 0xbc, 0x40, 0x40, 0x12},
// 20TVI 1080P60
    {0xc8, 0x03, 0x50, 0x03, 0xde, 0x80, 0x28, 0x38, 0x47, 0x08, 0x94, 0x50, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x0a, 0x40, 0x70, 0x74, 0x9b, 0xa5, 0xe0, 0x05, 0xbc, 0x40, 0x40, 0x12},
// 21TVI 5M12.5
    {0xc8, 0x03, 0x50, 0x13, 0x1f, 0x20, 0x34, 0x98, 0x7a, 0x0b, 0x9a, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x17, 0xd0, 0x00, 0x1c, 0x32},
// 22HDA 5M12.5
    {0xc8, 0x03, 0x50, 0x13, 0x10, 0x20, 0x1a, 0x98, 0x7a, 0x0b, 0xb8, 0x38, 0x46, 0x36, 0x3c, 0xfe, 0x01, 0x2d, 0x00, 0x60, 0x0a, 0x44, 0x40, 0x29, 0x68, 0x72, 0xb0, 0x17, 0xbc, 0x00, 0x1c, 0x32},
// 23TVI 720P30HDR
    {0xca, 0x13, 0x50, 0x03, 0xb2, 0x00, 0x60, 0xd0, 0x25, 0x05, 0xdc, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x28, 0x70, 0x48, 0xba, 0x2e, 0x90, 0x33, 0x39, 0x00, 0x18, 0x32},
// 24TVI WXGA
//  {0xca,0x13,0x50, 0x03,0xf4,0x00,0xa0,0x20,0x35,0x06,0x90, 0x60,0x84,0x36,0x3c, 0xff,0x05,0x2d,0x00, 0x60,0x0a,0x32,0x70, 0x48,0xbb,0x2e,0x90, 0x13,0xc0,0x00,0x18,0x32},
    {0xca, 0x13, 0x50, 0x03, 0xee, 0x00, 0x90, 0xd0, 0x25, 0x06, 0x3c, 0x60, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x32, 0x70, 0x4c, 0x30, 0xc3, 0x00, 0x13, 0xe8, 0x00, 0x18, 0x32},
// 25TVI 1080P27.5
    {0xc8, 0x03, 0x50, 0x13, 0x98, 0x80, 0x29, 0x38, 0x47, 0x09, 0x60, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 26TVI 1080P24
    {0xc8, 0x03, 0x50, 0x13, 0x40, 0x80, 0x29, 0x38, 0x47, 0x0a, 0xbe, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x05, 0xdc, 0x00, 0x1c, 0x32},
// 27TVI 1080P15
    {0xc8, 0x03, 0x50, 0x03, 0xd2, 0x80, 0x29, 0x38, 0x47, 0x08, 0x98, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xbb, 0x2e, 0x90, 0x25, 0xdc, 0x00, 0x18, 0x32},
// 28TVI 1280x320P60
    {0xca, 0x03, 0x50, 0x13, 0x10, 0x00, 0x32, 0x40, 0x15, 0x06, 0x72, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xba, 0x2e, 0x90, 0x31, 0x77, 0x00, 0x18, 0x32},
// 29HDA 720P30 for TP2860
    {0xca, 0x13, 0x70, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x86, 0x70, 0x38, 0x46, 0x36, 0x3c, 0xfe, 0x01, 0xad, 0x00, 0x60, 0x3a, 0x48, 0x40, 0x4e, 0xe5, 0x00, 0xf0, 0x25, 0xdc, 0x00, 0x08, 0x32},
// 30HDA 720P25 for TP2860
    {0xca, 0x13, 0x70, 0x13, 0x16, 0x00, 0x19, 0xd0, 0x25, 0x87, 0xba, 0x38, 0x46, 0x36, 0x3c, 0xfe, 0x01, 0xad, 0x00, 0x60, 0x3a, 0x48, 0x40, 0x4f, 0x10, 0x08, 0x40, 0x25, 0xdc, 0x00, 0x08, 0x32},
// 31HDA 1080P30 for TP2860
    {0xc8, 0x03, 0x70, 0x01, 0xf0, 0x80, 0x29, 0x38, 0x47, 0x88, 0x96, 0x38, 0x46, 0x36, 0x3c, 0xfe, 0x0d, 0xad, 0x00, 0x60, 0x3a, 0x48, 0x40, 0x52, 0xca, 0xf0, 0x20, 0x25, 0xdc, 0x00, 0x0c, 0x32},
// 32HDA 1080P25 for TP2860
    {0xc8, 0x03, 0x70, 0x01, 0xf0, 0x80, 0x29, 0x38, 0x47, 0x8a, 0x4e, 0x3c, 0x46, 0x36, 0x3c, 0xfe, 0x0d, 0xad, 0x00, 0x60, 0x3a, 0x48, 0x40, 0x52, 0xc3, 0x7d, 0xa0, 0x25, 0xdc, 0x00, 0x0c, 0x32},
// 33HDA 960P25 for TP2860
    {0xc8, 0x03, 0x70, 0x03, 0xe6, 0x00, 0x89, 0xc0, 0x35, 0x8a, 0x8a, 0x40, 0x46, 0x36, 0x3c, 0xfe, 0x05, 0xad, 0x00, 0x60, 0x3a, 0x50, 0x58, 0x52, 0xd2, 0x1c, 0x00, 0x34, 0x4c, 0x00, 0x0c, 0x32},
// 34 NTSC 720H
    // {0xcf,0x13,0x50, 0x13,0x60,0xbc,0x12,0xf0,0x07,0x09,0x38, 0x40,0x84,0x36,0x3c, 0xff,0x05,0x2d,0x00, 0x70,0x2a,0x68,0x57, 0x62,0xbb,0x96,0xc0, 0x25,0xdc,0x00,0x04,0x32},
    {0xcf, 0x13, 0x50, 0x03, 0xd6, 0xa0, 0x12, 0xf0, 0x05, 0x06, 0xb4, 0x40, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x70, 0x2a, 0x4b, 0x57, 0x62, 0xbb, 0x96, 0xcb, 0x65, 0xdc, 0x00, 0x04, 0x32},
// 35 PAL 720H
    // {0xcf,0x13,0x51, 0x13,0x76,0xbc,0x17,0x20,0x17,0x09,0x48, 0x48,0x84,0x37,0x3f, 0xff,0x05,0x2d,0x00, 0x70,0x2a,0x64,0x56, 0x7a,0x4a,0x4d,0xf0, 0x25,0xdc,0x00,0x04,0x32},
    {0xcf, 0x13, 0x51, 0x03, 0xf0, 0xa0, 0x17, 0x20, 0x15, 0x06, 0xc0, 0x48, 0x84, 0x37, 0x3f, 0xff, 0x05, 0x2d, 0x00, 0x70, 0x2a, 0x4b, 0x56, 0x7a, 0x4a, 0x4d, 0xfb, 0x65, 0xdc, 0x00, 0x04, 0x32},
// 36 TVI 960P30
    {0xc8, 0x03, 0x50, 0x13, 0x16, 0x00, 0xa0, 0xc0, 0x35, 0x06, 0x72, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x43, 0x3b, 0x79, 0x90, 0x14, 0x65, 0x00, 0x18, 0x32},           // reg30-33 = 0x43_3B_79_90(29.2500M)
// 37 TVI 960P25
    {0xc8, 0x03, 0x50, 0x13, 0x16, 0x00, 0xa0, 0xc0, 0x35, 0x07, 0xbc, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x48, 0xba, 0x2e, 0x90, 0x14, 0x65, 0x00, 0x18, 0x32},           // reg30-33 = 0x43_3B_79_90(29.2500M)
// 38 AHD 960P30 54M 1800x1000
    {0xca, 0x13, 0x50, 0x13, 0x60, 0x00, 0x90, 0xd0, 0x25, 0x07, 0x08, 0x60, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x3a, 0x32, 0x70, 0x40, 0x62, 0x4d, 0xd3, 0x13, 0xe8, 0x00, 0x18, 0x32},
// 39 AHD 960P30 54M 1800x1000
    {0xca, 0x13, 0x50, 0x13, 0x60, 0x00, 0x90, 0xd0, 0x25, 0x87, 0x06, 0x60, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x3a, 0x32, 0x70, 0x40, 0x62, 0x4d, 0xd3, 0x33, 0xe8, 0x00, 0x18, 0x32},
// 40 TVI 960P30 54M
    {0xc8, 0x03, 0x50, 0x13, 0x16, 0x00, 0xa0, 0xc0, 0x35, 0x06, 0x40, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x45, 0x55, 0x55, 0x50, 0x14, 0x65, 0x00, 0x18, 0x32},           // reg30-33 = 0x43_3B_79_90(29.2500M)
// 41 TVI 960P25 54M
    {0xc8, 0x03, 0x50, 0x13, 0x16, 0x00, 0xa0, 0xc0, 0x35, 0x07, 0x80, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x30, 0x70, 0x4b, 0x00, 0x00, 0x00, 0x14, 0x65, 0x00, 0x18, 0x32},           // reg30-33 = 0x43_3B_79_90(29.2500M)
// 42 TVI 720P25HDR
    {0xca, 0x13, 0x50, 0x03, 0xb2, 0x00, 0x60, 0xd0, 0x25, 0x07, 0x08, 0x30, 0x84, 0x36, 0x3c, 0xff, 0x05, 0x2d, 0x00, 0x60, 0x0a, 0x28, 0x70, 0x48, 0xba, 0x2e, 0x90, 0x33, 0x39, 0x00, 0x18, 0x32},
// 43 AHD 960P30 1650x1125
    {0xcc, 0x03, 0x70, 0x12, 0x12, 0x00, 0xa0, 0xc0, 0x35, 0x06, 0x72, 0x48, 0x84, 0x36, 0x3c, 0xff, 0x0d, 0x2d, 0x00, 0x60, 0x0a, 0x58, 0x70, 0x9d, 0xca, 0x01, 0xe0, 0x14, 0x65, 0x00, 0x88, 0x32},           // ~17M
// 44 AHD 960P30 1400x1125
    {0xc8, 0x03, 0x76, 0x03, 0x5f, 0x00, 0x9c, 0xc0, 0x35, 0x85, 0x78, 0x14, 0x84, 0x36, 0x3c, 0xff, 0x0d, 0x2d, 0x00, 0x60, 0x0a, 0x1e, 0x50, 0x29, 0x01, 0x76, 0x80, 0x14, 0x65, 0x00, 0x88, 0x32},           // ~15M
// 45 TVI 1080P50
    {0xc8, 0x03, 0x50, 0x03, 0xde, 0x80, 0x28, 0x38, 0x47, 0x0a, 0x48, 0x50, 0x84, 0x36, 0x3c, 0xff, 0x05, 0xad, 0x00, 0x60, 0x0a, 0x40, 0x70, 0x74, 0x9b, 0xa5, 0xe0, 0x05, 0xbc, 0x40, 0x40, 0x12},
};

// =============================================================================
//                Private Function Definition
// =============================================================================
static void
TP2825BEVK_Switch_Vin (
    void)
{
    unsigned char tmp = 0;

    tmp = tp28xx_byte_read(0x41);
    tmp &= 0x07;
    tmp |= TP2825B_VIN5;// Vin5
    tp28xx_byte_write(0x41, tmp);

    // vin5 with inside PTZ2
    tmp = tp28xx_byte_read(0xf0);
    tmp &= 0xcf;
    tmp |= 0x20;
    tp28xx_byte_write(0xf0, tmp);

    tp28xx_byte_write(0x38, 0x40);  // single-end
    tp28xx_byte_write(0x3d, 0x60);
    // tp28xx_byte_write(0x3e, 0x00);
}

static void
TP2825B_SYSCLK_54M (
    void)
{
    tp28xx_byte_write(0x40, 0x08);
    tp28xx_byte_write(0x11, 0x4e);
    tp28xx_byte_write(0x13, 0x04);
    tp28xx_byte_write(0x14, 0x04);
    tp28xx_byte_write(0x40, 0x00);
}

static void
TP2825B_SYSCLK_945 (
    void)
{
    tp28xx_byte_write(0x40, 0x08);
    tp28xx_byte_write(0x11, 0x5A);
    tp28xx_byte_write(0x13, 0x24);
    tp28xx_byte_write(0x14, 0x04);
    tp28xx_byte_write(0x40, 0x00);
}

static void
TP2825B_SYSCLK_V2 (
    void)
{
    tp28xx_byte_write(0x40, 0x08);
    tp28xx_byte_write(0x11, 0x54);
    tp28xx_byte_write(0x13, 0x24);
    tp28xx_byte_write(0x14, 0x04);
    tp28xx_byte_write(0x40, 0x00);
}

static void
TP2825B_SYSCLK_V1 (
    void)
{
    tp28xx_byte_write(0x40, 0x08);
    tp28xx_byte_write(0x11, 0x54);
    tp28xx_byte_write(0x13, 0x04);
    tp28xx_byte_write(0x14, 0x04);
    tp28xx_byte_write(0x40, 0x00);
}

static void
TP2825B_SYSCLK_V3 (
    void)
{
    uint8_t tmp = 0;

    tp28xx_byte_write(0x40, 0x08);
    tp28xx_byte_write(0x11, 0x54);
    tp28xx_byte_write(0x13, 0x04);
    tp28xx_byte_write(0x14, 0x73);
    tp28xx_byte_write(0x40, 0x00);
    if (!BT1120_out)
    {
        tmp = tp28xx_byte_read(0x35);
        tmp |= 0x40;
        tp28xx_byte_write(0x35, tmp);
    }
}

static void
TP2825B_reset_default ()
{
    uint8_t tmp;

    TP2825B_SYSCLK_V1();
    tp28xx_byte_write(0x35, 0x05);
    tp28xx_byte_write(0x07, 0xc0);
    tp28xx_byte_write(0x0b, 0xc0);
    tp28xx_byte_write(0x26, 0x04);
    tmp = tp28xx_byte_read(0x06);
    tmp &= 0xfb;
    tp28xx_byte_write(0x06, tmp);
}

//////////////////////////////////////////////////////////////////////////////
// PTZ TX init
//////////////////////////////////////////////////////////////////////////////
void
TP2825B_SetTX (
    uint8_t index)
{
    uint8_t i;
    // unsigned char page_bak;

    uint8_t PTZ_dat[][10] =
    {
        {0x02, 0x00, 0x00, 0x0b, 0x0c, 0x0d, 0x0e, 0x19, 0x78, 0x21},   // TVI 1080p
        {0x02, 0x00, 0x00, 0x0b, 0x0c, 0x0d, 0x0e, 0x19, 0x78, 0x21},   // TVI 720p
        {0x02, 0x00, 0x00, 0x0e, 0x0f, 0x10, 0x11, 0x24, 0xf0, 0x57},   // ACP2.0 for 2825B 148.5M
        {0x02, 0x00, 0x00, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x00, 0x57},   // ACP2.0 for 2825B 74.25M
        {0x02, 0x00, 0x00, 0x0f, 0x10, 0x00, 0x00, 0x12, 0xf0, 0x6f},   // 960H for 2825B 74.25M
        {0x0a, 0x1d, 0x80, 0x10, 0x11, 0x12, 0x13, 0x15, 0xb8, 0x9f},   // HDC 2825B 148.5M
        {0x0a, 0x1d, 0x80, 0x10, 0x11, 0x12, 0x13, 0x0a, 0x00, 0x9f},   // HDC 2825B 74.25M
        {0x02, 0x00, 0x00, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x00, 0x57},   // ACP2.0 for 2825B 74.25M
        // {0x02,0x00,0x00,0x0a,0x0b,0x0c,0x0d,0x0b,0x5a,0x17}, //BYD AHD960p30 94.5M
        {0x02, 0x00, 0x00, 0x0d, 0x0e, 0x0f, 0x10, 0x0b, 0x5a, 0x17},   // BYD AHD960p30 94.5M
    };
    uint8_t PTZ_reg[10] = {0x6f, 0x70, 0x71, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8};

    if (PTZ_960P == index)
    {
        index = 0;
    }
    else if (PTZ_960P30_AHD == index)
    {
        index = 8;
    }

    for (i = 0; i < 10; i++)
    {
        tp28xx_byte_write(PTZ_reg[i], PTZ_dat[index][i]);
    }
    for (i = 0; i < 24; i++)
    {
        tp28xx_byte_write(0x55 + i, 0x00);
    }
}

//////////////////////////////////////////////////////////////////////////////
// PTZ RX
//////////////////////////////////////////////////////////////////////////////
void
TP2825B_SetRX (
    uint8_t index)
{
    uint8_t i;

    // reg0xc9~d7 A8 A7
    uint8_t PTZ_dat[][17] =
    {
        {0x00, 0x00, 0x07, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x60, 0x10, 0x06, 0xbe, 0x39, 0x27, 0x00, 0x03}, // TVI 1080p/960p
        {0x00, 0x00, 0x07, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x60, 0x08, 0x06, 0x5f, 0x39, 0x27, 0x00, 0x03}, // TVI 720p
        {0x00, 0x00, 0x06, 0x07, 0x08, 0x09, 0x03, 0x48, 0x34, 0x60, 0x38, 0x04, 0xf0, 0xd8, 0x07, 0x00, 0x27}, // AHD 1080p
        {0x00, 0x00, 0x06, 0x07, 0x08, 0x09, 0x03, 0x48, 0x34, 0x60, 0x1c, 0x04, 0xf0, 0xd8, 0x17, 0x00, 0x27}, // AHD 720p
        {0x00, 0x00, 0x07, 0x08, 0x00, 0x00, 0x04, 0xec, 0x4e, 0x60, 0x10, 0x06, 0xbe, 0x39, 0x27, 0x00, 0x03}, // BYD new A960p
        {0x00, 0x00, 0x07, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x60, 0x10, 0x06, 0xbe, 0x39, 0x27, 0x00, 0x03}, // TVI 1080p/960p
        {0x00, 0x00, 0x07, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x60, 0x08, 0x06, 0x5f, 0x39, 0x27, 0x00, 0x03}, // TVI 720p
        // {0x00,0x00,0x05,0x06,0x07,0x08,0x04,0x69,0xee,0x60,0x28,0x06,0xbe,0x39,0x07, 0x00,0x27}, //BYD new A960p30
        {0x00, 0x00, 0x08, 0x09, 0x0a, 0x0b, 0x04, 0x69, 0xee, 0x60, 0x28, 0x06, 0xbe, 0x39, 0x07, 0x00, 0x27}, // BYD new A960p30
    };
    /* only valid for TP2850/TP2825B
            tp28xx_byte_write(0x40, 0x08); //mipi page
            i = tp28xx_byte_read(0x13);
            if(0x20&i) index +=1; //720p setting
            tp28xx_byte_write(0x40, 0x00); //mipi page
    */
    if (PTZ_960P == index)
    {
        index = 4;
    }
    else if (PTZ_960P30_AHD == index)
    {
        index = 7;
    }
    for (i = 0; i < 15; i++)
    {
        tp28xx_byte_write(0xc9 + i, PTZ_dat[index][i]);
    }
    tp28xx_byte_write(0xa8, PTZ_dat[index][15]);
    tp28xx_byte_write(0xa7, PTZ_dat[index][16]);
}

void
Set_VidRes (
    uint8_t Res,
    uint8_t Std)
{
    uint8_t ptz;
    uint8_t i, index, tmp;

    ptz     = PTZ_TVI;
    index   = 0xff;

    // (void)printf("[Set_VidRes]Res = %d, Std = %d\n", Res, Std);

    if (STD_HDA == Std)
    {
        ptz = PTZ_HDA_1;
        switch (Res)
        {
            case TP2802_1080P30:

                if (en_960P)
                {
                    index   = 44;
                    ptz     = PTZ_960P30_AHD;
                    TP2825B_SYSCLK_945();
                    tp28xx_byte_write(0x05, 0x01);
                    tp28xx_byte_write(0x0e, 0x12);
                }
                else
                {
                    index = 9;
                    TP2825B_SYSCLK_V1();
                }
                break;

            case TP2802_1080P25:

                if (en_960P)
                {
                }
                else
                {
                    index = 10;
                    TP2825B_SYSCLK_V1();
                }

                break;

            case TP2802_720P30V2:

                index = 7;
                TP2825B_SYSCLK_V2();
                break;

            case TP2802_720P25V2:

                index = 8;
                TP2825B_SYSCLK_V2();

                break;

            case TP2802_WXGA:

                index = 38;

                TP2825B_SYSCLK_54M();

                break;

            default:
                break;
        }
    }
    else if ((STD_HDC == Std) || (STD_HDC_DEFAULT == Std))
    {
    }
    else
    {
        switch (Res)
        {
            case TP2802_1080P30:
                if (en_960P)
                {
                    index = 40;
                    TP2825B_SYSCLK_54M();
                }
                else
                {
                    index = 5;
                    TP2825B_SYSCLK_V1();
                }
                break;

            case TP2802_1080P25:
                if (en_960P)
                {
                    index = 41;
                    TP2825B_SYSCLK_54M();
                }
                else
                {
                    index = 6;
                    TP2825B_SYSCLK_V1();
                }
                break;

            case TP2802_720P30V2:

                index   = 3;
                ptz     = PTZ_TVI_720P;
                TP2825B_SYSCLK_V2();

                break;

            case TP2802_720P25V2:
                index   = 4;
                ptz     = PTZ_TVI_720P;
                TP2825B_SYSCLK_V2();

                break;

            case TP2802_720P60:
                index = 11;
                TP2825B_SYSCLK_V1();
                break;

            case TP2802_720P50:
                index = 12;
                TP2825B_SYSCLK_V1();

                break;

            case TP2802_720P30:
                index = 15;
                TP2825B_SYSCLK_V1();
                break;

            case TP2802_720P25:
                index = 16;
                TP2825B_SYSCLK_V1();
                break;

                break;

            case TP2802_1080P60:
                index = 20;
                TP2825B_SYSCLK_V3();

                break;

            case TP2802_1080P50:
                index = 45;
                TP2825B_SYSCLK_V3();

                break;

            case TP2802_NTSC:
                ptz     = PTZ_CVBS;
                TP2825B_SYSCLK_V2();
                // index = 1; //960H
                index   = 34;
                break;

            case TP2802_PAL:
                ptz     = PTZ_CVBS;
                TP2825B_SYSCLK_V2();
                // index = 2; //960H
                index   = 35;
                break;

                break;

            case TP2802_720P30HDR:
                index   = 23;
                ptz     = PTZ_TVI_720P;
                TP2825B_SYSCLK_V2();

                break;

            case TP2802_720P25HDR:
                index   = 42;
                ptz     = PTZ_TVI_720P;
                TP2825B_SYSCLK_V2();

                break;

            case TP2802_WXGA:
                index = 24;
                TP2825B_SYSCLK_945();
                tp28xx_byte_write(0x0b, 0x80);

                break;

            case TP2802_1080P27:
                index = 25;
                TP2825B_SYSCLK_V1();

                break;

            case TP2802_1080P24:
                index = 26;
                TP2825B_SYSCLK_V1();

                break;

            case TP2802_1080P15:
                index   = 27;
                ptz     = PTZ_TVI_720P;
                TP2825B_SYSCLK_V2();

                break;

                // case TP2802_1280x320P60:

                // index = 28;
                // ptz = PTZ_TVI_720P;
                // TP2825B_SYSCLK_V2();

                // break;
            default:
                break;
        }
    }

    if (index != 0xff)
    {
        for (i = 0; i < 32; i++)
        {
            tp28xx_byte_write(TP2825B_FORMAT_DataSet[0][i], TP2825B_FORMAT_DataSet[index][i]);
        }

        if (BT1120_out)
        {
            tmp = tp28xx_byte_read(0x02);
            tmp &= 0x7f;
            tp28xx_byte_write(0x02, tmp);
        }

        TP2825B_SetTX(ptz);
        TP2825B_SetRX(ptz);

        tmp = tp28xx_byte_read(0x06);
        tmp |= 0x80;
        tp28xx_byte_write(0x06, tmp);
    }
}

void
EQ_Detect ()
{
    uint8_t cvstd = 0, status = 0, i = 0;
    uint8_t gain = 0, tmp = 0;

    i = 0;
    {
        status = tp28xx_byte_read(0x01);
        // (void)printf("reg0x1 status = %x\n", status);
        // state machine for video checking
        if (status & FLAG_LOSS)
        {
            if (VIDEO_UNPLUG == state[i])
            {
                TP2825BEVK_Switch_Vin();
            }
            else
            {
                state[i]    = VIDEO_UNPLUG;
                count[i]    = 0;
                std[i]      = INVALID_FORMAT;
                TP2825B_reset_default();
                (void)printf("[TP2825B]video loss \n");
            }
        }
        else
        {
            if (FLAG_LOCKED == (status & FLAG_LOCKED))  // video locked
            {
                if (VIDEO_LOCKED == state[i])
                {
                    count[i]++;
                }
                else if (VIDEO_UNPLUG == state[i])
                {
                    state[i]    = VIDEO_IN;
                    count[i]    = 0;
                }
                else    // if(mode[i] != INVALID_FORMAT)
                {
                    state[i]    = VIDEO_LOCKED;
                    count[i]    = 0;
                }
            }
            else        // video in but unlocked
            {
                if (VIDEO_UNPLUG == state[i])
                {
                    state[i]    = VIDEO_IN;
                    count[i]    = 0;
                    (void)printf("[TP2825B]video in \r\n");
                }
                else if (VIDEO_LOCKED == state[i])
                {
                    state[i]    = VIDEO_UNLOCK;
                    count[i]    = 0;
                    (void)printf("[TP2825B]video unstable \r\n");
                }
                else
                {
                    count[i]++;
                    if ((VIDEO_UNLOCK == state[i]) && (count[i] > 2))
                    {
                        state[i]    = VIDEO_IN;
                        count[i]    = 0;
                        TP2825B_reset_default();
                        (void)printf("[TP2825B]video unlocked \r\n");
                    }
                }
            }
        }

        if (VIDEO_IN == state[i])
        {
            cvstd   = tp28xx_byte_read(0x03);
            (void)printf("[TP2825B]video format %2x\r\n", (cvstd));
            cvstd   &= 0x07;
            std[i]  = STD_TVI;

            if (cvstd > 0x02)
            {
                tp28xx_byte_write(0x2f, 0x09);
                tmp = tp28xx_byte_read(0x04);
                (void)printf("[TP2825B]extend format %2x\r\n", (tmp));

                if (0x4e == tmp)
                {
                }
                else if (0x4d == tmp)
                {
                    mode[i] = TP2802_WXGA;
                }
                else if (0x5d == tmp)
                {
                    if (count[i] & 1)
                    {
                        // mode[i] = TP2802_5M12;
                        // std[i] = STD_HDA;
                        // tp2802_set_video_mode(iChip, wdi-> mode[i], i,  wdi-> std[i]);
                    }
                    else
                    {
                        mode[i] = TP2802_720P30HDR;
                        // std[i] = STD_TVI;
                        // tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                    }
                }
                else if (0x70 == tmp)
                {
                    mode[i] = TP2802_720P25HDR;
                }
                else if (0x5c == tmp)
                {
                    // mode[i] = TP2802_5M12;
                }
                else if (0x3a == tmp)  // invalid for TP2853C
                {
                    // mode[i] = TP2802_5M20;
                }

                else if (0x89 == tmp)
                {
                    mode[i] = TP2802_1080P15;
                }

                else if (0x22 == tmp)
                {
                    mode[i] = TP2802_1080P60;
                }
                else if (0x29 == tmp)
                {
                    mode[i] = TP2802_1080P50;
                }
                else if (0x4a == tmp)
                {
                    mode[i] = TP2802_1080P27;
                }
                else if (0x55 == tmp)
                {
                    mode[i] = TP2802_1080P24;
                }
                else if (0x94 == tmp)
                {
                    mode[i] = TP2802_PAL;
                }
                else if (0x93 == tmp)
                {
                    mode[i] = TP2802_NTSC;
                }
                else if (0x54 == tmp)
                {
                    mode[i] = TP2802_960P25;
                }
                else if (0x52 == tmp)
                {
                    mode[i] = TP2802_1080P25;
                    if (count[i] & 0x01)
                    {
                        // mode[i] = TP2802_WXGA;
                        // mode[i] = TP2802_8M12;
                    }
                }
                else if (0x67 == tmp)
                {
                    // if(count[i] & 0x01)
                    //    mode[i] = TP2802_1280x320P60;
                    // else
                    mode[i] = TP2802_720P30V2;
                }
                else if (0x7b == tmp)
                {
                    mode[i] = TP2802_720P25V2;
                }
                // Set_VidRes(mode[i], STD_TVI);
            }
            else
            {
                mode[i] = cvstd;

                if (DetAutoStd)
                {
                    if (TP2802_1080P30 == cvstd)
                    {
                        if (en_960P)
                        {
                            std[i] = STD_HDA;
                        }
                    }
                    else if (TP2802_720P60 == cvstd)
                    {
                        // if(count[i] & 0x01)
                        // {
                        //    mode[i] = TP2802_QHD30;
                        // }
                    }
                    else if (TP2802_720P50 == cvstd)
                    {
                        // if(count[i] & 0x01)
                        // {
                        // mode[i] = TP2802_QHD25;
                        // }
                    }
                }
            }
            Set_VidRes(mode[i], std[i]);
        }
#define EQ_COUNT 10
        if (VIDEO_LOCKED == state[i]) // check signal lock
        {
            if (0 == count[i])
            {
                (void)printf("[TP2825B]count=0 \r\n");

                tmp = tp28xx_byte_read(0x26);

                tmp |= 0x01;
                tp28xx_byte_write(0x26, tmp);
                if ((TP2802_720P30V2 == mode[i]) || (TP2802_720P25V2 == mode[i]))
                {
                    if (!(0x08 & tp28xx_byte_read(0x03)))
                    {
                        (void)printf("[TP2825B]720P V1 Detected ch%02x \r\n", i);
                        if (TP2802_720P30V2 == mode[i]) // to speed the switching
                        {
                            mode[i] = TP2802_720P30;
                        }
                        else if (TP2802_720P25V2 == mode[i])
                        {
                            mode[i] = TP2802_720P25;
                        }
                        Set_VidRes(mode[i], STD_TVI);
                    }
                }
            }
            else if (1 == count[i])
            {
                // count[i]++;
                (void)printf("[TP2825B]count=1 \r\n");
                tmp = tp28xx_byte_read(0x01);

                if ((TP2802_PAL == mode[i]) || (TP2802_NTSC == mode[i]))
                {
                    // do nothing
                }
                else if ((0x60 == (tmp & 0x64)) && DetAutoStd)
                {
                    (void)printf("[TP2825B]UTC set\r\n");

                    if (std[i] == STD_TVI)
                    {
                        std[i] = STD_HDA;
                        Set_VidRes(mode[i], std[i]);
                    }
                }
                else
                {
                    if ((std[i] == STD_HDA) && DetAutoStd)
                    {
                        std[i] = STD_TVI;
                        Set_VidRes(mode[i], std[i]);
                    }
                }

                if (half_scaler) // down scaler output
                {
                    tmp = tp28xx_byte_read(0x35);
                    tmp |= 0x40;
                    tp28xx_byte_write(0x35, tmp);
                }
            }
            else if (count[i] < EQ_COUNT - 3)
            {
                egain[i][3] = egain[i][2];
                egain[i][2] = egain[i][1];
                egain[i][1] = egain[i][0];
                gain        = tp28xx_byte_read(0x03);
                // egain[i][EQ_COUNT - count[i]] = gain&0xf0;
                egain[i][0] = gain & 0xf0;
                (void)printf("[TP2825B]Egain=0x%02x ch%02x\r\n", gain, i);
            }
            else if (EQ_COUNT == count[i])
            {
                count[i]--;

                egain[i][3] = egain[i][2];
                egain[i][2] = egain[i][1];
                egain[i][1] = egain[i][0];

                gain        = tp28xx_byte_read(0x03);
                egain[i][0] = gain & 0xf0;
                if ((abs(egain[i][3] - egain[i][0]) < 0x20) && (abs(egain[i][2] - egain[i][0]) < 0x20) && (abs(egain[i][1] - egain[i][0]) < 0x20))
                {
                    count[i]++;

                    tmp = tp28xx_byte_read(0x03);
                    (void)printf("[TP2825B]result Egain=0x%02x\r\n", tmp);
                }
            }

            else if (EQ_COUNT + 1 == count[i])
            {
                tp28xx_byte_write(0x2f, 00);
                #if 0
                gain = tp28xx_byte_read(0x04);

                // if( STD_TVI !=  std[i] )
                // {
                // tp28xx_byte_write(iChip, 0x07, 0x80|(gain>>2));  // manual mode
                // tp28xx_byte_write(0x07, 0x80|(gain>>2));
                // }
                #endif
            }
            else if (EQ_COUNT + 2 == count[i])
            {
                // count[i]++;
                if (STD_HDC_DEFAULT == std[i])
                {
                    tp28xx_byte_write(0x2f, 0x0c);
                    #if 0
                    tmp     = tp28xx_byte_read(0x04);
                    #endif
                    status  = tp28xx_byte_read(0x01);
                    // if(0x10 == (0x11 & status) && (tmp < 0x18 || tmp > 0xf0))
                    if (0x10 == (0x11 & status))
                    {
                        std[i] = STD_HDC;
                        // TP2825B_SetTX(0x05);
                    }
                    else
                    {
                        std[i] = STD_HDA;
                        // TP2825B_SetTX(0x02);
                    }

                    // Set_VidRes(mode[i], std[i]);
                    // (void)printf("reg01=%02x reg04@2f=0c %02x std%02x \r\n", (WORD)status, (WORD)tmp, (WORD)std[i]);
                }
                else
                {
                    // TP2825B_SetTX(0x00);
                }

                if (STD_TVI != std[i])
                {
                    // tp2802_manual_agc(iChip, i); //fix AGC when PTZ operation, it can be removed on TP2826/2827
                    // tp283x_egain(iChip, 0x0c);
                }
            }
            else
            {
            }
        }
    }
}

// =============================================================================
//                USER IMPLEMENT FUNCTION START
// =============================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////

// X10LightDriver_t1.c
void
TP2825BInitialize (
    uint16_t Mode)
{
    int i = 0;
    (void)printf("[TP2825B]Initialize\n");

    count[0]    = 0;
    state[0]    = VIDEO_UNPLUG;
    std[0]      = STD_HDA;
    mode[0]     = INVALID_FORMAT;

    tp28xx_I2c_Init();

    // input configuration,according to pcb layout
    TP2825BEVK_Switch_Vin();

    // I2CDeviceSet
    for (i = 0; i < sizeof(TP2825B_DataSet) / sizeof(REGPAIR); i++)
    {
        tp28xx_byte_write((TP2825B_DataSet[i].addr & 0xff), TP2825B_DataSet[i].value);
        usleep(50 * 1000);
    }

    // soft reset
    TP2825B_reset_default();

    // Set_VidRes(mode[0], std[0]);

    (void)printf("[TP2825B]Initialize End\n");
}

void
TP2825BTerminate (
    void)
{
    /* Please implement terminate code here */
}

void
TP2825BOutputPinTriState (
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
TP2825BIsSignalStable (
    uint16_t Mode)
{
    /* Please implement checking signal stable code here */
    bool isStable = false;

    EQ_Detect();

    usleep(200 * 1000);

    if (state[0] == VIDEO_LOCKED)
    {
        isStable = true;
    }

    return isStable;
}

uint16_t
TP2825BGetProperty (
    MODULE_GETPROPERTY property)
{
    int i = 0;
    for (i = 0; i < TP2825B_RES_TABLE_INDEX; i++)
    {
        if (cap_table[i].mode == mode[0])
        {
            break;
        }
    }

    if (i >= (TP2825B_RES_TABLE_INDEX - 1))
    {
        (void)printf("No Soupport This Video Mode\n");
        return 0;
    }

    /* Please implement get information from device code here */
    switch (property)
    {
    // case GetTopFieldPolarity:
        case GetHeight:
            return cap_table[i].height;
            break;

        case GetWidth:
            return cap_table[i].width;
            break;

            // frame rate
        case Rate:
            return cap_table[i].framerate;
            break;

        case GetModuleIsInterlace:
            return cap_table[i].interlaced;
            break;

        case matchResolution:
            return 1;
            break;

        case GetIsAnalogDecoder:
            return 1;
            break;

        default:
            return 0;
            break;
    }
}

uint8_t
TP2825BGetStatus (
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
TP2825BSetProperty (
    MODULE_SETPROPERTY  Property,
    uint16_t            Value)
{
    /* Please implement set property to device code here */
}

void
TP2825BPowerDown (
    uint8_t enable)
{
    /* Please implement power down code here */
    if (enable == false)
    {
#ifdef CFG_SENSOR_RESETPIN_ENABLE
        ithGpioClear(CFG_SN1_GPIO_RST);
        ithGpioSetOut(CFG_SN1_GPIO_RST);
        ithGpioSetMode(CFG_SN1_GPIO_RST, ITH_GPIO_MODE0);
        usleep(30 * 1000);
        ithGpioSet(CFG_SN1_GPIO_RST);
        usleep(100 * 1000);
        (void)printf("[TP2825B] Hw Reset\n");
#endif
    }
}

// =============================================================================
//                USER IMPLEMENT FUNCTION END
// =============================================================================
void
TP2825BSensorDriver_Destory (
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
    TP2825BInitialize,
    TP2825BTerminate,
    TP2825BOutputPinTriState,
    TP2825BIsSignalStable,
    TP2825BGetProperty,
    TP2825BGetStatus,
    TP2825BSetProperty,
    TP2825BPowerDown,
    TP2825BSensorDriver_Destory
};

SensorDriver
TP2825BSensorDriver_Create ()
{
    TP2825BSensorDriver self = calloc(1, sizeof(TP2825BSensorDriverStruct));
    if (self != NULL)
    {
        self->base.vtable   = &interface;
        self->base.type     = "TP2825B";
    }
    return (SensorDriver)self;
}

// end of X10LightDriver_t1.c
