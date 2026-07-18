///*****************************************
//  Copyright (C) 2009-2019
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <it660x.c>
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2019/10/31
//   @fileversion: ITE_HDMI1.4_RXSAMPLE_1.27
// ******************************************/

// FIX_ID_001       //Dr. Liu suggestion to enable Auto EQ with Manual EQ to avoid some special HDMI cable issue.
// FIX_ID_002       //Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
// FIX_ID_003       //Add IT6602 Video Output Configure setting
// FIX_ID_004       //Add 100ms calibration for Cbus
// FIX_ID_005       //Add Cbus Event Handler
// FIX_ID_006       //Add P2_0 for switch Exteranl 24c04 EEPROM and Internal IT6602 EDID RAM
// FIX_ID_007       //for debug IT6681 HDCP issue7
// FIX_ID_008       //Add SW reset when HDMI / MHL device un-plug  !!!
// FIX_ID_009       //Verify interrupt event with reg51[0] select port
// FIX_ID_010       //Add JudgeBestEQ to avoid wrong EQ setting
// FIX_ID_011       //Use FW send PATH_EN{Sink}=1
// FIX_ID_012       //For SamSung Galaxy Note wake up fail iss ue
// FIX_ID_013       //For MSC 3D request issue
// FIX_ID_014       //For HDCP Auth Start with EQ Adjust issue
// FIX_ID_015       //Add RCP timeout mechanism for peer device no RCPK or RCPE response
// FIX_ID_016       //Support Dual Pixel Mode for IT6803 Only
// FIX_ID_017       //Disable IPLockChk
// FIX_ID_018       //modify 1K pull-down to 1.033K ohm HDMI Reg1C0[3:2]=2
// FIX_ID_019       //modify ENHYS control for MHL mode
// FIX_ID_020       //Turn off DEQ for HDMI port 1 with 20m DVI Cable
// FIX_ID_021       //To use CP_100ms calibration for CBus and CEC
// FIX_ID_022       //Fixed for CEC capacitor issue
// FIX_ID_023       //Fixed for Audio Channel Status Error with invalid HDMI source
// FIX_ID_024       //Fixed for RCP compliance issue
// FIX_ID_025       //Adjust H/W Mute time
// FIX_ID_026       //Support RB SWAP for TTL output
// FIX_ID_027       //Support RGB limited / Full range convert
// FIX_ID_028       //For Debug Audio error with S2
// FIX_ID_029       //fixed Ulta-2000 HDCP fail issue at Receiver mode
// FIX_ID_030       //fixed video lost at 640x480 timing
// FIX_ID_031       //change CBus pull down 1K after 75ms power 5V stable
// FIX_ID_032       //Support HDCP Repeater function for HDMI Tx device
// FIX_ID_033       //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
// FIX_ID_034       //Add MHL HPD Control by IT6602HPDCtrl( )
// FIX_ID_035       //For MTK6592 HDMI to SII MHL TX compliance issue
// FIX_ID_036       //Enable MHL Function for IT68XX
// FIX_ID_037       //Allion MHL compliance issue !!!
// FIX_ID_038       //Add Reg8B[1] reset for IT6803 Dual Pixel mode
// FIX_ID_039       //fix image flick when enable RGB limited / Full range convert
// FIX_ID_040       //fix MSC 3D request issue for Huawei Mobile and Action Micro Dongle
// FIX_ID_041       //Add EDID reset
// FIX_ID_042       //Disable HDCP 1.1 feature to avoid compilance issue from ilegal HDCP 1.1 source device
// FIX_ID_043        // Limit Auto EQ Polarity Toggle Count by Special Pattern.
// FIX_ID_044        // Add YUV420 support
// omit FIX_ID_045        // 4.3.5.1 change MHL 2A[2:1] = 00 -> 11
// FIX_ID_046        // MHL version changed from 2.0 -> 2.2
// FIX_ID_047        // FIX 3D report string.
// FIX_ID_048        // 4.3.5.1 fail because of HPD not tri-state
// FIX_ID_049        // 4.3.5.1 change MHL 2A[2:1] = 11 -> 00
// FIX_ID_050        // 6.3.11.23 CBM: DUT Receives (0x68) MSC_MSG - Sub-command Illegal
// if not process UCP error, need to reply the UCP command ACK.
// FIX_ID_051        // unify CEC timer with MHL timer
// FIX_ID_052        // DIFF THE DCLK Delay under CSC power on and power down.
// FIX_ID_053        // Change AIO Strength for some weak connectivity. default value -> 0xAA
// FIX_ID_054        // seperate EDID into a dedicated file.
// FIX_ID_055        // it660x_Init_HDMIRepeater add option for enable/disable HDCP repeater
// FIX_ID_056        // add it660x_Enable_HDCPRepeater() for enable/disable HDCP repeater dynamically.
// FIX_ID_057        // 6.3.11.23 CBM: DUT Receives (0x68) MSC_MSG - Sub-command Illegal
// if not process UCP error, need to reply the UCP command ACK.
// if not defined _SUPPORT_UCP_,
// #define MSCRxUCP2Nack    FALSE
// FIX_ID_058        // mhl_parse_RCPkey() should ignore the "release" status.
// FIX_ID_059        // Fix bug for IT6602->prevAVIDB[1] to sync the register value(Reg215), or the interrupt "Reg09h&0x01" may not be triggered correctly

// 2014-0527 MHL compliance issue !!!
// REG_RX_1C0 : 0x80 [7] PWSB_LV =0 , [3:2] 1K resistance
// REG_RX_1BB : 0x0A [4] Cbus Schimitter trigger =0
//
//
/*****************************************************************************/
/* Header Files Included *****************************************************/
/*****************************************************************************/

#include "hdmirx/it66021/it660x.h"
#include "hdmirx/it66021/IO.h"
#include "hdmirx/it66021/config.h"

// #define OUTPUT_COLOR_DEPTH OUT8B
// #define OUTPUT_COLOR_DEPTH OUT10B
#define OUTPUT_COLOR_DEPTH OUT12B

/*****************************************************************************/
/* Private and Local Variables    ********************************************/
/*****************************************************************************/

struct IT6602_dev_data  IT6602DEV;
unsigned long           m_ROSCCLK;

unsigned char           V3D_EntryCnt = 0;
unsigned char           wrburstoff, wrburstnum;
unsigned char           TxWrBstSeq      = 0;

unsigned char           wakeupcnt       = 0;
#define MinEQValue 0x03
unsigned char           CLKStbChkCnt    = 0;    // ite_180509
unsigned int            ucHDCP_Satus    = 0;    // [2020 03 19 bu5 modify]

// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
#ifdef _SUPPORT_EQ_ADJUST_
    #define MaxEQIndex 3
unsigned char IT6602EQTable[] = {0xFF, 0x9F, 0x83};

// for EQ state machine handler
// #define  MAXSYNCOFF      5
    #define MAXECCWAIT      (10)
    #define EQSTATE_WAIT    (20)
    #define EQSTATE_START   (EQSTATE_WAIT + MAXECCWAIT)
    #define EQSTATE_LOW     (EQSTATE_WAIT + EQSTATE_START + (MAXECCWAIT * 1))
    #define EQSTATE_MIDDLE  (EQSTATE_WAIT + EQSTATE_START + (MAXECCWAIT * 2))
    #define EQSTATE_HIGH    (EQSTATE_WAIT + EQSTATE_START + (MAXECCWAIT * 3))
    #define EQSTATE_END     (255 - 100)// 155
    #define MINECCFAILCOUNT (MAXECCWAIT / 2)
#endif

#ifdef _SUPPORT_AUTO_EQ_
unsigned char   ucPortAMPOverWrite[2];
unsigned char   ucPortAMPValid[2];
unsigned char   ucChannelB[2];      // ch0
unsigned char   ucChannelG[2];      // ch1
unsigned char   ucChannelR[2];      // ch2
unsigned char   ucEQMode[2];
#endif
// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
unsigned char HdmiI2cAddr = IT6602B0_HDMI_ADDR;
// FIX_ID_002 xxxxx

// for debug video format only
int             CurTMDSCLK;
VTiming         CurVTiming;
AVI_InfoFrame   aviinfoframe;
// int GCP_CD       = CD8BIT; //24 bits per pixel
int             InColorMode     = RGB444;   // RGB444, YCbCr422, YCbCr444
int             OutColorMode    = RGB444;   // RGB444, YCbCr422, YCbCr444
int             OutCD           = OUT8B;
int             VIC;
// for debug only

#ifdef _FIX_ID_028_
// FIX_ID_028 xxxxx //For Debug Audio error with S2
static unsigned char m_bAudioWaiting = 0;
// FIX_ID_028 xxxxx
#else
    #ifdef EnableCalFs
// FIX_ID_023 xxxxx     //Fixed for Audio Channel Status Error with invalid HDMI source
static unsigned int     m_u16TMDSCLK                    = 0;
static unsigned char    m_ForceFsValue                  = 0;
// static unsigned long m_ROSCCLK;
static unsigned char    m_AudioChannelStatusErrorCount  = 0;
        #define MAX_AUDIO_CHANNEL_STATUS_ERROR 4
// FIX_ID_023 xxxxx
    #endif
#endif

#ifdef Enable_IT6602_CEC
extern u8_t timeunit;
#endif
// ~FIX_ID_051
/****************************************************************************/
/*                          EDID Argument                                   */
/****************************************************************************/
unsigned char   VSDB_Addr;                                                  // for EDID RAM function
unsigned char   txphyadr[2], txphyA, txphyB, txphyC, txphyD, txphylevel;    // for CEC function
unsigned char   rxphyadr[2][2];                                             // for EDID RAM function
unsigned char   rxphyA, rxphyB, rxphyC, rxphyD, rxcurport;                  // for CEC function

/*****************************************************************************/
/* Init, Power, and IO Structures ********************************************/
/*****************************************************************************/

////////////////////////////////////////////////////////////////////
// IT6602 chip inital table
////////////////////////////////////////////////////////////////////
_CODE struct IT6602_REG_INI IT6602_HDMI_INIT_TABLE[] = {
// port 0
    {REG_RX_00F, 0x03,                                             0x00                                                                },   // change bank 0
    {REG_RX_010, 0xFF,                                             0x08                                                                },   // [3]1: Register reset
    {REG_RX_00F, 0x03,                                             0x00                                                                },   // change bank 0

    {REG_RX_010, 0xFF,                                             0x17                                                                },   // [4]Auto Video Reset [2]Int Reset [1]Audio Reset [0]Video Reset

    {REG_RX_011, 0xFF,                                             0x1F                                                                },   // Port 0�G[4]EQ Reset [3]CLKD5 Reset [2]CDR Reset [1]HDCP Reset [0]All logic Reset
    {REG_RX_018, 0xFF,                                             0x1F                                                                },   // Port 1�G[4]EQ Reset [3]CLKD5 Reset [2]CDR Reset [1]HDCP Reset [0]All logic Reset

    {REG_RX_012, 0xFF,                                             0xF8                                                                },   // Port 0�G[7:3] Logic reset

    {REG_RX_010, 0xFF,                                             0x10                                                                },   // [4]Auto Video Reset [2]Int Reset [1]Audio Reset [0]Video Reset

    {REG_RX_011, 0xFF,                                             0xA0                                                                },   // Port 0�G[7] Enable Auto Reset when Clock is not stable [5]Enable Auto Reset
    {REG_RX_018, 0xFF,                                             0xA0                                                                },   // Port 1�G[7] Enable Auto Reset when Clock is not stable [5]Enable Auto Reset

    {REG_RX_012, 0xFF,                                             0x00                                                                },   // Port 0�G[7:3]  Logic reset

    {REG_RX_00F, 0x03,                                             0x01                                                                },   // change bank 1    //2013-0430 Andrew suggestion
// FIX_ID_048
    {REG_RX_1B0, 0x03,                                             0x00                                                                },   // Port Set HPD tri-state at Power On initial state
// ~FIX_ID_048

    {REG_RX_00F, 0x03,                                             0x00                                                                },   // change bank 0    //2013-0430 Andrew suggestion
    {REG_RX_017, 0xC0,                                             0x80                                                                },   // Port 0�G[7:6] = 10 invert Port 0 input HCLK , CLKD5I //2013-0430 Andrew suggestion
    {REG_RX_01E, 0xC0,                                             0x00                                                                },   // Port 1�G[7:6] = 00 invert Port 1 input TMDS , CLKD5I //2013-0430 Andrew suggestion

#ifdef Enable_IT6602_CEC
    {REG_RX_00E, 0xFF,                                             0xFF                                                                },   // for enable CEC Clock
    {REG_RX_086, 0xFF,                                             (CEC_ADDR | 0x01)                                                   },   // CEC chip Slave Adr
#endif

    {REG_RX_016, 0x08,                                             0x08                                                                },   // Port 0�G[3]1: Enable CLKD5 auto power down
    {REG_RX_01D, 0x08,                                             0x08                                                                },   // Port 1�G[3]1: Enable CLKD5 auto power down

    {REG_RX_02B, 0xFF,                                             0x07                                                                },   // FixTek3D
    // FIX_ID_042 xxxxx //Disable HDCP 1.1 feature to avoid compilance issue from ilegal HDCP 1.1 source device
    {REG_RX_031, 0xFF,                                             0x09                                                                },   // [7:4]Enable repeater function [3:0] SCL hold time count & Update Ri sel
    {REG_RX_049, 0xFF,                                             0x09                                                                },   // [7:4]Enable repeater function [3:0] SCL hold time count & Update Ri sel
    // FIX_ID_042 xxxxx
// FIX_ID_017 xxxxx Disable IPLockChk
// FIX_ID_001 xxxxx UseIPLock = 0 for avoid clk change
    {REG_RX_035, 0x1E,                                             (0x10 + (DeltaNum << 2))                                            },   // [3:2] RCLKDeltaSel , [1] UseIPLock = 0
    {REG_RX_04B, 0x1E,                                             (0x10 + (DeltaNum << 2))                                            },   // [3:2] RCLKDeltaSel , [1] UseIPLock = 0
// FIX_ID_001 xxxxx
// FIX_ID_017 xxxxx
    {REG_RX_054, 0xFF,                                             (1 << 4) + RCLKFreqSel                                              },   // [1:0]RCLK frequency select
    {REG_RX_06A, 0xFF,                                             GenPktRecType                                                       },   // Decide which kind of packet to be fully recorded on General PKT register
    {REG_RX_074, 0xFF,                                             0xA0                                                                },   // [7]Enable i2s and SPDIFoutput [5]Disable false DE output
    {REG_RX_050, 0x1F,                                             0x10 | DCLK_DELAY_CSC_ON                                            },   // [4]1: Invert output DCLK and DCLK DELAY 2 Step

//  {REG_RX_065,    0x0C,   0x00},  //[3:2]0=8bits Output color depth
//  {REG_RX_065,    0x0C,   0x04},  //[3:2]1=10bits Output color depth

    {REG_RX_065, 0x0C,                                             (OUTPUT_COLOR_DEPTH << 2)                                           },   // [3:2]2=12bits Output color depth

    {REG_RX_07A, 0x80,                                             0x80                                                                },   // [7]1: enable audio B Frame Swap Interupt
//  {REG_RX_02D,    0x03,   0x03},  //[1:0] 11: Enable HDMI/DVI mode over-write

    {REG_RX_085, 0x02,                                             0x02                                                                },   // [1]1: gating avmute in video detect module

//  {REG_RX_051,    0x80,   0x80},  //[7]1: power down color space conversion logic

#ifdef  _SUPPORT_EDID_RAM_
    #define DISABLE_PORT0_EDIDRAM   0
    #define DISABLE_PORT1_EDIDRAM   0
    {REG_RX_0C0, 0x43,                                             0x40 | (DISABLE_PORT1_EDIDRAM << 1) | (DISABLE_PORT0_EDIDRAM << 1)  },   // [0]1:Reg_P0DisableShadow
    {REG_RX_087, 0xFF,                                             (EDID_ADDR | 0x01)                                                  },   // [7:1] EDID RAM Slave Adr ,[0]1: Enable access EDID block
#else
    {REG_RX_0C0, 0x03,                                             0x03                                                                },   // [0]1:Reg_P0DisableShadow
    {REG_RX_087, 0xFF,                                             (0x00)                                                              },   // [7:1] EDID RAM Slave Adr ,[0]1: Enable access EDID block
#endif

    {REG_RX_071, 0x08,                                             0x00                                                                },   // Reg71[3] RegEnPPColMode must clear to 0 for andrew suggestion 2013-0502
// FIX_ID_030 xxxxx fixed video lost at 640x480 timing
    {REG_RX_037, 0xFF,                                             0xA6                                                                },   // Reg37 Reg_P0_WCLKValidNum must set to 0xA6 for andrew suggestion 2014-0403
    {REG_RX_04D, 0xFF,                                             0xA6                                                                },   // Reg4D Reg_P1_WCLKValidNum must set to 0xA6 for andrew suggestion 2014-0403
// FIX_ID_030 xxxxx
    {REG_RX_067, 0x80,                                             0x00                                                                },   // Reg67[7] disable HW CSCSel

    {REG_RX_07A, B_CTS_RES,                                        B_CTS_RES                                                           },

#if defined(_IT6602_)
    {REG_RX_077, 0x80,                                             0x00                                                                },
    {REG_RX_00F, 0x03,                                             0x01                                                                },   // change bank 1
    {REG_RX_1C0, 0x8C,                                             0x08                                                                },
    {REG_RX_00F, 0x03,                                             0x00                                                                },   // change bank 0
#else
    {REG_RX_077, 0x80,                                             0x80                                                                },   // 1 Audio i2s sck and mclk is common pin
    {REG_RX_00F, 0x03,                                             0x01                                                                },   // change bank 1
    {REG_RX_1C0, 0x8C,                                             0x88                                                                },
    {REG_RX_00F, 0x03,                                             0x00                                                                },   // change bank 0
#endif

// FIX_ID_037 xxxxx

#ifdef _HBR_I2S_
    {REG_RX_07E, B_HBRSel,                                         0x00                                                                },
#else
    {REG_RX_07E, B_HBRSel,                                         B_HBRSel                                                            },
#endif

    {REG_RX_052, (B_DisVAutoMute),                                 (B_DisVAutoMute)                                                    },   // Reg52[5] = 1 for disable Auto video MUTE
    {REG_RX_053, (B_VDGatting | B_VIOSel | B_TriVDIO | B_TriSYNC), (B_VIOSel | B_TriVDIO | B_TriSYNC)                                  },   // Reg53[7][5] = 01    // for disable B_VDIO_GATTING

    {REG_RX_058, 0xFF,                                             0x33                                                                },   // Reg58 for 4Kx2K Video output Driving Strength

    // FIX_ID_053        // Change AIO Strength for some weak connectivity. default value -> 0xAA
    {REG_RX_059, 0xFF,                                             0xAA                                                                },   // Reg59 for Audio output Driving Strength

    ////////////////////////////////////////////////////////////////
    // EQ Part.
    ////////////////////////////////////////////////////////////////
// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
// !!!  For Manual Adjust EQ only  !!!
#ifdef _SUPPORT_MANUAL_ADJUST_EQ_
    {REG_RX_03E, 0x20,                                             0x20                                                                },   // Enable OvWrRsCs
    {REG_RX_026, 0x20,                                             0x20                                                                },   // Enable OvWrRsCs
#endif
// FIX_ID_001 xxxxx
#ifdef _ONLY_SUPPORT_MANUAL_EQ_ADJUST_
    {REG_RX_026, 0xFF,                                             0x20                                                                },   // Reg26=0x00 disable Auto Trigger
    {REG_RX_03E, 0xFF,                                             0x20                                                                },   // Reg3E=0x00 disable Auto Trigger
#endif

// RS initial valie
// 2013/06/06 added by jau-chih.tseng@ite.com.tw
// Dr. Liu said, reg25/reg3D should set as 0x1F for auto EQ start option.
    {REG_RX_025, 0xFF,                                             DEFAULT_EQVALUE                                                     },
    {REG_RX_03D, 0xFF,                                             DEFAULT_EQVALUE                                                     },
// ~jau-chih.tseng@ite.com.tw
    {REG_RX_027, 0xFF,                                             DEFAULT_EQVALUE                                                     },   // B ch
    {REG_RX_028, 0xFF,                                             DEFAULT_EQVALUE                                                     },   // G
    {REG_RX_029, 0xFF,                                             DEFAULT_EQVALUE                                                     },   // R
    {REG_RX_03F, 0xFF,                                             DEFAULT_EQVALUE                                                     },
    {REG_RX_040, 0xFF,                                             DEFAULT_EQVALUE                                                     },
    {REG_RX_041, 0xFF,                                             DEFAULT_EQVALUE                                                     },

    {REG_RX_00F, 0x03,                                             0x01                                                                },   // change bank 1    //2013-0515 Andrew suggestion   for Auto EQ
    {REG_RX_1BC, 0xFF,                                             0x06                                                                },   // Reg1BC=0x06      //2013-0515 Andrew suggestion   for Auto EQ
// FIX_ID_020 xxxxx     //Turn off DEQ for HDMI port 1 with 20m DVI Cable
    {REG_RX_1CC, 0xFF,                                             0x00                                                                },   // Reg1CC=0x00      for TURN OFF DEQ
    {REG_RX_1C6, 0x07,                                             0x03                                                                },   // [2:0]Reg_P1_ENHYS = 03 for default enable filter to gating output
// FIX_ID_020 xxxxx

    {REG_RX_1B5, 0x03,                                             0x03                                                                },   // Reg1B5[1:0]='11' for fix Korea K706 pattern Generator    //2013-0515 Andrew suggestion
// FIX_ID_019   xxxxx
    {REG_RX_1B8, 0x80,                                             0x00                                                                },   // [7] Reg_HWENHYS = 0
    {REG_RX_1B6, 0x07,                                             0x03                                                                },   // [2:0]Reg_P0_ENHYS = 03 for default enable filter to gating output
// FIX_ID_019   xxxxx

// FIX_ID_029   xxxxx fixed Ulta-2000 HDCP fail issue at Receiver mode
    {REG_RX_110, 0xFF,                                             0x00                                                                },
    {REG_RX_111, 0xFF,                                             0x00                                                                },
    {REG_RX_112, 0xFF,                                             0x00                                                                },
    {REG_RX_113, 0xFF,                                             0x00                                                                },   // for receiver, BKSV should be zero on inital
    {REG_RX_128, 0xFF,                                             0x00                                                                },   // Clear KSV LIST
    {REG_RX_129, 0xFF,                                             0x00                                                                },   // Clear KSV LIST
    {REG_RX_12A, 0xFF,                                             0x00                                                                },   // Clear KSV LIST
    {REG_RX_12B, 0xFF,                                             0x00                                                                },   // Clear KSV LIST
    {REG_RX_12C, 0xFF,                                             0x00                                                                },   // Clear KSV LIST
// FIX_ID_029   xxxxx

    {REG_RX_00F, 0x03,                                             0x00                                                                },   // change bank 0    //2013-0515 Andrew suggestion   for Auto EQ

// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
//  for Auto EQ
#ifdef _SUPPORT_AUTO_EQ_
// 0704 disable ->  {REG_RX_022,    0xFF,   0x38},  //Reg22=0x38        //2013-0515 Andrew suggestion   for Auto EQ
// 0704 disable ->  {REG_RX_03A,    0xFF,   0x38},  //Reg3A=0x38        //2013-0515 Andrew suggestion   for Auto EQ
    {REG_RX_022, 0xFF,                                             0x00                                                                },   // 07-16 Reg22=0x30 power down auto EQ
    {REG_RX_03A, 0xFF,                                             0x00                                                                },   // 07-16 Reg3A=0x30 power down auto EQ

    #ifdef ENABLE_AUTO_TRIGGER
    {REG_RX_026, 0xFF,                                             0x80                                                                },   // Reg26=0x80   enable Auto Trigger
    {REG_RX_03E, 0xFF,                                             0x80                                                                },   // Reg3E=0x80   enable Auto Trigger
    #else
    {REG_RX_026, 0xFF,                                             0x00                                                                },   // Reg26=0x00 disable Auto Trigger
    {REG_RX_03E, 0xFF,                                             0x00                                                                },   // Reg3E=0x00 disable Auto Trigger
    #endif

#else
    {REG_RX_022, 0xFF,                                             0x00                                                                },   // 07-16 Reg22=0x30 power down auto EQ
    {REG_RX_03A, 0xFF,                                             0x00                                                                },   // 07-16 Reg3A=0x30 power down auto EQ

    {REG_RX_026, 0xFF,                                             0x00                                                                },   // 07-16 Reg26=0x00 disable Auto Trigger
    {REG_RX_03E, 0xFF,                                             0x00                                                                },   // 07-16 Reg3E=0x00 disable Auto Trigger

#endif
// FIX_ID_001 xxxxx

    {REG_RX_063, 0xFF,                                             0x3F                                                                },   // for enable interrupt output Pin
    {REG_RX_073, 0x08,                                             0x00                                                                },   // for HDCPIntKey = false

    {REG_RX_060, 0x40,                                             0x00                                                                },   // disable interrupt mask for NoGenPkt_Rcv

// FIX_ID_017 xxxxx Disable IPLockChk
    {REG_RX_02A, 0x01,                                             0x00                                                                },   // disable PORT 0 EnIPLockChk
    {REG_RX_042, 0x01,                                             0x00                                                                },   // disable PORT 1 EnIPLockChk
// FIX_ID_017 xxxxx

// FIX_ID_025 xxxxx Audio lock method select for HDMI Repeater / splitter application
    {REG_RX_077, 0x0C,                                             0x08                                                                },   // Reg77[3:2] = 01  Audio lock method select
// FIX_ID_025 xxxxx

    {0xFF,       0xFF,                                             0xFF                                                                },
};

_CODE unsigned char         bCSCMtx_RGB2YUV_ITU601_16_235[] =
{
    0x00, 0x80,       0x10,
    0xB2, 0x04,       0x65,      0x02,  0xE9,  0x00,
    0x93, 0x3C,       0x18,      0x04,  0x55,  0x3F,
    0x49, 0x3D,       0x9F,      0x3E,  0x18,  0x04
};

_CODE unsigned char bCSCMtx_RGB2YUV_ITU601_0_255[] =
{
    0x10, 0x80,       0x10,
    0x09, 0x04,       0x0E,      0x02,  0xC9,  0x00,
    0x0F, 0x3D,       0x84,      0x03,  0x6D,  0x3F,
    0xAB, 0x3D,       0xD1,      0x3E,  0x84,  0x03
};

_CODE unsigned char bCSCMtx_RGB2YUV_ITU709_16_235[] =
{
    0x00, 0x80,       0x10,
    0xB8, 0x05,       0xB4,      0x01,  0x94,  0x00,
    0x4A, 0x3C,       0x17,      0x04,  0x9F,  0x3F,
    0xD9, 0x3C,       0x10,      0x3F,  0x17,  0x04
};

_CODE unsigned char bCSCMtx_RGB2YUV_ITU709_0_255[] =
{
    0x10, 0x80,       0x10,
    0xEA, 0x04,       0x77,      0x01,  0x7F,  0x00,
    0xD0, 0x3C,       0x83,      0x03,  0xAD,  0x3F,
    0x4B, 0x3D,       0x32,      0x3F,  0x83,  0x03
};

_CODE unsigned char bCSCMtx_YUV2RGB_ITU601_16_235[] =
{
    0x00, 0x00,       0x00,
    0x00, 0x08,       0x6B,      0x3A,  0x50,  0x3D,
    0x00, 0x08,       0xF5,      0x0A,  0x02,  0x00,
    0x00, 0x08,       0xFD,      0x3F,  0xDA,  0x0D
};

_CODE unsigned char bCSCMtx_YUV2RGB_ITU601_0_255[] =
{
    0x04, 0x00,       0xA7,
    0x4F, 0x09,       0x81,      0x39,  0xDD,  0x3C,
    0x4F, 0x09,       0xC4,      0x0C,  0x01,  0x00,
    0x4F, 0x09,       0xFD,      0x3F,  0x1F,  0x10
};

_CODE unsigned char bCSCMtx_YUV2RGB_ITU709_16_235[] =
{
    0x00, 0x00,       0x00,
    0x00, 0x08,       0x55,      0x3C,  0x88,  0x3E,
    0x00, 0x08,       0x51,      0x0C,  0x00,  0x00,
    0x00, 0x08,       0x00,      0x00,  0x84,  0x0E
};

_CODE unsigned char bCSCMtx_YUV2RGB_ITU709_0_255[] =
{
    0x04, 0x00,       0xA7,
    0x4F, 0x09,       0xBA,      0x3B,  0x4B,  0x3E,
    0x4F, 0x09,       0x57,      0x0E,  0x02,  0x00,
    0x4F, 0x09,       0xFE,      0x3F,  0xE8,  0x10
};

// FIX_ID_027 xxxxx Support Full/Limited Range convert
// full 2 limit
_CODE unsigned char bCSCMtx_RGB_0_255_RGB_16_235[] =
{
    0x10, 0x10,       0x00,
    0xe0, 0x06,       0x00,      0x00,  0x00,  0x00,
    0x00, 0x00,       0xe0,      0x06,  0x00,  0x00,
    0x00, 0x00,       0x00,      0x00,  0xe0,  0x06,
};

// limit 2 full
_CODE unsigned char bCSCMtx_RGB_16_235_RGB_0_255[] =
{
    0xED, 0xED,       0x00,
    0x50, 0x09,       0x00,      0x00,  0x00,  0x00,
    0x00, 0x00,       0x50,      0x09,  0x00,  0x00,
    0x00, 0x00,       0x00,      0x00,  0x50,  0x09,
};
// FIX_ID_027 xxxxx

#ifdef  _SUPPORT_EDID_RAM_
// FIX_ID_054        // seperate EDID into a dedicated file.
extern unsigned char _CODE Default_Edid_Block[256];
#endif

// disable ->static struct IT6602_VIDEO_CONFIGURE_REG _CODE IT6602_OUTPUT_VIDEO_MODE[] =
// disable ->{
// disable ->
// disable ->// { Reg51 , Reg65}
// disable ->//
// disable ->// Reg51 [6] Half PCLK DDR , [5] Half Bus DDR , [2] CCIR656 mode
// disable ->// Reg65 [7] BTA1004Fmt , [6] SyncEmb , [5:4] output color 0x00 RGB, 0x10 YUV422, 0x20 YUV444
// disable ->//
// disable ->// 1. RGB/YUV422/YUV444
// disable ->// 2. SDR/HalfPCLKDDR/HalfBusDDR
// disable ->// 3. EmbSync/SepSync
// disable ->//
// disable ->{0x00,0x00},  //RGB444_SDR=0,
// disable ->{0x00,0x10},  //YUV422_SDR
// disable ->{0x00,0x20},  //YUV444_SDR
// disable ->
// disable ->{0x00,0x50},  //YUV422_SDR_Emb_Sync
// disable ->{0x04,0x50},  //CCIR656_SDR_Emb_Sync
// disable ->{0x04,0x10},  //CCIR656_SDR_Sep_Sync
// disable ->
// disable ->{0x40,0x00},  //RGB444_HalfPCLKDDR
// disable ->{0x40,0x20},  //YUV444_HalfPCLKDDR
// disable ->{0x40,0x50},  //YUV422_HalfPCLKDDR_Emb_Sync
// disable ->{0x40,0x10},  //YUV422_HalfPCLKDDR_Sep_Sync
// disable ->
// disable ->{0x44,0x10},  //CCIR656_HalfPCLKDDR_Sep_Sync
// disable ->
// disable ->{0x20,0x00},  //RGB444_HalfBusDDR
// disable ->{0x20,0x20},  //YUV444_HalfBusDDR
// disable ->
// disable ->{0x00,0xD0},  //BTA1004_SDR
// disable ->{0x40,0xD0},  //BTA1004_DDR
// disable ->};

_CODE char  * VStateStr[] = {
    "VSTATE_Off",
    "VSTATE_TerminationOff",
    "VSTATE_TerminationOn",
    "VSTATE_5VOff",
    "VSTATE_SyncWait",
    "VSTATE_SWReset",
    "VSTATE_SyncChecking",
    "VSTATE_HDCPSet",
    "VSTATE_HDCP_Reset",
    "VSTATE_ModeDetecting",
    "VSTATE_VideoOn",
    "VSTATE_ColorDetectReset",
    "VSTATE_HDMI_OFF",
    "VSTATE_Reserved",
};

_CODE char  * AStateStr[] = {
    "ASTATE_AudioOff",
    "ASTATE_RequestAudio",
    "ASTATE_ResetAudio",
    "ASTATE_WaitForReady",
    "ASTATE_AudioOn",
    "ASTATE_Reserved"
};

_CODE char  * VModeStateStr[] = {
    "0 eRGB444_SDR",
    "1 eYUV444_SDR",
    "2 eRGB444_DDR",
    "3 eYUV444_DDR",
    "4 eYUV422_Emb_Sync_SDR",
    "5 eYUV422_Emb_Sync_DDR",
    "6 eYUV422_Sep_Sync_SDR",
    "7 eYUV422_Sep_Sync_DDR",
    "8 eCCIR656_Emb_Sync_SDR",
    "9 eCCIR656_Emb_Sync_DDR",
    "10 eCCIR656_Sep_Sync_SDR",
    "11 eCCIR656_Sep_Sync_DDR",
    "12 eRGB444_Half_Bus",
    "13 eYUV444_Half_Bus",
    "14 eBTA1004_SDR",
    "15 eBTA1004_DDR",
};

/*****************************************************************************/
/*  Function Definitions    **************************************************/
/*****************************************************************************/
// #define VideoTimeOutCheck(x) TimeOutCheck(IT6602->m_VideoCountingTimer, (x))
// #define AudioTimeOutCheck(x) TimeOutCheck(IT6602->m_AudioCountingTimer, (x))
// #define AssignSWResetVirtualTime() { IT6602->m_SWResetTimeOut     = GetCurrentVirtualTime(); }
// #define AssignVideoVirtualTime()   { IT6602->m_VideoCountingTimer = GetCurrentVirtualTime(); }
// #define AssignAudioVirtualTime()   { IT6602->m_AudioCountingTimer = GetCurrentVirtualTime();}
// #define AssignVideoTimerTimeout(TimeOut) {IT6602->m_VideoCountingTimer = (TimeOut);}
// #define AssignAudioTimerTimeout(TimeOut) {IT6602->m_AudioCountingTimer = (TimeOut);}

/*****************************************************************************/
/*  Function Prototypes    **************************************************/
/*****************************************************************************/

/* it660x IO Functions   ***********************************************************/

//
// #ifdef MEGAWIN82516
// static void IT6602AutoPortSelect(void);
// #endif

/*****************************************************************************/

/*  IO Functions   ***********************************************************/

/*****************************************************************************/
SYS_STATUS
EDID_RAM_Write (
    unsigned char   offset,
    unsigned char   byteno,
    unsigned char   * p_data)
{
#if 0
    unsigned char flag;
    flag = i2c_write_byte(EDID_ADDR, offset, byteno, p_data, HDMI_DEV);
    return !flag;
#else
    unsigned char flag;
    // flag = i2c_write_byte(EDID_ADDR, offset, byteno, p_data, HDMI_DEV);
    flag = HDMIRX_WriteI2C_ByteN(EDID_ADDR, offset, p_data, byteno);
    return !flag;
#endif
}

unsigned char
EDID_RAM_Read (
    unsigned char offset)
{
#if 0
    unsigned char p_data;
    i2c_read_byte(EDID_ADDR, offset, 1, &p_data, HDMI_DEV);
    return p_data;
#else
    unsigned char p_data;
    // i2c_read_byte(EDID_ADDR, offset,1, &p_data, HDMI_DEV);
    HDMIRX_ReadI2C_ByteN(EDID_ADDR, offset, &p_data, 1);
    return p_data;
#endif
}

// ===============================================================================//
unsigned char
IT6602VersionRead (
    unsigned char RegAddr)
{
#if 0
    unsigned char   FLAG;
    unsigned char   mDataIn;
// FIX_ID_002 xxxxx check IT6602 chip version
    FLAG = i2c_read_byte(HdmiI2cAddr, RegAddr, 1, &mDataIn, HDMI_DEV);
// FIX_ID_002 xxxxx
    if (FLAG == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("IT6602 I2C Read ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Read Reg0x%X=\n", (int) RegAddr));
    }
    return mDataIn;
#else
    unsigned char   FLAG;
    unsigned char   mDataIn;
// FIX_ID_002 xxxxx check IT6602 chip version
    // FLAG= i2c_read_byte(HdmiI2cAddr, RegAddr, 1, &mDataIn, HDMI_DEV);
    FLAG = HDMIRX_ReadI2C_ByteN(HdmiI2cAddr, RegAddr, &mDataIn, 1);
// FIX_ID_002 xxxxx
    if (FLAG == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("IT6602 I2C Read ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Read Reg0x%X=  \n", (int) RegAddr));
    }
    return mDataIn;
#endif
}

unsigned char
hdmirxrd (
    unsigned char RegAddr)
{
#if 0
    unsigned char   FLAG;
    unsigned char   mDataIn;
// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
    FLAG = i2c_read_byte(HdmiI2cAddr, RegAddr, 1, &mDataIn, HDMI_DEV);
// FIX_ID_002 xxxxx
    if (FLAG == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("HDMI Read ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Read Reg0x%X=\n", (int) RegAddr));
    }
    return mDataIn;
#else
    unsigned char   FLAG;
    unsigned char   mDataIn;
// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
    // FLAG= i2c_read_byte(HdmiI2cAddr, RegAddr, 1, &mDataIn, HDMI_DEV);
    FLAG = HDMIRX_ReadI2C_ByteN(HdmiI2cAddr, RegAddr, &mDataIn, 1);
// FIX_ID_002 xxxxx
    if (FLAG == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("HDMI Read ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Read Reg0x%X=  \n", (int) RegAddr));
    }
    return mDataIn;
#endif
}

unsigned char
hdmirxwr (
    unsigned char   RegAddr,
    unsigned char   DataIn)
{
#if 0
    unsigned char flag;
    // MHLRX_DEBUG_PRINTF(("HDMI_IIC_Write(%#BX,%#BX)\n", RegAddr, DataIn));

// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
    flag = i2c_write_byte(HdmiI2cAddr, RegAddr, 1, &DataIn, HDMI_DEV);
// FIX_ID_002 xxxxx
    if (flag == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
        IT6602_DEBUG_INT_PRINTF(("HDMI I2C ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Write Reg0x%X=%X =====\n", (int)RegAddr, (int)DataIn));
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
    }
    return !flag;
#else
    unsigned char flag;

    flag = HDMIRX_WriteI2C_ByteN(HdmiI2cAddr, RegAddr, &DataIn, 1);

    if (flag == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
        IT6602_DEBUG_INT_PRINTF(("HDMI I2C ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Write Reg0x%X=%X =====  \n", (int)RegAddr, (int)DataIn));
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
    }
    return !flag;
#endif
}

unsigned char
hdmirxset (
    unsigned char   offset,
    unsigned char   mask,
    unsigned char   ucdata)
{
    unsigned char temp;
    temp    = hdmirxrd(offset);
    temp    = (temp & ((~mask) & 0xFF)) + (mask & ucdata);
    return hdmirxwr(offset, temp);
}

void
hdmirxbwr (
    unsigned char   offset,
    unsigned char   byteno,
    unsigned char   * rddata)
{
#if 0
    unsigned char flag;
    if (byteno > 0)
    {
// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
        flag = i2c_write_byte(HdmiI2cAddr, offset, byteno, rddata, HDMI_DEV);
    }
// FIX_ID_002 xxxxx
    if (flag == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
        IT6602_DEBUG_INT_PRINTF(("IT6602 I2C ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Write Reg0x%X=%X =====\n", (int)offset, (int)rddata));
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
    }
#else
    unsigned char flag;
    if (byteno > 0)
    {
        flag = HDMIRX_WriteI2C_ByteN(HdmiI2cAddr, offset, rddata, byteno);
    }
    if (flag == 0)
    {
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
        IT6602_DEBUG_INT_PRINTF(("IT6602 I2C ERROR !!!"));
        IT6602_DEBUG_INT_PRINTF(("=====  Write Reg0x%X=%X =====  \n", (int)offset, (int)rddata));
        IT6602_DEBUG_INT_PRINTF(("====================================\n"));
    }
#endif
}

/*****************************************************************************/
/* it660x Configuration and Initialization ***********************************/
/*****************************************************************************/
#ifdef _it660x_
struct IT6602_dev_data *

get_IT6602_dev_data (
    void)
{
    return &IT6602DEV;
}

void
hdimrx_write_init (
    struct IT6602_REG_INI _CODE * tdata)
{
    int cnt = 0;
    while (tdata[cnt].ucAddr != 0xFF)
    {
        // (void)printf(" Cnt = %d, addr = %02X,andmask = %02X,ucValue=%02X\n", cnt,(int)tdata[cnt].ucAddr,(int)tdata[cnt].andmask,(int)tdata[cnt].ucValue);
        hdmirxset(tdata[cnt].ucAddr, tdata[cnt].andmask, tdata[cnt].ucValue);
        cnt++;
    }
}

// FIX_ID_003 xxxxx //Add IT6602 Video Output Configure setting
void
IT6602_VideoOutputConfigure_Init (
    struct IT6602_dev_data  * IT6602,
    Video_Output_Configure  eVidOutConfig)
{
    IT6602->m_VidOutConfigMode = eVidOutConfig;

    switch (eVidOutConfig)
    {
        case eRGB444_SDR:
            IT6602->m_bOutputVideoMode  = F_MODE_RGB444;
// FIX_ID_027 xxxxx Support Full/Limited Range convert
            IT6602->m_bOutputVideoMode  = F_MODE_RGB444 | F_MODE_0_255;   // it660x output RGB Full Range
            // IT6602->m_bOutputVideoMode = F_MODE_RGB444|F_MODE_16_235;    // it660x output RGB limited Range
// FIX_ID_027 xxxxx
            IT6602->m_VidOutDataTrgger  = eSDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eYUV444_SDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV444;
            IT6602->m_VidOutDataTrgger  = eSDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eRGB444_DDR:
            IT6602->m_bOutputVideoMode  = F_MODE_RGB444;
            IT6602->m_VidOutDataTrgger  = eHalfPCLKDDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eYUV444_DDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV444;
            IT6602->m_VidOutDataTrgger  = eHalfPCLKDDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eYUV422_Emb_Sync_SDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eSDR;
            IT6602->m_VidOutSyncMode    = eEmbSync;
            break;

        case eYUV422_Emb_Sync_DDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eHalfPCLKDDR;
            IT6602->m_VidOutSyncMode    = eEmbSync;
            break;

        case eYUV422_Sep_Sync_SDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eSDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eYUV422_Sep_Sync_DDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eHalfPCLKDDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eCCIR656_Emb_Sync_SDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eSDR;
            IT6602->m_VidOutSyncMode    = eCCIR656EmbSync;
            break;

        case eCCIR656_Emb_Sync_DDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eHalfPCLKDDR;
            IT6602->m_VidOutSyncMode    = eCCIR656EmbSync;
            break;

        case eCCIR656_Sep_Sync_SDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eSDR;
            IT6602->m_VidOutSyncMode    = eCCIR656SepSync;
            break;

        case eCCIR656_Sep_Sync_DDR:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eHalfPCLKDDR;
            IT6602->m_VidOutSyncMode    = eCCIR656SepSync;
            break;

        case eRGB444_Half_Bus:
            IT6602->m_bOutputVideoMode  = F_MODE_RGB444;
            IT6602->m_VidOutDataTrgger  = eHalfBusDDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eYUV444_Half_Bus:
            IT6602->m_bOutputVideoMode  = F_MODE_YUV444;
            IT6602->m_VidOutDataTrgger  = eHalfBusDDR;
            IT6602->m_VidOutSyncMode    = eSepSync;
            break;

        case eBTA1004_SDR:                              // BTA1004_SDR_Emb_Sync
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eSDR_BTA1004;
            IT6602->m_VidOutSyncMode    = eEmbSync;
            break;

        case eBTA1004_DDR:                              // BTA1004_DDR_Emb_Sync
            IT6602->m_bOutputVideoMode  = F_MODE_YUV422;
            IT6602->m_VidOutDataTrgger  = eDDR_BTA1004; // eHalfPCLKDDR
            IT6602->m_VidOutSyncMode    = eEmbSync;
            break;
    }

    // only for debug use !!!
    // IT6602_VideoOutputModeSet(IT6602);
}
// FIX_ID_003 xxxxx

////////////////////////////////////////////////////////////////////
// int hdmirx_Var_init( void )
//
//
//
////////////////////////////////////////////////////////////////////
void
hdmirx_Var_init (
    struct IT6602_dev_data * IT6602)
{
    // IT6602->m_ucCurrentHDMIPort=0xFF;
    // IT6602->m_ucDVISCDToffCNT=0;
    IT6602->m_ucSCDTOffCount        = 0;
    IT6602->m_ucEccCount_P0         = 0;
    IT6602->m_ucEccCount_P1         = 0;
    IT6602->m_ucDeskew_P0           = 0;
    IT6602->m_ucDeskew_P1           = 0;

    IT6602->m_SWResetTimeOut        = 0;
    IT6602->m_VideoCountingTimer    = 0;
    IT6602->m_AudioCountingTimer    = 0;

    IT6602->m_bVideoOnCountFlag     = FALSE;

    IT6602->m_MuteAutoOff           = FALSE;
    IT6602->m_bUpHDMIMode           = FALSE;
    IT6602->m_bUpHDCPMode           = FALSE;
    IT6602->m_NewAVIInfoFrameF      = FALSE;
    IT6602->m_NewAUDInfoFrameF      = FALSE;
    IT6602->m_HDCPRepeater          = FALSE;

// 06-27 disable -->     IT6602->m_bOutputVideoMode=HDMIRX_OUTPUT_VID_MODE;

// FIX_ID_003 xxxxx //Add IT6602 Video Output Configure Table
    IT6602_VideoOutputConfigure_Init(IT6602, HDMIRX_OUTPUT_VID_MODE);
// FIX_ID_003 xxxxx

    IT6602->m_bRxAVmute = FALSE;

// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_EQ_ADJUST_
    IT6602->EQPort[F_PORT_SEL_0].ucEQState          = 0xFF;
    IT6602->EQPort[F_PORT_SEL_0].ucAuthR0           = 0;
    IT6602->EQPort[F_PORT_SEL_0].ucECCvalue         = 0;
    IT6602->EQPort[F_PORT_SEL_0].ucECCfailCount     = 0;
    IT6602->EQPort[F_PORT_SEL_0].ucPkt_Err          = 0; // Pkt_Err
    IT6602->EQPort[F_PORT_SEL_0].ucPortID           = F_PORT_SEL_0;

    IT6602->EQPort[F_PORT_SEL_1].ucEQState          = 0xFF;
    IT6602->EQPort[F_PORT_SEL_1].ucAuthR0           = 0;
    IT6602->EQPort[F_PORT_SEL_1].ucECCvalue         = 0;
    IT6602->EQPort[F_PORT_SEL_1].ucECCfailCount     = 0;
    IT6602->EQPort[F_PORT_SEL_1].ucPkt_Err          = 0;
    IT6602->EQPort[F_PORT_SEL_1].ucPortID           = F_PORT_SEL_1;

    IT6602->EQPort[F_PORT_SEL_0].f_manualEQadjust   = FALSE;
    IT6602->EQPort[F_PORT_SEL_1].f_manualEQadjust   = FALSE;
    // FIX_ID_043
    IT6602->ToggleCount[F_PORT_SEL_0]               = 0;
    IT6602->ToggleCount[F_PORT_SEL_1]               = 0;
    // ~FIX_ID_043

    #endif

    #ifdef _SUPPORT_AUTO_EQ_

    ucPortAMPOverWrite[1]   = 0;    // 2013-0801
    ucPortAMPValid[1]       = 0;
    ucChannelB[1]           = 0;
    ucChannelG[1]           = 0;
    ucChannelR[1]           = 0;

    ucPortAMPOverWrite[0]   = 0;    // 2013-0801
    ucPortAMPValid[0]       = 0;
    ucChannelB[0]           = 0;
    ucChannelG[0]           = 0;
    ucChannelR[0]           = 0;
    #endif
// FIX_ID_001 xxxxx

// FIX_ID_005 xxxxx     //Add Cbus Event Handler
    IT6602->HDMIIntEvent    = 0;
    IT6602->HDMIWaitNo[0]   = 0;
    IT6602->HDMIWaitNo[1]   = 0;
// FIX_ID_005 xxxxx

    #ifdef _IT6607_GeNPacket_Usage_
    IT6602->m_PollingPacket         = 0;
    IT6602->m_PacketState           = 0;
    IT6602->m_ACPState              = 0;
    IT6602->m_GamutPacketRequest    = FALSE;
    IT6602->m_GeneralRecPackType    = 0x00;
    #endif
    IT6602->m_ucCurrentHDMIPort     = 0xff;

    IT6602->m_HDCP_ContentOff       = 0;
// xxxxx 2014-0529
// FIX_ID_037 xxxxx
    IT6602->m_VState        = VSTATE_Unknown;
    IT6602->m_AState        = ASTATE_Unknown;
    IT6602->m_RxHDCPState   = RxHDCP_PwrOff;
    #ifndef _SUPPORT_HDMI_REPEATER_
    IT6602->m_bHDCPrepeater = FALSE;
    #endif

    IT6602->prevAVIDB[0]    = 0xFF;
    IT6602->prevAVIDB[1]    = 0xFF;
}

////////////////////////////////////////////////////////////////////
// void hdmitx_rst( void )
//
//
//
////////////////////////////////////////////////////////////////////
void
IT6602_Rst (
    struct IT6602_dev_data * IT6602)
{
    hdmirx_Var_init(IT6602);
    hdimrx_write_init(IT6602_HDMI_INIT_TABLE);

    Cal_oclk();

    #ifdef _SUPPORT_HDMI_REPEATER_
    IT6602SwitchVideoState(IT6602, VSTATE_Off);
    #else
    IT6602SwitchVideoState(IT6602, VSTATE_SWReset);
    #endif
    IT6602SwitchAudioState(IT6602, ASTATE_AudioOff);
}

// =========================================================================//
void
IT6602_Identify_Chip (
    void)
{
// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
    #if 1
// FIX_ID_002 xxxxx

    unsigned char   i = 0, j = 0;
    unsigned char   VenID;
    unsigned char   Result                  = 0;
    unsigned char   acIT6602A0Version[4]    = {0x54, 0x49, 0x02, 0x68};
    unsigned char   I2cAddr[2]              = {IT6602B0_HDMI_ADDR, IT6602A0_HDMI_ADDR}; // IT6602A0 i2c addr = 0x94 , but IT6602B0 i2c addr = 0x90 !!!

    do
    {
        for (j = 0; j < 1; j++)                                                         // for(j=0;j<2;j++)
        {
            HdmiI2cAddr = I2cAddr[j];

            for (i = 0; i < 4; i++)
            {
                VenID = hdmirxrd(i);

                if (VenID == acIT6602A0Version[i])
                {
                    Result = 1;
                }
                else
                {
                    Result = 0;
                    it660x_DEBUG_PRINTF(("I2C Addr %X Error: Can not find IT6602 Chip !!!\n", (int)HdmiI2cAddr));
                    break;
                }
            }
            if (Result == 1)
            {
                it660x_DEBUG_PRINTF(("OK , Find IT6602 Chip I2C Addr %X !!!\n", (int)HdmiI2cAddr));
                break;
            }
        }
    }
    while (Result == 0);
    #endif
}

// FIX_ID_016 xxxxx

// =========================================================================//
void
IT6602_fsm_init (
    void)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    it660x_DEBUG_PRINTF(("IT6602_fsm_init( )\n"));

// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
    IT6602_Identify_Chip();
// FIX_ID_002 xxxxx

    IT6602_Rst(IT6602data);

    #ifdef  _SUPPORT_HDMI_REPEATER_// 20090916
    it660x_Init_HDMIRepeater(IT6602data, TRUE);
    #endif //_SUPPORT_HDCP_REPEATER_

    #ifdef Enable_IT6602_CEC
    IT6602_CECInit();
    DumpIT6602_CECReg();
    #endif

// FIX_ID_006 xxxxx     //Add P2_0 for switch Exteranl 24c04 EEPROM and Internal IT6602 EDID RAM
    #ifndef MEGAWIN82516
    // P2_0=1;
    if (P2_0 == 0)
    {
        // for Disable EDID RAM function !!!
        hdmirxset(REG_RX_0C0, 0x03, 0x03);
//      hdmirxset(REG_RX_087, 0xFF, 0x00);
    }
    else
    #endif
    {
    #ifdef  _SUPPORT_EDID_RAM_
        // (void)printf("!!!Use IT6602 EDID RAM !!!");

        EDIDRAMInitial(&Default_Edid_Block[0]);
        // FIX_ID_041 xxxxx Add EDID reset

        hdmirxset(REG_RX_0C0, 0x20, 0x20);      // xxxxx 2014-0731 [5] 1 for  reset edid
        delay1ms(1);
        hdmirxset(REG_RX_0C0, 0x20, 0x00);
        // FIX_ID_041 xxxxx
    #else

        st3DParse.bVSDBspport3D = 0;
        st3DParse.ucVicCnt      = 0;
        st3DParse.ucDtdCnt      = 0;

        // for Disable EDID RAM function !!!
        hdmirxset(REG_RX_0C0, 0x03, 0x03);
//          hdmirxset(REG_RX_087, 0xFF, 0x00);

    #endif
    }

    IT6602PortSelect(0);    // select port 0
}

#endif

/*****************************************************************************/
/* HDMIRX functions    *******************************************************/
/*****************************************************************************/
#ifdef _it660x_

void
chgbank (
    int bank)
{
    #if 0
    switch (bank)
    {
        case 0:
            hdmirxset(0x0F, 0x03, 0x00);
            break;

        case 1:
            hdmirxset(0x0F, 0x03, 0x01);
            break;

        case 2:
            hdmirxset(0x0F, 0x03, 0x02);
            break;

        case 3:
            hdmirxset(0x0F, 0x03, 0x03);
            break;

        default:
            break;
    }
    #else
    hdmirxset(0x0F, 0x03, bank & 3);
    #endif
}

unsigned char
CheckSCDT (
    struct IT6602_dev_data * IT6602)
{
    unsigned char   ucPortSel;
    unsigned char   sys_state_P0;

    ucPortSel       = hdmirxrd(REG_RX_051) & B_PORT_SEL;
    sys_state_P0    = hdmirxrd(REG_RX_P0_SYS_STATUS);

    if (ucPortSel == IT6602->m_ucCurrentHDMIPort)
    {
        if (sys_state_P0 & B_P0_SCDT)
        {
            // SCDT on
            // IT6602->m_ucSCDTOffCount=0;
            return TRUE;
        }
        else
        {
            // SCDT off
            return FALSE;
        }
    }
    return FALSE;
}

void
WaitingForSCDT (
    struct IT6602_dev_data * IT6602)
{
    unsigned char   sys_state_P0;
    unsigned char   sys_state_P1;
    unsigned char   ucPortSel;
//  unsigned char ucTMDSClk;

    sys_state_P0    = hdmirxrd(REG_RX_P0_SYS_STATUS) & (B_P0_SCDT | B_P0_PWR5V_DET | B_P0_RXCK_VALID);
    sys_state_P1    = hdmirxrd(REG_RX_P1_SYS_STATUS) & (B_P1_SCDT | B_P1_PWR5V_DET | B_P1_RXCK_VALID);
    ucPortSel       = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    // ite_180509
    if (ucPortSel == 0)
    {
        if ((hdmirxrd(0x0A) & 0x40) == 0x00)
        {
            IPLLOvw_chk(ucPortSel);
        }
    }
    else
    {
        IPLLOvw_chk(ucPortSel);
    }
    // ite_180509
    if (sys_state_P0 & B_P0_SCDT)
    {
        IT6602SwitchVideoState(IT6602, VSTATE_SyncChecking);    // 2013-0520
        return;
    }
    else
    {
    #ifdef _SUPPORT_EQ_ADJUST_
        if (IT6602->EQPort[ucPortSel].f_manualEQadjust == TRUE) // ignore SCDT off when manual EQ adjust !!!
        {
            return;
        }
    #endif

        if (ucPortSel == F_PORT_SEL_0)
        {
            if ((sys_state_P0 & (B_P0_PWR5V_DET | B_P0_RXCK_VALID)) == (B_P0_PWR5V_DET | B_P0_RXCK_VALID))
            {
                IT6602->m_ucSCDTOffCount++;
                EQ_PORT0_PRINTF((" SCDT off count = %X\n", (int)IT6602->m_ucSCDTOffCount));
                EQ_PORT0_PRINTF((" sys_state_P0 = %X\n", (int)hdmirxrd(REG_RX_P0_SYS_STATUS)));
            }
        }
        else
        {
            if ((sys_state_P1 & (B_P1_PWR5V_DET | B_P1_RXCK_VALID)) == (B_P1_PWR5V_DET | B_P1_RXCK_VALID))
            {
                IT6602->m_ucSCDTOffCount++;
                EQ_PORT1_PRINTF((" SCDT off count = %X\n", (int)IT6602->m_ucSCDTOffCount));
                EQ_PORT1_PRINTF((" sys_state_P1 = %X\n", (int)hdmirxrd(REG_RX_P1_SYS_STATUS)));
            }
        }

        if ((IT6602->m_ucSCDTOffCount) > SCDT_OFF_TIMEOUT)
        {
            IT6602->m_ucSCDTOffCount = 0;
            EQ_PORT0_PRINTF((" WaitingForSCDT( ) CDR reset !!!\n"));
            hdmirx_ECCTimingOut(ucPortSel);

    #ifdef _SUPPORT_AUTO_EQ_
            // xxxxx
            DisableOverWriteRS(ucPortSel);
            TMDSCheck(ucPortSel);
            // xxxxx
    #endif
        }
    }
}

unsigned char
CLKCheck (
    unsigned char ucPortSel)
{
    unsigned char sys_state;
    if (ucPortSel == F_PORT_SEL_1)
    {
        sys_state = hdmirxrd(REG_RX_P1_SYS_STATUS) & (B_P1_RXCK_VALID);
    }
    else
    {
        sys_state = hdmirxrd(REG_RX_P0_SYS_STATUS) & (B_P0_RXCK_VALID);
    }
    if (sys_state == B_P0_RXCK_VALID)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// ---------------------------------------------------------------------------------------------------
// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_AUTO_EQ_

void
DisableOverWriteRS (
    unsigned char ucPortSel)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    if (ucPortSel == F_PORT_SEL_1)
    {
        #ifdef _SUPPORT_AUTO_EQ_
        ucPortAMPOverWrite[F_PORT_SEL_1]    = 0;    // 2013-0801
        ucPortAMPValid[F_PORT_SEL_1]        = 0;

// FIX_ID_035 xxxxx
        ucEQMode[F_PORT_SEL_1] = 0;                 // 0 for Auto Mode
        hdmirxset(REG_RX_03A, 0xFF, 0x00);          // 07-16 Reg3A=0x30 power down auto EQ
        hdmirxset(REG_RX_03E, 0x20, 0x00);          // Manually set RS Value
        IT6602_DEBUG_INT_PRINTF((" ############# DisableOverWriteRS( ) port 1 ###############\n"));
        #endif
        #ifdef _SUPPORT_EQ_ADJUST_
        IT6602data->EQPort[F_PORT_SEL_1].f_manualEQadjust   = FALSE;
        IT6602data->EQPort[F_PORT_SEL_1].ucEQState          = 0xFF;
        #endif
        IT6602data->m_ucDeskew_P1                           = 0;
        IT6602data->m_ucEccCount_P1                         = 0;

        // FIX_ID_014 xxxxx
        IT6602data->HDMIIntEvent                &= 0x0F;
        IT6602data->HDMIWaitNo[F_PORT_SEL_1]    = 0;
        // FIX_ID_014 xxxxx
    }
    else
    {
        #ifdef _SUPPORT_AUTO_EQ_
        ucPortAMPOverWrite[F_PORT_SEL_0]    = 0;    // 2013-0801
        ucPortAMPValid[F_PORT_SEL_0]        = 0;
        ucEQMode[F_PORT_SEL_0]              = 0;    // 0 for Auto Mode
        hdmirxset(REG_RX_022, 0xFF, 0x00);          // 07-16 Reg22=0x30 power down auto EQ
        hdmirxset(REG_RX_026, 0x20, 0x00);          // Manually set RS Value
        IT6602_DEBUG_INT_PRINTF((" ############# DisableOverWriteRS( ) port 0 ###############\n"));
        #endif

        #ifdef _SUPPORT_EQ_ADJUST_
        IT6602data->EQPort[F_PORT_SEL_0].f_manualEQadjust   = FALSE;
        IT6602data->EQPort[F_PORT_SEL_0].ucEQState          = 0xFF;
        #endif
        IT6602data->m_ucDeskew_P0                           = 0;
        IT6602data->m_ucEccCount_P0                         = 0;

        // FIX_ID_014 xxxxx
        IT6602data->HDMIIntEvent                &= 0xF0;
        IT6602data->HDMIWaitNo[F_PORT_SEL_0]    = 0;
        // FIX_ID_014 xxxxx
    }
}

void
AmpValidCheck (
    unsigned char ucPortSel)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

        #ifdef _SUPPORT_AUTO_EQ_
    unsigned char uc;

    if (ucPortSel == F_PORT_SEL_1)
    {
        chgbank(1);
        uc = hdmirxrd(REG_RX_1D8);
        EQ_PORT1_PRINTF((" ############# AmpValidCheck( ) port 1 ###############\n"));
        EQ_PORT1_PRINTF((" ############# Reg1D8 = %X ###############\n", (int)uc));
        EQ_PORT1_PRINTF((" ############# Reg1DC = %X ###############\n", (int)hdmirxrd(REG_RX_1DC)));

        if ((uc & 0x03) == 0x03)
        {
            ucChannelB[1]       = hdmirxrd(REG_RX_1DD);
            ucPortAMPValid[1]   |= 0x03;
            EQ_PORT1_PRINTF((" ############# B AMP VALID port 1 Reg1DD = 0x%X  ###############\n", (int)ucChannelB[1]));
        }

        if ((uc & 0x0C) == 0x0C)
        {
            ucChannelG[1]       = hdmirxrd(REG_RX_1DE);
            ucPortAMPValid[1]   |= 0x0C;
            EQ_PORT1_PRINTF((" ############# G AMP VALID port 1 Reg1DD = 0x%X  ###############\n", (int)ucChannelG[1]));
        }

        if ((uc & 0x30) == 0x30)
        {
            ucChannelR[1]       = hdmirxrd(REG_RX_1DF);
            ucPortAMPValid[1]   |= 0x30;
            EQ_PORT1_PRINTF((" ############# R AMP VALID port 1 Reg1DD = 0x%X  ###############\n", (int)ucChannelR[1]));
        }
        chgbank(0);

// FIX_ID_010 xxxxx     //Add JudgeBestEQ to avoid wrong EQ setting
        if ((ucPortAMPValid[1] & 0x3F) == 0x3F)
        {
            OverWriteAmpValue2EQ(F_PORT_SEL_1);

            // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
            #ifdef _SUPPORT_EQ_ADJUST_
            HDMIStartEQDetect(&(IT6602data->EQPort[F_PORT_SEL_1]));
            #endif
            // FIX_ID_001 xxxxx
        }
// FIX_ID_010 xxxxx
    }
    else
    {
        chgbank(1);
        uc = hdmirxrd(REG_RX_1D0);
        EQ_PORT0_PRINTF((" ############# AmpValidCheck( ) port 0 ###############\n"));
        EQ_PORT0_PRINTF((" ############# REG_RX_1D0 = %X ###############\n", (int) uc));
        EQ_PORT0_PRINTF((" ############# Reg1D4 = %X ###############\n", (int) hdmirxrd(REG_RX_1D4)));

        if ((uc & 0x03) == 0x03)
        {
            ucChannelB[0]       = hdmirxrd(REG_RX_1D5);
            ucPortAMPValid[0]   |= 0x03;
            EQ_PORT0_PRINTF((" ############# B AMP VALID port 0 Reg1D5 = 0x%X  ###############\n", (int) ucChannelB[0]));
        }

        if ((uc & 0x0C) == 0x0C)
        {
            ucChannelG[0]       = hdmirxrd(REG_RX_1D6);
            ucPortAMPValid[0]   |= 0x0C;
            EQ_PORT0_PRINTF((" ############# G AMP VALID port 0 Reg1D6 = 0x%X  ###############\n", (int) ucChannelG[0]));
        }

        if ((uc & 0x30) == 0x30)
        {
            ucChannelR[0]       = hdmirxrd(REG_RX_1D7);
            ucPortAMPValid[0]   |= 0x30;
            EQ_PORT0_PRINTF((" ############# R AMP VALID port 0 Reg1D7 = 0x%X  ###############\n", (int) ucChannelR[0]));
        }
        chgbank(0);

        // 07-08
        {
            if ((ucPortAMPValid[0] & 0x3F) == 0x3F)
            {
                //          ucPortAMPOverWrite[0]=1;    //2013-0801
                //          MHLRX_DEBUG_PRINTF(("#### REG_RX_026 = 0x%X ####\n",hdmirxrd(REG_RX_026)));
                //          hdmirxset(REG_RX_026,0x20,0x20);    //Manually set RS Value
                //          MHLRX_DEBUG_PRINTF(("#### REG_RX_026 = 0x%X ####\n",hdmirxrd(REG_RX_026)));
                //
                //          hdmirxset(REG_RX_027,0xFF,(ucChannelB[0] & 0x7F));
                //          hdmirxset(REG_RX_028,0xFF,(ucChannelG[0] & 0x7F));
                //          hdmirxset(REG_RX_029,0xFF,(ucChannelR[0] & 0x7F));
                //          EQ_DEBUG_PRINTF((" ############# Over-Write port 0 EQ###############\n"));
                //          EQ_DEBUG_PRINTF((" ############# B port 0 REG_RX_027 = 0x%X  ###############\n",hdmirxrd(REG_RX_027)));
                //          EQ_DEBUG_PRINTF((" ############# G port 0 REG_RX_028 = 0x%X  ###############\n",hdmirxrd(REG_RX_028)));
                //          EQ_DEBUG_PRINTF((" ############# R port 0 REG_RX_029 = 0x%X  ###############\n",hdmirxrd(REG_RX_029)));
                //
                //          HDMICheckErrorCount(&(IT6602data->EQPort[F_PORT_SEL_0]));   //07-04 for port 1
                //
                //          hdmirxset(REG_RX_022, 0xFF, 0x00);  //07-08 [3] power down

// FIX_ID_010 xxxxx     //Add JudgeBestEQ to avoid wrong EQ setting
                OverWriteAmpValue2EQ(F_PORT_SEL_0);
// FIX_ID_010 xxxxx

                // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
            #ifdef _SUPPORT_EQ_ADJUST_
                HDMIStartEQDetect(&(IT6602data->EQPort[F_PORT_SEL_0]));
            #endif
                // FIX_ID_001 xxxxx
            }
        }
    }
        #endif
}

void
TogglePolarity (
    unsigned char ucPortSel)
{
        #ifdef _SUPPORT_AUTO_EQ_

    // xxxxx only for IT6602A0 Version
    unsigned char ucPortSelCurrent;
    ucPortSelCurrent = hdmirxrd(REG_RX_051) & B_PORT_SEL;

            #ifdef _ONLY_SUPPORT_MANUAL_EQ_ADJUST_
    return;
            #endif

    // FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
    if (HdmiI2cAddr == IT6602A0_HDMI_ADDR)
    // FIX_ID_002 xxxxx
    {
        if (ucPortSelCurrent != ucPortSel)
        {
            return;
        }
    }
    // xxxxx

    if (ucPortSel == F_PORT_SEL_1)
    {
        EQ_PORT1_PRINTF((" ############# TogglePolarity Port 1###############\n"));
        chgbank(1);

        hdmirxset(REG_RX_1C5, 0x10, 0x00);

        // FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
        if (HdmiI2cAddr == IT6602A0_HDMI_ADDR)
        {
            // xxxxx only for IT6602A0 Version
            if ((hdmirxrd(REG_RX_1B9) & 0x80) >> 7)
            {
                hdmirxset(REG_RX_1B9, 0x80, 0x00);  // Change Polarity
            }
            else
            {
                hdmirxset(REG_RX_1B9, 0x80, 0x80);  // Change Polarity
            }
            // xxxxx
        }
        else
        {
            if ((hdmirxrd(REG_RX_1C9) & 0x80) >> 7)
            {
                hdmirxset(REG_RX_1C9, 0x80, 0x00);  // Change Polarity
            }
            else
            {
                hdmirxset(REG_RX_1C9, 0x80, 0x80);  // Change Polarity
            }
        }
        // FIX_ID_002 xxxxx

        hdmirxset(REG_RX_1C5, 0x10, 0x10);

        chgbank(0);

        EQ_PORT1_PRINTF((" ############# TogglePolarity Trigger Port 1 EQ ###############\n"));
        // hdmirxset(0x3E, 0x80, 0x00);
        // #ifdef   ENABLE_AUTO_TRIGGER
        // hdmirxset(0x3E, 0x80, 0x80);
        // #endif
        hdmirxset(REG_RX_03A, 0xFF, 0x38);
        hdmirxset(REG_RX_03A, 0x04, 0x04);
        hdmirxset(REG_RX_03A, 0x04, 0x00);
    }
    else
    {
        EQ_PORT0_PRINTF((" ############# TogglePolarity Port 0###############\n"));
        chgbank(1);
        hdmirxset(REG_RX_1B5, 0x10, 0x00);

        // xxxxx only for IT6602A0 Version
        if ((hdmirxrd(REG_RX_1B9) & 0x80) >> 7)
        {
            hdmirxset(REG_RX_1B9, 0x80, 0x00);  // Change Polarity
        }
        else
        {
            hdmirxset(REG_RX_1B9, 0x80, 0x80);  // Change Polarity
        }
        // xxxxx

        hdmirxset(REG_RX_1B5, 0x10, 0x10);
        chgbank(0);

        EQ_PORT0_PRINTF((" ############# TogglePolarity Trigger Port 0 EQ ###############\n"));
        // hdmirxset(0x26, 0x80, 0x00);
        // #ifdef   ENABLE_AUTO_TRIGGER
        // hdmirxset(0x26, 0x80, 0x80);
        // #endif
//      hdmirxset(REG_RX_022, 0x04, 0x00);
        hdmirxset(REG_RX_022, 0xFF, 0x38);  // 07-04
        hdmirxset(REG_RX_022, 0x04, 0x04);
        hdmirxset(REG_RX_022, 0x04, 0x00);
    }
        #endif
}

void
TMDSCheck (
    unsigned char ucPortSel)
{
        #ifdef _SUPPORT_AUTO_EQ_
    unsigned int    ucTMDSClk;
    unsigned char   rddata;
    unsigned char   ucClk;

// FIX_ID_009 xxxxx //verify interrupt event with reg51[0] select port
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    EQ_PORT0_PRINTF(("TMDSCheck() !!!\n"));

    if (ucPortSel == F_PORT_SEL_1)
    {
        ucClk   = hdmirxrd(REG_RX_092);
        rddata  = hdmirxrd(0x90);

        if (ucClk != 0)
        {
            if (rddata & 0x04)
            {
                ucTMDSClk = 2 * RCLKVALUE * 256 / ucClk;
            }
            else if (rddata & 0x08)
            {
                ucTMDSClk = 4 * RCLKVALUE * 256 / ucClk;
            }
            else
            {
                ucTMDSClk = RCLKVALUE * 256 / ucClk;
            }

            EQ_PORT1_PRINTF((" Port 1 TMDS CLK  = %d\n", (int) ucTMDSClk));
        }

// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
        if (HdmiI2cAddr == IT6602A0_HDMI_ADDR)
        {
            if ((ucTMDSClk < TMDSCLKVALUE_480P) || (ucTMDSClk > TMDSCLKVALUE_1080P))
            {
                hdmirxwr(REG_RX_038, 0x00); // Dr. Liu suggestion to 0x00
            }
            else
            {
                hdmirxwr(REG_RX_038, 0x3F); // Dr. Liu suggestion to 0x3F
            }
        }
// FIX_ID_002 xxxxx

        EQ_PORT1_PRINTF((" HDMI Reg038  = %X\n", (int) hdmirxrd(REG_RX_038)));

        chgbank(1);
        EQ_PORT1_PRINTF((" HDMI Reg1C1  = %X ,Reg1C2  = %X\n", hdmirxrd(REG_RX_1C1), hdmirxrd(REG_RX_1C2)));
        chgbank(0);

        // hdmirxset(0x3E, 0x80, 0x00);
        // #ifdef   ENABLE_AUTO_TRIGGER
        // hdmirxset(0x3E, 0x80, 0x80);
        // #endif

        if (ucPortAMPOverWrite[1] == 0)    // 2013-0801
        {
            // FIX_ID_001 xxxxx check State of AutoEQ
            chgbank(1);
            rddata = hdmirxrd(REG_RX_1DC);
            chgbank(0);
            if (rddata == 0)
            // FIX_ID_001 xxxxx
            {
                EQ_PORT1_PRINTF((" ############# Trigger Port 1 EQ ###############\n"));
                hdmirxset(REG_RX_03A, 0xFF, 0x38);  // 07-04
                hdmirxset(REG_RX_03A, 0x04, 0x04);
                hdmirxset(REG_RX_03A, 0x04, 0x00);
                // FIX_ID_043
                // Tirgger Auto EQ, ToggleCount = 0;
                IT6602data->ToggleCount[F_PORT_SEL_1] = 0;
                // ~FIX_ID_043
            }
        }
        else
        {
            EQ_PORT1_PRINTF((" ############# B_PORT1_TimingChgEvent###############\n"));
            IT6602data->HDMIIntEvent    |= (B_PORT1_Waiting);
            IT6602data->HDMIIntEvent    |= (B_PORT1_TimingChgEvent);
            IT6602data->HDMIWaitNo[1]   = MAX_TMDS_WAITNO;
        }
    }
    else
    {
        EQ_PORT0_PRINTF((" HDMI Reg90  = %X ,Reg91  = %X\n", (int) hdmirxrd(0x90), (int) hdmirxrd(0x91)));
        ucClk   = hdmirxrd(REG_RX_091);
        rddata  = hdmirxrd(0x90);

        if (ucClk != 0)
        {
            if (rddata & 0x01)
            {
                ucTMDSClk = 2 * RCLKVALUE * 256 / ucClk;
            }
            else if (rddata & 0x02)
            {
                ucTMDSClk = 4 * RCLKVALUE * 256 / ucClk;
            }
            else
            {
                ucTMDSClk = RCLKVALUE * 256 / ucClk;
            }

            EQ_PORT0_PRINTF((" Port 0 TMDS CLK  = %X\n", (int) ucTMDSClk));
        }

// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
        if (HdmiI2cAddr == IT6602A0_HDMI_ADDR)
        {
            // FIX_ID_007 xxxxx     //for debug IT6681  HDCP issue
            {
                chgbank(1);
                hdmirxset(REG_RX_1B1, 0x20, 0x00);  // Reg1b1[5] = 0 for disable over-write
                hdmirxset(REG_RX_1B2, 0x07, 0x04);  // default 0x04
                EQ_PORT0_PRINTF((" HDMI Reg1B1  = %X ,Reg1B2  = %X\n", (int) hdmirxrd(REG_RX_1B1), (int) hdmirxrd(REG_RX_1B2)));
                chgbank(0);
            }
            // FIX_ID_007 xxxxx
        }
// FIX_ID_002 xxxxx

// FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
        if (HdmiI2cAddr == IT6602A0_HDMI_ADDR)
        {
            if ((ucTMDSClk < TMDSCLKVALUE_480P) || (ucTMDSClk > TMDSCLKVALUE_1080P))
            {
                hdmirxwr(REG_RX_020, 0x00); // Dr. Liu suggestion to 0x00
            }
            else
            {
                hdmirxwr(REG_RX_020, 0x3F); // Dr. Liu suggestion to 0x3F
            }
        }
// FIX_ID_002 xxxxx

        EQ_PORT0_PRINTF((" HDMI Reg020  = %X\n", (int) hdmirxrd(REG_RX_020)));

        // hdmirxset(0x26, 0x80, 0x00);
        // #ifdef   ENABLE_AUTO_TRIGGER
        // hdmirxset(0x26, 0x80, 0x80);
        // #endif

        if (ucPortAMPOverWrite[0] == 0)    // 2013-0801
        {
            // FIX_ID_001 xxxxx check State of AutoEQ
            chgbank(1);
            rddata = hdmirxrd(REG_RX_1D4);
            chgbank(0);

// FIX_ID_032 xxxxx //Support HDCP Repeater function for HDMI Tx device
            #ifdef _ONLY_SUPPORT_MANUAL_EQ_ADJUST_
                #ifndef _SUPPORT_HDCP_REPEATER_
            // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
                    #ifdef _SUPPORT_EQ_ADJUST_
            HDMIStartEQDetect(&(IT6602data->EQPort[F_PORT_SEL_0]));
                    #endif
            // FIX_ID_001 xxxxx
                #endif
            #else
            if (rddata == 0)
            // FIX_ID_001 xxxxx
            {
                EQ_PORT0_PRINTF((" ############# Trigger Port 0 EQ ###############\n"));
                hdmirxset(REG_RX_022, 0xFF, 0x38);  // 07-04
                hdmirxset(REG_RX_022, 0x04, 0x04);
                hdmirxset(REG_RX_022, 0x04, 0x00);
                // FIX_ID_043
                // Tirgger Auto EQ, ToggleCount = 0;
                IT6602data->ToggleCount[F_PORT_SEL_0] = 0;
                // ~FIX_ID_043
            }
            #endif
// FIX_ID_032 xxxxx

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
            // if Authentication start interrupt with CKon interrupt then do TMDSCheck() at first.
            IT6602data->HDMIIntEvent &= ~(B_PORT0_TMDSEvent | B_PORT0_Waiting | B_PORT0_TimingChgEvent);
// FIX_ID_033 xxxxx
        }
        else
        {
            EQ_PORT0_PRINTF((" ############# B_PORT0_TimingChgEvent###############\n"));
            IT6602data->HDMIIntEvent    |= (B_PORT0_Waiting);
            IT6602data->HDMIIntEvent    |= (B_PORT0_TimingChgEvent);
            IT6602data->HDMIWaitNo[0]   = MAX_TMDS_WAITNO;
        }
    }

        #endif
}

void
OverWriteAmpValue2EQ (
    unsigned char ucPortSel)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    EQ_DEBUG_PRINTF((" 111111111111111111 OverWriteAmpValue2EQ 111111111111111111111111111111\n"));
    EQ_DEBUG_PRINTF((" 111111111111111111 OverWriteAmpValue2EQ 111111111111111111111111111111\n"));
    EQ_DEBUG_PRINTF((" 111111111111111111 OverWriteAmpValue2EQ 111111111111111111111111111111\n"));
    EQ_DEBUG_PRINTF((" 111111111111111111 OverWriteAmpValue2EQ 111111111111111111111111111111\n"));
    EQ_DEBUG_PRINTF((" 111111111111111111 OverWriteAmpValue2EQ 111111111111111111111111111111\n"));
    EQ_DEBUG_PRINTF((" 111111111111111111 OverWriteAmpValue2EQ 111111111111111111111111111111\n"));

    if (ucPortSel == F_PORT_SEL_1)
    {
        if ((ucPortAMPValid[1] & 0x3F) == 0x3F)
        {
            ucPortAMPOverWrite[F_PORT_SEL_1]    = 1;    // 2013-0801
            ucEQMode[F_PORT_SEL_1]              = 0;    // 0 for Auto Mode
            EQ_PORT1_PRINTF(("#### REG_RX_03E = 0x%X ####\n", (int) hdmirxrd(REG_RX_03E)));
            hdmirxset(REG_RX_03E, 0x20, 0x20);          // Manually set RS Value
            EQ_PORT1_PRINTF(("#### REG_RX_03E = 0x%X ####\n", (int) hdmirxrd(REG_RX_03E)));

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
            if (ucChannelB[F_PORT_SEL_1] < MinEQValue)
            {
                hdmirxwr(REG_RX_03F, MinEQValue);
            }
            else
            {
                hdmirxwr(REG_RX_03F, (ucChannelB[F_PORT_SEL_1] & 0x7F));
            }

            if (ucChannelG[F_PORT_SEL_1] < MinEQValue)
            {
                hdmirxwr(REG_RX_040, MinEQValue);
            }
            else
            {
                hdmirxwr(REG_RX_040, (ucChannelG[F_PORT_SEL_1] & 0x7F));
            }

            if (ucChannelR[F_PORT_SEL_1] < MinEQValue)
            {
                hdmirxwr(REG_RX_041, MinEQValue);
            }
            else
            {
                hdmirxwr(REG_RX_041, (ucChannelR[F_PORT_SEL_1] & 0x7F));
            }

            // xxxxx 2014-0421 disable ->           hdmirxset(REG_RX_03F,0xFF,(ucChannelB[F_PORT_SEL_1] & 0x7F));
            // xxxxx 2014-0421 disable ->           hdmirxset(REG_RX_040,0xFF,(ucChannelG[F_PORT_SEL_1] & 0x7F));
            // xxxxx 2014-0421 disable ->           hdmirxset(REG_RX_041,0xFF,(ucChannelR[F_PORT_SEL_1] & 0x7F));

            // if Auto EQ done  interrupt then clear HDMI Event !!!
            IT6602data->HDMIIntEvent &= ~(B_PORT1_TMDSEvent | B_PORT1_Waiting | B_PORT1_TimingChgEvent);

            EQ_PORT1_PRINTF((" ############# Over-Write port 1 EQ###############\n"));
            EQ_PORT1_PRINTF((" ############# B port 1 Reg03F = 0x%X  ###############\n", (int) hdmirxrd(REG_RX_03F)));
            EQ_PORT1_PRINTF((" ############# G port 1 Reg040 = 0x%X  ###############\n", (int) hdmirxrd(REG_RX_040)));
            EQ_PORT1_PRINTF((" ############# R port 1 Reg041 = 0x%X  ###############\n", (int) hdmirxrd(REG_RX_041)));

            //  HDMICheckErrorCount(&(IT6602data->EQPort[F_PORT_SEL_1]));   //07-04 for port 1

            // hdmirxset(REG_RX_03A, 0xFF, 0x00);   //07-08 [3] power down  ?????
            hdmirxwr(REG_RX_03A, 0x00); // power down auto EQ
            hdmirxwr(0xD0, 0xC0);
// FIX_ID_033 xxxxx
        }
    }
    else
    {
        // 07-08
        {
            if ((ucPortAMPValid[F_PORT_SEL_0] & 0x3F) == 0x3F)
            {
                ucPortAMPOverWrite[F_PORT_SEL_0]    = 1;    // 2013-0801
                ucEQMode[F_PORT_SEL_0]              = 0;    // 0 for Auto Mode
                EQ_PORT0_PRINTF(("#### REG_RX_026 = 0x%X ####\n", (int) hdmirxrd(REG_RX_026)));
                hdmirxset(REG_RX_026, 0x20, 0x20);          // Manually set RS Value
                EQ_PORT0_PRINTF(("#### REG_RX_026 = 0x%X ####\n", (int) hdmirxrd(REG_RX_026)));

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
                if (ucChannelB[F_PORT_SEL_0] < MinEQValue)
                {
                    hdmirxwr(REG_RX_027, MinEQValue);
                }
                else
                {
                    hdmirxwr(REG_RX_027, (ucChannelB[F_PORT_SEL_0] & 0x7F));
                }

                if (ucChannelG[F_PORT_SEL_0] < MinEQValue)
                {
                    hdmirxwr(REG_RX_028, MinEQValue);
                }
                else
                {
                    hdmirxwr(REG_RX_028, (ucChannelG[F_PORT_SEL_0] & 0x7F));
                }

                if (ucChannelR[F_PORT_SEL_0] < MinEQValue)
                {
                    hdmirxwr(REG_RX_029, MinEQValue);
                }
                else
                {
                    hdmirxwr(REG_RX_029, (ucChannelR[F_PORT_SEL_0] & 0x7F));
                }

// xxxxx 2014-0421 disable ->           hdmirxset(REG_RX_027,0xFF,(ucChannelB[F_PORT_SEL_0] & 0x7F));
// xxxxx 2014-0421 disable ->           hdmirxset(REG_RX_028,0xFF,(ucChannelG[F_PORT_SEL_0] & 0x7F));
// xxxxx 2014-0421 disable ->           hdmirxset(REG_RX_029,0xFF,(ucChannelR[F_PORT_SEL_0] & 0x7F));

                // if Auto EQ done  interrupt then clear HDMI Event !!!
                IT6602data->HDMIIntEvent &= ~(B_PORT0_TMDSEvent | B_PORT0_Waiting | B_PORT0_TimingChgEvent);

                EQ_PORT0_PRINTF((" ############# Over-Write port 0 EQ###############\n"));
                EQ_PORT0_PRINTF((" ############# B port 0 REG_RX_027 = 0x%X  ###############\n", (int) hdmirxrd(REG_RX_027)));
                EQ_PORT0_PRINTF((" ############# G port 0 REG_RX_028 = 0x%X  ###############\n", (int) hdmirxrd(REG_RX_028)));
                EQ_PORT0_PRINTF((" ############# R port 0 REG_RX_029 = 0x%X  ###############\n", (int) hdmirxrd(REG_RX_029)));

                //  HDMICheckErrorCount(&(IT6602data->EQPort[F_PORT_SEL_0]));   //07-04 for port 1

                // hdmirxset(REG_RX_022, 0xFF, 0x00);   //07-08 [3] power down
                hdmirxwr(REG_RX_022, 0x00); // power down auto EQ
                hdmirxwr(0xD0, 0x30);
// FIX_ID_033 xxxxx
            }
        }
    }
}
// -------------------------------------------------------------------------------------------------------
    #endif

// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_EQ_ADJUST_

void
HDMIStartEQDetect (
    struct IT6602_eq_data * ucEQPort)
/*
 * This is the HDMIRX Start EQ Detect
 * @param IT6602_eq_data
 * @return void
 */
{
    unsigned char ucPortSel;

// FIX_ID_035 xxxxx

    if (ucEQPort->ucEQState == 0xFF)
    {
        ucPortSel = hdmirxrd(REG_RX_051) & B_PORT_SEL;

        if (ucPortSel == ucEQPort->ucPortID)
        {
            HDMISwitchEQstate(ucEQPort, 0);                 // for SCDT off state
        }
        else
        {
            HDMISwitchEQstate(ucEQPort, EQSTATE_WAIT + 1);  // for SCDT on state
        }

        ucEQPort->f_manualEQadjust = TRUE;
        HDMIAdjustEQ(ucEQPort);

// FIX_ID_010 xxxxx     //Add JudgeBestEQ to avoid wrong EQ setting
        ucEQPort->ErrorCount[0] = MAXECCWAIT;
        ucEQPort->ErrorCount[1] = MAXECCWAIT;
        ucEQPort->ErrorCount[2] = MAXECCWAIT;
// FIX_ID_010 xxxxx
    }
}

void
HDMISetEQValue (
    struct IT6602_eq_data   * ucEQPort,
    unsigned char           ucIndex)
/*
 * This is the HDMIRX Set Manual EQ value
 * @param IT6602_eq_data
 * @return void
 */
{
    if (ucIndex < MaxEQIndex)
    {
        if (ucEQPort->ucPortID == F_PORT_SEL_0)
        {
        #ifdef _SUPPORT_AUTO_EQ_
            ucEQMode[F_PORT_SEL_0] = 1;         // 1 for Manual Mode
        #endif
            hdmirxset(REG_RX_026, 0x20, 0x20);  // 07-04 add for adjust EQ
            hdmirxwr(REG_RX_027, IT6602EQTable[ucIndex]);
            EQ_PORT0_PRINTF(("Port=%X ,ucIndex = %X ,HDMISetEQValue Reg027 = %X\n", (int) ucEQPort->ucPortID, (int) ucIndex, (int) hdmirxrd(REG_RX_027)));
        }
        else
        {
        #ifdef _SUPPORT_AUTO_EQ_
            ucEQMode[F_PORT_SEL_1] = 1;         // 1 for Manual Mode
        #endif
            hdmirxset(REG_RX_03E, 0x20, 0x20);  // 07-04 add for adjust EQ
            hdmirxwr(REG_RX_03F, IT6602EQTable[ucIndex]);
            EQ_PORT1_PRINTF(("Port=%X ,ucIndex = %X ,HDMISetEQValue Reg03F = %X\n", (int) ucEQPort->ucPortID, (int) ucIndex, (int) hdmirxrd(REG_RX_03F)));
        }
    }
}

void
HDMISwitchEQstate (
    struct IT6602_eq_data   * ucEQPort,
    unsigned char           state)
/*
 * This is the HDMIRX Switch EQ State
 * @param IT6602_eq_data
 * @return void
 */
{
    ucEQPort->ucEQState = state;

    EQ_DEBUG_PRINTF(("!!! Port=%X ,HDMISwitchEQstate %X\n", (int) ucEQPort->ucPortID, (int) ucEQPort->ucEQState));

    switch (ucEQPort->ucEQState)
    {
        case EQSTATE_START:
            HDMISetEQValue(ucEQPort, 0);
            break;

        case EQSTATE_LOW:
            HDMISetEQValue(ucEQPort, 1);
            break;

        case EQSTATE_MIDDLE:
            HDMISetEQValue(ucEQPort, 2);
            break;

        case EQSTATE_HIGH:
            HDMISetEQValue(ucEQPort, 3);
            break;

        default:
        #if 1
            // xxxxx 2014-0529 //HDCP Content On/Off
            IT6602_HDCP_ContentOff(ucEQPort->ucPortID, 0);
            // xxxxx 2014-0529
            // FIX_ID_037 xxxxx

            HDMISetEQValue(ucEQPort, 0xff);  // dont care
        #endif
            break;
    }

    // !!! re-start the error count !!!

    ucEQPort->ucPkt_Err         = 0;
    ucEQPort->ucECCvalue        = 0;
    ucEQPort->ucECCfailCount    = 0;
}

void
HDMICheckSCDTon (
    struct IT6602_eq_data * ucEQPort)
/*
 * This is the HDMIRX SCDT on
 * @param IT6602_eq_data
 * @return void
 */
{
    unsigned char   ucResult = 0;
    unsigned char   Receive_Err;
    unsigned char   ucStatus;
    unsigned char   ucCurrentPort;
    unsigned char   ucHDCP;

    ucCurrentPort = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    if (ucEQPort->ucPortID != ucCurrentPort)
    {
        return;
    }

    if (ucEQPort->ucPortID == F_PORT_SEL_1)
    {
        ucStatus    = hdmirxrd(REG_RX_P1_SYS_STATUS);
        // !!! check ECC error register  !!!
        Receive_Err = hdmirxrd(REG_RX_0B7);
        hdmirxwr(REG_RX_0B7, Receive_Err);

        ucHDCP = hdmirxrd(REG_RX_095);
    }
    else
    {
        ucStatus    = hdmirxrd(REG_RX_P0_SYS_STATUS);
        // !!! check ECC error register  !!!
        Receive_Err = hdmirxrd(REG_RX_0B2);
        hdmirxwr(REG_RX_0B2, Receive_Err);

        ucHDCP = hdmirxrd(REG_RX_093);
    }

    if ((ucStatus & (B_P0_SCDT | B_P0_PWR5V_DET | B_P0_RXCK_VALID)) == (B_P0_PWR5V_DET | B_P0_RXCK_VALID))
    {
        ucEQPort->ucECCfailCount++;
    }

    EQ_DEBUG_PRINTF(("Port=%d, CheckSCDTon=%d, Receive_Err=%X, ucECCfailCount=%X, SCDT=%X, HDCP=%X\n",
        (int) ucEQPort->ucPortID, (int) ucEQPort->ucEQState, (int) Receive_Err, (int)ucEQPort->ucECCfailCount, (int) ucStatus, (int) ucHDCP));

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
    if ((Receive_Err & 0xC0) != 0x00)
    {
        ucEQPort->ucECCvalue++;

        // xxxxx 2014-0529 //Manual Content On/Off
        IT6602_HDCP_ContentOff(ucEQPort->ucPortID, 1);
        // xxxxx 2014-0529
        // FIX_ID_037 xxxxx

        // xxxxx 2014-0421
        // if((Receive_Err & 0xC0) == 0xC0)
        if (ucEQPort->ucECCvalue > ((MINECCFAILCOUNT / 2)))
        // xxxxx 2014-0421
        {
            ucEQPort->ucECCvalue = 0;
            EQ_DEBUG_PRINTF(("HDMICheckSCDTon() for ECC / Deskew issue !!!"));

            if (ucEQPort->ucPortID == F_PORT_SEL_1)
            {
                if (hdmirxrd(REG_RX_038) == 0x00)
                {
                    hdmirxwr(REG_RX_038, 0x3F);     // Dr. Liu suggestion to 0x00
                }
                // else
                //  hdmirxwr(REG_RX_038,0x00);  // Dr. Liu suggestion to 0x3F

                EQ_PORT1_PRINTF(("Port 1 Reg38=%X !!!\n", (int) hdmirxrd(REG_RX_038)));
            }
            else
            {
                if (hdmirxrd(REG_RX_020) == 0x00)
                {
                    hdmirxwr(REG_RX_020, 0x3F);     // Dr. Liu suggestion to 0x00
                    // else
                    //  hdmirxwr(REG_RX_020,0x00);  // Dr. Liu suggestion to 0x3F

                    EQ_PORT0_PRINTF(("Port 0 Reg20=%X !!!\n", (int) hdmirxrd(REG_RX_020)));
                }
            }
        }
    }
// FIX_ID_033 xxxxx

    if (ucEQPort->ucEQState == EQSTATE_WAIT - 1)
    {
        // FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
        EQ_DEBUG_PRINTF(("Port=%d, CheckSCDTon=%d, Receive_Err=%X, ucECCfailCount=%X, SCDT=%X, HDCP=%X\n",
            (int) ucEQPort->ucPortID, (int) ucEQPort->ucEQState, (int) Receive_Err, (int)ucEQPort->ucECCfailCount, (int) ucStatus, (int) ucHDCP));

        if ((Receive_Err & 0xC0) == 0xC0)
        {
            EQ_DEBUG_PRINTF(("HDMICheckSCDTon() CDR reset for Port %d ECC_TIMEOUT !!!\n", ucCurrentPort));
            hdmirx_ECCTimingOut(ucCurrentPort);

            HDMISwitchEQstate(ucEQPort, EQSTATE_END);
            return;
        }
        // FIX_ID_033 xxxxx

        #ifdef _SUPPORT_AUTO_EQ_
        if ((ucEQPort->ucECCfailCount) == 0)
        {
            if (ucEQPort->ucPortID == F_PORT_SEL_1)
            {
                if (ucEQMode[F_PORT_SEL_1] == 0) // verfiy Auto EQ Value wehn auto EQ finish
                {
                    if (((ucChannelB[F_PORT_SEL_1] & 0x7F) < 0x0F) ||
                        ((ucChannelG[F_PORT_SEL_1] & 0x7F) < 0x0F) ||
                        ((ucChannelR[F_PORT_SEL_1] & 0x7F) < 0x0F))

                    {
                        ucResult = 1;               // 1 for EQ start
                    }
                }
            }
            else
            {
                if (ucEQMode[F_PORT_SEL_0] == 0)    // verfiy Auto EQ Value when auto EQ finish
                {
                    {
                        if (((ucChannelB[F_PORT_SEL_0] & 0x7F) < 0x0F) ||
                            ((ucChannelG[F_PORT_SEL_0] & 0x7F) < 0x0F) ||
                            ((ucChannelR[F_PORT_SEL_0] & 0x7F) < 0x0F))

                        {
                            ucResult = 1;   // 1 for EQ start
                        }
                    }
                }
            }

            if (ucResult == 0)              // no need to do manual EQ adjust when SCDT always On !!!
            {
                HDMISwitchEQstate(ucEQPort, EQSTATE_END);
                return;
            }
        }
        #endif

        HDMISwitchEQstate(ucEQPort, EQSTATE_WAIT);
    }
}

void
HDMIPollingErrorCount (
    struct IT6602_eq_data * ucEQPort)
/*
 * This is the HDMIPollingErrorCount
 * @param
 * @return void
 */
{
    unsigned char   Receive_Err;
    unsigned char   Video_Err;
    unsigned char   Code_Err;
    unsigned char   Pkt_Err;
    unsigned char   CrtErr;
    unsigned char   ucHDCP;
    unsigned char   ucStatus;

    unsigned char   ucCurrentPort;
    ucCurrentPort = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    if (ucEQPort->ucPortID == F_PORT_SEL_1)
    {
        ucStatus    = hdmirxrd(REG_RX_P1_SYS_STATUS);
        // !!! check ECC error register  !!!
        Receive_Err = hdmirxrd(REG_RX_0B7);
        Video_Err   = hdmirxrd(REG_RX_0B8) & 0xE0;
        Code_Err    = hdmirxrd(REG_RX_0B9);
        Pkt_Err     = hdmirxrd(REG_RX_0BA);
        CrtErr      = hdmirxrd(REG_RX_0BB);

        hdmirxwr(REG_RX_0B7, Receive_Err);
        hdmirxwr(REG_RX_0B8, Video_Err);
        hdmirxwr(REG_RX_0B9, Code_Err);
        hdmirxwr(REG_RX_0BA, Pkt_Err);
        hdmirxwr(REG_RX_0BB, CrtErr);

        ucHDCP = hdmirxrd(REG_RX_095);
    }
    else
    {
        ucStatus    = hdmirxrd(REG_RX_P0_SYS_STATUS);
        // !!! check ECC error register  !!!
        Receive_Err = hdmirxrd(REG_RX_0B2);
        Video_Err   = hdmirxrd(REG_RX_0B3) & 0xE0;
        Code_Err    = hdmirxrd(REG_RX_0B4);
        Pkt_Err     = hdmirxrd(REG_RX_0B5);
        CrtErr      = hdmirxrd(REG_RX_0B6);

        hdmirxwr(REG_RX_0B2, Receive_Err);
        hdmirxwr(REG_RX_0B3, Video_Err);
        hdmirxwr(REG_RX_0B4, Code_Err);
        hdmirxwr(REG_RX_0B5, Pkt_Err);
        hdmirxwr(REG_RX_0B6, CrtErr);

        ucHDCP = hdmirxrd(REG_RX_093);
    }

    // xxxxx 2013-0903
    if (ucCurrentPort == ucEQPort->ucPortID)
    {
        if ((ucStatus & B_P0_SCDT) == 0x00)
        {
            Receive_Err = 0xFF;

            // xxxxx 2013-0812  ++++
            ucEQPort->ucECCfailCount |= 0x80;
            // xxxxx 2013-0812
        }
    }
    // xxxxx 2013-0903

    EQ_DEBUG_PRINTF(("Port=%d ,EQState2No=%d, Receive_Err=%X, HDCP=%X\n",
        (int) ucEQPort->ucPortID, (int) ucEQPort->ucEQState, (int) Receive_Err, (int) ucHDCP));

        #if 1
// FIX_ID_007 xxxxx     //07-18 xxxxx for ATC 8-7 Jitter Tolerance
    if ((Pkt_Err == 0xFF) || (Code_Err == 0xFF))
    {
        ucEQPort->ucPkt_Err++;  // judge whether CDR reset
    }
    else
    {
        ucEQPort->ucPkt_Err = 0;
    }

    if (ucEQPort->ucPkt_Err > (MINECCFAILCOUNT - 2))
    {
        if (ucEQPort->ucEQState > EQSTATE_START)
        {
// FIX_ID_020 xxxxx     //Turn off DEQ for HDMI port 1 with 20m DVI Cable
            EQ_DEBUG_PRINTF(("1111111111111111111111111111111111111111111111111111111111111111111111111\n"));

            if (ucEQPort->ucPortID == F_PORT_SEL_1)
            {
                Code_Err = hdmirxrd(REG_RX_0B9);
                hdmirxwr(REG_RX_0B9, Code_Err);

                if (Code_Err == 0xFF)
                {
                    if (hdmirxrd(REG_RX_038) == 0x00)
                    {
                        hdmirxwr(REG_RX_038, 0x3F);     // Dr. Liu suggestion to 0x00
                    }
                    else
                    {
                        hdmirxwr(REG_RX_038, 0x00);     // Dr. Liu suggestion to 0x3F
                    }
                    EQ_DEBUG_PRINTF(("Port 1 Reg38=%X !!!\n", (int) hdmirxrd(REG_RX_038)));
                }
            }
            else
            {
                Code_Err = hdmirxrd(REG_RX_0B4);
                hdmirxwr(REG_RX_0B4, Code_Err);

                if (Code_Err == 0xFF)
                {
                    if (hdmirxrd(REG_RX_020) == 0x00)
                    {
                        hdmirxwr(REG_RX_020, 0x3F);     // Dr. Liu suggestion to 0x00
                    }
                    else
                    {
                        hdmirxwr(REG_RX_020, 0x00);     // Dr. Liu suggestion to 0x3F
                    }
                    EQ_DEBUG_PRINTF(("Port 0 Reg20=%X !!!\n", (int) hdmirxrd(REG_RX_020)));
                }
            }
            EQ_DEBUG_PRINTF(("1111111111111111111111111111111111111111111111111111111111111111111111111\n"));
// FIX_ID_020 xxxxx

            if (ucEQPort->ucPortID == F_PORT_SEL_0)
            {
                hdmirxset(REG_RX_011, (B_P0_DCLKRST | B_P0_CDRRST), (B_P0_DCLKRST | B_P0_CDRRST /*|B_P0_SWRST*/));
                hdmirxset(REG_RX_011, (B_P0_DCLKRST | B_P0_CDRRST), 0x00);
                EQ_PORT0_PRINTF((" HDMIPollingErrorCount( ) Port 0 CDR reset !!!!!!!!!!!!!!!!!!\n"));
            }
            else
            {
                hdmirxset(REG_RX_018, (B_P1_DCLKRST | B_P1_CDRRST), (B_P1_DCLKRST | B_P1_CDRRST /*|B_P1_SWRST*/));
                hdmirxset(REG_RX_018, (B_P1_DCLKRST | B_P1_CDRRST), 0x00);
                EQ_PORT0_PRINTF((" HDMIPollingErrorCount( ) Port 1 CDR reset !!!!!!!!!!!!!!!!!!\n"));
            }
        }
        ucEQPort->ucPkt_Err = 0;

// xxxxx 2013-0812  ++++
        ucEQPort->ucECCfailCount    |= 0x40;
        ucEQPort->ucECCfailCount    &= 0xF0;
// xxxxx 2013-0812
    }
    // 07-18 xxxxx
// FIX_ID_007 xxxxx
        #endif

//  if(Receive_Err>32 )

// xxxxx 2013-0812  ++++
    if (Receive_Err != 0)
// xxxxx 2013-0812
    {
        EQ_DEBUG_PRINTF(("Video_Err = %X\n", (int) Video_Err));
        EQ_DEBUG_PRINTF(("Code_Err = %X\n", (int) Code_Err));
        EQ_DEBUG_PRINTF(("Pkt_Err = %X\n", (int) Pkt_Err));
        EQ_DEBUG_PRINTF(("CrtErr = %X\n", (int) CrtErr));

        ucEQPort->ucECCvalue++;
        ucEQPort->ucECCfailCount++;
    }
    else
    {
        ucEQPort->ucECCfailCount = 0;
    }

//  EQ_DEBUG_PRINTF(("ucEQPort->ucECCvalue = %X 666666666666666666666666\n",ucEQPort->ucECCvalue));
// xxxxx 2013-0812  ++++
        #if 1
    if ((ucEQPort->ucECCfailCount & 0x7F) < (0x40))     // before CDR reset , dont care pkt_error and code_error
    {
        if ((Pkt_Err == 0xFF) || (Code_Err == 0xFF))
        {
            return;
        }
    }
        #endif
// xxxxx 2013-0812

//  if((ucEQPort->ucECCfailCount & 0x7F) > (0x40 + MINECCFAILCOUNT-2))

// xxxxx 2013-0812  ++++
    if ((ucEQPort->ucECCfailCount & 0x0F) > (MINECCFAILCOUNT - 2))
    {
        ucEQPort->ucECCvalue    = MAXECCWAIT;

        ucCurrentPort           = hdmirxrd(REG_RX_051) & B_PORT_SEL;

        if (ucEQPort->ucPortID == F_PORT_SEL_1)
        {
            ucStatus = hdmirxrd(REG_RX_P1_SYS_STATUS);
        }
        else
        {
            ucStatus = hdmirxrd(REG_RX_P0_SYS_STATUS);
        }

        if (ucCurrentPort == ucEQPort->ucPortID)
        {
            if (((ucStatus & B_P0_SCDT) == 0x00) || ((ucEQPort->ucECCfailCount & 0x80) != 0x00))
            {
                ucEQPort->ucECCvalue = MAXECCWAIT | 0x80;   // 0x80 for Identify SCDT off with Ecc error
            }
        }

        StoreEccCount(ucEQPort);                            // abnormal judge ucECCvalue mode

        if (ucEQPort->ucEQState < EQSTATE_START)
        {
            HDMISwitchEQstate(ucEQPort, EQSTATE_START);
        }
        else if (ucEQPort->ucEQState < EQSTATE_LOW)
        {
            HDMISwitchEQstate(ucEQPort, EQSTATE_LOW);
        }
        else if (ucEQPort->ucEQState < EQSTATE_MIDDLE)
        {
            HDMISwitchEQstate(ucEQPort, EQSTATE_MIDDLE);
        }
        else if (ucEQPort->ucEQState <= EQSTATE_HIGH)
        {
            HDMISwitchEQstate(ucEQPort, EQSTATE_HIGH);
        }
    }
// xxxxx 2013-0812
}

void
HDMIJudgeECCvalue (
    struct IT6602_eq_data * ucEQPort)
/*
 * This is the HDMIJudgeECCvalue
 * @param IT6602_eq_data
 * @return void
 */
{
    // unsigned char uc;

    EQ_DEBUG_PRINTF(("!!! HDMI Judge ECCvalue( ) %X!!!\n", (int) ucEQPort->ucECCvalue));

    StoreEccCount(ucEQPort);    // normal judge ucECCvalue mode

    if ((ucEQPort->ucECCvalue) > (MAXECCWAIT / 2))
    {
        // uc = CheckErrorCode(ucEQPort);

        // if(CheckErrorCode()==FALSE)
        // if(uc == FALSE)
        {
            if (ucEQPort->ucEQState == EQSTATE_START)
            {
                HDMISwitchEQstate(ucEQPort, EQSTATE_START);
            }
            else if (ucEQPort->ucEQState == EQSTATE_LOW)
            {
                HDMISwitchEQstate(ucEQPort, EQSTATE_LOW);
            }
            else if (ucEQPort->ucEQState == EQSTATE_MIDDLE)
            {
                HDMISwitchEQstate(ucEQPort, EQSTATE_MIDDLE);
            }
            else if (ucEQPort->ucEQState == EQSTATE_HIGH)
            {
                HDMISwitchEQstate(ucEQPort, EQSTATE_HIGH);
            }
        }
    }
    else
    {
        HDMISwitchEQstate(ucEQPort, EQSTATE_END);    // quit EQadjust( )
    }

    ucEQPort->ucPkt_Err         = 0;
    ucEQPort->ucECCvalue        = 0;
    ucEQPort->ucECCfailCount    = 0;
}

void
HDMIAdjustEQ (
    struct IT6602_eq_data * ucEQPort)
/*
 * This is the HDMIAdjustEQ
 * @param IT6602_eq_data
 * @return void
 */
{
    unsigned char ucCurrentPort;
    ucCurrentPort = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    switch (ucEQPort->ucEQState)
    {
        case EQSTATE_WAIT:
            break;

        case EQSTATE_START:
        case EQSTATE_LOW:
        case EQSTATE_MIDDLE:
            HDMIJudgeECCvalue(ucEQPort);
            break;

        case EQSTATE_HIGH:
            HDMIJudgeECCvalue(ucEQPort);
            ucEQPort->ucEQState = EQSTATE_END;
            break;

        case EQSTATE_HIGH + 1:
        case EQSTATE_END + 1:

// xxxxx 2013-0904 for pc debug only
            ucEQPort->f_manualEQadjust  = FALSE;
            ucEQPort->ucEQState         = 0xFF;
// xxxxx 2013-0904 for pc debug only

            if (ucEQPort->ucPortID == ucCurrentPort)
            {
                IT6602VideoCountClr();
            }

            break;

        case 0xff:
//          EQ_DEBUG_PRINTF(("====================== f_manualEQadjust = FALSE ======================\n"));
//          EQ_DEBUG_PRINTF(("====================== f_manualEQadjust = FALSE ======================\n"));
            // disable ->

        default:
            break;
    }

    if (ucEQPort->ucEQState != 0xFF)
    {
        if (ucEQPort->ucEQState < EQSTATE_WAIT)          // 20120410
        {
            HDMICheckSCDTon(ucEQPort);
        }
        else if (ucEQPort->ucEQState < EQSTATE_HIGH)
        {
            HDMIPollingErrorCount(ucEQPort);
        }
//      else
//          HDMICheckErrorCount(ucEQPort);

        ucEQPort->ucEQState++;
    }
    else
    {
        ucEQPort->f_manualEQadjust = FALSE;
    }
}
// disable ->

// FIX_ID_010 xxxxx     //Add JudgeBestEQ to avoid wrong EQ setting
void
StoreEccCount (
    struct IT6602_eq_data * ucEQPort)
{
    EQ_DEBUG_PRINTF(("StoreEccCount() ucEQPort->ucECCvalue = %02X\n", (int) ucEQPort->ucECCvalue));

    if (ucEQPort->ucEQState <= EQSTATE_LOW)
    {
        ucEQPort->ErrorCount[0] = ucEQPort->ucECCvalue;
    }
    else if (ucEQPort->ucEQState <= EQSTATE_MIDDLE)
    {
        ucEQPort->ErrorCount[1] = ucEQPort->ucECCvalue;
    }
    else if (ucEQPort->ucEQState <= EQSTATE_HIGH)
    {
        ucEQPort->ErrorCount[2] = ucEQPort->ucECCvalue;
        JudgeBestEQ(ucEQPort);
    }
}

void
JudgeBestEQ (
    struct IT6602_eq_data * ucEQPort)
{
    unsigned char i, j, Result;

    j       = 0;
    Result  = ucEQPort->ErrorCount[0];

    for (i = 1; i < MaxEQIndex; i++)
    {
// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
// use Min value to be best EQ !            if(Result>ucEQPort->ErrorCount[i])
// use Max value to be best EQ !            if(Result>=ucEQPort->ErrorCount[i])
        if (Result >= ucEQPort->ErrorCount[i])
// FIX_ID_033 xxxxx
        {
            Result  = ucEQPort->ErrorCount[i];
            j       = i;
        }
    }

    EQ_DEBUG_PRINTF((" Best IT6602EQTable ErrorCount[%X]=%X !!! IT6602EQTable Value=%X !!!\n", (int) j, (int) Result, (int) IT6602EQTable[j]));

    // if(j==0 && Result==0)
// 2014-0102 bug xxxxx !!!!!
    if (ucEQPort->ucPortID == F_PORT_SEL_0)
    {
        #ifdef _SUPPORT_AUTO_EQ_
        if ((hdmirxrd(REG_RX_027) & 0x80) == 0)
        {
            // (void)printf(" Use Auto EQ Value\n",j,Result,IT6602EQTable[j]);

            //          AmpValidCheck(ucEQPort->ucPortID);
            OverWriteAmpValue2EQ(ucEQPort->ucPortID);
        }
        else
        #endif
        {
            hdmirxset(REG_RX_026, 0x20, 0x20);          // 07-04 add for adjust EQ
            hdmirxwr(REG_RX_027, IT6602EQTable[j]);
            EQ_PORT0_PRINTF(("Port=%X ,ucIndex = %X ,JudgeBestEQ Reg027 = %X\n", (int) ucEQPort->ucPortID, (int) j, (int) hdmirxrd(REG_RX_027)));
        }
    }
// #ifdef ENABLE_INTPUT_PORT1
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    else
    {
        #ifdef _SUPPORT_AUTO_EQ_
        if ((hdmirxrd(REG_RX_03F) & 0x80) == 0)
        {
            // (void)printf(" Use Auto EQ Value\n",j,Result,IT6602EQTable[j]);

            //          AmpValidCheck(ucEQPort->ucPortID);
            OverWriteAmpValue2EQ(ucEQPort->ucPortID);
        }
        else
        #endif
        {
            hdmirxset(REG_RX_03E, 0x20, 0x20);          // 07-04 add for adjust EQ
            hdmirxwr(REG_RX_03F, IT6602EQTable[j]);
            EQ_PORT1_PRINTF(("Port=%X ,ucIndex = %X ,JudgeBestEQ Reg03F = %X\n", (int) ucEQPort->ucPortID, (int) j, (int) hdmirxrd(REG_RX_03F)));
        }
    }
// -------------------------------------------------------------------------------------------------------
// #endif
// 2014-0102 bug xxxxx
}

// ---------------------------------------------------------------------------------------------------
void
IT6602VideoCountClr (
    void)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();
    IT6602data->m_VideoCountingTimer = 1;
}
    #endif
// FIX_ID_001 xxxxx
// ---------------------------------------------------------------------------------------------------

unsigned char
CheckAVMute (
    void)
{
    unsigned char   ucAVMute;
    unsigned char   ucPortSel;

    ucAVMute    = hdmirxrd(REG_RX_0A8) & (B_P0_AVMUTE | B_P1_AVMUTE);
    ucPortSel   = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    if (((ucAVMute & B_P0_AVMUTE) && (ucPortSel == F_PORT_SEL_0 )) ||
    ((ucAVMute & B_P1_AVMUTE) && (ucPortSel == F_PORT_SEL_1 )))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

unsigned char
CheckPlg5VPwr (
    unsigned char ucPortSel)
{
    unsigned char   sys_state_P0;
    unsigned char   sys_state_P1;

    if (ucPortSel == 0)
    {
        sys_state_P0 = hdmirxrd(REG_RX_P0_SYS_STATUS) & B_P0_PWR5V_DET;
        if ((sys_state_P0 & B_P0_PWR5V_DET))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        sys_state_P1 = hdmirxrd(REG_RX_P1_SYS_STATUS) & B_P1_PWR5V_DET;
        if ((sys_state_P1 & B_P1_PWR5V_DET))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}
// FIX_ID_037 xxxxx

// ---------------------------------------------------------------------------
unsigned char
IsHDMIMode (
    void)
{
    unsigned char   sys_state_P0;
    unsigned char   sys_state_P1;
    unsigned char   ucPortSel;

    sys_state_P0    = hdmirxrd(REG_RX_P0_SYS_STATUS) & B_P0_HDMI_MODE;
    sys_state_P1    = hdmirxrd(REG_RX_P1_SYS_STATUS) & B_P1_HDMI_MODE;
    ucPortSel       = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    if (((sys_state_P0 & B_P0_HDMI_MODE) && (ucPortSel == F_PORT_SEL_0 )) ||
    ((sys_state_P1 & B_P1_HDMI_MODE) && (ucPortSel == F_PORT_SEL_1 )))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// ---------------------------------------------------------------------------
unsigned char
IsVideoOn (
    void)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    if (IT6602data->m_VState == VSTATE_VideoOn)

    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// ---------------------------------------------------------------------------
void
GetAVIInfoFrame (
    struct IT6602_dev_data * IT6602)
{
    chgbank(2);
    IT6602->ColorMode               = ((hdmirxrd(REG_RX_AVI_DB1) & 0x60) >> 5);
    IT6602->Colorimetry             = ((hdmirxrd(REG_RX_AVI_DB2) & 0xC0) >> 6);
    IT6602->ExtendedColorimetry     = ((hdmirxrd(REG_RX_AVI_DB3) & 0x70) >> 4);
    IT6602->RGBQuantizationRange    = ((hdmirxrd(REG_RX_AVI_DB3) & 0x0C) >> 2);
    IT6602->VIC                     = ((hdmirxrd(REG_RX_AVI_DB4) & 0x7F));
    IT6602->YCCQuantizationRange    = ((hdmirxrd(REG_RX_AVI_DB5) & 0xC0) >> 6);
    chgbank(0);

// FIX_ID_027 xxxxx Support RGB limited / Full range convert
    if (IT6602->RGBQuantizationRange == 0)
    {
        if (IT6602->VIC >= 2)
        {
            // CE Mode
            IT6602->RGBQuantizationRange = 1;   // limited range
        }
        else
        {
            // IT mode
            IT6602->RGBQuantizationRange = 2;   // Full range
        }
    }
// FIX_ID_027    xxxxx

    VIDEOTIMNG_DEBUG_PRINTF(("AVI ColorMode = %X\n", (int) IT6602->ColorMode));
    VIDEOTIMNG_DEBUG_PRINTF(("AVI Colorimetry = %X\n", (int) IT6602->Colorimetry));
    VIDEOTIMNG_DEBUG_PRINTF(("AVI ExtendedColorimetry = %X\n", (int) IT6602->ExtendedColorimetry));
    VIDEOTIMNG_DEBUG_PRINTF(("AVI RGBQuantizationRange = %X\n", (int) IT6602->RGBQuantizationRange));
    VIDEOTIMNG_DEBUG_PRINTF(("AVI VIC = %X\n", (int) IT6602->VIC));
    VIDEOTIMNG_DEBUG_PRINTF(("AVI YCCQuantizationRange = %X\n", (int) IT6602->YCCQuantizationRange));
}

// ---------------------------------------------------------------------------
void
SetVideoInputFormatWithInfoFrame (
    struct IT6602_dev_data * IT6602)
{
    unsigned char   uc;
    unsigned char   bAVIColorModeIndicated = FALSE;
//  unsigned char bOldInputVideoMode=IT6602->m_bInputVideoMode;

    chgbank(2);
    uc = hdmirxrd(REG_RX_215);    // REG_RX_AVI_DB1
    chgbank(0);
    // FIX_ID_059
    IT6602->prevAVIDB[1]        = uc;
    // FIX_ID_059
    IT6602->m_bInputVideoMode   &= ~F_MODE_CLRMOD_MASK;

    switch ((uc >> O_AVI_COLOR_MODE) & M_AVI_COLOR_MASK)
    {
        case B_AVI_COLOR_YUV444:
            HDMIRX_VIDEO_PRINTF(("input YUV444 mode "));
            IT6602->m_bInputVideoMode |= F_MODE_YUV444;
            break;

        case B_AVI_COLOR_YUV422:
            HDMIRX_VIDEO_PRINTF(("input YUV422 mode "));
            IT6602->m_bInputVideoMode |= F_MODE_YUV422;
            break;

        case B_AVI_COLOR_RGB24:
            HDMIRX_VIDEO_PRINTF(("input RGB24 mode "));
            IT6602->m_bInputVideoMode |= F_MODE_RGB24;
            break;

// FIX_ID_044  add YCbCr420 supporting
        case B_AVI_COLOR_YUV420:
            HDMIRX_VIDEO_PRINTF(("input YUV420 mode "));
            IT6602->m_bInputVideoMode |= F_MODE_YUV420;
            break;

// ~FIX_ID_044
        default:
            return;
    }

    #if 1    // InfoFrame BUG
    chgbank(2);
//  uc=hdmirxrd(REG_RX_218);
//  if(uc>0) IT6602->m_bInputVideoMode |= F_MODE_16_235;
//  else IT6602->m_bInputVideoMode &= ~ F_MODE_16_235;
    uc = (hdmirxrd(REG_RX_217) & 0x0F) >> 2;
    if (uc == 0x01)
    {
        IT6602->m_bInputVideoMode |= F_MODE_16_235;
    }
    else
    {
        IT6602->m_bInputVideoMode &= ~F_MODE_16_235;
    }
    chgbank(0);
    #endif

    HDMIRX_DEBUG_PRINT(("SetVideoInputFormatWithInfoFrame - RegAE=%X IT6602->m_bInputVideoMode=%X\n", (int)uc, (int) IT6602->m_bInputVideoMode));
    uc  = hdmirxrd(REG_RX_IN_CSC_CTRL);
    uc  &= ~B_IN_FORCE_COLOR_MODE;
    hdmirxwr(REG_RX_IN_CSC_CTRL, uc);
}

// ---------------------------------------------------------------------------
void
SetColorimetryByInfoFrame (
    struct IT6602_dev_data * IT6602)
{
    unsigned char i;

//    if(IT6602->m_NewAVIInfoFrameF)
    {
        chgbank(2);
        i   = hdmirxrd(REG_RX_216); // REG_RX_AVI_DB2
        chgbank(0);
        i   &= M_AVI_CLRMET_MASK << O_AVI_CLRMET;

        if (i == (B_AVI_CLRMET_ITU601 << O_AVI_CLRMET))
        {
            IT6602->m_bInputVideoMode &= ~F_MODE_ITU709;
        }
        else if (i == (B_AVI_CLRMET_ITU709 << O_AVI_CLRMET))
        {
            IT6602->m_bInputVideoMode |= F_MODE_ITU709;
        }
    }
}

    #if 0

void
DumpCSCReg (
    void)
{
    ushort  i, j;
    BYTE    ucData;

    (void)printf("\n       ");
    for (j = 0; j < 16; j++)
    {
        (void)printf(" %02X", (int) j);
        if ((j == 3) || (j == 7) || (j == 11))
        {
            (void)printf(" :");
        }
    }
    (void)printf("\n");

    chgbank(1);

    for (i = 0x70; i < 0x90; i += 16)
    {
        (void)printf("[%03X]  ", i);
        for (j = 0; j < 16; j++)
        {
            ucData = hdmirxrd((BYTE)((i + j) & 0xFF));
            (void)printf(" %02X", (int) ucData);
            if ((j == 3) || (j == 7) || (j == 11))
            {
                (void)printf(" :");
            }
        }
        (void)printf("\n");
    }

    (void)printf("\n        =====================================================\n");

    chgbank(0);
    for (i = 0x60; i < 0x70; i += 16)
    {
        (void)printf("[%03X]  ", i);
        for (j = 0; j < 16; j++)
        {
            ucData = hdmirxrd((BYTE)((i + j) & 0xFF));
            (void)printf(" %02X", (int) ucData);
            if ((j == 3) || (j == 7) || (j == 11))
            {
                (void)printf(" :");
            }
        }
        (void)printf("\n");
    }
}

void
IT6602_UpdateCSC (
    unsigned char   * cAVIInfoFrame,
    unsigned char   * cHDMI_DVI_mode,
    unsigned char   * cHDMI_Input_CSC,
    unsigned char   * cRGBQuantData)
{
    unsigned char   ucInputYUV;
    unsigned char   ucITU709;
    unsigned char   ucInputFullRange;
    // unsigned char ucOutputYUV;

    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    GetAVIInfoFrame(IT6602data);

    *cHDMI_Input_CSC = IT6602data->ColorMode;

    if ((IT6602data->ColorMode == 1) || (IT6602data->ColorMode == 2))
    {
        ucInputYUV = 1;

        if ((IT6602data->YCCQuantizationRange == 1))
        {
            ucInputFullRange = 1;
        }
        else
        {
            ucInputFullRange = 0;
        }

        if ((IT6602data->Colorimetry == 2))
        {
            ucITU709            = 1;
            *cHDMI_Input_CSC    = 2;    // Receiver_YUV_709;
        }
        else
        {
            ucITU709            = 0;
            *cHDMI_Input_CSC    = 1;    // Receiver_YUV_601;
        }
    }
    else
    {
        ucInputYUV          = 0;
        *cHDMI_Input_CSC    = 0;        // Receiver_RGB;

        if ((IT6602data->RGBQuantizationRange == 2))
        {
            ucInputFullRange = 1;
        }
        else
        {
            ucInputFullRange = 0;
        }
    }

    if (ucInputFullRange == 1)
    {
        *cRGBQuantData = 2;
    }
    else
    {
        *cRGBQuantData = 1;
    }

    if (IsHDMIMode() == TRUE)
    {
        *cAVIInfoFrame  = TRUE;
        *cHDMI_DVI_mode = TRUE;
    }
    else
    {
        *cAVIInfoFrame  = FALSE;
        *cHDMI_DVI_mode = FALSE;
    }
}

void
SetCSCbyOSD (
    unsigned char   ucInputYUV,
    unsigned char   ucITU709,
    unsigned char   ucFullRange,
    unsigned char   ucOutputYUV)
{
    // struct IT6602_dev_data *IT6602data = get_IT6602_dev_data();

    if (ucOutputYUV == 0)
    {
        // Convert input RGB / YUV To output RGB444
        // 0. RGB / YUV
        // 1. ITU601 / ITU709
        // 2. Data Range ( Limit / Full )
        pCSCLookup          = CSCOutputRGB[ucInputYUV][ucITU709][ucFullRange];
        pCSCLookupConfig    = &IT6602_OUTPUT_RGB_CONFIG_REG[ucInputYUV][ucFullRange];
    }
    else
    {
        // Convert input RGB / YUV To output YUV444
        // 0. RGB / YUV
        // 1. ITU601 / ITU709
        // 2. Data Range ( Limit / Full )
        pCSCLookup          = CSCOutputYUV[ucInputYUV][ucITU709][ucFullRange];
        pCSCLookupConfig    = &IT6602_OUTPUT_YUV_CONFIG_REG[ucInputYUV][ucFullRange];
    }

    chgbank(1);
    hdmirxbwr(REG_RX_170, 21, pCSCLookup);
    chgbank(0);
    hdmirxset(REG_RX_065, 0x33, pCSCLookupConfig->ucReg65);
    hdmirxwr(REG_RX_067, pCSCLookupConfig->ucReg67);
    hdmirxwr(REG_RX_068, pCSCLookupConfig->ucReg68);

    if (ucOutputYUV == 1)
    {
        if (ucInputYUV == 0)
        {
            hdmirxset(REG_RX_065, 0x30, 0x10);      // Input RGB Outptu YUV422 with CSC
        }
        else
        {
            hdmirxset(REG_RX_065, 0x33, 0x10);      // Input YUV Outptu YUV422 with bypass CSC
        }
    }

    DumpCSCReg();
}
// xxxxx
    #endif

// ---------------------------------------------------------------------------
void
SetColorSpaceConvert (
    struct IT6602_dev_data * IT6602)
{
    unsigned char   csc;
    //    unsigned char uc;
    unsigned char   cscpwd;
    unsigned char   filter = 0; // filter is for Video CTRL DN_FREE_GO, EN_DITHER, and ENUDFILT

    #ifdef DISABLE_HDMI_CSC
    HDMIRX_VIDEO_PRINTF(("it660x - HDMI Color Space Convert is disabled\n"));

    csc                         = B_CSC_BYPASS;
    IT6602->m_bOutputVideoMode  = IT6602->m_bInputVideoMode;
    cscpwd                      = B_PWD_CSC;
    #else
    HDMIRX_VIDEO_PRINTF(("\n!!! SetColorSpaceConvert( ) !!!\n"));
    cscpwd = 0;

// FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
// xxxxx FIX_ID_039 disable --> //FIX_ID_027 xxxxx Support Full/Limited Range convert
// xxxxx FIX_ID_039 disable -->     //default to turn off CSC offset
// xxxxx FIX_ID_039 disable -->     hdmirxset(REG_RX_067,0x78,0x00);
// xxxxx FIX_ID_039 disable -->     hdmirxwr(REG_RX_068,0x00);
// xxxxx FIX_ID_039 disable --> //FIX_ID_027 xxxxx
// FIX_ID_039 xxxxx

// FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
        #ifdef _AVOID_REDUNDANCE_CSC_
    if ((IT6602->m_Backup_OutputVideoMode == IT6602->m_bOutputVideoMode) && (IT6602->m_Backup_InputVideoMode == IT6602->m_bInputVideoMode))
    {
        HDMIRX_VIDEO_PRINTF(("I/P and O/P color without change , No need to setup CSC convert again\n"));
        return;
    }
        #endif
// FIX_ID_039 xxxxx

    // HDMIRX_VIDEO_PRINTF(("Input mode is YUV444 "));
    switch (IT6602->m_bOutputVideoMode & F_MODE_CLRMOD_MASK)
    {
        #if defined(SUPPORT_OUTPUTYUV444)
        case F_MODE_YUV444:
            HDMIRX_VIDEO_PRINTF(("Output mode is YUV444\n"));
            switch (IT6602->m_bInputVideoMode & F_MODE_CLRMOD_MASK)
            {
            // FIX_ID_044        // Add YUV420 support
                case F_MODE_YUV420:
                    HDMIRX_VIDEO_PRINTF(("Input mode is YUV420\n"));
                    csc     = B_CSC_BYPASS;
                    cscpwd  = B_PWD_CSC;

                    break;

                case F_MODE_YUV444:
                    HDMIRX_VIDEO_PRINTF(("Input mode is YUV444\n"));
                    csc     = B_CSC_BYPASS;
                    cscpwd  = B_PWD_CSC;

                    break;

                case F_MODE_YUV422:
                    HDMIRX_VIDEO_PRINTF(("Input mode is YUV422\n"));
                    csc = B_CSC_BYPASS;
                    if (IT6602->m_bOutputVideoMode & F_MODE_EN_UDFILT)  // RGB24 to YUV422 need up/dn filter.
                    {
                        filter |= B_RX_EN_UDFILTER;
                    }

                    if (IT6602->m_bOutputVideoMode & F_MODE_EN_DITHER)  // RGB24 to YUV422 need up/dn filter.
                    {
                        filter |= B_RX_EN_UDFILTER | B_RX_DNFREE_GO;
                    }

                    break;

                case F_MODE_RGB24:
                    HDMIRX_VIDEO_PRINTF(("Input mode is RGB444\n"));
                    csc = B_CSC_RGB2YUV;
                    break;
            }
            break;
        #endif

        #if defined(SUPPORT_OUTPUTYUV422)

        case F_MODE_YUV422:
            switch (IT6602->m_bInputVideoMode & F_MODE_CLRMOD_MASK)
            {
                case F_MODE_YUV444:
                    HDMIRX_VIDEO_PRINTF(("Input mode is YUV444\n"));
                    if (IT6602->m_bOutputVideoMode & F_MODE_EN_UDFILT)
                    {
                        filter |= B_RX_EN_UDFILTER;
                    }
                    csc = B_CSC_BYPASS;
                    break;

                case F_MODE_YUV422:
                    HDMIRX_VIDEO_PRINTF(("Input mode is YUV422\n"));
                    csc = B_CSC_BYPASS;

                    // if output is YUV422 and 16 bit or 565, then the dither is possible when
                    // the input is YUV422 with 24bit input, however, the dither should be selected
                    // by customer, thus the requirement should set in ROM, no need to check
                    // the register value .
                    if (IT6602->m_bOutputVideoMode & F_MODE_EN_DITHER)// RGB24 to YUV422 need up/dn filter.
                    {
                        filter |= B_RX_EN_UDFILTER | B_RX_DNFREE_GO;
                    }
                    break;

                case F_MODE_RGB24:
                    HDMIRX_VIDEO_PRINTF(("Input mode is RGB444\n"));
                    if (IT6602->m_bOutputVideoMode & F_MODE_EN_UDFILT)// RGB24 to YUV422 need up/dn filter.
                    {
                        filter |= B_RX_EN_UDFILTER;
                    }
                    csc = B_CSC_RGB2YUV;
                    break;
            }
            break;
        #endif

        #if defined(SUPPORT_OUTPUTRGB)
        case F_MODE_RGB24:
            HDMIRX_VIDEO_PRINTF(("Output mode is RGB24\n"));
            switch (IT6602->m_bInputVideoMode & F_MODE_CLRMOD_MASK)
            {
                case F_MODE_YUV444:
                    HDMIRX_VIDEO_PRINTF(("Input mode is YUV444\n"));
                    csc = B_CSC_YUV2RGB;
                    break;

                case F_MODE_YUV422:
                    HDMIRX_VIDEO_PRINTF(("Input mode is YUV422\n"));
                    csc = B_CSC_YUV2RGB;
                    if (IT6602->m_bOutputVideoMode & F_MODE_EN_UDFILT)  // RGB24 to YUV422 need up/dn filter.
                    {
                        filter |= B_RX_EN_UDFILTER;
                    }
                    if (IT6602->m_bOutputVideoMode & F_MODE_EN_DITHER)  // RGB24 to YUV422 need up/dn filter.
                    {
                        filter |= B_RX_EN_UDFILTER | B_RX_DNFREE_GO;
                    }
                    break;

                case F_MODE_RGB24:
                    HDMIRX_VIDEO_PRINTF(("Input mode is RGB444\n"));
                    csc     = B_CSC_BYPASS;
                    cscpwd  = B_PWD_CSC;

                    break;
            }
            break;
        #endif
    }

        #if defined(SUPPORT_OUTPUTYUV)
    // set the CSC associated registers
    if (csc == B_CSC_RGB2YUV)
    {
        // HDMIRX_VIDEO_PRINTF(("CSC = RGB2YUV "));
        // FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
        // default to turn off CSC offset
        hdmirxset(REG_RX_067, 0x78, 0x00);
        hdmirxwr(REG_RX_068, 0x00);
        HDMIRX_VIDEO_PRINTF((" Clear Reg67 and Reg68 ...\n"));
        // FIX_ID_039 xxxxx
        if (IT6602->m_bInputVideoMode & F_MODE_ITU709)
        {
            HDMIRX_VIDEO_PRINTF(("ITU709 "));

            if (IT6602->m_bInputVideoMode & F_MODE_16_235)
            {
                HDMIRX_VIDEO_PRINTF((" 16-235\n"));
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_RGB2YUV_ITU709_16_235), &bCSCMtx_RGB2YUV_ITU709_16_235[0]);
            }
            else
            {
                HDMIRX_VIDEO_PRINTF((" 0-255\n"));
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_RGB2YUV_ITU709_0_255), &bCSCMtx_RGB2YUV_ITU709_0_255[0]);
            }
        }
        else
        {
            HDMIRX_VIDEO_PRINTF(("ITU601 "));
            if (IT6602->m_bInputVideoMode & F_MODE_16_235)
            {
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_RGB2YUV_ITU601_16_235), &bCSCMtx_RGB2YUV_ITU601_16_235[0]);
                HDMIRX_VIDEO_PRINTF((" 16-235\n"));
            }
            else
            {
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_RGB2YUV_ITU601_0_255), &bCSCMtx_RGB2YUV_ITU601_0_255[0]);
                HDMIRX_VIDEO_PRINTF((" 0-255\n"));
            }
        }
    }
        #endif
        #if defined(SUPPORT_OUTPUTRGB)
    if (csc == B_CSC_YUV2RGB)
    {
        HDMIRX_VIDEO_PRINTF(("CSC = YUV2RGB "));
        // FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
        // default to turn off CSC offset
        hdmirxset(REG_RX_067, 0x78, 0x00);
        hdmirxwr(REG_RX_068, 0x00);
        HDMIRX_VIDEO_PRINTF((" Clear Reg67 and Reg68 ...\n"));
        // FIX_ID_039 xxxxx
        if (IT6602->m_bInputVideoMode & F_MODE_ITU709)
        {
            HDMIRX_VIDEO_PRINTF(("ITU709 "));
            if (IT6602->m_bOutputVideoMode & F_MODE_16_235)
            {
                HDMIRX_VIDEO_PRINTF(("16-235\n"));
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_YUV2RGB_ITU709_16_235), &bCSCMtx_YUV2RGB_ITU709_16_235[0]);
            }
            else
            {
                HDMIRX_VIDEO_PRINTF(("0-255\n"));
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_YUV2RGB_ITU709_0_255), &bCSCMtx_YUV2RGB_ITU709_0_255[0]);
            }
        }
        else
        {
            HDMIRX_VIDEO_PRINTF(("ITU601 "));
            if (IT6602->m_bOutputVideoMode & F_MODE_16_235)
            {
                HDMIRX_VIDEO_PRINTF(("16-235\n"));
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_YUV2RGB_ITU601_16_235), &bCSCMtx_YUV2RGB_ITU601_16_235[0]);
            }
            else
            {
                HDMIRX_VIDEO_PRINTF(("0-255\n"));
                chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_YUV2RGB_ITU601_0_255), &bCSCMtx_YUV2RGB_ITU601_0_255[0]);
            }
        }
    }

// FIX_ID_027 xxxxx Support Full/Limited Range convert
    if (csc == B_CSC_BYPASS)
    {
        if ((IT6602->m_bInputVideoMode & F_MODE_CLRMOD_MASK) == F_MODE_RGB24)
        {
            if (IT6602->RGBQuantizationRange == 1)                                  // Limited range from HDMI source
            {
                if ((IT6602->m_bOutputVideoMode & F_MODE_16_235) != F_MODE_16_235)  // Full range to back-end device
                {
                    // RedText;
                    VIDEOTIMNG_DEBUG_PRINTF((" bCSCMtx_RGB_16_235_RGB_0_255\n"));
                    // (void)printf("pccmd w 65 02 90;\n");
                    // (void)printf("pccmd w 67 78 90;\n");
                    // (void)printf("pccmd w 68 ED 90;\n");
                    // WhileText;
                    csc = B_CSC_RGB2YUV;
                    chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                    hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_RGB_16_235_RGB_0_255), &bCSCMtx_RGB_16_235_RGB_0_255[0]);
                    chgbank(0);
                    // hdmirxset(REG_RX_065,0x03,0x02); // B_CSC_RGB2YUV
                    hdmirxset(REG_RX_067, 0x78, 0x78);
                    hdmirxwr(REG_RX_068, 0xED);
                    cscpwd = 0;
                }
            }
            else if (IT6602->RGBQuantizationRange == 2)                             // Full range from HDMI source
            {
                if ((IT6602->m_bOutputVideoMode & F_MODE_16_235) == F_MODE_16_235)  // Limited range to back-end device
                {
                    // RedText;
                    VIDEOTIMNG_DEBUG_PRINTF((" bCSCMtx_RGB_0_255_RGB_16_235\n"));
                    // (void)printf("pccmd w 65 02 90;\n");
                    // (void)printf("pccmd w 67 40 90;\n");
                    // (void)printf("pccmd w 68 10 90;\n");
                    // WhileText;
                    csc = B_CSC_RGB2YUV;
                    chgbank(1); // for CSC setting Reg170 ~ Reg184 !!!!
                    hdmirxbwr(REG_RX_170, sizeof(bCSCMtx_RGB_0_255_RGB_16_235), &bCSCMtx_RGB_0_255_RGB_16_235[0]);
                    chgbank(0);
                    // hdmirxset(REG_RX_065,0x03,0x02); // B_CSC_RGB2YUV
                    hdmirxset(REG_RX_067, 0x78, 0x40);
                    hdmirxwr(REG_RX_068, 0x10);
                    cscpwd = 0;
                }
            }
        }
    }
// FIX_ID_027 xxxxx

        #endif // SUPPORT_OUTPUTRGB

    #endif  //end of DISABLE_HDMI_CSC

    chgbank(0);
    hdmirxset(REG_RX_OUT_CSC_CTRL, (M_CSC_SEL_MASK), csc);

    // set output Up/Down Filter, Dither control
    hdmirxset(REG_RX_VIDEO_CTRL1, (B_RX_DNFREE_GO | B_RX_EN_DITHER | B_RX_EN_UDFILTER), filter);

// FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
    if (csc == B_CSC_BYPASS)
    {
        // default to turn off CSC offset
        hdmirxset(REG_RX_067, 0x78, 0x00);
        hdmirxwr(REG_RX_068, 0x00);
    }

    // FIX_ID_052 separate the DCLK delay under CSC power down and on
    hdmirxset(REG_RX_051, B_PWD_CSC, cscpwd);
    if (cscpwd & B_PWD_CSC)
    {
        hdmirxset(REG_RX_050, 0x03, DCLK_DELAY_CSC_OFF);
    }
    else
    {
        hdmirxset(REG_RX_050, 0x03, DCLK_DELAY_CSC_ON);
    }
    // ~FIX_ID_052

    #ifdef _AVOID_REDUNDANCE_CSC_
    IT6602->m_Backup_OutputVideoMode    = IT6602->m_bOutputVideoMode;
    IT6602->m_Backup_InputVideoMode     = IT6602->m_bInputVideoMode;
    #endif
// FIX_ID_039 xxxxx
}

// ---------------------------------------------------------------------------
void
SetNewInfoVideoOutput (
    struct IT6602_dev_data * IT6602)
{
    HDMIRX_VIDEO_PRINTF(("SetNewInfoVideoOutput()\n"));

    SetVideoInputFormatWithInfoFrame(IT6602);
    SetColorimetryByInfoFrame(IT6602);
    SetColorSpaceConvert(IT6602);

    SetVideoOutputColorFormat(IT6602);  // 2013-0502

//  get_vid_info();
//  show_vid_info();
}

// ---------------------------------------------------------------------------
void
SetVideoInputFormatWithoutInfoFrame (
    struct IT6602_dev_data  * IT6602,
    unsigned char           bInMode)
{
    unsigned char i;

    i                           = hdmirxrd(REG_RX_IN_CSC_CTRL);
    i                           |= B_IN_FORCE_COLOR_MODE;

    i                           &= (~M_INPUT_COLOR_MASK);
    IT6602->m_bInputVideoMode   &= ~F_MODE_CLRMOD_MASK;
    #if 1    // InfoFrame BUG
    IT6602->m_bInputVideoMode   &= ~F_MODE_16_235;
    #endif
    switch (bInMode)
    {
        case F_MODE_YUV444:
            i                           |= B_INPUT_YUV444;
            IT6602->m_bInputVideoMode   |= F_MODE_YUV444;
            break;

        case F_MODE_YUV422:
            i                           |= B_INPUT_YUV422;
            IT6602->m_bInputVideoMode   |= F_MODE_YUV422;
            break;

        case F_MODE_RGB24:
            i                           |= B_INPUT_RGB24;
            IT6602->m_bInputVideoMode   |= F_MODE_RGB24;
            break;

        default:
            return;
    }
    hdmirxwr(REG_RX_IN_CSC_CTRL, i);

    // FIX_ID_059
    // reset the prevAVIDB while DVI mode.
    IT6602->prevAVIDB[0]    = 0xFF;
    IT6602->prevAVIDB[1]    = 0xFF;
    // FIX_ID_059
}

// ---------------------------------------------------------------------------
void
SetColorimetryByMode (
    struct IT6602_dev_data * IT6602)
{
    unsigned char RxClkXCNT;
    RxClkXCNT = hdmirxrd(REG_RX_PIXCLK_SPEED);

    HDMIRX_VIDEO_PRINTF((" SetColorimetryByMode() REG_RX_PIXCLK_SPEED=%X\n", (int) RxClkXCNT));

    IT6602->m_bInputVideoMode &= ~F_MODE_ITU709;

    if (RxClkXCNT < 0x34)
    {
        IT6602->m_bInputVideoMode |= F_MODE_ITU709;
    }
    else
    {
        IT6602->m_bInputVideoMode &= ~F_MODE_ITU709;
    }
}

// ---------------------------------------------------------------------------
void
SetDVIVideoOutput (
    struct IT6602_dev_data * IT6602)
{
    HDMIRX_VIDEO_PRINTF(("SetDVIVideoOutput()\n"));

    SetVideoInputFormatWithoutInfoFrame(IT6602, F_MODE_RGB24);
    SetColorimetryByMode(IT6602);
    SetColorSpaceConvert(IT6602);

    SetVideoOutputColorFormat(IT6602);  // 2013-0502
}

// FIX_ID_003 xxxxx //Add IT6602 Video Output Configure setting
void
IT6602_VideoOutputModeSet (
    struct IT6602_dev_data * IT6602)
{
    unsigned char   ucReg51;
    unsigned char   ucReg65;

    HDMIRX_VIDEO_PRINTF(("IT6602_VideoOutputModeSet()\n"));

    HDMIRX_VIDEO_PRINTF((VModeStateStr[(unsigned char)IT6602->m_VidOutConfigMode]));
    HDMIRX_VIDEO_PRINTF(("+++ \n"));

    ucReg51 = hdmirxrd(REG_RX_051) & 0x9B;      // Reg51 [6] Half PCLK DDR , [5] Half Bus DDR , [2] CCIR656 mode
    ucReg65 = hdmirxrd(REG_RX_065) & 0x0F;      // Reg65 [7] BTA1004Fmt , [6] SyncEmb , [5:4] output color 0x00 RGB, 0x10 YUV422, 0x20 YUV444

    switch ((IT6602->m_bOutputVideoMode & F_MODE_CLRMOD_MASK))
    {
        case F_MODE_RGB444:
            ucReg65 |= (B_OUTPUT_RGB24);        // 0x00 B_OUTPUT_RGB24
    #ifdef _SUPPORT_RBSWAP_
            hdmirxset(REG_RX_064, 0x18, 0x08);
    #endif
            break;

        case F_MODE_YUV422:
            ucReg65 |= (B_OUTPUT_YUV422);     // 0x10 B_OUTPUT_YUV422
    #ifdef _SUPPORT_RBSWAP_
            hdmirxset(REG_RX_064, 0x18, 0x10);
    #endif
            break;

        case F_MODE_YUV444:
            ucReg65 |= (B_OUTPUT_YUV444);     // 0x20 B_OUTPUT_YUV444
    #ifdef _SUPPORT_RBSWAP_
            hdmirxset(REG_RX_064, 0x18, 0x10);
    #endif
            break;
    }
    // SPECIAL CASE      ITE_150409
    if (( IT6602->m_VidOutDataTrgger == eHalfPCLKDDR) &&
        (IT6602->m_VidOutSyncMode != eCCIR656EmbSync) &&
        (IT6602->m_VidOutSyncMode != eCCIR656SepSync))
    {
        IT6602_HalfPCLKFinetune(IT6602);
    }

    if ((IT6602->m_VidOutSyncMode == eCCIR656EmbSync) ||
       (IT6602->m_VidOutSyncMode == eCCIR656SepSync))
    {
        hdmirxset(REG_RX_064, 0x80, 0x80);
        hdmirxset(REG_RX_064, 0x80, 0x00);
    }
    switch (IT6602->m_VidOutDataTrgger)
    {
        case eSDR:
            break;

        case eHalfPCLKDDR:
            ucReg51 |= (B_HALF_PCLKC);              // 0x40 half PCLK
            break;

        case eHalfBusDDR:
            ucReg51 |= (B_OUT_DDR);                 // 0x20 half bus
            break;

        case eSDR_BTA1004:
            ucReg65 |= (B_BTA1004Fmt | B_SyncEmb);  // 0x80 BTA1004 + 0x40 SyncEmb
            break;

        case eDDR_BTA1004:
            ucReg51 |= (B_HALF_PCLKC);              // 0x40 half PCLK
            ucReg65 |= (B_BTA1004Fmt | B_SyncEmb);  // 0x80 BTA1004 + 0x40 SyncEmb
            break;
    }

    switch (IT6602->m_VidOutSyncMode)
    {
        case eSepSync:
            break;

        case eEmbSync:
            ucReg65 |= (B_SyncEmb);     // 0x40 SyncEmb
            break;

        case eCCIR656SepSync:
            ucReg51 |= (B_CCIR656);     // 0x04 CCIR656
            break;

        case eCCIR656EmbSync:
            ucReg51 |= (B_CCIR656);     // 0x04 CCIR656
            ucReg65 |= (B_SyncEmb);     // 0x40 SyncEmb
            break;
    }

    HDMIRX_VIDEO_PRINTF(("Reg51 = %X ", (int) ucReg51));
    HDMIRX_VIDEO_PRINTF(("Reg65 = %X\n", (int) ucReg65));

    hdmirxwr(REG_RX_051, ucReg51);
    hdmirxwr(REG_RX_065, ucReg65);
}

// FIX_ID_003 xxxxx
void
IT6602_HalfPCLKFinetune (
    struct IT6602_dev_data * IT6602)                         // ITE_150409
{
    unsigned char   rddata, OutInMode;
    int             PCLK;
    rddata      = hdmirxrd(0x9A);
    PCLK        = (int)(124 * 255 / rddata) / 10;
    OutInMode   = ((IT6602->m_bOutputVideoMode & F_MODE_CLRMOD_MASK) << 4) | (IT6602->m_bInputVideoMode & F_MODE_CLRMOD_MASK);
    if (PCLK < 250)          // Clive
    {
        HDMIRX_VIDEO_PRINTF(("IT6602JudgeOutputMode( ) PCLK Low =================== %d\n", (int) PCLK));
        HDMIRX_VIDEO_PRINTF(("IT6602JudgeOutputMode( ) OutInMode = 0x%x\n", (int) OutInMode));
        HDMIRX_VIDEO_PRINTF(("IT6602JudgeOutputMode( ) REG_RX_050 = 0x%x\n", (int) hdmirxrd(0x50)));
        // RGB24     0
        // YUV422   1
        // YUV444   2
        // YUV420   3
        switch (OutInMode)
        {
            case 0x22:  // Out + In =>  YUV444 + YUV444
            case 0x23:  // Out + In =>  YUV444 + YUV420
            case 0x20:  // Out + In =>  YUV444 + RGB24
            case 0x13:  // Out + In =>  YUV422 + YUV420
            case 0x11:  // Out + In =>  YUV422 + YUV422
            case 0x03:  // Out + In =>  RGB444 + YUV420
            case 0x02:  // Out + In =>  RGB444 + YUV444
            case 0x00:  // Out + In =>  RGB444 + RGB24
                hdmirxset(REG_RX_050, 0x0C, 0x00);
                hdmirxset(REG_RX_064, 0x80, 0x80);
                hdmirxset(REG_RX_064, 0x80, 0x00);
                break;

            case 0x21:  // Out + In =>  YUV444 + YUV422
            case 0x12:  // Out + In =>  YUV422 + YUV444
            case 0x10:  // Out + In =>  YUV422 + RGB24
            case 0x01:  // Out + In =>  RGB444 + YUV422
                hdmirxset(REG_RX_050, 0x1C, 0x00);
                hdmirxset(REG_RX_064, 0x80, 0x80);
                hdmirxset(REG_RX_064, 0x80, 0x00);
                break;
        }
    }
    else       // PCLK > 250  run high speed mode.
    {
        HDMIRX_VIDEO_PRINTF(("IT6602JudgeOutputMode( ) PCLK High =================== %d\n", (int) PCLK));
        HDMIRX_VIDEO_PRINTF(("IT6602JudgeOutputMode( ) OutInMode = 0x%x\n", (int) OutInMode));
        HDMIRX_VIDEO_PRINTF(("IT6602JudgeOutputMode( ) REG_RX_050 = 0x%x\n", (int) hdmirxrd(0x50)));
        switch (OutInMode)
        {
            case 0x22:  // Out + In =>  YUV444 + YUV444
            case 0x23:  // Out + In =>  YUV444 + YUV420
            case 0x20:  // Out + In =>  YUV444 + RGB24
            case 0x13:  // Out + In =>  YUV422 + YUV420
            case 0x11:  // Out + In =>  YUV422 + YUV422
            case 0x03:  // Out + In =>  RGB444 + YUV420
            case 0x02:  // Out + In =>  RGB444 + YUV444
            case 0x00:  // Out + In =>  RGB444 + RGB24
                hdmirxset(REG_RX_050, 0x0C, 0x08);
                hdmirxset(REG_RX_064, 0x80, 0x80);
                hdmirxset(REG_RX_064, 0x80, 0x00);
                break;

            case 0x21:  // Out + In =>  YUV444 + YUV422
            case 0x12:  // Out + In =>  YUV422 + YUV444
            case 0x10:  // Out + In =>  YUV422 + RGB24
            case 0x01:  // Out + In =>  RGB444 + YUV422
                hdmirxset(REG_RX_050, 0x1C, 0x08);
                hdmirxset(REG_RX_064, 0x80, 0x80);
                hdmirxset(REG_RX_064, 0x80, 0x00);
                break;
        }
    }
    HDMIRX_VIDEO_PRINTF(("IT6602JudgeOutputMode( ) REG_RX_050 = 0x%x\n", (int) hdmirxrd(0x50)));
}

void
IT6602VideoOutputConfigure (
    struct IT6602_dev_data * IT6602)
{
    // Configure Output color space convert

// 06-27 disable -->    #ifndef DISABLE_HDMI_CSC
// 06-27 disable -->    IT6602->m_bOutputVideoMode = HDMIRX_OUTPUT_VID_MODE;
// 06-27 disable -->    #endif

    IT6602->m_bUpHDMIMode = IsHDMIMode();
    if (IT6602->m_bUpHDMIMode == FALSE)
    {
        SetDVIVideoOutput(IT6602);
    }
    else
    {
// FIX_ID_027 xxxxx     //Support RGB limited / Full range convert
        GetAVIInfoFrame(IT6602);
        SetNewInfoVideoOutput(IT6602);
// FIX_ID_027 xxxxx
    }
    IT6602->m_NewAVIInfoFrameF = FALSE;

    // Configure Output Color Depth

    IT6602->GCP_CD = ((hdmirxrd(0x99) & 0xF0) >> 4);
    switch (IT6602->GCP_CD)
    {
        case 5:
            VIDEOTIMNG_DEBUG_PRINTF(( "\n Output ColorDepth = 30 bits per pixel\n"));
            hdmirxset(0x65, 0x0C, 0x04);
            break;

        case 6:
            VIDEOTIMNG_DEBUG_PRINTF(( "\n Output ColorDepth = 36 bits per pixel\n"));
            hdmirxset(0x65, 0x0C, 0x08);
            break;

        default:
            VIDEOTIMNG_DEBUG_PRINTF(( "\n Output ColorDepth = 24 bits per pixel\n"));
            hdmirxset(0x65, 0x0C, 0x00);
            break;
    }

    // Configure TTL Video Output mode
    IT6602_VideoOutputModeSet(IT6602);
}

// ---------------------------------------------------------------------------
void
SetVideoOutputColorFormat (
    struct IT6602_dev_data * IT6602)
{
    switch (IT6602->m_bOutputVideoMode & F_MODE_CLRMOD_MASK)
    {
        case F_MODE_RGB24:
            hdmirxset(REG_RX_OUT_CSC_CTRL, (M_OUTPUT_COLOR_MASK), B_OUTPUT_RGB24);
    #ifdef _SUPPORT_RBSWAP_
            hdmirxset(REG_RX_064, 0x18, 0x08);
    #endif

            break;

        case F_MODE_YUV422:
            hdmirxset(REG_RX_OUT_CSC_CTRL, (M_OUTPUT_COLOR_MASK), B_OUTPUT_YUV422);
    #ifdef _SUPPORT_RBSWAP_
            hdmirxset(REG_RX_064, 0x18, 0x10);
    #endif
            break;

        case F_MODE_YUV444:
            hdmirxset(REG_RX_OUT_CSC_CTRL, (M_OUTPUT_COLOR_MASK), B_OUTPUT_YUV444);
    #ifdef _SUPPORT_RBSWAP_
            hdmirxset(REG_RX_064, 0x18, 0x08);
    #endif
            break;
    }
}

void
IT6602PortSelect (
    unsigned char ucPortSel)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    #if defined(_IT6602_)
    if (ucPortSel == F_PORT_SEL_0)
    {
        hdmirxset(REG_RX_051, B_PORT_SEL, F_PORT_SEL_0);    // select port 0
    }
    else
    {
        hdmirxset(REG_RX_051, B_PORT_SEL, F_PORT_SEL_1);    // select port 1
    }
    #else
    hdmirxset(REG_RX_051, B_PORT_SEL, F_PORT_SEL_0);        // select port 0
    IT6602data->m_ucCurrentHDMIPort = F_PORT_SEL_0;
    #endif

    if (IT6602data->m_ucCurrentHDMIPort != ucPortSel)
    {
        IT6602SwitchVideoState(IT6602data, VSTATE_SyncWait);
        IT6602data->m_ucCurrentHDMIPort = ucPortSel;
        IT6602data->prevAVIDB[0]        = 0xFF;
        IT6602data->prevAVIDB[1]        = 0xFF;
        VIDEOTIMNG_DEBUG_PRINTF(("IT6602PortSelect = %X\n", (int) ucPortSel));
    }
}

void
IT6602HPDCtrl (
    unsigned char   ucport,
    unsigned char   ucEnable)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();
    HDMIRX_VIDEO_PRINTF(("IT6602HPDCtrl(%d,%d);\n", ucport, ucEnable));
    if (ucport == 0)
    {
        if (ucEnable == 0)
        {
            // Disable HDMI DDC Bus to access it660x EDID RAM

            // FIX_ID_048
            // else // HDMI mode.
            {
                if ((hdmirxrd(REG_RX_P0_SYS_STATUS) & B_P0_PWR5V_DET))
                {
                    HDMIRX_VIDEO_PRINTF(("Port 0 HPD HDMI 00000\n"));
                    chgbank(1);
                    hdmirxset(REG_RX_1B0, 0x03, 0x01); // clear port 0 HPD=1 for EDID update
                    chgbank(0);
                }
                else
                {
                    HDMIRX_VIDEO_PRINTF(("Port 0 HPD Tri-State\n"));
                    chgbank(1);
                    hdmirxset(REG_RX_1B0, 0x03, 0x00); // clear port 0 HPD=1 for EDID update
                    chgbank(0);
                }
            } //
            // ~FIX_ID_048
        }
        else
        {
            if ((hdmirxrd(REG_RX_P0_SYS_STATUS) & B_P0_PWR5V_DET))
            {
                // Enable HDMI DDC bus to access it660x EDID RAM

                // FIX_ID_048 // only HDMI mode reply HPD pull high
                {
                    HDMIRX_VIDEO_PRINTF(("Port 0 HPD HDMI 11111\n"));
                    chgbank(1);
                    hdmirxset(REG_RX_1B0, 0x03, 0x03); // set port 0 HPD=1
                    chgbank(0);
                } //
                // ~FIX_ID_048
            } // ((hdmirxrd(REG_RX_P0_SYS_STATUS) & B_P0_PWR5V_DET))
            else
            {
                HDMIRX_VIDEO_PRINTF(("Port 0 HPD but HPD Tristate by 5V.\n"));
                chgbank(1);
                hdmirxset(REG_RX_1B0, 0x03, 0x00); // set port 0 Tri-State
                chgbank(0);
            } // not ((hdmirxrd(REG_RX_P0_SYS_STATUS) & B_P0_PWR5V_DET))
        } // if(ucEnable == 0)
    } // if(ucport == 0)
    #if defined(_IT6602_)
    else
    {
        if (ucEnable)
        {
            // Enable HDMI DDC bus to access it660x EDID RAM

            HDMIRX_VIDEO_PRINTF(("Port 1 HPD 11111\n"));
            gpHPD0 = HPDON;
        }
        else
        {
            // Disable HDMI DDC Bus to access it660x EDID RAM

            HDMIRX_VIDEO_PRINTF(("Port 1 HPD 00000\n"));
            gpHPD0 = HPDOFF;
        }
    }
    #endif
}

void
hdmirx_ECCTimingOut (
    unsigned char ucport)
{
    IT6602_DEBUG_INT_PRINTF(("CDR reset for hdmirx_ECCTimingOut()\n"));

    if (ucport == F_PORT_SEL_0)
    {
        IT6602HPDCtrl(0, 0); // set HPD = 0

        hdmirxset(REG_RX_011, (B_P0_DCLKRST | B_P0_CDRRST | B_P0_HDCPRST | B_P0_SWRST), (B_P0_DCLKRST | B_P0_CDRRST | B_P0_HDCPRST | B_P0_SWRST));
        delay1ms(300);
        hdmirxset(REG_RX_011, (B_P0_DCLKRST | B_P0_CDRRST | B_P0_HDCPRST | B_P0_SWRST), 0x00);

        // set port 0 HPD=1
        IT6602HPDCtrl(0, 1);    //  set HPD = 1
    }
    else
    {
        // set port 1 HPD=0
        IT6602HPDCtrl(1, 0);    // HDMI port , set HPD = 0

        hdmirxset(REG_RX_018, (B_P1_DCLKRST | B_P1_CDRRST | B_P1_HDCPRST | B_P1_SWRST), (B_P1_DCLKRST | B_P1_CDRRST | B_P1_HDCPRST | B_P1_SWRST));
        delay1ms(300);
        hdmirxset(REG_RX_018, (B_P1_DCLKRST | B_P1_CDRRST | B_P1_HDCPRST | B_P1_SWRST), 0x00);

        // set port 1 HPD=1
        IT6602HPDCtrl(1, 1); // HDMI port , set HPD = 1
    }
}

#endif

#ifdef _it660x_

// ***************************************************************************
// Audio function
// ***************************************************************************
void
aud_fiforst (
    void)
{
    unsigned char uc;
// FIX_ID_033 xxxxx for fix audio noise issue
    #ifndef _FIX_ID_028_
// FIX_ID_025 xxxxx     //Adjust H/W Mute time
    hdmirxset(REG_RX_074, 0x0c, 0x0c);      // enable Mute i2s and ws   and s/pdif
    delay1ms(100);
    hdmirxset(REG_RX_074, 0x0c, 0x00);      // disable Mute i2s and ws  and s/pdif
// FIX_ID_025 xxxxx
    #endif
// xxxxx 2014-0421
// FIX_ID_033 xxxxx

    hdmirxset(REG_RX_010, 0x02, 0x02);
    hdmirxset(REG_RX_010, 0x02, 0x00);
    uc = hdmirxrd(REG_RX_07B);
    hdmirxwr(REG_RX_07B, uc);
    hdmirxwr(REG_RX_07B, uc);
    hdmirxwr(REG_RX_07B, uc);
    hdmirxwr(REG_RX_07B, uc);
}

    #ifdef _FIX_ID_028_

// ---------------------------------------------------------------------------
// FIX_ID_028 xxxxx //For Debug Audio error with S2
void
IT6602AudioOutputEnable (
    unsigned char bEnable)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    m_bAudioWaiting = FALSE;

    hdmirxset(REG_RX_HWMuteCtrl, (B_HWMuteClr), (B_HWMuteClr));
    hdmirxset(REG_RX_HWMuteCtrl, (B_HWAudMuteClrMode), (B_HWAudMuteClrMode));
    hdmirxset(REG_RX_HWMuteCtrl, (B_HWMuteClr), (0));
    hdmirxset(REG_RX_HWMuteCtrl, (B_HWAudMuteClrMode), (0));
    aud_fiforst();

    if (bEnable == TRUE)
    {
        hdmirxset(REG_RX_052, (B_TriI2SIO | B_TriSPDIF), 0x00);
        IT6602_SwitchAudioState(IT6602, ASTATE_AudioOn);

        HDMIRX_AUDIO_PRINTF((" === IT6602AudioOutputEnable 11111111111 ====\n"));
    }
    else
    {
        hdmirxset(REG_RX_052, (B_TriI2SIO | B_TriSPDIF), (B_TriI2SIO | B_TriSPDIF));
        IT6602_SwitchAudioState(IT6602, ASTATE_AudioOff);
        HDMIRX_AUDIO_PRINTF((" === IT6602AudioOutputEnable 00000000000 ====\n"));
    }
}
// FIX_ID_028 xxxxx

    #else

// ---------------------------------------------------------------------------
void
IT6602AudioOutputEnable (
    unsigned char bEnable)
{
    if (bEnable == TRUE)
    {
        hdmirxset(REG_RX_052, (B_TriI2SIO | B_TriSPDIF), 0x00);
        #ifdef AUDIO_OFF_PULL_LOW
        hdmirxset(REG_RX_RST_CTRL, B_AUDRST, 0);
        #endif
    }
    else
    {
        #ifdef EnableCalFs
// FIX_ID_023 xxxxx     //Fixed for Audio Channel Status Error with invalid HDMI source
        m_u16TMDSCLK                    = 0;
        m_AudioChannelStatusErrorCount  = 0;
        hdmirxset(REG_RX_074, 0x40, 0x00);    // reg74[6]=0 disable Force FS mode
// FIX_ID_023 xxxxx
        #endif

        #ifndef AUDIO_OFF_PULL_LOW
        hdmirxset(REG_RX_052, (B_TriI2SIO | B_TriSPDIF), (B_TriI2SIO | B_TriSPDIF));
        #else
        hdmirxset(REG_RX_052, (B_TriI2SIO | B_TriSPDIF), (0));
        hdmirxset(REG_RX_RST_CTRL, B_AUDRST, B_AUDRST);
        #endif
    }
}
    #endif
// ---------------------------------------------------------------------------

void
hdmirx_ResetAudio (
    void)
{
    unsigned char uc;
    hdmirxset(REG_RX_RST_CTRL, B_AUDRST, B_AUDRST);
    hdmirxset(REG_RX_RST_CTRL, B_AUDRST, 0x00);
    uc = hdmirxrd(REG_RX_07B);
    hdmirxwr(REG_RX_07B, uc);
    hdmirxwr(REG_RX_07B, uc);
    hdmirxwr(REG_RX_07B, uc);
    hdmirxwr(REG_RX_07B, uc);
}

// ---------------------------------------------------------------------------
void
hdmirx_SetHWMuteClrMode (
    void)
{
    hdmirxset(REG_RX_HWMuteCtrl, (B_HWAudMuteClrMode), (B_HWAudMuteClrMode));
}

// ---------------------------------------------------------------------------
void
hdmirx_SetHWMuteClr (
    void)
{
    hdmirxset(REG_RX_HWMuteCtrl, (B_HWMuteClr), (B_HWMuteClr));
}

// ---------------------------------------------------------------------------
void
hdmirx_ClearHWMuteClr (
    void)
{
    hdmirxset(REG_RX_HWMuteCtrl, (B_HWMuteClr), 0);
}

// ---------------------------------------------------------------------------
void
getHDMIRXInputAudio (
    AUDIO_CAPS * pAudioCaps)
{
    unsigned char uc;

    uc                          = hdmirxrd(REG_RX_0AE); // REG_RX_AUD_CHSTAT3
    pAudioCaps->SampleFreq      = uc & M_FS;

    uc                          = hdmirxrd(REG_RX_0AA); // REG_RX_AUDIO_CH_STAT
    pAudioCaps->AudioFlag       = uc & 0xF0;
    pAudioCaps->AudSrcEnable    = uc & M_AUDIO_CH;
    pAudioCaps->AudSrcEnable    |= hdmirxrd(REG_RX_0AA) & M_AUDIO_CH;

    if ((uc & (B_HBRAUDIO | B_DSDAUDIO)) == 0)
    {
        uc = hdmirxrd(REG_RX_0AB);  // REG_RX_AUD_CHSTAT0

        if ((uc & B_NLPCM ) == 0)
        {
            pAudioCaps->AudioFlag |= B_CAP_LPCM;
        }
    }

    #ifdef EnableCalFs
// FIX_ID_023 xxxxx     //Fixed for Audio Channel Status Error with invalid HDMI source
    if (hdmirxrd(REG_RX_074) & 0x40)
    {
        AudioFsCal();
    }
// FIX_ID_023 xxxxx
    #endif
}

    #ifdef EnableCalFs

// FIX_ID_023 xxxxx     //Fixed for Audio Channel Status Error with invalid HDMI source
// ---------------------------------------------------------------------------
void
DumpNCTSReg (
    void)
{
        #if 0
    ushort  i, j;
    BYTE    ucData;

    HDMIRX_AUDIO_PRINTF(("\n       "));
    for (j = 0; j < 16; j++)
    {
        HDMIRX_AUDIO_PRINTF((" %02X", (int) j));
        if ((j == 3) || (j == 7) || (j == 11))
        {
            HDMIRX_AUDIO_PRINTF((" :"));
        }
    }
    HDMIRX_AUDIO_PRINTF(("\n"));

    chgbank(2);

    for (i = 0xB0; i < 0xD0; i += 16)
    {
        HDMIRX_AUDIO_PRINTF(("[%03X]  ", i));
        for (j = 0; j < 16; j++)
        {
            ucData = hdmirxrd((BYTE)((i + j) & 0xFF));
            HDMIRX_AUDIO_PRINTF((" %02X", (int) ucData));
            if ((j == 3) || (j == 7) || (j == 11))
            {
                HDMIRX_AUDIO_PRINTF((" :"));
            }
        }
        HDMIRX_AUDIO_PRINTF(("\n"));
    }

    HDMIRX_AUDIO_PRINTF(("\n        =====================================================\n"));

    chgbank(0);
        #endif
}

// ---------------------------------------------------------------------------
unsigned int
PCLKGet (
    void)
{
    unsigned char   rddata;
    unsigned int    PCLK; // , sump;

    rddata  = hdmirxrd(0x9A);
    PCLK    = (m_ROSCCLK * 255 / rddata) / 1000;
    // HDMIRX_AUDIO_PRINTF(("PCLKGet( ) PCLK = %ld\n",(int) PCLK));

    return PCLK;
}

// ---------------------------------------------------------------------------
void
TMDSGet (
    void)
{
    unsigned char   ucCurrentPort;
    unsigned int    ucTMDSClk = 0;
    unsigned char   rddata;
    unsigned char   ucClk;

    ucCurrentPort = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    if (ucCurrentPort == F_PORT_SEL_1)
    {
        ucClk   = hdmirxrd(REG_RX_092);
        rddata  = hdmirxrd(0x90);

        if (ucClk != 0)
        {
            if (rddata & 0x04)
            {
                ucTMDSClk = 2 * RCLKVALUE * 256 / ucClk;
            }
            else if (rddata & 0x08)
            {
                ucTMDSClk = 4 * RCLKVALUE * 256 / ucClk;
            }
            else
            {
                ucTMDSClk = RCLKVALUE * 256 / ucClk;
            }

            HDMIRX_AUDIO_PRINTF((" TMDSGet() Port 1 TMDS org  = %d\n", (int) m_u16TMDSCLK));
        }
    }
    else
    {
        // HDMI mode
        {
            ucClk   = hdmirxrd(REG_RX_091);
            rddata  = hdmirxrd(0x90);

            if (ucClk != 0)
            {
                if (rddata & 0x01)
                {
                    ucTMDSClk = 2 * RCLKVALUE * 256 / ucClk;
                }
                else if (rddata & 0x02)
                {
                    ucTMDSClk = 4 * RCLKVALUE * 256 / ucClk;
                }
                else
                {
                    ucTMDSClk = RCLKVALUE * 256 / ucClk;
                }

                // HDMIRX_AUDIO_PRINTF(("HDMI use TMDS to calculate FS %d\n",(int) ucTMDSClk));
            }
        }

        HDMIRX_AUDIO_PRINTF((" TMDSGet() Port 0 TMDS org  = %d\n", (int) m_u16TMDSCLK));
    }

    if (m_u16TMDSCLK == 0)
    {
        m_u16TMDSCLK = ucTMDSClk;
    }
    else
    {
        m_u16TMDSCLK    = (( ucTMDSClk + m_u16TMDSCLK ));
        m_u16TMDSCLK    /= 2;
    }
}

// ---------------------------------------------------------------------------
char m_prevForceFsValue = 0xF;

void
AudioFsCal (
    void)
{
    unsigned long   u32_N;
    unsigned long   u32_CTS;
    unsigned char   u8_FS;
    unsigned char   uc;
    unsigned long   sum         = 0;
    unsigned char   resetAudio  = 0;

    hdmirxset(REG_RX_077, 0x01, 0x01);
    chgbank(2);
    u32_N   = (unsigned long)(hdmirxrd(REG_RX_2C0) & 0x0F);
    u32_N   += (unsigned long)hdmirxrd(REG_RX_2BF) << 4;
    u32_N   += (unsigned long)hdmirxrd(REG_RX_2BE) << 12;

    u32_CTS = (unsigned long)((hdmirxrd(REG_RX_2C0) & 0xF0) >> 4);
    u32_CTS += (unsigned long)hdmirxrd(REG_RX_2C2) << 4;
    u32_CTS += (unsigned long)hdmirxrd(REG_RX_2C1) << 12;
    chgbank(0);

    DumpNCTSReg();
    HDMIRX_AUDIO_PRINTF((" u32_N %ld\n", (unsigned long) u32_N));
    HDMIRX_AUDIO_PRINTF((" u32_CTS %ld\n", (unsigned long) u32_CTS));

    if ((u32_N == 0) || (u32_CTS == 0))
    {
        return;
    }

    TMDSGet();

//  u8_FS=(unsigned char ) (((m_u16TMDSCLK*1000)*u32_N)/(128*u32_CTS));
    sum     = (u32_N * 1000 * m_u16TMDSCLK);
    u8_FS   = (unsigned char ) (sum / (128 * u32_CTS));

        #if 0
    u8_FS   = m_u16TMDSCLK;
    HDMIRX_AUDIO_PRINTF(("m_u16TMDSCLK %d\n", (int) u8_FS));
    u8_FS   = (((u32_N * m_u16TMDSCLK * 78) / u32_CTS) / 10);
        #endif
    HDMIRX_AUDIO_PRINTF(("u8_FS %d\n", (int) u8_FS));
    m_prevForceFsValue = m_ForceFsValue;

    // Judge FS by FS calulate
    if ((u8_FS > 25) && (u8_FS <= 38))
    {
        // FS=32k , Calu Value = 29k~36k
        m_ForceFsValue = (B_32K);
    }
    else if ((u8_FS > 38) && (u8_FS <= 46))
    {
        // FS=44k , Calu Value = 41k~46k
        m_ForceFsValue = (B_44P1K);
    }
    else if ((u8_FS > 46) && (u8_FS <= 58))
    {
        // FS=48k , Calu Value = 47k~51k
        m_ForceFsValue = (B_48K);
    }
    else if ((u8_FS > 78) && (u8_FS <= 92))
    {
        // FS=88k , Calu Value = 85k~91k
        m_ForceFsValue = (B_88P2K);
    }
    else if ((u8_FS > 92) && (u8_FS <= 106))
    {
        // FS=96k , Calu Value = 93k~99k
        m_ForceFsValue = (B_96K);
    }
    else if ((u8_FS > 166) && (u8_FS <= 182))
    {
        // FS=176k , Calu Value = 173k~179k
        m_ForceFsValue = (B_176P4K);
    }
    else if ((u8_FS > 182) && (u8_FS <= 202))
    {
        // FS=192k , Calu Value = 188k~195k
        m_ForceFsValue = (B_192K);
    }

    uc = hdmirxrd(REG_RX_0AE);  // REG_RX_AUD_CHSTAT3
    HDMIRX_AUDIO_PRINTF(("REG_RX_0AE %x ,", (int) ( uc & M_FS)));
    HDMIRX_AUDIO_PRINTF(("m_ForceFsValue %x\n", (int) ( m_ForceFsValue)));
    if (( uc & M_FS) == ( m_ForceFsValue ))
    {
        m_AudioChannelStatusErrorCount = 0;
        // no need to enable Force FS mode
        HDMIRX_AUDIO_PRINTF(("CHS_FS %x , !!!No need !!! to enable Force FS mode\n", (int) ( uc & M_FS)));
        if (hdmirxrd(REG_RX_074) & 0x40)
        {
            resetAudio = 1;
        }
        hdmirxset(REG_RX_074, 0x40, 0x00);    // reg74[6]=0 disable Force FS mode
        if (resetAudio == 1)
        {
            hdmirx_ResetAudio();
        }
        return;
    }

    HDMIRX_AUDIO_PRINTF(("m_AudioChannelStatusErrorCount=%bd\n", m_AudioChannelStatusErrorCount));

    if (m_AudioChannelStatusErrorCount > MAX_AUDIO_CHANNEL_STATUS_ERROR)
    {
        m_AudioChannelStatusErrorCount  = 0;
        m_ForceFsValue                  &= 0xF;
        m_ForceFsValue                  |= hdmirxrd(REG_RX_07B)  & 0xF0;
        // a. if find Audio Error in a period timers,assue the FS message is wrong,then try to force FS setting.
        // b. set Reg0x74[6]=1=> select Force FS mode.
        hdmirxset(REG_RX_074, 0x40, 0x40); // reg74[6]=1
        // f. set Reg0x7e[3:0]=0 (at leasst three times)=> force FS value
        // g. if Audio still Error,then repeat b~f setps.(on f setp,set another FS value
        // 0:44,1K,2: 48K,3:32K,8:88.2K,A:96K,C:176.4K,E:192K)
        hdmirxwr(REG_RX_07B, m_ForceFsValue);
        // hdmirxwr(REG_RX_07B,m_ForceFsValue);
        // hdmirxwr(REG_RX_07B,m_ForceFsValue);
        // hdmirxwr(REG_RX_07B,m_ForceFsValue);
        hdmirx_ResetAudio();
        HDMIRX_AUDIO_PRINTF(("ForceAudio\n", (int) ( uc & M_FS)));

        HDMIRX_AUDIO_PRINTF(("CHS_FS %x , !!! Enable Force FS mode !!!\n", (int) ( uc & M_FS)));
    }
}
// FIX_ID_023 xxxxx
    #endif

// ---------------------------------------------------------------------------

void
IT6602SwitchAudioState (
    struct IT6602_dev_data  * IT6602,
    Audio_State_Type        state)
{
//  unsigned char uc;

    if (IT6602->m_AState == state)
    {
        return;
    }

    HDMIRX_AUDIO_PRINTF((AStateStr[(unsigned char)state]));
    HDMIRX_AUDIO_PRINTF(("+++\n"));

    IT6602->m_AState = state;
    // AssignAudioVirtualTime();

    #ifdef _FIX_ID_028_
    // if under FIX_ID_028, do nothing on switch state.
        #ifdef _SUPPORT_HDMI_REPEATER_
    if (IT6602->m_AState != ASTATE_AudioOn)
    {
        it660x_DownStream_AudioReady(OFF);
    }
    else
    {
        it660x_DownStream_AudioReady(IT6602, ON);
    }
        #endif
    #else
        #ifdef _SUPPORT_HDMI_REPEATER_
    if (IT6602->m_AState != ASTATE_AudioOn)
    {
        it660x_DownStream_AudioReady(IT6602, OFF);
    }
        #endif
    switch (IT6602->m_AState)
    {
        case ASTATE_AudioOff:
            hdmirxset(REG_RX_RST_CTRL, B_AUDRST, B_AUDRST);
            IT6602AudioOutputEnable(OFF);

            break;

        case ASTATE_RequestAudio:
            IT6602AudioOutputEnable(OFF);

            break;

        case ASTATE_WaitForReady:
            hdmirx_SetHWMuteClr();
            hdmirx_ClearHWMuteClr();
            IT6602->m_AudioCountingTimer = AUDIO_READY_TIMEOUT;
            break;

        case ASTATE_AudioOn:
        #ifdef EnableCalFs
// FIX_ID_023 xxxxx     //Fixed for Audio Channel Status Error with invalid HDMI source
            AudioFsCal();
// FIX_ID_023 xxxxx
        #endif
            IT6602AudioOutputEnable(ON);

            HDMIRX_AUDIO_PRINTF(("Cat6023 Audio--> Audio flag=%02X,Ch No=%02X,Fs=%02X ...\n",
                (int)IT6602->m_RxAudioCaps.AudioFlag,
                (int)IT6602->m_RxAudioCaps.AudSrcEnable,
                (int)IT6602->m_RxAudioCaps.SampleFreq));
        #ifdef _SUPPORT_HDMI_REPEATER_
            it660x_DownStream_AudioReady(IT6602, ON);
        #endif

            break;
    }
    #endif
}

    #ifdef _FIX_ID_028_
// FIX_ID_028 xxxxx //For Debug Audio error with S2
// remove --> static void IT6602AudioHandler(struct IT6602_dev_data *IT6602)
// remove --> {
// remove --> }
// FIX_ID_028 xxxxx
    #else

void
IT6602AudioHandler (
    struct IT6602_dev_data * IT6602)
{
//    unsigned char uc;

    if (IT6602->m_AudioCountingTimer > MS_LOOP)
    {
        IT6602->m_AudioCountingTimer -= MS_LOOP;
    }
    else
    {
        IT6602->m_AudioCountingTimer = 0;
    }

    if (IT6602->m_RxHDCPState == RxHDCP_ModeCheck)
    {
        return;
    }

    switch (IT6602->m_AState)
    {
        case ASTATE_RequestAudio:

            getHDMIRXInputAudio(&(IT6602->m_RxAudioCaps));

            if (IT6602->m_RxAudioCaps.AudioFlag & B_CAP_AUDIO_ON)
            {
                hdmirxset(REG_RX_MCLK_CTRL, M_MCLKSel, B_256FS);

                if (IT6602->m_RxAudioCaps.AudioFlag & B_CAP_HBR_AUDIO)
                {
                    HDMIRX_AUDIO_PRINTF(("+++++++++++ B_CAP_HBR_AUDIO +++++++++++++++++\n"));

                    hdmirxset(REG_RX_MCLK_CTRL, M_MCLKSel, B_128FS); // MCLK = 128fs only for HBR audio

                    hdmirx_SetHWMuteClrMode();
                    hdmirx_ResetAudio();
                }
                else if (IT6602->m_RxAudioCaps.AudioFlag & B_CAP_DSD_AUDIO)
                {
                    hdmirx_SetHWMuteClrMode();
                    hdmirx_ResetAudio();
                }
                else
                {
                    hdmirxset(REG_RX_HWMuteCtrl, B_HWMuteClr, 0x00);
                    hdmirx_SetHWMuteClrMode();
                    hdmirx_ResetAudio();
                }

                IT6602SwitchAudioState(IT6602, ASTATE_WaitForReady);
            }
            break;

        case ASTATE_WaitForReady:

            // if(AudioTimeOutCheck(AUDIO_READY_TIMEOUT))
        #ifdef EnableCalFs
// FIX_ID_023 xxxxx     //Fixed for Audio Channel Status Error with invalid HDMI source
            TMDSGet();
// FIX_ID_023 xxxxx
        #endif
            if (IT6602->m_AudioCountingTimer == 0)
            {
                IT6602SwitchAudioState(IT6602, ASTATE_AudioOn);
            }
            break;

        case ASTATE_AudioOn:
            // if(AudioTimeOutCheck(AUDIO_MONITOR_TIMEOUT)==TRUE)
            if (IT6602->m_AudioCountingTimer == 0)
            {
                AUDIO_CAPS CurAudioCaps;
                // IT6602->m_AudioCountingTimer = GetCurrentVirtualTime();
                // AssignAudioTimerTimeout(AUDIO_MONITOR_TIMEOUT);
                IT6602->m_AudioCountingTimer = AUDIO_MONITOR_TIMEOUT;

                getHDMIRXInputAudio(&CurAudioCaps);

                if ((IT6602->m_RxAudioCaps.AudioFlag != CurAudioCaps.AudioFlag)
               || (IT6602->m_RxAudioCaps.AudSrcEnable != CurAudioCaps.AudSrcEnable)
               || (IT6602->m_RxAudioCaps.SampleFreq != CurAudioCaps.SampleFreq))
                {
                    // IT6602->m_ucHDMIAudioErrorCount=0;
                    IT6602SwitchAudioState(IT6602, ASTATE_RequestAudio);
                }
            }
            break;
    }
}

    #endif

#endif
#ifdef _it660x_
// ***************************************************************************
// Video function
// ***************************************************************************

void
IT6602_AFE_Rst (
    void)
{
    unsigned char           Reg51h;

    struct IT6602_dev_data  * IT6602data = get_IT6602_dev_data();   // 2013-0814

    chgbank(0);
    Reg51h = hdmirxrd(0x51);
    if (Reg51h & 0x01)
    {
        HDMIRX_VIDEO_PRINTF(("=== port 1 IT6602_AFE_Rst() ===\n"));
        hdmirxset(REG_RX_018, 0x01, 0x01);
        delay1ms(1);
        hdmirxset(REG_RX_018, 0x01, 0x00);
    #ifdef _SUPPORT_AUTO_EQ_
        DisableOverWriteRS(1);  // 2013-1129
    #endif
    }
    else
    {
        HDMIRX_VIDEO_PRINTF(("=== port 0 IT6602_AFE_Rst() ===\n"));
        hdmirxset(REG_RX_011, 0x01, 0x01);
        delay1ms(1);
        hdmirxset(REG_RX_011, 0x01, 0x00);
    #ifdef _SUPPORT_AUTO_EQ_
        DisableOverWriteRS(0);
    #endif
    }

    IT6602data->m_ucSCDTOffCount = 0; // 2013-0814

// FIX_ID_008 xxxxx
}

// xxxxx 2014-0529 //HDCP Content On/Off
void
IT6602_ManualVideoTristate (
    unsigned char bOff)
{
    if (bOff)
    {
        hdmirxset(REG_RX_053, (B_VDGatting | B_VIOSel), (B_VDGatting | B_VIOSel));  // Reg53[7][5] = 11    // for enable B_VDIO_GATTING and VIO_SEL
        hdmirxset(REG_RX_052, (B_DisVAutoMute), (B_DisVAutoMute));                  // Reg52[5] = 1 for disable Auto video MUTE
        hdmirxset(REG_RX_053, (B_TriVDIO), (0x00));                                 // Reg53[2:0] = 000;         // 0 for enable video io data output
        HDMIRX_VIDEO_PRINTF(("+++++++++++ Manual Video / Audio off  +++++++++++++++++\n"));
    }
    else
    {
        hdmirxset(REG_RX_053, (B_TriSYNC), (0x00));                                 // Reg53[0] = 0;                 // for enable video sync
        hdmirxset(REG_RX_053, (B_TriVDIO), (0x00));                                 // Reg53[3:1] = 000;         // 0 for enable video io data output
        hdmirxset(REG_RX_053, (B_TriVDIO), (B_TriVDIO));                            // Reg53[2:0] = 111;         // 1 for enable tri-state of video io data
        hdmirxset(REG_RX_053, (B_TriVDIO), (0x00));                                 // Reg53[2:0] = 000;         // 0 for enable video io data output
        hdmirxset(REG_RX_053, (B_VDGatting | B_VIOSel), (B_VDGatting | B_VIOSel));  // Reg53[7][5] = 11    // for enable B_VDIO_GATTING and VIO_SEL
        hdmirxset(REG_RX_053, (B_VDGatting | B_VIOSel), (B_VIOSel));                // Reg53[7][5] = 01    // for disable B_VDIO_GATTING
        HDMIRX_VIDEO_PRINTF(("+++++++++++ Manual Video on  +++++++++++++++++\n"));
    }
}

void
IT6602_HDCP_ContentOff (
    unsigned char   ucPort,
    unsigned char   bOff)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();     // 2013-0814

    if (IT6602_IsSelectedPort(ucPort) == FALSE)
    {
        return;
    }

    if (bOff != 0)
    {
        // ******** Content Off ********//
        IT6602_ManualVideoTristate(1);
        IT6602data->m_HDCP_ContentOff = 1;
        HDMIRX_VIDEO_PRINTF(("+++++++++++ HDCP Content Off   +++++++++++++++++\n"));
    }
    else
    {
        if (IT6602data->m_VState == VSTATE_VideoOn)
        {
            if (IT6602data->m_HDCP_ContentOff == 1)
            {
                // ******** Content On ********//
                IT6602_ManualVideoTristate(0);
                HDMIRX_VIDEO_PRINTF(("+++++++++++ HDCP Content On   +++++++++++++++++\n"));
            }
        }

        IT6602data->m_HDCP_ContentOff = 0;
    }
}

void
IT6602_SetVideoMute (
    struct IT6602_dev_data  * IT6602,
    unsigned char           bMute)
{
    if (bMute)
    {
        // ******** AV Mute -> ON ********//
        hdmirxset(REG_RX_053, (B_VDGatting | B_VIOSel), (B_VDGatting | B_VIOSel));  // Reg53[7][5] = 11    // for enable B_VDIO_GATTING and VIO_SEL
        hdmirxset(REG_RX_052, (B_DisVAutoMute), (B_DisVAutoMute));                  // Reg52[5] = 1 for disable Auto video MUTE
        hdmirxset(REG_RX_053, (B_TriVDIO), (0x00));                                 // Reg53[2:0] = 000;         // 0 for enable video io data output

        HDMIRX_VIDEO_PRINTF(("+++++++++++ IT6602_SetVideoMute -> On +++++++++++++++++\n"));
    }
    else
    {
        if (IT6602->m_VState == VSTATE_VideoOn)
        {
            // ******** AV Mute -> OFF ********//
            hdmirxset(REG_RX_053, (B_TriSYNC), (0x00));                                     // Reg53[0] = 0;                 // for enable video sync
            hdmirxset(REG_RX_053, (B_TriVDIO), (0x00));                                     // Reg53[3:1] = 000;         // 0 for enable video io data output

            if (CheckAVMute() == TRUE)
            {
                hdmirxset(REG_RX_052, (B_DisVAutoMute), (B_DisVAutoMute));                  // Reg52[5] = 1 for disable Auto video MUTE
            }
            else
            {
                // enable output
                hdmirxset(REG_RX_064, 0x80, 0x80);                                          // video fifo reset
                hdmirxset(REG_RX_064, 0x80, 0x00);                                          // video fifo reset
                delay1ms(10);

                hdmirxset(REG_RX_053, (B_TriVDIO), (B_TriVDIO));                            // Reg53[2:0] = 111;         // 1 for enable tri-state of video io data
                hdmirxset(REG_RX_053, (B_TriVDIO), (0x00));                                 // Reg53[2:0] = 000;         // 0 for enable video io data output

                hdmirxset(REG_RX_053, (B_VDGatting | B_VIOSel), (B_VDGatting | B_VIOSel));  // Reg53[7][5] = 11    // for enable B_VDIO_GATTING and VIO_SEL
                hdmirxset(REG_RX_053, (B_VDGatting | B_VIOSel), (B_VIOSel));                // Reg53[7][5] = 01    // for disable B_VDIO_GATTING

                HDMIRX_VIDEO_PRINTF(("+++++++++++  IT6602_SetVideoMute -> Off +++++++++++++++++\n"));

                //      get_vid_info();
                //      show_vid_info();
            }
        }
    }
}

void
IT6602VideoOutputEnable (
    unsigned char bEnableOutput)
{
//  struct IT6602_dev_data *IT6602data = get_IT6602_dev_data();
    if (bEnableOutput)
    {
        // enable output

        // enable output
        hdmirxset(REG_RX_064, 0x80, 0x80);  // video fifo reset
        hdmirxset(REG_RX_064, 0x80, 0x00);  // video fifo reset
        delay1ms(10);

        hdmirxset(REG_RX_053, (B_TriSYNC | B_TriVDIO), (0x00));
        HDMIRX_VIDEO_PRINTF(("---------------- IT6602VideoOutputEnable -> On ----------------\n"));
//      IT6602VideoOutputCDSet();
    }
    else
    {
        // disable output
        hdmirxset(REG_RX_053, (B_TriSYNC | B_TriVDIO), (B_TriSYNC | B_TriVDIO));
        HDMIRX_VIDEO_PRINTF(("---------------- IT6602VideoOutputEnable -> Off ----------------\n"));
    }
}

void
IT6602SwitchVideoState (
    struct IT6602_dev_data  * IT6602,
    Video_State_Type        eNewVState)
{
    if (IT6602->m_VState == eNewVState)
    {
        return;
    }

    HDMIRX_VIDEO_PRINTF((VStateStr[(unsigned char)eNewVState]));
    HDMIRX_VIDEO_PRINTF(("+++\n"));

    IT6602->m_VState = eNewVState;
//  IT6602->m_VideoCountingTimer = GetCurrentVirtualTime(); // get current time tick, and the next tick judge in the polling handler.

    #ifdef _SUPPORT_HDMI_REPEATER_
    if (IT6602->m_VState != VSTATE_VideoOn)
    {
        it660x_DownStream_VideoReady(IT6602, OFF);
    }
    #endif

    switch (IT6602->m_VState)
    {
        case VSTATE_Off:
            IT6602HPDCtrl(0, 0);
            IT6602HPDCtrl(1, 0); // if downstream switch off the state, it should be hold and HPD goes to off.

            break;

        case VSTATE_SWReset:
        {
            IT6602VideoOutputEnable(FALSE);
            // FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
    #ifdef _AVOID_REDUNDANCE_CSC_
            IT6602->m_Backup_OutputVideoMode    = 0xFF;
            IT6602->m_Backup_InputVideoMode     = 0xFF;
    #endif
            // FIX_ID_039 xxxxx
            IT6602_AFE_Rst();
    #ifdef Enable_IT6602_CEC
            // xxxxx FIX_ID_022     //Fixed for CEC capacitor issue
            IT6602_ResetCEC();
            // xxxxx
    #endif
            if (CheckPlg5VPwr(0) == FALSE)
            {
                IT6602HPDCtrl(0, 0);    // clear ucport's HPD = 0
            }
            else
            {
                IT6602HPDCtrl(0, 1);    // set ucport's HPD = 1
            }

            if (CheckPlg5VPwr(1) == FALSE)
            {
                IT6602HPDCtrl(1, 0);    // clear ucport's HPD = 0
            }
            else
            {
                IT6602HPDCtrl(1, 1);    // set ucport's HPD = 1
            }
            // Enable HDMI DDC bus to access it660x EDID RAM
        }
        break;

        case VSTATE_SyncWait: {
            // 1. SCDT off interrupt
            // 2. VideoMode_Chg interrupt
            IT6602VideoOutputEnable(FALSE);
// FIX_ID_039 xxxxx fix image flick when enable RGB limited / Full range convert
    #ifdef _AVOID_REDUNDANCE_CSC_
            IT6602->m_Backup_OutputVideoMode    = 0xFF;
            IT6602->m_Backup_InputVideoMode     = 0xFF;
    #endif
// FIX_ID_039 xxxxx
            IT6602->m_NewAVIInfoFrameF  = FALSE;
            IT6602->m_ucDeskew_P0       = 0;
            IT6602->m_ucDeskew_P1       = 0;
            // IT6602->m_ucSCDTOffCount=0;

    #ifdef Enable_Vendor_Specific_packet
            IT6602->f_de3dframe_hdmi = FALSE;
            hdmirxwr(REG_RX_06A, 0x82);
    #endif
        }
        break;

        case VSTATE_SyncChecking: {
            // 1. SCDT on interrupt
            // AssignVideoVirtualTime(VSATE_CONFIRM_SCDT_COUNT);
            // AssignVideoTimerTimeout(VSATE_CONFIRM_SCDT_COUNT);

            IT6602->m_VideoCountingTimer = VSATE_CONFIRM_SCDT_COUNT;

    #ifdef Enable_Vendor_Specific_packet
            hdmirxwr(REG_RX_06A, 0x82);
    #endif
        }
        break;

        case VSTATE_VideoOn: {
            IT6602VideoOutputConfigure(IT6602);
            delay1ms(800);// make sure get correct vid info [2020 03 19 bu5 modify]
            IT6602VideoOutputEnable(TRUE);
            IT6602SwitchAudioState(IT6602, ASTATE_RequestAudio);

            get_vid_info();
            show_vid_info();

            hdmirxwr(0x84, 0x8F);   // 2011/06/17 xxxxx, for enable Rx Chip count

    #ifdef Enable_Vendor_Specific_packet
            hdmirxwr(REG_RX_06A, 0x81);
    #endif

            // xxxxx 2013-0812 @@@@@
            IT6602->m_ucSCDTOffCount = 0;
            // xxxxx 2013-0812
    #ifdef _SUPPORT_HDMI_REPEATER_
            it660x_DownStream_VideoReady(IT6602, ON);
    #endif
        }
        break;
    }
}

// ---------------------------------------------------------------------------
void
IT6602VideoHandler (
    struct IT6602_dev_data * IT6602)
{
//  unsigned char uc;

    if (IT6602->m_VideoCountingTimer > MS_LOOP)
    {
        IT6602->m_VideoCountingTimer -= MS_LOOP;
    }
    else
    {
        IT6602->m_VideoCountingTimer = 0;
    }

    switch (IT6602->m_VState)
    {
        case VSTATE_SyncWait: {
            // Waiting for SCDT on interrupt !!!
            // if(VideoCountingTimer==0)

            WaitingForSCDT(IT6602);
        }
        break;

        case VSTATE_SyncChecking: {
            // if(VideoTimeOutCheck(VSATE_CONFIRM_SCDT_COUNT))
            if (IT6602->m_VideoCountingTimer == 0)
            {
                IT6602SwitchVideoState(IT6602, VSTATE_VideoOn);
            }
        }
        break;

        case VSTATE_VideoOn:

            if (IT6602->m_NewAVIInfoFrameF == TRUE)
            {
                if (IT6602->m_RxHDCPState != RxHDCP_ModeCheck)
                {
                    IT6602VideoOutputConfigure(IT6602);
                    IT6602->m_NewAVIInfoFrameF = FALSE;
                }
            }

            if (hdmirxrd(REG_RX_053) & B_VDGatting)
            {
                // xxxxx 2014-0529 //Manual Content On/Off
                // if(IT6602_IsSelectedPort(0)
                {
                    if ((IT6602->m_HDCP_ContentOff == 0))
                    {
                        if (CheckAVMute() == FALSE)
                        {
                            IT6602_SetVideoMute(IT6602, OFF);
                        }
                    }
                }
                // xxxxx 2014-0529
                // FIX_ID_037 xxxxx
            }

    #ifdef _FIX_ID_028_
// FIX_ID_028 xxxxx //For Debug Audio error with S2
            if (hdmirxrd(REG_RX_0AA) & 0x80)
            {
            }
            else
            {
                if (IT6602->m_AState == ASTATE_AudioOn)
                {
                    IT6602AudioOutputEnable(FALSE);
                }
            }
// FIX_ID_028 xxxxx
    #endif

            break;
    }
}

#endif

#ifdef _it660x_

// ***************************************************************************
// Interrupt function
// ***************************************************************************
void
hdmirx_INT_5V_Pwr_Chg (
    struct IT6602_dev_data  * IT6602,
    unsigned char           ucport)
{
    unsigned char ucPortSel;
    ucPortSel = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    // ite180509
    chgbank(1);
    hdmirxset(0xB1, 0x60, 0x00);
    hdmirxset(0xC1, 0x60, 0x00);
    chgbank(0);
    CLKStbChkCnt = 0;
    if (ucPortSel == ucport)
    {
        if (CheckPlg5VPwr(ucport) == TRUE)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Power 5V ON ####\n"));
            IT6602SwitchVideoState(IT6602, VSTATE_SyncWait);
            IT6602HPDCtrl(ucport, 1);       // set ucport's HPD = 1
        }
        else
        {
            IT6602_DEBUG_INT_PRINTF(("#### Power 5V OFF ####\n"));
            IT6602HPDCtrl(ucport, 0);       // clear ucport's HPD = 0
            IT6602SwitchVideoState(IT6602, VSTATE_SWReset);
        }
    }
    else
    {
        if (CheckPlg5VPwr(ucport) == FALSE)
        {
    #ifdef _SUPPORT_AUTO_EQ_
            DisableOverWriteRS(ucport);
    #endif
            IT6602HPDCtrl(ucport, 0);       // clear ucport's HPD = 0
        }
        else
        {
            IT6602HPDCtrl(ucport, 1);       // set ucport's HPD = 1
        }
    }
// FIX_ID_037 xxxxx
}

// ---------------------------------------------------------------------------
void
hdmirx_INT_P0_ECC (
    struct IT6602_dev_data * IT6602)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    if ((IT6602->m_ucEccCount_P0++) > ECC_TIMEOUT)
    {
    #ifdef _SUPPORT_EQ_ADJUST_
        if (IT6602->EQPort[F_PORT_SEL_0].f_manualEQadjust == TRUE) // ignore ECC interrupt when manual EQ adjust !!!
        {
            return;
        }
    #endif

        IT6602->m_ucEccCount_P0 = 0;

        IT6602_DEBUG_INT_PRINTF(("CDR reset for Port0 ECC_TIMEOUT !!!\n"));

        hdmirx_ECCTimingOut(F_PORT_SEL_0);
    }
}

// ---------------------------------------------------------------------------
void
hdmirx_INT_P1_ECC (
    struct IT6602_dev_data * IT6602)
{
    if ((IT6602->m_ucEccCount_P1++) > ECC_TIMEOUT)
    {
    #ifdef _SUPPORT_EQ_ADJUST_
        if (IT6602->EQPort[F_PORT_SEL_1].f_manualEQadjust == TRUE) // ignore ECC interrupt when manual EQ adjust !!!
        {
            return;
        }
    #endif

        IT6602->m_ucEccCount_P1 = 0;

        IT6602_DEBUG_INT_PRINTF(("CDR reset for Port1 ECC_TIMEOUT !!!\n"));

        hdmirx_ECCTimingOut(F_PORT_SEL_1);
    }
}

// ---------------------------------------------------------------------------
void
hdmirx_INT_P0_Deskew (
    struct IT6602_dev_data * IT6602)
{
    if ((IT6602->m_ucDeskew_P0++) > DESKEW_TIMEOUT)
    {
    #ifdef _SUPPORT_EQ_ADJUST_
        if (IT6602->EQPort[F_PORT_SEL_0].f_manualEQadjust == TRUE) // ignore ECC interrupt when manual EQ adjust !!!
        {
            return;
        }
    #endif
        IT6602->m_ucDeskew_P0 = 0;

        IT6602_DEBUG_INT_PRINTF(("CDR reset for Port0 DESKEW_TIMEOUT !!!\n"));

        if (hdmirxrd(REG_RX_020) == 0x00)
        {
            hdmirxwr(REG_RX_020, 0x3F);     // Dr. Liu suggestion to 0x00
        }
        else
        {
            hdmirxwr(REG_RX_020, 0x00);     // Dr. Liu suggestion to 0x3F
        }
    }
}

// ---------------------------------------------------------------------------
void
hdmirx_INT_P1_Deskew (
    struct IT6602_dev_data * IT6602)
{
    if ((IT6602->m_ucDeskew_P1++) > DESKEW_TIMEOUT)
    {
    #ifdef _SUPPORT_EQ_ADJUST_
        if (IT6602->EQPort[F_PORT_SEL_1].f_manualEQadjust == TRUE) // ignore ECC interrupt when manual EQ adjust !!!
        {
            return;
        }
    #endif

        IT6602->m_ucDeskew_P1 = 0;

        IT6602_DEBUG_INT_PRINTF(("CDR reset for Port1 DESKEW_TIMEOUT !!!\n"));

        if (hdmirxrd(REG_RX_038) == 0x00)
        {
            hdmirxwr(REG_RX_038, 0x3F);     // Dr. Liu suggestion to 0x00
        }
        else
        {
            hdmirxwr(REG_RX_038, 0x00);     // Dr. Liu suggestion to 0x3F
        }
    }
}

// ---------------------------------------------------------------------------
// FIX_ID_009 xxxxx //verify interrupt event with reg51[0] select port
void
hdmirx_INT_HDMIMode_Chg (
    struct IT6602_dev_data  * IT6602,
    unsigned char           ucport)
{
    unsigned char ucPortSel;
    ucPortSel = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    if (ucPortSel != ucport)
    {
        return;
    }
// FIX_ID_009 xxxxx

    if (IsHDMIMode())
    {
        if (IT6602->m_VState == VSTATE_VideoOn)
        {
            IT6602SwitchAudioState(IT6602, ASTATE_RequestAudio);
        }
        IT6602->m_bUpHDMIMode = TRUE;
        IT6602_DEBUG_INT_PRINTF(("#### HDMI/DVI Mode : HDMI ####\n"));
    }
    else
    {
        IT6602SwitchAudioState(IT6602, ASTATE_AudioOff);
        IT6602->m_NewAVIInfoFrameF = FALSE;
        if (IT6602->m_VState == VSTATE_VideoOn)
        {
            SetDVIVideoOutput(IT6602);
        }
        IT6602->m_bUpHDMIMode = FALSE;
        IT6602_DEBUG_INT_PRINTF(("#### HDMI/DVI Mode : DVI ####\n"));
    }
}

// ---------------------------------------------------------------------------
void
hdmirx_INT_SCDT_Chg (
    struct IT6602_dev_data * IT6602)
{
    if (CheckSCDT(IT6602) == TRUE)
    {
        IT6602_DEBUG_INT_PRINTF(("#### SCDT ON ####\n"));
        IT6602SwitchVideoState(IT6602, VSTATE_SyncChecking);
    }
    else
    {
        IT6602_DEBUG_INT_PRINTF(("#### SCDT OFF ####\n"));
        IT6602SwitchVideoState(IT6602, VSTATE_SyncWait);
        IT6602SwitchAudioState(IT6602, ASTATE_AudioOff);

//      TMDSCheck(IT6602->m_ucCurrentHDMIPort);
//      TogglePolarity (IT6602->m_ucCurrentHDMIPort);
    }
}

// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_AUTO_EQ_

void
hdmirx_INT_EQ_FAIL (
    struct IT6602_dev_data  * IT6602,
    unsigned char           ucPortSel)
{
    if (ucPortSel > F_PORT_SEL_1)
    {
        return;
    }

        #ifdef _SUPPORT_EQ_ADJUST_
    if (IT6602->EQPort[ucPortSel].f_manualEQadjust == FALSE)   // ignore EQ fail interrupt when manual EQ adjust !!!
        #endif
    {
        if (CheckPlg5VPwr(ucPortSel))
        {
            // 07-08
            if (ucPortSel == 0)
            {
                if ((IT6602->HDMIIntEvent & (B_PORT0_TMDSEvent)))
                {
                    IT6602_DEBUG_INT_PRINTF(("#### hdmirx_INT_EQ_FAIL not yet !!! ####\n"));

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
                    if ((IT6602->HDMIIntEvent & (B_PORT0_Waiting)) == 0)
                    {
                        hdmirxwr(REG_RX_022, 0x00); // power down auto EQ
                        IT6602->HDMIIntEvent    |= (B_PORT0_Waiting);
                        IT6602->HDMIIntEvent    |= (B_PORT0_TMDSEvent);
                        IT6602->HDMIWaitNo[0]   = MAX_TMDS_WAITNO;
                    }
                    else if ((IT6602->HDMIIntEvent & (B_PORT0_TMDSEvent)))
                    {
                        IT6602->HDMIIntEvent    |= (B_PORT0_Waiting);
                        IT6602->HDMIWaitNo[0]   += MAX_HDCP_WAITNO;
                    }

// FIX_ID_033 xxxxx

                    return;
                }

                {
                    if ((ucPortAMPValid[ucPortSel] & 0x3F) != 0x3F)
                    {
                        AmpValidCheck(ucPortSel);
                    }
                }
            }
            else
            {
                if ((IT6602->HDMIIntEvent & (B_PORT1_TMDSEvent)))
                {
                    IT6602_DEBUG_INT_PRINTF(("#### hdmirx_INT_EQ_FAIL not yet !!! ####\n"));

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
                    if ((IT6602->HDMIIntEvent & (B_PORT1_Waiting)) == 0)
                    {
                        hdmirxwr(REG_RX_03A, 0x00); // power down auto EQ
                        IT6602->HDMIIntEvent    |= (B_PORT1_Waiting);
                        IT6602->HDMIIntEvent    |= (B_PORT1_TMDSEvent);
                        IT6602->HDMIWaitNo[1]   = MAX_TMDS_WAITNO;
                    }
                    else if ((IT6602->HDMIIntEvent & (B_PORT1_TMDSEvent)))
                    {
                        IT6602->HDMIIntEvent    |= (B_PORT1_Waiting);
                        IT6602->HDMIWaitNo[1]   += MAX_HDCP_WAITNO;
                    }
// FIX_ID_033 xxxxx

                    return;
                }

                if ((ucPortAMPValid[ucPortSel] & 0x3F) != 0x3F)
                {
                    AmpValidCheck(ucPortSel);
                }
            }

            {
                // 07-08
                if (ucPortSel == 0)
                {
                    // FIX_ID_043
                    // If Toggle Count too much, stop AUTO EQ and Force manual EQ;
                    if (IT6602->ToggleCount[ucPortSel] >= MAX_TOGGLE_FAIL_COUNT)
                    {
                        IT6602_DEBUG_INT_PRINTF(("#### hdmirx_INT_EQ0_FAIL has toggled, and manual EQ!!! IT6602->ToggleCount[%d] = %d ####\n", ucPortSel, IT6602->ToggleCount[ucPortSel]));
                        hdmirxwr(REG_RX_022, 0x00);
                        HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_0]));
                    }
                    else
                    {
                        {
                            if ((ucPortAMPValid[ucPortSel] & 0x3F) != 0x3F)
                            {
                                TogglePolarity (ucPortSel);
                                IT6602->ToggleCount[ucPortSel]++;
                            }
                        }
                        IT6602_DEBUG_INT_PRINTF(("IT6602->ToggleCount[%d] = %d\n", ucPortSel, IT6602->ToggleCount[ucPortSel]));
                    }
                }
                else
                {
                    if (IT6602->ToggleCount[ucPortSel] >= MAX_TOGGLE_FAIL_COUNT)
                    {
                        IT6602_DEBUG_INT_PRINTF(("#### hdmirx_INT_EQ1_FAIL has toggled, and manual EQ!!! IT6602->ToggleCount[%d] = %d ####\n", ucPortSel, IT6602->ToggleCount[ucPortSel]));
                        hdmirxwr(REG_RX_03A, 0x00);
                        HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_1]));
                    }
                    else
                    {
                        if ((ucPortAMPValid[ucPortSel] & 0x3F) != 0x3F)
                        {
                            TogglePolarity (ucPortSel);
                            IT6602->ToggleCount[ucPortSel]++;
                        }
                    }
                    IT6602_DEBUG_INT_PRINTF(("IT6602->ToggleCount[%d] = %d\n", ucPortSel, IT6602->ToggleCount[ucPortSel]));
                }
                // FIX_ID_043
            }
        }
    }
}
    #endif
// FIX_ID_001 xxxxx
#endif

#if 1

    #define CP_MEAN_VALUE       (48696)
    #define CP_MAX_DEVIATION    (CP_MEAN_VALUE * 50 / 100)

// FIX_ID_021 xxxxx     //To use CP_100ms for CBus_100ms and CEC_100ms
////////////////////////////////////////////////////////////////////
// static unsigned char OSCvalueCompare(unsigned long *calibrationValue)
//
//
//
////////////////////////////////////////////////////////////////////
unsigned char
OSCvalueCompare (
    unsigned long * calibrationValue)
{
    unsigned long   diff_cp;
    unsigned long   cp_cal_mean = CP_MEAN_VALUE;

    if (*calibrationValue != 0)
    {
        *calibrationValue /= 100;
    }

    if (*calibrationValue >= cp_cal_mean)
    {
        diff_cp = (*calibrationValue - cp_cal_mean);
        IT6602_DEBUG_INT_PRINTF(("diff_cp=%ld\n", diff_cp));
    }
    else
    {
        diff_cp = (cp_cal_mean - *calibrationValue);
        IT6602_DEBUG_INT_PRINTF(("diff_cp=%ld\n", diff_cp));
    }

    if (diff_cp > CP_MAX_DEVIATION)
    {
        IT6602_DEBUG_INT_PRINTF(("The Calibration Value Error\n"));
        return FALSE;
    }

    IT6602_DEBUG_INT_PRINTF(("calibrationValue = %ld MHz\n", *calibrationValue));

    return TRUE;
}

////////////////////////////////////////////////////////////////////
// static unsigned long CP_OCLK( void )
//
//
//
////////////////////////////////////////////////////////////////////
unsigned long
CP_OCLK (
    void)
{
    unsigned char   SIP_00B;
    unsigned char   SIP_01B;
    unsigned char   SIP_02B;
    unsigned char   CP_100ms_00B;
    unsigned char   CP_100ms_01B;
    unsigned char   CP_100ms_02B;
    unsigned long   rddata;

    hdmirxwr(0x0F, 0x00);
    hdmirxwr(0x73, 0x05);
    hdmirxwr(0x0F, 0x01);
    hdmirxwr(REG_RX_157, 0x12);
    hdmirxwr(REG_RX_157, 0x02);
    hdmirxwr(REG_RX_156, 0x01);

    // set Read Address and enable
    hdmirxwr(REG_RX_150, 0x00);
    hdmirxwr(REG_RX_151, 0x00);
    hdmirxwr(REG_RX_153, 0x04); // reg153
    SIP_00B = hdmirxrd(REG_RX_161);
    SIP_01B = hdmirxrd(REG_RX_161);
    SIP_02B = hdmirxrd(REG_RX_161);

    IT6602_DEBUG_INT_PRINTF(("SIP_00B=%X\n", (int)SIP_00B));
    IT6602_DEBUG_INT_PRINTF(("SIP_01B=%X\n", (int)SIP_01B));
    IT6602_DEBUG_INT_PRINTF(("SIP_02B=%X\n", (int)SIP_02B));

    if ((SIP_00B == 0x02) && (SIP_01B == 0x02) && (SIP_02B == 0x02))
    {
        hdmirxwr(REG_RX_150, 0x01);
        hdmirxwr(REG_RX_151, 0x60);
        hdmirxwr(REG_RX_153, 0x04);
        CP_100ms_00B    = hdmirxrd(REG_RX_161);
        CP_100ms_01B    = hdmirxrd(REG_RX_161);
        CP_100ms_02B    = hdmirxrd(REG_RX_161);

        // disable -> if(CP_100ms_02B > 0x60 || CP_100ms_02B < 0x40)
        // disable -> {
        // disable -> CBUS_CAL_PRINTF(("SIPROM CP 100MS Error\n"));
        // disable -> rddata = 0;
        // disable -> }
        // disable -> else
        {
            rddata  = (unsigned long)(CP_100ms_02B);
            rddata  <<= 8;
            rddata  |= (unsigned long)(CP_100ms_01B);
            rddata  <<= 8;
            rddata  |= (unsigned long) (CP_100ms_00B);
            IT6602_DEBUG_INT_PRINTF(("SIPROM CP 100MS %x\n", rddata));
        }
    }
    else if ((SIP_00B == 0xFF) && (SIP_01B == 0x00) && (SIP_02B == 0xFF))
    {
        hdmirxwr(REG_RX_150, 0x03);
        hdmirxwr(REG_RX_151, 0x60);
        hdmirxwr(REG_RX_153, 0x04);
        CP_100ms_00B    = hdmirxrd(REG_RX_161);
        CP_100ms_01B    = hdmirxrd(REG_RX_161);
        CP_100ms_02B    = hdmirxrd(REG_RX_161);
        // disable -> if(CP_100ms_02B > 0x60 || CP_100ms_02B < 0x40)
        // disable -> {
        // disable -> CBUS_CAL_PRINTF(("SIPROM CP 100MS Error\n"));
        // disable -> rddata = 0;
        // disable -> }
        // disable -> else
        {
            rddata  = (unsigned long)(CP_100ms_02B);
            rddata  <<= 8;
            rddata  |= (unsigned long)(CP_100ms_01B);
            rddata  <<= 8;
            rddata  |= (unsigned long) (CP_100ms_00B);
            IT6602_DEBUG_INT_PRINTF(("SIPROM CP 100MS %x\n", rddata));
        }
    }
    else
    {
        IT6602_DEBUG_INT_PRINTF(("SIPROM CP 100MS Error\n"));
        rddata = 0;
    }

    hdmirxwr(0x0F, 0x00);
    hdmirxwr(0x73, 0x00);

    IT6602_DEBUG_INT_PRINTF(("CP_100ms_00B=%X\n", (int)CP_100ms_00B));
    IT6602_DEBUG_INT_PRINTF(("CP_100ms_01B=%X\n", (int)CP_100ms_01B));
    IT6602_DEBUG_INT_PRINTF(("CP_100ms_02B=%X\n", (int)CP_100ms_02B));

    return rddata;
}

////////////////////////////////////////////////////////////////////
// void cal_oclk( void )
//
//
//
////////////////////////////////////////////////////////////////////
void
Cal_oclk (
    void)
{
    unsigned char   oscdiv;
    unsigned long   OSCCLK;

    {
        OSCCLK = CP_OCLK();
        IT6602_DEBUG_INT_PRINTF(("CP_OSCCLK = %ld MHz\n", OSCCLK));
        if (OSCvalueCompare(&OSCCLK) == FALSE)
        {
            OSCCLK = CP_MEAN_VALUE;
            // OSCCLK = (unsigned long) sum;
            // OSCCLK /= 100;
            IT6602_DEBUG_INT_PRINTF(("Use CP_MEAN_VALUE = %ld MHz\n", OSCCLK));
        }
        else
        {
            IT6602_DEBUG_INT_PRINTF(("Use CP Calibration Value\n"));
        }
    }

    IT6602_DEBUG_INT_PRINTF(("OSCCLK=%ld MHz\n", OSCCLK / 1000));
    oscdiv = (unsigned char) (OSCCLK / 1000 / 10);

    if (((OSCCLK / 1000 / oscdiv) - 10) > (10 - (OSCCLK / 1000 / (oscdiv + 1))))
    {
        oscdiv++;
    }

    IT6602_DEBUG_INT_PRINTF(("OCLK=%ld MHz\n", OSCCLK / 1000 / oscdiv));

// FIX_ID_051 calibration CEC timeout
    #ifdef Enable_IT6602_CEC
    timeunit = (u8_t)(OSCCLK / 640);
    IT6602_DEBUG_INT_PRINTF(("CEC timeunit (T100us) = %X\n", (int)timeunit));
    #endif
}
// FIX_ID_021 xxxxx
// #endif
#endif

#ifdef _SUPPORT_EDID_RAM_
/*****************************************************************************/
/* EDID RAM  functions    *******************************************************/
/*****************************************************************************/

// static unsigned char UpdateEDIDRAM(_CODE unsigned char *pEDID,unsigned char BlockNUM)
unsigned char
UpdateEDIDRAM (
    unsigned char   * pEDID,
    unsigned char   BlockNUM)
{
    unsigned char i, offset, sum = 0;

    if (BlockNUM == 0x02)
    {
        offset = 0x00 + 128 * 0x01;
    }
    else
    {
        offset = 0x00 + 128 * BlockNUM;
    }

    EDID_DEBUG_PRINTF(("block No =%02X offset = %02X\n", (int) BlockNUM, (int) offset));

    for (i = 0; i < 0x7F; i++)
    {
        EDID_RAM_Write(offset, 1, (pEDID + offset));

        EDID_DEBUG_PRINTF(("%02X ", (int) *(pEDID + offset)));
        sum += *(pEDID + offset);
        offset++;
//      pEDID++;

        if ((i % 16) == 15)
        {
            EDID_DEBUG_PRINTF(("\n"));
        }
    }
    sum = 0x00 - sum;
//    EDID_RAM_Write(offset,1,&sum);
    return sum;
}

void
EnableEDIDupdata (
    void)
{
    EDID_DEBUG_PRINTF(("EnableEDIDupdata()\n"));

//  HotPlug(0); //clear port 1 HPD=0 for Enable EDID update

//  chgbank(1);
//  hdmirxset(REG_RX_1B0, 0x03, 0x01); //clear port 0 HPD=1 for EDID update
//  chgbank(0);

    IT6602HPDCtrl(0, 0);    // HDMI port 0, set HPD = 0
    IT6602HPDCtrl(1, 0);    // HDMI port 1, set HPD = 0
}

void
DisableEDIDupdata (
    void)
{
    EDID_DEBUG_PRINTF(("DisableEDIDupdata()\n"));

// xxxxx 2013-0801 disable HPD Control
    #if 0
    HotPlug(1);                         // set port 1 HPD=1

    chgbank(1);
    hdmirxset(REG_RX_1B0, 0x03, 0x03);  // set port 0 HPD=1
    chgbank(0);
    #endif
// xxxxx
}

// static void EDIDRAMInitial(_CODE unsigned char *pIT6602EDID)
void
EDIDRAMInitial (
    unsigned char * pIT6602EDID)
{
    unsigned char   Block0_CheckSum;
    unsigned char   Block1_CheckSum;
    unsigned char   u8_VSDB_Addr;
    unsigned char   BlockNo;

    u8_VSDB_Addr = 0;

    EnableEDIDupdata();

    for (BlockNo = 0; BlockNo < 2; BlockNo++)
    {
        EDID_DEBUG_PRINTF(("IT6602 EDIDRAMInitial = %02X\n", (int) BlockNo));

        if (BlockNo == 0)
        {
            Block0_CheckSum = UpdateEDIDRAM(pIT6602EDID, 0);
            hdmirxwr(REG_RX_0C4, Block0_CheckSum);          // Port 0 Bank 0 CheckSum
            hdmirxwr(REG_RX_0C8, Block0_CheckSum);          // Port 1 Bank 0 CheckSum

            EDID_DEBUG_PRINTF((" Block0_CheckSum = %02X\n", (int) Block0_CheckSum));
        }
        else
        {
            Block1_CheckSum = UpdateEDIDRAM(pIT6602EDID, 1);
            EDID_DEBUG_PRINTF((" Block1_CheckSum = %02X\n", (int) Block1_CheckSum));
            u8_VSDB_Addr    = Find_Phyaddress_Location(pIT6602EDID, 1);

            EDID_DEBUG_PRINTF(("u8_VSDB_Addr = %02X\n", (int) u8_VSDB_Addr));
            PhyAdrSet();

            if (u8_VSDB_Addr != 0)
            {
                UpdateEDIDReg(u8_VSDB_Addr, pIT6602EDID[u8_VSDB_Addr], pIT6602EDID[u8_VSDB_Addr + 1], Block1_CheckSum);
                EDID_DEBUG_PRINTF(("EDID Parsing OK\n"));
            }
        }
    }

    DisableEDIDupdata();
}

// static unsigned char Find_Phyaddress_Location(_CODE unsigned char *pEDID,unsigned char Block_Number)
unsigned char
Find_Phyaddress_Location (
    unsigned char   * pEDID,
    unsigned char   Block_Number)
{
    unsigned char   AddStart;
    unsigned char   tag, count;
    unsigned char   offset, End;
    unsigned char   u8_VSDB_Addr;

    if (Block_Number == 0x02)
    {
        AddStart = 0x00 + 128 * 0x01;
    }
    else
    {
        AddStart = 0x00 + 128 * Block_Number;
    }

    if (((*(pEDID + AddStart)) != 0x2) || ((*(pEDID + AddStart + 1)) != 0x3))
    {
        return 0;
    }
    End             = (*(pEDID + AddStart + 2));
    u8_VSDB_Addr    = 0;

    for (offset = (AddStart + 0x04); offset < (AddStart + End); )
    {
        tag     = (*(pEDID + offset)) >> 5;
        count   = (*(pEDID + offset)) & 0x1f;

        // #ifdef printf_EDID
        EDID_DEBUG_PRINTF(("offset = %X , Tag = %X , count =%X\n", (int) offset, (int)  tag, (int) count));
        // #endif

        offset++;
        if (tag == 0x03) // HDMI VSDB Block of EDID
        {
            // #ifdef printf_EDID
            EDID_DEBUG_PRINTF(("HDMI VSDB Block address = %X\n", (int)  offset));
            // #endif

            if (((*(pEDID + offset  )) == 0x03) &&
                ((*(pEDID + offset + 1)) == 0x0C) &&
                ((*(pEDID + offset + 2)) == 0x0))
            {
                u8_VSDB_Addr    = offset + 3;
                txphyadr[0]     = (*(pEDID + offset + 3));
                txphyadr[1]     = (*(pEDID + offset + 4));
                // #ifdef printf_EDID
                EDID_DEBUG_PRINTF(("txphyadr[0] = %X\n", (int)  txphyadr[0]));
                EDID_DEBUG_PRINTF(("txphyadr[1] = %X\n", (int)  txphyadr[1]));
                // #endif

                return u8_VSDB_Addr;
            }
        }

        offset = offset + count;
    }
    return 0;
}

void
UpdateEDIDReg (
    unsigned char   u8_VSDB_Addr,
    unsigned char   CEC_AB,
    unsigned char   CEC_CD,
    unsigned char   Block1_CheckSum)
{
    unsigned char   A_Addr_AB, A_Addr_CD, A_Block1_CheckSum;
    unsigned char   B_Addr_AB, B_Addr_CD, B_Block1_CheckSum;

    A_Addr_AB           = rxphyadr[0][0];
    A_Addr_CD           = rxphyadr[0][1];

    B_Addr_AB           = rxphyadr[1][0];
    B_Addr_CD           = rxphyadr[1][1];

    A_Block1_CheckSum   = (Block1_CheckSum + CEC_AB + CEC_CD - A_Addr_AB - A_Addr_CD) % 0x100;
    B_Block1_CheckSum   = (Block1_CheckSum + CEC_AB + CEC_CD - B_Addr_AB - B_Addr_CD) % 0x100;

    hdmirxwr(REG_RX_0C1, u8_VSDB_Addr);             // VSDB Start Address
    hdmirxwr(REG_RX_0C2, A_Addr_AB);                // Port 0 AB
    hdmirxwr(REG_RX_0C3, A_Addr_CD);                // Port 0 CD
    hdmirxwr(REG_RX_0C5, A_Block1_CheckSum);        // Port 0 Bank 1 CheckSum

    hdmirxwr(REG_RX_0C6, B_Addr_AB);                // Port 1 AB
    hdmirxwr(REG_RX_0C7, B_Addr_CD);                // Port 1 CD
    hdmirxwr(REG_RX_0C9, B_Block1_CheckSum);        // Port 1 Bank 1 CheckSum
}

void
PhyAdrSet (
    void)
{
// #ifdef Enable_IT6602_CEC
//  if(Myself_LogicAdr==DEVICE_ID_TV)
// #endif
    {
        rxphyadr[0][0]  = 0x10;
        rxphyadr[0][1]  = 0x00;
        rxphyadr[1][0]  = 0x20;
        rxphyadr[1][1]  = 0x00;
    }
}

#endif

// FIX_ID_014 xxxxx
void
IT6602HDMIEventManager (
    struct IT6602_dev_data * IT6602)
{
    if (IT6602->HDMIIntEvent != 0)
    {
// ============================================================================

        if ((IT6602->HDMIIntEvent & B_PORT0_Waiting) == B_PORT0_Waiting)
        {
            if (IT6602->HDMIWaitNo[0] == 0)
            {
                IT6602->HDMIIntEvent &= ~(B_PORT0_Waiting);
                IT6602_DEBUG_INT_PRINTF(("B_PORT0_Waiting  OK ...\n"));
            }
            else
            {
                IT6602->HDMIWaitNo[0]--;
                IT6602_DEBUG_INT_PRINTF(("B_PORT0_Waiting  %X ...Event=%X ...Reg93=%X\n",
                    (int) IT6602->HDMIWaitNo[0], (int) IT6602->HDMIIntEvent, (int) hdmirxrd(0x93)));
            }
        }
        else
        {
            if ((IT6602->HDMIIntEvent & B_PORT0_TMDSEvent) == B_PORT0_TMDSEvent)
            {
                if (CLKCheck(F_PORT_SEL_0))
                {
                    IT6602_DEBUG_INT_PRINTF(("TMDSEvent &&&&& Port 0 Rx CKOn Detect &&&&&\n"));
#ifdef _SUPPORT_AUTO_EQ_
                    TMDSCheck(F_PORT_SEL_0);
#else
                    // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_EQ_ADJUST_
                    HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_0]));
    #endif
                    // FIX_ID_001 xxxxx
#endif
                    IT6602->HDMIIntEvent &= ~(B_PORT0_TMDSEvent);       // finish MSC
                }
            }
            else if ((IT6602->HDMIIntEvent & B_PORT0_TimingChgEvent) == B_PORT0_TimingChgEvent)
            {
                if (CLKCheck(F_PORT_SEL_0))
                {
                    IT6602_DEBUG_INT_PRINTF(("TimingChgEvent &&&&& Port 0 Rx CKOn Detect &&&&&\n"));
                    // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
#ifdef _SUPPORT_EQ_ADJUST_
                    HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_0]));
#endif
                    // FIX_ID_001 xxxxx

                    IT6602->HDMIIntEvent &= ~(B_PORT0_TimingChgEvent);      // finish MSC
                }
            }
        }
// ============================================================================

// ============================================================================

        if ((IT6602->HDMIIntEvent & B_PORT1_Waiting) == B_PORT1_Waiting)
        {
            if (IT6602->HDMIWaitNo[1] == 0)
            {
                IT6602->HDMIIntEvent &= ~(B_PORT1_Waiting);
                IT6602_DEBUG_INT_PRINTF(("B_PORT1_Waiting  OK ...\n"));
            }
            else
            {
                IT6602->HDMIWaitNo[1]--;
                IT6602_DEBUG_INT_PRINTF(("B_PORT1_Waiting  %X ...\n", (int) IT6602->HDMIWaitNo[1]));
            }
        }
        else
        {
            if ((IT6602->HDMIIntEvent & B_PORT1_TMDSEvent) == B_PORT1_TMDSEvent)
            {
                if (CLKCheck(F_PORT_SEL_1))
                {
                    IT6602_DEBUG_INT_PRINTF(("TMDSEvent &&&&& Port 1 Rx CKOn Detect &&&&&\n"));
#ifdef _SUPPORT_AUTO_EQ_
                    TMDSCheck(F_PORT_SEL_1);
#else
                    // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_EQ_ADJUST_
                    HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_1]));
    #endif
                    // FIX_ID_001 xxxxx
#endif
                    IT6602->HDMIIntEvent &= ~(B_PORT1_TMDSEvent);       // finish MSC
                }
            }
            else if ((IT6602->HDMIIntEvent & B_PORT1_TimingChgEvent) == B_PORT1_TimingChgEvent)
            {
                if (CLKCheck(F_PORT_SEL_1))
                {
                    IT6602_DEBUG_INT_PRINTF(("TimingChgEvent &&&&& Port 1 Rx CKOn Detect &&&&&\n"));
#ifdef _SUPPORT_EQ_ADJUST_
                    HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_1]));
#endif
                    IT6602->HDMIIntEvent &= ~(B_PORT1_TimingChgEvent);      // finish MSC
                }
            }
        }
// ============================================================================
// ============================================================================
// ============================================================================
    }
}

// FIX_ID_009 xxxxx //verify interrupt event with reg51[0] select port
unsigned char
IT6602_IsSelectedPort (
    unsigned char ucPortSel)
{
    unsigned char           ucCurrentPort;

    struct IT6602_dev_data  * IT6602data = get_IT6602_dev_data();

    ucCurrentPort = hdmirxrd(REG_RX_051) & B_PORT_SEL;

    if (ucCurrentPort == ucPortSel)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
// FIX_ID_009 xxxxx

/*****************************************************************************/
/* Driver State Machine Process **********************************************/
/*****************************************************************************/
#ifdef _it660x_

void
IT6602HDMIInterruptHandler (
    struct IT6602_dev_data * IT6602)
{
    unsigned char   Reg05h;
    unsigned char   Reg06h;
    unsigned char   Reg07h;
    unsigned char   Reg08h;
    unsigned char   Reg09h;
    unsigned char   Reg0Ah;
//  unsigned char Reg0Bh;
    unsigned char   RegD0h;

    if (IT6602->m_VState == VSTATE_Off)
    {
        return;  // if VSTATE is off, do not proecess any event.
    }
    chgbank(0);

    Reg05h  = hdmirxrd(REG_RX_005);
    Reg06h  = hdmirxrd(REG_RX_006);
    Reg07h  = hdmirxrd(REG_RX_007);
    Reg08h  = hdmirxrd(REG_RX_008);
    Reg09h  = hdmirxrd(REG_RX_009);

    Reg0Ah  = hdmirxrd(REG_RX_P0_SYS_STATUS);
//  Reg0Bh = hdmirxrd(REG_RX_P1_SYS_STATUS);
    RegD0h  = hdmirxrd(REG_RX_0D0);

    hdmirxwr(REG_RX_005, Reg05h);
    hdmirxwr(REG_RX_006, Reg06h);
    hdmirxwr(REG_RX_007, Reg07h);
    hdmirxwr(REG_RX_008, Reg08h);
    hdmirxwr(REG_RX_009, Reg09h);
// 2013-0606 disable ==>
    hdmirxwr(REG_RX_0D0, RegD0h & 0x0F);

//  IT6602_DEBUG_INT_PRINTF(("111111111111111111111111 STATUS 111111111111111111111= %X\n",hdmirxrd(REG_RX_P0_SYS_STATUS)));
    if (Reg05h != 0x00)
    {
        IT6602_DEBUG_INT_PRINTF(("Reg05 = %X\n", (int) Reg05h));

        if (Reg05h & 0x80)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 0 HDCP Off Detected ###\n"));
            IT6602->m_ucEccCount_P0 = 0;
        }

        if (Reg05h & 0x40)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 0 ECC Error %X ####\n", (int) (IT6602->m_ucEccCount_P0)));
//          HDMICheckErrorCount(&(IT6602->EQPort[F_PORT_SEL_0]));   //07-04 for port 0
            hdmirx_INT_P0_ECC(IT6602);
        }

        if (Reg05h & 0x20)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 0 HDMI/DVI Mode change ####\n"));
// FIX_ID_009 xxxxx //verify interrupt event with reg51[0] select port
            if (CLKCheck(0))
            {
                hdmirx_INT_HDMIMode_Chg(IT6602, 0);
            }
// FIX_ID_009 xxxxx
        }

        if (Reg05h & 0x08)
        {
            IT6602_DEBUG_INT_PRINTF3(("#### Port 0 HDCP Authentication Start ####\n"));

            IT6602->m_ucEccCount_P0 = 0;
    #ifdef _SUPPORT_HDMI_REPEATER_
            hdmirxset(REG_RX_P0_BCAPS, B_KSV_READY, 0);     // Clear KSV FIFO Ready.
            if (IT6602->m_ucCurrentHDMIPort == 0)
            {
                it660x_DownStream_AuthStart(IT6602);
            }
    #endif

    #ifdef _SUPPORT_AUTO_EQ_
            // FIX_ID_014 xxxxx
            if (ucPortAMPOverWrite[F_PORT_SEL_0] == 0)
            {
                if ((IT6602->HDMIIntEvent & (B_PORT0_Waiting)) == 0)
                {
                    hdmirxwr(REG_RX_022, 0x00);     // power down auto EQ

                    IT6602->HDMIIntEvent    |= (B_PORT0_Waiting);
                    IT6602->HDMIIntEvent    |= (B_PORT0_TMDSEvent);
                    IT6602->HDMIWaitNo[0]   = MAX_TMDS_WAITNO;
                }
                else if ((IT6602->HDMIIntEvent & (B_PORT0_TMDSEvent)))
                {
                    IT6602->HDMIIntEvent    |= (B_PORT0_Waiting);
                    IT6602->HDMIWaitNo[0]   += MAX_HDCP_WAITNO;
                }
            }
            else
            {
                if ((IT6602->HDMIIntEvent & (B_PORT0_TMDSEvent)))
                {
                    IT6602->HDMIIntEvent    |= (B_PORT0_Waiting);
                    IT6602->HDMIWaitNo[0]   += MAX_HDCP_WAITNO;
                }
            }
            // FIX_ID_014 xxxxx
    #endif
        }

        if (Reg05h & 0x10)
        {
            IT6602setHDCPStatus(1);                     // [2020 03 19 bu5 modify]
            IT6602_DEBUG_INT_PRINTF3(("#### Port 0 HDCP Authentication Done ####\n"));

    #ifdef _SUPPORT_HDMI_REPEATER_
            // FIX_ID_032 xxxxx //Support HDCP Repeater function for HDMI Tx device
        #ifdef _SUPPORT_HDMI_REPEATER_
            hdmirxset(REG_RX_P0_BCAPS, B_KSV_READY, 0); // Clear KSV FIFO Ready.
            if (IT6602->m_ucCurrentHDMIPort == 0)
            {
                it660x_DownStream_AuthDone(IT6602);
            }
        #endif

        #ifdef _SUPPORT_AUTO_EQ_
            // FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
            if (ucPortAMPOverWrite[0] == 0)         // 2013-0801
            {
                IT6602->HDMIIntEvent    &= ~(B_PORT0_Waiting);
                IT6602->HDMIWaitNo[0]   = 0;
                IT6602->HDMIIntEvent    |= B_PORT0_TMDSEvent;
                // return;
            }
            // FIX_ID_033 xxxxx
        #endif

    #else

        #ifdef _SUPPORT_AUTO_EQ_
            // FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
            // FIX_ID_032 xxxxx //Support HDCP Repeater function for HDMI Tx device
            // xxxxx 2014-0421
            // FIX_ID_014 xxxxx
            if (ucPortAMPOverWrite[0] == 0)         // 2013-0801
            {
                IT6602->HDMIIntEvent    &= ~(B_PORT0_Waiting);
                IT6602->HDMIWaitNo[0]   = 0;
                IT6602->HDMIIntEvent    |= B_PORT0_TMDSEvent;
                // return;
            }
            // FIX_ID_014 xxxxx
        #endif

            // FIX_ID_032 xxxxx
    #endif
        }

        if (Reg05h & 0x04)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 0 Input Clock Change Detect ####\n"));
            if ((hdmirxrd(0x0C) & 0x40) == 0x00)// P0 clock not stable  ite180509
            {
                chgbank(1);
                hdmirxset(0xB1, 0x60, 0x00);
                chgbank(0);
                CLKStbChkCnt = 0;
            }
        }

        if (Reg05h & 0x02)
        {
            IT6602->m_ucEccCount_P0 = 0;
            IT6602->m_ucDeskew_P0   = 0;
            // IT6602->m_ucDeskew_P1=0;
            // IT6602->m_ucEccCount_P1=0;

            IT6602_DEBUG_INT_PRINTF(("#### Port 0 Rx CKOn Detect ####\n"));

// #ifdef _SUPPORT_HDCP_REPEATER_
    #if 1
// FIX_ID_032 xxxxx //Support HDCP Repeater function for HDMI Tx device
            if (CLKCheck(F_PORT_SEL_0))
            {
        #ifdef _SUPPORT_AUTO_EQ_
                TMDSCheck(F_PORT_SEL_0);
        #else
                // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
            #ifdef _SUPPORT_EQ_ADJUST_
                HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_0]));
            #endif
                // FIX_ID_001 xxxxx
        #endif
            }
    #else

            // NO --> Authentication Start  &&  Input Clock Change Detect   &&   B_PORT1_TMDSEvent
            if ((( Reg05h & 0x08 ) == 0) && (( Reg05h & 0x04 ) == 0) && ((IT6602->HDMIIntEvent & (B_PORT0_TMDSEvent)) == 0))
            {
                if (CLKCheck(F_PORT_SEL_0))
                {
        #ifdef _SUPPORT_AUTO_EQ_
                    TMDSCheck(F_PORT_SEL_0);
        #endif
                }
            }
            else
            {
                if (( Reg05h & 0x10 ) == 0)
                {
                    if ((IT6602->HDMIIntEvent & (B_PORT0_Waiting)) == 0)
                    {
                        hdmirxwr(REG_RX_022, 0x00); // power down auto EQ

                        // FIX_ID_019 xxxxx modify ENHYS control for MHL mode
                        if (hdmirxrd(REG_RX_P0_SYS_STATUS) & B_P0_RXCK_VALID)
                        {
                            IT6602->HDMIIntEvent    |= (B_PORT0_Waiting);
                            IT6602->HDMIIntEvent    |= (B_PORT0_TMDSEvent);
                            IT6602->HDMIWaitNo[0]   = MAX_TMDS_WAITNO;
                        }
                        // FIX_ID_019 xxxxx
                    }
                }
                else
                {
                    if (CLKCheck(F_PORT_SEL_0))
                    {
        #ifdef _SUPPORT_AUTO_EQ_
                        TMDSCheck(F_PORT_SEL_0);
        #endif
                    }
                }
            }
// FIX_ID_032 xxxxx
    #endif
        }

        if (Reg05h & 0x01)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 0 Power 5V change ####\n"));
            IT6602setHDCPStatus(0);// [2020 03 19 bu5 modify]
            hdmirx_INT_5V_Pwr_Chg(IT6602, 0);
        }
    }

    if (Reg06h != 0x00)
    {
        if (Reg06h & 0x80)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 HDCP Off Detected ###\n"));
            IT6602setHDCPStatus(0);// [2020 03 19 bu5 modify]
            IT6602->m_ucEccCount_P1 = 0;
        }

        if (Reg06h & 0x40)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 ECC Error ####\n"));
            hdmirx_INT_P1_ECC(IT6602);
        }

        if (Reg06h & 0x20)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 HDMI/DVI Mode change ####\n"));
// FIX_ID_009 xxxxx //verify interrupt event with reg51[0] select port
            if (CLKCheck(1))
            {
                hdmirx_INT_HDMIMode_Chg(IT6602, 1);
            }
// FIX_ID_009 xxxxx
        }

        if (Reg06h & 0x08)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 HDCP Authentication Start ####\n"));
            IT6602->m_ucEccCount_P1 = 0;

    #ifdef _SUPPORT_AUTO_EQ_
// FIX_ID_014 xxxxx
            if (ucPortAMPOverWrite[F_PORT_SEL_1] == 0)
            {
                if ((IT6602->HDMIIntEvent & (B_PORT1_Waiting)) == 0)
                {
                    IT6602_DEBUG_INT_PRINTF((" power down auto EQ of PORT 1\n"));
                    hdmirxwr(REG_RX_03A, 0x00); // power down auto EQ

                    IT6602->HDMIIntEvent    |= (B_PORT1_Waiting);
                    IT6602->HDMIIntEvent    |= (B_PORT1_TMDSEvent);
                    IT6602->HDMIWaitNo[1]   = MAX_TMDS_WAITNO;
                }
                else if ((IT6602->HDMIIntEvent & (B_PORT1_TMDSEvent)))
                {
                    IT6602->HDMIIntEvent    |= (B_PORT1_Waiting);
                    IT6602->HDMIWaitNo[1]   += MAX_HDCP_WAITNO;
                }
            }
            else
            {
                if ((IT6602->HDMIIntEvent & (B_PORT1_TMDSEvent)))
                {
                    IT6602->HDMIIntEvent    |= (B_PORT1_Waiting);
                    IT6602->HDMIWaitNo[1]   += MAX_HDCP_WAITNO;
                }
            }
// FIX_ID_014 xxxxx
    #endif
    #ifdef _SUPPORT_HDMI_REPEATER_
            hdmirxset(REG_RX_P1_BCAPS, B_KSV_READY, 0);     // Clear KSV FIFO Ready.
            if (IT6602->m_ucCurrentHDMIPort == 1)
            {
                it660x_DownStream_AuthStart(IT6602);
            }
    #endif
        }

        if (Reg06h & 0x10)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 HDCP Authentication Done ####\n"));
// FIX_ID_014 xxxxx
            if ((IT6602->HDMIIntEvent & (B_PORT1_Waiting)))
            {
                IT6602->HDMIWaitNo[1] = 0;
            }
// FIX_ID_014 xxxxx

    #ifdef _SUPPORT_AUTO_EQ_
// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
// disable ->   if( ucPortAMPValid[F_PORT_SEL_1] == 0)
            if (ucPortAMPOverWrite[1] == 0)    // 2013-0801
            {
                IT6602->HDMIIntEvent    &= ~(B_PORT1_Waiting);
                IT6602->HDMIWaitNo[1]   = 0;
                IT6602->HDMIIntEvent    |= B_PORT1_TMDSEvent;
            }
// FIX_ID_033 xxxxx
    #endif

    #ifdef _SUPPORT_HDMI_REPEATER_
            hdmirxset(REG_RX_P1_BCAPS, B_KSV_READY, 0);     // Clear KSV FIFO Ready.
            if (IT6602->m_ucCurrentHDMIPort == 1)
            {
                it660x_DownStream_AuthDone(IT6602);
            }
    #endif
        }

        if (Reg06h & 0x04)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 Input Clock Change Detect ####\n"));
            if ((hdmirxrd(0x0C) & 0x80) == 0x00)// P1 clock not stable  ite180509
            {
                chgbank(1);
                hdmirxset(0xC1, 0x60, 0x00);
                chgbank(0);
                CLKStbChkCnt = 0;
            }
        }

        if (Reg06h & 0x02)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 Rx CKOn Detect ####\n"));
            // IT6602->m_ucEccCount_P0=0;
            // IT6602->m_ucDeskew_P0=0;
            IT6602->m_ucDeskew_P1   = 0;
            IT6602->m_ucEccCount_P1 = 0;

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
// FIX_ID_032 xxxxx //Support HDCP Repeater function for HDMI Tx device
    #if 0

            if (CLKCheck(F_PORT_SEL_1))
            {
                TMDSCheck(F_PORT_SEL_1);
            }
    #else
            // NO --> Authentication Start  &&  Input Clock Change Detect   &&   B_PORT1_TMDSEvent
            if ((( Reg06h & 0x08 ) == 0) && (( Reg06h & 0x04 ) == 0) && ((IT6602->HDMIIntEvent & (B_PORT1_TMDSEvent)) == 0))
            {
                if (CLKCheck(F_PORT_SEL_1))
                {
        #ifdef _SUPPORT_AUTO_EQ_
                    TMDSCheck(F_PORT_SEL_1);
        #endif
                }
            }
            else
            {
                if (( Reg06h & 0x10 ) == 0)
                {
                    if ((IT6602->HDMIIntEvent & (B_PORT1_Waiting)) == 0)
                    {
                        hdmirxwr(REG_RX_03A, 0x00); // power down auto EQ
                        IT6602->HDMIIntEvent    |= (B_PORT1_Waiting);
                        IT6602->HDMIIntEvent    |= (B_PORT1_TMDSEvent);
                        IT6602->HDMIWaitNo[1]   = MAX_TMDS_WAITNO;
                    }
                }
                else
                {
                    if (CLKCheck(F_PORT_SEL_1))
                    {
        #ifdef _SUPPORT_AUTO_EQ_
                        TMDSCheck(F_PORT_SEL_1);
        #else
                        // FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
            #ifdef _SUPPORT_EQ_ADJUST_
                        HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_1]));
            #endif
                        // FIX_ID_001 xxxxx
        #endif
                    }
                }
            }
    #endif
// FIX_ID_032 xxxxx
// FIX_ID_033 xxxxx
        }

        if (Reg06h & 0x01)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 Power 5V change ####\n"));
            hdmirx_INT_5V_Pwr_Chg(IT6602, 1);
            // disable ->//FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
            // disable ->if(CheckPlg5VPwr(F_PORT_SEL_1)==FALSE)
            // disable ->{
            // disable ->   #ifdef _SUPPORT_EQ_ADJUST_
            // disable ->   DisableOverWriteRS(F_PORT_SEL_1);
            // disable ->   #endif
            // disable ->}
            // disable ->//FIX_ID_001 xxxxx
        }
    }

    if (Reg07h != 0x00)
    {
        if (Reg07h & 0x80)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Audio FIFO Error ####\n"));
            aud_fiforst();
    #ifdef EnableCalFs
// FIX_ID_023 xxxxx     //Fixed for Audio Channel Status Error with invalid HDMI source
            m_AudioChannelStatusErrorCount++;
            AudioFsCal();
// FIX_ID_023 xxxxx
    #endif
        }

        if (Reg07h & 0x40)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Audio Auto Mute ####\n"));
        }

        if (Reg07h & 0x20)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Packet Left Mute ####\n"));
            IT6602_SetVideoMute(IT6602, OFF);
    #ifdef _SUPPORT_HDMI_REPEATER_
            it660x_DownStream_AVMuteSet(IT6602, FALSE);
    #endif
        }

        if (Reg07h & 0x10)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Set Mute Packet Received ####\n"));
    #ifdef _SUPPORT_HDMI_REPEATER_
            it660x_DownStream_AVMuteSet(IT6602, TRUE);
    #endif
            IT6602_SetVideoMute(IT6602, ON);
        }

        if (Reg07h & 0x08)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Timer Counter Tntterrupt ####\n"));
            // if(IT6602->m_VState == VSTATE_VideoOn)
            //  hdmirxset(0x84,0x80,0x80);  //2011/06/17 xxxxx, for enable Rx Chip count
        }

        if (Reg07h & 0x04)
        {
            IT6602_DEBUG_INT_PRINTF(("#### Video Mode Changed ####\n"));
        }

        if (Reg07h & 0x02)
        {
            hdmirx_INT_SCDT_Chg(IT6602);
        }

        if (Reg07h & 0x01)
        {
            if ((Reg0Ah & 0x40) >> 6)
            {
                IT6602_DEBUG_INT_PRINTF(("#### Port 0 Bus Mode : Non-HDMI ####\n"));
            }
            else
            {
                IT6602_DEBUG_INT_PRINTF(("#### Port 0 Bus Mode : HDMI ####\n"));
                // FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
                if (HdmiI2cAddr == IT6602A0_HDMI_ADDR)
                {
                    chgbank(1);
                    hdmirxset(REG_RX_1B6, 0x07, 0x03);
                    ////FIX_ID_007 xxxxx    //for debug IT6681  HDCP issue
                    hdmirxset(REG_RX_1B1, 0x20, 0x00);  // Reg1b1[5] = 0 for disable over-write
                    hdmirxset(REG_RX_1B2, 0x07, 0x04);  // default 0x04 , change to 0x01
                    EQ_PORT0_PRINTF((" Port 0 Bus Mode Reg1B1  = %X ,Reg1B2  = %X\n", (int) hdmirxrd(REG_RX_1B1), (int) hdmirxrd(REG_RX_1B2)));
                    ////FIX_ID_007 xxxxx
                    chgbank(0);
                }
                // FIX_ID_002 xxxxx
            }
        }
    }

    if (Reg08h != 0x00)
    {
        if (Reg08h & 0x20)
        {
            IT6602_DEBUG_INT_PRINTF(("#### No Audio InfoFrame Received ####\n"));
        }

        if (Reg08h & 0x10)
        {
            IT6602_DEBUG_INT_PRINTF(("#### No AVI InfoFrame Received ####\n"));
        }

        if (Reg08h & 0x08)
        {
            IT6602_DEBUG_INT_PRINTF(("#### CD Detect ####\n"));
        }

        if (Reg08h & 0x04)
        {
            IT6602_DEBUG_INT_PRINTF(("#### 3D InfoFrame Detect ####\n"));

    #ifdef Enable_Vendor_Specific_packet
            if (IT6602->f_de3dframe_hdmi == FALSE)
            {
                IT6602->f_de3dframe_hdmi = IT6602_DE3DFrame(TRUE);
            }
    #endif
        }

        if (Reg08h & 0x02)
        {
            IT6602_DEBUG_INT_PRINTF(("#### ISRC2 Detect ####\n"));
        }

        if (Reg08h & 0x01)
        {
            IT6602_DEBUG_INT_PRINTF(("#### ISRC1 Detect ####\n"));
        }
    }

    if (Reg09h != 0x00)
    {
        if (Reg09h & 0x80)
        {
            IT6602_DEBUG_INT_PRINTF(("#### H2V Buffer Skew Fail ####\n"));
        }

        if (Reg09h & 0x40)
        {
            // FIX_ID_002 xxxxx     Check IT6602 chip version Identify for TogglePolarity and Port 1 Deskew
            if (HdmiI2cAddr == IT6602A0_HDMI_ADDR)
            {
                hdmirxwr(0x09, 0x20); // bug ~ need to update by Andrew
            }
            else
            {
                hdmirxwr(0x09, 0x40);
            }
            // FIX_ID_002 xxxxx
            IT6602_DEBUG_INT_PRINTF(("#### Port 1 Deskew Error ####\n"));
            hdmirx_INT_P1_Deskew(IT6602);
        }

        if (Reg09h & 0x20)
        {
            hdmirxwr(0x09, 0x20);
            IT6602_DEBUG_INT_PRINTF(("#### Port 0 Deskew Error ####\n"));
            hdmirx_INT_P0_Deskew(IT6602);
        }

        if (Reg09h & 0x10)
        {
            IT6602_DEBUG_INT_PRINTF(("#### New Audio Packet Received ####\n"));
        }

        if (Reg09h & 0x08)
        {
            IT6602_DEBUG_INT_PRINTF(("#### New ACP Packet Received ####\n"));
        }

        if (Reg09h & 0x04)
        {
            IT6602_DEBUG_INT_PRINTF(("#### New SPD Packet Received ####\n"));
        }

        if (Reg09h & 0x02)
        {
            IT6602_DEBUG_INT_PRINTF(("#### New MPEG InfoFrame Received ####\n"));
        }

        if (Reg09h & 0x01)
        {
            unsigned char AVIDB[2];

            IT6602_DEBUG_INT_PRINTF(("#### New AVI InfoFrame Received ####\n"));
            // IT6602VideoOutputConfigure();
            chgbank(2);
            AVIDB[0]    = hdmirxrd(REG_RX_AVI_DB0);
            AVIDB[1]    = hdmirxrd(REG_RX_AVI_DB1);
            chgbank(0);
            if ((AVIDB[0] != IT6602->prevAVIDB[0]) ||
            (AVIDB[1] != IT6602->prevAVIDB[1]))
            {
                IT6602->m_NewAVIInfoFrameF = TRUE;
            }
            IT6602->prevAVIDB[0]    = AVIDB[0];
            IT6602->prevAVIDB[1]    = AVIDB[1];
        }
    }

    if (RegD0h != 0x00)
    {
// disable      if( RegD0h&0x08)
// disable      {
// disable          EQ_DEBUG_PRINTF(("#### Port 1 Rx Clock change detect Interrupt ####\n"));
// disable      }
// disable
// disable      if( RegD0h&0x04)
// disable      {
// disable          EQ_DEBUG_PRINTF(("#### Port 0 Rx Clock change detect Interrupt ####\n"));
// disable      }
        if (RegD0h & 0x10)
        {
            hdmirxwr(0xD0, 0x30);
            RegD0h &= 0x30;

            EQ_PORT0_PRINTF(("#### Port 0 EQ done interrupt ####\n"));

    #ifdef _SUPPORT_AUTO_EQ_
            // 2013-0923 disable -> ucPortAMPOverWrite[0]=1;    //2013-0801
            AmpValidCheck(0); // 2013-0801
    #endif

// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_EQ_ADJUST_
            HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_0]));
    #endif
// FIX_ID_001 xxxxx
        }

        if (RegD0h & 0x40)
        {
            hdmirxwr(0xD0, 0xC0);
            RegD0h &= 0xC0;
            EQ_PORT1_PRINTF(("#### Port 1 EQ done interrupt ####\n"));

    #ifdef _SUPPORT_AUTO_EQ_
            // 2013-0923 disable -> ucPortAMPOverWrite[1]=1;    //2013-0801
            AmpValidCheck(1); // 2013-0801
    #endif

            // disable -> //FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
            // disable ->   #ifdef _SUPPORT_EQ_ADJUST_
            // disable ->   HDMIStartEQDetect(&(IT6602->EQPort[F_PORT_SEL_1]));
            // disable ->   #endif
            // disable -> //FIX_ID_001 xxxxx
        }

        if (RegD0h & 0x20)
        {
            hdmirxwr(0xD0, 0x20);
            EQ_PORT0_PRINTF(("#### Port 0 EQ Fail Interrupt ####\n"));
//  HDMICheckErrorCount(&(IT6602->EQPort[F_PORT_SEL_0]));   //07-04 for port 0
// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_AUTO_EQ_
            hdmirx_INT_EQ_FAIL(IT6602, F_PORT_SEL_0);
    #endif
// FIX_ID_001 xxxxx
        }

        if (RegD0h & 0x80)
        {
            hdmirxwr(0xD0, 0x80);
            EQ_PORT1_PRINTF(("#### Port 1 EQ Fail Interrupt ####\n"));
//  HDMICheckErrorCount(&(IT6602->EQPort[F_PORT_SEL_1]));   //07-04 for port 0
// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_AUTO_EQ_
            hdmirx_INT_EQ_FAIL(IT6602, F_PORT_SEL_1);
    #endif
// FIX_ID_001 xxxxx
        }
    }
}

void
IT6602_fsm (
    void)
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    #if defined(_IT6602_)
        #ifdef SUPPORT_UART_CMD
    if (m_UartCmd == 0)
        #endif
    {
        #ifndef Enable_IR
        IT6602AutoPortSelect(IT6602data);
        #endif
    }
    #endif

    IT6602HDMIInterruptHandler(IT6602data);
    IT6602VideoHandler(IT6602data);
    #ifndef _FIX_ID_028_
// FIX_ID_028 xxxxx //For Debug Audio error with S2
    IT6602AudioHandler(IT6602data);
// FIX_ID_028 xxxxx
    #endif

    #ifdef _SUPPORT_HDMI_REPEATER_
    it660x_DownStream_Polling(IT6602data);
    #endif

// FIX_ID_001 xxxxx Add Auto EQ with Manual EQ
    #ifdef _SUPPORT_EQ_ADJUST_
    if (IT6602data->EQPort[F_PORT_SEL_0].f_manualEQadjust == TRUE)
    {
        HDMIAdjustEQ(&(IT6602data->EQPort[F_PORT_SEL_0]));      // for port 0
    }
    if (IT6602data->EQPort[F_PORT_SEL_1].f_manualEQadjust == TRUE)
    {
        HDMIAdjustEQ(&(IT6602data->EQPort[F_PORT_SEL_1]));      // for port 1
    }
    #endif
// FIX_ID_001 xxxxx

// FIX_ID_033 xxxxx  //Fine-tune EQ Adjust function for HDCP receiver and repeater mode
// xxxxx 2014-0421 disable -> #ifndef _SUPPORT_HDCP_REPEATER_
// FIX_ID_032 xxxxx //Support HDCP Repeater function for HDMI Tx device
// FIX_ID_014 xxxxx //Add HDMI Event Handler
    IT6602HDMIEventManager(IT6602data);
// FIX_ID_014 xxxxx
// FIX_ID_032 xxxxx
// xxxxx 2014-0421 disable -> #endif
// FIX_ID_033 xxxxx

    #ifdef Enable_IT6602_CEC
    CECManager();               // detect CEC for IT6602_CEC
    #endif
}

    #if defined(_IT6602_)

void
IT6602AutoPortSelect (
    struct IT6602_dev_data * IT6602)
{
    if (SEL_PORT_1 == 1)
    {
        if (IT6602->m_ucCurrentHDMIPort != 0)
        {
            IT6602PortSelect(0);
        }
    }
    else
    {
        if (IT6602->m_ucCurrentHDMIPort == 0)
        {
            IT6602PortSelect(1);
        }
    }
}
    #endif

void
get_vid_info (
    void)
{
    #if 1
    int HSyncPol, VSyncPol, InterLaced;
    int HTotal, HActive, HFP, HSYNCW;
    int VTotal, VActive, VFP, VSYNCW;
//  int rddata;
//  int i;
//  unsigned long PCLK, sump;
    unsigned int    ucTMDSClk;  // , sumt;
    unsigned char   ucPortSel;
    unsigned char   rddata;
    unsigned char   ucClk;
    int             PCLK;       // , sump;

    rddata      = hdmirxrd(0x9A);
    PCLK        = (124 * 255 / rddata) / 10;

    ucPortSel   = hdmirxrd(REG_RX_051) & B_PORT_SEL;
    rddata      = hdmirxrd(0x90);

    if (ucPortSel == F_PORT_SEL_1)
    {
        VIDEOTIMNG_DEBUG_PRINTF(("Reg51[0] = 1 Active Port 1\n"));
        ucClk = hdmirxrd(REG_RX_092);
        if (ucClk != 0)
        {
            if (rddata & 0x04)
            {
                ucTMDSClk = 2 * RCLKVALUE * 256 / ucClk;
            }
            else if (rddata & 0x08)
            {
                ucTMDSClk = 4 * RCLKVALUE * 256 / ucClk;
            }
            else
            {
                ucTMDSClk = RCLKVALUE * 256 / ucClk;
            }

            VIDEOTIMNG_DEBUG_PRINTF((" Port 1 TMDS CLK  = %d\n", (int)ucTMDSClk));
        }
        // EQ_DEBUG_PRINTF((" HDMI Reg92  = %X\n",(int) hdmirxrd(0x92)));
        // EQ_DEBUG_PRINTF((" HDMI Reg38  = %X\n",(int) hdmirxrd(0x38)));
    }
    else
    {
        VIDEOTIMNG_DEBUG_PRINTF(("Reg51[0] = 0 Active Port 0\n"));
        ucClk = hdmirxrd(REG_RX_091);
        if (ucClk != 0)
        {
            if (rddata & 0x01)
            {
                ucTMDSClk = 2 * RCLKVALUE * 256 / ucClk;
            }
            else if (rddata & 0x02)
            {
                ucTMDSClk = 4 * RCLKVALUE * 256 / ucClk;
            }
            else
            {
                ucTMDSClk = RCLKVALUE * 256 / ucClk;
            }

            VIDEOTIMNG_DEBUG_PRINTF(("Port 0 TMDS CLK  = %d\n", (int)ucTMDSClk));
        }
        // EQ_DEBUG_PRINTF((" HDMI Reg91  = %X\n",(int) hdmirxrd(0x91)));
        // EQ_DEBUG_PRINTF((" HDMI Reg20  = %X\n",(int) hdmirxrd(0x20)));
    }

    InterLaced  = (hdmirxrd(0x99) & 0x02) >> 1;

    HTotal      = ((hdmirxrd(0x9D) & 0x3F) << 8) + hdmirxrd(0x9C);
    HActive     = ((hdmirxrd(0x9F) & 0x3F) << 8) + hdmirxrd(0x9E);
    HFP         = ((hdmirxrd(0xA1) & 0xF0) << 4) + hdmirxrd(0xA2);
    HSYNCW      = ((hdmirxrd(0xA1) & 0x01) << 8) + hdmirxrd(0xA0);
    HSyncPol    = hdmirxrd(0xA8) & 0x04 >> 2;

    VTotal      = ((hdmirxrd(0xA4) & 0x0F) << 8) + hdmirxrd(0xA3);
    VActive     = ((hdmirxrd(0xA4) & 0xF0) << 4) + hdmirxrd(0xA5);
    VFP         = hdmirxrd(0xA7) & 0x3F;
    VSYNCW      = hdmirxrd(0xA6) & 0x1F;
    VSyncPol    = (hdmirxrd(0xA8) & 0x08) >> 3;

//  CurVTiming.TMDSCLK     = (int)TMDSCLK;
    CurTMDSCLK              = (int)ucTMDSClk;
    CurVTiming.PCLK         = (int)PCLK;
    CurVTiming.HActive      = HActive;
    CurVTiming.HTotal       = HTotal;
    CurVTiming.HFrontPorch  = HFP;
    CurVTiming.HSyncWidth   = HSYNCW;
    CurVTiming.HBackPorch   = HTotal - HActive - HFP - HSYNCW;
    CurVTiming.VActive      = VActive;
    CurVTiming.VTotal       = VTotal;
    CurVTiming.VFrontPorch  = VFP;
    CurVTiming.VSyncWidth   = VSYNCW;
    CurVTiming.VBackPorch   = VTotal - VActive - VFP - VSYNCW;
    CurVTiming.ScanMode     = (InterLaced) & 0x01;
    CurVTiming.VPolarity    = (VSyncPol) & 0x01;
    CurVTiming.HPolarity    = (HSyncPol) & 0x01;
    #endif
}

void
IPLLOvw_chk (
    int port)
{
    unsigned char reg0, reg1, Reg4Eh, Regsts, IPLL_HS2, CLKStable, tmp;
    // emily add 20180508 add for SDI-HDMI source start

    // float CalTMDSCLK=0;
    reg0 = hdmirxrd(0x90);
    if (!port)
    {
        reg0    &= 0x03;
        reg1    = hdmirxrd(0x91);
        Regsts  = hdmirxrd(0x0A);
    }
    else// port1
    {
        reg0    = (reg0 & 0x0C) >> 2;
        reg1    = hdmirxrd(0x92);
        Regsts  = hdmirxrd(0x0B);
    }
    if (reg0 == 0)
    {
        tmp = reg1;
    }
    else
    {
        if (reg0 == 0x01)
        {
            tmp = reg1 >> 1;    // div2
        }
        else// 2,3
        {
            tmp = reg1 >> 2;    // div4
        }
    }
    Reg4Eh      = hdmirxrd(0x4E);
    IPLL_HS2    = (Reg4Eh & 0xC0) >> (6 + port);
    CLKStable   = (hdmirxrd(0x0C) & 0xC0) >> (6 + port);
    if (CLKStable)
    {
        it660x_DEBUG_PRINTF(("IPLL check %x \n", (int)CLKStbChkCnt));
        if (CLKStbChkCnt == 0x10)
        {
            if ((tmp <= 27) && (!IPLL_HS2)) // high mode
            {                               // OverWrite
                if ((Regsts & 0x80) == 0)
                {
                    chgbank(1);
                    // (void)printf("TMDSCLK = %3.3fMHz  :: ", (float)(CalTMDSCLK)/1000);
                    // (void)printf(" Regsts=%02X, Reg4Eh=%02X, reg0=%02X, reg1=%02X\n", (int)Regsts, (int)Reg4Eh, (int)reg0, (int)reg1);

                    it660x_DEBUG_PRINTF(("OverWrite IPLL/OPLL \n"));
                    if (!port)
                    {
                        hdmirxset(0xB2, 0x05, 0x04);
                        hdmirxset(0xB1, 0xE0, 0xE0);
                    }
                    else
                    {
                        hdmirxset(0xB2, 0x05, 0x04);
                        hdmirxset(0xB1, 0xE0, 0xE0);
                    }
                    chgbank(0);
                }
            }
            CLKStbChkCnt = 0;
        }
        else
        {
            CLKStbChkCnt++;
        }
    }
    else
    { // not OverWrite
        chgbank(1);
        if (!port)
        {
            hdmirxset(0xB1, 0x60, 0x00);
        }
        else
        {
            hdmirxset(0xC1, 0x60, 0x00);
        }
        chgbank(0);
    }
}

void
show_vid_info (
    void)
{
    #if 1
    int             InBPC, InBPP;
    int             GCP_CD = CD8BIT; // 24 bits per pixel

    unsigned long   FrameRate;

    GCP_CD = ((hdmirxrd(0x99) & 0xF0) >> 4);

    switch (GCP_CD)
    {
        case 5:
            VIDEOTIMNG_DEBUG_PRINTF(("I/P ColorDepth = 30 bits per pixel\n"));
            InBPC   = 10;
            hdmirxset(0x65, 0x0C, 0x04);
            OutCD   = OUT10B;
            break;

        case 6:
            VIDEOTIMNG_DEBUG_PRINTF(("I/P ColorDepth = 36 bits per pixel\n"));
            InBPC   = 12;
            hdmirxset(0x65, 0x0C, 0x08);
            OutCD   = OUT12B;
            break;

        default:
            VIDEOTIMNG_DEBUG_PRINTF(("I/P ColorDepth = 24 bits per pixel\n"));
            InBPC   = 8;
            hdmirxset(0x65, 0x0C, 0x00);
            OutCD   = OUT8B;
            break;
    }

    switch (OutCD)
    {
        case 1:
            VIDEOTIMNG_DEBUG_PRINTF(("O/P ColorDepth = 30 bits per pixel\n"));
            break;

        case 2:
            VIDEOTIMNG_DEBUG_PRINTF(("O/P ColorDepth = 36 bits per pixel\n"));
            break;

        default:
            VIDEOTIMNG_DEBUG_PRINTF(("O/P ColorDepth = 24 bits per pixel\n"));
            break;
    }

    chgbank(2);
    InColorMode = (hdmirxrd(0x15) & 0x60) >> 5;
    chgbank(0);

    if (InColorMode == 1)  // YCbCr422
    {
        InBPP = InBPC * 2;
    }
    else
    {
        InBPP = InBPC * 3;
    }

    switch (InColorMode)
    {
        case 0:
            VIDEOTIMNG_DEBUG_PRINTF(("Input Color Mode = RGB444\n"));
            //       hdmirxset(0xAE, 0x01, 0x01);
            //       defaultrgb();
            break;

        case 1:
            VIDEOTIMNG_DEBUG_PRINTF(("Input Color Mode = YCbCr422\n"));
            //       hdmirxset(0xAE, 0x01, 0x00);
            //       yuv422torgb();
            break;

        case 2:
            VIDEOTIMNG_DEBUG_PRINTF(("Input Color Mode = YCbCr444\n"));
            //       hdmirxset(0xAE, 0x01, 0x00);
            //       yuv444torgb();
            break;

        default:
            VIDEOTIMNG_DEBUG_PRINTF(("Input Color Mode = Reserved !!!\n"));
            break;
    }

    OutColorMode = (hdmirxrd(0x65) & 0x30) >> 4;
    switch (OutColorMode)
    {
        case 0:
            VIDEOTIMNG_DEBUG_PRINTF(("Output Color Mode = RGB444\n"));
            //       hdmirxset(0x65, 0x30, 0x00);
            break;

        case 1:
            VIDEOTIMNG_DEBUG_PRINTF(("Output Color Mode = YCbCr422\n"));
            //       hdmirxset(0x65, 0x30, 0x10);
            break;

        case 2:
            VIDEOTIMNG_DEBUG_PRINTF(("Output Color Mode = YCbCr444\n"));
            //       hdmirxset(0x65, 0x30, 0x20);
            break;

        default:
            VIDEOTIMNG_DEBUG_PRINTF(("Output Color Mode = Reserved !!!\n"));
            break;
    }

    //    VIDEOTIMNG_DEBUG_PRINTF(("Video Input Timing: %s\n", s_VMTable[VIC].format));
    //    VIDEOTIMNG_DEBUG_PRINTF(("TMDSCLK = %3.3fMHz\n", (unsigned long)(CurTMDSCLK)/1000));
    //    VIDEOTIMNG_DEBUG_PRINTF(("PCLK = %3.3fMHz\n", (unsigned long)(CurVTiming.PCLK)/1000));

    VIDEOTIMNG_DEBUG_PRINTF(("HFrontPorch = %d\n", CurVTiming.HFrontPorch));
    VIDEOTIMNG_DEBUG_PRINTF(("HSyncWidth = %d\n", CurVTiming.HSyncWidth));
    VIDEOTIMNG_DEBUG_PRINTF(("HBackPorch = %d\n", CurVTiming.HBackPorch));
    VIDEOTIMNG_DEBUG_PRINTF(("VFrontPorch = %d\n", CurVTiming.VFrontPorch));
    VIDEOTIMNG_DEBUG_PRINTF(("VSyncWidth = %d\n", CurVTiming.VSyncWidth));
    VIDEOTIMNG_DEBUG_PRINTF(("VBackPorch = %d\n", CurVTiming.VBackPorch));

    FrameRate   = (unsigned long)(CurVTiming.PCLK) * 1000 * 1000;// [2020 03 19 bu5 modify]
    FrameRate   /= CurVTiming.HTotal;
    FrameRate   /= CurVTiming.VTotal;
    VIDEOTIMNG_DEBUG_PRINTF(("FrameRate = %ld Hz\n", FrameRate));

    if (CurVTiming.ScanMode == 0)
    {
        VIDEOTIMNG_DEBUG_PRINTF(("ScanMode = Progressive\n"));
    }
    else
    {
        VIDEOTIMNG_DEBUG_PRINTF(("ScanMode = InterLaced\n"));
    }

    if (CurVTiming.VPolarity == 1)
    {
        VIDEOTIMNG_DEBUG_PRINTF(("VSyncPol = Positive\n"));
    }
    else
    {
        VIDEOTIMNG_DEBUG_PRINTF(("VSyncPol = Negative\n"));
    }

    if (CurVTiming.HPolarity == 1)
    {
        VIDEOTIMNG_DEBUG_PRINTF(("HSyncPol = Positive\n"));
    }
    else
    {
        VIDEOTIMNG_DEBUG_PRINTF(("HSyncPol = Negative\n"));
    }

    if (((hdmirxrd(0x51) & 0x01)))
    {
        VIDEOTIMNG_DEBUG_PRINTF(("Port= 1 ,Reg18=%X ,", (int)hdmirxrd(REG_RX_018)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg38=%X, ", (int)hdmirxrd(REG_RX_038)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg3E=%X, ", (int)hdmirxrd(REG_RX_03E)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg3F=%X, ", (int)hdmirxrd(REG_RX_03F)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg40=%X\n", (int)hdmirxrd(REG_RX_040)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg41=%X\n", (int)hdmirxrd(REG_RX_041)));
        chgbank(1);
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_B_CS=%X  ", (int)(hdmirxrd(REG_RX_1DD) & 0x80) >> 7));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_G_CS=%X  ", (int)(hdmirxrd(REG_RX_1DE) & 0x80) >> 7));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_R_CS=%X\n", (int)(hdmirxrd(REG_RX_1DF) & 0x80) >> 7));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_B_RS=%X  ", (int)(hdmirxrd(REG_RX_1DD) & 0x7F)));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_G_RS=%X  ", (int)(hdmirxrd(REG_RX_1DE) & 0x7F)));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_R_RS=%X\n", (int)(hdmirxrd(REG_RX_1DF) & 0x7F)));
        VIDEOTIMNG_DEBUG_PRINTF((" Reg1C1  = %X , Reg1C2  = %X\n", (int)hdmirxrd(REG_RX_1C1), (int)hdmirxrd(REG_RX_1C2)));
        chgbank(0);
    }
    else
    {
        VIDEOTIMNG_DEBUG_PRINTF(("Port= 0 ,Reg11=%X ,", (int)hdmirxrd(REG_RX_011)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg20=%X, ", (int)hdmirxrd(REG_RX_020)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg26=%X, ", (int)hdmirxrd(REG_RX_026)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg27=%X, ", (int)hdmirxrd(REG_RX_027)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg28=%X, ", (int)hdmirxrd(REG_RX_028)));
        VIDEOTIMNG_DEBUG_PRINTF(("Reg29=%X\n", (int)hdmirxrd(REG_RX_029)));
        chgbank(1);
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_B_CS=%X  ", (int)(hdmirxrd(REG_RX_1D5) & 0x80) >> 7));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_G_CS=%X  ", (int)(hdmirxrd(REG_RX_1D6) & 0x80) >> 7));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_R_CS=%X\n", (int)(hdmirxrd(REG_RX_1D7) & 0x80) >> 7));

        VIDEOTIMNG_DEBUG_PRINTF(("Rec_B_RS=%X  ", (int)(hdmirxrd(REG_RX_1D5) & 0x7F)));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_G_RS=%X  ", (int)(hdmirxrd(REG_RX_1D6) & 0x7F)));
        VIDEOTIMNG_DEBUG_PRINTF(("Rec_R_RS=%X\n", (int)(hdmirxrd(REG_RX_1D7) & 0x7F)));
        VIDEOTIMNG_DEBUG_PRINTF(("REG_RX_1B1 = %X ,  REG_RX_1B2 = %X\n", (int)hdmirxrd(REG_RX_1B1), (int)hdmirxrd(REG_RX_1B2)));

        chgbank(0);
    }

    VIDEOTIMNG_DEBUG_PRINTF(("TMDSCLK = %d MHz\n", (int)(CurTMDSCLK)));
    VIDEOTIMNG_DEBUG_PRINTF(("PCLK = %d MHz\n", (int)(CurVTiming.PCLK)));
    VIDEOTIMNG_DEBUG_PRINTF(("HActive = %d\n", CurVTiming.HActive));
    VIDEOTIMNG_DEBUG_PRINTF(("VActive = %d\n", CurVTiming.VActive));
    VIDEOTIMNG_DEBUG_PRINTF(("HTotal = %d\n", CurVTiming.HTotal));
    VIDEOTIMNG_DEBUG_PRINTF(("VTotal = %d\n", CurVTiming.VTotal));

    if (IsHDMIMode())
    {
        VIDEOTIMNG_DEBUG_PRINTF(("HDMI/DVI Mode : HDMI\n"));
    }
    else
    {
        VIDEOTIMNG_DEBUG_PRINTF(("HDMI/DVI Mode : DVI\n"));
    }
    #endif
}

#endif

/*********************************************************************************/
/* End of it660x.c ***************************************************************/
/*********************************************************************************/

#ifdef Enable_Vendor_Specific_packet

    #define HDMI_3DFORMAT_PRESENT   0x40
    #define HDMI_3DFORMAT_OFF       0x00
    #define FRAME_PACKING           0x00
    #define TOP_AND_BOTTOM          0x60
    #define SIDE_BY_SIDE            0x80

SET_DE3D_FRAME t_3d_syncgen[] =
{
    // 640x480      //524   //559   //514   //526
    {0x01, 0x020C, 0x022F, 0x0202, 0x020E, 480   }, // 60Hz
    // 480p      //524   //560   //515   //530
    {0x02, 0x020C, 0x0230, 0x0203, 0x0212, 480   }, // 60Hz
    {0x03, 0x020C, 0x0230, 0x0203, 0x0212, 480   }, // 60Hz
    // 576p      //624   //668   //619   //629
    {0x11, 0x0270, 0x029C, 0x026B, 0x0275, 576   }, // 50Hz
    {0x12, 0x0270, 0x029C, 0x026B, 0x0275, 576   }, // 50Hz
    // 720p      //749   //774   //744   //754
    {0x3c, 0x02ED, 0x0306, 0x02E8, 0x02F2, 720   }, // 24Hz
    {0x3d, 0x02ED, 0x0306, 0x02E8, 0x02F2, 720   }, // 25Hz
    {0x3e, 0x02ED, 0x0306, 0x02E8, 0x02F2, 720   }, // 30Hz
    {0x13, 0x02ED, 0x0306, 0x02E8, 0x02F2, 720   }, // 50Hz
    {0x04, 0x02ED, 0x0306, 0x02E8, 0x02F2, 720   }, // 60Hz

    // 1080p    //1124   //1165   //1120   //1129
    {0x20, 0x0464, 0x048D, 0x0460, 0x0469, 1080  }, // 24Hz
    {0x21, 0x0464, 0x048D, 0x0460, 0x0469, 1080  }, // 25Hz
    {0x22, 0x0464, 0x048D, 0x0460, 0x0469, 1080  }, // 30Hz

    // default
    {0xFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}
};

    #define Reg_PGVTotal_19D        0x9D            // [11:4]   0x19D[7:0]
    #define Reg_PGVTotal_19C        0x9C            // [3:0]        0x19C[7:4]
    #define Reg_PGVActSt_192        0x92            // [7:0]        0x192[7:0]
    #define Reg_PGVActSt_193        0x93            // [11:8]   0x193[3:0]
    #define Reg_PGVActEd_193        0x93            // [3:0]        0x193[7:4]
    #define Reg_PGVActEd_194        0x94            // [11:4]   0x194[7:0]
    #define Reg_PGVSyncEd_19F       0x9F            // [3:0]        0x19F[7:4]
    #define Reg_PGVSyncSt_19F       0x9F            // [11:8]   0x19F[3:0]
    #define Reg_PGVSyncSt_19E       0x9E            // [7:0]        0x19E[7:0]

    #define Reg_PG3DRSt_18F         0x8F            // [7:0]        0x190[11:8] 0x18F[7:0]
    #define Reg_PG3DRStEd_190       0x90            // [7:0]        0x191[3:0] 0x18F[11:8]
    #define Reg_PG3DREd_191         0x91            // [11:4]       0x191[11:4] 0x190[3:0]

    #define REG_RX_066_4_DE3DFrame  0x66            // [4] 1: 3D frame-packet mode to sequence mode
    #define REG_RX_085_5_En3DROut   0x85            // [5] 1: Enable 3DR output
//

void
Dump3DReg (
    void)
{
    unsigned short  i, j;
    unsigned char   ucData;

    VSDB_DEBUG_PRINTF(("\n       "));
    for (j = 0; j < 16; j++)
    {
        VSDB_DEBUG_PRINTF((" %02X", (int) j));
        if ((j == 3) || (j == 7) || (j == 11))
        {
            VSDB_DEBUG_PRINTF((" :"));
        }
    }
    VSDB_DEBUG_PRINTF(("\n"));

    chgbank(1);

    for (i = 0x80; i < 0xa0; i += 16)
    {
        VSDB_DEBUG_PRINTF(("[%03X]  ", i));
        for (j = 0; j < 16; j++)
        {
            ucData = hdmirxrd((unsigned char)((i + j) & 0xFF));
            VSDB_DEBUG_PRINTF((" %02X", (int) ucData));
            if ((j == 3) || (j == 7) || (j == 11))
            {
                VSDB_DEBUG_PRINTF((" :"));
            }
        }
        VSDB_DEBUG_PRINTF(("\n"));
    }

    VSDB_DEBUG_PRINTF(("\n        =====================================================\n"));

    chgbank(0);
}

unsigned char
IT6602_DE3DFrame (
    unsigned char ena_de3d)
/*
 * This function configures the HDMI DE3DFrame
 * @param uunsigned char ena_de3d
 * @return      TRUE
 *              FALSE
 */
{
    unsigned char   i, uc, uc1, uc2, uc3;
    unsigned int    v_total;
    unsigned int    v_act_start;
    unsigned int    v_act_end;
    unsigned int    v_sync_end;
    unsigned int    v_act_bspace;
    unsigned int    v_2d_Vtotal;
    unsigned int    HActive;
    unsigned int    LR_3D_Start;
    unsigned int    LR_3D_End;

    #ifdef DEBUG_MODE
    // dbmsg_trace(DBM_DPATH,"it660x - HDMI_DE3DFrame\n");
    #endif

    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();

    if (ena_de3d == TRUE)
    {
        chgbank(2);
        uc  = hdmirxrd(REG_RX_224);
        uc1 = hdmirxrd(REG_RX_228);
        uc2 = hdmirxrd(REG_RX_229);
        uc3 = hdmirxrd(REG_RX_22A);
        chgbank(0);
        if ((uc == 0x81) && (uc1 == 0x03) && (uc2 == 0x0C) && (uc3 == 0x00))    // ITE_150420  check  IEEE OUI
        // 3D InfoFrame Packet Type is valid
        {
            chgbank(2);
            IT6602data->s_Current3DFr.VIC   = hdmirxrd(REG_RX_218);             // AVI INFO PB4
            IT6602data->s_Current3DFr.HB0   = hdmirxrd(REG_RX_224);             // General Packet Header Byte 0
            IT6602data->s_Current3DFr.HB1   = hdmirxrd(REG_RX_225);
            IT6602data->s_Current3DFr.HB2   = hdmirxrd(REG_RX_226);
            IT6602data->s_Current3DFr.PB0   = hdmirxrd(REG_RX_227);             // General Packet Data Byte 0
            IT6602data->s_Current3DFr.PB1   = hdmirxrd(REG_RX_228);
            IT6602data->s_Current3DFr.PB2   = hdmirxrd(REG_RX_229);
            IT6602data->s_Current3DFr.PB3   = hdmirxrd(REG_RX_22A);
            IT6602data->s_Current3DFr.PB4   = hdmirxrd(REG_RX_22B);
            IT6602data->s_Current3DFr.PB5   = hdmirxrd(REG_RX_22C);
            IT6602data->s_Current3DFr.PB6   = hdmirxrd(REG_RX_22D);
            IT6602data->s_Current3DFr.PB7   = hdmirxrd(REG_RX_22E);
            chgbank(0);

            // #ifdef DEBUG_MODE_3D
            VSDB_DEBUG_PRINTF(("\nIT653x - HDMI_DumpDE3DFrameInfo:\n"));
            VSDB_DEBUG_PRINTF(("        HDMI VIC = 0x%X\n", IT6602data->s_Current3DFr.VIC));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe HB0 = 0x%X\n", (int) IT6602data->s_Current3DFr.HB0));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe HB1 = 0x%X\n", (int) IT6602data->s_Current3DFr.HB1));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe HB2 = 0x%X\n", (int) IT6602data->s_Current3DFr.HB2));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB0 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB0));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB1 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB1));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB2 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB2));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB3 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB3));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB4 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB4));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB5 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB5));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB6 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB6));
            VSDB_DEBUG_PRINTF(("        Record HDMI vender specific inforframe PB7 = 0x%X\n", (int) IT6602data->s_Current3DFr.PB7));
            // #endif

            /******************************  3D integration  *************************************/

            IT6602data->de3dframe_config.LR_Reference               = 2;    // Source of the 3D L/R reference.
            IT6602data->de3dframe_config.FrameDominance             = 0;    // Left or Right Eye is first in L/R image pair.
            IT6602data->de3dframe_config.LR_Encoding                = 1;    // Type of 3D L/R encoding
            IT6602data->de3dframe_config.TB_Reference               = 2;    // Top/Bottom reference for vertically sub-sampled sources
            IT6602data->de3dframe_config.OE_Reference               = 2;    // Odd/Even reference for horizontally sub-sampled sources

            IT6602data->de3dframe_config.NumActiveBlankLines        = 0;    // Number of lines separating vertically packed L/R data to be removed (cropped)before being displayed
            IT6602data->de3dframe_config.NumberOfEncodedLines       = 0;    // Number of encoded lines in one L/R eye frame of the display data to be blanked out with "Blanking Color".
            IT6602data->de3dframe_config.LeftEncodedLineLocation    = -1;   // Active line number of 1st encoded line in one Left eye frame of the display data (-1=unknown).
            IT6602data->de3dframe_config.RightEncodedLineLocation   = -1;   // Active line number of 1st encoded line in one Right eye frame of the display data (-1=unknown).
            IT6602data->de3dframe_config.BlankingColor              = 7;    // Color to use when blanking (or masking off) any embedded L/R encoding

            if (((IT6602data->s_Current3DFr.PB4 & 0xE0) == HDMI_3DFORMAT_PRESENT) && ((IT6602data->s_Current3DFr.PB5 & 0xF0) == FRAME_PACKING))
            {
                i = 0;

                while (t_3d_syncgen[i].Vic != 0xFF)
                {
                    if (t_3d_syncgen[i].Vic == IT6602data->s_Current3DFr.VIC)
                    {
                        break;
                    }
                    i++;
                }
                v_total     = t_3d_syncgen[i].V_total;
                v_act_start = t_3d_syncgen[i].V_act_start;
                v_act_end   = t_3d_syncgen[i].V_act_end;
                v_sync_end  = t_3d_syncgen[i].V_sync_end;
                v_2d_Vtotal = t_3d_syncgen[i].V_2D_active_total;
                chgbank(1);
                hdmirxset(Reg_PGVTotal_19D, 0xFF, (unsigned char) ((v_total & 0xFF0) >> 4));        // pccmd w 9d 2e
                hdmirxset(Reg_PGVTotal_19C, 0xF0, (unsigned char) ((v_total & 0x00F) << 4));        // pccmd w 9c d0
                hdmirxset(Reg_PGVActSt_192, 0xFF, (unsigned char) ((v_act_start & 0x0FF)));         // pccmd w 92 06
                hdmirxset(Reg_PGVActSt_193, 0x0F, (unsigned char) ((v_act_start & 0xF00) >> 8));    // pccmd w 93 83
                hdmirxset(Reg_PGVActEd_193, 0xF0, (unsigned char) ((v_act_end & 0x00F) << 4));      // pccmd w 93 83
                hdmirxset(Reg_PGVActEd_194, 0xFF, (unsigned char) ((v_act_end & 0xFF0) >> 4));      // pccmd w 94 2E
                hdmirxset(Reg_PGVSyncEd_19F, 0xF0, (unsigned char) ((v_sync_end & 0x00F) << 4));    // pccmd w 9f 22

    #if 1
                LR_3D_Start = (v_act_start - (v_2d_Vtotal / 2));
                LR_3D_End   = (v_act_start + (v_2d_Vtotal / 2));
    #else
                LR_3D_Start = ((v_total / 2));
                LR_3D_End   = (LR_3D_Start * 3);
    #endif

                hdmirxset(Reg_PG3DRSt_18F, 0xFF, (unsigned char) ((LR_3D_Start & 0x0FF)));
                hdmirxset(Reg_PG3DRStEd_190, 0x0F, (unsigned char) ((LR_3D_Start & 0xF00) >> 8));
                hdmirxset(Reg_PG3DRStEd_190, 0xF0, (unsigned char) ((LR_3D_End & 0x00F) << 4));
                hdmirxset(Reg_PG3DREd_191, 0xFF, (unsigned char) ((LR_3D_End & 0xFF0) >> 4));

                VSDB_DEBUG_PRINTF(("\nv_total = %X or %d\n", (int)  (v_total), (int)  (v_total)));
                VSDB_DEBUG_PRINTF(("Reg_PGVTotal_19D = %X\n", (int)  (hdmirxrd(Reg_PGVTotal_19D))));
                VSDB_DEBUG_PRINTF(("Reg_PGVTotal_19C = %X\n", (int)  (hdmirxrd(Reg_PGVTotal_19C))));
                VSDB_DEBUG_PRINTF(("\nv_act_start = %X or %d\n", (int)  (v_act_start), (int)  (v_act_start)));
                VSDB_DEBUG_PRINTF(("Reg_PGVActSt_192 = %X\n", (int)  (hdmirxrd(Reg_PGVActSt_192))));
                VSDB_DEBUG_PRINTF(("Reg_PGVActSt_193 = %X\n", (int)  (hdmirxrd(Reg_PGVActSt_193))));
                VSDB_DEBUG_PRINTF(("\nv_act_end = %X or %d\n", (int)  (v_act_end), (int)  (v_act_end)));
                VSDB_DEBUG_PRINTF(("Reg_PGVActEd_193 = %X\n", (int)  (hdmirxrd(Reg_PGVActEd_193))));
                VSDB_DEBUG_PRINTF(("Reg_PGVActEd_194 = %X\n", (int)  (hdmirxrd(Reg_PGVActEd_194))));
                VSDB_DEBUG_PRINTF(("\nv_sync_end = %X or %d\n", (int)  (v_sync_end), (int)  (v_sync_end)));
                VSDB_DEBUG_PRINTF(("Reg_PGVSyncEd_19F = %X\n", (int)  (hdmirxrd(Reg_PGVSyncEd_19F))));

                VSDB_DEBUG_PRINTF(("LR_3D_Start = %X or %d\n", (int)  (LR_3D_Start), (int)  (LR_3D_Start)));
                VSDB_DEBUG_PRINTF(("Reg_PG3DRSt_18F = %X\n", (int)  (hdmirxrd(Reg_PG3DRSt_18F))));
                VSDB_DEBUG_PRINTF(("Reg_PG3DRStEd_190 = %X\n", (int)  (hdmirxrd(Reg_PG3DRStEd_190))));
                VSDB_DEBUG_PRINTF(("Reg_PG3DREd_191 = %X\n", (int)  (hdmirxrd(Reg_PG3DREd_191))));
                VSDB_DEBUG_PRINTF(("LR_3D_End = %X or %d\n", (int)  (LR_3D_End), (int)  (LR_3D_End)));

                VSDB_DEBUG_PRINTF(("\n\nv_total = %X or %d\n", (int)  (v_total), (int)  (v_total)));
                VSDB_DEBUG_PRINTF(("v_act_start = %X or %d\n", (int)  (v_act_start), (int)  (v_act_start)));
                VSDB_DEBUG_PRINTF(("v_act_end = %X or %d\n", (int)  (v_act_end), (int)  (v_act_end)));
                VSDB_DEBUG_PRINTF(("v_sync_end = %X or %d\n", (int)  (v_sync_end), (int)  (v_sync_end)));
                VSDB_DEBUG_PRINTF(("LR_3D_Start = %X or %d\n", (int)  (LR_3D_Start), (int)  (LR_3D_Start)));
                VSDB_DEBUG_PRINTF(("LR_3D_End = %X or %d\n", (int)  (LR_3D_End), (int)  (LR_3D_End)));

                chgbank(0);
                hdmirxset(REG_RX_066_4_DE3DFrame, 0x10, 0x10);  // Reg66[4] = 1 for enable 3D FP2FS
                hdmirxset(REG_RX_085_5_En3DROut, 0x20, 0x20);   // Reg85[5] = 1 for enable 3DR output

                Dump3DReg();

                // enable output
                HActive         = ((hdmirxrd(0x9F) & 0x3F) << 8) + hdmirxrd(0x9E);
                // ChangePicoResolution(HActive,v_2d_Vtotal);
                v_act_bspace    = v_act_start - v_act_end;
            }

            if (((IT6602data->s_Current3DFr.PB4 & 0xE0) == HDMI_3DFORMAT_PRESENT) && (!IT6602data->DE3DFormat_HDMIFlag))
            {
                IT6602data->DE3DFormat_HDMIFlag = TRUE;
            }

            if (((IT6602data->s_Current3DFr.PB4 & 0xE0) == HDMI_3DFORMAT_PRESENT) && (IT6602data->DE3DFormat_HDMIFlag))
            {
                if (((IT6602data->s_Current3DFr.PB5 & 0xF0) == FRAME_PACKING) && (!IT6602data->FramePacking_Flag))
                {
                    IT6602data->FramePacking_Flag   = TRUE;
                    IT6602data->TopAndBottom_Flag   = FALSE;
                    IT6602data->SideBySide_Flag     = FALSE;
                    IT6602data->oldVIC              = 0;
                }

                if (((IT6602data->s_Current3DFr.PB5 & 0xF0) == FRAME_PACKING) && (IT6602data->FramePacking_Flag))
                {
                    IT6602data->newVIC = IT6602data->s_Current3DFr.VIC;
                    if (IT6602data->newVIC != IT6602data->oldVIC)
                    {
                        if ((IT6602data->s_Current3DFr.VIC == 0x3c) || (IT6602data->s_Current3DFr.VIC == 0x3e) || (IT6602data->s_Current3DFr.VIC == 0x13) ||
                           (IT6602data->s_Current3DFr.VIC == 0x04) || (IT6602data->s_Current3DFr.VIC == 0x20) || (IT6602data->s_Current3DFr.VIC == 0x22))
                        // (IT6602data->s_Current3DFr.VIC == 0x05) ||(IT6602data->s_Current3DFr.VIC == 0x14) // 1080i@50&60Hz not supported for frame packing
                        {
                            IT6602data->de3dframe_config.NumActiveBlankLines    = (unsigned char)v_act_bspace;
                            IT6602data->de3dframe_config.Format                 = VERT_PACKED_FULL; // Type of 3D source format is FRAME_PACKING(VERT_PACKED_FULL)

    #ifdef DEBUG_MODE_3D
                            dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is FRAME_PACKING\n");
    #else
                            VSDB_DEBUG_PRINTF(("it660x - HDMI_3DFORMAT is FRAME_PACKING\n"));

    #endif
                        }
                        else
                        {
                            IT6602data->de3dframe_config.Format = 6;     // Type of 3D source format is UNDEFINED_FORMAT

    #ifdef DEBUG_MODE_3D
                            dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is UNDEFINED_FORMAT\n");
    #endif
                        }
    #ifdef DEBUG_MODE_3D
                        dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is FRAME_PACKING call detect3D_Port_3D_On( )\n");
    #endif
                        // detect3D_Port_3D_On(&IT6602data->de3dframe_config);  //ralph
                        // HDMI_DumpDE3DFrameInfo(&IT6602data->s_Current3DFr);
                        IT6602data->oldVIC = IT6602data->newVIC;
                    }
                }

                if (((IT6602data->s_Current3DFr.PB5 & 0xF0) == TOP_AND_BOTTOM) && (!IT6602data->TopAndBottom_Flag))
                {
                    if ((IT6602data->s_Current3DFr.VIC == 0x3c) || (IT6602data->s_Current3DFr.VIC == 0x3e) || (IT6602data->s_Current3DFr.VIC == 0x13) || (IT6602data->s_Current3DFr.VIC == 0x04) || (IT6602data->s_Current3DFr.VIC == 0x05) ||
                       (IT6602data->s_Current3DFr.VIC == 0x14) || (IT6602data->s_Current3DFr.VIC == 0x20) || (IT6602data->s_Current3DFr.VIC == 0x22) || (IT6602data->s_Current3DFr.VIC == 0x1f) || (IT6602data->s_Current3DFr.VIC == 0x10))
                    {
                        IT6602data->de3dframe_config.Format = VERT_PACKED_HALF;    // Type of 3D source format is TOP_AND_BOTTOM(VERT_PACKED_HALF)

    #ifdef DEBUG_MODE_3D
                        dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is TOP_AND_BOTTOM\n");
    #else
                        VSDB_DEBUG_PRINTF(("it660x - HDMI_3DFORMAT is TOP_AND_BOTTOM\n"));
    #endif
                    }
                    else
                    {
                        IT6602data->de3dframe_config.Format = 6;    // Type of 3D source format is UNDEFINED_FORMAT

    #ifdef DEBUG_MODE_3D
                        dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is UNDEFINED_FORMAT\n");
    #endif
                    }

                    // detect3D_Port_3D_On(&IT6602data->de3dframe_config);  //ralph
                    // HDMI_DumpDE3DFrameInfo(&IT6602data->s_Current3DFr);

                    IT6602data->FramePacking_Flag   = FALSE;
                    IT6602data->TopAndBottom_Flag   = TRUE;
                    IT6602data->SideBySide_Flag     = FALSE;
                }

                if (((IT6602data->s_Current3DFr.PB5 & 0xF0) == SIDE_BY_SIDE) && (!IT6602data->SideBySide_Flag))
                {
                    if ((IT6602data->s_Current3DFr.VIC == 0x3c) || (IT6602data->s_Current3DFr.VIC == 0x3e) || (IT6602data->s_Current3DFr.VIC == 0x13) || (IT6602data->s_Current3DFr.VIC == 0x04) || (IT6602data->s_Current3DFr.VIC == 0x05) ||
                       (IT6602data->s_Current3DFr.VIC == 0x14) || (IT6602data->s_Current3DFr.VIC == 0x20) || (IT6602data->s_Current3DFr.VIC == 0x22) || (IT6602data->s_Current3DFr.VIC == 0x1f) || (IT6602data->s_Current3DFr.VIC == 0x10))
                    {
                        IT6602data->de3dframe_config.Format = HORIZ_PACKED_HALF;    // Type of 3D source format is SIDE_BY_SIDE(HORIZ_PACKED_HALF)

    #ifdef DEBUG_MODE_3D
                        dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is SIDE_BY_SIDE\n");
    #else
                        VSDB_DEBUG_PRINTF(("it660x - HDMI_3DFORMAT is SIDE_BY_SIDE\n"));
    #endif
                    }
                    else
                    {
                        IT6602data->de3dframe_config.Format = 6;    // Type of 3D source format is UNDEFINED_FORMAT

    #ifdef DEBUG_MODE_3D
                        dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is UNDEFINED_FORMAT\n");
    #endif
                    }

                    // detect3D_Port_3D_On(&IT6602data->de3dframe_config);  //ralph
                    // HDMI_DumpDE3DFrameInfo(&IT6602data->s_Current3DFr);

                    IT6602data->FramePacking_Flag   = FALSE;
                    IT6602data->TopAndBottom_Flag   = FALSE;
                    IT6602data->SideBySide_Flag     = TRUE;
                }

    #ifdef DEBUG_MODE_3D
                dbmsg_trace(DBM_3D, "\nit660x - HDMI_3D_SourceConfiguration:\n");
                dbmsg_ftrace(DBM_3D, "        Format                   = %X\n", (int) IT6602data->de3dframe_config.Format);
                dbmsg_ftrace(DBM_3D, "        LR_Reference             = %X\n", (int) IT6602data->de3dframe_config.LR_Reference);
                dbmsg_ftrace(DBM_3D, "        FrameDominance           = %X\n", (int) IT6602data->de3dframe_config.FrameDominance);
                dbmsg_ftrace(DBM_3D, "        LR_Encoding              = %X\n", (int) IT6602data->de3dframe_config.LR_Encoding);
                dbmsg_ftrace(DBM_3D, "        TB_Reference             = %X\n", (int) IT6602data->de3dframe_config.TB_Reference);
                dbmsg_ftrace(DBM_3D, "        OE_Reference             = %X\n", (int) IT6602data->de3dframe_config.OE_Reference);
                dbmsg_ftrace(DBM_3D, "        NumActiveBlankLines      = %X\n", (int) IT6602data->de3dframe_config.NumActiveBlankLines);
                dbmsg_ftrace(DBM_3D, "        NumberOfEncodedLines     = %X\n", (int) IT6602data->de3dframe_config.NumberOfEncodedLines);
                dbmsg_ftrace(DBM_3D, "        LeftEncodedLineLocation  = %X\n", (int) IT6602data->de3dframe_config.LeftEncodedLineLocation);
                dbmsg_ftrace(DBM_3D, "        RightEncodedLineLocation = %X\n", (int) IT6602data->de3dframe_config.RightEncodedLineLocation);
                dbmsg_ftrace(DBM_3D, "        BlankingColor            = %X\n", (int) IT6602data->de3dframe_config.BlankingColor);

    #else
                VSDB_DEBUG_PRINTF(("\nit660x - HDMI_3D_SourceConfiguration:\n"));
                VSDB_DEBUG_PRINTF(("        Format                   = %X\n", (int) IT6602data->de3dframe_config.Format));
                VSDB_DEBUG_PRINTF(("        LR_Reference             = %X\n", (int) IT6602data->de3dframe_config.LR_Reference));
                VSDB_DEBUG_PRINTF(("        FrameDominance           = %X\n", (int) IT6602data->de3dframe_config.FrameDominance));
                VSDB_DEBUG_PRINTF(("        LR_Encoding              = %X\n", (int) IT6602data->de3dframe_config.LR_Encoding));
                VSDB_DEBUG_PRINTF(("        TB_Reference             = %X\n", (int) IT6602data->de3dframe_config.TB_Reference));
                VSDB_DEBUG_PRINTF(("        OE_Reference             = %X\n", (int) IT6602data->de3dframe_config.OE_Reference));
                VSDB_DEBUG_PRINTF(("        NumActiveBlankLines      = %X\n", (int) IT6602data->de3dframe_config.NumActiveBlankLines));
                VSDB_DEBUG_PRINTF(("        NumberOfEncodedLines     = %X\n", (int) IT6602data->de3dframe_config.NumberOfEncodedLines));
                VSDB_DEBUG_PRINTF(("        LeftEncodedLineLocation  = %X\n", (int) IT6602data->de3dframe_config.LeftEncodedLineLocation));
                VSDB_DEBUG_PRINTF(("        RightEncodedLineLocation = %X\n", (int) IT6602data->de3dframe_config.RightEncodedLineLocation));
                VSDB_DEBUG_PRINTF(("        BlankingColor            = %X\n", (int) IT6602data->de3dframe_config.BlankingColor ));
    #endif

                return TRUE;
            }
        }

        if (IT6602data->DE3DFormat_HDMIFlag)// 3D InfoFrame Packet Type is not valid
        {
    #ifdef DEBUG_MODE_3D
            dbmsg_trace(DBM_3D, "it660x - HDMI_3DFORMAT is OFF\n");
    #endif

// ralph
// detect3D_Port_3D_Off();
// mbSend( detect3DMbxID, D3DMSG_STATE_PORT_2D, -1, 0, FALSE, 0);
// dbmsg_ftrace( DBM_3D, "detect3D_Port_3D_Off: Current state=%s\n", detect3DStateStringTable[detect3DState]);

            IT6602data->DE3DFormat_HDMIFlag = FALSE;
            IT6602data->FramePacking_Flag   = FALSE;
            IT6602data->TopAndBottom_Flag   = FALSE;
            IT6602data->SideBySide_Flag     = FALSE;
        }

        /******************************  3D integration  *************************************/
    }
    else
    {
        // IT6602data->f_de3dframe_hdmi = FALSE;
        hdmirxwr(REG_RX_06A, 0x82);
        hdmirxset(REG_RX_066_4_DE3DFrame, 0x10, 0x00);  // Reg66[4] = 0 for disable 3D FP2FS
        hdmirxset(REG_RX_085_5_En3DROut, 0x20, 0x00);   // Reg85[5] = 0 for disable 3DR output
    }
    return FALSE;
}
#endif

// FIX_ID_003 xxxxx //Add IT6602 Video Output Configure Table
void
IT6602ChangeTTLVideoOutputMode (
    void)
{
    // for test video output format  only !!!
    unsigned char           i;
    struct IT6602_dev_data  * IT6602data = get_IT6602_dev_data();

    if (IT6602data->m_VidOutConfigMode < eVOMreserve)
    {
        IT6602data->m_VidOutConfigMode++;
    }
    else
    {
        IT6602data->m_VidOutConfigMode = 0;
    }

    i = IT6602data->m_VidOutConfigMode;

    IT6602_VideoOutputConfigure_Init(IT6602data, i);
    IT6602_VideoOutputModeSet(IT6602data);
// FIX_ID_003 xxxxx
}

void
it660x_StandbyMode (
    unsigned char ucstate)                    // ITE_151021
{
    struct IT6602_dev_data * IT6602data = get_IT6602_dev_data();
    // ON => Standby  OFF => Resume
    delay1ms(500);
    if (ucstate)
    {
        hdmirxwr(REG_RX_010, 0xC0);
        hdmirxwr(REG_RX_013, 0xF8);
        hdmirxwr(REG_RX_014, 0xFF);
        hdmirxwr(REG_RX_015, 0xFF);
        hdmirxwr(REG_RX_01A, 0xF8);
        hdmirxwr(REG_RX_01B, 0xFF);
        hdmirxwr(REG_RX_01C, 0xFF);
        hdmirxwr(REG_RX_052, 0x3F);
        hdmirxwr(REG_RX_053, 0x3F);
        hdmirxwr(REG_RX_00D, 0x00);
        hdmirxwr(REG_RX_00E, 0x80);
        IT6602_DEBUG_INT_PRINTF(("StandbyMode On \n"));
    }
    else
    {
        IT6602_Rst(&IT6602DEV);
#ifdef  _SUPPORT_HDMI_REPEATER_             // 20090916
        it660x_Init_HDMIRepeater(IT6602data, TRUE);
#endif //_SUPPORT_HDCP_REPEATER_
#ifdef Enable_IT6602_CEC
        IT6602_CECInit();
#endif
        IT6602PortSelect(0);
    }
}

// ###ADD FUNCTION###[2020 03 19 bu5 modify]
// =============================================================================
// VIDEO OPTION
unsigned int
IT6602getHorzTotal (
    void)
{
    unsigned int HTotal;
    HTotal = ((hdmirxrd(0x9D) & 0x3F) << 8) + hdmirxrd(0x9C);
    return HTotal;  // hTotal[13:0]
}

unsigned int
IT6602getHorzActive (
    void)
{
    unsigned int HActive;
    HActive = ((hdmirxrd(0x9F) & 0x3F) << 8) + hdmirxrd(0x9E);
    return HActive;
}

unsigned int
IT6602getHorzFrontPorch (
    void)
{
    unsigned int HFP;
    HFP = ((hdmirxrd(0xA1) & 0xF0) << 4) + hdmirxrd(0xA2);
    return HFP;
}

unsigned int
IT6602getHorzSyncWidth (
    void)
{
    unsigned int HSYNCW;
    HSYNCW = ((hdmirxrd(0xA1) & 0x01) << 8) + hdmirxrd(0xA0);
    return HSYNCW;
}

unsigned int
IT6602getHorzBackPorch (
    void)
{
    return IT6602getHorzTotal() - IT6602getHorzActive() -  IT6602getHorzFrontPorch() - IT6602getHorzSyncWidth();
}

unsigned int
IT6602getVertTotal (
    void)
{
    unsigned int VTotal;
    VTotal = ((hdmirxrd(0xA4) & 0x0F) << 8) + hdmirxrd(0xA3);
    return VTotal;
}

unsigned int
IT6602getVertActive (
    void)
{
    unsigned int VActive;
    VActive = ((hdmirxrd(0xA4) & 0xF0) << 4) + hdmirxrd(0xA5);
    return VActive;
}

unsigned int
IT6602getVertFrontPorch (
    void)
{
    unsigned int VFP;
    VFP = hdmirxrd(0xA7) & 0x3F;
    return VFP;
}

unsigned int
IT6602getVertSyncWidth (
    void)
{
    unsigned int VSYNCW;
    VSYNCW = hdmirxrd(0xA6) & 0x1F;
    return VSYNCW;
}

unsigned int
IT6602getVertBackPorch (
    void)
{
    // VTotal - VActive - VFP - VSYNCW;
    return IT6602getVertTotal() - IT6602getVertActive() - IT6602getVertFrontPorch() - IT6602getVertSyncWidth();
}

unsigned int
IT6602getOutputColorDepth (
    void)
{
    return (hdmirxrd(0x65) & 0xc) >> 2;
}

unsigned int
IT6602getIsInterlace (
    void)
{
    return (hdmirxrd(0x99) & 0x02) >> 1;
}

unsigned int
IT6602getSCDT (
    void)
{
    struct IT6602_dev_data * data = get_IT6602_dev_data();
    if (CheckSCDT(data) == TRUE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

unsigned int
IT6602getECC (
    void)
{
    struct IT6602_dev_data * data = get_IT6602_dev_data();
    return data->m_ucEccCount_P0;
}

unsigned int
IT6602getEQEND (
    void)
{
    struct IT6602_dev_data * data = get_IT6602_dev_data();
    if (data->EQPort[F_PORT_SEL_0].f_manualEQadjust == FALSE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

unsigned int
IT6602getPCLK (
    void)
{
    unsigned int PCLK;
    PCLK = (unsigned int) CurVTiming.PCLK * 100;
    return PCLK;
}

unsigned int
IT6602getHDMIMode (
    void)
{
    return (unsigned int)IsHDMIMode();
}

unsigned int
IT6602getDAR (
    void)
{
    unsigned char AVI_DB1;
    chgbank(2);
    AVI_DB1 = hdmirxrd(REG_RX_AVI_DB1);
    AVI_DB1 &= M_AVI_FMT_ASRATIO_MASK;
    chgbank(0);
    return AVI_DB1; // DAR
}

unsigned int
IT6602getOutputVideoMode (
    void)
{
    struct IT6602_dev_data * data = get_IT6602_dev_data();
    return (data->m_bOutputVideoMode & F_MODE_CLRMOD_MASK);
}

unsigned int
IT6602getOutputColorRange (
    void)
{
    struct IT6602_dev_data * data = get_IT6602_dev_data();
    if (data->RGBQuantizationRange == 1)
    {
        return 0;   // Limited range
    }
    else
    {
        return 1;   // Full range
    }
}

unsigned int
IT6602getColorRangeChange (
    void)
{
    static unsigned char    ColorRange  = 0;
    struct IT6602_dev_data  * data      = get_IT6602_dev_data();

    if (ColorRange != data->RGBQuantizationRange)
    {
        ColorRange = data->RGBQuantizationRange;
        return 1;
    }
    else
    {
        return 0;
    }
}

void
IT6602setHDCPStatus (
    unsigned int OnOff)
{
    ucHDCP_Satus = OnOff;
}

unsigned int
IT6602getHDCPStatus (
    void)
{
    return ucHDCP_Satus;
}

// =============================================================================
// AUDIO OPTION
unsigned int
IT6602getIsAudioOn (
    void)
{
    struct IT6602_dev_data * data = get_IT6602_dev_data();
    if (data->m_AState == ASTATE_AudioOn)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

unsigned int
IT6602getIsAudioReset (
    void)
{
    struct IT6602_dev_data * data = get_IT6602_dev_data();
    if ((data->m_AState == ASTATE_AudioOff) || (data->m_AState == ASTATE_ResetAudio))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

unsigned int
IT6602getAudioSampleFrq (
    void)
{
    unsigned char AUD_CHSTAT3;
    AUD_CHSTAT3 = hdmirxrd(REG_RX_AUD_CHSTAT3);
    AUD_CHSTAT3 &= M_FS;
    return AUD_CHSTAT3;
}

unsigned int
IT6602getAudioCannelNum (
    void)
{
    unsigned char AUD_CHSTAT2;
    AUD_CHSTAT2 = hdmirxrd(REG_RX_AUD_CHSTAT2);
    AUD_CHSTAT2 &= M_CH_NUM;
    return AUD_CHSTAT2;
}

unsigned int
IT6602getAudioMode (
    void)
{
    unsigned char AUDIO_CH_STAT;

    AUDIO_CH_STAT = hdmirxrd(REG_RX_AUDIO_CH_STAT);
    if (AUDIO_CH_STAT & B_HBRAUDIO)
    {
        return 1;   // HBR
    }
    else if (AUDIO_CH_STAT & B_DSDAUDIO)
    {
        return 2;   // DSD
    }
    else
    {
        unsigned char AUD_CHSTAT0;
        AUD_CHSTAT0 = hdmirxrd(REG_RX_AUD_CHSTAT0);
        if (AUD_CHSTAT0 & 0x2)
        {
            return 3;   // NLPCM
        }
        else
        {
            return 4;   // LPCM
        }
    }
}

// =============================================================================

// =============================================================================
// ###ADD FUNCTION###

// =============================================================================
void
Dump_it660xReg (
    void)                 // max7088
{
#if 0
    unsigned short i, j;
    // unsigned char reg;
    // unsigned char bank;
    unsigned char ucData;

//    (void)printf(" it660x\n");
//    (void)printf("\n 11111:");
//    (void)printf("\n        ===================================================\r");
    (void)printf("\n       ");
    for (j = 0; j < 16; j++)
    {
        (void)printf(" %02X", (int) j);
        if ((j == 3) || (j == 7) || (j == 11))
        {
            (void)printf(" :");
        }
    }
    (void)printf("\n        =====================================================\n");

    chgbank(0);

    for (i = 0; i < 0x100; i += 16)
    {
        (void)printf("[%03X]  ", i);
        for (j = 0; j < 16; j++)
        {
            ucData = hdmirxrd((unsigned char)((i + j) & 0xFF));
            (void)printf(" %02X", (int) ucData);
            if ((j == 3) || (j == 7) || (j == 11))
            {
                (void)printf(" :");
            }
        }
        (void)printf("\n");
        if ((i % 0x40) == 0x30)
        {
            (void)printf("\n        =====================================================\n");
        }
    }

    chgbank(1);
    for (i = 0x0; i < 0x100; i += 16)
    {
        (void)printf("[%03X]  ", i + 0x100);
        for (j = 0; j < 16; j++)
        {
            ucData = hdmirxrd((unsigned char)((i + j) & 0xFF));
            (void)printf(" %02X", (int) ucData);
            if ((j == 3) || (j == 7) || (j == 11))
            {
                (void)printf(" :");
            }
        }
        (void)printf("\n");
        if ((i % 0x40) == 0x30)
        {
            (void)printf("\n        =====================================================\n");
        }
    }

    chgbank(2);
    for (i = 0x0; i < 0x100; i += 16)
    {
        (void)printf("[%03X]  ", i + 0x200);
        for (j = 0; j < 16; j++)
        {
            ucData = hdmirxrd((unsigned char)((i + j) & 0xFF));
            (void)printf(" %02X", (int) ucData);
            if ((j == 3) || (j == 7) || (j == 11))
            {
                (void)printf(" :");
            }
        }
        (void)printf("\n");
        if ((i % 0x40) == 0x30)
        {
            (void)printf("\n        =====================================================\n");
        }
    }
    chgbank(0);
#endif
}
