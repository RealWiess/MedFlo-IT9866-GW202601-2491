///*****************************************
//  Copyright (C) 2009-2019
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <it660x.h>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2019/10/31
//   @fileversion: ITE_HDMI1.4_RXSAMPLE_1.27
//******************************************/
#ifndef _it660x_H_
#define _it660x_H_


#ifndef _it660x_
#define _it660x_
#endif

#if 1//def _MCU_
    //#define _CODE code
    #include "IT660x_reg.h"
    #include "typedef.h"
    #include "Utility.h"
    #include "IO.h"
    #include "debug.h"
#else
    #define _CODE const
    #include "common.h"
    #include "i2c.h"
    #include "int.h"
    #include "digrxs.h"
    #include "rta_tsk.h"
    #include "rta_sem.h"
    #include "rta_pub.h"
    #include "ddp442x_rtos_include.h"
    #include "sysmon.h"
    #include "mailbox.h"
    #include "tmr.h"
    #include "dbmessage.h"
    #include "gpiofunction.h"
    #include "datapath.h"
    #include "src_3d.h"
    #include "detect3D.h"
    #include "defines.h"
    #include "digcontrol.h"
#endif



/*****************************************************************************/
/* Local Defines    **********************************************************/
/*****************************************************************************/
#define HDMIRX_MAX_KSV 8
//#define DISABLE_HDMI_CSC
#define Enable_Vendor_Specific_packet
//#define EN_DUAL_PIXEL_MODE	//2013-0520


//FIX_ID_003 xxxxx	//Add IT6602 Video Output Configure setting
// 0 eRGB444_SDR=0,
// 1	eYUV444_SDR,
// 2	eRGB444_DDR,
// 3	eYUV444_DDR,
// 4	eYUV422_Emb_Sync_SDR,
// 5	eYUV422_Emb_Sync_DDR,
// 6	eYUV422_Sep_Sync_SDR,
// 7	eYUV422_Sep_Sync_DDR,
// 8	eCCIR656_Emb_Sync_SDR,
// 9	eCCIR656_Emb_Sync_DDR,
// 10 eCCIR656_Sep_Sync_SDR,
// 11 eCCIR656_Sep_Sync_DDR,
// 12 eRGB444_Half_Bus,
// 13 eYUV444_Half_Bus,
// 14 eBTA1004_SDR,
// 15 eBTA1004_DDR
//06-27 disable --> #define HDMIRX_OUTPUT_VID_MODE (F_MODE_EN_UDFILT | F_MODE_RGB444)
#define HDMIRX_OUTPUT_VID_MODE CFG_HDMI_OUTPUT_MODE
//FIX_ID_003 xxxxx

#define MS_TimeOut(x) (x+1)

//FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
#define VSATE_CONFIRM_SCDT_COUNT        MS_TimeOut(100)
//FIX_ID_033 xxxxx

#ifdef _FIX_ID_028_
//xxxxx 2014-0417
//FIX_ID_028 xxxxx //For Debug Audio error with S2
#define AUDIO_READY_TIMEOUT                 	MS_TimeOut(0)	// change 100ms to 0 for speed up audio on
//FIX_ID_028 xxxxx
//xxxxx 2014-0417
#else
//FIX_ID_023 xxxxx		//Fixed for Audio Channel Status Error with invalid HDMI source
#define AUDIO_READY_TIMEOUT                 	MS_TimeOut(200)
//FIX_ID_023 xxxxx
#endif
#define AUDIO_MONITOR_TIMEOUT              MS_TimeOut(150)

#define SCDT_OFF_TIMEOUT              		MS_TimeOut(20)		//100 x MS_LOOP = 5000 ms = 5 sec
#define ECC_TIMEOUT              				MS_TimeOut(20)
#define DESKEW_TIMEOUT            			MS_TimeOut(20)



// Debug Mode

#define  DDCCBusDbgCtrl 	FALSE
#define  RCLKFreqSel 	1	//; //0: RING/2; 1: RING/4; 2: RING/8; 3: RING/16
#define GenPktRecType	0x81


//#ifdef _SelectExtCrystalForCbus_
#define T10usSrcSel   TRUE	//FALSE: 100ms calibration , TRUR: 27MHz Crystal(only IT6602)


#define EnPktFIFOBurst	TRUE
// DDC Option
#define EnDDCSendAbort	TRUE  // Send ABORT after segment write with EOF

#define  HDCPIntKey   FALSE   //TRUE: Internal HDCP Key, FALSE: SIPROM

#define  VCLK_INV	0
#define  VCLK_DLY	0
#define  EnMultiSeg     TRUE
#define  EnIntEDID      TRUE


//FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
#define EQRETRYFAILCNT 1	// for EQ interrupt
#define RCLKVALUE 12			// for show TMDS and Pixel Clk
#define TMDSCLKVALUE 160	// for TMDS > 160 then set RS to 00, otherwise set to 3F

#define TMDSCLKVALUE_1080P 160	// for TMDS > 160 then set RS to 00, otherwise set to 3F
#define TMDSCLKVALUE_480P 35
#define TMDSCLKVALUE_MHL_ER1 90
#define JUDGE_ER1_VALUE 90


//#ifndef _SelectExtCrystalForCbus_
#define _RCLK_FREQ_20M  FALSE
//#endif
//FIX_ID_004 xxxxx


//FIX_ID_014 xxxx
#define MAX_TMDS_WAITNO 		(350/MS_LOOP)		// 400ms
#define MAX_HDCP_WAITNO 		(100/MS_LOOP)		// 150ms
//FIX_ID_014 xxxx


#define DeltaNum 	1
#define DEFAULT_EQVALUE 0x1F

//FIX_ID_052 DCLK DELAY DIFF FOR CSC ON AND OFF
#define DCLK_DELAY_CSC_ON  0x02
#define DCLK_DELAY_CSC_OFF 0x00
//~FIX_ID_052

#if 1



#define _SUPPORT_HDCP_				FALSE
#define _SUPPORT_EDID_RAM_		TRUE

//FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
#define _SUPPORT_AUTO_EQ_                        TRUE
#define _SUPPORT_EQ_ADJUST_                     TRUE
//FIX_ID_001 xxxxx

#define FIX_ID_013
#define FIX_ID_013_

/*****************************************************************************/
/* Type defs enums  **********************************************************/
/*****************************************************************************/
//FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
//xxxxx 2014-0421 modify 50 to 100
#define MS_LOOP                 100
//xxxxx 2014-0421
//FIX_ID_033 xxxxx

//FIX_ID_003 xxxxx	//Add IT6602 Video Output Configure setting
typedef enum _Video_Output_Configure
{
    eRGB444_SDR=0,
    eYUV444_SDR,
    eRGB444_DDR,
    eYUV444_DDR,
    eYUV422_Emb_Sync_SDR,
    eYUV422_Emb_Sync_DDR,
    eYUV422_Sep_Sync_SDR,
    eYUV422_Sep_Sync_DDR,
    eCCIR656_Emb_Sync_SDR,
    eCCIR656_Emb_Sync_DDR,
    eCCIR656_Sep_Sync_SDR,
    eCCIR656_Sep_Sync_DDR,
    eRGB444_Half_Bus,
    eYUV444_Half_Bus,
    eBTA1004_SDR,
    eBTA1004_DDR,
    eVOMreserve
}Video_Output_Configure;

typedef enum _Video_OutputDataTrigger_Mode
{
    eSDR=0,
    eHalfPCLKDDR,
    eHalfBusDDR,
    eSDR_BTA1004,
    eDDR_BTA1004
}Video_DataTrigger_Mode;

typedef enum _Video_OutputSync_Mode
{
    eSepSync=0,
    eEmbSync,
    eCCIR656SepSync,
    eCCIR656EmbSync
}Video_OutputSync_Mode;

//FIX_ID_003 xxxxx

typedef enum _Video_State_Type {
    VSTATE_Off=0,
    VSTATE_TerminationOff,
    VSTATE_TerminationOn,
    VSTATE_5VOff,
    VSTATE_SyncWait,
    VSTATE_SWReset,
    VSTATE_SyncChecking,
    VSTATE_HDCPSet,
    VSTATE_HDCP_Reset,
    VSTATE_ModeDetecting,
    VSTATE_VideoOn,
    VSTATE_ColorDetectReset,
    VSTATE_HDMI_OFF,
    VSTATE_Unknown,
    VSTATE_Reserved
} Video_State_Type;


typedef enum _Audio_State_Type {
    ASTATE_AudioOff = 0,
    ASTATE_RequestAudio ,
    ASTATE_ResetAudio,
    ASTATE_WaitForReady,
    ASTATE_AudioOn ,
    ASTATE_Unknown,
    ASTATE_Reserved
} Audio_State_Type;


typedef enum _RxHDCP_State_Type {
    RxHDCP_PwrOff=0,
    RxHDCP_ModeCheck,
    RxHDCP_Receiver,
    RxHDCP_Repeater,
    RxHDCP_SetKSVFifoList,
    RxHDCP_GenVR,
    RxHDCP_WriteVR,
    RxHDCP_Auth_WaitRi,
    RxHDCP_Authenticated,
    RxHDCP_Reserved
} RxHDCP_State_Type;





#define F_MODE_RGB24  0
#define F_MODE_RGB444  0
#define F_MODE_YUV422 1
#define F_MODE_YUV444 2
//FIX_ID_044  add YCbCr420 supporting
#define F_MODE_YUV420 3 // CEA-861F defined YUV420 3
//~FIX_ID_044
#define F_MODE_CLRMOD_MASK 3
#define F_MODE_ITU709  (1<<4)
#define F_MODE_ITU601  0
#define F_MODE_0_255   0
#define F_MODE_16_235  (1<<5)
#define F_MODE_EN_UDFILT (1<<6)
#define F_MODE_EN_DITHER (1<<7)

#define RCVABORT        2
#define RCVNACK         3
#define ARBLOSE         4
#define FWTXFAIL        5
#define FWRXPKT         6
#define FAIL			-1
#define ABORT           -2

#define HIGH			1
#define LOW				0

#define CD8BIT			4
#define CD10BIT	 		5
#define CD12BIT			6
#define CD16BIT			7

#define OUT8B           0
#define OUT10B          1
#define OUT12B          2

#define RGB444			0
#define YCbCr422		1
#define YCbCr444		2

#define NORM            0
#define FAST            1
#define SLOW            2

#define AUD32K			0x3
#define AUD48K			0x2
#define AUD96K			0xA
#define AUD192K			0xE
#define AUD44K			0x0
#define AUD88K			0x8
#define AUD176K			0xC

#define I2S				0
#define SPDIF			1


#define EDID_CHG        0x01

#define MUTED           0x10

#define HDMI 0

#define BUS10B 1
#define BUS20B 0


// FIX_ID_043
#define MAX_TOGGLE_FAIL_COUNT 16
//~FIX_ID_043

#define SUPPORT_INPUTRGB
#define SUPPORT_INPUTYUV444
#define SUPPORT_INPUTYUV422

#if defined(SUPPORT_INPUTYUV444)|| defined(SUPPORT_INPUTYUV422)
#define SUPPORT_INPUTYUV
#endif


#define SUPPORT_OUTPUTYUV
#define SUPPORT_OUTPUTYUV444
#define SUPPORT_OUTPUTYUV422
//FIX_ID_044  add YCbCr420 supporting
#define SUPPORT_OUTPUTYUV420
//~FIX_ID_044

#if (defined(SUPPORT_OUTPUTYUV444))||(defined(SUPPORT_OUTPUTYUV422))
#define SUPPORT_OUTPUTYUV
#endif

#define F_PORT_SEL_0      0
#define F_PORT_SEL_1      1

//###ADD define###
#define P2_0 1 //Add P2_0 for switch Exteranl 24c04 EEPROM and Internal IT6602 EDID RAM
//###ADD define###


#endif


#if 1
/*****************************************************************************/
/* Type defs struct **********************************************************/
/*****************************************************************************/
struct IT6602_REG_INI
{
    unsigned char ucAddr;
    unsigned char andmask;
    unsigned char ucValue;
};


struct IT6602_VIDEO_CONFIGURE_REG
{
    unsigned char ucReg51;
    unsigned char ucReg65;
};

typedef struct _3D_SourceConfiguration
{
    unsigned char          Format;              /**< Type of 3D source format expected or found.                                                        */
    unsigned char    LR_Reference;        /**< Source of the 3D L/R reference.                                                                    */
    unsigned char FrameDominance;      /**< Left or Right Eye is first in L/R image pair.                                                      */
    unsigned char     LR_Encoding;         /**< Type of 3D L/R encoding expected or detected.                                                      */
    unsigned char    TB_Reference;        /**< Top/Bottom reference for vertically sub-sampled sources.                                           */
    unsigned char    OE_Reference;        /**< Odd/Even reference for horizontally sub-sampled sources.                                           */
    unsigned char NumActiveBlankLines;                 /**< Number of lines separating vertically packed L/R data to be removed (cropped)
                                                  *  before being displayed. Does not include any embedded encoding.                                    */
    unsigned char NumberOfEncodedLines;                /**< Number of encoded lines in one L/R eye frame of the display data
                                                  *  to be blanked out with "Blanking Color". (assumed same number in second eye frame)                 */
    unsigned int  LeftEncodedLineLocation;             /**< Active line number of 1st encoded line in one Left eye frame of the display data (-1=unknown).     */
    unsigned int  RightEncodedLineLocation;            /**< Active line number of 1st encoded line in one Right eye frame of the display data (-1=unknown).
                                                  *  If format is Horizontally Packed, set RightEncodedLineLocation=LeftEncodedLineLocation             */
    unsigned char BlankingColor;                 /**< Color to use when blanking (or masking off) any embedded L/R encoding.                             */
} SRC_3D_SOURCE_CONFIG;

typedef struct _de3dframe
{
    unsigned char VIC;
    unsigned char HB0;
    unsigned char HB1;
    unsigned char HB2;
    unsigned char PB0;
    unsigned char PB1;
    unsigned char PB2;
    unsigned char PB3;
    unsigned char PB4;
    unsigned char PB5;
    unsigned char PB6;
    unsigned char PB7;
} DE3DFRAME;

typedef struct _set_de3d_frame
{
    unsigned char Vic;
    unsigned int V_total;        // Vtotal -1
    unsigned int V_act_start;    // VTotal -1 + Vactive_start -1
    unsigned int V_act_end;      // Vactive end -1
    unsigned int V_sync_end;     // LSB(Vtotal -1 + sync With)
    unsigned int V_2D_active_total;     // V_2D_active_total

} SET_DE3D_FRAME;

typedef enum
{
    VSYNC_SEPARATED_HALF,       /**< VSync separated (field sequential) format.                         */
    VSYNC_SEPARATED_FULL,       /**< VSync separated (frame sequential progressive) format.             */
    VERT_PACKED_HALF,           /**< Over Under (vertically packed) half resolution format.             */
    VERT_PACKED_FULL,           /**< Over Under (vertically packed) full resolution format.             */
    HORIZ_PACKED_HALF,          /**< Side by Side (horizontally packed) half resolution format.         */
    HORIZ_PACKED_FULL,          /**< Side by Side (horizontally packed) full resolution format.         */
    UNDEFINED_FORMAT            /**< Undefined format.                                                  */
} SRC_3D_FORMAT;

//FIX_ID_010 xxxxx 	//Add JudgeBestEQ to avoid wrong EQ setting
#define MaxEQIndex 3
//FIX_ID_010 xxxxx

//FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
#ifdef _SUPPORT_EQ_ADJUST_
struct IT6602_eq_data
{
unsigned char ucEQState;
unsigned char ucAuthR0;								//20130327 for R0 fail issue
unsigned char ucECCvalue;								//20130328 for acc ecc error
unsigned char ucECCfailCount;							//20130328 for acc ecc error
unsigned char ucPkt_Err;
unsigned char ucPortID;
unsigned char f_manualEQadjust;
//FIX_ID_010 xxxxx 	//Add JudgeBestEQ to avoid wrong EQ setting
unsigned char ErrorCount[MaxEQIndex];
//FIX_ID_010 xxxxx
};
#endif
//FIX_ID_001 xxxxx





//FIX_ID_014 xxxxx
    #define B_PORT1_TimingChgEvent	0x40
    #define B_PORT1_TMDSEvent	0x20
    #define B_PORT1_Waiting		0x10
    #define B_PORT0_TimingChgEvent	0x04
    #define B_PORT0_TMDSEvent	0x02
    #define B_PORT0_Waiting		0x01
//FIX_ID_014 xxxxx


typedef enum _PARSE3D_STA{
    PARSE3D_START,
    PARSE3D_LEN,
    PARSE3D_STRUCT_H,
    PARSE3D_STRUCT_L,
    PARSE3D_MASK_H,
    PARSE3D_MASK_L,
    PARSE3D_VIC,
    PARSE3D_DONE
}PARSE3D_STA;


struct PARSE3D_STR
{
    unsigned char	uc3DEdidStart;
    unsigned char	uc3DBlock;
    unsigned char	uc3DInfor[32];
    unsigned char	ucVicStart;
    unsigned char	ucVicCnt;
    unsigned char	uc3DTempCnt;
    unsigned char	ucDtdCnt;
    unsigned char	bVSDBspport3D;
};
//FIX_ID_013	xxxxx


struct AVI_info
{
    unsigned char ColorMode;
    unsigned char Colorimetry;
    unsigned char ExtendedColorimetry;
    unsigned char RGBQuantizationRange;
    unsigned char YCCQuantizationRange;
    unsigned char VIC;
    //unsigned char PixelRepetition;
};


struct IT6602_dev_data
{
    Video_State_Type m_VState;
    Audio_State_Type m_AState;
    RxHDCP_State_Type m_RxHDCPState;
    AUDIO_CAPS m_RxAudioCaps;
    unsigned short m_SWResetTimeOut;
    unsigned short m_VideoCountingTimer;
    unsigned short m_AudioCountingTimer;
    unsigned char m_ucCurrentHDMIPort;
    unsigned char m_bOutputVideoMode;
    unsigned char m_bInputVideoMode;

//FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
#ifdef _AVOID_REDUNDANCE_CSC_
    unsigned char m_Backup_OutputVideoMode;
    unsigned char m_Backup_InputVideoMode;
#endif
//FIX_ID_039 xxxxx

    unsigned char m_ucSCDTOffCount;
    unsigned char m_ucEccCount_P0;
    unsigned char m_ucEccCount_P1;
    unsigned char m_ucDeskew_P0;
    unsigned char m_ucDeskew_P1;

    SRC_3D_SOURCE_CONFIG de3dframe_config;
    DE3DFRAME s_Current3DFr;

    unsigned char oldVIC;
    unsigned char newVIC;
    unsigned char  f_de3dframe_hdmi;

//FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_EQ_ADJUST_
    struct IT6602_eq_data EQPort[2];
    // FIX_ID_043
    unsigned char ToggleCount[2];
    //~FIX_ID_043
    #endif
//FIX_ID_001 xxxxx

    //FIX_ID_003 xxxxx	//Add IT6602 Video Output Configure setting
    Video_Output_Configure m_VidOutConfigMode;
    Video_DataTrigger_Mode m_VidOutDataTrgger;
    Video_OutputSync_Mode m_VidOutSyncMode;
    //FIX_ID_003 xxxxx

//FIX_ID_014 xxxxx	//Add Cbus Event Handler
    unsigned char HDMIIntEvent;
    unsigned char HDMIWaitNo[2];
//FIX_ID_014 xxxxx


    //AVI_info m_avi;
    unsigned char ColorMode;
    unsigned char Colorimetry;
    unsigned char ExtendedColorimetry;
    unsigned char RGBQuantizationRange;
    unsigned char YCCQuantizationRange;
    unsigned char VIC;



    unsigned char m_HDCP_ContentOff;
    //xxxxx

    unsigned char GCP_CD;	//Output Color Depth
    unsigned char  DE3DFormat_HDMIFlag:1;
    unsigned char  FramePacking_Flag:1;
    unsigned char  TopAndBottom_Flag:1;
    unsigned char  SideBySide_Flag:1;

    #ifdef _IT6607_GeNPacket_Usage_
    BYTE m_PollingPacket;
    BYTE m_PacketState;
    BYTE m_ACPState;
    BYTE m_GeneralRecPackType;
    BYTE m_GamutPacketRequest:1;
    #endif

//#if(_SUPPORT_HDCP_)
//    //HDCP
//    unsigned char HDCPEnable;
//    HDCPSts_Type Hdcp_state;
//    unsigned int HDCPFireCnt;
//#endif
    // 2015/07/01 added by jjtseng
    // for solving the new AVI interrupt issued by Audio Force MUTE.
    unsigned char prevAVIDB[2] ;



    unsigned char m_bRxAVmute:1;
    unsigned char m_bVideoOnCountFlag:1;
    unsigned char m_MuteAutoOff:1;
    unsigned char m_bUpHDMIMode:1;
    unsigned char m_bUpHDCPMode:1;
    unsigned char m_NewAVIInfoFrameF:1;
    unsigned char m_NewAUDInfoFrameF:1;
    unsigned char m_HDCPRepeater:1;
    unsigned char m_MuteByPKG:1;
    // pit660xCallbackList_t	pCallbackFunctionsList;
    unsigned char m_bHDCPrepeater:1;


#ifdef _SUPPORT_HDMI_REPEATER_
    unsigned char m_sinkHPD ;
#endif // _SUPPORT_HDMI_REPEATER_

};

#endif


/*****************************************************************************/
/* Error codes ***************************************************************/
/*****************************************************************************/
/* Error code specifying an I2C driver error */
#define it660x_I2C_DRIVER_ERROR                     -4
#define it660x_I2C_DRIVER_TERMINAL_ERROR            -5

/* Error code specifying RTA errors */
#define it660x_RTA_SEM_IN_USE                       -6
#define it660x_RTA_ERROR                            -7

/* Error code specifying other errors */
#define it660x_DATA_INVALID                         -8

/*****************************************************************************/
/* Public function prototypes ************************************************/
/*****************************************************************************/


/* it660x IO Functions   ***********************************************************/
/* it660x Configuration and Initialization ***********************************/
void IT6602_fsm_init(void);
/* HDMI RX functions   *********************************************************/
void IT6602PortSelect(unsigned char ucPortSel);
void IT6602HPDCtrl(unsigned char ucport,unsigned char ucEnable);
/* HDMI Audio function    *********************************************************/
/* HDMI Video function    *********************************************************/
/* HDMI Interrupt function    *********************************************************/


void IT6602_fsm(void);

void IT6602ChangeTTLVideoOutputMode(void);
void  Dump_it660xReg(void);
void debugEQ(unsigned char ucCmd);
void debugxxx(void);

void show_vid_info( void );
void get_vid_info( void );


#ifdef SUPPORT_UART_CMD
extern unsigned char m_UartCmd;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////////////////////
unsigned char hdmirxrd( unsigned char RegAddr);
unsigned char hdmirxwr( unsigned char RegAddr,unsigned char DataIn);
unsigned char  hdmirxset( unsigned char  offset, unsigned char  mask, unsigned char  ucdata );
void hdmirxbwr( unsigned char offset, unsigned char byteno, unsigned char *rddata );



SYS_STATUS EDID_RAM_Write(unsigned char offset,unsigned char byteno,unsigned char *p_data );
unsigned char EDID_RAM_Read(unsigned char offset);
/* it660x Configuration and Initialization ***********************************/
struct IT6602_dev_data* get_IT6602_dev_data(void);
void hdimrx_write_init(struct IT6602_REG_INI _CODE *tdata);


//FIX_ID_003 xxxxx	//Add IT6602 Video Output Configure setting
void IT6602_VideoOutputConfigure_Init(struct IT6602_dev_data *IT6602,Video_Output_Configure eVidOutConfig);
//FIX_ID_003 xxxxx

void hdmirx_Var_init(struct IT6602_dev_data *IT6602);
void IT6602_Rst( struct IT6602_dev_data *IT6602 );
//FIX_ID_002 xxxxx 	Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
void IT6602_Identify_Chip(void);
//FIX_ID_002 xxxxx
//void IT6602_fsm_init(void);


/* HDMI RX functions   *********************************************************/
void chgbank(int bank);
unsigned char CheckSCDT(struct IT6602_dev_data *IT6602);
void WaitingForSCDT(struct IT6602_dev_data *IT6602);
unsigned char CLKCheck(unsigned char ucPortSel);

//FIX_ID_009 xxxxx	//verify interrupt event with reg51[0] select port
unsigned char  IT6602_IsSelectedPort(unsigned char ucPortSel);
//FIX_ID_009 xxxxx

//FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
#ifdef _SUPPORT_EQ_ADJUST_
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void HDMIStartEQDetect(struct IT6602_eq_data *ucEQPort);
void HDMISetEQValue(struct IT6602_eq_data *ucEQPort,unsigned char ucIndex);
void HDMISwitchEQstate(struct IT6602_eq_data *ucEQPort,unsigned char state);
void HDMICheckSCDTon(struct IT6602_eq_data *ucEQPort);
void HDMIPollingErrorCount(struct IT6602_eq_data *ucEQPort);
void HDMIJudgeECCvalue(struct IT6602_eq_data *ucEQPort);
void HDMIAdjustEQ(struct IT6602_eq_data *ucEQPort);
//FIX_ID_010 xxxxx 	//Add JudgeBestEQ to avoid wrong EQ setting
void JudgeBestEQ(struct IT6602_eq_data *ucEQPort);
void StoreEccCount(struct IT6602_eq_data *ucEQPort);
//FIX_ID_010 xxxxx
void IT6602VideoCountClr(void);
//-------------------------------------------------------------------------------------------------------
#endif

#ifdef _SUPPORT_AUTO_EQ_
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void DisableOverWriteRS (unsigned char ucPortSel);
void AmpValidCheck (unsigned char ucPortSel);
void TogglePolarity (unsigned char ucPortSel);
void TMDSCheck(unsigned char ucPortSel);
void OverWriteAmpValue2EQ (unsigned char ucPortSel);
//-------------------------------------------------------------------------------------------------------
#endif
//FIX_ID_001 xxxxx
unsigned char CheckAVMute(void);
unsigned char CheckPlg5VPwr(unsigned char ucPortSel);
//FIX_ID_037 xxxxx
unsigned char IsHDMIMode(void);
void GetAVIInfoFrame(struct IT6602_dev_data *IT6602);
void SetVideoInputFormatWithInfoFrame(struct IT6602_dev_data *IT6602);
void SetColorimetryByInfoFrame(struct IT6602_dev_data *IT6602);
//static void SetCSCBYPASS(struct IT6602_dev_data *IT6602);
void SetColorSpaceConvert(struct IT6602_dev_data *IT6602);
void SetNewInfoVideoOutput(struct IT6602_dev_data *IT6602);
void SetVideoInputFormatWithoutInfoFrame(struct IT6602_dev_data *IT6602,unsigned char bInMode);
void SetColorimetryByMode(struct IT6602_dev_data *IT6602);
void SetDVIVideoOutput(struct IT6602_dev_data *IT6602);

//FIX_ID_003 xxxxx	//Add IT6602 Video Output Configure setting
void IT6602_VideoOutputModeSet(struct IT6602_dev_data *IT6602);
//FIX_ID_003 xxxxx

void IT6602VideoOutputConfigure(struct IT6602_dev_data *IT6602);
void SetVideoOutputColorFormat(struct IT6602_dev_data *IT6602);
//void IT6602PortSelect(unsigned char ucPortSel);
void IT6602_HalfPCLKFinetune(struct IT6602_dev_data *IT6602); //ITE_150409
void it660x_StandbyMode(unsigned char ucstate);//ITE_151021

void hdmirx_ECCTimingOut(unsigned char ucport);

/* HDMI Audio function    *********************************************************/
void aud_fiforst( void );
void IT6602AudioOutputEnable(unsigned char bEnable);
void hdmirx_ResetAudio(void);
void hdmirx_SetHWMuteClrMode(void);
void hdmirx_SetHWMuteClr(void);
void hdmirx_ClearHWMuteClr(void);
void getHDMIRXInputAudio(AUDIO_CAPS *pAudioCaps);
void IT6602SwitchAudioState(struct IT6602_dev_data *IT6602,Audio_State_Type state);
#ifdef _FIX_ID_028_
//FIX_ID_028 xxxxx //For Debug Audio error with S2
// remove --> static void IT6602AudioHandler(struct IT6602_dev_data *IT6602);
//FIX_ID_028 xxxxx
#else
void IT6602AudioHandler(struct IT6602_dev_data *IT6602);
#endif

#ifdef EnableCalFs
//FIX_ID_023 xxxxx		//Fixed for Audio Channel Status Error with invalid HDMI source
void AudioFsCal(void);
unsigned int PCLKGet(void);
void TMDSGet(void);
void DumpNCTSReg(void);
//FIX_ID_023 xxxxx
#endif
unsigned char OSCvalueCompare(unsigned long *calibrationValue);
unsigned long CP_OCLK( void );

/* HDMI Video function    *********************************************************/
void IT6602_AFE_Rst( void );

//xxxxx 2014-0529 //Content On/Off
void IT6602_HDCP_ContentOff(unsigned char ucPort , unsigned char bOff);
//xxxxx 2014-0529
//FIX_ID_037 xxxxx

void IT6602_SetVideoMute(struct IT6602_dev_data *IT6602,unsigned char bMute);
//static void IT6602VideoOutputCDSet(void);
void IT6602VideoOutputEnable(unsigned char bEnableOutput);
void IT6602VideoCountClr(void);
void IT6602SwitchVideoState(struct IT6602_dev_data *IT6602,Video_State_Type  eNewVState);
void IT6602VideoHandler(struct IT6602_dev_data *IT6602);
void it660x_StandbyMode(unsigned char ucstate);//ITE_150408


/* HDMI Interrupt function    *********************************************************/
void hdmirx_INT_5V_Pwr_Chg(struct IT6602_dev_data *IT6602,unsigned char ucport);
void hdmirx_INT_P0_ECC(struct IT6602_dev_data *IT6602);
void hdmirx_INT_P1_ECC(struct IT6602_dev_data *IT6602);
void hdmirx_INT_P0_Deskew(struct IT6602_dev_data *IT6602);
void hdmirx_INT_P1_Deskew(struct IT6602_dev_data *IT6602);
//FIX_ID_009 xxxxx	//verify interrupt event with reg51[0] select port
void hdmirx_INT_HDMIMode_Chg(struct IT6602_dev_data *IT6602,unsigned char ucport);
//FIX_ID_009 xxxxx
void hdmirx_INT_SCDT_Chg(struct IT6602_dev_data *IT6602);

//FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
#ifdef _SUPPORT_AUTO_EQ_
void hdmirx_INT_EQ_FAIL(struct IT6602_dev_data *IT6602,unsigned char ucPortSel);
#endif
//FIX_ID_001 xxxxx


// disable -> static void hdcpsts( void );


/* EDID RAM  functions    *******************************************************/
#ifdef _SUPPORT_EDID_RAM_
//static unsigned char UpdateEDIDRAM(_CODE unsigned char *pEDID,unsigned char BlockNUM);
unsigned char UpdateEDIDRAM(unsigned char *pEDID,unsigned char BlockNUM);
void EnableEDIDupdate(void);
void DisableEDIDupdate(void);
//static void EDIDRAMInitial(_CODE unsigned char *pIT6602EDID);
void EDIDRAMInitial(unsigned char *pIT6602EDID);
//static unsigned char Find_Phyaddress_Location(_CODE unsigned char *pEDID,unsigned char Block_Number);
unsigned char Find_Phyaddress_Location(unsigned char *pEDID,unsigned char Block_Number);
void UpdateEDIDReg(unsigned char u8_VSDB_Addr, unsigned char CEC_AB, unsigned char CEC_CD, unsigned char Block1_CheckSum);
void PhyAdrSet(void);
#endif
void IT6602HPDCtrl(unsigned char ucport,unsigned char ucEnable);
unsigned long CP_OCLK( void );
void Cal_oclk( void );
void IPLLOvw_chk(int port);

void IT6602HDMIInterruptHandler(struct IT6602_dev_data *IT6602);
//void IT6602_fsm(void);


#ifndef Enable_IR
void IT6602AutoPortSelect(struct IT6602_dev_data *IT6602);
#endif
// disable -> static void IT6602ShowErrorCount(void);
//void show_vid_info( void );
//void get_vid_info( void );

#ifdef Enable_Vendor_Specific_packet
void Dump3DReg(void);
unsigned char IT6602_DE3DFrame(unsigned char ena_de3d);
#endif

#ifdef 	_SUPPORT_HDMI_REPEATER_
	#include "it660x_repeater.h"
#endif

//###ADD FUNCTION###
void IT6602VideoOutputEnable(unsigned char bEnableOutput);
void IT6602AudioOutputEnable(unsigned char bEnable);
unsigned char IsVideoOn(void);
unsigned int IT6602getHorzTotal(void);
unsigned int IT6602getHorzActive(void);
unsigned int IT6602getHorzFrontPorch(void);
unsigned int IT6602getHorzSyncWidth(void);
unsigned int IT6602getHorzBackPorch(void);
unsigned int IT6602getVertTotal(void);
unsigned int IT6602getVertActive(void);
unsigned int IT6602getVertFrontPorch(void);
unsigned int IT6602getVertSyncWidth(void);
unsigned int IT6602getVertBackPorch(void);
unsigned int IT6602getOutputColorDepth(void);
unsigned int IT6602getIsInterlace(void);
unsigned int IT6602getSCDT(void);
unsigned int IT6602getECC(void);
unsigned int IT6602getEQEND(void);
unsigned int IT6602getPCLK(void);
unsigned int IT6602getHDMIMode(void);
unsigned int IT6602getDAR(void);
unsigned int IT6602getOutputVideoMode(void);
unsigned int IT6602getOutputColorRange(void);
unsigned int IT6602getColorRangeChange(void);
void         IT6602setHDCPStatus(unsigned int OnOff);
unsigned int IT6602getHDCPStatus(void);
unsigned int IT6602getIsAudioOn(void);
unsigned int IT6602getIsAudioReset(void);
unsigned int IT6602getAudioSampleFrq(void);
unsigned int IT6602getAudioCannelNum(void);
unsigned int IT6602getAudioMode(void);
//###ADD FUNCTION###


#endif // _it660x_H_
