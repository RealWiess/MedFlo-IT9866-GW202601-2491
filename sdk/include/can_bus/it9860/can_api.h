#ifndef CAN_API_H
#define CAN_API_H

#include "ite/itp.h"
#include "can_hw.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Show Error Log
#if 0
    #define CANPRINT(...) (void)printf(__VA_ARGS__)
#else
    #define CANPRINT(...)
#endif

#if 0
    #define Min(a, b)         ((a) < (b) ? a : b)
    #define Alignment4BYTE(a) (a >> 2) + ((a % 4) ? 1 : 0)
    #define NPRESCALER_MIN    1
    #define NPRESCALER_MAX    255
    #define TIME_QUANTA_MIN   9
    #define TIME_QUANTA_MAX   40
    #define SLOWBTMAX         255
    #define FASTBTMAX         31
// TBPTR pointer slot (0 - 63)
    #define TBSLOT_BUFFER_MAX 63
#endif
#if CFG_CANBUS_FD_ENABLE
    #define DLC_MAX 64
#else
    #define DLC_MAX 8
#endif

#define CAN_MAX_INSTANCE              0x2UL

// interrupt define
#define Error_INT                     (0x0001UL)
#define TS_INT                        (0x0002UL)
#define TP_INT                        (0x0004UL)
#define RB_Almost_Full_INT            (0x0008UL)
#define RB_Full_INT                   (0x0010UL)
#define RB_Overrun_INT                (0x0020UL)
#define Receive_INT                   (0x0040UL)
#define Bus_Error_INT                 (0x0080UL)
#define Arbitration_Lost_INT          (0x0100UL)
#define Error_Passive_INT             (0x0200UL)

#define CAN_SUCCESS                   0
#define CAN_RX_FIFO_EMPTY             1
#define CAN_RX_STATUS_ERROR           2
#define CAN_TX_FIFO_FULL_ERROR        3
#define CAN_TX_BUSY_ERROR             4
#define CAN_TX_USER_ABORT_ERROR       5
#define CAN_TX_WAIT_TIMEOUT_ERROR     6
#define CAN_NOT_OPEN_ERROR            7
#define CAN_PARAMETER_INCORRECT_ERROR 8
#define CAN_ENTERLOM_TIMEOUT_ERROR    9
#define CAN_BUS_OFF_ERROR             10

//=============================================================================
//                Structure Definition
//=============================================================================

typedef enum
{
    CAN_DLC_0,
    CAN_DLC_1,
    CAN_DLC_2,
    CAN_DLC_3,
    CAN_DLC_4,
    CAN_DLC_5,
    CAN_DLC_6,
    CAN_DLC_7,
    CAN_DLC_8,
#if CFG_CANBUS_FD_ENABLE
    CAN_DLC_12,
    CAN_DLC_16,
    CAN_DLC_20,
    CAN_DLC_24,
    CAN_DLC_32,
    CAN_DLC_48,
    CAN_DLC_64
#endif
} CAN_DLC;

typedef struct
{
    uint8_t  TX;
    uint32_t KOER;
} CAN_RBUF_STATUS;

typedef struct
{
    uint32_t DLC; // Data len code
    uint8_t BRS; // Bit Rate Switch 0 - nominal / slow bit rate for the complete
                 // frame ,1 - switch to data / fast bit rate for the data
                 // payload and the CRC (only FD use)
    uint8_t EDL; // 0 - CAN 2.0B or 1 - FD
    uint8_t RTR; // 0 - data frame , 1 - remote frame  Remote Transmission
                 // Request(only CAN 2.0B use)
    uint8_t IDE; // 0 - STD , 1 - EXT identifier format
} CAN_BUF_CONTROL;

// Receive Buffer Registers RBUF
typedef struct
{
    uint32_t        Identifier; // Id
    CAN_BUF_CONTROL Control;
    CAN_RBUF_STATUS Status;
    uint8_t         RXData[DLC_MAX];
    uint32_t        RXRTS[2];
#if CFG_CANBUS_FD_ENABLE
    uint8_t ESI; // Error State Indicator
#endif
} CAN_RXOBJ;

// Transmit Buffer User Sel
typedef struct
{
    uint32_t        Identifier; // Id
    CAN_BUF_CONTROL Control;
    bool            SingleShot; // 0 - off,  1 - on single shot mode
    bool            TTSENSEL; // 0 - disable , 1 - enable CiA 603 time stamping
    uint32_t        Timeout;  // config send timeout, unit:tick
} CAN_TXOBJ;

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================
/**
 * @brief Initialize canbus controller.
 *
 * @param base Canbus handler.
 */
void ithCANOpen (CAN_HANDLE * base);

/**
 * @brief Close canbus controller.
 *
 * @param base Canbus handler.
 * @return int Result 0-success.
 */
int  ithCANClose (CAN_HANDLE * base);

/**
 * @brief Get data from controller FIFO.
 *
 * @param base Canbus handler.
 * @param info Struct pointer of receive data.
 * @return int Result 0-read success.
 */
int  ithCANRead (CAN_HANDLE * base, CAN_RXOBJ * info);

/**
 * @brief Use high priority buffer (PTB), send data to the network.
 *
 * @param base Canbus handler.
 * @param info Struct pointer of transmit data config.
 * @param dataptr Struct pointer of transmit data.
 * @return int Result 0-write success.
 */
int  ithCANWrite (CAN_HANDLE * base, CAN_TXOBJ * info, uint8_t * dataptr);

/**
 * @brief update data to STB FIFO.
 *
 * @param base Canbus handler.
 * @param info Struct pointer of transmit data config.
 * @param dataptr Struct pointer of transmit data.
 * @return int Result 0-update success.
 */
int  ithCANFIFOUpdate (CAN_HANDLE * base, CAN_TXOBJ * info, uint8_t * dataptr);

/**
 * @brief Send data in low priority buffer (STB) to the network.
 *
 * @param base Canbus handler.
 * @param info Struct pointer of transmit data config.
 * @return int Result 0-write success.
 */
int  ithCANFIFOWrite (CAN_HANDLE * base, CAN_TXOBJ * info);

/**
 * @brief CAN enter ListOnlyMode, no transmission can be started. In the this
 * mode does not generate a dominant ACK.
 *
 * @param base Canbus handler.
 * @return int Result 0-enter success.
 */
int  ithCANEnterListOnlyMode (CAN_HANDLE * base);

/**
 * @brief CAN leave ListOnlyMode.
 *
 * @param base Canbus handler.
 * @return int Result 0-leave success.
 */
int  ithCANLeaveListOnlyMode (CAN_HANDLE * base);

/**
 * @brief Convert Data length code(DLC) to Bytes.
 *
 * @param dlc Data length code.
 * @return uint32_t Number of bytes.
 */
uint32_t        ithCANDlcToBytes (CAN_DLC dlc);

/**
 * @brief Get Transmission Time Stamp.
 *
 * @param base Canbus handler.
 * @return int Time Stamp.
 */
int             ithCANGetTTS (CAN_HANDLE * base);

/**
 * @brief Reset canbus controller.
 *
 * @param base Canbus handler.
 */
void            ithCANReset (CAN_HANDLE * base);

/**
 * @brief  Get canbus controller error state
 *
 * @param base Canbus handler.
 * @return ERROR STATE
 */
CAN_ERROR_STATE ithCANGetErrorState (CAN_HANDLE * base);

/**
 * @brief  Get number of errors during reception.
 *
 * @param base Canbus handler.
 * @return number of errors.
 */
uint32_t        ithCANGetREC (CAN_HANDLE * base);

/**
 * @brief  Get number of errors during transmission.
 *
 * @param base Canbus handler.
 * @return number of errors.
 */
uint32_t        ithCANGetTEC (CAN_HANDLE * base);

/**
 * @brief  Get the core recognizes errors on the bus.
 *         KOER(kind of error) is updated with each new error.
 *
 * @param base Canbus handler.
 * @return CAN ERROR TYPE
 */
CAN_ERROR_TYPE  ithCANGetKOER (CAN_HANDLE * base);

/**
 * @brief Set TX/RX GPIO number.
 *
 * @param Instance CAN0 or CAN1.
 * @param rxpin RX pin number.
 * @param txpin TX pin number.
 */
void ithCANSetGPIO (uint32_t Instance, uint32_t rxpin, uint32_t txpin);

/**
 * @brief This API used to Ctrl Source CLK.
 *
 * @param ctl: 0x1 Enable Source CLK , 0x0 Disable Source CLK.
 * @return void
 */
void ithCANCLKEnable (uint8_t ctl);

#ifdef __cplusplus
}
#endif

#endif // CAN_API_H
