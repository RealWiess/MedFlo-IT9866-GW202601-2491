// =============================================================================
// =============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "iic/mmp_iic.h"
#include "RN6854M.h"

// =============================================================================
//                Constant Definition
// =============================================================================
static uint8_t  IICADDR                     = 0x5A >> 1;            /*0x5A or 0x58 */
#ifdef CFG_SENSOR_ENABLE
static uint8_t  IICPORT                     = CFG_SENSOR_IIC_PORT;  /* please assign IIC PORT      */
#else
static uint8_t  IICPORT                     = IIC_PORT_2;
#endif

static int              gMasterDev          = 0;
static unsigned char    current_ch          = 0x0;
static RN68_VIDEO_STATE video_state         = RN_VIDEO_UNKNOWN;
static RN68_TIMING_ID   pre_dectect_format  = RN68_UNKNOWN;
static uint8_t          format_count        = 0;

// =============================================================================
//                Macro Definition
// =============================================================================
#define MAX_CHANNEL         4
#define FORMAT_CHECK_TIMES  15 // by camera

RN68_TIMING_ID CH_SET[MAX_CHANNEL] = {RN68_HD720P_30FPS, RN68_HD720P_30FPS, RN68_HD720P_30FPS, RN68_HD720P_30FPS};

// =============================================================================
//                Structure Definition
// =============================================================================
typedef struct _REGPAIR
{
    uint8_t addr;
    uint8_t value;
} REGPAIR;

typedef struct RN6854SensorDriverStruct
{
    SensorDriverStruct base;
} RN6854SensorDriverStruct;

// =============================================================================
//                IIC API FUNCTION START
// =============================================================================
void
RN68_I2c_init ()
{
    char portname[16] = {0};

    sprintf(portname, ":i2c%d", IICPORT);
    gMasterDev = open(portname, 0, 0);
}

uint8_t
RN68_ReadI2c_Byte (
    uint8_t RegAddr)
{
    int         result  = 0;
    uint8_t     dbuf[4] = {0};
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
        ithPrintf("RN68_ReadI2c_Byte read address 0x%02x error!\n", RegAddr);
    }
    // (void)printf("Reg = %x, value[0] = %x ,value[1] = %x\n",RegAddr, dbuf[0],dbuf[1]);
    return dbuf[1];
}

uint32_t
RN68_WriteI2c_Byte (
    uint8_t RegAddr,
    uint8_t data)
{
    int         result  = 0;
    uint8_t     dbuf[4] = {0};
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt     = {0};

    *pdbuf++            = (uint8_t)(RegAddr & 0xff);
    *pdbuf              = (uint8_t)(data);

    evt.slaveAddress    = IICADDR;
    evt.cmdBuffer       = dbuf;
    evt.cmdBufferSize   = 2;

    if (0 != (result = write(gMasterDev, (char *)&evt, 1)))
    {
        (void)printf("RN68_WriteI2c_Byte Write Error, reg=%02x val=%02x\n", RegAddr, data);
    }
    return result;
}

uint32_t
RN68_WriteI2c_ByteMask (
    uint8_t RegAddr,
    uint8_t data,
    uint8_t mask)
{
    uint8_t     value   = 0;
    uint32_t    flag    = 0;

    value   = RN68_ReadI2c_Byte(RegAddr);
    value   = ((value & ~mask) | (data & mask));
    flag    = RN68_WriteI2c_Byte(RegAddr, value);

    return flag;
}

// =============================================================================
//                IIC API FUNCTION END
// =============================================================================

// =============================================================================
//                Global Data Definition
// =============================================================================
REGPAIR RN685x_720P25fps[] = {
    {0x00, 0x20},   // internal use*
    {0x06, 0x08},   // internal use*
    {0x07, 0x63},   // HD format
    {0x2A, 0x01},   // filter control
    {0x3A, 0x00},   // No Insert Channel ID in SAV/EAV code
    {0x3F, 0x10},   // channel ID
    {0x4C, 0x37},   // equalizer
    {0x4F, 0x03},   // sync control
    {0x50, 0x02},   // 720p resolution
    {0x56, 0x02},   // 144M mode and BT656 mode
    {0x5F, 0x40},   // blank level
    {0x63, 0xF5},   // filter control
    {0x59, 0x00},   // extended register access
    {0x5A, 0x42},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x59, 0x33},   // extended register access
    {0x5A, 0x23},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x51, 0xE1},   // scale factor1
    {0x52, 0x88},   // scale factor2
    {0x53, 0x12},   // scale factor3
    {0x5B, 0x07},   // H-scaling control
    {0x5E, 0x08},   // enable H-scaling control
    {0x6A, 0x82},   // H-scaling control
    {0x28, 0x92},   // cropping
    {0x03, 0x80},   // saturation
    {0x04, 0x80},   // hue
    {0x05, 0x00},   // sharpness
    {0x57, 0x23},   // black/white stretch
    {0x68, 0x32},   // coring
    {0x37, 0x33},
    {0x61, 0x6C},
};

REGPAIR RN685x_720P30fps[] = {
    {0x00, 0x20},   // internal use*
    {0x06, 0x08},   // internal use*
    {0x07, 0x63},   // HD format
    {0x2A, 0x01},   // filter control
    {0x3A, 0x00},   // No Insert Channel ID in SAV/EAV code
    {0x3F, 0x10},   // channel ID
    {0x4C, 0x37},   // equalizer
    {0x4F, 0x03},   // sync control
    {0x50, 0x02},   // 720p resolution
    {0x56, 0x02},   // 144M mode and BT656 mode
    {0x5F, 0x40},   // blank level
    {0x63, 0xF5},   // filter control
    {0x59, 0x00},   // extended register access
    {0x5A, 0x44},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x59, 0x33},   // extended register access
    {0x5A, 0x23},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x51, 0x4E},   // scale factor1
    {0x52, 0x87},   // scale factor2
    {0x53, 0x12},   // scale factor3
    {0x5B, 0x07},   // H-scaling control
    {0x5E, 0x08},   // enable H-scaling control
    {0x6A, 0x82},   // H-scaling control
    {0x28, 0x92},   // cropping
    {0x03, 0x80},   // saturation
    {0x04, 0x80},   // hue
    {0x05, 0x00},   // sharpness
    {0x57, 0x23},   // black/white stretch
    {0x68, 0x32},   // coring
    {0x37, 0x33},
    {0x61, 0x6C},
};

REGPAIR RN685x_1080P25fps[] = {
    {0x00, 0x20},   // internal use*
    {0x06, 0x08},   // internal use*
    {0x07, 0x63},   // HD format
    {0x2A, 0x01},   // filter control
    {0x3A, 0x00},   // No Insert Channel ID in SAV/EAV code
    {0x3F, 0x10},   // channel ID
    {0x4C, 0x37},   // equalizer
    {0x4F, 0x03},   // sync control
    {0x50, 0x03},   // 1080p resolution
    {0x56, 0x02},   // 144M and BT656 mode
    {0x5F, 0x44},   // blank level
    {0x63, 0xF8},   // filter control
    {0x59, 0x00},   // extended register access
    {0x5A, 0x48},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x59, 0x33},   // extended register access
    {0x5A, 0x23},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x51, 0xF4},   // scale factor1
    {0x52, 0x29},   // scale factor2
    {0x53, 0x15},   // scale factor3
    {0x5B, 0x01},   // H-scaling control
    {0x5E, 0x08},   // enable H-scaling control
    {0x6A, 0x87},   // H-scaling control
    {0x28, 0x92},   // cropping
    {0x03, 0x80},   // saturation
    {0x04, 0x80},   // hue
    {0x05, 0x04},   // sharpness
    {0x57, 0x23},   // black/white stretch
    {0x68, 0x00},   // coring
    {0x37, 0x33},
    {0x61, 0x6C},
};

REGPAIR RN685x_1080P30fps[] = {
    {0x00, 0x20},   // internal use*
    {0x06, 0x08},   // internal use*
    {0x07, 0x63},   // HD format
    {0x2A, 0x01},   // filter control
    {0x3A, 0x00},   // No Insert Channel ID in SAV/EAV code
    {0x3F, 0x10},   // channel ID
    {0x4C, 0x37},   // equalizer
    {0x4F, 0x03},   // sync control
    {0x50, 0x03},   // 1080p resolution
    {0x56, 0x02},   // 144M and BT656 mode
    {0x5F, 0x44},   // blank level
    {0x63, 0xF8},   // filter control
    {0x59, 0x00},   // extended register access
    {0x5A, 0x49},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x59, 0x33},   // extended register access
    {0x5A, 0x23},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x51, 0xF4},   // scale factor1
    {0x52, 0x29},   // scale factor2
    {0x53, 0x15},   // scale factor3
    {0x5B, 0x01},   // H-scaling control
    {0x5E, 0x08},   // enable H-scaling control
    {0x6A, 0x87},   // H-scaling control
    {0x28, 0x92},   // cropping
    {0x03, 0x80},   // saturation
    {0x04, 0x80},   // hue
    {0x05, 0x04},   // sharpness
    {0x57, 0x23},   // black/white stretch
    {0x68, 0x00},   // coring
    {0x37, 0x33},
    {0x61, 0x6C},
};

REGPAIR RN685x_NTSC[] = {
    {0x00, 0x00},   // internal use*
    {0x06, 0x08},   // internal use*
    {0x07, 0x63},   // HD format
    {0x2A, 0x81},   // filter control
    {0x3A, 0x20},   // Insert Channel ID in SAV/EAV code
    {0x3F, 0x10},   // channel ID
    {0x4C, 0x37},   // equalizer
    {0x4F, 0x00},   // sync control
    {0x50, 0x00},   // D1 resolution
    {0x56, 0x00},   // 27M mode and BT656 mode
    {0x5F, 0x00},   // blank level
    {0x63, 0x75},   // filter control
    {0x59, 0x00},   // extended register access
    {0x5A, 0x00},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x59, 0x33},   // extended register access
    {0x5A, 0x02},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x5B, 0x00},   // H-scaling control
    {0x5E, 0x01},   // enable H-scaling control
    {0x6A, 0x00},   // H-scaling control
    {0x28, 0x92},   // cropping
    {0x20, 0x24},   // USE_CVBS_ONE_FIELD
    {0x23, 0x11},
    {0x24, 0x05},
    {0x25, 0x11},
    {0x26, 0x00},
    {0x42, 0x00},
    {0x03, 0x80},   // saturation
    {0x04, 0x80},   // hue
    {0x05, 0x03},   // sharpness
    {0x57, 0x20},   // black/white stretch
    {0x68, 0x32},   // coring
    {0x37, 0x33},
    {0x61, 0x6C},
};

REGPAIR RN685x_PAL[] = {
    {0x00, 0x00},   // internal use*
    {0x06, 0x08},   // internal use*
    {0x07, 0x62},   // HD format
    {0x2A, 0x81},   // filter control
    {0x3A, 0x20},   // Insert Channel ID in SAV/EAV code
    {0x3F, 0x10},   // channel ID
    {0x4C, 0x37},   // equalizer
    {0x4F, 0x00},   // sync control
    {0x50, 0x00},   // D1 resolution
    {0x56, 0x00},   // 27M mode and BT656 mode
    {0x5F, 0x00},   // blank level
    {0x63, 0x75},   // filter control
    {0x59, 0x00},   // extended register access
    {0x5A, 0x00},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x59, 0x33},   // extended register access
    {0x5A, 0x02},   // data for extended register
    {0x58, 0x01},   // enable extended register write
    {0x5B, 0x00},   // H-scaling control
    {0x5E, 0x01},   // enable H-scaling control
    {0x6A, 0x00},   // H-scaling control
    {0x28, 0x92},   // cropping
    {0x20, 0x24},   // USE_CVBS_ONE_FIELD
    {0x23, 0x17},
    {0x24, 0x37},
    {0x25, 0x17},
    {0x26, 0x00},
    {0x42, 0x00},
    {0x03, 0x80},   // saturation
    {0x04, 0x80},   // hue
    {0x05, 0x03},   // sharpness
    {0x57, 0x20},   // black/white stretch
    {0x68, 0x32},   // coring
    {0x37, 0x33},
    {0x61, 0x6C},
};

// =============================================================================
//                Private Function Definition
// =============================================================================
void
RN685x_HDinitial (
    REGPAIR         * table,
    unsigned int    count,
    unsigned char   ch)
{
    int i = 0;

    // RN68_WriteI2c_Byte(0x81, 0x0F); // turn on video decoder
    RN68_WriteI2c_Byte(0xA3, 0x04);
    RN68_WriteI2c_Byte(0xDF, 0xF0); // enable HD format
    RN68_WriteI2c_Byte(0xF0, 0xC0);
    RN68_WriteI2c_Byte(0x88, 0x40); // disable SCLK0B out
    RN68_WriteI2c_Byte(0xF6, 0x40); // disable SCLK3A out
    RN68_WriteI2c_Byte(0xD3, 0x50);
    RN68_WriteI2c_Byte(0xFF, ch);   // switch to ch Register Set Select

    for (i = 0; i < count; i++)
    {
        // (void)printf("(a = %x d = %x)\n",table[i].addr,table[i].value);
        RN68_WriteI2c_Byte((table[i].addr & 0xff), table[i].value);
    }
}

void
RN685x_D1initial (
    REGPAIR         * table,
    unsigned int    count,
    unsigned char   ch)
{
    int i = 0;

    RN68_WriteI2c_Byte(0x81, 0x0F); // turn on video decoder
    RN68_WriteI2c_Byte(0xA3, 0x00);
    RN68_WriteI2c_Byte(0xDF, 0xFF); // enable CVBS format
    RN68_WriteI2c_Byte(0x88, 0x00); // disable SCLK0B out
    RN68_WriteI2c_Byte(0xF6, 0x00); // disable SCLK3A out
    RN68_WriteI2c_Byte(0xD3, 0x50);
    RN68_WriteI2c_Byte(0xFF, ch);   // switch to ch Register Set Select

    for (i = 0; i < count; i++)
    {
        // (void)printf("(a = %x d = %x)\n",table[i].addr,table[i].value);
        RN68_WriteI2c_Byte((table[i].addr & 0xff), table[i].value);
    }
}

void
RN685x_Enable (
    unsigned char   ch,
    bool            HD)
{
    RN68_WriteI2c_Byte(0x8E, ch << 2);  // ch0-0x00, ch1-0x04, ch2-0x08, ch3-0x0C
    if (HD)
    {
        RN68_WriteI2c_Byte(0x8F, 0x80); // HD mode for VP
        RN68_WriteI2c_Byte(0x8D, 0x31); // enable VP out
        RN68_WriteI2c_Byte(0x89, 0x0A); // select 144MHz for SCLK
        RN68_WriteI2c_Byte(0x88, 0x41); // enable SCLK out
    }
    else
    {
        RN68_WriteI2c_Byte(0x8F, 0x00); // D1 mode for VP
        RN68_WriteI2c_Byte(0x8D, 0x31); // enable VP out
        RN68_WriteI2c_Byte(0x89, 0x00); // select 27MHz for SCLK
        RN68_WriteI2c_Byte(0x88, 0x41); // enable SCLK out
    }
}

void
RN685xM_Pre_initial (
    void)
{
    uint8_t rom_byte5   = 0;
    uint8_t rom_byte6   = 0;

    RN68_WriteI2c_Byte(0xE1, 0x80);
    RN68_WriteI2c_Byte(0xFA, 0x81);
    (void)RN68_ReadI2c_Byte (0xFB);
    (void)RN68_ReadI2c_Byte (0xFB);
    (void)RN68_ReadI2c_Byte (0xFB);
    (void)RN68_ReadI2c_Byte (0xFB);
    rom_byte5   = RN68_ReadI2c_Byte (0xFB);
    rom_byte6   = RN68_ReadI2c_Byte (0xFB);

    // config. decoder accroding to rom_byte5 and rom_byte6
    if ((rom_byte6 == 0x00) && (rom_byte5 == 0x00))
    {
        RN68_WriteI2c_Byte(0xEF, 0xAA);
        RN68_WriteI2c_Byte(0xE7, 0xFF);
        RN68_WriteI2c_Byte(0xFF, 0x09);
        RN68_WriteI2c_Byte(0x03, 0x0C);
        RN68_WriteI2c_Byte(0xFF, 0x0B);
        RN68_WriteI2c_Byte(0x03, 0x0C);
    }
    else if (((rom_byte6 == 0x34) && (rom_byte5 == 0xA9)) ||
         ((rom_byte6 == 0x2C) && (rom_byte5 == 0xA8)))
    {
        RN68_WriteI2c_Byte(0xEF, 0xAA);
        RN68_WriteI2c_Byte(0xE7, 0xFF);
        RN68_WriteI2c_Byte(0xFC, 0x60);
        RN68_WriteI2c_Byte(0xFF, 0x09);
        RN68_WriteI2c_Byte(0x03, 0x18);
        RN68_WriteI2c_Byte(0xFF, 0x0B);
        RN68_WriteI2c_Byte(0x03, 0x18);
    }
    else
    {
        RN68_WriteI2c_Byte(0xEF, 0xAA);
        RN68_WriteI2c_Byte(0xFC, 0x60);
        RN68_WriteI2c_Byte(0xFF, 0x09);
        RN68_WriteI2c_Byte(0x03, 0x18);
        RN68_WriteI2c_Byte(0xFF, 0x0B);
        RN68_WriteI2c_Byte(0x03, 0x18);
    }

    switch (CH_SET[current_ch])
    {
        case RN68_HD720P_25FPS:
            (void)printf("RN6854 CH(%d) = HD720P_25FPS \n", current_ch);
            RN685x_HDinitial(RN685x_720P25fps, sizeof(RN685x_720P25fps) / sizeof(REGPAIR), current_ch);
            RN685x_Enable(current_ch, true);
            break;

        case RN68_HD720P_30FPS:
            (void)printf("RN6854 CH(%d) = HD720P_30FPS \n", current_ch);
            RN685x_HDinitial(RN685x_720P30fps, sizeof(RN685x_720P30fps) / sizeof(REGPAIR), current_ch);
            RN685x_Enable(current_ch, true);
            break;

        case RN68_FHD1080P_25FPS:
            (void)printf("RN6854 CH(%d) =  FHD1080P_25FPS \n", current_ch);
            RN685x_HDinitial(RN685x_1080P25fps, sizeof(RN685x_1080P25fps) / sizeof(REGPAIR), current_ch);
            RN685x_Enable(current_ch, true);
            break;

        case RN68_FHD1080P_30FPS:
            (void)printf("RN6854 CH(%d) =  FHD1080P_30FPS \n", current_ch);
            RN685x_HDinitial(RN685x_1080P30fps, sizeof(RN685x_1080P30fps) / sizeof(REGPAIR), current_ch);
            RN685x_Enable(current_ch, true);
            break;

        case RN68_D1NTSC_60FPS:
            (void)printf("RN6854 CH(%d) =  NTSC_60I \n", current_ch);
            RN685x_D1initial(RN685x_NTSC, sizeof(RN685x_NTSC) / sizeof(REGPAIR), current_ch);
            RN685x_Enable(current_ch, false);
            break;

        case RN68_D1PAL_50FPS:
            (void)printf("RN6854 CH(%d) =  PAL_50I \n", current_ch);
            RN685x_D1initial(RN685x_PAL, sizeof(RN685x_PAL) / sizeof(REGPAIR), current_ch);
            RN685x_Enable(current_ch, false);
            break;

        default:
            (void)printf("RN6854 TIMING Error \n");
            break;
    }
}

RN68_TIMING_ID
RN685x_FormatDectect (
    uint8_t reg00)
{
    if ((reg00 & 0x20) == 0x20)
    {
        if ((reg00 & 0x01) == 0x01)
        {
            return RN68_HD720P_30FPS;
        }
        else
        {
            return RN68_HD720P_25FPS;
        }
    }
    else if ((reg00 & 0x40) == 0x40)
    {
        if ((reg00 & 0x01) == 0x01)
        {
            return RN68_FHD1080P_30FPS;
        }
        else
        {
            return RN68_FHD1080P_25FPS;
        }
    }
    else
    {
        if ((reg00 & 0x01) == 0x01)
        {
            return RN68_D1NTSC_60FPS;
        }
        else if ((reg00 & 0x01) == 0x00)
        {
            return RN68_D1PAL_50FPS;
        }
    }

    return RN68_UNKNOWN;
}

// =============================================================================
//                RN6854 Public Function Definition
// =============================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////
// X10LightDriver_t1.c
void
RN6854Initialize (
    uint16_t Mode)
{
    unsigned char i = 0;
    (void)printf("RN6854M Initialize \n");
    RN68_I2c_init();
    RN685xM_Pre_initial();
}

void
RN6854Terminate (
    void)
{
    /* Please implement terminate code here */
    video_state         = RN_VIDEO_UNKNOWN;
    pre_dectect_format  = RN68_UNKNOWN;
    format_count        = 0;
    current_ch          = 0;
}

void
RN6854OutputPinTriState (
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
RN6854IsSignalStable (
    uint16_t Mode)
{
    uint8_t         status_reg      = 0x0;
    uint8_t         lock_res        = false;
    RN68_TIMING_ID  dectect_format  = RN68_UNKNOWN;

    switch (video_state)
    {
        case RN_VIDEO_UNKNOWN:
            status_reg = RN68_ReadI2c_Byte(0x0);
            // (void)printf("[UNKNOWN]reg(0x0) = %x\n", status_reg);
            if ((status_reg & 0x10) == 0x00)// have video
            {
                video_state = RN_VIDEO_DECTECT;
                usleep(1000 * 500);
            }

            break;

        case RN_VIDEO_DECTECT:
            status_reg      = RN68_ReadI2c_Byte(0x0);
            dectect_format  = RN685x_FormatDectect(status_reg);
            // (void)printf("[DECTECT]format = %d format_count = %d\n", dectect_format, format_count);
            if (pre_dectect_format == dectect_format)
            {
                format_count++;
            }
            else
            {
                pre_dectect_format  = dectect_format;
                format_count        = 0;
            }

            if ((dectect_format != CH_SET[current_ch]) &&
                (format_count > FORMAT_CHECK_TIMES))
            {
                CH_SET[current_ch] = dectect_format;
                switch (CH_SET[current_ch])
                {
                    case RN68_HD720P_25FPS:
                        (void)printf("RN6854 CH(%d) = HD720P_25FPS \n", current_ch);
                        RN685x_HDinitial(RN685x_720P25fps, sizeof(RN685x_720P25fps) / sizeof(REGPAIR), current_ch);
                        RN685x_Enable(current_ch, true);
                        break;

                    case RN68_HD720P_30FPS:
                        (void)printf("RN6854 CH(%d) = HD720P_30FPS \n", current_ch);
                        RN685x_HDinitial(RN685x_720P30fps, sizeof(RN685x_720P30fps) / sizeof(REGPAIR), current_ch);
                        RN685x_Enable(current_ch, true);
                        break;

                    case RN68_FHD1080P_25FPS:
                        (void)printf("RN6854 CH(%d) =  FHD1080P_25FPS \n", current_ch);
                        RN685x_HDinitial(RN685x_1080P25fps, sizeof(RN685x_1080P25fps) / sizeof(REGPAIR), current_ch);
                        RN685x_Enable(current_ch, true);
                        break;

                    case RN68_FHD1080P_30FPS:
                        (void)printf("RN6854 CH(%d) =  FHD1080P_30FPS \n", current_ch);
                        RN685x_HDinitial(RN685x_1080P30fps, sizeof(RN685x_1080P30fps) / sizeof(REGPAIR), current_ch);
                        RN685x_Enable(current_ch, true);
                        break;

                    case RN68_D1NTSC_60FPS:
                        (void)printf("RN6854 CH(%d) =  NTSC_60I \n", current_ch);
                        RN685x_D1initial(RN685x_NTSC, sizeof(RN685x_NTSC) / sizeof(REGPAIR), current_ch);
                        RN685x_Enable(current_ch, false);
                        break;

                    case RN68_D1PAL_50FPS:
                        (void)printf("RN6854 CH(%d) =  PAL_50I \n", current_ch);
                        RN685x_D1initial(RN685x_PAL, sizeof(RN685x_PAL) / sizeof(REGPAIR), current_ch);
                        RN685x_Enable(current_ch, false);
                        break;

                    default:
                        (void)printf("RN6854 TIMING Error \n");
                        break;
                }
                video_state = RN_VIDEO_LOCKED;
            }
            else if ((dectect_format == CH_SET[current_ch]) && (format_count > FORMAT_CHECK_TIMES))
            {
                video_state = RN_VIDEO_LOCKED;
            }

            break;

        case RN_VIDEO_LOCKED:
            status_reg = RN68_ReadI2c_Byte(0x0);
            // (void)printf("[LOCKED] reg(0x0) = %x\n",status_reg);
            if ((status_reg & 0x10) == 0x00)
            {
                lock_res = true;
            }
            else
            {
                video_state         = RN_VIDEO_UNKNOWN;
                pre_dectect_format  = RN68_UNKNOWN;
                lock_res            = false;
            }
            break;

        default:

            break;
    }

    return lock_res;
}

uint16_t
RN6854GetProperty (
    MODULE_GETPROPERTY property)
{
    /* Please implement get information from device code here */
    switch (property)
    {
    // case GetTopFieldPolarity:
        case GetHeight:

            if (CH_SET[current_ch] <= 1)
            {
                return 720;
            }
            else if (CH_SET[current_ch] <= 3)
            {
                return 1080;
            }
            else if (CH_SET[current_ch] == RN68_D1NTSC_60FPS)
            {
                return 487;
            }
            else
            {
                return 576;
            }

            break;

        case GetWidth:

            if (CH_SET[current_ch] <= 1)
            {
                return 1280;
            }
            else if (CH_SET[current_ch] <= 3)
            {
                return 1920;
            }
            else
            {
                return 720;
            }

            break;

            // frame rate
        case Rate:

            if ((CH_SET[current_ch] == RN68_HD720P_25FPS) || (CH_SET[current_ch] == RN68_FHD1080P_25FPS))
            {
                return 2500;
            }
            else if ((CH_SET[current_ch] == RN68_HD720P_30FPS) || (CH_SET[current_ch] == RN68_FHD1080P_30FPS))
            {
                return 3000;
            }
            else if (CH_SET[current_ch] == RN68_D1NTSC_60FPS)
            {
                return 5994;
            }
            else
            {
                return 5000;
            }

            break;

        case GetModuleIsInterlace:
            if ((CH_SET[current_ch] == RN68_D1NTSC_60FPS) || (CH_SET[current_ch] == RN68_D1PAL_50FPS))
            {
                return 1;
            }
            else
            {
                return 0;
            }
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
RN6854GetStatus (
    MODULE_GETSTATUS Status)
{
    /* Please implement get status from device code here */
    switch (Status)
    {
        default:
            return 0;
            break;
    }
}

void
RN6854SetProperty (
    MODULE_SETPROPERTY  Property,
    uint16_t            Value)
{
    /* Please implement set property to device code here */
    switch (Property)
    {
        case VideoInCH:
            if ((Value & 0xff) != current_ch)
            {
                current_ch  = (unsigned char)Value;
                video_state = RN_VIDEO_UNKNOWN;
                (void)printf("[RN6854SetProperty]CH CHANGE = %d\n", current_ch);
                RN685xM_Pre_initial();
            }
            break;

        default:
            break;
    }
}

void
RN6854PowerDown (
    uint8_t enable)
{
#ifdef CFG_SENSOR_POWERDOWNPIN_ENABLE
    if (enable)
    {
        // Power off
        (void)printf("RN6854 power off \n");
        ithGpioClear(CFG_SN1_GPIO_PWN);
        ithGpioSetOut(CFG_SN1_GPIO_PWN);
        ithGpioSetMode(CFG_SN1_GPIO_PWN, ITH_GPIO_MODE0);
    }
    else
    {
        // Power ON
        (void)printf("RN6854 power on \n");
        ithGpioSetMode(CFG_SN1_GPIO_PWN, ITH_GPIO_MODE0);
        ithGpioSetOut(CFG_SN1_GPIO_PWN);
        ithGpioSet(CFG_SN1_GPIO_PWN);
        usleep(10 * 1000);
    #ifdef CFG_SENSOR_RESETPIN_ENABLE
        ithGpioClear(CFG_SN1_GPIO_RST);
        ithGpioSetOut(CFG_SN1_GPIO_RST);
        ithGpioSetMode(CFG_SN1_GPIO_RST, ITH_GPIO_MODE0);
        usleep(30 * 1000);
        ithGpioSet(CFG_SN1_GPIO_RST);
        usleep(60 * 1000);
    #endif
    }
#endif
}

// =============================================================================
//                RN6854 Public Function END
// =============================================================================
void
RN6854SensorDriver_Destory (
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
    RN6854Initialize,
    RN6854Terminate,
    RN6854OutputPinTriState,
    RN6854IsSignalStable,
    RN6854GetProperty,
    RN6854GetStatus,
    RN6854SetProperty,
    RN6854PowerDown,
    RN6854SensorDriver_Destory
};

SensorDriver
RN6854SensorDriver_Create ()
{
    RN6854SensorDriver self = calloc(1, sizeof(RN6854SensorDriverStruct));
    if (self != NULL)
    {
        self->base.vtable   = &interface;
        self->base.type     = "RN6854";
    }
    return (SensorDriver)self;
}

// end of X10LightDriver_t1.c
