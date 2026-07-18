// =============================================================================
// =============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "iic/mmp_iic.h"
#include "TP2860.h"

// =============================================================================
//                Constant Definition
// =============================================================================
/* Note:
TP2860 i2c Thd minimum is 250 ns.
I2c driver need set SDA delay time.
file path: itp_i2c.c
function:mmpIicInitialize(.....,uint32_t delay)
*/
static uint8_t          IICADDR                     = (0x88 >> 1);          /* please assign IIC ADDRESS */
//static uint8_t          IICADDR                     = (0x8A >> 1);          /* please assign IIC ADDRESS */

#ifdef CFG_SENSOR_ENABLE
static uint8_t          IICPORT                     = CFG_SENSOR_IIC_PORT;  /* please assign IIC PORT      */
#else
static uint8_t          IICPORT                     = IIC_PORT_2;
#endif
static int              gMasterDev                  = 0;
static uint16_t         gCurVin                     = 0xff;

const TP2860RESOLUTIONINFO    tp2860_info_table[TP2860_MAX_ID]   = {
    {1280, 720,  2500, 0},
    {1280, 720,  3000, 0},
    {1920, 1080, 2500, 0},
    {1920, 1080, 3000, 0},
    {720,  480,  5994, 1},
    {720,  576,  5000, 1},    
};

// =============================================================================
//                Macro Definition
// =============================================================================

static uint16_t CurrentResolution = TP2860_MAX_ID;

// =============================================================================
//                Structure Definition
// =============================================================================

typedef struct TP2860SensorDriverStruct
{
    SensorDriverStruct base;
} TP2860SensorDriverStruct;

// =============================================================================
//                IIC API FUNCTION START
// =============================================================================
void
TP2860_I2c_Init ()
{
    char portname[16] = {0};

    sprintf(portname, ":i2c%d", IICPORT);
    gMasterDev = open(portname, 0, 0);
}

uint8_t
TP2860_ReadI2c_Byte (
    uint8_t RegAddr)
{
    uint8_t     dbuf[2] = {0};
    int         result  = 0;
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt;

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
        (void)printf("ReadI2c_Byte read address 0x%02x error!\n", RegAddr);
    }

    return dbuf[1];
}

uint32_t
TP2860_WriteI2c_Byte (
    uint8_t RegAddr,
    uint8_t data)
{
    uint8_t     dbuf[2] = {0};
    int         result  = 0;
    uint8_t     * pdbuf = dbuf;
    ITPI2cInfo  evt;

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
TP2860_WriteI2c_ByteMask (
    uint8_t RegAddr,
    uint8_t data,
    uint8_t mask)
{
    uint8_t     value;
    uint32_t    flag;

    value   = TP2860_ReadI2c_Byte(RegAddr);
    value   = ((value & ~mask) | (data & mask));
    flag    = TP2860_WriteI2c_Byte(RegAddr, value);

    return flag;
}

// =============================================================================
//                IIC API FUNCTION END
// =============================================================================

// =============================================================================
//                Global Data Definition
// =============================================================================
#define CVBS_960H 0 //1->960H 0->720H

enum{
	VIN1 = 0,
    VIN2 = 1,
    VIN3 = 2,
    VIN4 = 3,
};

/* TVI , AHD */
enum{
    STD_TVI = 0,
    STD_HDA,
};

enum{
    PAL = 0,
    NTSC,
    HD25,
    HD30,
    HD275,	//720p27.5
    FHD25,
    FHD30,
    FHD275,	//1080p27.5
    FHD28,
    HD50,
    HD60,
};
// =============================================================================
//                Private Function Definition
// =============================================================================
void TP9951_dvp_out(unsigned char fmt, unsigned char std)
{
    uint8_t tmp = 0U;

	/* mipi page setting */
	TP2860_WriteI2c_Byte(0x40, 0x08); //select mipi page
	
	if(FHD30 == fmt || FHD25 == fmt || FHD275 == fmt || FHD28 == fmt || HD50 == fmt  || HD60 == fmt)
	{
        TP2860_WriteI2c_Byte(0x12, 0x54);			
        TP2860_WriteI2c_Byte(0x13, 0xef);	
        TP2860_WriteI2c_Byte(0x14, 0x41);
        TP2860_WriteI2c_Byte(0x15, 0x02);		
        TP2860_WriteI2c_Byte(0x40, 0x00);				
	}
	else if(HD30 == fmt || HD25 == fmt || HD275 == fmt)
	{
        TP2860_WriteI2c_Byte(0x12, 0x54);			
        TP2860_WriteI2c_Byte(0x13, 0xef);	
        TP2860_WriteI2c_Byte(0x14, 0x41);
        TP2860_WriteI2c_Byte(0x15, 0x12);		
        TP2860_WriteI2c_Byte(0x40, 0x00);
	}
	else if(NTSC == fmt || PAL == fmt)
	{
		TP2860_WriteI2c_Byte(0x12, 0x54);
		TP2860_WriteI2c_Byte(0x13, 0xef);		
		TP2860_WriteI2c_Byte(0x14, 0x41);
		TP2860_WriteI2c_Byte(0x15, 0x12);		
		TP2860_WriteI2c_Byte(0x40, 0x00);
	}
	
	TP2860_WriteI2c_Byte(0x40, 0x00); //back to decoder page	
	tmp = TP2860_ReadI2c_Byte(0x06); //PLL reset
	TP2860_WriteI2c_Byte(0x06, 0x80|tmp);

	TP2860_WriteI2c_Byte(0x40, 0x08); //back to mipi page	

	tmp = TP2860_ReadI2c_Byte(0x14); //PLL reset
	TP2860_WriteI2c_Byte(0x14, 0x80|tmp);
	TP2860_WriteI2c_Byte(0x14, tmp);
	
	TP2860_WriteI2c_Byte(0x40, 0x00); //back to decoder page
}
/*
ch: video channel
fmt: PAL/NTSC/HD25/HD30
std: STD_TVI/STD_HDA
sample: TP9951_sensor_init(VIN1,HD30,STD_TVI); //video is TVI 720p30 from Vin1
*/
void TP9951_sensor_init(unsigned char ch,unsigned char fmt,unsigned char std)
{
	
	TP2860_WriteI2c_Byte(0x40, 0x00); //select decoder page
	TP2860_WriteI2c_Byte(0x06, 0x12); //default value		
	TP2860_WriteI2c_Byte(0x42, 0x00); //common setting for all format	
	TP2860_WriteI2c_Byte(0x4c, 0x43); //common setting for all format		
	TP2860_WriteI2c_Byte(0x4e, 0x1d); //common setting for dvp output
	TP2860_WriteI2c_Byte(0x54, 0x04); //common setting for dvp output
	
	TP2860_WriteI2c_Byte(0xf6, 0x00); //common setting for all format	
	TP2860_WriteI2c_Byte(0xf7, 0x44); //common setting for dvp output
	if(PAL == fmt || NTSC == fmt)
    {
		TP2860_WriteI2c_Byte(0xfa, 0x04); //common setting for dvp output	CVBS clock/2
    }		
	else
    {
		TP2860_WriteI2c_Byte(0xfa, 0x00); //common setting for dvp output	
    }
	TP2860_WriteI2c_Byte(0x1b, 0x01); //common setting for dvp output		
	TP2860_WriteI2c_Byte(0x41, ch);   //video MUX select
		
	TP2860_WriteI2c_Byte(0x40, 0x08); //common setting for all format	
	TP2860_WriteI2c_Byte(0x13, 0xef); //common setting for dvp output
	TP2860_WriteI2c_Byte(0x14, 0x41); //common setting for dvp output	
	TP2860_WriteI2c_Byte(0x15, 0x02); //common setting for dvp output			
	
	TP2860_WriteI2c_Byte(0x40, 0x00); //select decoder page

	TP9951_dvp_out(fmt, std);
	
	if(PAL == fmt)
	{
#if CVBS_960H	
		TP2860_WriteI2c_Byte(0x02, 0xcf);
		TP2860_WriteI2c_Byte(0x0c, 0x13); 
		TP2860_WriteI2c_Byte(0x0d, 0x51);  

		TP2860_WriteI2c_Byte(0x15, 0x13);
		TP2860_WriteI2c_Byte(0x16, 0x76); 
		TP2860_WriteI2c_Byte(0x17, 0x80); 
		TP2860_WriteI2c_Byte(0x18, 0x17);
		TP2860_WriteI2c_Byte(0x19, 0x20);
		TP2860_WriteI2c_Byte(0x1a, 0x17);				
		TP2860_WriteI2c_Byte(0x1c, 0x09);
		TP2860_WriteI2c_Byte(0x1d, 0x48);
	
		TP2860_WriteI2c_Byte(0x20, 0x48);  
		TP2860_WriteI2c_Byte(0x21, 0x84); 
		TP2860_WriteI2c_Byte(0x22, 0x37);
		TP2860_WriteI2c_Byte(0x23, 0x3f);

		TP2860_WriteI2c_Byte(0x2b, 0x70);  
		TP2860_WriteI2c_Byte(0x2c, 0x2a); 
		TP2860_WriteI2c_Byte(0x2d, 0x64);
		TP2860_WriteI2c_Byte(0x2e, 0x56);

		TP2860_WriteI2c_Byte(0x30, 0x7a);  
		TP2860_WriteI2c_Byte(0x31, 0x4a); 
		TP2860_WriteI2c_Byte(0x32, 0x4d);
		TP2860_WriteI2c_Byte(0x33, 0xf0);	
		
		TP2860_WriteI2c_Byte(0x35, 0x65); 
		TP2860_WriteI2c_Byte(0x38, 0x00);				
		TP2860_WriteI2c_Byte(0x39, 0x04); 
			
#else //PAL 720H

		TP2860_WriteI2c_Byte(0x02, 0xcf);
		TP2860_WriteI2c_Byte(0x06, 0x32);				
		TP2860_WriteI2c_Byte(0x0c, 0x13); 
		TP2860_WriteI2c_Byte(0x0d, 0x51);  

		TP2860_WriteI2c_Byte(0x15, 0x03);
		TP2860_WriteI2c_Byte(0x16, 0xf0); 
		TP2860_WriteI2c_Byte(0x17, 0xa0); 
		TP2860_WriteI2c_Byte(0x18, 0x17);
		TP2860_WriteI2c_Byte(0x19, 0x20);
		TP2860_WriteI2c_Byte(0x1a, 0x15);				
		TP2860_WriteI2c_Byte(0x1c, 0x06);
		TP2860_WriteI2c_Byte(0x1d, 0xc0);
	
		TP2860_WriteI2c_Byte(0x20, 0x48);  
		TP2860_WriteI2c_Byte(0x21, 0x84); 
		TP2860_WriteI2c_Byte(0x22, 0x37);
		TP2860_WriteI2c_Byte(0x23, 0x3f);

		TP2860_WriteI2c_Byte(0x2b, 0x70);  
		TP2860_WriteI2c_Byte(0x2c, 0x2a); 
		TP2860_WriteI2c_Byte(0x2d, 0x4b);
		TP2860_WriteI2c_Byte(0x2e, 0x56);

		TP2860_WriteI2c_Byte(0x30, 0x7a);  
		TP2860_WriteI2c_Byte(0x31, 0x4a); 
		TP2860_WriteI2c_Byte(0x32, 0x4d);
		TP2860_WriteI2c_Byte(0x33, 0xfb);	
		
		TP2860_WriteI2c_Byte(0x35, 0x65); 
		TP2860_WriteI2c_Byte(0x38, 0x00);				
		TP2860_WriteI2c_Byte(0x39, 0x04); 	
#endif
        TP2860_WriteI2c_Byte(0xf3, 0x00);/* CLK delay , (0.2ns * 0) */
	}
	else if(NTSC == fmt)
	{
#if CVBS_960H		
		TP2860_WriteI2c_Byte(0x02, 0xcf);
		TP2860_WriteI2c_Byte(0x0c, 0x13); 
		TP2860_WriteI2c_Byte(0x0d, 0x50);  

		TP2860_WriteI2c_Byte(0x15, 0x13);
		TP2860_WriteI2c_Byte(0x16, 0x60); 
		TP2860_WriteI2c_Byte(0x17, 0x80); 
		TP2860_WriteI2c_Byte(0x18, 0x12);
		TP2860_WriteI2c_Byte(0x19, 0xf0);
		TP2860_WriteI2c_Byte(0x1a, 0x07);				
		TP2860_WriteI2c_Byte(0x1c, 0x09);
		TP2860_WriteI2c_Byte(0x1d, 0x38);
	
		TP2860_WriteI2c_Byte(0x20, 0x40);  
		TP2860_WriteI2c_Byte(0x21, 0x84); 
		TP2860_WriteI2c_Byte(0x22, 0x36);
		TP2860_WriteI2c_Byte(0x23, 0x3c);

		TP2860_WriteI2c_Byte(0x2b, 0x70);  
		TP2860_WriteI2c_Byte(0x2c, 0x2a); 
		TP2860_WriteI2c_Byte(0x2d, 0x68);
		TP2860_WriteI2c_Byte(0x2e, 0x57);

		TP2860_WriteI2c_Byte(0x30, 0x62);  
		TP2860_WriteI2c_Byte(0x31, 0xbb); 
		TP2860_WriteI2c_Byte(0x32, 0x96);
		TP2860_WriteI2c_Byte(0x33, 0xc0);
		
		TP2860_WriteI2c_Byte(0x35, 0x65); 
		TP2860_WriteI2c_Byte(0x38, 0x00);			
		TP2860_WriteI2c_Byte(0x39, 0x04); 
		
#else	//NTSC 720H
		
		TP2860_WriteI2c_Byte(0x02, 0xcf);
		TP2860_WriteI2c_Byte(0x0c, 0x13); 
		TP2860_WriteI2c_Byte(0x0d, 0x50);  

		TP2860_WriteI2c_Byte(0x15, 0x03);
		TP2860_WriteI2c_Byte(0x16, 0xd6); 
		TP2860_WriteI2c_Byte(0x17, 0xa0); 
		TP2860_WriteI2c_Byte(0x18, 0x12);
		TP2860_WriteI2c_Byte(0x19, 0xf0);
		TP2860_WriteI2c_Byte(0x1a, 0x05);				
		TP2860_WriteI2c_Byte(0x1c, 0x06);
		TP2860_WriteI2c_Byte(0x1d, 0xb4);
	
		TP2860_WriteI2c_Byte(0x20, 0x40);  
		TP2860_WriteI2c_Byte(0x21, 0x84); 
		TP2860_WriteI2c_Byte(0x22, 0x36);
		TP2860_WriteI2c_Byte(0x23, 0x3c);

		TP2860_WriteI2c_Byte(0x2b, 0x70);  
		TP2860_WriteI2c_Byte(0x2c, 0x2a); 
		TP2860_WriteI2c_Byte(0x2d, 0x4b);
		TP2860_WriteI2c_Byte(0x2e, 0x57);

		TP2860_WriteI2c_Byte(0x30, 0x62);  
		TP2860_WriteI2c_Byte(0x31, 0xbb); 
		TP2860_WriteI2c_Byte(0x32, 0x96);
		TP2860_WriteI2c_Byte(0x33, 0xcb);
		
		TP2860_WriteI2c_Byte(0x35, 0x65); 
		TP2860_WriteI2c_Byte(0x38, 0x00);			
		TP2860_WriteI2c_Byte(0x39, 0x04); 			
#endif
        TP2860_WriteI2c_Byte(0xf3, 0x00);/* CLK delay , (0.2ns * 0) */
	}
	else if(HD25 == fmt)
	{
		TP2860_WriteI2c_Byte(0x02, 0xca);
		TP2860_WriteI2c_Byte(0x07, 0xc0); 
		TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
		TP2860_WriteI2c_Byte(0x0c, 0x13); 
		TP2860_WriteI2c_Byte(0x0d, 0x50);  

		TP2860_WriteI2c_Byte(0x15, 0x13);
		TP2860_WriteI2c_Byte(0x16, 0x15); 
		TP2860_WriteI2c_Byte(0x17, 0x00); 
		TP2860_WriteI2c_Byte(0x18, 0x19);
		TP2860_WriteI2c_Byte(0x19, 0xd0);
		TP2860_WriteI2c_Byte(0x1a, 0x25);			
		TP2860_WriteI2c_Byte(0x1c, 0x07);  //1280*720, 25fps
		TP2860_WriteI2c_Byte(0x1d, 0xbc);  //1280*720, 25fps

		TP2860_WriteI2c_Byte(0x20, 0x30);  
		TP2860_WriteI2c_Byte(0x21, 0x84); 
		TP2860_WriteI2c_Byte(0x22, 0x36);
		TP2860_WriteI2c_Byte(0x23, 0x3c);

		TP2860_WriteI2c_Byte(0x2b, 0x60);  
		TP2860_WriteI2c_Byte(0x2c, 0x2a); 
		TP2860_WriteI2c_Byte(0x2d, 0x30);
		TP2860_WriteI2c_Byte(0x2e, 0x70);

		TP2860_WriteI2c_Byte(0x30, 0x48);  
		TP2860_WriteI2c_Byte(0x31, 0xbb); 
		TP2860_WriteI2c_Byte(0x32, 0x2e);
		TP2860_WriteI2c_Byte(0x33, 0x90);
		
		TP2860_WriteI2c_Byte(0x35, 0x25); 
		TP2860_WriteI2c_Byte(0x38, 0x00);	
		TP2860_WriteI2c_Byte(0x39, 0x18); 

		if(STD_HDA == std)  //AHD720p25 extra
		{
            TP2860_WriteI2c_Byte(0x02, 0xce);

            TP2860_WriteI2c_Byte(0x0d, 0x71);
            
            TP2860_WriteI2c_Byte(0x18, 0x1b);
            
            TP2860_WriteI2c_Byte(0x20, 0x40);
            TP2860_WriteI2c_Byte(0x21, 0x46);

            TP2860_WriteI2c_Byte(0x25, 0xfe);
            TP2860_WriteI2c_Byte(0x26, 0x01);

            TP2860_WriteI2c_Byte(0x2c, 0x3a);
            TP2860_WriteI2c_Byte(0x2d, 0x5a);
            TP2860_WriteI2c_Byte(0x2e, 0x40);

            TP2860_WriteI2c_Byte(0x30, 0x9e);
            TP2860_WriteI2c_Byte(0x31, 0x20);
            TP2860_WriteI2c_Byte(0x32, 0x10);
            TP2860_WriteI2c_Byte(0x33, 0x90);
		}
        TP2860_WriteI2c_Byte(0xf3, 0x00);/* CLK delay , (0.2ns * 0) */	
	}
	else if(HD30 == fmt)
	{
		TP2860_WriteI2c_Byte(0x02, 0xca);
		TP2860_WriteI2c_Byte(0x07, 0xc0); 
		TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
		TP2860_WriteI2c_Byte(0x0c, 0x13); 
		TP2860_WriteI2c_Byte(0x0d, 0x50);  

		TP2860_WriteI2c_Byte(0x15, 0x13);
		TP2860_WriteI2c_Byte(0x16, 0x15); 
		TP2860_WriteI2c_Byte(0x17, 0x00); 
		TP2860_WriteI2c_Byte(0x18, 0x19);
		TP2860_WriteI2c_Byte(0x19, 0xd0);
		TP2860_WriteI2c_Byte(0x1a, 0x25);			
		TP2860_WriteI2c_Byte(0x1c, 0x06);  //1280*720, 30fps
		TP2860_WriteI2c_Byte(0x1d, 0x72);  //1280*720, 30fps

		TP2860_WriteI2c_Byte(0x20, 0x30);  
		TP2860_WriteI2c_Byte(0x21, 0x84); 
		TP2860_WriteI2c_Byte(0x22, 0x36);
		TP2860_WriteI2c_Byte(0x23, 0x3c);

		TP2860_WriteI2c_Byte(0x2b, 0x60);  
		TP2860_WriteI2c_Byte(0x2c, 0x2a); 
		TP2860_WriteI2c_Byte(0x2d, 0x30);
		TP2860_WriteI2c_Byte(0x2e, 0x70);

		TP2860_WriteI2c_Byte(0x30, 0x48);  
		TP2860_WriteI2c_Byte(0x31, 0xbb); 
		TP2860_WriteI2c_Byte(0x32, 0x2e);
		TP2860_WriteI2c_Byte(0x33, 0x90);
		
		TP2860_WriteI2c_Byte(0x35, 0x25); 
		TP2860_WriteI2c_Byte(0x38, 0x00);	
		TP2860_WriteI2c_Byte(0x39, 0x18); 

		if(STD_HDA == std) //AHD720p30 extra
		{
            TP2860_WriteI2c_Byte(0x02, 0xce);

            TP2860_WriteI2c_Byte(0x0d, 0x70);
            
            TP2860_WriteI2c_Byte(0x18, 0x1b);
            
            TP2860_WriteI2c_Byte(0x20, 0x40);
            TP2860_WriteI2c_Byte(0x21, 0x46);

            TP2860_WriteI2c_Byte(0x25, 0xfe);
            TP2860_WriteI2c_Byte(0x26, 0x01);

            TP2860_WriteI2c_Byte(0x2c, 0x3a);
            TP2860_WriteI2c_Byte(0x2d, 0x5a);
            TP2860_WriteI2c_Byte(0x2e, 0x40);

            TP2860_WriteI2c_Byte(0x30, 0x9d);
            TP2860_WriteI2c_Byte(0x31, 0xca);
            TP2860_WriteI2c_Byte(0x32, 0x01);
            TP2860_WriteI2c_Byte(0x33, 0xd0);
		}
        TP2860_WriteI2c_Byte(0xf3, 0x00);/* CLK delay , (0.2ns * 0) */
	}
	else if(HD275 == fmt)		//720P27.5
	{
		TP2860_WriteI2c_Byte(0x02, 0xca);
		TP2860_WriteI2c_Byte(0x07, 0xc0); 
		TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
		TP2860_WriteI2c_Byte(0x0c, 0x13); 
		TP2860_WriteI2c_Byte(0x0d, 0x50);  

		TP2860_WriteI2c_Byte(0x15, 0x13);
		TP2860_WriteI2c_Byte(0x16, 0x15); 
		TP2860_WriteI2c_Byte(0x17, 0x00); 
		TP2860_WriteI2c_Byte(0x18, 0x19);
		TP2860_WriteI2c_Byte(0x19, 0xd0);
		TP2860_WriteI2c_Byte(0x1a, 0x25);			
		TP2860_WriteI2c_Byte(0x1c, 0x07);  //1280*720, 27.5fps
		TP2860_WriteI2c_Byte(0x1d, 0x08);  //1280*720, 27.5fps

		TP2860_WriteI2c_Byte(0x20, 0x30);  
		TP2860_WriteI2c_Byte(0x21, 0x84); 
		TP2860_WriteI2c_Byte(0x22, 0x36);
		TP2860_WriteI2c_Byte(0x23, 0x3c);

		TP2860_WriteI2c_Byte(0x2b, 0x60);  
		TP2860_WriteI2c_Byte(0x2c, 0x2a); 
		TP2860_WriteI2c_Byte(0x2d, 0x30);
		TP2860_WriteI2c_Byte(0x2e, 0x70);

		TP2860_WriteI2c_Byte(0x30, 0x48);  
		TP2860_WriteI2c_Byte(0x31, 0xbb); 
		TP2860_WriteI2c_Byte(0x32, 0x2e);
		TP2860_WriteI2c_Byte(0x33, 0x90);
		
		TP2860_WriteI2c_Byte(0x35, 0x25); 
		TP2860_WriteI2c_Byte(0x38, 0x00);	
		TP2860_WriteI2c_Byte(0x39, 0x18); 

		if(STD_HDA == std) //AHD720p30 extra
		{

		}
        TP2860_WriteI2c_Byte(0xf3, 0x00);/* CLK delay , (0.2ns * 0) */	
	}	
	else if(FHD30 == fmt)
	{	
        TP2860_WriteI2c_Byte(0x02, 0xc8);
        TP2860_WriteI2c_Byte(0x07, 0xc0); 
        TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
        TP2860_WriteI2c_Byte(0x0c, 0x03); 
        TP2860_WriteI2c_Byte(0x0d, 0x50);  

        TP2860_WriteI2c_Byte(0x15, 0x03);
        TP2860_WriteI2c_Byte(0x16, 0xd2); 
        TP2860_WriteI2c_Byte(0x17, 0x80); 
        TP2860_WriteI2c_Byte(0x18, 0x29);
        TP2860_WriteI2c_Byte(0x19, 0x38);
        TP2860_WriteI2c_Byte(0x1a, 0x47);				
        TP2860_WriteI2c_Byte(0x1c, 0x08);  //1920*1080, 30fps
        TP2860_WriteI2c_Byte(0x1d, 0x98);  //

        TP2860_WriteI2c_Byte(0x20, 0x30);  
        TP2860_WriteI2c_Byte(0x21, 0x84); 
        TP2860_WriteI2c_Byte(0x22, 0x36);
        TP2860_WriteI2c_Byte(0x23, 0x3c);

        TP2860_WriteI2c_Byte(0x2b, 0x60);  
        TP2860_WriteI2c_Byte(0x2c, 0x2a); 
        TP2860_WriteI2c_Byte(0x2d, 0x30);
        TP2860_WriteI2c_Byte(0x2e, 0x70);

        TP2860_WriteI2c_Byte(0x30, 0x48);  
        TP2860_WriteI2c_Byte(0x31, 0xbb); 
        TP2860_WriteI2c_Byte(0x32, 0x2e);
        TP2860_WriteI2c_Byte(0x33, 0x90);
        
        TP2860_WriteI2c_Byte(0x35, 0x05);
        TP2860_WriteI2c_Byte(0x38, 0x00); 
        TP2860_WriteI2c_Byte(0x39, 0x1C); 	
    
        if(STD_HDA == std) //AHD1080p30 extra
        {
            TP2860_WriteI2c_Byte(0x02, 0xcc);
            
            TP2860_WriteI2c_Byte(0x0d, 0x72);
            
            TP2860_WriteI2c_Byte(0x15, 0x01);
            TP2860_WriteI2c_Byte(0x16, 0xf0);
            TP2860_WriteI2c_Byte(0x18, 0x2a);
                        
            TP2860_WriteI2c_Byte(0x20, 0x38);
            TP2860_WriteI2c_Byte(0x21, 0x46);
            
            TP2860_WriteI2c_Byte(0x25, 0xfe);
            TP2860_WriteI2c_Byte(0x26, 0x0d);
            
            TP2860_WriteI2c_Byte(0x2c, 0x3a);
            TP2860_WriteI2c_Byte(0x2d, 0x54);
            TP2860_WriteI2c_Byte(0x2e, 0x40);
            
            TP2860_WriteI2c_Byte(0x30, 0xa5);
            TP2860_WriteI2c_Byte(0x31, 0x95);
            TP2860_WriteI2c_Byte(0x32, 0xe0);
            TP2860_WriteI2c_Byte(0x33, 0x60);		  		
        }
        
        TP2860_WriteI2c_Byte(0xf3, 0x0);/* CLK delay , (0.2ns * 0) */
	}
	else if(FHD25 == fmt)
	{
        TP2860_WriteI2c_Byte(0x02, 0xc8);
        TP2860_WriteI2c_Byte(0x07, 0xc0); 
        TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
        TP2860_WriteI2c_Byte(0x0c, 0x03); 
        TP2860_WriteI2c_Byte(0x0d, 0x50);  
    
        TP2860_WriteI2c_Byte(0x15, 0x03);
        TP2860_WriteI2c_Byte(0x16, 0xd2); 
        TP2860_WriteI2c_Byte(0x17, 0x80); 
        TP2860_WriteI2c_Byte(0x18, 0x29);
        TP2860_WriteI2c_Byte(0x19, 0x38);
        TP2860_WriteI2c_Byte(0x1a, 0x47);				
    
        TP2860_WriteI2c_Byte(0x1c, 0x0a);  //1920*1080, 25fps
        TP2860_WriteI2c_Byte(0x1d, 0x50);  //
        
        TP2860_WriteI2c_Byte(0x20, 0x30);  
        TP2860_WriteI2c_Byte(0x21, 0x84); 
        TP2860_WriteI2c_Byte(0x22, 0x36);
        TP2860_WriteI2c_Byte(0x23, 0x3c);
    
        TP2860_WriteI2c_Byte(0x2b, 0x60);  
        TP2860_WriteI2c_Byte(0x2c, 0x2a); 
        TP2860_WriteI2c_Byte(0x2d, 0x30);
        TP2860_WriteI2c_Byte(0x2e, 0x70);
    
        TP2860_WriteI2c_Byte(0x30, 0x48);  
        TP2860_WriteI2c_Byte(0x31, 0xbb); 
        TP2860_WriteI2c_Byte(0x32, 0x2e);
        TP2860_WriteI2c_Byte(0x33, 0x90);
                
        TP2860_WriteI2c_Byte(0x35, 0x05);
        TP2860_WriteI2c_Byte(0x38, 0x00); 
        TP2860_WriteI2c_Byte(0x39, 0x1C); 	

        if(STD_HDA == std)  //AHD1080p25 extra                   
        {                                     
            TP2860_WriteI2c_Byte(0x02, 0xcc);
            
            TP2860_WriteI2c_Byte(0x0d, 0x73);
            
            TP2860_WriteI2c_Byte(0x15, 0x01);
            TP2860_WriteI2c_Byte(0x16, 0xf0);
            TP2860_WriteI2c_Byte(0x18, 0x2a);
                        
            TP2860_WriteI2c_Byte(0x20, 0x3c);
            TP2860_WriteI2c_Byte(0x21, 0x46);
            
            TP2860_WriteI2c_Byte(0x25, 0xfe);
            TP2860_WriteI2c_Byte(0x26, 0x0d);
            
            TP2860_WriteI2c_Byte(0x2c, 0x3a);
            TP2860_WriteI2c_Byte(0x2d, 0x54);
            TP2860_WriteI2c_Byte(0x2e, 0x40);
            
            TP2860_WriteI2c_Byte(0x30, 0xa5);
            TP2860_WriteI2c_Byte(0x31, 0x86);
            TP2860_WriteI2c_Byte(0x32, 0xfb);
            TP2860_WriteI2c_Byte(0x33, 0x60); 		   			
        }
        TP2860_WriteI2c_Byte(0xf3, 0x0);/*CLK delay , (0.2ns * 0)*/			
	}
	else if(FHD275 == fmt)	//TVI 1080p27.5
	{
        TP2860_WriteI2c_Byte(0x02, 0xc8);
        TP2860_WriteI2c_Byte(0x07, 0xc0); 
        TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
        TP2860_WriteI2c_Byte(0x0c, 0x03); 
        TP2860_WriteI2c_Byte(0x0d, 0x50);  
    
        TP2860_WriteI2c_Byte(0x15, 0x13);
        TP2860_WriteI2c_Byte(0x16, 0x88); 
        TP2860_WriteI2c_Byte(0x17, 0x80); 
        TP2860_WriteI2c_Byte(0x18, 0x29);
        TP2860_WriteI2c_Byte(0x19, 0x38);
        TP2860_WriteI2c_Byte(0x1a, 0x47);				
    
        TP2860_WriteI2c_Byte(0x1c, 0x09);  //1920*1080, 27.5fps
        TP2860_WriteI2c_Byte(0x1d, 0x60);  //
        
        TP2860_WriteI2c_Byte(0x20, 0x30);  
        TP2860_WriteI2c_Byte(0x21, 0x84); 
        TP2860_WriteI2c_Byte(0x22, 0x36);
        TP2860_WriteI2c_Byte(0x23, 0x3c);
    
        TP2860_WriteI2c_Byte(0x2b, 0x60);  
        TP2860_WriteI2c_Byte(0x2c, 0x2a); 
        TP2860_WriteI2c_Byte(0x2d, 0x30);
        TP2860_WriteI2c_Byte(0x2e, 0x70);
    
        TP2860_WriteI2c_Byte(0x30, 0x48);  
        TP2860_WriteI2c_Byte(0x31, 0xbb); 
        TP2860_WriteI2c_Byte(0x32, 0x2e);
        TP2860_WriteI2c_Byte(0x33, 0x90);
                
        TP2860_WriteI2c_Byte(0x35, 0x05);
        TP2860_WriteI2c_Byte(0x38, 0x00); 
        TP2860_WriteI2c_Byte(0x39, 0x1C); 	
		if(STD_HDA == std)                    
		{
            TP2860_WriteI2c_Byte(0x02, 0xc8);

            TP2860_WriteI2c_Byte(0x0d, 0x50);

            TP2860_WriteI2c_Byte(0x15, 0x11);
            TP2860_WriteI2c_Byte(0x16, 0xd2);
            TP2860_WriteI2c_Byte(0x18, 0x2a);
                
            TP2860_WriteI2c_Byte(0x20, 0x38);
            TP2860_WriteI2c_Byte(0x21, 0x46);

            TP2860_WriteI2c_Byte(0x25, 0xfe);
            TP2860_WriteI2c_Byte(0x26, 0x0d);

            TP2860_WriteI2c_Byte(0x2c, 0x3a);
            TP2860_WriteI2c_Byte(0x2d, 0x54);
            TP2860_WriteI2c_Byte( 0x2e, 0x40);

            TP2860_WriteI2c_Byte(0x30, 0x29);
            TP2860_WriteI2c_Byte(0x31, 0x85);
            TP2860_WriteI2c_Byte(0x32, 0x1e);
            TP2860_WriteI2c_Byte(0x33, 0xb0); 			
		}
        TP2860_WriteI2c_Byte(0xf3, 0x0);/* CLK delay , (0.2ns * 0) */
	
	}
	else if(FHD28 == fmt)	//TVI 1080p28
	{
        TP2860_WriteI2c_Byte(0x02, 0xc8);
        TP2860_WriteI2c_Byte(0x07, 0xc0); 
        TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
        TP2860_WriteI2c_Byte(0x0c, 0x03); 
        TP2860_WriteI2c_Byte(0x0d, 0x50);  
    
        TP2860_WriteI2c_Byte(0x15, 0x03);
        TP2860_WriteI2c_Byte(0x16, 0xd2); 
        TP2860_WriteI2c_Byte(0x17, 0x80); 
        TP2860_WriteI2c_Byte(0x18, 0x79);
        TP2860_WriteI2c_Byte(0x19, 0x38);
        TP2860_WriteI2c_Byte(0x1a, 0x47);				
    
        TP2860_WriteI2c_Byte(0x1c, 0x08);  //1920*1080, 27.5fps
        TP2860_WriteI2c_Byte(0x1d, 0x98);  //
        
        TP2860_WriteI2c_Byte(0x20, 0x30);  
        TP2860_WriteI2c_Byte(0x21, 0x84); 
        TP2860_WriteI2c_Byte(0x22, 0x36);
        TP2860_WriteI2c_Byte(0x23, 0x3c);
    
        TP2860_WriteI2c_Byte(0x2b, 0x60);  
        TP2860_WriteI2c_Byte(0x2c, 0x2a); 
        TP2860_WriteI2c_Byte(0x2d, 0x30);
        TP2860_WriteI2c_Byte(0x2e, 0x70);
    
        TP2860_WriteI2c_Byte(0x30, 0x48);  
        TP2860_WriteI2c_Byte(0x31, 0xbb); 
        TP2860_WriteI2c_Byte(0x32, 0x2e);
        TP2860_WriteI2c_Byte(0x33, 0x90);
                
        TP2860_WriteI2c_Byte(0x35, 0x14);
        TP2860_WriteI2c_Byte(0x36, 0xb5);			
        TP2860_WriteI2c_Byte(0x38, 0x00); 
        TP2860_WriteI2c_Byte(0x39, 0x1C);

        TP2860_WriteI2c_Byte(0xf3, 0x0);/* CLK delay , (0.2ns * 0) */
	
	}
    else if(HD50 == fmt) 
    {
        TP2860_WriteI2c_Byte(0x02, 0xca);
        TP2860_WriteI2c_Byte(0x07, 0xc0); 
        TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
        TP2860_WriteI2c_Byte(0x0c, 0x13); 
        TP2860_WriteI2c_Byte(0x0d, 0x50);  

        TP2860_WriteI2c_Byte(0x15, 0x13);
        TP2860_WriteI2c_Byte(0x16, 0x15); 
        TP2860_WriteI2c_Byte(0x17, 0x00); 
        TP2860_WriteI2c_Byte(0x18, 0x19);
        TP2860_WriteI2c_Byte(0x19, 0xd0);
        TP2860_WriteI2c_Byte(0x1a, 0x25);			
        TP2860_WriteI2c_Byte(0x1c, 0x07);  //1280*720, 
        TP2860_WriteI2c_Byte(0x1d, 0xbc);  //1280*720, 50fps

        TP2860_WriteI2c_Byte(0x20, 0x30);  
        TP2860_WriteI2c_Byte(0x21, 0x84); 
        TP2860_WriteI2c_Byte(0x22, 0x36);
        TP2860_WriteI2c_Byte(0x23, 0x3c);

        TP2860_WriteI2c_Byte(0x2b, 0x60);  
        TP2860_WriteI2c_Byte(0x2c, 0x2a); 
        TP2860_WriteI2c_Byte(0x2d, 0x30);
        TP2860_WriteI2c_Byte(0x2e, 0x70);

        TP2860_WriteI2c_Byte(0x30, 0x48);  
        TP2860_WriteI2c_Byte(0x31, 0xbb); 
        TP2860_WriteI2c_Byte(0x32, 0x2e);
        TP2860_WriteI2c_Byte(0x33, 0x90);
        
        TP2860_WriteI2c_Byte(0x35, 0x05); 
        TP2860_WriteI2c_Byte(0x38, 0x00);	
        TP2860_WriteI2c_Byte(0x39, 0x1c); 
        
        if(STD_HDA == std)      //subcarrier=24M              
        {                                     
            TP2860_WriteI2c_Byte(0x02, 0xce);			
            TP2860_WriteI2c_Byte(0x05, 0x01);	
            TP2860_WriteI2c_Byte(0x0d, 0x76);	
            TP2860_WriteI2c_Byte(0x0e, 0x0a);	
            TP2860_WriteI2c_Byte(0x14, 0x00);	
            TP2860_WriteI2c_Byte(0x15, 0x13);
            TP2860_WriteI2c_Byte(0x16, 0x1a);
            TP2860_WriteI2c_Byte(0x18, 0x1b);
                        
            TP2860_WriteI2c_Byte(0x20, 0x40);
            
            TP2860_WriteI2c_Byte(0x26, 0x01);
            
            TP2860_WriteI2c_Byte(0x2c, 0x3a);
            TP2860_WriteI2c_Byte(0x2d, 0x54);
            TP2860_WriteI2c_Byte(0x2e, 0x50);
            
            TP2860_WriteI2c_Byte(0x30, 0xa5);
            TP2860_WriteI2c_Byte(0x31, 0x9f);
            TP2860_WriteI2c_Byte(0x32, 0xce);
            TP2860_WriteI2c_Byte(0x33, 0x60);
        }
        TP2860_WriteI2c_Byte(0xf3, 0x0);/* CLK delay , (0.2ns * 0) */
    }
    else if(HD60 == fmt)
    {
        TP2860_WriteI2c_Byte(0x02, 0xca);
        TP2860_WriteI2c_Byte(0x07, 0xc0); 
        TP2860_WriteI2c_Byte(0x0b, 0xc0);  		
        TP2860_WriteI2c_Byte(0x0c, 0x13); 
        TP2860_WriteI2c_Byte(0x0d, 0x50);  

        TP2860_WriteI2c_Byte(0x15, 0x13);
        TP2860_WriteI2c_Byte(0x16, 0x15); 
        TP2860_WriteI2c_Byte(0x17, 0x00); 
        TP2860_WriteI2c_Byte(0x18, 0x19);
        TP2860_WriteI2c_Byte(0x19, 0xd0);
        TP2860_WriteI2c_Byte(0x1a, 0x25);			
        TP2860_WriteI2c_Byte(0x1c, 0x06);  //1280*720, 
        TP2860_WriteI2c_Byte(0x1d, 0x72);  //1280*720, 60fps

        TP2860_WriteI2c_Byte(0x20, 0x30);  
        TP2860_WriteI2c_Byte(0x21, 0x84); 
        TP2860_WriteI2c_Byte(0x22, 0x36);
        TP2860_WriteI2c_Byte(0x23, 0x3c);

        TP2860_WriteI2c_Byte(0x2b, 0x60);  
        TP2860_WriteI2c_Byte(0x2c, 0x2a); 
        TP2860_WriteI2c_Byte(0x2d, 0x30);
        TP2860_WriteI2c_Byte(0x2e, 0x70);

        TP2860_WriteI2c_Byte(0x30, 0x48);  
        TP2860_WriteI2c_Byte(0x31, 0xbb); 
        TP2860_WriteI2c_Byte(0x32, 0x2e);
        TP2860_WriteI2c_Byte(0x33, 0x90);
        
        TP2860_WriteI2c_Byte(0x35, 0x05); 
        TP2860_WriteI2c_Byte(0x38, 0x00);	
        TP2860_WriteI2c_Byte(0x39, 0x1c); 
		
        if(STD_HDA == std)		////subcarrier=22M 
        {
            TP2860_WriteI2c_Byte(0x02, 0xce);
            TP2860_WriteI2c_Byte(0x05, 0x55);
            TP2860_WriteI2c_Byte(0x0d, 0x76);
            TP2860_WriteI2c_Byte(0x0e, 0x08); 
            TP2860_WriteI2c_Byte(0x14, 0x00); 
            TP2860_WriteI2c_Byte(0x15, 0x13); 
            TP2860_WriteI2c_Byte(0x16, 0x25);     	    	    	   	
            TP2860_WriteI2c_Byte(0x18, 0x1b);
            
            TP2860_WriteI2c_Byte(0x20, 0x50);
            TP2860_WriteI2c_Byte(0x21, 0x84);

            TP2860_WriteI2c_Byte(0x25, 0xff);
            TP2860_WriteI2c_Byte(0x26, 0x01);

            TP2860_WriteI2c_Byte(0x2c, 0x2a);
            TP2860_WriteI2c_Byte(0x2d, 0x54);
            TP2860_WriteI2c_Byte(0x2e, 0x60);

            TP2860_WriteI2c_Byte(0x30, 0xa5);
            TP2860_WriteI2c_Byte(0x31, 0x8b);
            TP2860_WriteI2c_Byte(0x32, 0xf2);
            TP2860_WriteI2c_Byte(0x33, 0x60);
        }
        TP2860_WriteI2c_Byte(0xf3, 0x0);/* CLK delay , (0.2ns * 0) */
	}
}

void SetTimingRegTable(uint16_t res)
{

    switch (res)
    {
        case TP2860_CVBS_NTSC:
            TP9951_sensor_init(gCurVin,  NTSC, STD_HDA);/*NTSC , VIN1*/
            (void)printf("Resolution (NTSC) \n");
            break;

        case TP2860_CVBS_PAL:
            TP9951_sensor_init(gCurVin,  PAL, STD_HDA);/*PAL , VIN1*/
            (void)printf("Resolution (PAL) \n");
            break;

        case TP2860_HDA720P_25FPS:
            TP9951_sensor_init(gCurVin,  HD25, STD_HDA); /*AHD 720P 25 , VIN1*/
            (void)printf("Resolution (HDA720P_25FPS) \n");
            break;

        case TP2860_HDA720P_30FPS:
            TP9951_sensor_init(gCurVin,  HD30, STD_HDA); /*AHD 720P 30 , VIN1*/
            (void)printf("Resolution (HDA720P_30FPS) \n");
            break;

        case TP2860_HDA1080P_25FPS:
            TP9951_sensor_init(gCurVin,  FHD25, STD_HDA);/*AHD 1080P 25 , VIN1*/
            (void)printf("Resolution (HDA1080P_30FPS) \n");
            break;

        case TP2860_HDA1080P_30FPS:
            TP9951_sensor_init(gCurVin,  FHD30, STD_HDA);/*AHD 1080P 30 , VIN1*/
            (void)printf("Resolution (HDA1080P_30FPS) \n");
            break;

        default:
            (void)printf("unknown resolution \n");
            break;
    }
}
// =============================================================================
//                USER IMPLEMENT FUNCTION START
// =============================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////

// X10LightDriver_t1.c
void
TP2860Initialize (
    uint16_t Mode)
{

    (void)printf("TP2860 Initialize: \n");

    TP2860_I2c_Init();

    /* Reset global value */
    gCurVin = 0xff;

    /* Set video resolution */
    CurrentResolution = Mode;
    #if 0
    CurrentResolution = TP2860_HDA1080P_30FPS;
    #endif
}

void
TP2860Terminate (
    void)
{

}

void
TP2860OutputPinTriState (
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
TP2860IsSignalStable (
    uint16_t Mode)
{
    uint8_t status      = 0;
    bool    isStable    = true;

    status = TP2860_ReadI2c_Byte(TP2860_VIDEO_INPUT_STATUS_REG);
    (void)printf("TP2860 reg(0x01) = %x\n", status);

    if ((status & 0xF0) == (TP2860_V_PLL_LOCK | TP2860_H_PLL_LOCK | TP2860_Carrier_PLL_LOCK))
    {
        if ((status & 0x08) == TP2860_Video_Dectect)
        {
            isStable = true;
        }
    }
    else
    {
        isStable = false;
    }

    /* stable return true else return false               */
    return isStable;
}

uint16_t
TP2860GetProperty (
    MODULE_GETPROPERTY property)
{
    /* Please implement get information from device code here */
    switch (property)
    {
        case GetHeight:
            return tp2860_info_table[CurrentResolution].height;

        case GetWidth:
            return tp2860_info_table[CurrentResolution].width;

        /* frame rate */
        case Rate:
            return tp2860_info_table[CurrentResolution].framerate;

        case GetModuleIsInterlace:
            return tp2860_info_table[CurrentResolution].interlaced;

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
TP2860GetStatus (
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
TP2860SetProperty (
    MODULE_SETPROPERTY  Property,
    uint16_t            Value)
{
    /* Please implement set property to device code here */
    switch (Property)
    {
        case VideoInCH:
            if ((Value & 0xff) != gCurVin)
            {
                gCurVin = (unsigned char)Value;
                (void)printf("[TP2860 SetProperty]CH %d\n", gCurVin);
                SetTimingRegTable(CurrentResolution);
            }
            break;
        
        default:
            break;
    } 
}

void
TP2860PowerDown (
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
        (void)printf("[TP2860] Hw Reset\n");
#endif
    }    
}

// =============================================================================
//                USER IMPLEMENT FUNCTION END
// =============================================================================
void
TP2860SensorDriver_Destory (
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
    TP2860Initialize,
    TP2860Terminate,
    TP2860OutputPinTriState,
    TP2860IsSignalStable,
    TP2860GetProperty,
    TP2860GetStatus,
    TP2860SetProperty,
    TP2860PowerDown,
    TP2860SensorDriver_Destory
};

SensorDriver
TP2860SensorDriver_Create ()
{
    TP2860SensorDriver self = calloc(1, sizeof(TP2860SensorDriverStruct));
    if (self != NULL)
    {
        self->base.vtable   = &interface;
        self->base.type     = "TP2860";
    }
    return (SensorDriver)self;
}

// end of X10LightDriver_t1.c
