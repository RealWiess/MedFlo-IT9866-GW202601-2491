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
 * MACRO define of FT5316
 ****************************************************************************/
#ifdef	TP_I2C_ID_KCONFIG
#define TP_I2C_DEVICE_ID       (TP_I2C_ID_KCONFIG)
#else
#define TP_I2C_DEVICE_ID       (0x46)
#endif

#define TP_SAMPLE_RATE	       (33)

#ifdef	CFG_LCD_ENABLE
#define	TP_SCREEN_WIDTH	    ithLcdGetWidth()
#define	TP_SCREEN_HEIGHT    ithLcdGetHeight()
#else
#define	TP_SCREEN_WIDTH	    800
#define	TP_SCREEN_HEIGHT	480
#endif

#define IT7260_FW_VERSION_SIZE		(0x09)	/* F/W version size of IT7260 */

/****************************************
 *
 ***************************************/
typedef struct 
{
	char tpCurrINT;
	char tpStatus;
	char tpNeedToGetSample;
	char tpNeedUpdateSample;
	char tpFirstSampHasSend;
	char tpIntr4Probe;
	char tpIsInitFinished;
	int  tpDevFd;
	int  tpIntrCnt;
}tp_info_tag;//tp_gv_tag

typedef struct tp_spec_tag
{
    //TP H/W setting
    char tpIntPin;		    //INT signal GPIO pin number
    char tpIntActiveState;	//High=1, Low=0
    char tpIntTriggerType;  //interrupt trigger type. 0:edge trigger, 1:level trigger
    char tpWakeUpPin;		//Wake-Up pin GPIO pin number, -1: means NO Wake-Up pin.
    char tpResetPin;		//Reset pin GPIO pin number, -1: means NO reset pin.
    char tpIntrType;		//0:keep state when touch down(like ZT2083), 1:like FT5XXX type 2:like IT7260, 3:others....  
    char tpInterface; 		//0:I2C, 1:SPI, 2:other...
    char tpI2cDeviceId; 	//I2C device ID(slave address) if TP has I2C interface
    char tpHasTouchKey;		//0: NO touch key, 1:touch key type I, 2:touch key type II, ...
    char tpIntUseIsr;	    //0:polling INT siganl, 1:INT use interrupt, 
    char tpMaxFingerNum;	//The TP native maximun of finger numbers
    char tpIntActiveMaxIdleTime;    //default: 33ms, cytma568: 100ms
        
    //TP resolution
    int  tpMaxRawX;
    int  tpMaxRawY;
    int  tpScreenX;
    int  tpScreenY;
    
    //TP convert function
    char tpCvtSwapXY;		//0:Disable, 1:Enable
    char tpCvtReverseX;     //0:Disable, 1:Enable
    char tpCvtReverseY;     //0:Disable, 1:Enable 
    char tpCvtScaleX;		//0:Disable, 1:Enable
    char tpCvtScaleY;		//0:Disable, 1:Enable
    
    //TP sample specification
    char tpEnTchPressure;	//0:disable pressure info, 1:enable pressure info
    char tpSampleNum;		//0:NO scense, 1: single touch 2~10:multi-touch("tpSampleNum" must be <= "tpMaxFingerNum") 
    char tpSampleRate;		//UNIT: mill-second, range 8~16 ms(60~120 samples/per second)  
    
    //TP idle time
    int  tpIdleTime;		//sleep time for polling INT signal(even if interrupt mode).    
    int  tpIdleTimeB4Init;	//sleep time if TP not initial yet.    
    int  tpReadChipRegCnt;	//read register count for getting touch xy coordination
    int  tpMoveDetectUnit;	//read register count for getting touch xy coordination
    
    //TP specific function
    char tpHasPowerOnSeq;	//0:NO power-on sequence, 1:TP has power-on sequence
    char tpNeedProgB4Init;	//0:TP IC works well without programe flow, 1:TP IC need program before operation.
    char tpNeedAutoTouchUp;
    char tpIntPullEnable;	//use internal pull up/down function    
} TP_SPEC;

typedef struct it7236_fw_tag {
	uint8_t verBuf[IT7260_FW_VERSION_SIZE];
	uint8_t bufLen;
}TP_FW_INFO;
/***************************
 * global variable
 **************************/
static struct ts_sample g_sample[MAX_FINGER_NUM];
static struct ts_sample gTmpSmp[10];

static char g_TouchDownIntr = false;
static char g_IsTpInitialized = false;
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

static TP_FW_INFO gFwInfo;
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

static void* _tpProbeHandler(void* arg);
static int  _tpProbeSample(struct ts_sample *samp, int nr);

/***********************************************************************************
 * IT7260's MACRO defination
 ***********************************************************************************/
#define SET_IT7260_ENABLE_INTERRUPT			0x01
#define SET_IT7260_DISABLE_INTERRUPT		0x00

#define SET_IT7260_INTR_HIGH_LEVEL_TRIGGER	0x00
#define SET_IT7260_INTR_LOW_LEVEL_TRIGGER	0x01
#define SET_IT7260_INTR_FALL_EDGE_TRIGGER	0x10	//only support after FW version 1.0.0.7. If developer want to use falling edge
#define SET_IT7260_INTR_RISE_EDGE_TRIGGER	0x11	//only support after FW version 1.0.0.7. If developer want to use rising edge

/*##################################################################################
 *                        the private function for IT7260 ONLY
 ###################################################################################*/
/******************************************************************************
 * the read flow for reading the IT7260's register by using iic repead start
 ******************************************************************************/
static int _readChipReg(int fd, unsigned char regAddr, unsigned char *dBuf, unsigned char dLen)
{
	ITPI2cInfo evt;
	unsigned char	I2cCmd;
	int 			i2cret;
	
	#ifdef	ENABLE_TOUCH_IIC_DBG_MSG
	printf("	RdIcReg(fd=%x, reg=%x, buf=%x, len=%x)\n", fd, regAddr, dBuf, dLen);
	#endif
	
	I2cCmd = regAddr;	//1000 0010		
	evt.slaveAddress   = TP_I2C_DEVICE_ID;
	evt.cmdBuffer      = &I2cCmd;
	evt.cmdBufferSize  = 1;
	evt.dataBuffer     = dBuf;
	evt.dataBufferSize = dLen;	
	
	i2cret = read(fd, &evt, 1);
		
	if(i2cret<0)
	{
		printf("[TOUCH ERROR].iic read fail\n");
		return -1;		
	}
	
	return 0;
}

/******************************************************************************
 * to read the Query Buffer by reading the register "0x80"
 ******************************************************************************/
static int _readQueryBuffer(int fd, unsigned char *Qbuf)
{
	ITPI2cInfo 		evt;
	unsigned char	I2cCmd;
	int 			i2cret;
	int				cnt=0;
	
	#ifdef	ENABLE_TOUCH_IIC_DBG_MSG
	printf("	RdQrybuf,fd=%x, qbuf=%x\n",fd,Qbuf);
	#endif	
	
	do
	{
		usleep(10);
		i2cret = _readChipReg(fd, 0x80, Qbuf, 1);
		if(i2cret<0)
		{
			printf("[TOUCH ERROR].Read Query buffer fail\n");
			return -1;
		}
		
		if( !g_IsTpInitialized )
		{
			cnt++;
			
			if(*Qbuf&0x01)	printf("initial IT7260 fail, read query buffer is busy, cnt=%d, Qbuf=%x\n", cnt, *Qbuf);
			
			if(cnt>10)	
			{
				printf("initial IT7260 fail, read query buffer over 10 times, cnt =%d, Qbuf=%x\n", cnt, *Qbuf);
				return -1;
			}
		}	
		
		usleep(100);
	}while(*Qbuf&0x01);	//IT7260 is still busy
	
	return 0;
}

/******************************************************************************
 * to read the Point Buffer by reading the register "0xE0"
 ******************************************************************************/
static int _readPointBuffer(int fd, unsigned char *Pbuf)
{
	ITPI2cInfo evt;
	unsigned char	I2cCmd;
	int 			i2cret;
	
	#ifdef	ENABLE_TOUCH_IIC_DBG_MSG
	printf("	RdPibuf,%x,%x\n",fd,Pbuf);
	#endif
	
	i2cret = _readChipReg(fd, 0xE0, Pbuf, 14);
	if(i2cret<0)
	{
		printf("[TOUCH ERROR].Read Point buffer fail\n");
		return -1;
	}

	return 0;
}

/******************************************************************************
 * to read the command response by reading the register "0xA0"
 ******************************************************************************/
static int _readCmdResponse(int fd, unsigned char *rbuf, unsigned char len)
{
	ITPI2cInfo 		evt;
	unsigned char	I2cCmd;
	int 			i2cret;
	
	#ifdef	ENABLE_TOUCH_IIC_DBG_MSG
	printf("	RdCmdResponse,%x\n",fd);
	#endif
	
	i2cret = _readChipReg(fd, 0xA0, rbuf, len);
	if(i2cret<0)
	{
		printf("[TOUCH ERROR].Read command Response fail\n");
		return -1;
	}
	
	return 0;
}

/******************************************************************************
 * the command flow of writing command buffer "0x20"
 * It's referenced from the BU4's IT7260 debug tool
 ******************************************************************************/
static int _cmd(int fd, unsigned char *wBuf, unsigned char  wLen, unsigned char *rBuf, unsigned char  rLen)
{
	ITPI2cInfo 		evt;
	unsigned char	Qbuf;
	int 			i2cret;
	
	#ifdef	ENABLE_TOUCH_IIC_DBG_MSG
	printf("	_cmd: fd=%x, wbuf[1]=%x, wLen=%x\n", fd, wBuf[1], wLen);
	#endif
	
	//check query buffer
	do
	{
		i2cret = _readQueryBuffer(fd, &Qbuf);
		if(i2cret<0)
		{
			printf("[TOUCH ERROR]._cmd() fail.01\n");
			return -1;
		}	
	}while(Qbuf&0x01);

	//send command register 0x20
	evt.slaveAddress   = TP_I2C_DEVICE_ID;
	evt.cmdBuffer      = wBuf;
	evt.cmdBufferSize  = wLen;
	evt.dataBuffer     = rBuf;
	evt.dataBufferSize = 0;	
		
	i2cret = write(fd, &evt, 1);

	#ifdef	ENABLE_TOUCH_INTR
	while(!_checkTouchIntr())
	{
		usleep(100);
	}
	#endif
	
	usleep(200);
	
	if(i2cret<0)
	{
		printf("[TOUCH ERROR]._cmd() fail.02\n");
		return -1;		
	}	
	if(wBuf[1]==0x0C)
	{
		#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
		printf("DO RESET COMMAND(wait for 50ms)\n");
		#endif
		usleep(50000);
	}
	
	//check query buffer
	do
	{
		i2cret = _readQueryBuffer(fd, &Qbuf);
		if(i2cret<0)
		{
			printf("[TOUCH ERROR]._cmd() fail.03\n");
			return -1;
		}
	}while(Qbuf&0x01);
	
	//read command response
	i2cret = _readCmdResponse(fd, rBuf, rLen);
	if(i2cret<0)
	{
		printf("[TOUCH ERROR]._cmd() fail.04\n");
		return -1;		
	}
	usleep(200);
	
	return 0;
}

/******************************************************************************
 * send the command "0x07" to reset queue
 ******************************************************************************/
static int _cmdResetQueue(int fd)
{
	unsigned char	cmdBuf[2]={0x20,0x07};
	unsigned char	rBuf[2];
	int 			i2cret;
	
	i2cret = _cmd(fd, cmdBuf, 2, rBuf, 2);
	if(i2cret<0)
	{
		printf("tchRd.err.ResetQueue.01:\n");
		return -1;		
	}
	
	if( (rBuf[0]) || (rBuf[1]) )
	{
		printf("tchRd.err.ResetQueue.02:buf[%x,%x]\n",rBuf[0],rBuf[1]);
		return -1;		
	}
	
	return 0;
}

/******************************************************************************
 * send the command "0x01 0x04" to read the interrupt information
 ******************************************************************************/
static int _cmdReadIntrInfo(int fd, unsigned char *ibuf)
{
	unsigned char	cmdBuf[3]={0x20, 0x01, 0x04};
	int 			i2cret;
	
	i2cret = _cmd(fd, cmdBuf, 3, ibuf, 2);
	if(i2cret<0)
	{
		printf("tchRd.err.rdIntrInfo.01:");
		return -1;		
	}
	
	return 0;
}

/******************************************************************************
 * send the command "0x02 0x04" to set the interrupt information
 ******************************************************************************/
static int _cmdSetIntrInfo(int fd, unsigned char mode1, unsigned char mode2)
{
	unsigned char	cmdBuf[5]={0x20, 0x02, 0x04, 0x00, 0x00};
	unsigned char	ibuf[2];
	int 			i2cret;
	
	cmdBuf[3] = mode1;
	cmdBuf[4] = mode2;
	
	i2cret = _cmd(fd, cmdBuf, 5, ibuf, 2);
	if(i2cret<0)
	{
		printf("tchRd.err.SetIntrInfo.01:\n");
		return -1;		
	}

	if( (ibuf[0]) || (ibuf[1]) )
	{
		printf("tchRd.err.SetIntrInfo.02:\n",ibuf[0],ibuf[1]);
		return -1;		
	}
	
	return 0;
}


void _identifyTouchChip(int fd)
{
	//TODO:	
	
}

/******************************************************************************
 * send re-initial command "0x0C"
 ******************************************************************************/
static int _cmdReinitialIt7260(int fd)
{
	unsigned char	cmdBuf[2]={0x20,0x0C};
	unsigned char	rBuf[2]={0xFF,0xFF};
	int 			i2cret;
	
	while(1)
	{
		i2cret = _cmd(fd, cmdBuf, 2, rBuf, 2);
		if(i2cret<0)
		{
			printf("tchRd.err._cmdReinitialIt7260.01:\n");
			continue;
		}
		
		if( (!rBuf[0]) && (!rBuf[1]) )
		{
			#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
			printf("_cmdReinitialIt7260() success\n");
			#endif
			break;		
		}
	}
	
	return 0;
}

/******************************************************************************
 * get 2D resolution command "0x01 0x20 0x00"
 ******************************************************************************/
static int _cmdGet2DResolution(int fd, unsigned short *xRes, unsigned short *yRes)
{
	unsigned char	cmdBuf[4]={0x20, 0x01, 0x02, 0x00};
	unsigned char	ibuf[16];
	int 			i2cret;
	
	i2cret = _cmd(fd, cmdBuf, 4, ibuf, 11);
	if(i2cret<0)
	{
		printf("tchRd.err._cmdGet2DResolution.01:\n");
		return -1;		
	}
	
	if(xRes != NULL)
	{
		*xRes = ibuf[2] + (ibuf[3] << 8);
	}
	
	if(yRes!= NULL)
	{
		*yRes = ibuf[4] + (ibuf[5] << 8);
	}

	return 0;
}

/******************************************************************************
 * get F/W version informantion command "0x01 0x00"
 ******************************************************************************/
static int _cmdGetFwInfo(int fd, TP_FW_INFO* fwInfo)
{
	unsigned char	cmdBuf[3]={0x20, 0x01, 0x00};
	int 			i2cret;
	
	gFwInfo.bufLen = (uint8_t)IT7260_FW_VERSION_SIZE;
	i2cret = _cmd(fd, cmdBuf, 3, fwInfo->verBuf, IT7260_FW_VERSION_SIZE);
	if(i2cret<0)
	{
		printf("tchRd.err._cmdGetFwInfo:\n");
		return -1;		
	}
	else
	{
		int i;
		printf("IT7260 FW version:(");
		for(i=5; i<IT7260_FW_VERSION_SIZE; i++)		printf("%02x,",fwInfo->verBuf[i]);
		printf(")\n");
	}
	
	return 0;
}

/******************************************************************************
 * It's for setting the interrupt mode of IT7260(command::0x20 0x02 0x04 0x10 0x00)
 * 1.0x20 : command buffer address
 * 2.0x02 : Set Cap Sensor Information
 * 3.0x04 : Interrupt Notification Status
 * 4.0x01 : Enable interrupt
 * 5.0x00 : Low level trigger
 *
 * To see page 38 of IT7260's programming guide
 * the setting flow is referenced from BU4's IT7260 debug tool
 ******************************************************************************/
static int _setChip_IntrInfo(int fd, unsigned char intrMode, unsigned char activeMode)
{
	int i2cret;
	unsigned char infoBuf[4];
	
	//reset queue (0x07)
	i2cret = _cmdResetQueue(fd);
	if(i2cret<0)
	{
		printf("tchRd.err.setChip.01:\n");
		return -1;
	}
	
	//read Intr Info (0x01 0x04)
	i2cret = _cmdReadIntrInfo(fd, infoBuf);
	if(i2cret<0)
	{
		printf("tchRd.err.setChip.02:\n");
		return -1;
	}

	//reset queue (0x07)
	i2cret = _cmdResetQueue(fd);
	if(i2cret<0)
	{
		printf("tchRd.err.setChip.03:\n");
		return -1;
	}
	
	//set intr Info (0x02 0x04 0x01 0x00)
	i2cret = _cmdSetIntrInfo(fd, intrMode, activeMode);
	if(i2cret<0)
	{
		printf("tchRd.err.setChip.04:\n");
		return -1;
	}
	
	//reset queue (0x07)
	i2cret = _cmdResetQueue(fd);
	if(i2cret<0)
	{
		printf("tchRd.err.setChip.05:\n");
		return -1;
	}
	
	//read Intr Info (0x01 0x04)
	i2cret = _cmdReadIntrInfo(fd, infoBuf);
	if(i2cret<0)
	{
		printf("tchRd.err.setChip.06:\n");
		return -1;
	}
	if( (infoBuf[0]!=intrMode) || (infoBuf[1]!=activeMode) )
	{
		printf("tchRd.err.setChip.06:[%x,%x]\n",infoBuf[0],infoBuf[1]);
		//return -1;
	}
	
	//reset queue (0x07)
	i2cret = _cmdResetQueue(fd);
	if(i2cret<0)
	{
		printf("tchRd.err.setChip.07:\n");
		return -1;
	}
	return 0;
}

/*##################################################################################
 *                        the private function(above) for IT7260 ONLY
 ###################################################################################*/

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
    gTpSpec.tpIntTriggerType    = (char)TP_INT_EDGE_TRIGGER; //from Kconfig setting   level/edge
    
    gTpSpec.tpInterface         = (char)TP_INTERFACE_I2C;	  //from Kconfig setting
    gTpSpec.tpIntActiveMaxIdleTime = (char)TP_SAMPLE_RATE;	  //from Kconfig setting
        
    gTpSpec.tpMaxRawX           = (int)CFG_TOUCH_X_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpMaxRawY           = (int)CFG_TOUCH_Y_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpScreenX           = (int)TP_SCREEN_WIDTH;       //from Kconfig setting
    gTpSpec.tpScreenY           = (int)TP_SCREEN_HEIGHT;      //from Kconfig setting
    
    gTpSpec.tpCvtSwapXY        = (char)TP_SWAP_XY;            //from Kconfig setting
    gTpSpec.tpCvtReverseX      = (char)TP_REVERSE_X;          //from Kconfig setting
    gTpSpec.tpCvtReverseY      = (char)TP_REVERSE_Y;          //from Kconfig setting
    gTpSpec.tpCvtScaleX        = (char)0;                    //from Kconfig setting
    gTpSpec.tpCvtScaleY        = (char)0;                    //from Kconfig setting
    
    gTpSpec.tpI2cDeviceId       = (char)TP_I2C_DEVICE_ID;	  //from this driver setting
    gTpSpec.tpEnTchPressure     = (char)0;                    //from this driver setting
    gTpSpec.tpSampleNum         = (char)1;                    //from this driver setting
    gTpSpec.tpSampleRate        = (char)TP_SAMPLE_RATE;       //from this driver setting
    gTpSpec.tpIntrType          = (char)TP_INT_TYPE_IT7260;	  //from this driver setting
    gTpSpec.tpHasTouchKey       = (char)TP_WITHOUT_KEY;       //from this driver setting                                                               
    gTpSpec.tpIdleTime          = (int)TP_IDLE_TIME;          //from this driver setting
    gTpSpec.tpIdleTimeB4Init    = (int)TP_IDLE_TIME_NO_INITIAL;//from this driver setting 
    gTpSpec.tpMoveDetectUnit    = (int)1;
    gTpSpec.tpReadChipRegCnt    = (int)14;
    
    //special initial flow
    gTpSpec.tpHasPowerOnSeq     = (char)0;
    gTpSpec.tpNeedProgB4Init    = (char)1;    
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
	int i=0;
	int i2cret;
		
	//identify FW
	//TODO:
	//_identifyTouchChip(gTpInfo.tpDevFd);		
	
	//get 2D resolution
	//_cmdGet2DResolution(gTpInfo.tpDevFd, x, y);
	
	while(i++<3)
	{
		i2cret = _setChip_IntrInfo(gTpInfo.tpDevFd, SET_IT7260_ENABLE_INTERRUPT, SET_IT7260_INTR_HIGH_LEVEL_TRIGGER);
		if(i2cret==0)
		{
			break;
		}
		usleep(1000);
	}
	
	/* to get FW info */
	if( _cmdGetFwInfo(gTpInfo.tpDevFd, &gFwInfo) )
	{
	    printf("[IT7260 warning]::Can NOT get the IT7260 FW version!!\n");
	    return -1;
	}

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
	int i2cret;
	int real_nr=0;
	unsigned char qbuf;
	
	while( _readQueryBuffer(gTpInfo.tpDevFd, &qbuf)<0 );
	
	if(qbuf&0x80)
	{		
		i2cret = _readPointBuffer(gTpInfo.tpDevFd,buf);
		if(i2cret<0)
		{
   			printf("tch.err.read_fail.1,qbuf=%x\n",buf[0]);
			return -1;
		}
	}	
	else
	{
  		printf("tch.err.read_fail.2,qbuf=%x\n",buf[0]);
  		return -1;
	}
	return 0;
}                               

static int _tpParseRawPxy_vendor(struct ts_sample *s, unsigned char *buf)
{
	if( ((buf[0]&0xF0)==0x00) && (buf[0]&0x08) )
	{
	    int nr = 1;
    	int i = 0;
    	struct ts_sample *samp=s;
    	
    	while(i<nr)
    	{
    		if(buf[0] & (1 << i))
    		{
    			samp->x =  ((int)(buf[i * 4 + 3] & 0x0F) << 8) + (int)buf[i * 4 + 2];
    			samp->y =  ((int)(buf[i * 4 + 3] & 0xF0) << 4) + (int)buf[i * 4 + 4];
    			samp->pressure = 1;
    			//printf("	##-Raw x,y = %d, %d ::",tmpX,tmpY);		
    		}
    		i++;    
    		samp++;
    	}
        return 0;
	}
	else
	{
		//to keep and report the lasted touch sample
		pthread_mutex_lock(&gTpMutex);
		memcpy((void *)s, (void *)&g_sample, sizeof(struct ts_sample)*1);
		pthread_mutex_unlock(&gTpMutex);
		
		if( (buf[0]==0) && (buf[1]==1) )
		{
			//printf("it's a palm case~~\n");	//ignore it
		}
		
		if( (buf[0]==0) && (buf[1]==0) )
		{
			//printf("it's a touch-up case~~\n");	//ignore it			
		}

		#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
		printf("	P3.Buf[0~4]=[%x,%x,%x,%x,%x], last xy=[%d,%d]\n",buf[0],buf[1],buf[2],buf[3],buf[4],s->x,s->y);
		#endif
		return 0;
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
static int it7260_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
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

static const struct tslib_ops it7260_ops =
{
	it7260_read,
};

TSAPI struct tslib_module_info *it7260_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &it7260_ops;
	return m;
}

#ifndef TSLIB_STATIC_CASTOR3_MODULE
	TSLIB_MODULE_INIT(it7260_mod_init);
#endif
