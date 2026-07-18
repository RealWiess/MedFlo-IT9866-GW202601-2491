#ifndef CAN_REG_H
#define CAN_REG_H

#include "ith/ith_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//Constant Definition
//=============================================================================
//HW Parameter Description
#define CFG_ACF_NUMBER            0x10
#define CFG_STB_ENABLE            0x1


//=============================================================================
//Setting Register Define
//=============================================================================
// (*) :  register can only be written if bit RESET in register CFG_STAT is set.

//Receive Buffer Registers // 80byte
#define CAN_RB_IDENTIFIER_REG             0x00U
//
#define CAN_RB_CONTROL_REG                0x04U
#define CAN_RB_DLC                          (N04_BITS_MSK<< 0U)
#define CAN_RB_DLC_SHIFT                     0U
#define CAN_RB_BRS                          (N01_BITS_MSK<< 4U)
#define CAN_RB_BRS_SHIFT                     4U
#define CAN_RB_EDL                          (N01_BITS_MSK<< 5U)
#define CAN_RB_EDL_SHIFT                     5U
#define CAN_RB_RTR                          (N01_BITS_MSK<< 6U)
#define CAN_RB_RTR_SHIFT                     6U
#define CAN_RB_IDE                          (N01_BITS_MSK<< 7U)
#define CAN_RB_IDE_SHIFT                     7U
#define CAN_RB_TX                           (N01_BITS_MSK<<12U)
#define CAN_RB_TX_SHIFT                      12U
#define CAN_RB_KOER                         (N03_BITS_MSK<<13U)
#define CAN_RB_KOER_SHIFT                    13U
//
#define CAN_RB_DATA0_REG                  0x08U  /*if TTCAN = 1 , bit (15:0) is CYCLE TIME*/
#define CAN_RB_DATA1_REG                  0x0cU
#define CAN_RB_DATA2_REG                  0x10U
#define CAN_RB_DATA3_REG                  0x14U
#define CAN_RB_DATA4_REG                  0x18U
#define CAN_RB_DATA5_REG                  0x1cU
#define CAN_RB_DATA6_REG                  0x20U
#define CAN_RB_DATA7_REG                  0x24U
#define CAN_RB_DATA8_REG                  0x28U
#define CAN_RB_DATA9_REG                  0x2cU
#define CAN_RB_DATA10_REG                 0x30U
#define CAN_RB_DATA11_REG                 0x34U
#define CAN_RB_DATA12_REG                 0x38U
#define CAN_RB_DATA13_REG                 0x3cU
#define CAN_RB_DATA14_REG                 0x40U
#define CAN_RB_DATA15_REG                 0x44U
//
#define CAN_RB_RTS0_REG                   0x48U
//
#define CAN_RB_RTS1_REG                   0x4cU

/*Transmit Buffer Registers 72byte*/
#define CAN_TB_IDENTIFIER_REG             0x50U
#define CAN_TB_CONTROL_REG                0x54U
#define CAN_TB_DLC                          (N04_BITS_MSK<< 0U)
#define CAN_TB_DLC_SHIFT                     0U
#define CAN_TB_BRS                          (N01_BITS_MSK<< 4U)
#define CAN_TB_BRS_SHIFT                     4U
#define CAN_TB_EDL                          (N01_BITS_MSK<< 5U)
#define CAN_TB_EDL_SHIFT                     5U
#define CAN_TB_RTR                          (N01_BITS_MSK<< 6U)
#define CAN_TB_RTR_SHIFT                     6U
#define CAN_TB_IDE                          (N01_BITS_MSK<< 7U)
#define CAN_TB_IDE_SHIFT                     7U
#define CAN_TB_DATA0_REG                  0x58U
#define CAN_TB_DATA1_REG                  0x5cU
#define CAN_TB_DATA2_REG                  0x60U
#define CAN_TB_DATA3_REG                  0x64U
#define CAN_TB_DATA4_REG                  0x68U
#define CAN_TB_DATA5_REG                  0x6cU
#define CAN_TB_DATA6_REG                  0x70U
#define CAN_TB_DATA7_REG                  0x74U
#define CAN_TB_DATA8_REG                  0x78U
#define CAN_TB_DATA9_REG                  0x7cU
#define CAN_TB_DATA10_REG                 0x80U
#define CAN_TB_DATA11_REG                 0x84U
#define CAN_TB_DATA12_REG                 0x88U
#define CAN_TB_DATA13_REG                 0x8cU
#define CAN_TB_DATA14_REG                 0x90U
#define CAN_TB_DATA15_REG                 0x94U

/* Transmission Time Stamp TTS (0x98 to 0x9f) */
#define CAN_TT_STAMP0_REG                 0x98U
#define CAN_TT_STAMP1_REG                 0x9cU
//
#define CAN_CFG_STAT_REG                  0xa0U
#define CAN_BUSOFF                          (N01_BITS_MSK<< 0U)//Bus Off
#define CAN_TACTIVE                         (N01_BITS_MSK<< 1U)//Transmission ACTIVE
#define CAN_RACTIVE                         (N01_BITS_MSK<< 2U)//Reception ACTIVE
#define CAN_TSSS                            (N01_BITS_MSK<< 3U)//Transmission Secondary Single Shot mode for STB
#define CAN_TPSS                            (N01_BITS_MSK<< 4U)//Transmission Primary Single Shot mode for PTB
#define CAN_LBMI                            (N01_BITS_MSK<< 5U)//Loop Back Mode, Internal
#define CAN_LBME                            (N01_BITS_MSK<< 6U)//Loop Back Mode, External
#define CAN_RESET                           (N01_BITS_MSK<< 7U)//RESET request bit  
#define CAN_TCMD_REG                      0xa1U
#define CAN_TSA                             (N01_BITS_MSK<< 8U)//Transmit Secondary Abort
#define CAN_TSA_SHIFT                        8U
#define CAN_TSALL                           (N01_BITS_MSK<< 9U)//Transmit Secondary ALL frames
#define CAN_TSALL_SHIFT                      9U
#define CAN_TSONE                           (N01_BITS_MSK<<10U)//Transmit Secondary ONE frame
#define CAN_TSONE_SHIFT                      10U
#define CAN_TPA                             (N01_BITS_MSK<<11U)//Transmit Primary Abort
#define CAN_TPA_SHIFT                        11U
#define CAN_TPE                             (N01_BITS_MSK<<12U)//Transmit Primary Enable
#define CAN_TPE_SHIFT                        12U
#define CAN_STBY                            (N01_BITS_MSK<<13U)//Transceiver Standby Mode
#define CAN_STBY_SHIFT                       13U
#define CAN_LOM                             (N01_BITS_MSK<<14U)//Listen Only Mode
#define CAN_LOM_SHIFT                        14U
#define CAN_TBSEL                           (N01_BITS_MSK<<15U)//Transmit Buffer Select
#define CAN_TBSEL_SHIFT                      15U
#define CAN_TCTRL_REG                     0xa2U
#define CAN_TSSTAT                          (N02_BITS_MSK<<16U)//Transmission Secondary STATus bits
#define CAN_TSSTAT_SHIFT                     16U
#define CAN_TTTBM                           (N01_BITS_MSK<<20U)//TTCAN Transmit Buffer Mode
#define CAN_TTTBM_SHIFT                      20U
#define CAN_TSMODE                          (N01_BITS_MSK<<21U)//Transmit buffer Secondary operation MODE
#define CAN_TSMODE_SHIFT                     21U
#define CAN_TSNEXT                          (N01_BITS_MSK<<22U)//Transmit buffer Secondary NEXT
#define CAN_TSNEXT_SHIFT                     22U
#define CAN_FD_ISO                          (N01_BITS_MSK<<23U)//CAN FD ISO mode //(*)
#define CAN_FD_ISO_SHIFT                     23U
#define CAN_RCTRL_REG                     0xa3U
#define CAN_RSTAT                           (N02_BITS_MSK<<24U)//Receive buffer STATus
#define CAN_RSTAT_SHIFT                      24U
#define CAN_RBALL                           (N01_BITS_MSK<<27U)//Receive Buffer stores ALL data frames
#define CAN_RBALL_SHIFT                      27U
#define CAN_RREL                            (N01_BITS_MSK<<28U)//Receive buffer RELease
#define CAN_RREL_SHIFT                       28U
#define CAN_ROV                             (N01_BITS_MSK<<29U)//Receive buffer OVerflow
#define CAN_ROV_SHIFT                        29U
#define CAN_ROM                             (N01_BITS_MSK<<30U)//Receive buffer Overflow Mode
#define CAN_ROM_SHIFT                        30U
#define CAN_SACK                            (0x80000000UL)//Self-ACKnowledge
#define CAN_SACK_SHIFT                       31U 
//
#define CAN_RTIE_REG                      0xa4U
#define CAN_TSFF                            (N01_BITS_MSK<< 0U)//Transmit Secondary buffer Full Flag
#define CAN_EIE                             (N01_BITS_MSK<< 1U)//Error Interrupt Enable
#define CAN_TSIE                            (N01_BITS_MSK<< 2U)//Transmission Secondary Interrupt Enable
#define CAN_TPIE                            (N01_BITS_MSK<< 3U)//Transmission Primary Interrupt Enable
#define CAN_RAFIE                           (N01_BITS_MSK<< 4U)//RB Almost Full Interrupt Enable
#define CAN_RFIE                            (N01_BITS_MSK<< 5U)//RB Full Interrupt Enable
#define CAN_ROIE                            (N01_BITS_MSK<< 6U)//RB Overrun Interrupt Enable
#define CAN_RIE                             (N01_BITS_MSK<< 7U)//Receive Interrupt Enable
#define CAN_RTIF_REG                      0xa5U
#define CAN_AIF                             (N01_BITS_MSK<< 8U)//Abort Interrupt Flag
#define CAN_EIF                             (N01_BITS_MSK<< 9U)//Error Interrupt Flag
#define CAN_TSIF                            (N01_BITS_MSK<<10U)//Transmission Secondary Interrupt Flag
#define CAN_TPIF                            (N01_BITS_MSK<<11U)//Transmission Primary Interrupt Flag
#define CAN_RAFIF                           (N01_BITS_MSK<<12U)//RB Almost Full Interrupt Flag
#define CAN_RFIF                            (N01_BITS_MSK<<13U)//RB Full Interrupt Flag
#define CAN_ROIF                            (N01_BITS_MSK<<14U)//RB Overrun Interrupt Flag
#define CAN_RIF                             (N01_BITS_MSK<<15U)//Receive Interrupt Flag
#define CAN_ERRINT_REG                    0xa6U
#define CAN_BEIF                            (N01_BITS_MSK<<16U)//Bus Error Interrupt Flag
#define CAN_BEIE                            (N01_BITS_MSK<<17U)//Bus Error Interrupt Enable
#define CAN_ALIF                            (N01_BITS_MSK<<18U)//Arbitration Lost Interrupt Flag
#define CAN_ALIE                            (N01_BITS_MSK<<19U)//Arbitration Lost Interrupt Enable
#define CAN_EPIF                            (N01_BITS_MSK<<20U)//Error Passive Interrupt Flag
#define CAN_EPIE                            (N01_BITS_MSK<<21U)//Error Passive Interrupt Enable
#define CAN_EPASS                           (N01_BITS_MSK<<22U)//Error Passive mode active
#define CAN_EWARN                           (N01_BITS_MSK<<23U)//Error WARNing limit reached
#define CAN_LIMIT_REG                     0xa7U
#define CAN_EWL                             (N04_BITS_MSK<<24U)//Programmable Error Warning Limit
#define CAN_AFWL                            (N04_BITS_MSK<<28U)//receive buffer Almost Full Warning Limit
//
#define CAN_S_Seg_1_REG                   0xa8U //(*)
#define CAN_S_Seg_1                         (N08_BITS_MSK<< 0U)//Bit Timing Segment 1(slow speed)
#define CAN_S_Seg_1_SHIFT                    0U
#define CAN_S_Seg_2_REG                   0xa9U //(*)
#define CAN_S_Seg_2                         (N07_BITS_MSK<< 8U)//Bit Timing Segment 2(slow speed)
#define CAN_S_Seg_2_SHIFT                    8U
#define CAN_S_SJW_REG                     0xaaU //(*)
#define CAN_S_SJW                           (N07_BITS_MSK<<16U)//Synchronization Jump Width(slow speed)
#define CAN_S_SJW_SHIFT                      16U
#define CAN_S_PRESC_REG                   0xabU //(*)
#define CAN_S_PRESC                         (N08_BITS_MSK<<24U)//Prescaler  slow speed
#define CAN_S_PRESC_SHIFT                    24U
//
#define CAN_F_Seg_1_REG                   0xacU //(*)
#define CAN_F_Seg_1                         (N05_BITS_MSK<< 0U)//Bit Timing Segment 1 (fast speed)
#define CAN_F_Seg_1_SHIFT                    0U
#define CAN_F_Seg_2_REG                   0xadU //(*)
#define CAN_F_Seg_2                         (N04_BITS_MSK<< 8U)//Bit Timing Segment 2 (fast speed)
#define CAN_F_Seg_2_SHIFT                    8U
#define CAN_F_SJW_REG                     0xaeU //(*)
#define CAN_F_SJW                           (N04_BITS_MSK<<16U)//Synchronization Jump Width (fast speed)
#define CAN_F_SJW_SHIFT                      16U
#define CAN_F_PRESC_REG                   0xafU //(*)
#define CAN_F_PRESC                         (N08_BITS_MSK<<24U)//Prescaler fast speed
#define CAN_F_PRESC_SHIFT                    24U
//
#define CAN_EALCAP_REG                    0xb0U
#define CAN_ALC                             (N05_BITS_MSK<< 0U)//Arbitration Lost Capture
#define CAN_ALC_SHIFT                        0U
#define CAN_KOER                            (N03_BITS_MSK<< 5U)//Kind Of ERror
#define CAN_KOER_SHIFT                       5U

#define CAN_TDC_REG                       0xb1U //(*)
#define CAN_SSPOFF                          (N07_BITS_MSK<< 8U)//Secondary Sample Point OFFset
#define CAN_SSPOFF_SHIFT                     8U
#define CAN_TDCEN                           (N01_BITS_MSK<<15U)//Transmitter Delay Compensation ENable
#define CAN_TDCEN_SHIFT                      15U

#define CAN_RECNT_REG                     0xb2U
#define CAN_RECNT                           (N08_BITS_MSK<<16U)//Receive Error CouNT (number of errors during reception)
#define CAN_RECNT_SHIFT                     16U
#define CAN_TECNT_REG                     0xb3U
#define CAN_TECNT                           (0xFF000000UL)//Transmit Error CouNT (number of errors during transmission)
#define CAN_TECNT_SHIFT                     24U
//
#define CAN_ACFCTRL_REG                   0xb4U
#define CAN_ACFADR                          (N04_BITS_MSK<< 0U)//acceptance filter address
#define CAN_SELMASK                         (N01_BITS_MSK<< 5U)//acceptance filter address
#define CAN_TIMECFG_REG                   0xb5U
#define CAN_TIMEEN                          (N01_BITS_MSK<< 8U)//TIME-stamping ENable
#define CAN_TIMEEN_SHIFT                     8U
#define CAN_TIMEPOS                         (N01_BITS_MSK<< 9U)//TIME-stamping POSition
#define CAN_TIMEPOS_SHIFT                    9U
#define CAN_ACF_EN_0_REG                  0xb6U
#define CAN_AE0_x                           (N08_BITS_MSK<<16U)//Acceptance filter Enable AE7 - AE0
#define CAN_AE0_x_SHIFT                      16U
#define CAN_ACF_EN_1_REG                  0xb7U
#define CAN_AE1_x                           (N08_BITS_MSK<<24U)//Acceptance filter Enable AE15 -AE8
#define CAN_AE1_x_SHIFT                      24U
//
#define CAN_ACF_0_REG                     0xb8U //(*)
#define CAN_ACODE_MASK_0                    (N08_BITS_MSK<< 0U)//Acceptance CODE or MASK
#define CAN_ACF_1_REG                     0xb9U //(*)
#define CAN_ACODE_MASK_1                    (N08_BITS_MSK<< 8U)//Acceptance CODE or MASK
#define CAN_ACF_2_REG                     0xbaU //(*)
#define CAN_ACODE_MASK_2                    (N08_BITS_MSK<<16U)//Acceptance CODE or MASK
#define CAN_ACF_3_REG                     0xbbU //(*)
#define CAN_ACODE_MASK_3                    (N08_BITS_MSK<<24U)//Acceptance CODE or MASK
//
#define CAN_VER_0_REG                     0xbcU
#define CAN_VER_0                           (N08_BITS_MSK<< 0U)//Version of CAN-CTRL,VER_0 the minor version
#define CAN_VER_1_REG                     0xbdU
#define CAN_VER_1                           (N08_BITS_MSK<< 8U)//Version of CAN-CTRL,VER_1 holds the major version
#define CAN_TBSLOT_REG                    0xbeU
#define CAN_TBPTR                           (N06_BITS_MSK<<16U)//Pointer to a TB message slot
#define CAN_TBPTR_SHIFT                      16U
#define CAN_TBF                             (N01_BITS_MSK<<22U)//set TB slot to "Filled"
#define CAN_TBF_SHIFT                        22U
#define CAN_TBE                             (N01_BITS_MSK<<23U)//set TB slot to "Empty"
#define CAN_TBE_SHIFT                        23U
#define CAN_TTCFG_REG                     0xbfU
#define CAN_TTEN                            (N01_BITS_MSK<<24U)//Time Trigger Enable
#define CAN_TTEN_SHIFT                       24U
#define CAN_T_PRESC                         (N02_BITS_MSK<<25U)//TTCAN Timer PRESCaler
#define CAN_T_PRESC_SHIFT                    25U
#define CAN_TTIF                            (N01_BITS_MSK<<27U)//Time Trigger Interrupt Flag
#define CAN_TTIE                            (N01_BITS_MSK<<28U)//Time Trigger Interrupt Enable
#define CAN_TEIF                            (N01_BITS_MSK<<29U)//Trigger Error Interrupt Flag
#define CAN_WTIF                            (N01_BITS_MSK<<30U)//Watch Trigger Interrupt Flag
#define CAN_WTIE                            (N01_BITS_MSK<<31U)//Watch Trigger Interrupt Enable
//
#define CAN_REF_MSG_0_REG                 0xc0U
#define CAN_REF_ID                          (0xFFFFFFFUL   << 0U)//REFerence message IDentifier
#define CAN_REF_IDE                         (0x80000000UL)//REFerence message IDentifier  1:REF_ID(28:0) 0:REF_ID(10:0)
//
#define CAN_TRIG_CFG_0_REG                0xc4U
#define CAN_TTPTR                           (N06_BITS_MSK<< 0U)//Transmit Trigger TB slot Pointer
#define CAN_TRIG_CFG_1_REG                0xc5U
#define CAN_TTYPE                           (N03_BITS_MSK<< 8U)//Trigger Type
#define CAN_TTYPE_SHIFT                      8U
#define CAN_TEW                             (N04_BITS_MSK<<12U)//Transmit Enable Window
#define CAN_TEW_SHIFT                        12U
#define CAN_TT_TRIG_0_REG                 0xc6U
#define CAN_TT_TRIG0                        (N08_BITS_MSK<<16U)//Trigger Time
#define CAN_TT_TRIG0_SHIFT                   16U
#define CAN_TT_TRIG_1_REG                 0xc7U
#define CAN_TT_TRIG1                        (0xFF000000UL)//Trigger Time
#define CAN_TT_TRIG1_SHIFT                   24U
//
#define CAN_TT_WTRIG_0_REG                0xc8U
#define CAN_TT_WTRIG0                       (N08_BITS_MSK<< 0U)//Watch Trigger Time
#define CAN_TT_WTRIG0_SHIFT                  0U
#define CAN_TT_WTRIG_1_REG                0xc9U
#define CAN_TT_WTRIG1                       (N08_BITS_MSK<< 8U)//Watch Trigger Time
#define CAN_TT_WTRIG1_SHIFT                  8U

#define CAN_TIMESTAMP_CLK_REG             0xf0U

//=============================================================================
//Status REGister Define
//=============================================================================

//=============================================================================
//Struct Definition
//=============================================================================

//=============================================================================
//Private Function Definition
//=============================================================================

//=============================================================================
//Public Function Definition
//=============================================================================

#ifdef __cplusplus
}
#endif

#endif
