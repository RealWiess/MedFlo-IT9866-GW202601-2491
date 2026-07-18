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
//#define TP_IDLE_TIME                (2000)
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
 * MACRO define of ILI2511  setting form kenny
 ****************************************************************************/
#ifdef	TP_I2C_ID_KCONFIG
#define TP_I2C_DEVICE_ID       (TP_I2C_ID_KCONFIG)
#else
#define TP_I2C_DEVICE_ID       (0x41)
#endif

//TP的INT周期为86ms，高电平为3~8ms，量波形的报点间隔是33ms
#define TP_SAMPLE_RATE	       (100)   //Not active timeout 设定为100ms
#define TP_IDLE_TIME                (10000)   //轮询时间10ms
//TP的原始坐标和SCALE最终坐标范围
#define TP_X_MAX_VALUE (799) //readback form tp
#define TP_Y_MAX_VALUE (1279) //readback form tp
#define	TP_MAX_SCREEN_X	    (800)  //scale max x
#define	TP_MAX_SCREEN_Y	(1280)  //scale max y
//TP坐标的缩放，交换，反转
#define TP_SCALE    (1)
//#define TP_SWAP_XY     (0)
//#define TP_REVERSE_X  (0)
//#define TP_REVERSE_Y  (0)


#ifdef	CFG_LCD_ENABLE
#define	TP_SCREEN_WIDTH	    ithLcdGetWidth()
#define	TP_SCREEN_HEIGHT	ithLcdGetHeight()
#else
#define	TP_SCREEN_WIDTH	    (1280)
#define	TP_SCREEN_HEIGHT	(400)
#endif

// i2c command for ilitek touch screen, protocol V3.0
#define ILITEK_TP_CMD_READ_DATA			    0x10
#define ILITEK_TP_CMD_READ_SUB_DATA		    0x11   
#define ILITEK_TP_CMD_GET_RESOLUTION		0x20
#define ILITEK_TP_CMD_GET_KEY_INFORMATION	0x22
#define ILITEK_TP_CMD_SLEEP                 0x30
#define ILITEK_TP_CMD_GET_FIRMWARE_VERSION	0x40
#define ILITEK_TP_CMD_GET_PROTOCOL_VERSION	0x42
#define ILITEK_TP_CMD_GET_CHIP_TYPE     	0x61
#define	ILITEK_TP_CMD_CALIBRATION			0xCC
#define	ILITEK_TP_CMD_CALIBRATION_STATUS	0xCD
#define ILITEK_TP_CMD_ERASE_BACKGROUND		0xCE

#define TOUCH_POINT    0x80
#define TOUCH_KEY      0xC0
#define RELEASE_KEY    0x40
#define RELEASE_POINT  0x00

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

// declare i2c data member
struct i2c_data {

	//firmware version
	unsigned char firmware_ver[4];
	// maximum x
	int max_x;
	// maximum y
	int max_y;
	// maximum touch point
	int max_tp;
	// maximum key button
	int max_btn;
	// the total number of x channel
	int x_ch;
	// the total number of y channel
	int y_ch;
	// protocol version
	int protocol_ver;
	
	int keycount;
	int status;
};           

typedef struct
{
	int open_test_ret;	//1--fail,0--pass,2--not support
	int open_row_lines;
	int buf_row[64];
	int open_column_lines;
	int buf_column[64];
	int vendor_id;
}_s_tp_open_test_ret;
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

static struct i2c_data i2c;
static bool sg_get_TP_flag = false;
static char sg_TP_str[32] = {0};
static _s_tp_open_test_ret g_tp_open_ret;

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
#ifdef	CFG_TOUCH_BUTTON
static uint32_t	gTpKeypadValue = 0;
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

/* ******************************************************************************* */

bool itpGetTPVersion_ili(char* str_ver)
{
    if (NULL != str_ver)
    {
        strcpy(str_ver, sg_TP_str);
    }
    
    return sg_get_TP_flag;
}

void itpGetTPOpenTestRet_ili(_s_tp_open_test_ret *ret)
{
    if (NULL == ret)
    {
        return;
    }

    memcpy(ret, &g_tp_open_ret, sizeof(g_tp_open_ret));
}

/*##################################################################################
 *                        the private function implementation
 ###################################################################################*/
static int ilitek_i2c_read_cmd(int fd,unsigned char cmd, unsigned char *data, int length)
{
    ITPI2cInfo *evt;
    unsigned char	I2cCmd;
    int i2cret;

    evt = alloca(sizeof(ITPI2cInfo));

	I2cCmd = cmd;		//1100 0010
	evt->slaveAddress   = gTpSpec.tpI2cDeviceId;
	evt->cmdBuffer      = &I2cCmd;
	evt->cmdBufferSize  = 1;
	evt->dataBuffer     = data;
	evt->dataBufferSize = length;
	i2cret = read(gTpInfo.tpDevFd, evt, 1);
	if(i2cret<0)	return -1;
		
	#ifdef	ENABLE_TOUCH_PANEL_DBG_MSG
	{
		int i;		
		printf("\n	raw-buf:");
		for(i=0; i<length; i++)	printf("%02x ",data[i]);
		printf("\n\n");		
	}
	#endif
	
	return 0;
}

static int ilitek_i2c_read_tp_info(int fd)
{
    int res_len, i;
    unsigned char max_tp,buf[64]={0};
    int error = 0;
    int open_line_x[64] = {0};
    int open_line_y[64] = {0};
    
    // read firmware version
    if(ilitek_i2c_read_cmd(fd, ILITEK_TP_CMD_GET_FIRMWARE_VERSION, buf, 4) < 0){
        printf("read firmware version fail\n");
        error = -1;

        return error;
    }
    for(i = 0;i<4;i++)  
    {
        i2c.firmware_ver[i] = buf[i];
    }
    printf( "%s, firmware version %d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2], buf[3]);
    if (0 == buf[0]
        && 0 == buf[1]
        && 0 == buf[2]
        && 0 == buf[3])
    {
        return -1;
    }
    sprintf(sg_TP_str, "%d.%d.%d.%d", buf[0], buf[1], buf[2], buf[3]);
    sg_get_TP_flag = true;
    
	// read protocol version
	res_len = 6;
	if(ilitek_i2c_read_cmd(fd, ILITEK_TP_CMD_GET_PROTOCOL_VERSION, buf, 2) < 0){
		printf("read protocol version fail\n");
		error = -1;

        return error;
	}	
	
	i2c.protocol_ver = (((int)buf[0]) << 8) + buf[1];			
    printf("%s, protocol version: %d.%d\n", __func__, buf[0], buf[1]);
		
	if((i2c.protocol_ver & 0xFF00) == 0x200){
		res_len = 8;
	}
	else if((i2c.protocol_ver & 0xFF00) == 0x300){
		res_len = 10;
	}

    // read touch resolution
	i2c.max_tp = 2;
        if(ilitek_i2c_read_cmd(fd, ILITEK_TP_CMD_GET_RESOLUTION, buf, res_len) < 0){
		error = -1;

        return error;
	}
	
	if((i2c.protocol_ver & 0xFF00) == 0x200){
		// maximum touch point
		i2c.max_tp = buf[6];
		// maximum button number
		i2c.max_btn = buf[7];
	}

	if((i2c.protocol_ver & 0xFF00) == 0x300){
		// maximum touch point
		i2c.max_tp = buf[6];
		// maximum button number
		i2c.max_btn = buf[7];
		// key count
		i2c.keycount = buf[8];
	}

	// calculate the resolution for x and y direction
	i2c.max_x = buf[0];
	i2c.max_x+= ((int)buf[1]) * 256;
	i2c.max_y = buf[2];
	i2c.max_y+= ((int)buf[3]) * 256;
	i2c.x_ch = buf[4];
	i2c.y_ch = buf[5];

	printf( "%s, max_x: %d, max_y: %d, ch_x: %d, ch_y: %d\n", 
	__func__, i2c.max_x, i2c.max_y, i2c.x_ch, i2c.y_ch);
	
	if((i2c.protocol_ver & 0xFF00) == 0x200){
		printf( "%s, max_tp: %d, max_btn: %d\n", __func__, i2c.max_tp, i2c.max_btn);
	}
	else if((i2c.protocol_ver & 0xFF00) == 0x300){
		printf( "%s, max_tp: %d, max_btn: %d, key_count: %d\n", __func__, i2c.max_tp, i2c.max_btn, i2c.keycount);
	}

    res_len = (i2c.y_ch>i2c.x_ch)?i2c.y_ch:i2c.x_ch;

    printf("D0-y:\n");
    ilitek_i2c_read_cmd(fd, 0xD0, buf, res_len);
    for(i=0; i<res_len; i++)
    {
        printf("%02x,", buf[i]);
        if (buf[i] == 0xDD)
        {
            open_line_x[g_tp_open_ret.open_row_lines++] = i;
        }
    }
    printf("\n");
    
    if (buf[0] == 0xEE)
    {
        g_tp_open_ret.open_test_ret = 2;
        return error;
    }

    printf("D1-x:\N");
    ilitek_i2c_read_cmd(fd, 0xD1, buf, res_len);
    for(i=0; i<res_len; i++)
    {
        printf("%02x,", buf[i]);
        if (buf[i] == 0xDD)
        {
            open_line_y[g_tp_open_ret.open_column_lines++] = i;
        }
    }
    printf("\n");

    if (buf[0] == 0xEE)
    {
        g_tp_open_ret.open_test_ret = 2;
        return error;
    }

    if (g_tp_open_ret.open_row_lines>0
        || g_tp_open_ret.open_column_lines>0)
    {
        g_tp_open_ret.open_test_ret = 1;

        printf("row has:");
        for(i=0; i<g_tp_open_ret.open_row_lines; i++)
        {
            printf("%d,", open_line_x[i]);
            g_tp_open_ret.buf_row[i] = open_line_x[i];
        }
        printf("\n");

        printf("column has:");
        for(i=0; i<g_tp_open_ret.open_column_lines; i++)
        {
            printf("%d,", open_line_y[i]);
            g_tp_open_ret.buf_column[i] = open_line_y[i];
        }
        printf("\n");
    }
    else
    {
        g_tp_open_ret.open_test_ret = 0;
    }

	
	return error;
}

static void _tpInitSpec_vendor(void)
{
    gTpSpec.tpIntPin          	= (char)TP_INT_PIN;           //from Kconfig setting
    gTpSpec.tpWakeUpPin         = (char)TP_GPIO_PIN_NO_DEF;   //from Kconfig setting
    gTpSpec.tpResetPin          = (char)TP_GPIO_RESET_PIN;    //from Kconfig setting //my.wei modify
    gTpSpec.tpIntUseIsr         = (char)TP_ENABLE_INTERRUPT;  //from Kconfig setting
    gTpSpec.tpIntActiveState    = (char)TP_ACTIVE_LOW;        //from Kconfig setting    
    gTpSpec.tpIntTriggerType    = (char)TP_INT_EDGE_TRIGGER; //from Kconfig setting   level/edge
    
    gTpSpec.tpInterface         = (char)TP_INTERFACE_I2C;	  //from Kconfig setting
        
    gTpSpec.tpMaxRawX           = (int)TP_X_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpMaxRawY           = (int)TP_Y_MAX_VALUE; //from Kconfig setting
    gTpSpec.tpScreenX           = (int)TP_MAX_SCREEN_X;       //from Kconfig setting
    gTpSpec.tpScreenY           = (int)TP_MAX_SCREEN_Y;      //from Kconfig setting
    
    gTpSpec.tpCvtSwapXY        = (char)TP_SWAP_XY;                    //from Kconfig setting
    gTpSpec.tpCvtReverseX      = (char)TP_REVERSE_X;                    //from Kconfig setting
    gTpSpec.tpCvtReverseY      = (char)TP_REVERSE_Y;                    //from Kconfig setting
    gTpSpec.tpCvtScaleX        = (char)TP_SCALE;                    //from Kconfig setting
    gTpSpec.tpCvtScaleY        = (char)TP_SCALE;                    //from Kconfig setting
    
    gTpSpec.tpI2cDeviceId       = (char)TP_I2C_DEVICE_ID;	  //from this driver setting
    gTpSpec.tpEnTchPressure     = (char)0;                    //from this driver setting
    gTpSpec.tpSampleNum         = (char)MAX_FINGER_NUM;       //from this driver setting
    gTpSpec.tpSampleRate        = (char)TP_SAMPLE_RATE;       //from this driver setting
    gTpSpec.tpIntrType          = (char)TP_INT_TYPE_KEEP_STATE;	  //from this driver setting
    gTpSpec.tpHasTouchKey       = (char)TP_WITHOUT_KEY;       //from this driver setting                                                               
    gTpSpec.tpIdleTime          = (int)TP_IDLE_TIME;          //from this driver setting
    gTpSpec.tpIdleTimeB4Init    = (int)TP_IDLE_TIME_NO_INITIAL;//from this driver setting    
    gTpSpec.tpMoveDetectUnit    = (int)1;
    gTpSpec.tpReadChipRegCnt    = (int)(5 * MAX_FINGER_NUM) + 1;
    
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
	//for Gt911 Linux  power on methods.	
	char rstPin = (char)TP_GPIO_RESET_PIN;

    printf("DO POWER-ON sequence, reset pin:%d\n",rstPin);	
    if(rstPin == (char)-1) return;
    
	//1.set "Reset pin" & "INT pin" are output-low for 1ms	
	ithGpioSetMode( rstPin, ITH_GPIO_MODE0);
	ithGpioSet(rstPin);
	ithGpioSetOut(rstPin);
	ithGpioEnable(rstPin);	
	
	//my.wei generate RST signal	
	usleep(1000); //delay 1ms

    //2.set "Reset pin" output HIGH for 1 ms
	ithGpioClear(rstPin);
	usleep(5*1000); //low hold 1ms

    //3.set "INT pin" output HIGH for 100 ms
   	ithGpioSet(rstPin);    	
	usleep(300*1000); //delay 100ms
}

static int _tpDoInitProgram_vendor(void)
{
    //TODO: 
	unsigned char max_tp,buf[64]={0};
    printf("DO intial programming!!!\n");
    int ret = 0;
    int re_try = 0;

    while(re_try < 3)
    {
        ret = ilitek_i2c_read_tp_info(gTpInfo.tpDevFd);
        if (ret == 0)
        {
            break;
        }

        usleep(500*1000);
		re_try++;
    }
	// read chip type
	if(ilitek_i2c_read_cmd(gTpInfo.tpDevFd, ILITEK_TP_CMD_GET_CHIP_TYPE, buf, 1) < 0)
	{
		printf("read chip type fail\n");
		//ret = -1;
	}
	else
	{
		ret = 0;
		printf("read chip type = 0x%x\n", buf[0]);		
	}
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
    ITPI2cInfo *evt;
	unsigned char	I2cCmd;
    int i2cret;
    unsigned char tmpbuf[64];

	if((i2c.protocol_ver & 0xFF00) == 0x200)
	{
		printf("skip tp read point, need upgrade tp protocol V3.0\n");
		return -1;
	}
	
	//printf("READ BUFFER POINTER!!!\n\n");	

	//initial buffer
	memset(tmpbuf,0,64);
	memset(buf,0,cnt);

	i2cret = ilitek_i2c_read_cmd(gTpInfo.tpDevFd, ILITEK_TP_CMD_READ_DATA, tmpbuf, 32);
	if(i2cret<0)	return -1;

	//if 2 packets, then read more 20-bytes
	if(tmpbuf[0] == 2)	//has 2 packets
	{
    	ITPI2cInfo *e;
    	unsigned char	I2cCmd;

    	e = alloca(sizeof(ITPI2cInfo));

		e->slaveAddress   = gTpSpec.tpI2cDeviceId;
		e->cmdBuffer      = tmpbuf;
		e->cmdBufferSize  = 0;
		e->dataBuffer     = &tmpbuf[31];
		e->dataBufferSize = 20;
		i2cret = read(gTpInfo.tpDevFd, e, 1);
		if(i2cret<0)	return -1;
	}
	
	//Just copy the first 2 points that touch-down to buf(because buf has only 11-bytes)
	{
		int i=0;
		int	f_num = 6;	//packet 1 has 6 points
		int	down_num = 0;
		
		if(tmpbuf[0] == 2)	f_num = 10;	//packet 2 has 4 points
			
		for(i=0; i<f_num; i++)
		{
			if(tmpbuf[1 + i * 5] & 0x80)	//touch-down flag
			{
				memcpy(&buf[5*down_num + 1], &tmpbuf[5*i + 1], 5);
				down_num++;
			}
			if(down_num >= gTpSpec.tpSampleNum)	break;	//To limit the max output finger numbers
		}
		
		buf[0] = tmpbuf[0];
	}

	#ifndef	ENABLE_TOUCH_PANEL_DBG_MSG
	{
		int i,c = 32;

        if(tmpbuf[0]==2)	c += 20;
		printf("\n	raw-buf(c=%d):",cnt);
		for(i=0; i<c; i++)
		{
			printf("%02x ",tmpbuf[i]);
			if( (i&0x0F)==0x0F )	printf("\n		      ");
		}
		printf("\n\n");		
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
	int ret;
	int fgrNum = 0;
	char numOfTchPnt = 0;
	unsigned int lfnum = gLastNumFinger;
	struct ts_sample *smp = (struct ts_sample*)s;
	char loopNum = 0;

	//skip it if buf[0] = 0
	if(buf[0]==0)   return (-1);
	
	//only valid when buf[0] = 1 or 2
	if(buf[0] > 2)   return (-1);
	
	if( gTpSpec.tpSampleNum == 1)
	{
		numOfTchPnt = 1;
	}
	else
	{
		for(i=0; i<gTpSpec.tpSampleNum; i++)	if(buf[(i * 5) + 1] & 0x80)	numOfTchPnt++;
	}
	
	if(lfnum < numOfTchPnt)
	{
		//printf("fgr_chg++: %d, %d\n", lfnum, numOfTchPnt);
	}		
	
	if(numOfTchPnt)
	{		
		struct ts_sample *smp = (struct ts_sample*)s;		

		for(i = 0; i < gTpSpec.tpSampleNum; i++)
		{
			if(buf[(i * 5) + 1] & 0x80)	smp->pressure = 1;
			else	smp->pressure = 0;
				
			smp->id = (unsigned int)i; //touch id
			smp->finger = numOfTchPnt;
			smp->y = (short)(((unsigned int)(buf[i*5 + 1]&0x0F)<<8) + buf[i*5 + 2]);
			smp->x = (short)(((unsigned int)(buf[i*5 + 3]&0x0F)<<8) + buf[i*5 + 4]);

            //skip the sample that finger id > MAX_FINGER_NUM
			if( (smp->id >= gTpSpec.tpSampleNum) && (gTpSpec.tpSampleNum > 1) )
			{
			    //clear this sample & continue
			    smp->finger = 0;			    
			    smp->pressure = 0;
			    smp->x = 0;
			    smp->y = 0;
			    continue;
			}

			//printf("	RAW->[%d][%x, %d, %d]--> %d %d %d\n", i, smp, smp->id, smp->finger, smp->pressure, smp->x, smp->y);
			if(gTpSpec.tpSampleNum > 1)   smp++;
		}
		
		pthread_mutex_lock(&gTpMutex);
		gLastNumFinger = s->finger;
		pthread_mutex_unlock(&gTpMutex);			
	
		//exchange sample(only for ILI2511)
		if((gTpSpec.tpSampleNum > 1) && s->finger && !s->pressure)
		{
			struct ts_sample *s2=s;
			struct ts_sample tmp;
			s2++;
			tmp = s[0];
			s[0] = s2[0];
			s2[0] = tmp;
		}	
			
		return 0;
	}

	//printf("	raw-got fail~~\n");
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

#ifdef	CFG_TOUCH_BUTTON
static uint32_t _tpSendKeypadValue(void)
{
	uint32_t var=0;
    pthread_mutex_lock(&gTpMutex);	
    var = gTpKeypadValue;
    pthread_mutex_unlock(&gTpMutex);	
    
    //printf("SEND TP-KEY, Key Val =%x\n",var);
	return var;
}
#endif

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
static int ili2511_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
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

static const struct tslib_ops ili2511_ops =
{
	ili2511_read,
};

TSAPI struct tslib_module_info *ili2511_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &ili2511_ops;
	return m;
}

#ifndef TSLIB_STATIC_CASTOR3_MODULE
	TSLIB_MODULE_INIT(ili2511_mod_init);
#endif
