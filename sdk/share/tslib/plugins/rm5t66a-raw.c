/*
 you have to check 3 things before porting TP driver
 1).The INT pin can work normally
 2).The I2C BUS can read data from TP chip
 3).Parse the X,Y coordination correctly

 These functions are customized.
 You Have to modify these functions with "(*)" mark.
 These functions(3&4) could harldy be modified.
 Function(5~7) will be modified deponding on chip's feature.
  0._tpInitSpec_vendor()           //set control config(*)
  1._tpReadPointBuffer_vendor()    //read point buffer(*)
  2._tpParseRawPxy_vendor()        //parse the touch point(*)
  3._tpIntActiveRule_vendor()      //touch-down RULE
  4._tpIntNotActiveRule_vendor()   //touch-up RULE
  5._tpParseKey_vendor()           //depend on TP with key
  6._tpDoPowerOnSeq_vendor();      //depend on TP with power-on sequence
  7._tpDoInitProgram_vendor();     //depend on TP with initial programming
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

#ifdef CFG_TOUCH_MULTI_FINGER
#define TP_MULTI_FINGER_ENABLE
#endif

#define USE_RAW_API
#define TP_USE_XQUEUE
//#define TP_ID_REMAP_DEBUG_MSG

#define ENABLE_MEASURE_US_TIME

#ifdef ENABLE_MEASURE_US_TIME
    #define TP_USE_TIMER (3)
#endif
/****************************************************************************
 * initial Kconfig setting
 ****************************************************************************/

#if defined(CFG_TOUCH_I2C0) || defined(CFG_TOUCH_I2C1) || defined(CFG_TOUCH_I2C2) || defined(CFG_TOUCH_I2C3)
#define TP_INTERFACE_I2C   (0)
#endif

#if defined(CFG_TOUCH_SPI) || defined(CFG_TOUCH_SPI0) || defined(CFG_TOUCH_SPI1)
#define TP_INTERFACE_SPI   (1)
#endif

#define TP_INT_PIN      CFG_GPIO_TOUCH_INT
#define TP_GPIO_MASK    (1<<(TP_INT_PIN%32))

#ifdef  CFG_GPIO_TOUCH_WAKE
#if (CFG_GPIO_TOUCH_WAKE<128)
#define TP_GPIO_WAKE_PIN    CFG_GPIO_TOUCH_WAKE
#endif
#endif

#ifdef  CFG_GPIO_TOUCH_RESET
#if (CFG_GPIO_TOUCH_RESET<128)
#define TP_GPIO_RESET_PIN   CFG_GPIO_TOUCH_RESET
#else
#define TP_GPIO_RESET_PIN   (-1)
#endif
#else
#define TP_GPIO_RESET_PIN   (-1)
#endif

#ifdef  CFG_TOUCH_ADVANCE_CONFIG

#ifdef  CFG_TOUCH_SWAP_XY
#define TP_SWAP_XY      (1)
#else
#define TP_SWAP_XY      (0)
#endif

#ifdef  CFG_TOUCH_REVERSE_X
#define TP_REVERSE_X    (1)
#else
#define TP_REVERSE_X    (0)
#endif

#ifdef  CFG_TOUCH_REVERSE_Y
#define TP_REVERSE_Y    (1)
#else
#define TP_REVERSE_Y    (0)
#endif

#if defined(CFG_TOUCH_I2C_SLAVE_ID) && (CFG_TOUCH_I2C_SLAVE_ID != -1)
#define	TP_I2C_ID_KCONFIG		CFG_TOUCH_I2C_SLAVE_ID
#endif

#else

#define TP_SWAP_XY      (0)
#define TP_REVERSE_X    (0)
#define TP_REVERSE_Y    (0)

#endif

#define TOUCH_NO_CONTACT        (0)
#define TOUCH_DOWN              (1)
#define TOUCH_UP                (2)

#define TP_ACTIVE_LOW           (0)
#define TP_ACTIVE_HIGH          (1)

#ifdef  CFG_GPIO_TOUCH_INT_ACTIVE_HIGH
#define TP_INT_ACTIVE_STATE     TP_ACTIVE_HIGH
#else
#define TP_INT_ACTIVE_STATE     TP_ACTIVE_LOW
#endif

#define TP_INT_LEVLE_TRIGGER    (1)
#define TP_INT_EDGE_TRIGGER     (0)

#define TP_INT_TYPE_KEEP_STATE  (0)
#define TP_INT_TYPE_ZT2083      (0)
#define TP_INT_TYPE_FT5XXX      (1)
#define TP_INT_TYPE_IT7260      (2)

#define TP_WITHOUT_KEY          (0)
#define TP_HAS_TOUCH_KEY        (1)
#define TP_GPIO_PIN_NO_DEF      (-1)

#ifdef  CFG_TOUCH_BUTTON
#define TP_TOUCH_BUTTON     TP_HAS_TOUCH_KEY
#else
#define TP_TOUCH_BUTTON     TP_WITHOUT_KEY
#endif

#ifdef CFG_TOUCH_INTR
#define TP_ENABLE_INTERRUPT     (1)
#else
#define TP_ENABLE_INTERRUPT     (0)
#endif

#ifdef TP_MULTI_FINGER_ENABLE
#define MAX_FINGER_NUM  (5)       //depend on TP Native Max Finger Numbers
#else
#define MAX_FINGER_NUM  (1)
#endif

#ifdef TP_USE_XQUEUE
#define TP_QUEUE_LEN    (64)
#endif

/****************************************************************************
 * touch cofig setting
 ****************************************************************************/
#define TP_IDLE_TIME                (2000)
#define TP_IDLE_TIME_NO_INITIAL     (100000)

/****************************************************************************
 * ENABLE_TOUCH_POSITION_MSG :: just print X,Y coordination &
 *                              touch-down/touch-up
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
 * MACRO define of rm5t66a
 ****************************************************************************/
#ifdef	TP_I2C_ID_KCONFIG
#define TP_I2C_DEVICE_ID       (TP_I2C_ID_KCONFIG)
#else
#define TP_I2C_DEVICE_ID       0x48
#endif

#define TP_SAMPLE_RATE  (33)

#ifdef  CFG_LCD_ENABLE
#define TP_SCREEN_WIDTH     ithLcdGetWidth()
#define TP_SCREEN_HEIGHT    ithLcdGetHeight()
#else
#define	TP_SCREEN_WIDTH	    (800)
#define	TP_SCREEN_HEIGHT	(480)
#endif

#define RAYDIUM_I2C_PID_READ                1
#define RAYDIUM_I2C_PID_WRITE               0
#define RAYDIUM_I2C_PID_WORD_LEN            4
#define RAYDIUM_I2C_PID_BYTE_LEN            1
#define RAYDIUM_I2C_PDA_ADDRESS_LENGTH      5
#define RAYDIUM_I2C_PDA_MODE_WORD_MODE      0x8
#define RAYDIUM_I2C_MAX_PACKET_SIZE         128
#define RAYDIUM_PDA_TCH_RPT_ADDR            0x20005080

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
    char tpIntPin;          //INT signal GPIO pin number
    char tpIntActiveState;  //High=1, Low=0
    char tpIntTriggerType;  //interrupt trigger type. 0:edge trigger, 1:level trigger
    char tpWakeUpPin;       //Wake-Up pin GPIO pin number, -1: means NO Wake-Up pin.
    char tpResetPin;        //Reset pin GPIO pin number, -1: means NO reset pin.
    char tpIntrType;        //0:keep state when touch down(like ZT2083), 1:like FT5XXX type 2:like IT7260, 3:others....
    char tpInterface;       //0:I2C, 1:SPI, 2:other...
    char tpI2cDeviceId;     //I2C device ID(slave address) if TP has I2C interface
    char tpHasTouchKey;     //0: NO touch key, 1:touch key type I, 2:touch key type II, ...
    char tpIntUseIsr;       //0:polling INT siganl, 1:INT use interrupt,
    char tpMaxFingerNum;    //The TP native maximun of finger numbers
    char tpIntActiveMaxIdleTime;    //default: 33ms, cytma568: 100ms

    //TP resolution
    int  tpMaxRawX;
    int  tpMaxRawY;
    int  tpScreenX;
    int  tpScreenY;

    //TP convert function
    char tpCvtSwapXY;       //0:Disable, 1:Enable
    char tpCvtReverseX;     //0:Disable, 1:Enable
    char tpCvtReverseY;     //0:Disable, 1:Enable
    char tpCvtScaleX;       //0:Disable, 1:Enable
    char tpCvtScaleY;       //0:Disable, 1:Enable

    //TP sample specification
    char tpEnTchPressure;   //0:disable pressure info, 1:enable pressure info
    char tpSampleNum;       //0:NO scense, 1: single touch 2~10:multi-touch("tpSampleNum" must be <= "tpMaxFingerNum")
    char tpSampleRate;      //UNIT: mill-second, range 8~16 ms(60~120 samples/per second)

    //TP idle time
    int  tpIdleTime;		//sleep time for polling INT signal(even if interrupt mode).    
    int  tpIdleTimeB4Init;  //sleep time if TP not initial yet.
    int  tpReadChipRegCnt;  //read register count for getting touch xy coordination
    int  tpMoveDetectUnit;	//read register count for getting touch xy coordination

    //TP specific function
    char tpHasPowerOnSeq;   //0:NO power-on sequence, 1:TP has power-on sequence
    char tpNeedProgB4Init;  //0:TP IC works well without programe flow, 1:TP IC need program before operation.
    char tpNeedAutoTouchUp;
    char tpIntPullEnable;   //use internal pull up/down function
} TP_SPEC;

/***************************
 * global variable
 **************************/
static struct ts_sample g_sample[10];
static struct ts_sample gTmpSmp[10];

static struct ts_sample g_ReduceSmpBase[10];
static struct ts_sample g_ProbeSmpBase[10];

static unsigned char g_CurrReadBuf[RAYDIUM_I2C_MAX_PACKET_SIZE];
static unsigned char g_LastReadBuf[RAYDIUM_I2C_MAX_PACKET_SIZE];

static char g_TouchDownIntr = false;
static char  g_IsTpInitialized = false;
static pthread_mutex_t  gTpMutex;

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

static uint32_t gIdMapCnt = 0;
static uint8_t gMaxMapCnt = 32;
static uint8_t tpIdMap[32] = {0};
static uint8_t curFgId[10] = {0};
static uint8_t curFgIdCnt = 0;

/*************************************************
 global variable: gTpKeypadValue
 key0 is pressed if gTpKeypadValue's bit 0 is 1
 key1 is pressed if gTpKeypadValue's bit 1 is 1
   ...and so on

 NO key event if gTpKeypadValue = 0
 MAX key number: 32 keys
***************************************************/
static uint32_t gTpKeypadValue = 0;

#ifdef ENABLE_MEASURE_US_TIME
static uint32_t gT0 = 0, gT1 = 0;
static uint32_t minDur              = 0xFFFFFFFF;
static uint32_t totalDur            = 0;
static uint32_t avgDur              = 0;
static uint32_t mCnt                = 0;
static uint32_t gTimerAlignValue    = 0;
#endif
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
uint8_t _raydium_i2c_cal_crc8(uint8_t* data, uint8_t data_len)
{
    uint8_t crc_output = 0xFF;
    uint8_t i;
    uint8_t din[8];
    uint8_t shift[8];
    uint8_t crc[8];

    while(data_len--) {
        for (i = 0; i < 8; i++) {
            din[i] = (*data >> i) & 0x01;
            crc[i] = (crc_output >> i) & 0x01;
        }
        data++;

        shift[0] = din[7] ^ crc[7];
        shift[1] = din[6] ^ crc[6];
        shift[2] = din[5] ^ crc[5];
        shift[3] = din[4] ^ crc[4];
        shift[4] = din[3] ^ crc[3] ^ shift[0];
        shift[5] = din[2] ^ crc[2] ^ shift[0] ^ shift[1];
        shift[6] = din[1] ^ crc[1] ^ shift[0] ^ shift[1] ^ shift[2];
        shift[7] = din[0] ^ crc[0] ^ shift[1] ^ shift[2] ^ shift[3];

        crc[7] = shift[0] ^ shift[2] ^ shift[3] ^ shift[4];
        crc[6] = shift[1] ^ shift[3] ^ shift[4] ^ shift[5];
        crc[5] = shift[2] ^ shift[4] ^ shift[5] ^ shift[6];
        crc[4] = shift[3] ^ shift[5] ^ shift[6] ^ shift[7];
        crc[3] = shift[4] ^ shift[6] ^ shift[7];
        crc[2] = shift[5] ^ shift[7];
        crc[1] = shift[6];
        crc[0] = shift[7];

        crc_output = 0;
        for (i = 0; i < 8; i++) {
            crc_output = (crc[i] << i) | crc_output;
        }
    }

    return crc_output;
}

static int _raydium_i2c_pda_read(uint32_t addr, uint8_t *data, uint32_t data_len)
{
    ITPI2cInfo info = {0};
    uint8_t cmd_buff[RAYDIUM_I2C_PDA_ADDRESS_LENGTH] = {0};
    uint8_t read_buff[RAYDIUM_I2C_MAX_PACKET_SIZE + 1] = {0};
    uint8_t retry;
    uint8_t mode = 0x00;
    uint8_t crc;
    uint8_t crc_buff[11] = {0};
    uint16_t crc_index;
    int ret;

    if ((data_len % 4) == 0) {
        mode |= RAYDIUM_I2C_PDA_MODE_WORD_MODE;
    }

    cmd_buff[0] = (addr & 0x000000FF) >> 0;
    cmd_buff[1] = (addr & 0x0000FF00) >> 8;
    cmd_buff[2] = (addr & 0x00FF0000) >> 16;
    cmd_buff[3] = (addr & 0xFF000000) >> 24;
    cmd_buff[4] = mode;

    info.slaveAddress   = TP_I2C_DEVICE_ID;
    info.cmdBuffer      = cmd_buff;
    info.cmdBufferSize  = sizeof(cmd_buff);
    info.dataBuffer     = read_buff;
    info.dataBufferSize = data_len + 1;

    ret = read(gTpInfo.tpDevFd, &info, 1);
    if (ret < 0) {
		printf("i2cRd: r<0, r=%x\n", ret);
        return ret;
    }

    crc_buff[0] = (TP_I2C_DEVICE_ID << 1) | RAYDIUM_I2C_PID_WRITE;
    crc_buff[1] = (addr & 0x000000FF) >> 0;
    crc_buff[2] = (addr & 0x0000FF00) >> 8;
    crc_buff[3] = (addr & 0x00FF0000) >> 16;
    crc_buff[4] = (addr & 0xFF000000) >> 24;
    crc_buff[5] = mode;
    crc_buff[6] = (TP_I2C_DEVICE_ID << 1) | RAYDIUM_I2C_PID_READ;

    if ((data_len % 4) == 0) {
        crc_index = RAYDIUM_I2C_PID_WORD_LEN;
        memcpy(&data[0], &read_buff[0], crc_index);
        memcpy(&data[4], &read_buff[5], data_len - crc_index);
    } else {
        crc_index = RAYDIUM_I2C_PID_BYTE_LEN;
        memcpy(&data[0], &read_buff[0], crc_index);
        memcpy(&data[1], &read_buff[2], data_len - crc_index);
    }

    memcpy(&crc_buff[7], &read_buff[0], crc_index);

    crc = _raydium_i2c_cal_crc8(&crc_buff[0], crc_index + 7);
    if (crc !=  read_buff[crc_index]) {
        ret = -1;
		printf("i2cRde: CRC r=%x\n", ret);
    }

    return ret;
}

#ifdef  ENABLE_MEASURE_US_TIME

static uint32_t
tp_getUsByTimer (
    void)
{
    uint8_t     tmr = TP_USE_TIMER; // timer1 offset = 0x00, timer2 = 0x10, timer3 = 0x20, ....
    uint32_t    tmrCtlReg = ITH_TIMER_BASE + 0x80 + 4 * (tmr - 1);
    uint32_t    tmrValReg = ITH_TIMER_BASE + 0x10 * (tmr - 1);
    uint32_t    tmrSetReg = tmrValReg + 0xC;
    uint32_t    alignment = 0;
    uint32_t    gTa = 0, gTb = 0;

    /* Set first ticks value */
    if ((ithReadRegA(tmrCtlReg) & 0xFF) == 0)
    {
        ithWriteRegA(tmrSetReg, 0xFFFFFFFF);
        usleep(1000);
        ithWriteRegA(tmrCtlReg, 0x35);
        usleep(1000);

        /* to calculate the alignment of timer counter in 1000us */
        gTa = ithReadRegA(tmrValReg) & 0xFFFFFFFF;
        usleep(1000);
        gTb = ithReadRegA(tmrValReg) & 0xFFFFFFFF;

        if (gTb >= gTa)
        {
            alignment = gTb - gTa;
        }
        else
        {
            alignment = (0xFFFFFFFF - gTa) + gTb;
        }

        gTimerAlignValue = alignment / 1000;
        (void)printf("gTimerAlignValue = %d\n", gTimerAlignValue);
    }

    return (ithReadRegA(tmrValReg) & 0xFFFFFFFF);
}

static uint32_t
tp_getDurByTimer (
    uint32_t    t1,
    uint32_t    t0)
{
    uint32_t dur = 0;
    if (t1 >= t0)
    {
        dur = t1 - t0;
    }
    else
    {
        dur = (0xFFFFFFFF - t0) + t1;
    }

    if (gTimerAlignValue)
    {
        dur = dur / gTimerAlignValue;
    }

    /* statistics */
    if ((minDur > dur) && (mCnt > 3))
    {
        minDur = dur;
    }

    mCnt++;
    avgDur      = (totalDur + dur) / mCnt;
    totalDur    = totalDur + dur;

    // (void)printf("duration = %d us, (%x - %x)\n", dur, t0, t1);
    // (void)printf("SUM dur: min=%d, avg=%d, total=%d, c=%d\n", minDur, avgDur, totalDur, mCnt);

    return dur;
}
#endif

/*
  To check if any last ID has Touch-up in this time
  input: *b1, the pointer-buffer of rm5t66a
  input: *lastId, the last ID array
  input: lastIdCnt, the last ID count
  output: the removed count of Finger ID
*/
uint8_t _chkFingerIdRemoveCnt(uint8_t *b1, uint8_t *lastId, uint8_t lastIdCnt)
{
	uint8_t changeCnt = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint8_t id1 = 0;
	uint8_t id2 = 0;
	uint8_t gotMatchId = 0;
	uint8_t oriFnNum = b1[1];
	
#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("lastIdCnt:: %d, %d, %d\n",lastIdCnt, oriFnNum, gTpSpec.tpSampleNum);
#endif
	//check if finger ID changed?

	//to get the removed id
	for(i=0; i<lastIdCnt; i++)
	{
		id1 = lastId[i];
		
		#ifdef TP_ID_REMAP_DEBUG_MSG
		if((id1 == 0) || (id1 >= 31))
		{
			printf("Error Case1: Finger ID = 0, i=%d, id:%x\n", i, id1);
			while(1);
		}
		printf("If Changed?:: i=%d, id=%d\n",i, id1);
		#endif

		gotMatchId = 0;
		for(j=0; j<oriFnNum; j++)
		{
			id2 = (b1[4UL + (11UL * j)] & 0x3Fu);
			#ifdef TP_ID_REMAP_DEBUG_MSG
			if(id2 == 0)
			{
				printf("Error Case2: Finger ID = 0, i=%d, id:%x,%x\n", i, id1, id2);
				while(1);
			}
			#endif
			if(id1 == id2)
			{
				gotMatchId = 1;
				break;
			}
		}
		if(gotMatchId == 0u)
		{
			changeCnt++;
			#ifdef TP_ID_REMAP_DEBUG_MSG
			printf("Got a removed id: %d\n",id1);
			#endif
		}
	}
	#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("return chang cnt: %d\n",changeCnt);
	#endif
	return changeCnt;
}

/*
  To remove some finger ID data from the pointer-buffer
  input: *buf, the pointer-buffer of rm5t66a
  input: *lastId, the last ID array
  input: lastIdCnt, the last ID count
  output: 
*/
void _RemovePntBufData(uint8_t *buf, uint8_t *lastId, uint8_t lastIdCnt)
{
	uint32_t i=0;
	uint32_t j=0;
	uint8_t id1 = 0;
	uint8_t id2 = 0;
	uint8_t needRemoveIt = 0;
	uint8_t oriFnNum = buf[1];
	
	for(i=0; i<oriFnNum; i++)
	{
		id1 = (buf[4u + (11u * i)] & 0x3Fu);
		#ifdef TP_ID_REMAP_DEBUG_MSG
		if((id1 == 0) || (id1 >= 31))
		{
			printf("Error Case3: Finger ID = 0, i=%d, id:%x\n", i, id1);
			while(1);
		}
		printf("	chk1: i=%x, id:%x\n", i, id1);
		#endif
		needRemoveIt = 1;
		for(j=0; j<lastIdCnt; j++)
		{
			id2 = lastId[j];			
			if(id1 == id2)
			{
				needRemoveIt = 0;
				break;
			}
		}
		
		if(needRemoveIt != 0u)
		{
			#ifdef TP_ID_REMAP_DEBUG_MSG
			printf("got needRemoveIt, clear this id data: %d, %x, %x\n", i, id1, id2);
			#endif
			(void)memset(&buf[4u + (11u * i)], 0, 11);
		}
	}
}

/*
  To rearrange the pointer-buffer data
  input: 
  output: 
*/
void _RearrangPntBufData(uint8_t *buf)
{
	uint32_t i=0;
	uint32_t j=0;
	uint8_t id1 = 0;
	uint8_t gotValidBuf = 0;
	uint8_t oriFnNum = buf[1];
	uint8_t *bf1 = NULL;
	uint8_t *bf2 = NULL;
	
	//swap ID data
	for(i=0; i<oriFnNum; i++)
	{		
		id1 = (buf[4u + (11u * i)] & 0x3Fu);
		if(id1 == 0u)
		{
			gotValidBuf = 0;
			for(j = i + 1u; j<oriFnNum; j++)
			{				
				bf2 = (uint8_t*)&buf[4u + (11u * j)];
				
				if(bf2[0] != 0u)
				{
					bf1 = (uint8_t*)&buf[4u + (11u * i)];
					
					gotValidBuf = 1;
					(void)memcpy(bf1, bf2, 11);
					(void)memset(bf2, 0, 11);
					break;
				}
			}
			
			if(gotValidBuf == 0u)
			{
				#ifdef TP_ID_REMAP_DEBUG_MSG
				(void)printf("No other Data: %d, %d, %d\n",i,id1,j);
				#endif
				break;
			}
		}
		
		#ifdef TP_ID_REMAP_DEBUG_MSG
		if(i > oriFnNum)
		{
			(void)printf("Error case4, i > numOfTchPnt, i=%d, c=%d\n",i,oriFnNum);
		}
		#endif
	}				
}

/*
  To get a NEW finger ID when lastIdCnt < current ID count
  input: *buf, the pointer-buffer of rm5t66a
  input: *lastId, the last ID array
  input: lastIdCnt, the last ID count
  input: newId, the new ID array
  input: needCnt, the wanted count
  output: 
*/
void _getNewId(uint8_t *buf, uint8_t *lastId, uint8_t lastIdCnt, uint8_t *newId, uint8_t needCnt)
{
	uint8_t newCnt = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint8_t id1 = 0;
	uint8_t id2 = 0;
	uint8_t gotNewId = 0;
	uint8_t oriFnNum = buf[1];
	
	#ifdef TP_ID_REMAP_DEBUG_MSG
	(void)printf("lastIdCnt:: %d, %d, %d\n",lastIdCnt, oriFnNum, gTpSpec.tpSampleNum);
	#endif

	for(i=0; i<oriFnNum; i++)
	{
		id1 = (buf[4u + (11u * i)] & 0x3Fu);
		gotNewId = 1;
		for(j=0; j<lastIdCnt; j++)
		{
			id2 = lastId[j];
			if(id1 == id2)
			{
				gotNewId = 0;
			}
		}
		
		if((id1 == 0u) || (id1 >= gMaxMapCnt))
		{
			(void)printf("	tp_getNewId00: id1=%d, %d, max=%d\n", i, id1, gMaxMapCnt);
		}
		
		if((gotNewId != 0u) && (id1 > 0u) && (id1 < gMaxMapCnt))
		{
			newId[newCnt] = id1;
			newCnt++;
			if(newCnt >= needCnt)
			{
				break;
			}
		}
	}
}

/*
  pre-handle the pointer-buffer(remove some redundant data for driver)
  input: *buf, the pointer-buffer of rm5t66a
  output: 
*/
void _preHandlePointBuffer(uint8_t *buf)
{
	uint8_t numOfTchPnt = buf[1];
	uint32_t i = 0;
	uint8_t tmpID[10] = {0};
	uint8_t newIdLists[10] = {0};
	uint8_t removedCnt = 0;
	
#ifdef TP_ID_REMAP_DEBUG_MSG
	(void)printf("### ID mapping: ###\n");
	for(i=0; i<32; i++)
	{
		(void)printf("%02x ", tpIdMap[i]);
		if((i&0xF)==0xF)	printf("\n");
	}
	printf("\n");
#endif

	// calculate total ID count
	gIdMapCnt = 0;
	for (i = 1; i < gMaxMapCnt; i++)
	{
		if(tpIdMap[i] != 0xFFu)
		{
			tmpID[gIdMapCnt] = (uint8_t)i;
			gIdMapCnt++;
		}
	}
	tpIdMap[0] = (uint8_t)gIdMapCnt;	
	
#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("gIdMapCnt:: %x, %x, [%02x, %02x, %02x, %02x, %02x]\n",gIdMapCnt, gTpSpec.tpSampleNum, tmpID[0], tmpID[1], tmpID[2], tmpID[3], tmpID[4]);
#endif
	
	//check if ID removed?
	removedCnt = _chkFingerIdRemoveCnt(buf, tmpID, (uint8_t)gIdMapCnt);
	if(removedCnt != 0u)
	{
		if(gIdMapCnt == removedCnt)
		{
			#ifdef TP_ID_REMAP_DEBUG_MSG
			//printf("remove Case1.1: %d, %d\n", gIdMapCnt, removedCnt);
			#endif
			if(numOfTchPnt > removedCnt)
			{
				#ifdef TP_ID_REMAP_DEBUG_MSG
				printf("remove Case1.1: %d, %d, %d\n", gIdMapCnt, removedCnt, numOfTchPnt);
				#endif
				//_RemovePntBufData(buf, tmpID, (uint8_t)removedCnt);
				for(i=0; i<removedCnt; i++)
				{
					(void)memset(&buf[4u + (11u * i)], 0, 11);
				}
			
				_RearrangPntBufData(buf);
				buf[1] = (uint8_t)(numOfTchPnt - removedCnt);
			}
			else
			{
				#ifdef TP_ID_REMAP_DEBUG_MSG
				printf("remove Case1.2: %d, %d, %d\n", gIdMapCnt, removedCnt,numOfTchPnt);
				#endif
			}
			
		}
		else
		{
			#ifdef TP_ID_REMAP_DEBUG_MSG
			printf("remove Case2: %d, %d\n", gIdMapCnt, removedCnt);
			#endif
			
			//remove other finger ID(not in tpIdMap[] array)
			_RemovePntBufData(buf, tmpID, (uint8_t)gIdMapCnt);
			
			//rearrange pointer-buffer(keep original data)
			_RearrangPntBufData(buf);
			buf[1] = (uint8_t)(gIdMapCnt - removedCnt);
		}
	}
	else
	{
		#ifdef TP_ID_REMAP_DEBUG_MSG
		printf("remove Case2: %d, %d, %d\n", removedCnt, numOfTchPnt, gIdMapCnt);
		#endif
		
		if(numOfTchPnt > gIdMapCnt)
		{
			//keep original lastfingerNum data, remove others
			if(numOfTchPnt > (uint8_t)gTpSpec.tpSampleNum)
			{
				if(gIdMapCnt == (uint8_t)gTpSpec.tpSampleNum)
				{
					#ifdef TP_ID_REMAP_DEBUG_MSG
					printf("remove Case4: %d, %d\n", gIdMapCnt, gTpSpec.tpSampleNum);
					#endif
					
					//remove other finger ID
					_RemovePntBufData(buf, tmpID, (uint8_t)gIdMapCnt);			
				}
				else
				{
					uint8_t newIdCnt = (uint8_t)(gTpSpec.tpSampleNum - gIdMapCnt);
					
					#ifdef TP_ID_REMAP_DEBUG_MSG
					printf("remove Case5: %d, %d, %d\n", gIdMapCnt, gTpSpec.tpSampleNum, newIdCnt);
					#endif
					
					 _getNewId(buf, tmpID, (uint8_t)gIdMapCnt, newIdLists, newIdCnt);
					
					//add (numOfTchPnt - gIdMapCnt) finger buffer data
					for(i=0; i<newIdCnt; i++)
					{
						tmpID[gIdMapCnt + i] = newIdLists[i];
					}
					
					//remove other finger ID
					_RemovePntBufData(buf, tmpID, (uint8_t)(gIdMapCnt + newIdCnt));
				}

				//rearrange pointer-buffer
				_RearrangPntBufData(buf);
				
				//modify the value of buf[1] as gTpSpec.tpSampleNum
				buf[1] = (uint8_t)(gTpSpec.tpSampleNum);	
			}
			else
			{
				#ifdef TP_ID_REMAP_DEBUG_MSG
				printf("remove Case3: %d, %d\n", numOfTchPnt, gIdMapCnt);
				#endif
			}
		}
		
		if(numOfTchPnt == gIdMapCnt)
		{
			#ifdef TP_ID_REMAP_DEBUG_MSG
			printf("remove Case6: %d, %d\n", numOfTchPnt, gIdMapCnt);
			#endif
			//keep origin buffer
		}
		
		if(numOfTchPnt < gIdMapCnt)
		{
			#ifdef TP_ID_REMAP_DEBUG_MSG
			printf("remove Case7: %d, %d\n", numOfTchPnt, gIdMapCnt);
			#endif
			
			//finger ID must be removed(impossible here)
		}
		
		if(numOfTchPnt == gIdMapCnt)
		{
			#ifdef TP_ID_REMAP_DEBUG_MSG
			printf("remove Case8: %d, %d\n", numOfTchPnt, gIdMapCnt);
			#endif
			//keep origin buffer
		}
	}
	
#ifdef TP_ID_REMAP_DEBUG_MSG
	{
		uint8_t num = buf[1];
		int loop = (num)*11+4+11;
		
		printf("	New TP buf2::\n	");
		for (i = 0; i < 4; i++)
		{
			printf("%02x ", buf[i]);
		}
		printf("\n	");
		for (i = 4; i < loop; i++)
		{
			printf("%02x ", buf[i]);
			if(((i-4)%11)==10)	printf("\n	");
		}
		printf("\n");
		printf("\n");			
	}
#endif	
}

/*
  To search a free ID number
  input:
  output: a free finger ID which is not in tpIdMap[]
*/
uint8_t _tpSearchUnUsedId(void)
{
	uint8_t j=0;
	uint8_t tmpId = 0;
	uint8_t gotUsedId = 0;
	uint8_t idFull = 0;
	
	while(true)
	{
		gotUsedId = 0;
		//search if tmpId has in idMap[]
		for (j = 1; j < gMaxMapCnt; j++) {
			if(tpIdMap[j] == tmpId)
			{
				gotUsedId = 1;
				break;
			}
		}
		
		//got a unused fingerId
		if(gotUsedId == 0u)
		{
			break;		
		}
		
		//check if fingerId full in idMap[]?
		if(tmpId++ >= 10)
		{
			idFull = 1u;
			break;
		}
	}

	if((gotUsedId==0u) && (idFull == 0u))
	{
		#ifdef TP_ID_REMAP_DEBUG_MSG
		printf("[OK] got fingerId(%d)!!\n", tmpId);
		#endif
		return tmpId;
	}
	else
	{
		(void)printf("[Error] can not get unused fingerId: %x, %x\n", gotUsedId, idFull);
		return 0xFF;
	}
}

/*
  To assign a ID number for upper layer
  input: tpId, the finger id from rm5t66a
  output: a ID number for SDL
*/
uint8_t _assignFingerID(uint8_t tpId)
{
	uint32_t i=0;
	uint8_t tmpId = 0;
	
	if((tpId == 0u) || (tpId > (gMaxMapCnt - 1u)))
	{
		(void)printf("Error Case, incorrect tpID=%d, Need to Debug it:gMaxMapCnt=%d\n", tpId, gMaxMapCnt);
		return (uint8_t)gTpSpec.tpSampleNum;
	}
	
	if(curFgIdCnt == 0u)
	{
		// calculate total ID count
		gIdMapCnt = 0;
		for (i = 1; i < gMaxMapCnt; i++)
		{
			if(tpIdMap[i] != 0xFFu)
			{
				gIdMapCnt++;
			}
		}
		tpIdMap[0] = (uint8_t)gIdMapCnt;
	}	
	
#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("B4.afID:: cnt=%d \n", gIdMapCnt);
	for (i = 0; i < gMaxMapCnt; i++)
	{
		printf("%02x ",tpIdMap[i]);
		if((i&0xF)==0xF)	printf("\n");
	}
	printf("\n");
#endif

	if(gIdMapCnt == 0u)
	{
		tpIdMap[tpId] = 0;
		gIdMapCnt++;
	}
	else
	{
		if(tpIdMap[tpId] != 0xFFu)
		{
#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("already exist:: %d, %d\n", tpId, tpIdMap[tpId]);
#endif
			//already exist
			tmpId = tpIdMap[tpId];
		}
		else
		{
			if( (gIdMapCnt >= (uint32_t)gTpSpec.tpSampleNum) && (gTpSpec.tpSampleNum > (char)1) )
			{
#ifdef TP_ID_REMAP_DEBUG_MSG
				printf("out of range:: %d, %d\n", gIdMapCnt, gTpSpec.tpSampleNum);
#endif
				tmpId = 0xFF;
			}
			else
			{
				//search a unused ID
				tmpId = (unsigned char)_tpSearchUnUsedId();
				if(tmpId != 0xFFu)
				{					
					tpIdMap[tpId] = tmpId;
					gIdMapCnt++;
				}
				else
				{
					#ifdef TP_ID_REMAP_DEBUG_MSG
					printf("	Error Case: _tpSearchUnUsedId() fail, di: %x, %x\n",tpId, tmpId);
					#endif
				}
			}
		}	
	}
		
	//record curFgIdCnt & tpId
	if(tmpId != 0xFFu)
	{
		curFgId[curFgIdCnt] = tpId;
		curFgIdCnt++;
	}

#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("after.afID:: gIdMapCnt = %x, %x\n", gIdMapCnt, curFgIdCnt);
	for (i = 0; i < gMaxMapCnt; i++)
	{
		printf("%02x ",tpIdMap[i]);
		if((i&0xF)==0xF)	printf("\n");
	}
	printf("\n");
#endif

#ifdef TP_ID_REMAP_DEBUG_MSG
	if(tmpId == 0xFF)
	{
		printf("assign ID error0, ori=%x, new=%x\n", tpId, tmpId);
	}
#endif

	return tmpId;
}

/*
  remove the Unused Finger Id from tpIdMap[]
*/
void _tpRemoveUnUsedId(void)
{
	uint32_t i=0;
	uint32_t j=0;
	uint8_t removedCnt = 0;
	uint8_t needRemoveId = 1;	

#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("B4.Remove:: \n");
	for (i = 0; i < gMaxMapCnt; i++)
	{
		printf("%02x ",tpIdMap[i]);
		if((i&0xF)==0xF)	printf("\n");
	}
	printf("\n");
	printf("B4.needId:: ");
	for (i = 0; i < curFgIdCnt; i++)
	{
		printf("%02x ",curFgId[i]);
	}
	printf("\n");
#endif

	for (i = 1; i < gMaxMapCnt; i++)
	{
		if(tpIdMap[i] != 0xFFu)
		{
			needRemoveId = 1;
			
			for (j = 0; j < curFgIdCnt; j++) 
			{
				if(curFgId[j] == i)
				{
					needRemoveId = 0;
					break;
				}
			}
			
			if(needRemoveId != 0u)
			{
				removedCnt++;
				#ifdef TP_ID_REMAP_DEBUG_MSG
				printf("Remove this ID: id=%x, c=%x\n", i, removedCnt);
				#endif
				tpIdMap[i] = 0xFF;
			}
		}
	}

	//clear curFgId[]
	curFgIdCnt = 0;
	for (i = 0; i < 10u; i++)
	{
		curFgId[i] = 0;
	}
	
#ifdef TP_ID_REMAP_DEBUG_MSG
	printf("after.Remove:: \n");
	for (i = 0; i < gMaxMapCnt; i++)
	{
		printf("%02x ",tpIdMap[i]);
		if((i&0xF)==0xF)	printf("\n");
	}
	printf("\n");
#endif
}

static void _tpInitSpec_vendor(void)
{
	//initialize tpIdMap[]
	{
		int i = 0;
		for (i = 0; i < gMaxMapCnt; i++) {
			tpIdMap[i] = 0xFF;
		}
	}

    gTpSpec.tpIntPin            = (char)TP_INT_PIN;           //from Kconfig setting
    gTpSpec.tpWakeUpPin         = (char)TP_GPIO_PIN_NO_DEF;   //from Kconfig setting
    gTpSpec.tpResetPin          = (char)TP_GPIO_RESET_PIN;   //from Kconfig setting
    gTpSpec.tpIntUseIsr         = (char)TP_ENABLE_INTERRUPT;  //from Kconfig setting
    gTpSpec.tpIntActiveState    = (char)TP_ACTIVE_LOW;        //from Kconfig setting
    gTpSpec.tpIntTriggerType    = (char)TP_INT_EDGE_TRIGGER; //from Kconfig setting   level/edge

    gTpSpec.tpInterface         = (char)TP_INTERFACE_I2C;     //from Kconfig setting
    gTpSpec.tpIntActiveMaxIdleTime = (char)TP_SAMPLE_RATE;    //from Kconfig setting

    gTpSpec.tpMaxRawX           = (int)CFG_TOUCH_X_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpMaxRawY           = (int)CFG_TOUCH_Y_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpScreenX           = (int)TP_SCREEN_WIDTH;       //from Kconfig setting
    gTpSpec.tpScreenY           = (int)TP_SCREEN_HEIGHT;      //from Kconfig setting

    gTpSpec.tpCvtSwapXY        = (char)TP_SWAP_XY;            //from Kconfig setting
    gTpSpec.tpCvtReverseX      = (char)TP_REVERSE_X;          //from Kconfig setting
    gTpSpec.tpCvtReverseY      = (char)TP_REVERSE_Y;          //from Kconfig setting
    gTpSpec.tpCvtScaleX        = (char)0;                    //from Kconfig setting
    gTpSpec.tpCvtScaleY        = (char)0;                    //from Kconfig setting

    gTpSpec.tpI2cDeviceId       = (char)TP_I2C_DEVICE_ID;     //from this driver setting
    gTpSpec.tpEnTchPressure     = (char)0;                    //from this driver setting
    gTpSpec.tpSampleNum         = (char)MAX_FINGER_NUM;       //from this driver setting
    gTpSpec.tpSampleRate        = (char)TP_SAMPLE_RATE;       //from this driver setting
    gTpSpec.tpIntrType          = (char)TP_INT_TYPE_IT7260;   //from this driver setting
    gTpSpec.tpHasTouchKey       = (char)TP_WITHOUT_KEY;       //from this driver setting
    gTpSpec.tpIdleTime          = (int)TP_IDLE_TIME;          //from this driver setting
    gTpSpec.tpIdleTimeB4Init    = (int)TP_IDLE_TIME_NO_INITIAL;//from this driver setting
    gTpSpec.tpMoveDetectUnit    = (int)1;
    gTpSpec.tpReadChipRegCnt    = (int)RAYDIUM_I2C_MAX_PACKET_SIZE;

    //special initial flow
    gTpSpec.tpHasPowerOnSeq     = (char)1;
    gTpSpec.tpNeedProgB4Init    = (char)1;
    gTpSpec.tpNeedAutoTouchUp   = (char)1;
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
	char rstPin = (char)TP_GPIO_RESET_PIN;

    printf("DO POWER-ON sequence, reset pin:%d\n",rstPin);	
    if(rstPin == (char)-1) return;

    //set reset pin as 1 for 1ms
	ithGpioSetMode(rstPin,ITH_GPIO_MODE0);
    ithGpioEnable(rstPin);
    
    ithGpioSetOut(rstPin);
	ithGpioSet(rstPin);
	usleep(1000);
	
	//set reset pin as 0 for 10ms
	ithGpioClear(rstPin);

	usleep(10*1000);

	//output reset pin as 1 again
    ithGpioSet(rstPin);    	
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
    bool recv = false;
    int ret;
    //uint8_t buff[RAYDIUM_I2C_MAX_PACKET_SIZE] = {0};
    static uint8_t seq_num;
    uint8_t touch_points;
    uint16_t crc;
    uint32_t i, j;

    ret = _raydium_i2c_pda_read(RAYDIUM_PDA_TCH_RPT_ADDR, buf, cnt);
    if (ret != 0) {
		if(ret != -1)
		{
			printf("%s: _raydium_i2c_pda_read error\n", __func__);
			return -1;
		}
    }

    seq_num = buf[0];
#ifdef  ENABLE_TOUCH_PANEL_DBG_MSG
    printf("%s: seq_num %d\n", __func__, seq_num);
#endif

    touch_points = buf[1];
	if(touch_points == 0)
	{
		#ifdef TP_ID_REMAP_DEBUG_MSG
		printf("Got a FnNum = 0, then clear all finger array[]!!\n");
		#endif
		if(gLastNumFinger)
		{
			#ifdef TP_ID_REMAP_DEBUG_MSG
			printf("clear tpIdMap[] \n");
			#endif
			tpIdMap[0] = 0;
			for (i = 1; i < gMaxMapCnt; i++)
			{
				tpIdMap[i] = 0xFF;
			}
			gLastNumFinger = 0;			
		}
	}

	if (touch_points == 0 || touch_points > 10) {
        if(touch_points > 10)	printf("%s: touch_points %d invalid!\n", __func__, touch_points);
        return -1;
    }
#ifdef  ENABLE_TOUCH_PANEL_DBG_MSG
    printf("%s: touch_points %d\n", __func__, touch_points);
#endif

    crc = 0xFFFF;

    for (i = 0; i < 126; i++) {
        crc ^= buf[i];

        for (j = 0; j < 8; j++) {
            if (crc & 0x1) {
                crc = (crc >> 1) ^0xA001;
            }
            else {
                crc = (crc >> 1);
            }
        }
    }

    if (((crc & 0xFF) != buf[126]) || (((crc >> 8) & 0xFF) != buf[127])) {
        printf("%s: crc %x - %x%x error!\n", __func__, crc, buf[127], buf[126]);
        return -1;
    }

#ifdef TP_ID_REMAP_DEBUG_MSG
	if(touch_points > 0)
	{
		int loop = touch_points*11+4+11;
		printf("	TP buf::\n	");
		for (i = 0; i < 4; i++)
		{
			printf("%02x ", buf[i]);
		}
		printf("\n	");
		for (i = 4; i < loop; i++)
		{
			printf("%02x ", buf[i]);
			if(((i-4)%11)==10)	printf("\n	");
		}
		printf("\n");
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
    int i=0;
    char device_mode = 0;
    char numOfTchPnt = 0;
    unsigned int lfnum=gLastNumFinger;
    char loopNum = 0;
	unsigned char tpId = 0;

    if (buf[0] == 0) {
        return (-1);
    }

	//gettime
	#ifdef ENABLE_MEASURE_US_TIME
        gT0 = tp_getUsByTimer();
	#endif

	_preHandlePointBuffer(&buf[0]);
    
    numOfTchPnt = buf[1];

    if ( gTpSpec.tpSampleNum == 1) {
        numOfTchPnt = 1;
    }

    loopNum = numOfTchPnt;

    if (lfnum < numOfTchPnt) {
        //printf("fgr_chg++: %d, %d\n", lfnum, numOfTchPnt);
    }

    if (loopNum) {
        struct ts_sample *smp = (struct ts_sample*)s;

        for (i = 0; i < loopNum; i++) {
			tpId = (buf[4u + (11u * i)] & 0x3Fu);
			smp->id = _assignFingerID(tpId);
			if(smp->id > 10)
			{
				continue;
			}

            smp->pressure = 1;
			if(loopNum >= gTpSpec.tpSampleNum) {
				smp->finger = (uint32_t)gTpSpec.tpSampleNum;
			}
            else {
				smp->finger = (uint32_t)loopNum;
			}

            smp->x = (int)((((unsigned int)buf[6u + (11u * i)] << 8) & 0x0F00u) | ((unsigned int)buf[5u + (11u * i)]));
            smp->y = (int)((((unsigned int)buf[8u + (11u * i)] << 8) & 0x0F00u) | ((unsigned int)buf[7u + (11u * i)]));

            //printf("  RAW->[%d][%x, %x][%d, %d]--> %d %d %d\n", i, tpId, tpIdMap[tpId], smp->id, smp->finger, smp->pressure, smp->x, smp->y);
            if(gTpSpec.tpSampleNum > 1)   smp++;
        }

		_tpRemoveUnUsedId();	
		
#ifdef  ENABLE_MEASURE_US_TIME
            {
                uint32_t gTdur = 0;
                gT1     = tp_getUsByTimer();
                gTdur   = tp_getDurByTimer(gT1, gT0);
                //(void)printf("tp dur=%d us, (%x - %x)\n", gTdur, gT0, gT1);
				(void)printf("tp dur=%d us\n", gTdur);
            }
#endif
        pthread_mutex_lock(&gTpMutex);
        gLastNumFinger = s->finger;
        pthread_mutex_unlock(&gTpMutex);

        return 0;
    }

    return (-1);
}

static void _tpParseKey_vendor(struct ts_sample *s, unsigned char *buf)
{
    //TODO: get key information and input to xy sample...? as a special xy?
    //maybe define a special area for key
    //(like touch is 800x480, for example, y>500 for key, x=0~100 for keyA, x=100~200 for keyB... )
    //SDL layer could parse this special defination xy into key event(but this layer is not ready yet).
    //uint32_t  val = 0;
    //printf("Keypad Val = %x\n",gTpKeypadValue);

    //val = buf[5];
    /*
    if(x<320)   val |= 0x01;
    if(320<= x <640)    val |= 0x02;
    if(640<= x <960)    val |= 0x04;
    if(960<= x <1280)   val |= 0x08;
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
static int rm5t66a_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
    struct tsdev *ts = inf->dev;
    unsigned int regValue;
    int ret;
    int total = 0;
    int tchdev = ts->fd;

    if (samp == NULL)
    {
        printf("TP ERROR: L#%d samp is null!!\n", __LINE__);
        return 0;
    }

    #ifdef  ENABLE_SEND_FAKE_SAMPLE
    return _getFakeSample(samp,nr);
    #endif
	
    if(g_IsTpInitialized==false)
    {
#ifdef  USE_RAW_API
        _tpRegRawFunc(tchdev);

        if(!_raDoInitial()) return -1;
        else                return 0;
#else
        printf("TP first init(INT is GPIO %d)\n",TP_INT_PIN);
        gTpInfo.tpDevFd = tchdev;
        if(!_tpDoInitial()) return 0;
        else                return -1;
#endif
    }
	
	//for passing information to SDL
	samp->chkCurrFn = TSLIB_CHECK_MAGIC_NUM;
	
    //to probe touch sample
#ifdef  USE_RAW_API
    ret = _raProbeSample(samp, nr);
#else
    ret = _tpProbeSample(samp, nr);
#endif

    #ifdef  ENABLE_TOUCH_PANEL_DBG_MSG
    if(ret) printf("    deQue-O:fn=%d (%d,%d,%d)r=%d\n", samp->finger, samp->pressure, samp->x, samp->y, ret );
    #endif

    #ifdef  ENABLE_TOUCH_PANEL_DBG_MSG
    if(samp->pressure)  gNoEvtCnt = 3;
    if( gNoEvtCnt )
    {
        printf("    deQue-O1:[%x] fn=%d, id=%d (%d,%d,%d)r=%d\n", samp, samp->finger, samp->id, samp->pressure, samp->x, samp->y, ret );
        if(samp->finger>1)
        {
            struct ts_sample *tsp = (struct ts_sample *)samp->next;
            printf("    deQue-Q2:[%x] fn=%d, id=%d (%d,%d,%d)r=%d\n", tsp, tsp->finger, tsp->id, tsp->pressure, tsp->x, tsp->y, ret );
        }
        if( !samp->pressure )   gNoEvtCnt--;
    }
    #endif

    return ret;
}

static const struct tslib_ops rm5t66a_ops =
{
    rm5t66a_read,
};

TSAPI struct tslib_module_info *rm5t66a_mod_init(struct tsdev *dev, const char *params)
{
    struct tslib_module_info *m;

    m = malloc(sizeof(struct tslib_module_info));
    if (m == NULL)
        return NULL;

    m->ops = &rm5t66a_ops;
    return m;
}

#ifndef TSLIB_STATIC_CASTOR3_MODULE
    TSLIB_MODULE_INIT(rm5t66a_mod_init);
#endif
