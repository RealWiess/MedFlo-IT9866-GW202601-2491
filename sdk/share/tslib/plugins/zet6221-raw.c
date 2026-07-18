/*
 you have to make sure 3 things before porting TP driver
 1).INT is work normal
 2).I2C BUS can read data from TP chip
 3).Parse the X,Y coordination correctly

 These function are customized.
 You Have to modify these function with "(*)" mark.
 These functions(3&4) are almost without modification
 Function(5~7) will be modified deponding on chip's feature.
  0._tpInitSpec_vendor()           //set control config(*)
  1._tpReadPointBuffer_vendor()    //read point buffer(*)
  2._tpParseRawPxy_vendor()        //parse the touch point(*)
  3._tpIntActiveRule_vendor()      //touch-down RULE
  4._tpIntNotActiveRule_vendor()   //touch-up RULE
5._tpParseKey_vendor()           //depend on TP with key
6._tpDoPowerOnSeq_vendor();      //depend on TP with power-on sequence
7._tpDoInitProgram_vendor();         //depend on TP with initial programming
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>
#include <pthread.h>	
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "ite/ith.h" 
#include "ite/itp.h"
#include "config.h"
#include "tslib-private.h"

#include "api-raw.h"

#ifdef	CFG_TOUCH_MULTI_FINGER
    #define TP_MULTI_FINGER_ENABLE
#endif

#define USE_RAW_API
#define TP_USE_XQUEUE
/****************************************************************************
 * initial Kconfig setting
 ****************************************************************************/
#ifndef	CFG_TOUCH_INTR
	#define CFG_TOUCH_INTR	//FORCE TO ENABLE ISR &interrupt for this panel
#endif

#if	defined(CFG_TOUCH_I2C0) || defined(CFG_TOUCH_I2C1) || defined(CFG_TOUCH_I2C2) || defined(CFG_TOUCH_I2C3)
#define TP_INTERFACE_I2C   (0)
#endif

#if	defined(CFG_TOUCH_SPI) || defined(CFG_TOUCH_SPI0) || defined(CFG_TOUCH_SPI1)
#define TP_INTERFACE_SPI   (1)
#endif

#define TP_INT_PIN	    CFG_GPIO_TOUCH_INT
#define TP_GPIO_MASK    (1<<(TP_INT_PIN%32))

#ifdef	CFG_GPIO_TOUCH_WAKE
#if (CFG_GPIO_TOUCH_WAKE<128)
#define TP_GPIO_WAKE_PIN	CFG_GPIO_TOUCH_WAKE
#endif 
#endif 

#ifdef	CFG_GPIO_TOUCH_RESET
#if (CFG_GPIO_TOUCH_RESET<128)
#define TP_GPIO_RESET_PIN	CFG_GPIO_TOUCH_RESET
#else
#define TP_GPIO_RESET_PIN	(-1)
#endif 
#else
#define TP_GPIO_RESET_PIN	(-1)
#endif 

#ifdef	CFG_TOUCH_ADVANCE_CONFIG

#ifdef	CFG_TOUCH_SWAP_XY
#define	TP_SWAP_XY		(1)
#else
#define	TP_SWAP_XY		(0)
#endif

#ifdef	CFG_TOUCH_REVERSE_X
#define	TP_REVERSE_X	(1)
#else
#define	TP_REVERSE_X	(0)
#endif

#ifdef	CFG_TOUCH_REVERSE_Y
#define	TP_REVERSE_Y	(1)
#else
#define	TP_REVERSE_Y	(0)
#endif

#if defined(CFG_TOUCH_I2C_SLAVE_ID) && (CFG_TOUCH_I2C_SLAVE_ID != -1)
#define	TP_I2C_ID_KCONFIG		CFG_TOUCH_I2C_SLAVE_ID
#endif

#else

#define	TP_SWAP_XY		(0)
#define	TP_REVERSE_X	(0)
#define	TP_REVERSE_Y	(0)

#endif

#define	TOUCH_NO_CONTACT		(0)
#define	TOUCH_DOWN				(1)
#define	TOUCH_UP				(2)

#define	TP_ACTIVE_LOW           (0)
#define	TP_ACTIVE_HIGH          (1)

#ifdef	CFG_GPIO_TOUCH_INT_ACTIVE_HIGH
#define	TP_INT_ACTIVE_STATE     TP_ACTIVE_HIGH
#else
#define	TP_INT_ACTIVE_STATE     TP_ACTIVE_LOW
#endif

#define	TP_INT_LEVLE_TRIGGER    (1)
#define	TP_INT_EDGE_TRIGGER     (0)

#define	TP_INT_TYPE_KEEP_STATE  (0)
#define	TP_INT_TYPE_ZT2083      (0)
#define	TP_INT_TYPE_FT5XXX      (1)
#define	TP_INT_TYPE_IT7260      (2)

#define	TP_WITHOUT_KEY          (0)
#define	TP_HAS_TOUCH_KEY        (1)
#define	TP_GPIO_PIN_NO_DEF      (-1)

#ifdef	CFG_TOUCH_BUTTON
#define	TP_TOUCH_BUTTON		TP_HAS_TOUCH_KEY
#else
#define	TP_TOUCH_BUTTON		TP_WITHOUT_KEY
#endif

#ifdef CFG_TOUCH_INTR
#define	TP_ENABLE_INTERRUPT     (1)
#else
#define	TP_ENABLE_INTERRUPT     (0)
#endif

#ifdef TP_MULTI_FINGER_ENABLE
#define	MAX_FINGER_NUM	(1)		//depend on TP Native Max Finger Numbers  
#else
#define	MAX_FINGER_NUM	(1)
#endif

#ifdef TP_USE_XQUEUE
#define	TP_QUEUE_LEN	(64)
#endif

/****************************************************************************
 * touch cofig setting
 ****************************************************************************/
#define TP_IDLE_TIME                (2000)
#define TP_IDLE_TIME_NO_INITIAL     (100000)

/****************************************************************************
 * ENABLE_TOUCH_POSITION_MSG :: just print X,Y coordination & 
 * 								touch-down/touch-up
 * ENABLE_TOUCH_IIC_DBG_MSG  :: show the IIC command 
 * ENABLE_TOUCH_PANEL_DBG_MSG:: show send-queue recieve-queue, 
 *                              and the xy value of each INTr
 ****************************************************************************/
//#define ENABLE_TOUCH_POSITION_MSG
//#define ENABLE_TOUCH_RAW_POINT_MSG
//#define ENABLE_TOUCH_PANEL_DBG_MSG
//#define ENABLE_TOUCH_IIC_DBG_MSG
//#define ENABLE_SEND_FAKE_SAMPLE

/****************************************************************************
 * MACRO define of ZET6221
 ****************************************************************************/
#ifdef	TP_I2C_ID_KCONFIG
#define TP_I2C_DEVICE_ID       (TP_I2C_ID_KCONFIG)
#else
#define TP_I2C_DEVICE_ID       (0x76)
#endif

#define TP_SAMPLE_RATE	       (33)

#ifdef	CFG_LCD_ENABLE
#define	TP_SCREEN_WIDTH	    ithLcdGetWidth()
#define	TP_SCREEN_HEIGHT    ithLcdGetHeight()
#else
#define	TP_SCREEN_WIDTH	    960
#define	TP_SCREEN_HEIGHT	576
#endif

/***************************
 * global variable
 **************************/
static struct ts_sample g_sample[MAX_FINGER_NUM];
static struct ts_sample gTmpSmp[10];

static char g_TouchDownIntr = false;
static char  g_IsTpInitialized = false;
static pthread_mutex_t 	gTpMutex;

#ifdef USE_RAW_API
static RA_TP_SPEC  gTpSpec;
static RA_GV       gTpInfo = { 0,RA_TOUCH_NO_CONTACT,1,0,0,0,0,0,0};
#else
static TP_SPEC     gTpSpec;
static tp_info_tag gTpInfo = { 0,TOUCH_NO_CONTACT,1,0,0,0,0,0,0};
#endif

static unsigned int dur=0;
static unsigned int iDur=0;
static unsigned int lowDur=0;

struct timeval T1, T2;
static int g_tpCntr = 0;
static unsigned int gLastNumFinger = 0;

//for the function "_tpFixIntHasNoResponseIssue()"
static int  g_IntrLowCnt = 0;
static int  g_IntrAtvCnt = 0;

struct timeval tv1, tv2;
static int  gNoEvtCnt = 0;

#ifdef TP_USE_XQUEUE
static QueueHandle_t tpQueue;
static int  SendQueCnt = 0;
#endif

/*************************************************
 global variable: gTpKeypadValue
 key0 is pressed if gTpKeypadValue's bit 0 is 1
 key1 is pressed if gTpKeypadValue's bit 1 is 1
   ...and so on

 NO key event if gTpKeypadValue = 0 
 MAX key number: 32 keys
***************************************************/
static uint32_t	gTpKeypadValue = 0;

/*##################################################################################
 *                         the protocol of private function
 ###################################################################################*/
static void _tpInitSpec_vendor(void);
static int  _tpReadPointBuffer_vendor(unsigned char *buf, int cnt);
static int  _tpParseRawPxy_vendor(struct ts_sample *s, unsigned char *buf);
static void _tpParseKey_vendor(struct ts_sample *s, unsigned char *buf);

static void _tpIntActiveRule_vendor(struct ts_sample *tpSmp);
static void _tpIntNotActiveRule_vendor(struct ts_sample *tpSmp);

static void _tpDoPowerOnSeq_vendor(void);
static int _tpDoInitProgram_vendor(void);

/* *************************************************************** */

/* *************************************************************** */
void (*tp_DoPowerOnSeq_callback)(void) = _tpDoPowerOnSeq_vendor;

/*##################################################################################
 *                        the private function implementation
 ###################################################################################*/
static void _tpInitSpec_vendor(void)
{
    gTpSpec.tpIntPin          	= (char)TP_INT_PIN;           //from Kconfig setting
    gTpSpec.tpWakeUpPin         = (char)TP_GPIO_PIN_NO_DEF;   //from Kconfig setting
    gTpSpec.tpResetPin          = (char)TP_GPIO_PIN_NO_DEF;   //from Kconfig setting
    gTpSpec.tpIntUseIsr         = (char)TP_ENABLE_INTERRUPT;  //from Kconfig setting
    gTpSpec.tpIntActiveState    = (char)TP_ACTIVE_LOW;        //from Kconfig setting    
    gTpSpec.tpIntTriggerType    = (char)TP_INT_EDGE_TRIGGER;  //from Kconfig setting   level/edge
    
    gTpSpec.tpInterface         = (char)TP_INTERFACE_I2C;	  //from Kconfig setting
    gTpSpec.tpIntActiveMaxIdleTime = (char)TP_SAMPLE_RATE;	  //from Kconfig setting
        
    gTpSpec.tpMaxRawX           = (int)CFG_TOUCH_X_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpMaxRawY           = (int)CFG_TOUCH_Y_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpScreenX           = (int)TP_SCREEN_WIDTH;       //from Kconfig setting
    gTpSpec.tpScreenY           = (int)TP_SCREEN_HEIGHT;      //from Kconfig setting
    
    gTpSpec.tpCvtSwapXY        = (char)TP_SWAP_XY;            //from Kconfig setting
    gTpSpec.tpCvtReverseX      = (char)TP_REVERSE_X;          //from Kconfig setting
    gTpSpec.tpCvtReverseY      = (char)TP_REVERSE_Y;          //from Kconfig setting
    gTpSpec.tpCvtScaleX        = (char)0;                     //from Kconfig setting
    gTpSpec.tpCvtScaleY        = (char)0;                     //from Kconfig setting
    
    gTpSpec.tpI2cDeviceId       = (char)TP_I2C_DEVICE_ID;	  //from this driver setting
    gTpSpec.tpEnTchPressure     = (char)0;                    //from this driver setting
    gTpSpec.tpSampleNum         = (char)1;                    //from this driver setting
    gTpSpec.tpSampleRate        = (char)TP_SAMPLE_RATE;       //from this driver setting
    gTpSpec.tpIntrType          = (char)TP_INT_TYPE_FT5XXX;	  //from this driver setting
    gTpSpec.tpHasTouchKey       = (char)TP_WITHOUT_KEY;       //from this driver setting                                                               
    gTpSpec.tpIdleTime          = (int)TP_IDLE_TIME;          //from this driver setting
    gTpSpec.tpIdleTimeB4Init    = (int)TP_IDLE_TIME_NO_INITIAL;//from this driver setting    
    gTpSpec.tpMoveDetectUnit    = (int)1;
    gTpSpec.tpReadChipRegCnt    = (int)24;
    
    //special initial flow
    gTpSpec.tpHasPowerOnSeq     = (char)0;
    gTpSpec.tpNeedProgB4Init    = (char)0;    
    gTpSpec.tpNeedAutoTouchUp	= (char)1; 
    gTpSpec.tpIntPullEnable     = (char)0;

/*
    printf("gTpSpec.tpIntPin         = %d\n",gTpSpec.tpIntPin);
    printf("gTpSpec.tpIntActiveState = %x\n",gTpSpec.tpIntActiveState);
    printf("gTpSpec.tpWakeUpPin      = %d\n",gTpSpec.tpWakeUpPin);
    printf("gTpSpec.tpResetPin       = %d\n",gTpSpec.tpResetPin);
    printf("gTpSpec.tpIntrType       = %x\n",gTpSpec.tpIntrType);
    printf("gTpSpec.tpInterface      = %x\n",gTpSpec.tpInterface);
    printf("gTpSpec.tpI2cDeviceId    = %x\n",gTpSpec.tpI2cDeviceId);
    printf("gTpSpec.tpHasTouchKey    = %x\n",gTpSpec.tpHasTouchKey);
    printf("gTpSpec.tpIntUseIsr      = %x\n",gTpSpec.tpIntUseIsr);
    printf("gTpSpec.tpMaxRawX        = %d\n",gTpSpec.tpMaxRawX);
    printf("gTpSpec.tpMaxRawY        = %d\n",gTpSpec.tpMaxRawY);
    printf("gTpSpec.tpScreenX        = %d\n",gTpSpec.tpScreenX);
    printf("gTpSpec.tpScreenY        = %d\n",gTpSpec.tpScreenY);
    printf("gTpSpec.tpCvtSwapXY     = %x\n",gTpSpec.tpCvtSwapXY);
    printf("gTpSpec.tpCvtReverseX   = %x\n",gTpSpec.tpCvtReverseX);
    printf("gTpSpec.tpCvtReverseY   = %x\n",gTpSpec.tpCvtReverseY);
    printf("gTpSpec.tpCvtScaleX     = %x\n",gTpSpec.tpCvtScaleX);
    printf("gTpSpec.tpCvtScaleY     = %x\n",gTpSpec.tpCvtScaleY);
    printf("gTpSpec.tpEnTchPressure  = %x\n",gTpSpec.tpEnTchPressure);
    printf("gTpSpec.tpSampleNum      = %x\n",gTpSpec.tpSampleNum);
    printf("gTpSpec.tpSampleRate     = %x\n",gTpSpec.tpSampleRate);
    printf("gTpSpec.tpIdleTime       = %d\n",gTpSpec.tpIdleTime);
    printf("gTpSpec.tpIdleTimeB4Init = %d\n",gTpSpec.tpIdleTimeB4Init);
    printf("gTpSpec.tpHasPowerOnSeq  = %x\n",gTpSpec.tpHasPowerOnSeq);
    printf("gTpSpec.tpNeedProgB4Init = %x\n",gTpSpec.tpNeedProgB4Init);
	printf("gTpSpec.tpNeedAutoTouchUp= %x\n",gTpSpec.tpNeedAutoTouchUp);
*/
    //initial global variable "gTpInfo"
/*    
    printf("gTpInfo.tpCurrINT              = %x\n",gTpInfo.tpCurrINT);
    printf("gTpInfo.tpStatus               = %x\n",gTpInfo.tpStatus);
    printf("gTpInfo.tpNeedToGetSample      = %x\n",gTpInfo.tpNeedToGetSample);
    printf("gTpInfo.tpNeedUpdateSample     = %x\n",gTpInfo.tpNeedUpdateSample);
    printf("gTpInfo.tpFirstSampHasSend     = %x\n",gTpInfo.tpFirstSampHasSend);
    printf("gTpInfo.tpFirstSampHasSend     = %x\n",gTpInfo.tpIsInitFinished);
    printf("gTpInfo.tpIntr4Probe           = %x\n",gTpInfo.tpIntr4Probe);
    printf("gTpInfo.tpDevFd                = %x\n",gTpInfo.tpDevFd);    
    printf("gTpInfo.tpIntrCnt              = %x\n",gTpInfo.tpIntrCnt);
*/
}    

static void _tpDoPowerOnSeq_vendor(void)
{
    //TODO:: for power-on sequence
}

static int _tpDoInitProgram_vendor(void)
{
    //TODO: program touch IC for initial at first.(like IT7260)
    return 0;
}

/****************************************************************
input: 
    buf: the buffer base, 
    cnt: the buffer size in bytes
output: 
    0: pass(got valid data)
    1: skip sample this time 
    -1: i2c error (upper-layer will send touch-up event)
*****************************************************************/
static int _tpReadPointBuffer_vendor(unsigned char *buf, int cnt)
{
    ITPI2cInfo *evt;
    unsigned char	I2cCmd;
    int i2cret;

    evt = alloca(sizeof(ITPI2cInfo));

    evt->slaveAddress   = gTpSpec.tpI2cDeviceId;
    evt->cmdBuffer      = &I2cCmd;
    evt->cmdBufferSize  = 0;
    evt->dataBuffer     = buf;
    evt->dataBufferSize = cnt;
    i2cret = read(gTpInfo.tpDevFd, evt, 1);
    if(i2cret<0)	return -1;

	#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
	{
		int i;		
		printf("\n	raw-buf(cnt=%d):",cnt);
		for(i=0; i<9; i++)
		{
			printf("%02x ",buf[i]);
			if((i&0xf)==0xf)	printf("\n			");
		}
		printf("\n");		
	}
	#endif
	
	return 0;
}                               


/*
 In multi-finger case, parse the registers of TP controller, event if fingers became zero.
 Because it still has its touch-up event need to be reported when finger is 0.
*/
static int _tpParseRawPxy_vendor(struct ts_sample *s, unsigned char *buf)
{
	if( (buf[0] == 0x3C) && (buf[1] & 0x80) )
	{
		s->x = (int)((((unsigned int)buf[3]<<4)&0x0700) | ((unsigned int)buf[4]));
		s->y = (int)((((unsigned int)buf[3]<<8)&0x0700) | ((unsigned int)buf[5]));
		s->pressure = 1;
		
		#ifdef	ENABLE_TOUCH_RAW_POINT_MSG
       	printf("	RAW->---------> %d %d %d\n", s->pressure, s->x, s->y);
		#endif
		
		return 0;		
	}
	else if( (buf[0] == 0x3C) && (buf[1] == 0x00) )
	{
		s->x = (int)((((unsigned int)buf[3]<<4)&0x0700) | ((unsigned int)buf[4]));
		s->y = (int)((((unsigned int)buf[3]<<8)&0x0700) | ((unsigned int)buf[5]));
		s->pressure = 0;
	}
    else
    {
		#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
		printf("tch inv buf3=%x\r\n",buf[3]);
		#endif
		return -1;		
	}
}

static void _tpParseKey_vendor(struct ts_sample *s, unsigned char *buf)
{
    //TODO: get key information and input to xy sample...? as a special xy?
    //maybe define a special area for key
    //(like touch is 800x480, for example, y>500 for key, x=0~100 for keyA, x=100~200 for keyB... )
    //SDL layer could parse this special defination xy into key event(but this layer is not ready yet).
    //uint32_t	val = 0;
    //printf("Keypad Val = %x\n",gTpKeypadValue);
    
    //val = buf[5];
    /*
    if(x<320)	val |= 0x01;
    if(320<= x <640)	val |= 0x02;
    if(640<= x <960)	val |= 0x04;
    if(960<= x <1280)	val |= 0x08;
    */
    
    //example::(ref. cytma568-raw.c)
    //pthread_mutex_lock(&gTpMutex);	
    //gTpKeypadValue = val&0x0F;
    //pthread_mutex_unlock(&gTpMutex);	
}

static void _tpIntActiveRule_vendor(struct ts_sample *tpSmp)
{
    //IGNORE: default use _raIntActiveRule_vendor() in api-raw.c
}

static void _tpIntNotActiveRule_vendor(struct ts_sample *tpSmp)
{
    //IGNORE: default use _raIntActiveRule_vendor() in api-raw.c
}

#ifdef USE_RAW_API
static void _tpRegRawFunc(int fd)
{
	//initial raw api function
	RA_FUNC *ra = (RA_FUNC *)&gTpSpec.rawApi;
	
   	ra->raInitSpec 			= _tpInitSpec_vendor;
   	ra->raParseRawPxy 		= _tpParseRawPxy_vendor;
   	ra->raReadPointBuffer 	= _tpReadPointBuffer_vendor;
   	ra->raParseKey 			= _tpParseKey_vendor;
   	ra->raDoInitProgram 	= _tpDoInitProgram_vendor;
   	ra->raDoPowerOnSeq 		= _tpDoPowerOnSeq_vendor;
   	ra->raIntActiveRule 	= NULL;
   	ra->raIntNotActiveRule 	= NULL;
   	//ra->raIntActiveRule 	= _tpIntActiveRule_vendor;
   	//ra->raIntNotActiveRule = _tpIntNotActiveRule_vendor;   	
   	
   	gTpInfo.tpDevFd = fd;
   	gTpSpec.gTpSmpBase = (struct ts_sample *)&g_sample[0];
   	
   	gTpSpec.raInfoBase = &gTpInfo;
   	gTpSpec.raMutex = &gTpMutex;
   	gTpSpec.pTouchDownIntr = &g_TouchDownIntr;
   	gTpSpec.pTpInitialized = &g_IsTpInitialized;   	

	#ifdef	CFG_TOUCH_BUTTON
	gTpSpec.pTpKeypadValue = &gTpKeypadValue;
	#endif
   	
   	_raSetSpecBase(&gTpSpec);
}
#endif

#ifdef	ENABLE_SEND_FAKE_SAMPLE
int _getFakeSample(struct ts_sample *samp, int nr)
{
	printf("tp_getXY::cnt=%x\n",g_tpCntr);
	
	if(g_tpCntr++>0x100)
	{
		if( !(g_tpCntr&0x07) )
		{
			unsigned int i;
			i = (g_tpCntr>>3)&0x1F;
			if(i<MAX_FAKE_NUM)
			{
				samp->pressure = 1;
				samp->x = gFakeTableX[i];
				samp->y = gFakeTableY[i];
				printf("sendXY.=%d,%d\n",samp->x,samp->y);	
			}
		}
	}

	return nr;
}
#endif
/*##################################################################################
 *                           private function above
 ###################################################################################*/

/*##################################################################################
 #                       public function implementation
 ###################################################################################*/

/**
 * Send touch sample(samp->pressure, samp->x, samp->y, and samp->tv)
 *
 * @param inf: the module information of tslibo(just need to care "inf->dev")
 * @param samp: the touch samples
 * @param nr: the sample count that upper layer wanna get.
 * @return: the total touch sample count
 *
 * [HINT 1]:this function will be called by SDL every 33 ms. 
 * [HINT 2]:Upper layer(SDL) will judge finger-down(contact on TP) if samp->pressure>0, 
 *          finger-up(no touch) if samp->pressure=0.
 * [HINT 3]:please return either 0 or 1 (don't return other number for tslib rule, even if sample number is > 1) 
 */ 
static int zet6221_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	unsigned int regValue;
	int ret;
	int total = 0;
	int tchdev = ts->fd;
	struct ts_sample *s=samp;
	
	#ifdef	ENABLE_SEND_FAKE_SAMPLE
	return _getFakeSample(samp,nr);
	#endif	
	
	if(g_IsTpInitialized==false)
	{
		_tpRegRawFunc(tchdev);

		if(!_raDoInitial())	return -1;
		else                return 0;
	}
		
	//to probe touch sample 
	ret = _raProbeSample(samp, nr);
	
	#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
	if(ret)	printf("    deQue-O:fn=%d (%d,%d,%d)r=%d\n", samp->finger, samp->pressure, samp->x, samp->y, ret );
	#endif
	
	#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
	if(samp->pressure)	gNoEvtCnt = 3;	
	if( gNoEvtCnt )	
	{
		printf("    deQue-O1:[%x] fn=%d, id=%d (%d,%d,%d)r=%d\n", samp, samp->finger, samp->id, samp->pressure, samp->x, samp->y, ret );
		if(samp->finger>1)
		{
			struct ts_sample *tsp = (struct ts_sample *)samp->next;
			printf("    deQue-Q2:[%x] fn=%d, id=%d (%d,%d,%d)r=%d\n", tsp, tsp->finger, tsp->id, tsp->pressure, tsp->x, tsp->y, ret );
		}
		if( !samp->pressure )	gNoEvtCnt--;
	}
	#endif
	
	return ret;
}

static const struct tslib_ops zet6221_ops =
{
	zet6221_read,
};

TSAPI struct tslib_module_info *zet6221_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &zet6221_ops;
	return m;
}

#ifndef TSLIB_STATIC_CASTOR3_MODULE
	TSLIB_MODULE_INIT(zet6221_mod_init);
#endif
