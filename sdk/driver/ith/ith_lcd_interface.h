#ifndef ITE_ITH_LCD_INTERFACE_H
#define ITE_ITH_LCD_INTERFACE_H

#include <stdbool.h>
#include "ith/ith_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
//#define IT6122    //MIPI to LVDS
//#define IT6151    //MIPI to EDP
//#define GM8775    //MIPI to LVDS
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//for IT6122 defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef IT6122

//video mode
#define MIPI_EVENT_MODE	1

#define	MIPI_HSYNC_W		(8U)
#define MIPI_VSYNC_W		(2U)

#define MPRxDevAddr     (0x6CU << 0U) //0xD8

#define CLKTIMECNT		5
#define RCLKTIMECNT		1

#ifndef EN_INT_MODE
#define EN_INT_MODE		false 
#endif

#define RGB_24b         (0x3E)
#define RGB_30b         (0x0D)
#define RGB_36b         (0x1D)
#define RGB_18b         (0x1E)
#define RGB_18b_L       (0x2E)
#define YCbCr_16b       (0x2C)
#define YCbCr_20b       (0x0C)
#define YCbCr_24b       (0x1C)

#ifndef MPVidType
#define MPVidType		RGB_24b
#endif

#ifndef MPLaneNum
#define MPLaneNum		3U
#endif

#ifndef EnPNSwap
#define EnPNSwap		0U
#endif
#ifndef EnLaneSwap
#define EnLaneSwap		0U
#endif

#ifndef MPMode
#define MPMode			1	//1:Bypass mode 2:Normal mode
#endif
//system control
#define EnMPx1PCLK  	0  	// FALSE: 3/4(for 4 Lane) , 3/2(for 2 Lane) , 3(for 1 Lane) PCLK
#define InvMCLK  		1U 	//FALSE for solomon, if NonUFO, MCLK max = 140MHz with InvMCLK=TRUE
#define InvPCLK  		1U

#ifndef EnExtStdby
#define EnExtStdby		0U	//TRUE
#endif
// PPS option
#if(MPMode == 1)
#define  EnMBPM         1U    // BYPASS Mode
#define	 PREC_Update  	1U	// enable P-timing update
#define  MREC_Update  	1U
#else
#define  EnMBPM         0U   // NORMAL Mode
#define	 PREC_Update  	0U
#define  MREC_Update  	0U
#endif

#ifndef EnHReSync
#define EnHReSync  		0
#endif
#define EnVReSync  		0
#define EnFReSync  		0

#define	BPHSW	40U
#define	BPVSW	2U

#ifndef	EnPNSwap
#define EnPNSwap		0U
#endif
#ifndef EnLaneSwap
#define EnLaneSwap		0U
#endif
//VIDFMT
#define DisECCErr  		0

// PPI option
#define HSSetNum  		2U   //5
#define SkipStg  		2U   //5

#define EnSyncErr		1U

#if( MPLaneNum==0 )
#define EnDeSkew		false
#else
#define EnDeSkew		true
#endif

#if( MPVidType == RGB_18b )
#if( MPLaneNum == 3 )
#if( EnMPx1PCLK )
#define MPPCLKSel 0x02  //(1) 4-lane : MCLK = 1/1 PCLK
#else
#define MPPCLKSel 0x02  //(1) 4-lane : MCLK = 1/1 PCLK
#endif
# elif ( MPLaneNum == 1 )
#if( EnMPx1PCLK )
#define MPPCLKSel 0x05  //(6) 2-lane : MCLK = 1/1 PCLK
#else
#define MPPCLKSel 0x05  //(6) 2-lane : MCLK = 1/1 PCLK
#endif
# elif( MPLaneNum == 0 )
#if( EnMPx1PCLK )
#define MPPCLKSel 0x0b  //(7) 1-lane : MCLK = 1/1 PCLK
#else
#define MPPCLKSel 0x08  //(8) 1-lane : MCLK = 3/4 PCLK
#endif
#else
#define MPPCLKSel 0x03	
#endif

#else
#if( MPLaneNum == 3 )
#if( EnMPx1PCLK )
#define MPPCLKSel 0x03  //(0) 4-lane : MCLK = 1/1 PCLK
#else
#define MPPCLKSel 0x02  //(1) 4-lane : MCLK = 3/4 PCLK
#endif
# elif( MPLaneNum == 1 )
#if( EnMPx1PCLK )
#define MPPCLKSel 0x07  //(2) 2-lane : MCLK = 1/1 PCLK
#else
#define MPPCLKSel 0x05  //(3) 2-lane : MCLK = 3/4 PCLK
#endif
# elif( MPLaneNum == 0 )
#if( EnMPx1PCLK )
#define MPPCLKSel 0x0F  //(4) 1-lane : MCLK = 1/1 PCLK
#else
#define MPPCLKSel 0x0B  //(5) 1-lane : MCLK = 3/4 PCLK
#endif
#else
#define MPPCLKSel 0x03	
#endif

#endif

// for LVDS
#ifndef MAPVESA
#define MAPVESA 		1U  // (default = FALSE), FALSE:JEIDA(MAP1), TRUE:VESA(MAP3)
#endif
#ifndef En6bitout
#define En6bitout  		0U  // (default = FALSE)
#endif
#ifndef EnLVDMode
#define EnLVDMode  		1U  //LVDSTx Dual out enable
#endif

#ifndef EnLVL0LnSwap
#define EnLVL0LnSwap	0U
#endif
#ifndef EnLVL1LnSwap
#define EnLVL1LnSwap	0U
#endif
#ifndef EnLVL0PNSwap
#define EnLVL0PNSwap	0U
#endif
#ifndef EnLVL1PNSwap
#define EnLVL1PNSwap	0U
#endif
#ifndef EnLVLinkSwap
#define	EnLVLinkSwap	0U
#endif

#ifndef LVSwingLevel
#define LVSwingLevel	6U
#endif

//LVDS Tx SSC Setting
#define EnLVTxSSC  			false
#define LVSDM 	 			0x147bU //0x147b; 10000ppm    //0x3333; 25000ppm   //0x0a3d; 5000ppm  //0x3333; 25000ppm/162MHz
#define LVSDMINC 			0x009U  //0x0x009;              //0x016;             //0x005;           //0x014;
#ifndef EnHighPCLK
#define EnHighPCLK			0U  //for daul LVDS
#endif
#define EnSP_S1				false  //for daul LVDS

#define EnSSCBufConcat  	false  //enable sscbufB used when LVDS single Link only
#define EnSSCBufAutoRst 	true
#define EnSSCPLL 			0U

#define EnLVVidRecInt 		true   //default: TRUE
#define EnMPRxPRBS	 		false

#define EnCalcCLK			false

#define STA_CHANGE(a, b, c)				do{		(a) = ((a) & (~(b))) | ((c) & (b));	}while(0)

#define PROG 1
#define INTR 0
#define Vneg 0
#define Hneg 0
#define Vpos 1
#define Hpos 1
//-----------------------------------------------------------------------------
//g_u8SysSts
#define FW_CTL_TXOUT_SHIFT					(0)
#define FW_CTL_TXOUT_MASK					(0x01 << FW_CTL_TXOUT_SHIFT)
#define FW_CTL_TXOUT_ENABLE					(0x01 << FW_CTL_TXOUT_SHIFT)
#define FW_CTL_TXOUT_DISABLE					0x00

#define STANDBY_MODE_SHIFT				(1U)
#define STANDBY_MODE_MASK				(0x01U << STANDBY_MODE_SHIFT)
#define STANDBY_MODE_ENABLE				(0x01U << STANDBY_MODE_SHIFT)
#define STANDBY_MODE_DISABLE			0x00U
#endif //IT6122

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for it6151 defines start
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef IT6151

//video mode
#define MIPI_EVENT_MODE	1

#define PANEL_RESOLUTION_1920x1080p60_4LANE_24B

#define MIPI_4_LANE		(3)
#define MIPI_3_LANE		(2)
#define MIPI_2_LANE		(1)
#define MIPI_1_LANE		(0)

// MIPI Packed Pixel Stream
#define RGB_24b         (0x3E)
#define RGB_30b         (0x0D)
#define RGB_36b         (0x1D)
#define RGB_18b_P       (0x1E)
#define RGB_18b_L       (0x2E)
#define YCbCr_16b       (0x2C)
#define YCbCr_20b       (0x0C)
#define YCbCr_24b       (0x1C)

// DPTX reg62[3:0]
#define B_DPTXIN_6Bpp   (0U)
#define B_DPTXIN_8Bpp   (1U)
#define B_DPTXIN_10Bpp  (2U)
#define B_DPTXIN_12Bpp  (3U)

#define B_AutoBR		(0x00)
#define B_LBR    		(0x01)
#define B_HBR    		(0x80)

#define DP_4_LANE 		(3)
#define DP_2_LANE 		(1)
#define DP_1_LANE 		(0)

#define B_SSC_ENABLE   	(1U)
#define B_SSC_DISABLE   (0U)

#define H_Neg			(0)
#define H_Pos			(1)

#define V_Neg			(0)
#define V_Pos			(1)

#define En_UFO			(1)
#define H_ReSync		(0x01)
#define V_ReSync		(0x02)
#define F_ReSync		(0x10)

// CONFIGURE
#define DPTX_SSC_SETTING	(B_SSC_ENABLE)//(B_SSC_DISABLE)
#define MP_MCLK_INV			(1U)
#define MP_CONTINUOUS_CLK	(1U)
#define MP_LANE_DESKEW		(1U)
#define MP_LANE_SWAP		(0U)
#define MP_PN_SWAP			(0U)

#define DP_PN_SWAP			(0U)
#define DP_AUX_PN_SWAP		(0U)
#define DP_LANE_SWAP		(0U)
#define DP_BPP				(B_DPTXIN_8Bpp)

#define LVDS_LANE_SWAP		(0U)
#define LVDS_PN_SWAP		(0U)
#define LVDS_DC_BALANCE		(0U)

#define LVDS_6BIT			(0U) // '0' for 8 bit, '1' for 6 bit
#define VESA_MAP		    (1U) // '0' for JEIDA , '1' for VESA MAP

#define INT_MASK			(3)
#define MIPI_RECOVER		(1)

#define	MIPI_HSYNC_W		(8U)
#define MIPI_VSYNC_W		(2U)

#define MIPI_FFRdStg		(0x10U)

#define REDUCE_SWING		(0)

#if (REDUCE_SWING == 0)
#define REG_C2				(0x47)
#else
#define REG_C2				(0x41)
#endif

#define AUTO_OUTPUT			(0)
#define TIMER_CNT			(0x0A)

#define DP_I2C_ADDR 	(0x5CU << 0U) //0xB8
#define MIPI_I2C_ADDR 	(0x6CU << 0U) //0xD8

struct PanelInfoStr{
    unsigned char	ucVic;
    unsigned short	usPWidth;
    unsigned char	ucDpLanes;
    unsigned char	ucDpBR;
    unsigned char	ucMpLanes;
    unsigned char	ucMpHPol;
    unsigned char	ucMpVPol;
    unsigned char	ucUFO;
    unsigned char	ucMpFmt;
    unsigned char	ucDpReSync;
    unsigned char	ucMpReSync;
    unsigned char	ucMpClkDiv;
    unsigned char	ucHighPclk;
    unsigned char	ucIntMask;
    unsigned char	ucHSSetNum;
};

#endif //IT6151


#ifdef __cplusplus
}
#endif

#endif
