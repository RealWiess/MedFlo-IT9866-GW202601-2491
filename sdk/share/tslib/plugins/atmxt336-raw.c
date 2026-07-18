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
#define TP_GPIO_RESET_PIN	TP_GPIO_PIN_NO_DEF
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
#define	MAX_FINGER_NUM	(2)		//depend on TP Native Max Finger Numbers  
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
 * MACRO define of ATMXT336UD
 ****************************************************************************/
#ifdef	TP_I2C_ID_KCONFIG
#define TP_I2C_DEVICE_ID       (TP_I2C_ID_KCONFIG)
#else
#define TOUCH_DEVICE_ID1      (0x4A)
#define TP_I2C_DEVICE_ID       TOUCH_DEVICE_ID1
#endif

#define TP_SAMPLE_RATE	(33)

#ifdef	CFG_LCD_ENABLE
#define	TP_SCREEN_WIDTH	    ithLcdGetWidth()
#define	TP_SCREEN_HEIGHT    ithLcdGetHeight()
#else
#define	TP_SCREEN_WIDTH	    800
#define	TP_SCREEN_HEIGHT	480
#endif

#define GTP_POLL_TIME         10
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_MAX_LENGTH 186
#define FAIL                  0
#define SUCCESS               1

/* Object types */
#define MXT_DEBUG_DIAGNOSTIC_T37	37
#define MXT_GEN_MESSAGE_T5		5
#define MXT_GEN_COMMAND_T6		6
#define MXT_GEN_POWER_T7		7
#define MXT_GEN_ACQUIRE_T8		8
#define MXT_GEN_DATASOURCE_T53		53
#define MXT_TOUCH_MULTI_T9		9
#define MXT_TOUCH_KEYARRAY_T15		15
#define MXT_TOUCH_PROXIMITY_T23		23
#define MXT_TOUCH_PROXKEY_T52		52
#define MXT_PROCI_GRIPFACE_T20		20
#define MXT_PROCG_NOISE_T22		22
#define MXT_PROCI_ONETOUCH_T24		24
#define MXT_PROCI_TWOTOUCH_T27		27
#define MXT_PROCI_GRIP_T40		40
#define MXT_PROCI_PALM_T41		41
#define MXT_PROCI_TOUCHSUPPRESSION_T42	42
#define MXT_PROCI_STYLUS_T47		47
#define MXT_PROCG_NOISESUPPRESSION_T48	48
#define MXT_SPT_COMMSCONFIG_T18		18
#define MXT_SPT_GPIOPWM_T19		19
#define MXT_SPT_SELFTEST_T25		25
#define MXT_SPT_CTECONFIG_T28		28
#define MXT_SPT_USERDATA_T38		38
#define MXT_SPT_DIGITIZER_T43		43
#define MXT_SPT_MESSAGECOUNT_T44	44
#define MXT_SPT_CTECONFIG_T46		46
#define MXT_SPT_TIMER_T61			61
#define MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71 71
#define MXT_PROCI_SYMBOLGESTUREPROCESSOR	92
#define MXT_PROCI_TOUCHSEQUENCELOGGER	93
#define MXT_TOUCH_MULTITOUCHSCREEN_T100 100
#define MXT_PROCI_ACTIVESTYLUS_T107	107

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

uint8_t I2C_application_address = 0x4A; //Need change to 336UD address
uint8_t ID_information[7] = {0xA6, 0x14, 0x10, 0xAA, 0x0E, 0x18, 0x1F}; 
uint8_t Txseq_num = 0;

uint8_t T5_address[2] = {0x48, 0x01};
uint8_t T6_address[2];
uint8_t T7_address[2];
uint8_t T44_address[2];
uint8_t T61_address[2];
uint8_t T100_address[2] = {0x56, 0x05};

uint8_t T5_message_size = 12;

uint8_t T6_reportid;
uint8_t T15_reportid_min;
uint8_t T15_reportid_max;
uint8_t T15_reportid_number;
uint8_t T61_reportid_min;
uint8_t T61_reportid_max;
uint8_t T61_reportid_number;
uint8_t T100_reportid_min = 0x23;
uint8_t T100_reportid_max = 0x2E;
uint8_t T100_reportid_number = 0x0A;

uint8_t ID_information_address[2] = {0x00, 0x00};

/*
  ATMXT336UD NOTE:
  2020/11/3: ONLY single touch CAN WORK, multi-touch is NOT work with the current driver
             Because one INTr can only get one finger's X/Y data
             It need to get multi-fingers's X/Y with multi-INTr(current ITE TP driver can not handle this condition)
             _tpDoInitProgram_vendor() is NOT necessary to read the ID infomation registers
             Just Read data-buffer to make the INT pull-high
*/
/*##################################################################################
 *                        the private function implementation
 ###################################################################################*/

void I2C_readNBytes(uint8_t cmd, uint8_t *buf, uint8_t len)
{
	int i2cret;
	ITPI2cInfo *evt;

	evt = alloca(sizeof(*evt));
	evt->slaveAddress   = TP_I2C_DEVICE_ID;
	evt->cmdBuffer      = &cmd;
	evt->cmdBufferSize  = 0;
	evt->dataBuffer     = buf;
	evt->dataBufferSize = (uint32_t)len;	
	i2cret = read(gTpInfo.tpDevFd, evt, 1);	
}

void I2C_writeNBytes(uint8_t cmd, uint8_t *buf, uint8_t len)
{
	int i2cret;
	ITPI2cInfo *evt;

	evt = alloca(sizeof(*evt));
	evt->slaveAddress   = TP_I2C_DEVICE_ID;
	evt->cmdBuffer      = buf;
	evt->cmdBufferSize  = (uint32_t)len;
	evt->dataBuffer     = buf;
	evt->dataBufferSize = 0;		
	i2cret = write(gTpInfo.tpDevFd, evt, 1);	
}

uint8_t mxt_calc_crc8(unsigned char crc, unsigned char data)
{
	static const uint8_t crcpoly = 0x8C;
	uint8_t index;
	uint8_t fb;
	index = 8;
	
	do {
		fb = (crc ^ data) & 0x01;
		data >>= 1;
		crc >>= 1;
		if (fb)
		crc ^= crcpoly;
	} while (--index);
	
	return crc;
}

uint8_t mxt_update_seq_num(bool reset_counter)
{
	uint8_t current_val;

	current_val = Txseq_num;

	if (Txseq_num == 0xFF || reset_counter) {
		Txseq_num = 0;
		} else {
		Txseq_num++;
	}
	return current_val;
}	

void mxt_write_Nbytes(uint8_t I2C_address, uint8_t *data, uint8_t lens)
{
	I2C_writeNBytes(I2C_address, data, lens);
}

bool mxt_read_Nbytes(uint8_t I2C_address, uint8_t *Memory_address, uint8_t *data, uint8_t lens)
{
	uint8_t buf[4];
	uint8_t count;
	uint8_t crc_data = 0;
	
	//Set address pointer
	count = 4;	//16bit addr, tx_seq_num, 8bit crc
	
	buf[0] = Memory_address[0];
	buf[1] = Memory_address[1];
	buf[2] = mxt_update_seq_num(false);
	for (int i = 0; i < (count-1); i++) {
		crc_data = mxt_calc_crc8(crc_data, buf[i]);
	}
	buf[3] = crc_data;
	//printf("mxtI2cWt: %02x,%02x,%02x,%02x, c=%x\n",buf[0],buf[1],buf[2],buf[3],count);

	I2C_writeNBytes(I2C_address, buf, count);
	for(uint16_t j = 0; j < 2000; j++)
	{
		;
	}		
	I2C_readNBytes(I2C_address, data, lens);
	crc_data = 0;
	for (int i = 0; i < (lens - 1); i++){
		crc_data = mxt_calc_crc8(crc_data, data[i]);
	}

	if (crc_data == data[lens - 1]){
		return true;//T5 Read CRC Passed
	}
	else {
		return false; //T5 Read CRC Failed
	}
}

void mxt_process_t100_message(uint8_t * message)
{
	uint8_t touchscreen_reportid;
	uint8_t touchscreen_status;
	uint8_t finger_id;
	uint8_t finger_event;
	uint16_t position_x;
	uint16_t position_y;
	
	touchscreen_reportid = message[0];
	if(touchscreen_reportid == T100_reportid_min)
	{
		//first report id
	}
	else if(touchscreen_reportid == (T100_reportid_min + 1))
	{
		//second report id
	}
	else if(touchscreen_reportid > (T100_reportid_min + 1))
	{
		touchscreen_status = message[1];
		position_x = message[2] + (message[3]<<8);
		position_y = message[4] + (message[5]<<8);
		if(touchscreen_status&0x80)
		{
			if(touchscreen_status&0x10)
			{
				finger_id = touchscreen_reportid - T100_reportid_min - 2;
				finger_event = message[1]&0x0F;
				if(finger_event == 0x01)
				{
					//report move event
				}
				else if(finger_event == 0x04)
				{
					//report down event
				}
			}
		}
		else
		{
			if(touchscreen_status&0x10)
			{
				finger_id = touchscreen_reportid - T100_reportid_min - 2;
				finger_event = message[1]&0x0F;
				if(finger_event == 0x05)
				{
					//report up event
				}
			}
		}
	}
}

void mxt_read_messages(void)
{
	uint8_t reportid_tmp;
	uint8_t message[64];

	mxt_read_Nbytes(I2C_application_address, T5_address, message, T5_message_size);
	reportid_tmp = message[0];	

	//printf("mxtRdMsg: msgSz = %d, T5Addr = %x, %x, m[0] = %x \n",T5_message_size, T5_address[0], T5_address[1], message[0]);
	//printf("mxtRdMsg: reportid_tmp = %d, T100_reportid_min = %x, T100_reportid_max = %x\n",reportid_tmp, T100_reportid_min, T100_reportid_max);
	if((reportid_tmp >= T100_reportid_min) && (reportid_tmp <= T100_reportid_max))
	{
		mxt_process_t100_message(message);
	}
}

/*##################################################################################
 *                        the private function implementation
 ###################################################################################*/
static void _tpInitSpec_vendor(void)
{
    gTpSpec.tpIntPin          	= (char)TP_INT_PIN;           //from Kconfig setting
    gTpSpec.tpWakeUpPin         = (char)TP_GPIO_PIN_NO_DEF;   //from Kconfig setting
    gTpSpec.tpResetPin          = (char)TP_GPIO_RESET_PIN;   //from Kconfig setting
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
    gTpSpec.tpSampleNum         = (char)MAX_FINGER_NUM;       //from this driver setting
    gTpSpec.tpSampleRate        = (char)TP_SAMPLE_RATE;       //from this driver setting
    gTpSpec.tpIntrType          = (char)TP_INT_TYPE_IT7260;	  //from this driver setting
    gTpSpec.tpHasTouchKey       = (char)TP_WITHOUT_KEY;       //from this driver setting                                                               
    gTpSpec.tpIdleTime          = (int)TP_IDLE_TIME;          //from this driver setting
    gTpSpec.tpIdleTimeB4Init    = (int)TP_IDLE_TIME_NO_INITIAL;//from this driver setting    
    gTpSpec.tpMoveDetectUnit    = (int)1;
#ifdef  TP_MULTI_FINGER_ENABLE
    gTpSpec.tpReadChipRegCnt    = (int)60;
#else
    gTpSpec.tpReadChipRegCnt    = (int)12;
#endif
    
    //special initial flow
    gTpSpec.tpHasPowerOnSeq     = (char)1;
    gTpSpec.tpNeedProgB4Init    = (char)1;    
    gTpSpec.tpNeedAutoTouchUp	= (char)0; 
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
	//for Atmxt336 power on methods.	
	char rstPin = (char)TP_GPIO_RESET_PIN;

    printf("DO POWER-ON sequence, reset pin:%d\n",rstPin);	
    if(rstPin == (char)-1) return;
    
	//1.set "Reset pin" & "INT pin" are output-low for 1ms	
	ithGpioSetMode( rstPin, ITH_GPIO_MODE0);
	ithGpioClear(rstPin);
	ithGpioSetOut(rstPin);
	ithGpioEnable(rstPin);	
	
	usleep(10*1000);

    //2.set "Reset pin" output HIGH for 150ms
	ithGpioSet(rstPin);
	usleep(150*1000);
}

static int _tpDoInitProgram_vendor(void)
{
	uint8_t buf = 0x4;
	uint8_t writeCmd[2] = { 0x00, 0x00 };	//ID info address
	uint8_t tmp[8];       
	int rst = 0;
	int ret;
	
	uint8_t ID_information_tmp[7];
	uint8_t Object_table_number;
	
	/*Read Object Table*/	
	uint16_t memory_address_16u;
	uint8_t memory_address_8u[2];
	uint8_t Object_table[6];
	uint8_t report_id = 1;
	uint8_t min_id = 0;
	uint8_t max_id;
	
	Txseq_num = 0;
	/*Read ID Information and Compare*/
	mxt_read_Nbytes(I2C_application_address, ID_information_address, ID_information_tmp, 7);
	for(int i = 0; i < sizeof(ID_information_tmp); i++)
	{
		if(ID_information_tmp[i] != ID_information[i])
		{
			printf("check ID info Error: tmp[%x]=%x , inf[%x]=%x\n",i,ID_information_tmp[i],i,ID_information[i]);
			ret = 1;
		}
	}
	
	//printf("init program Flow, num = %x\n",ID_information_tmp[6]);
    {
    	int k=0;
    	printf("## ID_information :: ");
    	for(k=0; k<7; k++)	printf("%02X ",ID_information_tmp[k]);
    	printf("\n\n");
    }
    Object_table_number = ID_information_tmp[6];

	for(int i = 0; i < Object_table_number; i++)
	{
		memory_address_16u = 7 + i*6;
		memory_address_8u[0] = (uint8_t) memory_address_16u;
		memory_address_8u[1] = (uint8_t) memory_address_16u >> 8;
		mxt_read_Nbytes(I2C_application_address, memory_address_8u, Object_table, 6);
		/*
	    {
	    	int k=0;
	    	printf("## i=%x, ,addr=%x OBJ[0~5]:: ", i, memory_address_16u);
	    	for(k=0; k<6; k++)	printf("%02X ",Object_table[k]);
	    	printf("\n\n");
	    }
	    */
		
		if(Object_table[5] > 0)
		{
			min_id = report_id;
			report_id += Object_table[5]*(Object_table[4] + 1);
			max_id = report_id -1;
		}
		else
		{
			min_id = 0;
			max_id = 0;
		}
		//printf("init program Flow 2: obj[0] = %x, on=%x\n", Object_table[0], Object_table_number);
		switch (Object_table[0])
		{
			case MXT_GEN_MESSAGE_T5:
				T5_message_size = Object_table[3] + 1;
				T5_address[0] = Object_table[1];
				T5_address[1] = Object_table[2];
				break;
			case MXT_GEN_COMMAND_T6:
				T6_reportid = min_id;
				T6_address[0] = Object_table[1];
				T6_address[1] = Object_table[2];
				break;
			case MXT_GEN_POWER_T7:
				T7_address[0] = Object_table[1];
				T7_address[1] = Object_table[2];
				break;
			case MXT_TOUCH_KEYARRAY_T15:
				T15_reportid_min = min_id;
				T15_reportid_max = max_id;
				T15_reportid_number = Object_table[5];
				break;
			case MXT_SPT_MESSAGECOUNT_T44:
				T44_address[0] = Object_table[1];
				T44_address[1] = Object_table[2];
				break;
			case MXT_SPT_TIMER_T61:
				T61_reportid_min = min_id;
				T61_reportid_max = max_id;
				T61_reportid_number = Object_table[5];
				T61_address[0] = Object_table[1];
				T61_address[1] = Object_table[2];
				break;
			case MXT_TOUCH_MULTITOUCHSCREEN_T100:
				T100_reportid_min = min_id;
				T100_reportid_max = max_id;
				T100_reportid_number = Object_table[5] - 2;
				T100_address[0] = Object_table[1];
				T100_address[1] = Object_table[2];
				//printf("i=%d, T100_reportid_min = %x\n",i,T100_reportid_min);
				break;
		}
	}
	//printf("Read point in programFlow\n");
	mxt_read_messages();
	
	g_TouchDownIntr = 0;
	ithGpioEnableIntr(TP_INT_PIN); 

    return ret;
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
    bool recv = false;
	uint8_t reportid_tmp;
	uint8_t message[64];
	static uint8_t last_buf[64];
	static uint8_t tpDownUpEvent = 0;	

	memset(message, 0, cnt);

	mxt_read_Nbytes(I2C_application_address, T5_address, message, T5_message_size);
	reportid_tmp = message[0];	

	//printf("mxtRdMsg: msgSz = %d, T5Addr = %x, %x, m[0] = %x \n",T5_message_size, T5_address[0], T5_address[1], buf[0]);
	//printf("mxtRdMsg: reportid_tmp = %d, T100_reportid_min = %x, T100_reportid_max = %x\n",reportid_tmp, T100_reportid_min, T100_reportid_max);
	//if((reportid_tmp >= T100_reportid_min) && (reportid_tmp <= T100_reportid_max))
	if((reportid_tmp >= 0x25) && (reportid_tmp <= 0x25))
	{
		//memcpy(buf, buf, T5_message_size);
		if(message[1] & 0x80)
		{					
			memcpy(buf, message, cnt);
			memcpy(last_buf, message, cnt);
			//if(tpDownUpEvent == 0)	printf("	--\\Dn\n");
								
			tpDownUpEvent = 1;		
		}                     
		
		if(message[1]==0x15)
		{
			//printf("	__/UP\n");			
			memcpy(buf, message, cnt);
			memset(last_buf, 0, cnt);
			tpDownUpEvent = 0;
		}                         

		if(tpDownUpEvent == 1)	recv = true;
	}
	else
	{
		if(tpDownUpEvent == 1)
		{
			memcpy(buf, last_buf, cnt);
		}
		//printf("	--tpDnUpEvt = %x\n",tpDownUpEvent);
		if(tpDownUpEvent == 1)	return 0;
		else					return -1;
	}

    if(recv==true)  return 0;
	else            return -1;
}                               

static int _tpParseRawPxy_vendor(struct ts_sample *smp, unsigned char *buf)
{
	uint8_t touchscreen_reportid;
	uint8_t touchscreen_status;
	uint8_t finger_id;
	uint8_t finger_event;
	uint16_t position_x;
	uint16_t position_y;
	
	touchscreen_reportid = buf[0];
	if(touchscreen_reportid == T100_reportid_min)
	{
		//first report id
		return 1;
	}
	else if(touchscreen_reportid == (T100_reportid_min + 1))
	{
		//second report id
		return 1;
	}
	else if(touchscreen_reportid > (T100_reportid_min + 1))
	{
		touchscreen_status = buf[1];
		position_x = buf[2] + (buf[3]<<8);
		position_y = buf[4] + (buf[5]<<8);

		if(touchscreen_status&0x80)
		{
			if(touchscreen_status&0x10)
			{
				finger_id = touchscreen_reportid - T100_reportid_min - 2;
				finger_event = buf[1]&0x0F;
				if(finger_event == 0x01)
				{
					//report move event
					smp->pressure = 1;
					smp->id = (unsigned int)finger_id;
					smp->finger = 1;
					smp->x = (int)position_x;
					smp->y = (int)position_y;
				}
				else if(finger_event == 0x04)
				{
					//report down event
					smp->pressure = 1;
					smp->id = (unsigned int)finger_id;
					smp->finger = 1;
					smp->x = (int)position_x;
					smp->y = (int)position_y;
				}
			}
			pthread_mutex_lock(&gTpMutex);
			gLastNumFinger = smp->finger;
			pthread_mutex_unlock(&gTpMutex);	
			return 0;
		}
		else
		{
			if(touchscreen_status&0x10)
			{
				finger_id = touchscreen_reportid - T100_reportid_min - 2;
				finger_event = buf[1]&0x0F;
				if(finger_event == 0x05)
				{
			    	//clear this sample & continue
			    	smp->finger = 0;			    
			    	smp->pressure = 0;
			    	smp->x = 0;
			    	smp->y = 0;
				}
				pthread_mutex_lock(&gTpMutex);
				gLastNumFinger = smp->finger;
				pthread_mutex_unlock(&gTpMutex);					
				return 0;
			}
		}		
	}
	//printf("got End p\n");  //it's NOT make sense to be here!!!
	//printf("	RAW->[%d][%x, %d, %d]--> %d %d %d\n", 0, smp, smp->id, smp->finger, smp->pressure, smp->x, smp->y);			
	return (-1);
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
static int atmxt336_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
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

static const struct tslib_ops atmxt336_ops =
{
	atmxt336_read,
};

TSAPI struct tslib_module_info *atmxt336_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &atmxt336_ops;
	return m;
}

#ifndef TSLIB_STATIC_CASTOR3_MODULE
	TSLIB_MODULE_INIT(atmxt336_mod_init);
#endif
