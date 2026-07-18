#ifndef CAN_HW_H
#define CAN_HW_H

#include <assert.h>
#include "ite/itp.h"
#include "can_reg.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CAN0_BASE_ADDRESS 0XDC100000
#define CAN1_BASE_ADDRESS 0XDC200000

#define IDFILTER_BUFFER   2

//=============================================================================
//                Structure Definition
//=============================================================================

// enum
typedef enum
{
    Error_Enable            = 1,
    TS_Enable               = 2,
    TP_Enable               = 3,
    RB_Almost_Full_Enable   = 4,
    RB_Full_Enable          = 5,
    RB_Overrun_Enable       = 6,
    Receive_Enable          = 7,
    Bus_Error_Enable        = 17,
    Arbitration_Lost_Enable = 19,
    Error_Passive_Enable    = 21,
    // Time Trigged Mode
    // Time_Trigger_Enable = 28,
    // Watch_Trigger_Enable = 31,
} CAN_INTERRUPTS_EN;

typedef enum
{
    Abort_Interrupt_Flag            = 8, // from TPA and TSA
    Error_Interrupt_Flag            = 9,
    TS_Interrupt_Flag               = 10,
    TP_Interrupt_Flag               = 11,
    RB_Almost_Full_Interrupt_Flag   = 12,
    RB_Full_Interrupt_Flag          = 13,
    RB_Overrun_Interrupt_Flag       = 14,
    Receive_Interrupt_Flag          = 15,
    Bus_Error_Interrupt_Flag        = 16,
    Arbitration_Lost_Interrupt_Flag = 18,
    Error_Passive_Interrupt_Flag    = 20,
    // Time Trigged Mode
    // Time_Trigger_Interrupt_Flag = 27,
    // Trigger_Error_Interrupt_Flag = 29,
    // Watch_Trigger_Interrupt_Flag = 30,
} CAN_INTERRUPTS_FLAG;

typedef enum
{
    BUFF_EMPTY     = 0,
    BUFF_LESS_HALF = 1,
    BUFF_MORE_HALF = 2,
    BUFF_FULL      = 3,
} CAN_BUFF_STATUS;

typedef enum
{
    CAN_NO_ERROR              = 0,
    CAN_BIT_ERROR             = 1,
    CAN_FORM_ERROR            = 2,
    CAN_STUFF_ERROR           = 3,
    CAN_ACKNOWLEDGEMENT_ERROR = 4,
    CAN_CRC_ERROR             = 5,
    CAN_OTHER_ERROR           = 6,
} CAN_ERROR_TYPE;

// ERROR STATE
typedef enum
{
    ERROR_ACTIVE,
    ERROR_PASSIVE,
    BUS_OFF,
} CAN_ERROR_STATE;

typedef enum
{
    TTCAN_Immediate_Trigger      = 0,
    TTCAN_Time_Trigger           = 1,
    TTCAN_OneShot_Trans_Trigger  = 2,
    TTCAN_Transmit_Start_Trigger = 3,
    TTCAN_Transmit_Stop_Trigger  = 4,
    TTCAN_No_Action              = 5,
} CAN_TRIGGER_TYPE;

typedef enum
{
    protocol_FD_BOSCH = 0, // Bosch CAN FD (non-ISO) mode
    protocol_FD_ISO   = 1, // ISO CAN FD mode (ISO 11898-1:2015)
    protocol_CAN_2_0B,
} CAN_PROTOCOL;

typedef enum
{
    CAN_SRCCLK_20M = 20000000,
    CAN_SRCCLK_40M = 40000000,
    CAN_SRCCLK_60M = 60000000,
    CAN_SRCCLK_80M = 80000000,
} CAN_SOURCECLK;

//=============================================================================
/**
This type is combine Slow Bitrate(arbitration phase) and Fast Bitrate(Data
phase). CAN2.0B only use slow bitrate(nominal bit rate) for the compelte
frame,don't care fast bitrate.
**/
//=============================================================================
typedef enum
{
    CAN_500K_1M,   // Slow Bitrate 500K  / Fast Bitrate 1M
    CAN_500K_2M,   // Slow Bitrate 500K  / Fast Bitrate 2M
    CAN_500K_4M,   // Slow Bitrate 500K  / Fast Bitrate 4M
    CAN_500K_5M,   // Slow Bitrate 500K  / Fast Bitrate 5M
    CAN_250K_500K, // Slow Bitrate 250K  / Fast Bitrate 500K
    CAN_250K_1M,   // Slow Bitrate 250K  / Fast Bitrate 1M
    CAN_250K_1M5,  // Slow Bitrate 250K  / Fast Bitrate 1.5M
    CAN_250K_2M,   // Slow Bitrate 250K  / Fast Bitrate 2M
    CAN_250K_4M,   // Slow Bitrate 250K  / Fast Bitrate 4M
    CAN_1000K_4M,  // Slow Bitrate 1000K / Fast Bitrate 4M
    CAN_125K_500K, // Slow Bitrate 125K  / Fast Bitrate 500K
    CAN_100K,      // Slow Bitrate 100K
    CAN_50K,       // Slow Bitrate 50K
    CAN_20K,       // Slow Bitrate 20K
    CAN_USER_DEFINED,
} CAN_BAUDRATE;

typedef enum
{
    CAN_ACFE0 = 0,
    CAN_ACFE1,
    CAN_ACFE2,
    CAN_ACFE3,
    CAN_ACFE4,
    CAN_ACFE5,
    CAN_ACFE6,
    CAN_ACFE7,
    CAN_ACFE8,
    CAN_ACFE9,
    CAN_ACFE10,
    CAN_ACFE11,
    CAN_ACFE12,
    CAN_ACFE13,
    CAN_ACFE14,
    CAN_ACFE15,
} CAN_ACFE;

typedef enum
{
    CAN_ACODE0 = 0,
    CAN_ACODE1,
    CAN_ACODE2,
    CAN_ACODE3,
    CAN_ACODE4,
    CAN_ACODE5,
    CAN_ACODE6,
    CAN_ACODE7,
    CAN_ACODE8,
    CAN_ACODE9,
    CAN_ACODE10,
    CAN_ACODE11,
    CAN_ACODE12,
    CAN_ACODE13,
    CAN_ACODE14,
    CAN_ACODE15,
} CAN_ACODE;

typedef enum
{
    CAN_AMASK0 = 0,
    CAN_AMASK1,
    CAN_AMASK2,
    CAN_AMASK3,
    CAN_AMASK4,
    CAN_AMASK5,
    CAN_AMASK6,
    CAN_AMASK7,
    CAN_AMASK8,
    CAN_AMASK9,
    CAN_AMASK10,
    CAN_AMASK11,
    CAN_AMASK12,
    CAN_AMASK13,
    CAN_AMASK14,
    CAN_AMASK15,
} CAN_AMASK;

typedef struct
{
    uint32_t Prescaler;
    uint32_t Bit_Time;
    uint32_t Seg_1;
    uint32_t Seg_2;
    uint32_t SJW;
    uint32_t SSPOFF;
} CAN_BTATTR; // Bit Time Attributes

typedef struct
{
    bool     FilterEnable;
    uint32_t ACode;
    uint32_t AMask;
    bool     AIDEE;
    bool     AIDE;
} CAN_FILTEROBJ;

typedef struct
{
    uint32_t      Instance;       // device instance
    uint32_t      ADDR;           // hardware baseaddress
    uint32_t      InterruptTable; // Interrupts enable or not
    CAN_BAUDRATE  BaudRate;       // bus baud rate
    CAN_SOURCECLK SourceClock;    // can clk
    CAN_PROTOCOL  ProtocolType;   // can protocol
    void *        InterruptHD;    // interrupt handler
    /* debug mode */
    bool ExternalLoopBackMode; // Loop Back Mode, External(self-tests used)
    bool InternalLoopBackMode; // Loop Back Mode, Internal(self-tests used)
    bool ListenOnlyMode;       // Listen Only Mode
    CAN_FILTEROBJ * TPtr;
    /*user define*/
    CAN_BTATTR      SlowBitRate; // user config baudrate
    CAN_BTATTR      FastBitRate; // user config baudrate(CAN FD)
} CAN_HANDLE;

//=============================================================================
//                Public Function Definition
//=============================================================================
void     ithCANEnableIntr (CAN_HANDLE * base, CAN_INTERRUPTS_EN intr);
void     ithCANDisableIntr (CAN_HANDLE * base, CAN_INTERRUPTS_EN intr);
void     ithCANDisableIntrAll (CAN_HANDLE * base);
void     ithCANClearIntrFlag (CAN_HANDLE * base, CAN_INTERRUPTS_FLAG flag);
uint32_t ithCANGetIntrFlag (CAN_HANDLE * base);
uint32_t ithTTCANGetIntrFlag (CAN_HANDLE * base);

//=============================================================================
/**
 * CAN-CTRL Set  Error Warning Limit
 * @param[in]   base address
 * @param[in]   EWL : programmable  Error Warning Limit value:8~128
 * @return none
 */
//=============================================================================
void     ithCANSetEWL (CAN_HANDLE * base, uint32_t EWL);

//=============================================================================
/**
 * CAN-CTRL Reset
 * @param[in]   base address
 * @param[in]   0 - no local reset of CAN-CTRL, 1 - The host controller performs
 * a local reset of CAN-CTRL.
 * @return none
 */
//=============================================================================
void     ithCANSWReset (CAN_HANDLE * base, bool OnOff);

//=============================================================================
/**
 * CAN-CTRL Set Bus Off
 * @param[in]   base address
 * @param[in]   write 1 to BUSOFF will reset TECNT and RECNT
 * @return none
 */
//=============================================================================
void     ithCANSetBusOFF (CAN_HANDLE * base, bool OnOff);

//=============================================================================
/**
 * CAN-CTRL Loop Back Mode Setting
 * @param[in]   base address
 * @param[in]   external 0 - Disabled, 1 - Enabled
 * @param[in]   internal 0 - Disabled, 1 - Enabled
 * @return none
 */
//=============================================================================
void     ithCANSetLoopBack (CAN_HANDLE * base, bool external, bool internal);
//=============================================================================
/**
 * CAN-CTRL Listen Only Mode Setting
 * @param[in]   base address
 * @param[in]   0 - Disabled, 1 - Enabled
 * @return none
 */
//=============================================================================
void     ithCANSetListenOnlyMode (CAN_HANDLE * base, bool onoff);
//=============================================================================
/**
 * CAN-CTRL Transceiver Standby Mode Setting
 * @param[in]   base address
 * @param[in]   0 - Disabled, 1 - Enabled
 * @return none
 */
//=============================================================================
void     ithCANSetTransStandbyMode (CAN_HANDLE * base, bool onoff);
//=============================================================================
/**
 * CAN-CTRL Transceiver PTB Single Shot Mode Setting
 * @param[in]   base address
 * @param[in]   0 - Disabled, 1 - Enabled
 * @return none
 */
//=============================================================================
void     ithCANSetTransPTBSSMode (CAN_HANDLE * base, bool onoff);
//=============================================================================
/**
 * CAN-CTRL Transceiver STB Single Shot Mode Setting
 * @param[in]   base address
 * @param[in]   0 - Disabled, 1 - Enabled
 * @return none
 */
//=============================================================================
void     ithCANSetTransSTBSSMode (CAN_HANDLE * base, bool onoff);
//=============================================================================
/**
 * CAN-CTRL Transceiver STB Operation Mode
 * @param[in]   base address
 * @param[in]   0 - FIFO mode, 1 - ID priority mode
 * @return none
 */
//=============================================================================
void     ithCANSetSTBMode (CAN_HANDLE * base, bool mode);
//=============================================================================
/**
 * CAN FD ISO mode
 * @param[in]   base address
 * @param[in]   0 - Bosch CAN FD (non-ISO) mode , 1 - ISO mode
 * @return none
 */
//=============================================================================
void     ithCANSetFDISO (CAN_HANDLE * base);
//=============================================================================
/**
 * CAN Bit Timing Setting
 * @param[in]   base address
 * @param[in]   s_bt Slow speed is used for CAN 2.0
 * @return none
 */
//=============================================================================
void     ithCANSetBitRate (CAN_HANDLE * base, CAN_BTATTR s_bt, CAN_BTATTR f_bt);
//=============================================================================
/**
 * CiA 603 Time-Stamping
 * @param[in]  base address
 * @param[in]  TIME-stamping Enable ,0-disabled , 1-enabled
 * @param[in]  TIME-stamping Position, 0-SOF , 1-EOF
 * @return none
 */
//=============================================================================
void     ithCANSetCIA603 (CAN_HANDLE * base, bool Onoff, bool position);
//=============================================================================
/**
 * Set Acceptance CODE
 * @param[in]  base address
 * @param[in]  Acceptance index range(0 - 15)
 * @param[in]  CODE number  bit(0-28)
 * @return none
 */
//=============================================================================
void     ithCANSetACODE (CAN_HANDLE * base, CAN_ACODE c_index, uint32_t value);
//=============================================================================
/**
 * Set Acceptance MASK
 * @param[in]  base address
 * @param[in]  Acceptance MASK index range(0 - 15)
 * @param[in]  MASK number  bit(0-28)
 * @param[in]  AIDEE 0 - accepts both standard or extended 1 - defined by AIDE
 * @param[in]  AIDE   0 - only standard frames 1- only extended frames
 * @return none
 */
//=============================================================================
void     ithCANSetAMASK (CAN_HANDLE * base, CAN_AMASK m_index, uint32_t value,
                         bool AIDEE, bool AIDE);
//=============================================================================
/**
 * Acceptance filter Enable
 * @param[in]   base address
 * @param[in]   index ACF id (range 0x0 - CFG_ACF_NUMBER)
 * @param[in]   On/Off
 * @return none
 */
//=============================================================================
void     ithCANACFEnable (CAN_HANDLE * base, CAN_ACFE index, bool OnOff);
//=============================================================================
/**
 * Receive buffer STATus
 * @param[in]   base address
 * @return 00 - empty , 01 - > empty and < almost full (AFWL) , 10 - almost full
 * , 11 - full
 */
//=============================================================================
uint8_t  ithCANGetRBStatus (CAN_HANDLE * base);
//=============================================================================
/**
 * Transmission Primary/Secondary buffer STATus
 * @param[in]   base address
 * @param[in]   TB Type 0x1 - STB status 0x0 - PTB status
 * @return 00 - empty , 01 - > empty and < almost full (AFWL) , 10 - almost full
 * , 11 - full
 */
//=============================================================================

//=============================================================================
uint8_t  ithCANGetTBStatus (CAN_HANDLE * base, bool TBType);
//=============================================================================
/**
 * Transmit Primary Enable
 * @param[in]   base address
 * @param[in]   Ctrl bit  1 - Start Transmit,  0 - Stop Transmit
 * @return none
 */
//=============================================================================
void     ithCANPrimarySendCtrl (CAN_HANDLE * base, bool OnOff);
//=============================================================================
/**
 * Transmit Secondary ALL frame Enable
 * @param[in]   base address
 * @param[in]   Ctrl bit  1 - Start Transmit,  0 - Stop Transmit
 * @return none
 */
//=============================================================================
void     ithCANSecondarySendCtrl (CAN_HANDLE * base, bool OnOff);
//=============================================================================

//=============================================================================
/**
 * Transmission Primary Send STATus
 * @param[in]   base address
 * @return 0 - idle  1- busy
 */
//=============================================================================
uint8_t  ithCANGetPrimarySendIdle (CAN_HANDLE * base);

//=============================================================================
/**
 * Transmission Secondary Send STATus
 * @param[in]   base address
 * @return 0 - idle  1- busy
 */
//=============================================================================
uint8_t  ithCANGetSecondarySendIdle (CAN_HANDLE * base);

/**
 * Get Receive Error Count
 * @param[in]   base address
 * @return Receive Error Count
 */
//=============================================================================
uint32_t ithCANGetReceiveErrorCouNT (CAN_HANDLE * base);
//=============================================================================
/**
 * Get Transmit Error Count
 * @param[in]   base address
 * @return Transmit Error Count
 */
//=============================================================================
uint32_t ithCANGetTransmitErrorCouNT (CAN_HANDLE * base);
//=============================================================================
/**
 * Get Kind of Error
 * @param[in]   base address
 * @return Kind of Error
 * @ 000 - no error
 * @ 001 - BIT ERROR
 * @ 010 - FORM ERROR
 * @ 011 - STUFF ERROR
 * @ 100 - ACKNOWLEDGEMENT ERROR
 * @ 101 - CRC ERROR
 * @ 110 - OTHER ERROR
 */
//=============================================================================
uint32_t ithCANGetKindOfError (CAN_HANDLE * base);
//=============================================================================
/**
 * Get Arbitration Lost Capture
 * @param[in]   base address
 * @return Arbitration Lost Capture
 */
//=============================================================================
uint32_t ithCANGetALC (CAN_HANDLE * base);
//=============================================================================
/**
 * Get Transmission Time Stamp
 * @param[in]   base address
 * @return Transmission Time Stamp
 */
//=============================================================================
uint32_t ithCANGetTransTimeStamp (CAN_HANDLE * base);
//=============================================================================
/**
 * Get Transmission Time Stamp
 * @param[in]   base address
 * @return Transmission Time Stamp
 */
//=============================================================================
void     ithTTCANCrtlON (CAN_HANDLE * base, uint16_t PRESCaler,
                         uint16_t Trigger_Time);
//=============================================================================
/**
 * Time Trigger Mode Disable
 * @param[in]   base address
 * @return none
 */
//=============================================================================
void     ithTTCANCrtlOFF (CAN_HANDLE * base);
//=============================================================================
/**
 * Time Trigger Mode , Set REFerence message IDentifier
 * @param[in]   base address
 * @param[in]  REFID , bit[30:0]  [2:0] forced 0
 * @param[in]  IDformat 1- extended ID, 0-standard ID
 * @return none
 */
//=============================================================================
void     ithTTCANSetREFID (CAN_HANDLE * base, uint32_t REFID, bool IDformat);
//=============================================================================
/**
 * Time Trigger Mode , Set Trigger Type
 * @param[in]   base address
 * @param[in]   trigger type
 * @return none
 */
//=============================================================================
void     ithTTCANSetTrig (CAN_HANDLE * base, CAN_TRIGGER_TYPE trigtype);
//=============================================================================
/**
 * Time Trigger Mode , Get Current Trigger Type
 * @param[in]   base address
 * @return Trigger Type
 */
//=============================================================================
uint32_t ithTTCANGetTrig (CAN_HANDLE * base);
//=============================================================================
/**
 * Time Trigger Mode , Set Trigger Time
 * @param[in]   base address
 * @param[in]   trigger time
 * @return none
 */
//=============================================================================
void     ithTTCANSetTrigTime (CAN_HANDLE * base, uint16_t trigtime);
//=============================================================================
/**
 * Time Trigger Mode , Set Watch Trigger Time
 * @param[in]   base address
 * @param[in]   watch Trigger Time
 * @return none
 */
//=============================================================================
void     ithTTCANSetWatchTrigTime (CAN_HANDLE * base, uint16_t watchtrigtime);
//=============================================================================
/**
 * CAN-CTRL Set Time stamp clk divider
 * @param[in]   base address
 * @return none
 */
//=============================================================================
void     ithCANSetTimeStampDiv (CAN_HANDLE * base, uint32_t value);
//=============================================================================
/**
 * CAN-CTRL Get Bus Off State
 * @param[in]   base address
 * @return Bus Off state
 */
//=============================================================================
uint32_t ithCANBUSState (CAN_HANDLE * base);
//=============================================================================
/**
 * CAN-CTRL Get Error Passive Mode
 * @param[in]   base address
 * @return 0 - Error active mdoe 1- Error passive mode
 */
//=============================================================================
uint32_t ithCANERRORState (CAN_HANDLE * base);

#ifdef __cplusplus
}
#endif

#endif // CAN_HW_H
