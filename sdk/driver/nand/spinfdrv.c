/*
 * Copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * SPI NAND Driver API header file.
 *
 */
#include <string.h>
#include <malloc.h>
#include "axispi_hw.h"
#include "spinfdrv.h"
#include "power_down_handle.h"
#include "ssp/mmp_axispi.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" {
#endif

// #define  ENABLE_NEW_WB_SPARE_MAP
#define ENABLE_PATCH_DIE_SELECT
// #define  ENABLE_DBG_MSG_DIE_SELECT
// #define  ENABLE_WP_RESERVED_AREA
// #define  ENABLE_SAVE_NAND_ERROR_LOG
//#define  ENABLE_SHOW_MORE_BITFLIP_INFO
#if defined (CFG_NAND_CHIP_SEL_ID)
    #define NAND_CSN                 CFG_NAND_CHIP_SEL_ID
#else
    #define NAND_CSN                 SPI_CSN_0
#endif
// =============================================================================
//                     SPI NAND CONFIGURATION TABLE
// =============================================================================
static SPI_NF_INFO  gSpiNfInfo;

static SPINF_CFG    g_SpiNfCfgArray[] =
{
    // name, MID, DID0, DID1, DID2, BBM, page, blk, blk cnt, spare
    {"GD_5F1GQ4UF",     0xC8, 0xB1, 0x48, 1, 2048, 64,  1024, 128}, // GD_5F1GQ4UC has been phased out
    {"GD_5F2GQ4UF",     0xC8, 0xB2, 0x48, 1, 2048, 64,  2048, 128}, // GD_5F2GQ4UC has been phased out
    {"GD_5F4GQ4UF",     0xC8, 0xB4, 0x68, 1, 4096, 64,  2048, 256}, // GD_5F4GQ4UC has been phased out
    {"GD_5F1GQ4UE",     0xC8, 0xD1, 0x00, 2, 2048, 64,  1024, 128}, // GD_5F1GQ4UB phased out, need spare remap
    {"GD_5F2GQ4UE",     0xC8, 0xD2, 0x00, 2, 2048, 64,  2048, 128}, // GD_5F2GQ4UB phased out, need spare remap
    {"GD_5F1GQ5UE",     0xC8, 0x51, 0x00, 2, 2048, 64,  1024, 128}, // need spare remap
    {"GD_5F2GQ5UE",     0xC8, 0x52, 0x00, 2, 2048, 64,  2048, 128}, // need spare remap
    {"GD_5F4GQ6UE",     0xC8, 0x55, 0x00, 2, 2048, 64,  4096, 128}, // need spare remap
    {"GD_5F1GM7UE",     0xC8, 0x91, 0x00, 1, 2048, 64,  1024, 128},
    {"GD_5F2GM7UE",     0xC8, 0x92, 0x00, 1, 2048, 64,  2048, 128},
    {"GD_5F4GM8UE",     0xC8, 0x95, 0x00, 1, 2048, 64,  4096, 128}, // 4Gb NAND flash U:3.3V R:1.8V
    {"F50L1G41LB",      0xC8, 0x01, 0x00, 5, 2048, 64,  1024, 128},
    {"MX35LF1GE4AB",    0xC2, 0x12, 0x00, 2, 2048, 64,  1024, 64 }, // CRBSY@bit6
    {"MX35LF2GE4AB",    0xC2, 0x22, 0x00, 2, 2048, 64,  2048, 64 }, // NO CRBSY bit
	{"MX35LF2GE4AC",    0xC2, 0x26, 0x01, 2, 2048, 64,  2048, 64 },		//CRBSY@bit7
    {"MX35LF2GE4AD",    0xC2, 0x26, 0x00, 2, 2048, 64,  2048, 64 }, // CRBSY@bit7
    {"MX35LF4GE4AD",    0xC2, 0x37, 0x03, 2, 4096, 64,  2048, 128}, // CRBSY@bit7, spare is 128-bytes
    {"MX35UF1G14AC",    0xC2, 0x90, 0x00, 2, 2048, 64,  1024, 64 }, // NOT support this NAND(1.8V)
    {"MX35UF2G14AC",    0xC2, 0xA0, 0x00, 2, 2048, 64,  2048, 64 }, // NOT support this NAND(1.8V)
    {"MX35LF2G24AD",    0xC2, 0x24, 0xFF, 2, 2048, 64,  2048, 128}, // NOT support this NAND(NO ECC)
    {"MX35LF4G24AD",    0xC2, 0x35, 0xFF, 2, 4096, 64,  2048, 256}, // NOT support this NAND(NO ECC)
    {"PN26G01AWSIUG",   0xA1, 0xE1, 0x00, 1, 2048, 64,  1024, 128},
    {"FM25S01A",        0xA1, 0xE4, 0x00, 4, 2048, 64,  1024, 64 },
    {"TX25G01",         0xA1, 0xF1, 0x00, 1, 2048, 64,  1024, 64 }, // there are three venders share this MID "0xA1"
    {"XT26G01A",        0x0B, 0xE1, 0x00, 1, 2048, 64,  1024, 64 },
    {"XT26G02A",        0x0B, 0xE2, 0x00, 1, 2048, 64,  2048, 64 },
    {"XT26G04A",        0x0B, 0xE3, 0x00, 1, 2048, 128, 2048, 64 },
    {"XT26G01C",        0x0B, 0x11, 0x00, 1, 2048, 64,  1024, 128},
    {"XT26G02C",        0x0B, 0x12, 0x00, 1, 2048, 64,  2048, 128},
    {"XT26G04C",        0x0B, 0x13, 0x00, 1, 4096, 64,  2048, 256},
    {"XT26G11C",        0x0B, 0x15, 0x00, 1, 2048, 64,  1024, 128},
    {"XT26G02E",        0x2C, 0x24, 0x00, 1, 2048, 64,  2048, 128}, // NOT support(NAND boot Does Not Support 2-plane read)
    {"F50L4G41XB",      0x2C, 0x34, 0x00, 1, 4096, 64,  2048, 256}, //
    {"W25N512GV",       0xEF, 0xAA, 0x20, 3, 2048, 64,  512,  64 },
    {"W25N01GV",        0xEF, 0xAA, 0x21, 3, 2048, 64,  1024, 64 },
    {"W25N02KV",        0xEF, 0xAA, 0x22, 3, 2048, 64,  2048, 64 }, // 128-bytes if need to dump raw spare data
    {"W25N04KV",        0xEF, 0xAA, 0x23, 3, 2048, 64,  4096, 64 },
    {"W25M02GV",        0xEF, 0xAB, 0x21, 3, 2048, 64,  2048, 64 }, // 2*1Gb(two 1Gb Die in one peckage)
    {"W25N01KV",        0xEF, 0xAE, 0x21, 3, 2048, 64,  1024, 96 }, // 96-bytes spare data
    {"ATO25D1GA",       0x9B, 0x12, 0x00, 1, 2048, 64,  1024, 64 },
    {"TC58CVG0S3HRAIG", 0x98, 0xC2, 0x00, 5, 2048, 64,  1024, 128},
    {"TC58CVG2S0HRAIG", 0x98, 0xCD, 0x00, 5, 4096, 64,  2048, 128},
    {"TC58CVG0S3HRAIJ", 0x98, 0xE2, 0x40, 5, 2048, 64,  1024, 128},
    {"TC58CVG1S3HRAIJ", 0x98, 0xEB, 0x40, 5, 2048, 64,  2048, 128},
    {"TC58CVG2S0HRAIJ", 0x98, 0xED, 0x51, 5, 4096, 64,  2048, 128},
    {"DS35Q1GA",        0xE5, 0x71, 0x00, 7, 2048, 64,  1024, 128},
    {"FS35ND02G",       0xCD, 0xEB, 0x00, 1, 2048, 64,  2048, 64 }, // It's is the same as GD's BBM
    {"F35SQA001G",      0xCD, 0x71, 0x00, 5, 2048, 64,  1024, 64 }, // It's is the same as Kioxia's BBM
    {"F35SQA002G",      0xCD, 0x72, 0x00, 5, 2048, 64,  2048, 64 }, // It's is the same as Kioxia's BBM
    {"S35ML02G3",       0x01, 0x25, 0x00, 8, 2048, 64,  2048, 128}  // Can NOT support because of reset command(only support RevB)
};

// =============================================================================
//                      MAPPING TABLE OF SPARE EREA
// =============================================================================
// re-arrange the spare data
// 0~3 -> 4~7, 4~7 -> 20~23, 8~11 -> 36~39, 12~15 -> 52~55,
// 16~19(good block code) -> 0~3(NO ECC protect); 20~23 -> 16~19(NO ECC protect)
// 2022/9/19 : modified the Winbond's Spare mappiing for avoiding using the bad block marker
static uint8_t gWbSprMapTable[] = {
    0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17,
    0x24, 0x25, 0x26, 0x27, 0x34, 0x35, 0x36, 0x37,
#ifdef ENABLE_NEW_WB_SPARE_MAP
    0x20, 0x21, 0x22, 0x23, 0x10, 0x11, 0x12, 0x13
};                                                                              // for new Winbond Spare mapping
#else
    0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13};                            // for old Winbond Spare mapping
#endif

static uint8_t gWb2SprMapTable[] = {
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};                                                                              // for new Winbond Spare mapping(spare 96-bytes)

// 0~3(4B) -> NO ECC(for bad block check)
// 4~5(2B) -> has ECC(user meta data1), 6~12(13B) -> has ECC(H/W ECC data)
// 13h~14h(2B) -> has ECC(user meta data1), 15h~21h(13B) -> has ECC(H/W ECC data)
// 22h~23h(2B) -> has ECC(user meta data1), 24h~30h(13B) -> has ECC(H/W ECC data)
// 31h~32h(2B) -> has ECC(user meta data1), 33h~3Fh(13B) -> has ECC(H/W ECC data)
// 40h~7Fh(64B) -> NO ECC(user meta data2)
//
// A[]->PARAGON's spr; B[]:FTL's spr
// A[0~3] <=B[0~3]
// A[4] <= B[4], A[5] <= B[6], A[13h~14h] <= B[8~9],
// A[22h,23h,31h,32h] <=B [16~19] (for good block code)
// A[64~] <=B [12~15,20~23]
static uint8_t gPrgSprMapTable[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x48, 0x05, 0x49,
    0x13, 0x14, 0x4A, 0x4B, 0x40, 0x41, 0x42, 0x43,
    0x22, 0x23, 0x31, 0x32, 0x44, 0x45, 0x46, 0x47
};

// 0~7(8B) -> NO ECC protect(for bad block check)
// 8~48(40B) -> has ECC protect(user meta data),    0~24(24B) -> has ECC(H/W ECC data)
// 49~63(16B) -> NO ECC(this area contains Internal ECC Data
// please check datasheet "XTX_SPI Nand_3.3V_1G_XT26G01A_Rev 0.1.pdf"
//
// A[]->XTX's spr; B[]:FTL's spr
// A[8~31] <=B[0~23]
static uint8_t gXtxSprMapTable[] = {
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

/*
please check datasheet "DS35Q1GA.pdf"
0~3, 0x10~0x13, 0x20~0x23, 0x30~0x33 -> NO ECC protect(for bad block check)
4~7, 0x14~0x17, 0x24~0x27, 0x34~0x37 -> has ECC protect
*/
static uint8_t gDsSprMapTable[] = {
    0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17,
    0x24, 0x25, 0x26, 0x27, 0x34, 0x35, 0x36, 0x37,
    0x00, 0x01, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13
};

/*
please refer to the datasheet "DS-00681-GD5F1GQ5xExxG-Rev1.1.pdf"
0~3, 0x10~0x13, 0x20~0x23, 0x30~0x33 -> NO ECC protect(0th byte for bad block check)
4~0xF, 0x14~0x1F, 0x24~0x2F, 0x34~0x3F -> has ECC protect
0x40~0x7F -> has ECC protect(ECC data area)
byte 12~15 of FTL spare is for ecc code(old version)
*/
static uint8_t  gGdSprMapTable[] = {
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x01, 0x02, 0x03,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B
};

static uint8_t  gOriSprMapTable[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
};

/*
please refer to the datasheet "F50L2G41XA(2B).pdf"
800h~81Fh -> NO ECC protect(0th byte for bad block check)
820h~83Fh -> has ECC protect
840h~87Fh -> for ecc meta-data
*/
static uint8_t gEsmt1SprMapTable[] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2A, 0x2B, 0x00, 0x01, 0x02, 0x03,
    0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33
};

/*
please refer to the datasheet "F50L2G41XA(2B).pdf"
800h~81Fh -> NO ECC protect(0th byte for bad block check)
820h~83Fh -> has ECC protect
840h~87Fh -> for ecc meta-data
*/
#if 0
static uint8_t gFsSprMapTable[] = {
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F
};
#endif

/*
please refer to the datasheet "F50L1G41XA(2M).pdf"
800h~81Fh -> NO ECC protect(0th byte for bad block check)
820h~83Fh -> has ECC protect
840h~87Fh -> for ecc meta-data
*/
static uint8_t          gEsmt2SprMapTable[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27
};

/*
please refer to the datasheet "F50L4G41XB(2X).pdf"
1000h~103Fh -> NO ECC protect(0th byte for bad block check)
1040h~107Fh -> has ECC protect
1080h~10FFh -> for ecc meta-data
*/
static uint8_t          gEsmt3SprMapTable[] = {
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57
};

/*
please refer to the datasheet "MX35LF2GE4AC_Automotive_Ver11_ITE.pdf"
800h~804h -> NO ECC protect(0th byte for bad block check)
804h~807h -> has ECC protect
808h~80Fh -> NO ECC protect

804h~807h & 814h~817h & 824h~827h & 834h~847h -> has ECC protect
*/
static uint8_t gMxicOldSprMapTable[] = {
    0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17,
    0x24, 0x25, 0x26, 0x27, 0x34, 0x35, 0x36, 0x37,
    0x20, 0x21, 0x22, 0x23, 0x10, 0x11, 0x12, 0x13
};    

#ifndef ENABEL_NEW_WAIT_READY
static int              gTimeOutInterval = 0;
#endif

static int              gHasDummyByteAddr = 0;
static int              gPARAGON_MID_0xA1 = 0;
static int              gTEXIN_MID_0xA1 = 0;
static int              gESMT_MID_0xC8 = 0;
static int              gESMT_MID_0X2C = 0;
static int              gXTX_TYPE_C = 0;
static int              gWINBOND_ECC_30H_FAIL = 0;
static int              gWINBOND_NEW_SPARE = 0;
static int              gGD_ECC_8BITS = 0;

#ifdef  USE_MMP_DRIVER
//static struct timeval  startT, endT;
#endif
#ifndef ENABEL_NEW_WAIT_READY
static int      gCurrTv = 0;
#endif

#ifdef  ENABLE_CTRL_1
static int gEnableRecoverNorGpioPin = 0;
#endif

#ifdef USE_NEW_MACRO
//static uint32_t gEnSpiCsCtrl        = 0;    // default: diable GPIO control SPI CS pin
#else
static uint32_t gEnSpiCsCtrl        = 1;    // default: enable GPIO control SPI CS pin
#endif

//static uint32_t gAddrHasDummyByte   = 0;
static uint32_t gUseDummyReadId     = 0;

//static int  gForceToStopErasing         = 0;
//static int  gEnWinbondContinueRead      = 0;

#ifdef USE_AXISPI_ENGINE
static uint8_t  * gSpiAddrBuf           = NULL; // AXISPI ENGINE NEED 32-byte alignment
static uint8_t  * gSpiDataBuf           = NULL; // AXISPI ENGINE NEED 32-byte alignment
#endif

static uint8_t  gNeedReMapSpare         = 0;
static uint8_t  gCS_ID                  = 0;
static uint8_t  g1stDieSel_D1           = 1;

static uint8_t  g2PlaneNandShift        = 0;
static uint8_t  g2PlaneReadCmd          = 0;

// Toshiba's SPI NAND(AIG series) has QUAD-read, but no Quad-Write
static uint8_t g_TOSHIBA_QuadMode       = 0;

#ifdef FAST_NAND_READ
static int  regA0                       = -1;
static int  regB0                       = -1;
#endif

uint32_t                gExtRawSprSize = 0;

uint8_t         gForcePioMode           = 0;
uint8_t         gNeedRawSpare           = 0;
uint8_t         gForceEccEnable         = 0;
uint8_t         gDualDieNand            = 0;
uint8_t         gIsSupportOtp           = 0;

//static uint8_t  gEccRegOffset           = 0xB0;	

NF_ECC_INFO         gEccInf;
uint8_t    gSupportEccErrBit   = 0;
uint8_t    gEccErrThreshold    = 4;

uint32_t        gCfgRsvBlkSize          = 0;
uint8_t         gAxiInfoBuffer[192]		= {0};

#ifdef ENABLE_SAVE_NAND_ERROR_LOG
volatile uint32_t   gDoSaveLog          = 1;
static uint32_t     gLogBlock           = 0;
static uint32_t     glb_cppo            = 0;
static uint32_t     gEnWpStopNfIfLogBlk = 1;
static uint32_t     gLogStartBlock      = 0x0C;
#endif

#ifdef ENABLE_WP_RESERVED_AREA
volatile uint8_t    gEnWpStopNfDriver   = 0;
volatile uint32_t   gEnWpReturnFail     = 1;
volatile uint8_t    gAxiPioGotAddr0     = 0;
volatile uint32_t   gFakeErrorHappen    = 0;
volatile uint32_t   gGlobalBlkIdx       = 0;

static int
chkWp (
    uint32_t blk);
uint8_t
spiNf_writepage (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     * dBuf);
uint8_t
spiNf_SetWtProtect (
    uint32_t    blk,
    uint32_t    bNum,
    uint8_t     mod);
#else
//volatile uint32_t   gGlobalBlkIdx   = 0;
//volatile uint32_t   gEnWpReturnFail = 0;
//volatile uint8_t    gAxiPioGotAddr0 = 0;
#endif

/* for check frequency of program NAND register */
uint8_t		RegAddr[32] = {0};
uint32_t	WtRegCnt[32] = {0};
uint32_t	WtRegNum = 0;

#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
extern uint8_t gIsCfgAllowAdjustRsvSize;
extern unsigned long g_ftl_reserved_size;
extern unsigned long g_pkg_reserved_size;
#endif

#ifdef ENABLE_SAVE_NAND_ERROR_LOG
static void
_save_log (
    uint8_t     * ibuf,
    uint32_t    s);
#endif
static uint8_t
n_cmdReset (
    bool w);
//extern void
//AxiSpiSetBusyBit (
//    uint32_t data);

uint8_t
n_SetEcc (
    uint8_t eccFn);

void
n_setBlkLockReg (
    uint32_t m);

void
n_SpiNf_InitAttr (
    unsigned char * pBuf);

// =============================================================================
//                              spi Function WRAP
// =============================================================================
#ifdef USE_AXISPI_ENGINE

static int
axispi_read (
    uint8_t     * cBuf,
    uint32_t    cLen,
    uint8_t     * dBuf,
    uint32_t    dLen)
{
    uint32_t    rst;
    uint32_t    * addr  = NULL;
    uint32_t    aLen    = 0;
	SPI_ADDR_LEN  addrType = ONEBYTEADDR;
    SPI_IOMODE  iomode  = SPI_IOMODE_0;

    addr = (uint32_t *)gSpiAddrBuf;
    (void)memset(gSpiAddrBuf, 0, 5);

    if (cLen >= 1)
    {
        //int i;
        aLen = cLen - 1;

        if (aLen > 4)
        {
            (void)printf("axispi_read() address len error!!\n");
            return !SPI_OK;
        }

        switch (aLen)
        {
            case 0: *addr   = 0;
				addrType = ONEBYTEADDR;
                break;

            case 1: *addr   = (uint32_t)cBuf[1];
				addrType = TWOBYTEADDR;
                break;

            case 2: *addr   = ((uint32_t)cBuf[1] | ((uint32_t)cBuf[2] << 8));
				addrType = THREEBYTEADDR;
                break;

            case 3: *addr   = (((uint32_t)cBuf[1] << 16) |  ((uint32_t)cBuf[2] << 8) | ((uint32_t)cBuf[3]));
				addrType = FOURBYTEADDR;
                break;

            case 4: *addr   = (((uint32_t)cBuf[1] << 8) | ((uint32_t)cBuf[2] << 16) | ((uint32_t)cBuf[3] << 24) | ((uint32_t)cBuf[4]));
				addrType = FOURBYTEADDR;
                break;
				
            default: (void)printf("incorrect Address length(%d)\n",aLen);
                break;
        }
    }

    #ifdef ENABLE_QUAD_OPERATION_MODE
    if (cBuf[0] == 0xEB)
    {
        //mmpAxiSpiSetDummyByte(NF_SPI_PORT, 0);// Set Quad mode dummy byte
        mmpAxiSpiSetDummyByteMulti(NF_SPI_PORT, NAND_CSN, 0);// Set Quad mode dummy byte
        iomode = SPI_IOMODE_4;
    }
    if (cBuf[0] == 0x6B)
    {
        if (gHasDummyByteAddr != 0)
        {
            //mmpAxiSpiSetDummyByte(NF_SPI_PORT, 0);                      // Set Quad mode dummy byte
            mmpAxiSpiSetDummyByteMulti(NF_SPI_PORT, NAND_CSN, 0);         // Set Quad mode dummy byte
        }
        else
        {
            //mmpAxiSpiSetDummyByte(NF_SPI_PORT, 8);                      // Set Quad mode dummy byte
            mmpAxiSpiSetDummyByteMulti(NF_SPI_PORT, NAND_CSN, 8);         // Set Quad mode dummy byte
        }
        iomode = SPI_IOMODE_2;
    }
    #endif

    if (gForcePioMode != 0)
    {
        //rst = mmpAxiSpiPioRead(NF_SPI_PORT, iomode, SPI_DTRMODE_0, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)dBuf, dLen);
        rst = mmpAxiSpiPioReadMulti(NF_SPI_PORT, NAND_CSN, iomode, SPI_DTRMODE_0, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)dBuf, dLen);
    }
    else
    {
        if (dLen < 32)
        {
            //rst = mmpAxiSpiPioRead(NF_SPI_PORT, iomode, SPI_DTRMODE_0, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)dBuf, dLen);
            rst = mmpAxiSpiPioReadMulti(NF_SPI_PORT, NAND_CSN, iomode, SPI_DTRMODE_0, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)dBuf, dLen);
        }
        else
        {
            // (void)printf(" axispi dma read: s=%x\n", dLen);
            if ((dLen & 0x1F) != 0)
            {
                (void)printf("[error] axispi dma read align issue: s=%x\n", dLen);
                rst = !SPI_OK;
            }
            else
            {
                //rst = mmpAxiSpiDmaRead((SPI_PORT)NF_SPI_PORT, (SPI_IOMODE)iomode, (SPI_DTRMODE)SPI_DTRMODE_0, (uint8_t *)cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint8_t *)dBuf, (uint32_t)dLen);
                rst = mmpAxiSpiDmaReadMulti((SPI_PORT)NF_SPI_PORT, NAND_CSN, (SPI_IOMODE)iomode, (SPI_DTRMODE)SPI_DTRMODE_0, (uint8_t *)cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint8_t *)dBuf, (uint32_t)dLen);
            }
        }
    }

    return rst;
}

/*
need 32-bit alignment
*/
static int
axispi_write (
    uint8_t     * cBuf,
    uint32_t    cLen,
    uint8_t     * dBuf,
    uint32_t    dLen)
{
    uint32_t    rst;
    uint32_t    * addr  = NULL;
    uint32_t    aLen    = 0;
	SPI_ADDR_LEN  addrType = ONEBYTEADDR;
    uint32_t    * pData = NULL;
    SPI_IOMODE  iomode  = SPI_IOMODE_0;

    addr    = (uint32_t *)gSpiAddrBuf;
    (void)memset(gSpiAddrBuf, 0, 5);

    pData   = (uint32_t *)gSpiDataBuf;
    (void)memcpy(gSpiDataBuf, dBuf, dLen);

    if (cLen >= 1)
    {
        //int i;

        aLen = cLen - 1;

        if (aLen > 8)
        {
            (void)printf("axispi_write address len error!!\n");
            return !SPI_OK;
        }

        switch (aLen)
        {
            case 0: *addr   = 0;
				addrType = ONEBYTEADDR;
                break;

            case 1: *addr   = (uint32_t)cBuf[1];
			addrType = TWOBYTEADDR;
                break;

            case 2: *addr   = ((uint32_t)cBuf[1] | ((uint32_t)cBuf[2] << 8));
			addrType = THREEBYTEADDR;
                break;

            case 3: *addr   = ((uint32_t)cBuf[3] | ((uint32_t)cBuf[2] << 8) | ((uint32_t)cBuf[1] << 16));
			addrType = FOURBYTEADDR;
                break;

            case 4: *addr   = ((uint32_t)cBuf[1] | ((uint32_t)cBuf[2] << 8) | ((uint32_t)cBuf[3] << 16) | ((uint32_t)cBuf[4] << 24));
			addrType = FOURBYTEADDR;
                break;

            default: (void)printf("incorrect Address length(%d)\n", aLen);
                break;
        }
    }

	/* for check if writing NAND register? */
	if(cBuf[0] == 0x1F)
	{
		int i;
		uint32_t	setSuccess = 0;
		
		if(cLen != 2)
		{
			(void)printf("incorrect set Feature Format(%d)\n",cLen);
		}
		//if(WtRegNum == 0)	WtRegNum++;
		//(void)printf("SetFeature: curr Set cmd: %02x, %02x, %02x, Num=%x\n",cBuf[0], cBuf[1], dBuf[0], WtRegNum);
		for(i=0; i<WtRegNum; i++)
		{
			if(cBuf[1] == RegAddr[i])
			{
				WtRegCnt[i] = WtRegCnt[i] + 1;
				setSuccess = 1;
				break;
				if((WtRegCnt[i] & 0xFF) == 0)
				{
					int k;
					
					(void)printf("show each Write register count: curr Set cmd: %02x, %02x, %02x\n",cBuf[0], cBuf[1], dBuf[0]);
					for(k=0; k<32; k++)
					{
						(void)printf("Reg[%02] = %04x\n", RegAddr[k], WtRegCnt[k]);
					}
				}
			}
		}
		if(!setSuccess)
		{
			RegAddr[WtRegNum] = cBuf[1];
			WtRegCnt[WtRegNum]++;
			WtRegNum++;					
			(void)printf("New Added reg & cnt: Set cmd: %02x, %02x, %02x: reg=%x, c=%x, n=%x\n",cBuf[0], cBuf[1], dBuf[0], RegAddr[WtRegNum], WtRegCnt[WtRegNum], WtRegNum);	
		}
	}

    #ifdef ENABLE_QUAD_OPERATION_MODE
    if (cBuf[0] == 0x32)
    {
        if (gHasDummyByteAddr != 0)
        {
            //mmpAxiSpiSetDummyByte(NF_SPI_PORT, 0);                      // Set Quad mode dummy byte
            mmpAxiSpiSetDummyByteMulti(NF_SPI_PORT, NAND_CSN, 0);         // Set Quad mode dummy byte
        }
        else
        {
            //mmpAxiSpiSetDummyByte(NF_SPI_PORT, 8);                      // Set Quad mode dummy byte
            mmpAxiSpiSetDummyByteMulti(NF_SPI_PORT, NAND_CSN, 8);         // Set Quad mode dummy byte
        }
        iomode = SPI_IOMODE_2;
    }
    #endif

    #ifdef ENABLE_WP_RESERVED_AREA
    if ((cBuf[0] == 0x10) && (cLen == 4) && !dLen)
    {
        uint32_t myBlk = (uint32_t)(gGlobalBlkIdx);

        if (chkWp(myBlk))
        {
            if (!gFakeErrorHappen || (gFakeErrorHappen == 0x5A5A5A5A))
            {
        #ifdef ENABLE_SAVE_NAND_ERROR_LOG
                uint8_t h[4] = {'c', 'b', 'u', 'f'};

                (void)printf("check axispi_write: S/W WP: myblk = (%x, %x) a=(%x, %x) f=%x\n", myBlk, (*addr / 64), *addr, addr, gFakeErrorHappen);
                sprintf(gAxiInfoBuffer, "axispiWt: myblk = (%x, %x) a=(%x, %x), f=%x\n", myBlk, (*addr / 64), *addr, addr, gFakeErrorHappen);
                sprintf(&gAxiInfoBuffer[64], "spiwt: c(%x, %x) d(%x, %x)\n", cBuf, cLen, dBuf, dLen);
                sprintf(&gAxiInfoBuffer[112], "mmpPioWt:(%x, %x)(%x, %x, %x)(%x, %x)\n", NF_SPI_PORT, iomode, cBuf, addr, aLen, pData, dLen);

                (void)memcpy(&gAxiInfoBuffer[176], &h[0], 4);
                (void)memcpy(&gAxiInfoBuffer[180], &cBuf[0], 4);
                h[0]    = 0x41;
                h[1]    = 0x44;
                h[2]    = 0x44;
                h[3]    = 0x52;
                (void)memcpy(&gAxiInfoBuffer[184], &h[0], 4);
                (void)memcpy(&gAxiInfoBuffer[188], addr, 4);
        #endif

                if (gFakeErrorHappen)
                {
                    gFakeErrorHappen = 0;
                }

                if (gEnWpStopNfDriver)
                {
                    // data abort by means
                    (void)printf("axispi_write: @ b=%x, then STOP it(%x)!!\n", myBlk, gEnWpStopNfDriver);
                    while (1)
                    {
                    }
                }
                if (gEnWpReturnFail)
                {
                    // reset NAND
                    n_cmdReset(0);
                    (void)usleep(10000);
                    gAxiPioGotAddr0 = 7;
                    return (16);
                }
            }
        }
    }
    #endif

    if (gForcePioMode != 0)
    {
        //rst = mmpAxiSpiPioWrite(NF_SPI_PORT, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)pData, dLen);
        rst = mmpAxiSpiPioWriteMulti(NF_SPI_PORT, NAND_CSN, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)pData, dLen);
    }
    else
    {
        if (dLen < 32)
        {
            //rst = mmpAxiSpiPioWrite(NF_SPI_PORT, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)pData, dLen);
            rst = mmpAxiSpiPioWriteMulti(NF_SPI_PORT, NAND_CSN, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint32_t *)pData, dLen);
        }
        else
        {
            if ((dLen & 0x1F) != 0)
            {
                (void)printf("[error] axispi dma write align issue: s=%x\n", dLen);
                rst = !SPI_OK;
            }
            else
            {
                //rst = mmpAxiSpiDmaWrite(NF_SPI_PORT, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint8_t *)pData, dLen);
                rst = mmpAxiSpiDmaWriteMulti(NF_SPI_PORT, NAND_CSN, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t *)addr, (SPI_ADDR_LEN)addrType, (uint8_t *)pData, dLen);
            }
        }
    }

    return rst;
}

/*
return true if OK;
result false if fail;
*/
static bool
axispi_write_in_isr (
    uint8_t     * cBuf,
    uint32_t    cLen,
    uint8_t     * dBuf,
    uint32_t    dLen)
{
    bool    rst;
    uint32_t    * addr  = NULL;
    //SPI_ADDR_LEN    aLen    = (SPI_ADDR_LEN)1;	//ONEBYTEADDR;TWOBYTEADDR
	//SPI_CMD_LEN cmdType = (SPI_CMD_LEN)1;	//SPI_1_BYTE_INST;
    uint32_t    * pData = NULL;
    SPI_IOMODE  iomode  = SPI_IOMODE_0;

    addr    = (uint32_t *)gSpiAddrBuf;
    (void)memset(gSpiAddrBuf, 0, 5);

    pData   = (uint32_t *)gSpiDataBuf;
    (void)memcpy(gSpiDataBuf, dBuf, dLen);

    //rst = (bool)mmpAxiSpiPioWriteInISR(NF_SPI_PORT, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t*)addr, (SPI_ADDR_LEN)TWOBYTEADDR, (uint32_t*)pData, dLen);	//ONEBYTEADDR
    rst = (bool)mmpAxiSpiPioWriteInISRMulti(NF_SPI_PORT, NAND_CSN, iomode, cBuf, (SPI_CMD_LEN)SPI_1_BYTE_INST, (uint32_t*)addr, (SPI_ADDR_LEN)TWOBYTEADDR, (uint32_t*)pData, dLen);	//ONEBYTEADDR

    return rst;
}
#endif

#ifdef  USE_MMP_DRIVER

static int
spi_read (
    uint8_t     * cBuf,
    uint32_t    cLen,
    uint8_t     * dBuf,
    uint32_t    dLen)                                                           // SPI_COMMAND_INFO *cmd)
{
    uint32_t rst;

    #ifdef  ENABLE_CTRL_1
    if (gEnSpiCsCtrl)
    {
        mmpSpiSetControlMode(SPI_CONTROL_NAND);
    }
    #endif

    #ifdef USE_AXISPI_ENGINE
    rst = axispi_read(cBuf, cLen, dBuf, dLen);
    #else
    if (dLen < 16)
    {
        rst = mmpSpiPioRead(NF_SPI_PORT, cBuf, cLen, dBuf, dLen, 8);
    }
    else
    {
        rst = mmpSpiDmaRead(NF_SPI_PORT, cBuf, cLen, dBuf, dLen, 8);
    }
    #endif

    #ifdef  ENABLE_CTRL_1
    if (gEnSpiCsCtrl)
    {
        mmpSpiResetControl();
        if (gEnableRecoverNorGpioPin)
        {
            mmpSpiSetControlMode(SPI_CONTROL_NOR);
            mmpSpiResetControl();
        }
    }
    #endif

    if (rst == SPI_OK)
    {
        return SPINF_OK;
    }
    else
    {
        return SPINF_FAIL;
    }
}

static int
spi_write (
    uint8_t     * cBuf,
    uint32_t    cLen,
    uint8_t     * dBuf,
    uint32_t    dLen)                                                               // (SPI_COMMAND_INFO *cmd)
{
    uint32_t rst;

    #ifdef  ENABLE_CTRL_1
    if (gEnSpiCsCtrl)
    {
        mmpSpiSetControlMode(SPI_CONTROL_NAND);
    }
    #endif

    #ifdef USE_AXISPI_ENGINE
    rst = axispi_write(cBuf, cLen, dBuf, dLen);
    #else
    if (dLen < 16)
    {
        /* workaround solution: I don't know why!! */
        /* It needs this delay time 2ms for SPI NAND + AXISPI NOR */
        (void)usleep(2);

        rst = mmpSpiPioWrite(NF_SPI_PORT, cBuf, cLen, dBuf, dLen, 8);

        /* workaround solution: I don't know why!! */
        /* It needs this delay time 2ms for SPI NAND + AXISPI NOR */
        (void)usleep(2);
    }
    else
    {
        rst = mmpSpiDmaWrite(NF_SPI_PORT, cBuf, cLen, dBuf, dLen, 8);
    }
    #endif

    #ifdef  ENABLE_CTRL_1
    if (gEnSpiCsCtrl)
    {
        mmpSpiResetControl();
        if (gEnableRecoverNorGpioPin)
        {
            mmpSpiSetControlMode(SPI_CONTROL_NOR);
            mmpSpiResetControl();
        }
    }
    #endif

    if (rst == SPI_OK)
    {
        return SPINF_OK;
    }
    else
    {
        return SPINF_FAIL;
    }
}

static void
spinf_error (
    uint8_t errCode)
{
    (void)printf("[SPINF ERROR] error code = %x\n", errCode);
}
#else   //use win32 spi driver

static int
spi_read (
    uint8_t     * cBuf,
    uint32_t    cLen,
    uint8_t     * dBuf,
    uint32_t    dLen)                                                           // (SPI_COMMAND_INFO *cmd)
{
    uint32_t rst;
    rst = SpiRead(cLen, (PWriteControlByteBuffer)cBuf, dLen, (PReadDataByteBuffer)dBuf);
    if (rst == SPI_OK)
    {
        return SPINF_OK;
    }
    else
    {
        return SPINF_FAIL;
    }
}

static int
spi_write (
    uint8_t     * cBuf,
    uint32_t    cLen,
    uint8_t     * dBuf,
    uint32_t    dLen)                                                               // (SPI_COMMAND_INFO *cmd)
{
    uint32_t rst;
    rst = SpiWrite(cLen, (PWriteControlByteBuffer)cBuf, dLen, (PWriteDataByteBuffer)dBuf);

    if (rst == SPI_OK)
    {
        return SPINF_OK;
    }
    else
    {
        return SPINF_FAIL;
    }
}

static void
spinf_error (
    uint8_t errCode)
{
    (void)printf("[SPINF ERROR] error code = %x\n", errCode);
}
#endif

#ifndef ENABEL_NEW_WAIT_READY
void
n_setTimeOutT (
    uint32_t tv)
{
#ifdef  USE_MMP_DRIVER
    #ifdef  EN_NO_RTC
    gCurrTv             = 0;
    gTimeOutInterval    = tv * 1000; // 10X
    #else
    gettimeofday(&startT, NULL);
    gTimeOutInterval    = tv;
    #endif
#else
    gCurrTv             = 0;
    gTimeOutInterval    = tv * 10;
#endif
}

static int
_waitTimeOut (
    void)
{
#ifdef  USE_MMP_DRIVER
    #ifdef  EN_NO_RTC
    gCurrTv++;
    if (gCurrTv == gTimeOutInterval)
    {
        return 1;
    }
    else
    {
        // it9830's cmd cycle is about 5us
        (void)usleep(10);
        return 0;
    }
    #else
    gettimeofday(&endT, NULL);
    if (itpTimevalDiff(&startT, &endT) > gTimeOutInterval)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    #endif
#else
    Sleep(1);

    gCurrTv++;
    if (gCurrTv == gTimeOutInterval)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}
#endif

// =============================================================================
//                              Private Function Definition
// =============================================================================
void
n_SpiNf_InitAttr (
    unsigned char * pBuf)
{
    (void)memset((void *)&gSpiNfInfo, 0, sizeof(SPI_NF_INFO));

    if (pBuf[0] == 0xC8)
    {
        gSpiNfInfo.MID = (unsigned char)pBuf[0];
    }
    else
    {
        (void)printf("Error: MID not support!!\n");
    }

    if (pBuf[1] == 0xB1)
    {
        gSpiNfInfo.DID0 = (unsigned char)pBuf[1];
    }
    else
    {
        (void)printf("Error: DID0 not support!!\n");
    }

    if (pBuf[2] == 0x48)
    {
        gSpiNfInfo.DID1 = (unsigned char)pBuf[2];
    }
    else
    {
        (void)printf("Error: DID1 not support!!\n");
    }

    if (gSpiNfInfo.MID != 0xC8)
    {
        // vendor not support
    }

    if ((gSpiNfInfo.DID0 == (uint8_t)0xB1) && (gSpiNfInfo.DID1 == (uint8_t)0x48))
    {
        (void)printf("SPI NF ID is VALID!!\n");
        gSpiNfInfo.Init         = 1;
        gSpiNfInfo.PageSize     = 2048;
        gSpiNfInfo.PageInBlk    = 64;
        gSpiNfInfo.TotalBlk     = 256;
        gSpiNfInfo.SprSize      = 128;
    }
}

static int
n_setAttribute (
    SPI_NF_INFO     * cInfo,
    unsigned char   * id)
{
    SPINF_CFG   * snCfg;
    int         snCfgCnt    = 0;
    int         isGotAttr   = 0;
    int         i           = 0;

    // (void)printf("setInfo:[%x,%x][%x,%x]\n",cInfo,cInfo->fromCfg,id,id[0]);

    if (cInfo->fromCfg != 0)
    {
        if (cInfo->CfgArray == NULL)
        {
            return (0x18);
        }

        if (cInfo->CfgCnt == 0)
        {
            return (0x19);
        }

        snCfg       = (SPINF_CFG *)cInfo->CfgArray;
        snCfgCnt    = cInfo->CfgCnt;
    }
    else
    {
        snCfg       = g_SpiNfCfgArray;
        snCfgCnt    = sizeof(g_SpiNfCfgArray) / sizeof(SPINF_CFG);
    }

    for (i = 0; i < snCfgCnt; i++)
    {
        if ((id[0] == snCfg[i].cfgMID) && (id[1] == snCfg[i].cfgDID0) && (id[2] == snCfg[i].cfgDID1))
        {
            isGotAttr = 1;
            break;
        }
    }

    if ((gUseDummyReadId != 0) && (isGotAttr != 0))
    {
        gHasDummyByteAddr = 1;
    }

    if (isGotAttr == 0)
    {
        return (0x1A);              // ID not support
    }
    gSpiNfInfo.Init         = 1;
    gSpiNfInfo.MID          = id[0];
    gSpiNfInfo.DID0         = id[1];
    gSpiNfInfo.DID1         = id[2];
    gSpiNfInfo.BBM_type     = snCfg[i].cfgBBM_type;
    gSpiNfInfo.BootRomSize  = 0; // 16MB = 2048*64*32;
    gSpiNfInfo.PageInBlk    = snCfg[i].cfgPageInBlk;
    gSpiNfInfo.PageSize     = snCfg[i].cfgPageSize;
    gSpiNfInfo.TotalBlk     = snCfg[i].cfgTotalBlk;
    gSpiNfInfo.SprSize      = snCfg[i].cfgSprSize;
    gSpiNfInfo.FtlSprSize   = 24;
#ifndef USE_MMP_DRIVER
    gSpiNfInfo.fromCfg      = cInfo->fromCfg;
    gSpiNfInfo.CfgMode      = cInfo->CfgMode;
    gSpiNfInfo.CfgCnt       = snCfgCnt;
    gSpiNfInfo.BurnRomAddr  = cInfo->BurnRomAddr;
#endif

    (void)memcpy((void *)gSpiNfInfo.name, (void *)&snCfg[i].cfgDevName, 32);

    (void)memcpy((void *)cInfo, (void *)&gSpiNfInfo, sizeof(SPI_NF_INFO));

    (void)printf("isGotAttr=%x, dummyAddr=%x\n", isGotAttr, gHasDummyByteAddr);

    return (0);
}

/*
return 0 is OK, others is False
*/
static uint8_t
spiNf_waitReady (
    uint8_t     * s,
    uint32_t    dwT)
#ifdef ENABEL_NEW_WAIT_READY
{
    unsigned char   SpiCmd[4];
    unsigned char   SpiData[4] = {0};
    int             spiret;

    SpiCmd[0]   = 0x0F;
    SpiCmd[1]   = 0xC0;

    spiret      = spi_read(SpiCmd, 2, &SpiData[0], 0); // wait ready bit, so dataLength = 0

    if ((SpiData[0] != 0) && (gSpiNfInfo.MID == 0xC2))
    {
        unsigned int    waitCnt = 0;
        unsigned char   chkBit  = 0x01;

        /* if MXIC's NAND, Need to check Cache-Status-Bit in C0h register, too. */
        /* if MX35LF1GE4AB, CsBit@bit6,  */
        /* if MX35LF2GE4AB, NO CsBit,  */
        /* if MX35LF2GE4AD, CsBit@bit7,  */
        /* if MX35LF4GE4AD, CsBit@bit7,  */
		/* if MX35LF2GE4AC, CsBit@bit7,  */
        if (gSpiNfInfo.DID0 == (uint8_t)0x12)
        {
            chkBit = 0x41;
        }
        if ((gSpiNfInfo.DID0 == (uint8_t)0x26) || (gSpiNfInfo.DID0 == (uint8_t)0x37))
        {
            chkBit = 0x81;
        }

        // (void)printf("   wait status != 0: %x\n", SpiData[0]);
        while ((SpiData[0] & chkBit) != 0)
        {
            spiret = spi_read(SpiCmd, 2, &SpiData[0], 0);
            waitCnt++;
            if (waitCnt > 10)
            {
                break;
            }
        }

        if (waitCnt > 1)
        {
            (void)printf("	wait status OK: C0=%x, c=%d\n", SpiData[0], waitCnt);
        }

        if (waitCnt > 10)
        {
            (void)printf("	NAND wait status ready timeout, C0=%x, cnt=%d\n", SpiData[0], waitCnt);
            return (5);
        }
    }

    if ((((uint8_t)SpiData[0] & (uint8_t)NF_STATUS_OIP) != 0) && (dwT > 2000))
    {
        (void)printf("Do H/W wait again, S=%x, T=%d\n", SpiData[0], dwT);
        spiret = spi_read(SpiCmd, 2, &SpiData[0], 0);   // wait ready bit, so dataLength = 0
    }

    *s = SpiData[0];
    // (void)printf("_wrS = %x, %x\n",*s, SpiData[0]);

    if (spiret == SPINF_OK)
    {
        return (0);
    }
    else
    {
        (void)printf("SPI NAND GET feature FAIL.00,result=%x\n", spiret);
        return (5);
    }
}
#else
{
    n_setTimeOutT(dwT);
    if (s != NULL)
    {
        *s = NF_STATUS_OIP;
    }

    while (*s & NF_STATUS_OIP)
    {
    #ifdef  USE_MMP_DRIVER
        if (spiNf_getFeature(0xC0, s))
        {
            (void)printf(" get feature FAIL(waitReady)!!\n");
        }
    #else
        uint8_t tmp = 0;
        if (spiNf_getFeature(0xC0, s))
        {
            tmp = *s;
        }
    #endif
        if (_waitTimeOut())
        {
            (void)printf("spiNf_waitReady timeout, S = %x\n", *s);
            return (12);
        }
        (void)usleep(0);
    }
    return (0);
}
#endif

/*
 input w: 0 is waiting for ready, 1 is "Don't wait"
 return: 0 is OK, others is reset fail.
*/
static uint8_t
n_cmdReset (
    bool w)
{
    unsigned char   SpiCmd[1];
    unsigned char   dBuf;
    int             spiret;
    uint8_t         status  = 0x01;
    uint8_t         result  = 0x31;

    if (w == 1)
    {
        (void)printf("n_cmdReset.1\n");
    }

    // FFH (reset)
    SpiCmd[0]   = 0xFF;

    spiret      = spi_write(&SpiCmd[0], 1, &dBuf, 0);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
    }
	
    // 0F (get feature)

    if (w == 0)
    {
        if (spiNf_waitReady(&status, 3000) != 0)
        {
            goto resetEnd;
        }
    }

    result          = 0;
    gCS_ID          = 0;
    g1stDieSel_D1   = 1;

#ifdef FAST_NAND_READ
    regA0           = -1;
    regB0           = -1;
#endif

resetEnd:

    // (void)printf("leave reset cmd R=%x\n",status);
    return result;
}

/*
"Die Select Cmd" only for Winbond W25N02GV
input ds: the DIE index(0 or 1)
*/
static void
n_cmdSwDieSelect (
    uint8_t ds)
{
    unsigned char   SpiCmd[2];
    unsigned char   dBuf;
    int             spiret;

    // (void)printf("n_cmdSwDieSelect: die=%x\n", ds);
    SpiCmd[0]   = 0xC2;
    SpiCmd[1]   = (unsigned char)ds;
    spiret      = spi_write(&SpiCmd[0], 2, &dBuf, 0);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_SDS_ERR);
    }

    // (void)printf("leave SwDieSelect cmd R=%x\n",spiret);
}

void
n_setBlkLockReg (
    uint32_t m)
{
    if (gSpiNfInfo.supportBlkLock != 0)
    {
        if (m == 1)
        {
            // 0x1C means Lower 1/16 (0000h ~ 1FFFh ) locked
            (void)spiNf_setFeature(0xA0, 0x1C);
        }
        else
        {
            (void)spiNf_setFeature(0xA0, 0x00);
        }
    }
}

static void
n_chkRegA0B4WtErs (
    void)
{
    uint8_t status          = 0;
    uint8_t default_status  = 0x38;

    if (gSpiNfInfo.supportBlkLock != 0)
    {
        default_status = 0x1C;
    }

    if (spiNf_getFeature(0xA0, &status) != 0)
    {
        (void)printf(" get feature.3 [0xA0] = %x!!\n", status);
    }
    if ((status & 0x38u) != 0)
    {
        // unlock blocks
        if (gSpiNfInfo.supportBlkLock != 0)
        {
            if (status != default_status)
            {
                (void)printf("unlock blocks, s=%x\n", status);
            }
            n_setBlkLockReg(gSpiNfInfo.wpMode);
        }
        else
        {
            (void)printf("unlock blocks, s=%x\n", status);
            (void)spiNf_setFeature(0xA0, 0x00);
            if (spiNf_getFeature(0xA0, &status) != 0)
            {
                (void)printf(" get_ftr1 [0xA0] = %x!!\n", status);
            }
            if ((status & default_status) != 0)
            {
                (void)printf(" get_ftr2 [0xA0] = %x!!\n", status);
            }
			(void)printf("unlock blocks2, s=%x\n",status);
        }
    }
    else
    {
        if (gSpiNfInfo.supportBlkLock != 0)
        {
            // 0x0E means WP reserved area(0~16MB)
            n_setBlkLockReg(gSpiNfInfo.wpMode);
        }
    }
}

#ifdef ENABLE_WP_RESERVED_AREA

/**
 * Check if the block is WP
 * @param blk: the block index that need to check
 * @output: the result, 0: not WP, others: WP
 */
static int
chkWp (
    uint32_t blk)
{
    if (gSpiNfInfo.wpMode && gSpiNfInfo.wpBlkCnt)
    {
        if (gFakeErrorHappen)
        {
            (void)printf("\n");
            (void)printf("gFakeErrorHappen = %x\n", gFakeErrorHappen);
            return (1);
        }

        if ((blk >= gSpiNfInfo.wpStartBlk) && (blk < (gSpiNfInfo.wpStartBlk + gSpiNfInfo.wpBlkCnt)))
        {
            (void)printf("NAND WP this block: %x (wpS=%x, wpC=%x, wpM=%x)\n", blk, gSpiNfInfo.wpStartBlk, gSpiNfInfo.wpBlkCnt, gSpiNfInfo.wpMode);
            return (1);
        }
    }

    return (0);
}
#endif

#ifdef ENABLE_SAVE_NAND_ERROR_LOG

static void
_save_log (
    uint8_t     * ibuf,
    uint32_t    size)
{
    uint8_t     tmpBuf[2048 * 2]    = {0};
    uint8_t     * ptr               = (uint8_t *)tmpBuf;
    uint32_t    cp                  = 0;
    uint32_t    i                   = 0;
    uint8_t     status;
    uint8_t     pgBuf[4096 + 512]   = {0};
    uint8_t     rst                 = 0;

    if (!gDoSaveLog)
    {
        (void)printf("Skip saving LOG block\n");
        return;
    }

    // search a good block(from 0x10 ~ 0x13)
    (void)printf("Searching for LOG block: b=%x, p=%x, s=%x\n", gLogBlock, glb_cppo, gDoSaveLog);
    if (gLogBlock == 0)
    {
        uint8_t     hdBuf[16]   = {0};
        uint8_t     isAll0xFF   = 1;
        uint8_t     isLogBlk    = 1;
        uint32_t    blk         = gLogStartBlock;

        sprintf(hdBuf, "FINE_ITE_LOG");
        (void)printf("	check blk = %x\n", blk);
        n_cmdReset(0);
        (void)usleep(10000);

        while (blk < (gLogStartBlock + 4))
        {
            uint8_t * p = (uint8_t *)&pgBuf[0];

            (void)printf("	read page 0 of blk = %x, bs(%x, %x)\n", blk, &pgBuf[0], pgBuf);
            status      = spiNf_PageRead(blk, 0, &pgBuf[0]);
            isAll0xFF   = 1;

            for (i = 0; i < 32; i++)
            {
                (void)printf("%02X ", pgBuf[i]);
                if ((i & 0xF) == 0xF)
                {
                    (void)printf("\n");
                }
            }
            (void)printf("\n");

            for (i = 0; i < 13; i++)
            {
                if (p[i] != hdBuf[i])
                {
                    isLogBlk = 0;
                }
            }
            (void)printf("blk %x , isLogBlk = %x \n", blk, isLogBlk);
            if (isLogBlk)
            {
                (void)printf("There is a LOG block @ b=%x, then STOP it!!\n", blk);
                gEnWpReturnFail = 0x55AA0001;
                gDoSaveLog      = 0;
                return;
            }

            p = (uint8_t *)&pgBuf[2048];
            for (i = 0; i < 24; i++)
            {
                if (p[i] != 0xFF)
                {
                    isAll0xFF = 0;
                    break;
                }
            }
            if (isAll0xFF)
            {
                break;
            }
            else
            {
                blk++;
            }
        }

        (void)printf("break loop, b=%x!\n", blk);
        if (blk >= (gLogStartBlock + 4))
        {
            (void)printf("There is No Good block from 0xC ~ 0xF!!\n");
            gEnWpReturnFail = 0x55AA0002;
            gDoSaveLog      = 0;
            return;
        }

        gLogBlock   = blk;
        glb_cppo    = 0;

        (void)printf("Save Log @ block %x!!\n", blk);
    }

    if (glb_cppo && (size == 2048))
    {
        // save whole page data
        if (gFakeErrorHappen)
        {
            gFakeErrorHappen = 0;
        }
        if (gAxiPioGotAddr0)
        {
            gAxiPioGotAddr0 = 0;
        }

        (void)memcpy(tmpBuf, ibuf, size);

        rst     = spiNf_SetWtProtect(0, gCfgRsvBlkSize, 0);
        status  = spiNf_writepage(gLogBlock, glb_cppo, tmpBuf);
        rst     = spiNf_SetWtProtect(0, gCfgRsvBlkSize, 1);
    }
    else
    {
        // if bootloader booting and got a log block
        // then STOP bootloader flow

        // create a blk-buffer for saving log

        (void)printf("start: cp=%x, p=%x\n", cp, ptr);
        // sprintf() LOG Header(16-byets)
        sprintf(tmpBuf, "FINE_ITE_LOG");
        cp  = 16;
        ptr = (uint8_t *)&tmpBuf[cp];
        (void)printf("b4 api: cp=%x, p=%x\n", cp, ptr);

        // save api peremeter(size-bytes)
        ptr = (uint8_t *)&tmpBuf[cp];
        (void)memcpy(ptr, ibuf, size);
        cp  += size;
        ptr = (uint8_t *)&tmpBuf[cp];

        (void)printf("after API: cp=%x, p=%x\n", cp, ptr);
        for (i = 0; i < 128; i++)
        {
            (void)printf("%02X ", tmpBuf[i]);
            if ((i & 0xF) == 0xF)
            {
                (void)printf("\n");
            }
        }
        (void)printf("\n");

        // save sNfInfo
        sprintf(ptr, "gSpiNfInfo:");
        cp  += 16;
        ptr = (uint8_t *)&tmpBuf[cp];
        (void)printf("b4 info: cp=%x, p=%x\n", cp, ptr);
        (void)memcpy(ptr, &gSpiNfInfo, sizeof(gSpiNfInfo));
        cp  += sizeof(gSpiNfInfo);
        ptr = (uint8_t *)&tmpBuf[cp];
        (void)printf("end: cp=%x, p=%x, cppo=%x\n", cp, ptr, glb_cppo);

        (void)printf("Show Error Log:: base:%x, size = %d, %d\n", &tmpBuf[0], sizeof(gSpiNfInfo), cp);
        for (i = 0; i < cp; i++)
        {
            (void)printf("%02X ", tmpBuf[i]);
            if ((i & 0xF) == 0xF)
            {
                (void)printf("\n");
            }
        }

        if (cp >= 2048)
        {
            (void)printf("Log size is too large, so limit it to 2048 bytes!!\n");
        }

        if (gFakeErrorHappen)
        {
            gFakeErrorHappen = 0;
        }

        if (gAxiPioGotAddr0)
        {
            gAxiPioGotAddr0 = 0;
        }

        // save log at log block
        rst     = spiNf_SetWtProtect(0, gCfgRsvBlkSize, 0);
        status  = spiNf_writepage(gLogBlock, glb_cppo, tmpBuf);
        rst     = spiNf_SetWtProtect(0, gCfgRsvBlkSize, 1);
    }
    (void)printf("finished to Save Log page(%x, %x)(%x, %x)!!\n", gLogBlock, glb_cppo, gFakeErrorHappen, gAxiPioGotAddr0);
    (void)printf("\n");

    glb_cppo++;
    if (glb_cppo >= 64)
    {
        (void)printf("Log Block is full, No empty page, STOP it(%x, %x, %x)!!\n", gLogBlock, glb_cppo, tmpBuf);
        while (1)
        {
        }
    }
}
#endif

/*
 * @return: 0 if success, 1 if cache miss, 2 if set ECC bit error
 */
uint8_t
n_SetEcc (
    uint8_t eccFn)
{
    uint8_t status = 0;

    (void)printf("Set ECC: %x\n", eccFn);
    gForceEccEnable = eccFn;

    status          = (uint8_t)regB0;
    if (regB0 == -1)
    {
        (void)printf("The ECC setup will be process on next B0 feature Read\n");
        return 1;
    }
    else
    {
        (void)printf("	Set ECC2: s=%x, rB0=%x, eccFn=%x, (%x, %x)\n", status, regB0, eccFn, (status & ~0x10u), (eccFn << 4));
        if ((eccFn != 0) && (((uint8_t)regB0 & 0x10u) != 0))
        {
            (void)printf("	Skip Set ECC3: s=%x, rB0=%x, eccFn=%x, (%x, %x)\n", status, regB0, eccFn, (status & ~0x10u), (eccFn << 4));
            return 0;
        }

        if ((eccFn == 0) && (((uint8_t)regB0 & 0x10u) ==0))
        {
            (void)printf("	Skip Set ECC4: s=%x, rB0=%x, eccFn=%x, (%x, %x)\n", status, regB0, eccFn, (status & ~0x10u), (eccFn << 4));
            return 0;
        }

        status = (status & ~0x10u) | (eccFn << 4);
        if (spiNf_setFeature(0xB0, status) != 0)
        {
            // spi cmd error
            return 2;
        }
    }
    return 0;
}

static uint8_t
n_checkEccStatus_GD (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
	uint8_t res = 0;
	
    if (gUseDummyReadId != 0)
    {
        uint8_t realFlip = 0;

        if ((gSupportEccErrBit != 0) && ((s & 0x30u) != 0))
        {
            uint8_t rF0         = 0;
            uint8_t s1          = 0;
            uint8_t cmdRetry    = 0;
			
			if((s & 0x30u) != 0x20)
			{
				while (true)
				{
					if (spiNf_getFeature(0xF0, &rF0) == 0)
					{
						// getFeature command OK
						s1 = (rF0 >> 4) & 0x03u;
						if ((s1 > 0) && ((rF0 & 0xF0u) > 0x10))
						{
							(void)printf("NfRdGdE5(%x,%x)(%02x,%02x,%02x)\n", blk, ppo, s, rF0, s1);
						}
						break;
					}
					if (cmdRetry++ > 10)
					{
						// get feature timeout, then break this loop
						(void)printf("SPI NAND command Error(get Feature F0h > 10 times): %d\n", cmdRetry);
						rF0 = 0;
						break;
					}
					(void)printf("SPI NAND command Error: cnt = %d\n", cmdRetry);
				}
	
				// (void)printf("Got to get ECC error bit\n");
	
				if ((realFlip != 0) && (((rF0 >> 4) & 0x03u) == 0))
				{
					unsigned int rnd = 0;   // 0
					/*
					rnd = rand()&0xFFFF;
					if(rnd < 1) rF0 |= 0x30;
					else if(rnd < 4)    rF0 |= 0x20;
					else if(rnd < 16)   rF0 |= 0x10;
					*/
	
					if (((rF0 >> 4) & 0x03u) > 0)
					{
						// s=0x10;
						(void)printf("FAKE FLIP: (%x,%x)(%02x,%02x,%x)\n", blk, ppo, s, rF0, rnd);
					}
				}
				else
				{
					if (rF0 > 0x28)
					{
						(void)printf("REAL FLIP2: (%x,%x)(%02x,%02x)\n", blk, ppo, s, rF0);
					}
				}

				gEccInf.enableEccBit    = 1;
				gEccInf.block           = blk;
				gEccInf.page            = ppo;
				gEccInf.RegC0           = s;
				gEccInf.Reg7C           = rF0;
				if(gEccErrThreshold == 4)
				{
					gEccInf.EccErrBit       = ((rF0 >> 4) & 0x3u) + 1;
				}
				else
				{
					gEccInf.EccErrBit       = ((rF0 >> 4) & 0x3u) + 4;
					if((s & 0x30u) == 0x30)
					{
						gEccInf.EccErrBit       = 8;
						/*
						if(gEccInf.EccErrBit != 7)
						{
							(void)printf("Got Incorrect ECC state: (%x,%x)(%02x,%02x)\n", blk, ppo, s, rF0);
						}
						*/
					}
					else
					{
						gEccInf.EccErrBit       = ((rF0 >> 4) & 0x3u) + 4;
					}
				}
				
				gEccInf.EccErrThreshold = gEccErrThreshold;
				if (gEccInf.EccErrBit > (gEccErrThreshold - 2))
				{
					//(void)printf("has %x bit flip\n", gEccInf.EccErrBit);
					(void)printf("[0]Blk=%x, pg=%x has %x bit flip(%02x, %02x)\n", blk, ppo, gEccInf.EccErrBit, gEccInf.RegC0, gEccInf.Reg7C);
				}
						
				if ((s & 0xF0u) == 0x00)
				{
					unsigned int rnd = 0x0100;
					// rnd = rand()&0xFF;
					if (rnd < 16)
					{
						s = 0x10;
					}
				}
				else
				{
					// (void)printf("REAL FLIP: (%x,%x)(%02x)\n",blk,ppo,s);
					realFlip = 1;
				}
			}
        }
        switch (s >> 4)
        {
            case 0x3:
                if (gGD_ECC_8BITS != 0)
                {
                    (void)printf("spiNfRdGdE4(%x,%x), s=%x\n", blk, ppo, s);
                    // it's corrected(if gGD_ECC_8BITS)
                }
                else
                {
                    (void)printf("spiNfRdGdEr3(%x,%x), s=%x\n", blk, ppo, s);
					res = (24);    // return ECC error(It can't be fixed)
                }
				break;
				
            case 0x2:
                (void)printf("spiNfRdGdE2(%x,%x), s=%x\n", blk, ppo, s);
				res = (23);
				break;

            case 0x1:
                // (void)printf("spiNfRdGdE1(%x,%x), b=%d, s=%x\n",blk,ppo,s);
                break;

            case 0x0:
                // (void)printf("spiNfRdGdE0(%x,%x), b=%d, s=%x\n",blk,ppo,s);
                break;

            default:
                (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
				res = (22);
				break;
        }
    }
    else
    {
		if ((gSupportEccErrBit != 0) && ((s & 0x70u) != 0))
		{
            gEccInf.enableEccBit    = 1;
            gEccInf.block           = blk;
            gEccInf.page            = ppo;
            gEccInf.RegC0           = s;
            gEccInf.Reg7C           = (s >> 4);
			gEccInf.EccErrBit       = ((s >> 4) & 0x7u) + 2;
            gEccInf.EccErrThreshold = gEccErrThreshold;
            if (gEccInf.EccErrBit > (gEccErrThreshold - 2))
            {
                (void)printf("GD.0 b=%x, p=%x has %x bit flip, rC0 = %x\n", blk, ppo, gEccInf.EccErrBit, s);
            }
		}
		
        switch ((s >> 4) & 0x7u)
        {
            case 0x7:
                (void)printf("spiNfRdGdErr3(%x,%x,%x)\n", blk, ppo, s);
				res = (20);
				break;

            case 0x6:
            case 0x5:
            case 0x4:
                (void)printf("spiNfRdGdErr2(%x,%x), b=%d, s=%x\n", blk, ppo, (s >> 4) + 2, s);
                break;

            case 0x3:
            case 0x2:
            case 0x1:
                // (void)printf("spiNfRdGdErr1(%x,%x), b=%d, s=%x\n",blk,ppo,(s>>4)+2,s);
                break;

            case 0x0:
                // (void)printf("spiNfRdGdErr0(%x,%x), b=%d, s=%x\n",blk,ppo,s);
                break;
				
            default:
                (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
				res = (26);
				break;
        }
    }

    return res;
}

/*
The bit shows the status of ECC as below:
00b = 0 bit error
01b = bit error are detected and been corrected, bit error count is less than the
bit flip threshold
10b = bit error and can not be corrected.
11b = bit error are detected and been corrected, bit error count is equal or more
than the bit flip threshold
*/
static uint8_t
n_checkEccStatus_MXIC (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
    static uint8_t r7C         = 0;
    static uint8_t rC0         = 0;
    uint8_t ErrBit      = 0;
    int     spiret      = 0;
    int     aErrCnt      = 0;
    int     ErrCntC0    = 0;
	uint8_t res = 0;
	
    if (gSupportEccErrBit != 0u)
    {
        uint8_t SpiCmd[2];

        // use 7Ch cmd to get ECC Error bit gEccInf
        SpiCmd[0]   = 0x7C;
        SpiCmd[1]   = 0x00;
        while (true)
        {
            r7C     = 0;
            spiret  = spi_read(&SpiCmd[0], 2, &r7C, 1); // read ECC error bit
            if (spiret == SPINF_OK)
            {
                if (aErrCnt != 0)
                {
                    (void)printf("ChkEcc7Ch: %x, c: %d\n", r7C, aErrCnt);
                }
                ErrBit = r7C & 0xFu;
                break;
            }
            if (aErrCnt++ > 10)
            {
                (void)printf("AXISPI Bus of SPI NAND Error, ErrCnt = %x\n", aErrCnt);
                break;
            }
        }

        // Retry C0h if Ecc Error(s>0x20)
        if (((s >> 4) & 0x3u) > 1)
        {
            SpiCmd[0]   = 0x0F;
            SpiCmd[1]   = 0xC0;
            while (true)
            {
                rC0     = 0;
                spiret  = spi_read(&SpiCmd[0], 2, &rC0, 1); // read ECC error bit
                if (spiret == SPINF_OK)
                {
                    if (ErrCntC0 != 0)
                    {
                        (void)printf("ChkEccC0h: %x, c: %d\n", rC0, ErrCntC0);
                    }
                    break;
                }
                if (ErrCntC0++ > 10)
                {
                    (void)printf("AXISPI Bus of SPI NAND Error, ErrCntC0 = %x\n", ErrCntC0);
                    break;
                }
            }
        }

        // check if ECC error match the ECC error bit?
        if ((((s >> 4) & 0x3u) == 2) && (ErrBit < 5))
        {
            uint8_t r1      = 0;
            uint8_t r2      = 0;
            int     spiret1 = 0;
            int     spiret2 = 0;
            int     ErrCnt2 = 0;

            (void)printf("\n");
            (void)printf("ReCheck C0h reg:(%x,%x,%x)(%x,%x)\n", blk, ppo, s, ErrBit, aErrCnt);

            while (true)
            {
                r1          = 0;
                r2          = 0;
                SpiCmd[0]   = 0x0F;
                SpiCmd[1]   = 0xC0;
                spiret1     = spi_read(&SpiCmd[0], 2, &r1, 1);  // read ECC error bit

                SpiCmd[0]   = 0x7C;
                SpiCmd[1]   = 0x00;
                spiret2     = spi_read(&SpiCmd[0], 2, &r2, 1);  // read ECC error bit

                if ((spiret1 == SPINF_OK) && (spiret2 == SPINF_OK))
                {
                    (void)printf("re-ChkEccC0h: C0=%x, 7C=%x, c=%d\n", r1, r2, ErrCnt2);
                    break;
                }
                if (ErrCnt2++ > 10)
                {
                    (void)printf("AXISPI Bus of SPI NAND Error, r1=%x, r2=%x, C0=%x, 7C=%x, ErrCntC0=%d\n", spiret1, spiret2, r1, r2, ErrCnt2);
                    break;
                }
            }
        }

        if ((((s >> 4) & 0x3u) > 0) && (ErrBit == 0))  //
        {
            //regC0 show ErrBit > 0, but reg7C show No ErrBit!!
            (void)printf("Got a abnormal case: blk=%x, p=%x, C0=%x, 7C=%x, Err-bit=%x\n", blk, ppo, s, r7C, ErrBit);
        }

        if (gSupportEccErrBit != 0u)
        {
            // (void)printf("Got to get ECC error bit\n");
            gEccInf.enableEccBit    = 1;
            gEccInf.block           = blk;
            gEccInf.page            = ppo;
            gEccInf.RegC0           = s;
            gEccInf.Reg7C           = r7C;
            gEccInf.EccErrBit       = r7C & 0xFu;
            gEccInf.EccErrThreshold = gEccErrThreshold;
            if (gEccInf.EccErrBit > (gEccErrThreshold - 2))
            {
                (void)printf("Got flip bit: %x, %x (%x, %x, %02x)\n", gEccInf.EccErrBit, r7C, blk, ppo, s);
            }
        }
    }

    switch ((s >> 4) & 0xFu)
    {
        case 0x3:
            (void)printf("NfRdMxicE3, but can correct, (%x, %x, %x), DID=  %02x\n", blk, ppo, s, gSpiNfInfo.DID0);
            break;

        case 0x2:
            if (gSupportEccErrBit != 0u)
            {
                (void)printf("NfRdMxicE2(%x,%x,%x)(%x,%x)(%x,%x)\n", blk, ppo, s, ErrBit, aErrCnt, rC0, ErrCntC0);
            }
            else
            {
                (void)printf("NfRdMxicE2(%x,%x,%x)\n", blk, ppo, s);
            }
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdMxicE1(%x,%x,%x)\n",blk,ppo,s);
            break;

        case 0x0:
            // (void)printf("NfRdMxicE0(%x,%x,%x)\n",blk,ppo,s);
            break;

        default:
            (void)printf("incorrect status of MXIC's feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }
    return res;
}

static uint8_t
n_checkEccStatus_WINBOND (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
#ifdef ENABLE_SHOW_MORE_BITFLIP_INFO
	uint8_t	reg10 = 0;
	uint8_t	reg20 = 0;
	uint8_t	reg30 = 0;
	uint8_t	reg40 = 0;
	uint8_t	reg50 = 0;
	uint8_t	rstReg[5] = {0};
#else
	uint8_t	reg30 = 0;
	uint8_t	rstReg[1] = {0};
#endif
	uint8_t res = 0;
	static uint8_t	bfCnt = 0;
	static uint8_t	bf4Cnt = 0;
	
	if (gSupportEccErrBit != 0u)
	{
		//if(reg30 & 0xF0)
		//if((s & 0x10) == 0x10)
		if((s & 0x10u) >= 0x10)
		{
			//(void)printf("WB0.realBf: blk=%x, pg=%x, s=%x\n",blk, ppo, s);
			#ifdef ENABLE_SHOW_MORE_BITFLIP_INFO
			rstReg[0] = spiNf_getFeature(0x10, &reg10);
			rstReg[1] = spiNf_getFeature(0x20, &reg20);
			rstReg[2] = spiNf_getFeature(0x30, &reg30);
			rstReg[3] = spiNf_getFeature(0x40, &reg40);
			rstReg[4] = spiNf_getFeature(0x50, &reg50);	
			if((reg30 & 0xF0u) > 0x50)
			{
				//(void)printf("realBF: blk=%x, pg=%x, s=%x, ### Reg 10~50: %02x, %02x, %02x, %02x, %02x, R= %02x, %02x, %02x, %02x, %02x ###\n"
				//				,blk, ppo, s, reg10,reg20,reg30,reg40,reg50 ,rstReg[0],rstReg[1],rstReg[2],rstReg[3],rstReg[4]);
				(void)printf("WB1.realBF: blk=%x, pg=%x, s=%x, ### Reg 10~50: %02x, %02x, %02x, %02x, %02x ###\n",blk, ppo, s, reg10,reg20,reg30,reg40,reg50);
			}	
			#else
			rstReg[0] = spiNf_getFeature(0x30, &reg30);
			if((reg30 & 0xF0u) > 0x50)
			{
				(void)printf("WB2.realBF: blk=%x, pg=%x, s=%x, ### Reg30: %02x ###\n",blk, ppo, s, reg30);
			}		
			#endif			
			
			//printf("Got to get ECC error bit\n");
			gEccInf.enableEccBit = 1;
			gEccInf.block = blk;
			gEccInf.page = ppo;
			gEccInf.RegC0 = s;
			gEccInf.Reg7C = reg30;
			gEccInf.EccErrBit = (uint8_t)((reg30 & 0xF0u) >> 4);		
			gEccInf.EccErrThreshold = gEccErrThreshold;		
			if(gEccInf.EccErrBit > (gEccErrThreshold - 2))
			{
				(void)printf("Got flip bit: %x, %x (%x, %x, %02x)\n", gEccInf.EccErrBit, reg30, blk, ppo, s);
			}
		}
		else
		{
			/* FAKE Bit Flip */
			unsigned int rnd = 0;	//0
			//rnd = rand()&0xFFFF;
			rnd = 1234;//rand();
			
			if(blk < 0x2FF)	//disable random FAKE flip
			{
				if(rnd < 1)	reg30 |= 0x30u;
				else if(rnd < 4)	reg30 |= 0x20u;
				else if(rnd < 16)	reg30 |= 0x10u;
			}
			
			if(((reg30>>4) & 0x03u) > 0)
			{
				//s=0x10;
				(void)printf("FAKE FLIP: (%x,%x)(%02x,%02x,%x)\n", blk, ppo, s, reg30, rnd);	
				
				if((reg30 & 0xF0u) >= 0x30)	bfCnt++;
				if(bfCnt > 16)
				{
					(void)printf("FAKE FLIP(4): (%x,%x)(%02x,%02x)(%x, %x)\n", blk, ppo, s, reg30, bfCnt, bf4Cnt);	
					reg30 = 0x40;
					bfCnt = 0;
					bf4Cnt++;
				}
				if(bf4Cnt > 16)
				{
					(void)printf("FAKE FLIP(5): (%x,%x)(%02x,%02x)(%x, %x)\n", blk, ppo, s, reg30, bfCnt, bf4Cnt);	
					reg30 = 0x50;
					bf4Cnt = 0;	
				}
			}
		}		
	}
	
    switch (s >> 4)
    {
        case 0x3:
            (void)printf("NfRdWbE3(%x,%x)(%x,%x)\n", blk, ppo, s, gWINBOND_ECC_30H_FAIL);
            if (gWINBOND_ECC_30H_FAIL != 0)
            {
				res = (24);
            }
            break;
				
        case 0x2:
            (void)printf("NfRdWbE2(%x,%x,%x)\n", blk, ppo, s);
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdWbE1(%x,%x,%x)\n",blk,ppo,s);
            break;

        case 0x0:
            // (void)printf("NfRdWbE0(%x,%x,%x)\n",blk,ppo,s);
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }
    return res;
}

/*
ref: page ?
00b = No bit errors.
01b = 1-bit error was detected and corrected.
10b = 2-bits error was detected and not corrected.
11b = reserved.
*/
static uint8_t
n_checkEccStatus_FM (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x2:
            (void)printf("NfRdFmE2(%x,%x,%x)\n", blk, ppo, s);
            // means ECC error and NOT be corrected
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdXtxE1(%x,%x,%x)\n",blk,ppo,s);
            // means ECC error and corrected (1 bits error)
            break;

        case 0x0:
            // (void)printf("NfRdXtxE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no ECC error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

/*
ref: page ?
00b = No bit errors were detected during the previous read algorithm.
01b = bit error was detected and corrected, error bit number = 1~7.
10b = bit error was detected and not corrected.
11b = bit error was detected and corrected, error bit number = 8.
*/
static uint8_t
n_checkEccStatus_PRG (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x3:
            (void)printf("NfRdPrgE3(%x,%x,%x)\n", blk, ppo, s);
            // means ECC error and corrected (8 bits error)
            break;

        case 0x2:
            (void)printf("NfRdPrgE2(%x,%x,%x)\n", blk, ppo, s);
            // means ECC error and NOT be corrected
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdPrgE1(%x,%x,%x)\n",blk,ppo,s);
            // means ECC error and corrected (1~7 bits error)
            break;

        case 0x0:
            // (void)printf("NfRdPrgE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no ECC error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

/*
00b = No bit errors were detected in last page read
01b = Bit errors were detected and corrected (<=7 bits)
10b = Bit errors greater than ECC capability (8bits) and not corrected
11b = Bit errors were detected and corrected
Bit errors count was equal to the threshold bit count (8 bits). Indicates data refreshment should be taken to guarantee data retention

ref: page ?
ECCS provides ECC Status as follows:
0000b = No bit errors were detected during the previous read algorithm.
0001b = bit errors were detected and corrected, error bit number = 1.
0010b = bit errors were detected and corrected, error bit number = 2.
0011b = bit errors were detected and corrected, error bit number = 3.
0100b = bit errors were detected and corrected, error bit number = 4.
0101b = bit errors were detected and corrected, error bit number = 5.
0110b = bit errors were detected and corrected, error bit number = 6.
0111b = bit errors were detected and corrected, error bit number = 7.
1000b = bit error was detected and not corrected(FOR TYPE A).
1000b = bit errors were detected and corrected, error bit number = 8(FOR TYPE C).
1100b = 8bit errors were detected and corrected, error bit number is going to exceed the tolerance.(FOR TYPE A)
1111b = Bit errors greater than ECC capability(8 bits) and not corrected(FOR TYPE C).
NOTE(FOR TYPE A): C0h register[bit7~0]: Reserved, Reserved, ECCS3, ECCS2, P_FAIL/ECCS1, E_FAIL/ECCS0, WEL OIP
NOTE(FOR TYPE C): C0h register[bit7~0]: ECCS3, ECCS2, ECCS1, ECCS0, P_FAIL, E_FAIL, WEL OIP
*/
static uint8_t
n_checkEccStatus_XTX (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
    uint8_t offset = 4;
	uint8_t res = 0;
	uint8_t bfCnt = 0;
	
    if (gXTX_TYPE_C != 0)
    {
        offset = 6;
		if((s & 0xF0u))
		{
			bfCnt = (s & 0xF0u)>>4;
			//(void)printf("Got XTX_0C's flip bit: %x, %x (%x, %x)\n", blk, ppo, bfCnt, s);
			if(bfCnt > 8)
			{
				(void)printf("Error!! Got XTX_0C's flip bit > 8: b=%x, p=%x (%x, %x)\n", blk, ppo, bfCnt, s);
			}
		}
    }
	else
	{
		if((s & 0x3Cu))
		{
			if(gSpiNfInfo.DID0 == 0x15)
			{
				//XT26G11C 
				//00b = No bit errors were detected in last page read
				//01b = Bit errors were detected and corrected (<=7 bits)
				//10b = Bit errors greater than ECC capability (8bits) and not corrected
				//11b = Bit errors were detected and corrected
				if((s & 0x3Cu) == 0x30u)
				{
					bfCnt = gEccErrThreshold;
				}
				if((s & 0x3Cu) == 0x20u)
				{
					bfCnt = gEccErrThreshold + 2;
				}
				//(void)printf("Got XTX_11C's flip bit: %x, %x (%x, %x)\n", blk, ppo, bfCnt, s);
			}
			else
			{
				bfCnt = (s>>2) & 0x0Fu;
				if((s & 0xF0u) == 0x30u)
				{
					bfCnt = 8;
				}
				if(bfCnt > 5)	(void)printf("Got XTX_0A's flip bit: %x, %x (%x, %x)\n", blk, ppo, bfCnt, s);
			}
		}
		}

    if (gSupportEccErrBit != 0u)
    {
        // (void)printf("Got to get ECC error bit\n");
        gEccInf.enableEccBit    = 1;
        gEccInf.block           = blk;
        gEccInf.page            = ppo;
        gEccInf.RegC0           = s;
        gEccInf.Reg7C           = s;
        gEccInf.EccErrBit       = bfCnt;
        gEccInf.EccErrThreshold = gEccErrThreshold;

        //if(gEccInf.EccErrBit > (gEccErrThreshold - 2))
        if (gEccInf.EccErrBit > 0)
        {
            (void)printf("Got flip bit: %x, (%x, %x, %02x)\n", gEccInf.EccErrBit, blk, ppo, s);
        }
    }

    switch (s >> offset)
    {
        case 0x3:
            if (gXTX_TYPE_C != 0)
            {
                (void)printf("NfRdXtxCE3(%x,%x,%x)\n", blk, ppo, s);
                // means ECC error and not corrected (>8 bits error)
				res = (24);
            }
            else
            {
                (void)printf("NfRdXtxE3(%x,%x,%x)\n", blk, ppo, s);
                // means ECC error and corrected (8 bits error)
				//do rewrite flow
            }
            break;
			
        case 0x2:
            if (gXTX_TYPE_C != 0)
            {
                (void)printf("NfRdXtxCE2(%x,%x,%x)\n", blk, ppo, s);
                // means ECC error and corrected (8 bits error)
				//do rewrite flow
            }
            else
            {
                (void)printf("NfRdXtxE2(%x,%x,%x)\n", blk, ppo, s);
                // means ECC error and NOT be corrected, return ECC error
				res = (23);
            }
			break;
			
        case 0x1:
            // (void)printf("NfRdXtxE1(%x,%x,%x)\n",blk,ppo,s);
            // means ECC error and corrected (1~7 bits error)
            break;

        case 0x0:
            // (void)printf("NfRdXtxE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no ECC error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

/*
ref: F50L2G41XA(2B).pdf's page 37
ECCS provides ECC Status as follows:
000b = No errors
001b = 1-3 bit errors detected and corrected.
010b = Bit errors greater than 8 bits detected and not corrected.
011b = 4-6 bit errors detected and corrected. Indicates data refreshment might be taken.
101b = 7-8 bit errors detected and corrected. Indicates data refreshment must be taken to guarantee data retention.
others = Reserved.
*/
static uint8_t
n_checkEccStatus_ESMT1 (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x5:
            (void)printf("NfRdXtxE3(%x,%x,%x)\n", blk, ppo, s);
            // means ECC error and corrected (7~8 bits error)
            break;

        case 0x3:
            (void)printf("NfRdXtxE2(%x,%x,%x)\n", blk, ppo, s);
            // means ECC error and corrected (4~6 bits error)
            break;

        case 0x2:
            (void)printf("NfRdXtxE4(%x,%x,%x)\n", blk, ppo, s);
            // means ECC error and NOT be corrected
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdXtxE1(%x,%x,%x)\n",blk,ppo,s);
            // means ECC error and corrected (1~3 bits error)
            break;

        case 0x0:
            // (void)printf("NfRdXtxE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no ECC error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

/*
ref: page ? NO status for checking ECC
*/
static uint8_t
n_checkEccStatus_ATO (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
    return (0);
}

static uint8_t
n_checkEccStatus_TOSHIBA (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
#ifdef ENABLE_SHOW_MORE_BITFLIP_INFO
	uint8_t	reg10 = 0;
	uint8_t	reg20 = 0;
	uint8_t	reg30 = 0;
	uint8_t	reg40 = 0;
	uint8_t	reg50 = 0;
	uint8_t	rstReg[5] = {0};
#else
	uint8_t	reg30 = 0;
	uint8_t	rstReg[1] = {0};
#endif
	uint8_t res = 0;
	static uint8_t	bfCnt = 0;
	static uint8_t	bf4Cnt = 0;

	if (gSupportEccErrBit != 0u)
	{
		//if(reg30 & 0xF0)
		if((s & 0x10u) == 0x10)
		{
			#ifdef ENABLE_SHOW_MORE_BITFLIP_INFO
			rstReg[0] = spiNf_getFeature(0x10, &reg10);
			rstReg[1] = spiNf_getFeature(0x20, &reg20);
			rstReg[2] = spiNf_getFeature(0x30, &reg30);
			rstReg[3] = spiNf_getFeature(0x40, &reg40);
			rstReg[4] = spiNf_getFeature(0x50, &reg50);	
			if((reg30 & 0xF0u) > 0x50)
			{
				//printf("realBF: blk=%x, pg=%x, s=%x, ### Reg 10~50: %02x, %02x, %02x, %02x, %02x, R= %02x, %02x, %02x, %02x, %02x ###\n"
				//				,blk, ppo, s, reg10,reg20,reg30,reg40,reg50 ,rstReg[0],rstReg[1],rstReg[2],rstReg[3],rstReg[4]);
				(void)printf("TSB.realBF: blk=%x, pg=%x, s=%x, ### Reg 10~50: %02x, %02x, %02x, %02x, %02x ###\n",blk, ppo, s, reg10,reg20,reg30,reg40,reg50);
			}	
			#else
			rstReg[0] = spiNf_getFeature(0x30, &reg30);
			if((reg30 & 0xF0u) > 0x50)
			{
				(void)printf("TSB.realBF: blk=%x, pg=%x, s=%x, ### Reg30: %02x ###\n",blk, ppo, s, reg30);
			}		
			#endif			
			
			//(void)printf("Got to get ECC error bit\n");
			gEccInf.enableEccBit = 1;
			gEccInf.block = blk;
			gEccInf.page = ppo;
			gEccInf.RegC0 = s;
			gEccInf.Reg7C = reg30;
			gEccInf.EccErrBit = (uint8_t)((reg30 & 0xF0u) >> 4);		
			gEccInf.EccErrThreshold = gEccErrThreshold;		
			if(gEccInf.EccErrBit > (gEccErrThreshold - 2))
			{
				(void)printf("Got flip bit: %x, %x (%x, %x, %02x)\n", gEccInf.EccErrBit, reg30, blk, ppo, s);
			}
		}
		else
		{
			/* FAKE Bit Flip */
			unsigned int rnd = 0;	//0
			//rnd = rand()&0xFFFF;
			rnd = 1234;//rand();
			
			if(blk < 0x2FF)	//disable random FAKE flip
			{
				if(rnd < 1)	reg30 |= 0x30u;
				else if(rnd < 4)	reg30 |= 0x20u;
				else if(rnd < 16)	reg30 |= 0x10u;
			}
			
			if(((reg30>>4) & 0x03u) > 0)
			{
				//s=0x10;
				(void)printf("FAKE FLIP: (%x,%x)(%02x,%02x,%x)\n", blk, ppo, s, reg30, rnd);	
				
				if((reg30 & 0xF0u) >= 0x30)	bfCnt++;
				if(bfCnt > 16)
				{
					(void)printf("FAKE FLIP(4): (%x,%x)(%02x,%02x)(%x, %x)\n", blk, ppo, s, reg30, bfCnt, bf4Cnt);	
					reg30 = 0x40;
					bfCnt = 0;
					bf4Cnt++;
				}
				if(bf4Cnt > 16)
				{
					(void)printf("FAKE FLIP(5): (%x,%x)(%02x,%02x)(%x, %x)\n", blk, ppo, s, reg30, bfCnt, bf4Cnt);	
					reg30 = 0x50;
					bf4Cnt = 0;	
				}
			}
		}		
	}
	
    switch (s >> 4)
    {
        case 0x3:
            // (void)printf("NfRdTsbE3(%x,%x,%x), data has been corrected\n",blk,ppo,s);
            // means ECC error and corrected (8 bits error)
            // return (4);
            break;

        case 0x2:
            (void)printf("NfRdTsbE2(%x,%x,%x)\n", blk, ppo, s);
            // means ECC error and NOT be corrected
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdXtxE1(%x,%x,%x)\n",blk,ppo,s);
            // means ECC error and corrected (1~7 bits error)
            break;

        case 0x0:
            // (void)printf("NfRdXtxE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no ECC error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

static uint8_t
n_checkEccStatus_DS (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
    // reg 0xC0: status bit:7~0 is Reserved, Reserved, ECC_S1, ECC_S0, P_Fail, E_Fail, WEL, OIP
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x2:
            (void)printf("NfRdDsE2(%x,%x,%x)\n", blk, ppo, s);
            // means More than 4-bit error and not corrected.
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdDsE1(%x,%x,%x)\n",blk,ppo,s);
            // means 1~4 bits error and corrected
            break;

        case 0x0:
            // (void)printf("NfRdDsE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no bit error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

static uint8_t
n_checkEccStatus_FS (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
    // reg 0xB0: status bit:7~0 is OTP-L, OTP-E, SR1-L, ECC-E, Reserved, Reserved, Reserved, Reserved
    // reg 0xC0: status bit:7~0 is Reserved, LUT-F, ECC-1, ECC-0, P_Fail, E_Fail, WEL, OIP
    // F35SQA001G has 1-bit ECC
    // FS35ND02G has 4-bit ECC
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x2:
            (void)printf("NfRdFsE2(%x,%x,%x)\n", blk, ppo, s);
            // means More than 1/4-bit error and not corrected.
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdFsE1(%x,%x,%x)\n",blk,ppo,s);
            // means 1/1~4 bits error and corrected
            break;

        case 0x0:
            // (void)printf("NfRdFsE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no bit error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

static uint8_t
n_checkEccStatus_ESMT2 (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
    // F50L1G41LB(2M) has 1-bit ECC
    // ECCS1 ECCS0
    // 0,0  No errors
    // 0,1  1-bit error detected and corrected
    // 1,0  2-bit or more than 2-bit errors detected and not corrected
    // 1,1  Reserved
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x2:
            (void)printf("NfRdEsmtE2(%x,%x,%x)\n", blk, ppo, s);
            // means More than 1/4-bit error and not corrected.
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdEsmtE1(%x,%x,%x)\n",blk,ppo,s);
            // means 1/1~4 bits error and corrected
            break;

        case 0x0:
            // (void)printf("NfRdEsmtE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no bit error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

static uint8_t
n_checkEccStatus_ESMT3 (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
    // F50L4G41XB(2X) has 8-bit ECC
    // ECCS2 ECCS1 ECCS0
    // 0,0,0  No errors
    // 0,0,1  1-3 bit errors detected and corrected
    // 0,1,0  Bit errors greater than 8 bits detected and not corrected
    // 1,0,1  7-8 bit errors detected and corrected. Indicates data refreshment must be taken to guarantee data retention
	// Others Reserved
	
	uint8_t res = 0;
	
	if (gSupportEccErrBit != 0u)
	{
		//(void)printf("Got to get ECC error bit\n");
		gEccInf.enableEccBit = 1;
		gEccInf.block = blk;
		gEccInf.page = ppo;
		gEccInf.RegC0 = s;
		gEccInf.Reg7C = s;
		gEccInf.EccErrBit = (uint8_t)0;		
		gEccInf.EccErrThreshold = gEccErrThreshold;		
	}
	
    switch ((s >> 4) & 0x7)
    {
        case 0x5:
            // (void)printf("NfRdEsmtE4(%x,%x,%x)\n",blk,ppo,s);
            // 7-8 bit errors detected and corrected. Indicates data refreshment must be taken to guarantee data retention
			if (gSupportEccErrBit != 0u)
			{
				//(void)printf("Got to get ECC error bit\n");
				gEccInf.EccErrBit = (uint8_t)8;		
				(void)printf("Got flip bit: %x, (%x, %x, %02x)\n", gEccInf.EccErrBit, blk, ppo, s);
			}
            break;
			
        case 0x3:
            // (void)printf("NfRdEsmtE3(%x,%x,%x)\n",blk,ppo,s);
            // means 4-6 bit errors detected and corrected
			if (gSupportEccErrBit != 0u)
			{
				gEccInf.EccErrBit = (uint8_t)5;	
			}
            break;
	
        case 0x2:
            (void)printf("NfRdEsmtE2(%x,%x,%x)\n", blk, ppo, s);
            // Bit errors greater than 8 bits detected and not corrected
			res = (23);
            break;

        case 0x1:
            // (void)printf("NfRdEsmtE1(%x,%x,%x)\n",blk,ppo,s);
            // means 1-3 bit errors detected and corrected
			if (gSupportEccErrBit != 0u)
			{
				gEccInf.EccErrBit = (uint8_t)2;	
			}
            break;

        case 0x0:
            // (void)printf("NfRdEsmtE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no bit error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

/*
ref: TX25G01's datasheet & page 30
ECCS provides ECC Status as follows:
000b = No bit errors were detected during the previous read algorithm.
001b = Bit errors(=1) were detected and corrected.
010b = Bit errors(=2) were detected and corrected.
011b = Bit errors(=3) were detected and corrected.
100b = Bit errors(=4) were detected and corrected.
111b = Internal error was detected and the data not promised correctly.
Others = Reserved.
*/
static uint8_t
n_checkEccStatus_TEXIN (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x7:
            (void)printf("NfRdTexinE2(%x,%x,%x)\n", blk, ppo, s);
            // means More than 1/4-bit error and not corrected.
			res = (23);
            break;

        case 0x4:
        case 0x3:
        case 0x2:
        case 0x1:
            // (void)printf("NfRdTexinE1(%x,%x,%x)\n",blk,ppo,s);
            // means 1/1~4 bits error and corrected
            break;

        case 0x0:
            // (void)printf("NfRdTexinE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no bit error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

static uint8_t
n_checkEccStatus_SKYHIGH (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     s)
{
	uint8_t res = 0;
	
    switch (s >> 4)
    {
        case 0x3:
            (void)printf("NfRdShE2(%x,%x,%x)\n", blk, ppo, s);
            // means More than 1/4-bit error and not corrected.
			res = (23);
            break;

        case 0x2:
        case 0x1:
            // (void)printf("NfRdShE1(%x,%x,%x)\n",blk,ppo,s);
            // means 1/1~4 bits error and corrected
            break;

        case 0x0:
            // (void)printf("NfRdShE0(%x,%x,%x)\n",blk,ppo,s);
            // mean no bit error
            break;

        default:
            (void)printf("incorrect status of feature register!!!reg[C0]=%x\n", s);
			res = (22);
            break;
    }

    return res;
}

#ifdef FAST_NAND_READ

static uint8_t
n_checkEccStatus (
    uint32_t    b,
    uint32_t    p,
    uint8_t     status)
#else

static uint8_t
n_checkEccStatus (
    uint32_t    b,
    uint32_t    p)
#endif
{
    uint8_t rst     = 0;
#ifndef FAST_NAND_READ
    uint8_t status  = 0;

    if (!spiNf_getFeature(0xC0, &status))
#endif
    {
        // (void)printf(" get feature OK for ECC check, status=%x!!\n",status);
        switch (gSpiNfInfo.MID)
        {
            case 0xC8:
                if (gESMT_MID_0xC8 != 0)
                {
                    rst = n_checkEccStatus_ESMT2(b, p, status);
                }
                else
                {
                    rst = n_checkEccStatus_GD(b, p, status);
                }
                break;

            case 0xC2:
                rst = n_checkEccStatus_MXIC(b, p, status);
                break;

            case 0xEF:
                rst = n_checkEccStatus_WINBOND(b, p, status);
                break;

            case 0x98:
                rst = n_checkEccStatus_TOSHIBA(b, p, status);
                break;

            case 0xA1:
                if (gPARAGON_MID_0xA1 != 0)
                {
                    rst = n_checkEccStatus_PRG(b, p, status);
                }
                else if (gTEXIN_MID_0xA1 != 0)
                {
                    rst = n_checkEccStatus_TEXIN(b, p, status);
                }
                else
                {
                    rst = n_checkEccStatus_FM(b, p, status);
                }
                break;

            case 0x0B:
                rst = n_checkEccStatus_XTX(b, p, status);
                break;

            case 0x2C:
				if(gESMT_MID_0X2C)
				{
					rst = n_checkEccStatus_ESMT3(b, p, status);
				}
				else
				{
					rst = n_checkEccStatus_ESMT1(b, p, status);
				}
                break;

            case 0x9B:
                rst = n_checkEccStatus_ATO(b, p, status);
                break;

            case 0xE5:
                rst = n_checkEccStatus_DS(b, p, status);
                break;

            case 0xCD:
                rst = n_checkEccStatus_FS(b, p, status);
                break;

            case 0x01:
                rst = n_checkEccStatus_SKYHIGH(b, p, status);
                break;

            default:
                (void)printf("n_checkEccStatus(): incorrect Factory ID(%x)!!\n", gSpiNfInfo.MID);
                rst = 0x23;
                break;
        }
    }

    if (rst != 0)
    {
        uint8_t rA0 = 0;
        uint8_t rB0 = 0;
        uint8_t r1C0 = 0;

        regA0   = -1;
        regB0   = -1;

        /* if ECC error, then show register value of A0h~C0h */
        if (spiNf_getFeature(0xA0, &rA0) != 0)
        {
            (void)printf("get rA0 fail!\n");
        }
        if (spiNf_getFeature(0xB0, &rB0) != 0)
        {
            (void)printf("get rB0 fail!\n");
        }
        if (spiNf_getFeature(0xC0, &r1C0) != 0)
        {
            (void)printf("get rC0 fail!\n");
        }
        (void)printf("[ECC ERROR] A0=%x, B0=%x, C0=%x\n", rA0, rB0, r1C0);
    }
    return rst;
}

/*
convert the real block address
input b: the original block address
output: the new block address
*/
static uint32_t
n_getNewBlkIndex (
    uint32_t b)
{
    uint8_t new_CS = 0;

    if ((gSpiNfInfo.MID == (uint8_t)0xEF) && (gSpiNfInfo.DID0 == (uint8_t)0xAB))
    {
        if (b > gSpiNfInfo.TotalBlk)
        {
            (void)printf("Error, block is OVER the MAX block Index, new_CS: %x, %x, %x\n", new_CS, gCS_ID, b);
        }
        new_CS = (uint8_t)(b / 1024);
        if (gCS_ID != new_CS)
        {
            // do CS cmd, send CS command
            // (void)printf("new_CS: %x, %x, %x\n",new_CS,gCS_ID, b);
            n_cmdSwDieSelect(new_CS);

#ifdef  FAST_NAND_READ
            regA0   = -1;
            regB0   = -1;
#endif

            if ((new_CS != 0) && (g1stDieSel_D1 != 0))
            {
                uint8_t status = 0;

                // check buffer mode of 0xB0
#ifdef  USE_MMP_DRIVER
                if (spiNf_getFeature(0xB0, &status) == 0)
                {
                    (void)printf(" get feature_swDie(B0)=%x!!\n", status);
                }
#else
                uint8_t s = 0;
                if (spiNf_getFeature(0xB0, &status) == 0)
                {
                    s = 1;
                }
#endif
                if ((status & 0x08u) == 0)
                {
                    uint8_t newVal = (status | 0x08u);
                    (void)spiNf_setFeature(0xB0, newVal);
                    (void)printf(" set feature(B0)=%x!!\n", newVal);
                }

                // check block protect status of 0xA0 reg
#ifdef  USE_MMP_DRIVER
                if (spiNf_getFeature(0xA0, &status) == 0)
                {
                    (void)printf(" get feature_swDie(A0)=%x!!\n", status);
                }
#else
                if (spiNf_getFeature(0xA0, &status) == 0)
                {
                    s = 2;
                }
#endif
                if ((status & 0x38u) != 0)
                {
                    (void)spiNf_setFeature(0xA0, 0x00);
#ifdef  USE_MMP_DRIVER
                    (void)printf(" set feature(A0)=%x!!\n", 0);
                    if (spiNf_getFeature(0xA0, &status) != 0)
                    {
                        (void)printf(" get feature_swDie(A0) fail, s=%x!!\n", status);
                    }
                    if ((status & 0x38u) != 0)
                    {
                        (void)printf(" get feature(A0)=%x!!\n", status);
                    }
#endif
                }

                g1stDieSel_D1 = 0;
            }

            // check A0 & B0 during DIE selecting
            {
                uint8_t rA0 = 0;
                uint8_t rB0 = 0;
#ifdef  ENABLE_DBG_MSG_DIE_SELECT
                static uint8_t rC0 = 0;
#endif
                uint8_t res = 0;

                res = spiNf_getFeature(0xA0, &rA0);
                if(res != 0)
                {
					(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
                }
                res = spiNf_getFeature(0xB0, &rB0);
                if(res != 0)
                {
					(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
                }
#ifdef  ENABLE_DBG_MSG_DIE_SELECT
                res = spiNf_getFeature(0xC0, &rC0);
                if(res != 0)
                {
					(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
                }
                (void)printf(" get feature_swDie2(A0)=%x!!\n", rA0);
                (void)printf(" get feature_swDie2(B0)=%x!!\n", rB0);
                (void)printf(" get feature_swDie2(C0)=%x!!\n", rC0);
                (void)printf("DS0: gCS_ID=(%x, %x) o=%x, n=%x\n", gCS_ID, new_CS, b, (b % 1024));
#endif

#ifdef  ENABLE_PATCH_DIE_SELECT
                if ((rA0 & 0x38u) != 0)
                {
                    (void)printf(" [warning] Got incorrect A0=%x!!\n", rA0);
                    res = spiNf_setFeature(0xA0, 0x00);
					if(res != 0)
					{
						(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
					}
                    res = spiNf_getFeature(0xA0, &rA0);
					if(res != 0)
					{
						(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
					}
                    if ((rA0 & 0x38u) != 0)
                    {
                        (void)printf(" [Error] incorrect value of A0=%x!!\n", rA0);
                    }
                }

                if (gForceEccEnable != 0)
                {
                    if (rB0 != 0x18)
                    {
                        uint8_t nrB0 = 0;
                        (void)printf(" [warning.1] M02's B0 != 18h, B0=%x!!\n", rB0);
                        rB0 = 0x18;
                        res = spiNf_setFeature(0xB0, rB0);
						if(res != 0)
						{
							(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
						}
                        res = spiNf_getFeature(0xB0, &nrB0);
						if(res != 0)
						{
							(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
						}
                        if (nrB0 != 0x18)
                        {
                            (void)printf(" [Error.1] M02's Can not enable ECC-en bit, B0=%x, %x!!\n", rB0, nrB0);
                        }
                    }
                }
                else
                {
                    if (rB0 != 0x08)
                    {
                        uint8_t nrB0 = 0;
                        (void)printf(" [warning.2] M02's ECC is not enabled, B0=%x!!\n", rB0);
                        rB0 = 0x08;
                        res = spiNf_setFeature(0xB0, rB0);
						if(res != 0)
						{
							(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
						}
                        res = spiNf_getFeature(0xB0, &nrB0);
						if(res != 0)
						{
							(void)printf("%s[%d]Get Feature Fail: r=%x\n", __FILE__, __LINE__,res);
						}
                        if (nrB0 != 0x08)
                        {
                            (void)printf(" [Error.2] M02 Can not enable ECC-en bit, B0=%x, %x!!\n", rB0, nrB0);
                        }
                    }
                }
#endif
            }

            gCS_ID = new_CS;
        }

#ifdef  ENABLE_DBG_MSG_DIE_SELECT
        // (void)printf("DS1: gCS_ID=%x, o=%x, n=%x\n", gCS_ID, b ,(b % 1024));
#endif
        return (b % 1024);
    }

    return b;
}

/*
remapping spare data structure
*/
static void
n_reMapSpare (
    uint8_t * ptr1,
    uint8_t * ptr2,
    uint8_t mapType)
{
    int     i;
    uint8_t * reMap             = NULL;
    uint8_t forFixGdOriReMap    = 0;

    if (gNeedReMapSpare == 0)
    {
        if ((gSpiNfInfo.MID == 0xC8) || (gSpiNfInfo.MID == 0x98) || (gSpiNfInfo.MID == 0x01) || (gSpiNfInfo.MID == 0xCD) || (gSpiNfInfo.MID == 0x9B))
        {
            forFixGdOriReMap    = 1;
            reMap               = (uint8_t *)gOriSprMapTable;
        }
        else
        {
            (void)printf("SPI NAND ERROR!! this MID(%x) is NOT suitable for spare data re-mapping\n", gSpiNfInfo.MID);
            return;
        }
    }

    if (gSpiNfInfo.MID == 0xEF)
    {
        if (gWINBOND_NEW_SPARE != 0)
        {
            // (void)printf("support WB2 spare format\n");
            reMap = (uint8_t *)gWb2SprMapTable;
        }
        else
        {
            reMap = (uint8_t *)gWbSprMapTable;
        }
    }

    if (gPARAGON_MID_0xA1 != 0)
    {
        reMap = (uint8_t *)gPrgSprMapTable;
    }

    if (gTEXIN_MID_0xA1 != 0)
    {
        reMap = (uint8_t *)gEsmt2SprMapTable;
    }

    if (gSpiNfInfo.MID == 0x0B)
    {
        reMap = (uint8_t *)gXtxSprMapTable;
    }

    if (gSpiNfInfo.MID == 0xE5)
    {
        reMap = (uint8_t *)gDsSprMapTable;
    }

    if (gSpiNfInfo.MID == 0xC8)
    {
        if (forFixGdOriReMap != 0)
        {
            reMap = (uint8_t *)gOriSprMapTable;
        }
        else
        {
            if (gESMT_MID_0xC8 != 0)
            {
                reMap = (uint8_t *)gEsmt2SprMapTable;
            }
            else
            {
                reMap = (uint8_t *)gGdSprMapTable;
            }
        }
    }

    if (gSpiNfInfo.MID == 0xC2)
    {
		//(void)printf("	##### MXIC Do Select Spare MAP: id0=%x, id1=%x #####\n",gSpiNfInfo.DID0,gSpiNfInfo.DID1);
		if((gSpiNfInfo.DID0 == (uint8_t)0x26) && (gSpiNfInfo.DID1 == (uint8_t)0x01))
		{
			//void)printf("	### MXIC original Spare mapping! ###\n");
			reMap = (uint8_t*)gMxicOldSprMapTable;	
		}
		else
		{
			reMap = (uint8_t*)gGdSprMapTable;
		}	
    }

    if (gSpiNfInfo.MID == 0x2C)
    {
		if(gESMT_MID_0X2C)
		{			
			reMap = (uint8_t *)gEsmt3SprMapTable;
		}
		else
		{			
			reMap = (uint8_t *)gEsmt1SprMapTable;
		}
    }

    if (reMap == NULL)
    {
        (void)printf("[Error] reMap is NULL!!\n");
        return;
    }

    if (mapType == SPINF_REMAP_SPARE_FOR_READ)
    {
        // from NAND to Memory(read)
        static uint32_t printCnt    = 0;
        uint8_t         directMap   = 0;
#ifdef  ENABLE_NEW_WB_SPARE_MAP
        uint8_t         oriwbMap    = 0;
#endif
        if (forFixGdOriReMap == 0)
        {
            if ((gSpiNfInfo.MID == 0xC8) || (gSpiNfInfo.MID == 0xC2))
            {
                if ((ptr2[0x10] == 0x57) && (ptr2[0x11] == 0x4C) && (ptr2[0x12] == 0x54) && (ptr2[0x13] == 0x46))
                {
                    if (printCnt++ < 10)
                    {
                        if (gSpiNfInfo.MID == 0xC8)
                        {
                            (void)printf("[Warning]: Got old GigaDevice's Spr format, use original remapping\n");
                        }
                        else
                        {
                            (void)printf("[Warning]: Got old MXIC's Spr format, use original remapping\n");
                        }
                    }
                    directMap = 1;
                }
            }

#ifdef  ENABLE_NEW_WB_SPARE_MAP
            if (gSpiNfInfo.MID == 0xEF)
            {
                // (void)printf("STOP for Winbond Spare Mapping\n");
                // ithPrintVram8(ptr2, 64);
                if ((ptr2[0x0] == 0x57) && (ptr2[0x1] == 0x4C) && (ptr2[0x2] == 0x54) && (ptr2[0x3] == 0x46))
                {
                    if (printCnt++ < 10)
                    {
                        (void)printf("[Warning]: Got old WinBond's Spare format, use original remapping\n");
                    }
                    oriwbMap = 1;
                }
            }
#endif
        }

        for (i = 0; i < 24; i++)
        {
            if (directMap != 0)
            {
                ptr1[i] = ptr2[i];
            }
            else
            {
                ptr1[i] = ptr2[reMap[i]];
            }
#ifdef  ENABLE_NEW_WB_SPARE_MAP
            if (oriwbMap && (i >= 16) && (i < 20))
            {
                ptr1[i] = ptr2[i - 16];
            }
#endif
            // (void)printf("i2=[%d][%x,%x,%x]\n",i,reMap[i],ptr2[reMap[i]],ptr1[i]);
        }
    }
    else
    {
        // from Memory to NAND(write)
        for (i = 0; i < 24; i++)
        {
            ptr1[reMap[i]] = ptr2[i];
            // (void)printf("i=[%d][%x,%x,%x]\n", i, reMap[i], ptr2[i], ptr1[reMap[i]]);
        }
    }
}

#define TOSHIBA_HSE_BIT     (0x02)
#define TOSHIBA_HSE_ENABLE  (1)
#define TOSHIBA_HSE_DISABLE (0)

#if 0
static void
_enHighSpeedMode (
    uint8_t mode)
{
    unsigned char regStatus = 0;

    if (spiNf_getFeature(0xB0, &regStatus))
    {
        (void)printf("[SPINF ERR] get feature commnad fail\n");
        // spiret = SPINF_ERROR_GET_FEATURE_CMD_FAIL;
        // goto hseEnd;
        return;
    }

    // (void)printf("enHSE:m=%d, reg=%x\n",mode,regStatus);

    if (mode)
    {
        if (!(regStatus & TOSHIBA_HSE_BIT))
        {
            // go to enable TOSHIBA_HSE_BIT
            spiNf_setFeature(0xB0, (regStatus | TOSHIBA_HSE_BIT));
        }
    }
    else
    {
        if (regStatus & TOSHIBA_HSE_BIT)
        {
            // go to disable TOSHIBA_HSE_BIT
            spiNf_setFeature(0xB0, (regStatus & ~TOSHIBA_HSE_BIT));
        }
    }
}
#endif

// =============================================================================
//                              Public Function Definition
// =============================================================================

// =============================================================================

/**
* read ID of SPI NAND
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_ReadId (
    unsigned char * id)
{
    unsigned char   SpiCmd[2];
    unsigned char   SpiData[3];
    unsigned char   regStatus;
    int             spiret, i;

    // reset 0xFF
    if (n_cmdReset(0) != 0)
    {
        (void)printf("[SPINF ERR] reset commnad fail:0\n");
        spiret = SPINF_ERROR_RESET_CMD_FAIL;
        goto errRdIdEnd;
    }

    for (i = 0; i < 3; i++)
    {
        if (spiNf_getFeature(0xA0 + (0x10 * i), &regStatus) != 0)
        {
            (void)printf("[SPINF ERR] get feature commnad fail\n");
            spiret = SPINF_ERROR_GET_FEATURE_CMD_FAIL;
            goto errRdIdEnd;
        }
    }

    SpiData[0]  = 0;
    SpiData[1]  = 0;
    SpiData[2]  = 0;
    SpiCmd[0]   = 0x9F;
    spiret      = spi_read(SpiCmd, 1, SpiData, 3);

    id[0]       = SpiData[0];
    id[1]       = SpiData[1];
    id[2]       = SpiData[2];

    if (spiret == SPINF_OK)
    {
        (void)printf("SPI READ ID PASS, data=%02x,%02x,%02x\n", id[0], id[1], id[2]);
        spiret = 0;
    }
    else
    {
        (void)printf("SPI READ ID FAIL,result=%x\n", spiret);
    }

errRdIdEnd:

    return (spiret);
}

// =============================================================================

/**
* read ID with dummy byte of SPI NAND
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_DummyReadId (
    unsigned char * id)
{
    unsigned char   SpiCmd[2];
    unsigned char   SpiData[3];
    unsigned char   regStatus;
    int             spiret, i;

    // reset 0xFF
    if (n_cmdReset(0) != 0)
    {
        (void)printf("[SPINF ERR] (dummy read ID)reset commnad fail:0\n");
        spiret = SPINF_ERROR_RESET_CMD_FAIL;
        goto errEnd;
    }

    for (i = 0; i < 3; i++)
    {
        if (spiNf_getFeature(0xA0 + (0x10 * i), &regStatus) != 0)
        {
            // (void)printf("[SPINF ERR] get feature commnad fail\n");
            spiret = SPINF_ERROR_GET_FEATURE_CMD_FAIL;
            goto errEnd;
        }
    }

    SpiData[0]  = 0;
    SpiData[1]  = 0;
    SpiData[2]  = 0;
    SpiCmd[0]   = 0x9F;
    SpiCmd[1]   = 0x00;
    spiret      = spi_read(SpiCmd, 2, SpiData, 3);

    id[0]       = SpiData[0];
    id[1]       = SpiData[1];

    // GD & MXIC has 2 IDs, but Winbond has 3 IDs.
    if (SpiData[0] != 0xEF)
    {
		id[2] = 0;

		if ((SpiData[0] == 0xC2) && (SpiData[1] == 0x37))
		{
			id[2] = SpiData[2];
		}
				
		if ((SpiData[0] == 0xC2) && (SpiData[1] == 0x26) && (SpiData[2] == 0x01))
		{
			id[2] = SpiData[2];			
		}

		if (SpiData[0] == 0xC2)
		{
			(void)printf("A.MXIC's ID: %02x,%02x,%02x\n",SpiData[0],SpiData[1],SpiData[2]);
			(void)printf("B.MXIC's ID: %02x,%02x,%02x\n",id[0],id[1],id[2]);
		}
    }
    else
    {
        id[2] = SpiData[2];
    }

    // KIOXIA also has 3 IDs.
    if (id[0] == 0x98)
    {
        // TOSHIBA 1st gen ID list:0xB2/C2, 0xBB/CB, 0xBD/CD has 2 IDs (No program X4 cmd 32h)
        // KIOXIA 2nd gen ID list:0xE2/D2, 0xEB/DB, 0xED/DD, 0xE4/D4 has 3 IDs (Has program X4 cmd 32h)
        if ((id[1] & 0xF0u) >= 0xD0)
        {
            id[2] = SpiData[2];         // KIOXIA 2nd gen NAND has 3 IDs
        }
        else
        {
            g_TOSHIBA_QuadMode = 1;     // (TOSHIBA 1st gen NAND has No program X4 cmd 32h)
        }
    }

    if (spiret == SPINF_OK)
    {
        (void)printf("SPI READ ID(with dummy byte) PASS, data=%02x,%02x,%02x\n", id[0], id[1], id[2]);
        gUseDummyReadId = 1;
        spiret          = 0;
    }
    else
    {
        // (void)printf("SPI READ ID FAIL,result=%x\n",spiret);
    }

errEnd:

    return (spiret);
}

// =============================================================================
/**
* set Quad I/O Operation of SPI NAND
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/
// =============================================================================
#ifdef USE_AXISPI_ENGINE

uint8_t
spiNf_setQuadIoOperation (
    unsigned char value)
{
    uint8_t B0reg   = 0;
    uint8_t setBit  = 0;

#ifdef USE_AXISPI_ENGINE
    #ifdef FAST_NAND_READ
static int  gCurrSpiIoMode  = -1;
    #else
static int  gCurrSpiIoMode  = SPI_NAND_SINGLE_OPERATION;
    #endif
#endif

    #ifdef FAST_NAND_READ
    if (gCurrSpiIoMode == value)
    {
        return 0;
    }
    #endif

    // because toshiba has NO Quad-IO register, so skip it derectly.
    if (gSpiNfInfo.MID == 0x98)
    {
        return 0;
    }

    // because SkyHigh has NO Quad-IO register, so skip it derectly.
    if (gSpiNfInfo.MID == 0x01)
    {
        return 0;
    }

    // because esmt has NO Quad-IO register, so skip it derectly.
    if (gSpiNfInfo.MID == 0x2C)
    {
        return 0;
    }

    // because esmt has NO Quad-IO register, so skip it derectly.
    if (gESMT_MID_0xC8 != 0)
    {
        return 0;
    }

    // because TEXIN has NO Quad-IO register, so skip it derectly.
    if (gTEXIN_MID_0xA1 != 0)
    {
        return 0;
    }

    // because winbond has NO Quad-IO register, so skip it derectly.
    if (gSpiNfInfo.MID == 0xEF)
    {
        return 0;
    }

    if (spiNf_getFeature(0xB0, &B0reg) == 0)
    {
        if (value != 0)
        {
            setBit = 0x01;
        }

        if (((B0reg & 0x10u) == 0) && (gForceEccEnable != 0))
        {
            // if ECC is not enabled, enable it.
            (void)printf("[Error] the ECC is not enabled. B0=%x\n", B0reg);
        }

        if ((value != 0) && ((B0reg & 0x01u) != 0))
        {
            gCurrSpiIoMode = SPI_NAND_QUAD_OPERATION;
            goto sEnd;
        }

        if ((value == 0) && ((B0reg & 0x01u) == 0))
        {
            gCurrSpiIoMode = SPI_NAND_SINGLE_OPERATION;
            goto sEnd;
        }

        if (spiNf_setFeature(0xB0, (B0reg & 0xFEu) | setBit) == 0)     // 0 is OK
        {
            if (value != 0)
            {
                gCurrSpiIoMode = SPI_NAND_QUAD_OPERATION;
            }
            else
            {
                gCurrSpiIoMode = SPI_NAND_SINGLE_OPERATION;
            }

            if (spiNf_getFeature(0xB0, &B0reg) == 0)
            {
                if (((B0reg & 0x10u) == 0) && (gForceEccEnable != 0))
                {
                    (void)printf("{Error] the ECC can not enable. %x, %x\n", value, B0reg);
                }

                if ((gTEXIN_MID_0xA1 == 0) && ((B0reg & 0x01u) != setBit))
                {
                    (void)printf("get new 0xB0 Reg = %x, %x\n", value, B0reg);
                }
            }
        }
    }

sEnd:
    if (value != 0)
    {
        if (gCurrSpiIoMode == SPI_NAND_QUAD_OPERATION)
        {
            return (0);       // OK
        }
        else
        {
            return (1);       // NOT SUCCESS
        }
    }
    else
    {
        if (gCurrSpiIoMode == SPI_NAND_SINGLE_OPERATION)
        {
            return (0);       // OK
        }
        else
        {
            return (1);       // NOT SUCCESS
        }
    }
    // if(gCurrSpiIoMode == SPI_NAND_QUAD_OPERATION)  return 0;    //OK
    // else  return 1;    //NOT SUCCESS
}
#endif

// =============================================================================

/**
* initialization of SPI NAND
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_Initial (
    SPI_NF_INFO * info)
{
    unsigned char   idBuf[4];
    uint32_t        result;
	static uint8_t  gSpiNfInitFinished = 0;
#ifdef USE_AXISPI_ENGINE
	static uint8_t  * gNfAddrBase = NULL;
	static uint8_t  * gNfDataBase = NULL;
#endif
	uint8_t * gSprMapTable = (uint8_t*)gOriSprMapTable;

    gUseDummyReadId = 0;

#ifdef USE_AXISPI_ENGINE
    if (gSpiAddrBuf == NULL)
    {
        gNfAddrBase = (uint8_t *)malloc(5 + AXISPI_ALIGN_SIZE - 1);
        gSpiAddrBuf = (uint8_t *)((uint32_t)((uint32_t)gNfAddrBase + AXISPI_ALIGN_SIZE - 1) & ~((uint32_t)AXISPI_ALIGN_SIZE - 1));
        (void)printf("spi nand address buffer base1: 0x%p, addrBuf = 0x%p\n", gNfAddrBase, gSpiAddrBuf);
    }

    if (gSpiDataBuf == NULL)
    {
        gNfDataBase = (uint8_t *)malloc(4096 + 512 +  AXISPI_ALIGN_SIZE - 1);
        gSpiDataBuf = (uint8_t *)((uint32_t)((uint32_t)gNfDataBase + AXISPI_ALIGN_SIZE - 1) &  ~((uint32_t)AXISPI_ALIGN_SIZE - 1));
        (void)printf("spi nand data buffer base: 0x%p, dataBuf = 0x%p\n", gNfDataBase, gSpiDataBuf);
    }

    if (gSpiNfInitFinished == 0)
    {
    #ifdef USE_AXISPI_ENGINE
        //result = mmpAxiSpiInitialize(NF_SPI_PORT, SPI_OP_MASTR, CPO_0_CPH_0, SPI_CLK_20M, SPI_CLK_DIV_0);
        result = mmpAxiSpiInitializeMulti(NF_SPI_PORT, NAND_CSN, SPI_OP_MASTR, CPO_0_CPH_0, SPI_CLK_20M, SPI_CLK_DIV_0);
        if (result != 0)
        {
            (void)printf("AxiInit:%x\n", result);
        }
    #else
        result = mmpSpiInitialize(NF_SPI_PORT, SPI_OP_MASTR, CPO_0_CPH_0, SPI_CLK_20M);
    #endif
    }
#else
    result = mmpSpiInitialize(NF_SPI_PORT, SPI_OP_MASTR, CPO_0_CPH_0, SPI_CLK_20M);
#endif

#ifdef USE_AXISPI_ENGINE
    #ifdef ENABEL_NEW_WAIT_READY
    AxiSpiSetBusyBit(0);
    #endif
#endif

    // readId
#if 0
    while (1)
    {
        if (spiNf_ReadId(idBuf))
        {
            return 0x11;
        }

        if ((idBuf[0] == 0xFF) || (idBuf[0] == 0x00))
        {
            break;
        }

        if (retry_id > 3)
        {
            break;
        }
    }
#else
    if (spiNf_ReadId(idBuf) != 0)
    {
        return 0x11;
    }
#endif

    if ((idBuf[0] == 0xFF) || (idBuf[0] == 0x00))
    {
        if (spiNf_DummyReadId(idBuf) != 0)
        {
            return 0x11;
        }
    }

    // get attribute from cfg(cfgArray)
    if (n_setAttribute(info, idBuf) != 0)
    {
        return 0x12;
    }
#if CFG_NAND_READ_CACHE_SIZE > 0
    if (NAND_cache_init (info->PageSize + info->SprSize))
    {
        (void)printf("NAND cache init fail.");
    }
#endif
    // force to enable ECC
    if (n_SetEcc(ECC_ENABLE) == 1)
    {
        uint8_t status = 0;
        if (spiNf_getFeature(0xB0, &status) == 0)
        {
            (void)printf(" get feature(B0)=%x!!\n", status);
        }
    }
	
	if(info->PageSize != 0)
    {
        //snCfg       = g_SpiNfCfgArray;
        //snCfgCnt    = sizeof(g_SpiNfCfgArray) / sizeof(SPINF_CFG);
		(void)printf("g_SpiNfCfgArray:%p, CfgCnt:%x, %x, %x\n", g_SpiNfCfgArray, (sizeof(g_SpiNfCfgArray) / sizeof(SPINF_CFG)), sizeof(g_SpiNfCfgArray), sizeof(SPINF_CFG));
    }
		

    gSpiNfInfo.supportBlkLock = 0;

    if (gSpiNfInfo.MID == 0xC2)
    {
        uint8_t status = 0;
		
        // do MXIC initial flow
        if (spiNf_getFeature(0x10, &status) == 0)
        {
            (void)printf(" get feature(10)=%x!!\n", status);
        }
        if (spiNf_getFeature(0x60, &status) == 0)
        {
            (void)printf(" get feature(60)=%x!!\n", status);
        }
        if (spiNf_getFeature(0x70, &status) == 0)
        {
            (void)printf(" get feature(70)=%x!!\n", status);
        }
        if (spiNf_getFeature(0xA0, &status) == 0)
        {
            (void)printf(" get feature(A0)=%x!!\n", status);
        }
        if ((status & 0x38u) != 0)
        {
            (void)spiNf_setFeature(0xA0, 0x00);
            if (spiNf_getFeature(0xA0, &status) == 0)
            {
                (void)printf(" get feature(A0)=%x!!\n", status);
            }
        }
        if (spiNf_getFeature(0xB0, &status) == 0)
        {
            (void)printf(" get feature(B0)=%x!!\n", status);
        }
        if (spiNf_getFeature(0xC0, &status) == 0)
        {
            (void)printf(" get feature(C0)=%x!!\n", status);
        }
        if (spiNf_getFeature(0xE0, &status) == 0)
        {
            (void)printf(" get feature(E0)=%x!!\n", status);
        }

        gNeedReMapSpare = 1;    // use "gGdSprMapTable[]" for MXIC's spare format
		gSprMapTable = (uint8_t *)gGdSprMapTable;

        if (gSpiNfInfo.DID0 == (uint8_t)0x35)
        {
            g2PlaneNandShift = 5;
        }

        if (gSpiNfInfo.DID0 == (uint8_t)0xA4)
        {
            g2PlaneNandShift = 4;                           // ?? No ID is "C2 A4"
        }
        if (gSpiNfInfo.DID0 == (uint8_t)0xA0)
        {
            g2PlaneNandShift    = 4;
            g2PlaneReadCmd      = 1;
        }

        if ((gSpiNfInfo.DID0 == (uint8_t)0x12) || (gSpiNfInfo.DID0 == (uint8_t)0x22))
        {
            gSupportEccErrBit = 1;      // enable this function
        }

        if ((gSpiNfInfo.DID0 == (uint8_t)0x37) || (gSpiNfInfo.DID0 == (uint8_t)0x26))
        {
            gSupportEccErrBit   = 1;    // enable this function
            gEccErrThreshold    = 8;
        }

		if((gSpiNfInfo.DID0 == (uint8_t)0x26) && (gSpiNfInfo.DID1 == (uint8_t)0x01))
		{
			gSupportEccErrBit = 1;	//enable this function
			gEccErrThreshold = 8;
			gSprMapTable = (uint8_t *)gMxicOldSprMapTable;			
			(void)printf("Set MX35LF2GE4AC ECC: %x, %x\n",gSupportEccErrBit, gEccErrThreshold);
		}
		
        (void)printf(" MXIC 2-plane NAND: %x!!\n", g2PlaneNandShift);
    }

    if (gSpiNfInfo.MID == (uint8_t)0xEF)
    {
        uint8_t status = 0;		
        // do winbond initial flow
        // Force to set ECC-E & BUF as 1
        // Winbond has 2 type SPI NAND
        // W25N01GVxxIT --> default BUF=0
        // W25N01GVxxIG --> default BUF=1
        // ITE's nand driver only can handle BUF=1
        if (spiNf_getFeature(0xA0, &status) == 0)
        {
            (void)printf(" get feature(A0)=%x!!\n", status);
        }		
        if ((status & 0x38u) != 0)
        {
            uint8_t tmp = 0;
            (void)spiNf_setFeature(0xA0, 0x00);
            if (spiNf_getFeature(0xA0, &tmp) == 0)
            {
                (void)printf(" re-get reg(A0)=%x!!\n", tmp);
            }
        }

        if (spiNf_getFeature(0xB0, &status) == 0)
        {
            (void)printf(" get feature(B0)=%x!!\n", status);
        }
        if ((status & 0x08u) == 0)
        {
            (void)spiNf_setFeature(0xB0, 0x18);
        }
        if ((status & 0x10u) == 0)
        {
            (void)printf("[Error] default B0reg's ECC is disabled=%x!!\n", status);
        }

        if (spiNf_getFeature(0xC0, &status) == 0)
        {
            (void)printf(" get feature(C0)=%x!!\n", status);
        }
		
		gSprMapTable = (uint8_t *)gWbSprMapTable;

        if ((gSpiNfInfo.MID == (uint8_t)0xEF) && (gSpiNfInfo.DID0 == (uint8_t)0xAB) && (gSpiNfInfo.DID1 == (uint8_t)0x21))
        {
            gDualDieNand = 1;
        }

        if (gSpiNfInfo.DID0 == (uint8_t)0xAE)
        {
            (void)printf("Winbond Use WB2 spare format\n");
            gWINBOND_NEW_SPARE = 1;			
			gSprMapTable = (uint8_t *)gWb2SprMapTable;
			gSupportEccErrBit = 1;	//enable this function
			gEccErrThreshold = 4;
        }

        /**********************************************************************
          W25N512GV, W25N01GV, W25M02GV(GV type) are gWINBOND_ECC_30H_FAIL = 1;
          W25N01GV, W25N02GV, W25N04GV(KV type) are gWINBOND_ECC_30H_FAIL = 0;
        **********************************************************************/
        if (gSpiNfInfo.DID0 == (uint8_t)0xAA)
        {
            if ((gSpiNfInfo.DID1 == (uint8_t)0x20) || (gSpiNfInfo.DID1 == (uint8_t)0x21))
            {
                (void)printf("Winbond ECC type II(ECC1 & ECC0 = 11 is fail)\n");
                gWINBOND_ECC_30H_FAIL = 1;
            }
			
			if((gSpiNfInfo.DID1 == (uint8_t)0x22) || (gSpiNfInfo.DID1 == (uint8_t)0x23))
			{
				(void)printf("##### WB support reWt #####\n");
				gSupportEccErrBit = 1;	//enable this function
				gEccErrThreshold = 8;
			}
        }

        if (gSpiNfInfo.DID0 == (uint8_t)0xAB)
        {
            if (gSpiNfInfo.DID1 == (uint8_t)0x21)
            {
                (void)printf("Winbond ECC type II(ECC1 & ECC0 = 11 is fail)\n");
                gWINBOND_ECC_30H_FAIL = 1;
            }
        }
		
		if (gSupportEccErrBit != 0u)
		{
			uint8_t res = 0;
			uint8_t sReg = 0;
			uint8_t SetEccCnt = (uint8_t)(gEccErrThreshold << 4);
			
			res = spiNf_setFeature(0x10, SetEccCnt);
			if(spiNf_getFeature(0x10, &sReg) == 0)
			{
				(void)printf(" get feature(10)=%x, t=%x, r=%x !!\n",sReg, gEccErrThreshold, SetEccCnt);
			}
			else
			{
				(void)printf(" get feature(0x10) Error!!\n");
			}
		}

		/* Summary: Only 4-bit/8-bit ECC Can Support reWrite function */
		/* 			In other Hand, 1-bit ECC must be gWINBOND_ECC_30H_FAIL=1 & without reWrite function */
    }
    (void)printf("[Spinfdrv.c] MID=%x,MID0=%x,MID1=%x!\n", gSpiNfInfo.MID, gSpiNfInfo.DID0, gSpiNfInfo.DID1);

    if (gSpiNfInfo.MID == (uint8_t)0xA1)
    {
        uint8_t status = 0;
		gSprMapTable = (uint8_t *)gPrgSprMapTable;

        // check if the PARAGON or Fudan MicroElectronics
        if (gSpiNfInfo.DID0 == (uint8_t)0xE1)
        {
            gPARAGON_MID_0xA1 = 1;
			gSprMapTable = (uint8_t *)gPrgSprMapTable;
        }

        if (gSpiNfInfo.DID0 == (uint8_t)0xF1)
        {
			uint8_t  gEccRegOffset           = 0x90;	//move this valuable here for MISRA C-2012 Rule 8.9
			
            gTEXIN_MID_0xA1 = 1;
			gSprMapTable = (uint8_t *)gEsmt2SprMapTable;

            //gEccRegOffset   = 0x90;

            // ECC enable bit is in Reg0x90(Not 0xB0)
            if (spiNf_getFeature(gEccRegOffset, &status) == 0)
            {
                (void)printf(" get feature(90)=%x!!\n", status);
            }
            if ((status & 0x10u) == 0)
            {
                (void)spiNf_setFeature(0x90, (status | 0x10u));                        // ENABLE ECC function
            }
        }

        if (spiNf_getFeature(0xA0, &status) == 0)
        {
            (void)printf(" get feature(A0)=%x!!\n", status);
        }
        if (spiNf_getFeature(0xB0, &status) == 0)
        {
            (void)printf(" get feature(B0)=%x!!\n", status);
        }
        if (((status & 0x10u) == 0) && (gTEXIN_MID_0xA1 == 0))
        {
            (void)spiNf_setFeature(0xB0, (status | 0x10u));                                        // ENABLE ECC function
        }
        if (spiNf_getFeature(0xC0, &status) == 0)
        {
            (void)printf(" get feature(C0)=%x!!\n", status);
        }
        if (spiNf_getFeature(0x90, &status) == 0)
        {
            (void)printf(" get feature(90)=%x!!\n", status);
        }
    }

    if (gSpiNfInfo.MID == 0x2C)
    {
        uint8_t status = 0;		
		
		if (gSpiNfInfo.DID0 == (uint8_t)0x34)
		{
			gSprMapTable = (uint8_t *)gEsmt3SprMapTable;
			gESMT_MID_0X2C = 1;
            gSupportEccErrBit   = 1;    // enable this function
            gEccErrThreshold    = 8;
			
			if (spiNf_getFeature(0xA0, &status) == 0)
			{
				(void)printf(" ESMT get feature(A0)=%x!!\n", status);
				if (status != 0u)
				{
					uint8_t s2 = 0;	
					(void)spiNf_setFeature(0xA0, 0u);                        // Set A0 as 0
					if (spiNf_getFeature(0xA0, &s2) == 0)
					{
						if (s2 != 0u)
						{
							(void)printf(" ESMT A0.reg still not 0: %x!!\n", s2);
							while(1);
						}
					}
				}
			}
	
			if (spiNf_getFeature(0xB0, &status) == 0)
			{
				(void)printf(" get feature(B0)=%x!!\n", status);
				
				if ((status & 0x01) == 0x01u)
				{
					uint8_t s2 = 0;	
					(void)spiNf_setFeature(0xB0, 0x10u);                        // Set B0 as 0x10
					if (spiNf_getFeature(0xB0, &s2) == 0)
					{
						if (s2 != 0x10u)
						{
							(void)printf(" ESMT B0.reg still not 0x10: %x!!\n", s2);
							while(1);
						}
					}
				}				
			}
			if ((status & 0x10u) == 0)
			{
				(void)spiNf_setFeature(0xB0, (status | 0x10u));                    // ENABLE ECC function
			}
			if (spiNf_getFeature(0xC0, &status) == 0)
			{
				(void)printf(" get feature(C0)=%x!!\n", status);
			}
			gNeedReMapSpare = 1;
		}

        if (gSpiNfInfo.DID0 == (uint8_t)0x24)
        {
			gSprMapTable = (uint8_t *)gEsmt1SprMapTable;
			
            g2PlaneNandShift    = 4;
            g2PlaneReadCmd      = 1;
			
			if (spiNf_getFeature(0xA0, &status) == 0)
			{
				(void)printf(" get feature(A0)=%x!!\n", status);
			}
			if((status & 0x7Cu) != 0)
			{
				uint8_t	spiret = 0;
				uint8_t	SpiCmd[2];
	
				SpiCmd[0] = 0x06;
				spiret = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
				if(spiret != SPINF_OK)
				{
					spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
				}
				(void)spiNf_setFeature(0xA0, 0x02);
				(void)spiNf_setFeature(0xA0, 0x00);
				
				SpiCmd[0] = 0x04;
				spiret = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
				if(spiret != SPINF_OK)
				{
					spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
				}
				(void)spiNf_getFeature(0xA0, &status);	    
				(void)printf(" XTX's get_ftr1[0xA0] = %x!!\n",status);		
			}
	
			if (spiNf_getFeature(0xB0, &status) == 0)
			{
				(void)printf(" get feature(B0)=%x!!\n", status);
			}
			if ((status & 0x10u) == 0)
			{
				(void)spiNf_setFeature(0xB0, (status | 0x10u));                    // ENABLE ECC function
			}
			if (spiNf_getFeature(0xC0, &status) == 0)
			{
				(void)printf(" get feature(C0)=%x!!\n", status);
			}
			if (spiNf_getFeature(0x90, &status) == 0)
			{
				(void)printf(" get feature(90)=%x!!\n", status);
			}
			gNeedReMapSpare = 1;
        }
     }

    if (gSpiNfInfo.MID == 0x0B)
    {
        uint8_t status = 0;
		
		gSprMapTable = (uint8_t *)gXtxSprMapTable;

        // check if the TYPE C chip
        if ((gSpiNfInfo.DID0 & 0xF0u) == (uint8_t)0x10)
        {
            uint8_t reg = gSpiNfInfo.DID0 & 0x0Fu;

            // gXTX_TYPE_C is just for DID0 = 0x11 ~ 0x14, exclude XT26G11C(ID: 0Bh 15h)
            if (reg <= 4)
            {
                gXTX_TYPE_C = 1;
            }

            // (void)printf(" gXTX_TYPE_C = %x!!\n",gXTX_TYPE_C);
        }

		gSupportEccErrBit = 1;	//enable this function
		gEccErrThreshold = 8;

        if (spiNf_getFeature(0xA0, &status) == 0)
        {
            (void)printf(" get feature(A0)=%x!!\n", status);
        }
        if (spiNf_getFeature(0xB0, &status) == 0)
        {
            (void)printf(" get feature(B0)=%x!!\n", status);
        }
        if ((status & 0x10u) == 0)
        {
            (void)spiNf_setFeature(0xB0, (status | 0x10u));                    // ENABLE ECC function
        }
        if (spiNf_getFeature(0xC0, &status) == 0)
        {
            (void)printf(" get feature(C0)=%x!!\n", status);
        }
        if (spiNf_getFeature(0x90, &status) == 0)
        {
            (void)printf(" get feature(90)=%x!!\n", status);
        }
    }

    if (gSpiNfInfo.MID == 0x98)
    {
        uint8_t status = 0;
		
		gSprMapTable = (uint8_t *)gXtxSprMapTable;
		
        // do Kioxia initial flow
        if (spiNf_getFeature(0xA0, &status) == 0)
        {
            (void)printf(" get feature(A0)=%x!!\n", status);
        }
        if ((status & 0x38u) == 0)
        {
            (void)spiNf_setFeature(0xA0, 0x00);
        }

        if (spiNf_getFeature(0xB0, &status) == 0)
        {
            (void)printf(" get feature(B0)=%x!!\n", status);
        }
        if ((status & 0x10u) == 0)
        {
            (void)spiNf_setFeature(0xB0, (status | 0x10u));
        }

        if (spiNf_getFeature(0xC0, &status) == 0)
        {
            (void)printf(" get feature(C0)=%x!!\n", status);
        }
		
        if (1)
        {
			uint8_t res = 0;
			uint8_t sReg = 0;
			uint8_t SetEccCnt = (uint8_t)0;		
			
			gSupportEccErrBit = 1;	//enable this function
			gEccErrThreshold = 8;
			
			SetEccCnt = (uint8_t)(gEccErrThreshold << 4);		
			
			res = spiNf_setFeature(0x10, SetEccCnt);
			if(spiNf_getFeature(0x10, &sReg) == 0)
			{
				(void)printf(" get feature(10)=%x, t=%x, r=%x !!\n",sReg, gEccErrThreshold, SetEccCnt);
			}
			else
			{
				(void)printf(" get feature(0x10) Error!!\n");
			}
        }
    }

    if ((gSpiNfInfo.MID == 0xEF) || (gPARAGON_MID_0xA1 != 0) || (gTEXIN_MID_0xA1 != 0) || (gSpiNfInfo.MID == 0x0B) || (gSpiNfInfo.MID == 0xE5))
    {
        gNeedReMapSpare = 1;
        (void)printf("[Spinfdrv.c] MID=%x, gNeedReMapSpare = %x\n", gSpiNfInfo.MID, gNeedReMapSpare);
		
		if(gSpiNfInfo.MID == 0xE5)
		{			
			gSprMapTable = (uint8_t *)gDsSprMapTable;
		}
    }

    if (gSpiNfInfo.MID == 0xC8)
    {
		gSprMapTable = (uint8_t *)gEsmt2SprMapTable;
		
        if (gSpiNfInfo.DID0 == (uint8_t)0x01)
        {
            (void)printf("Got ESMT type 2:MID=%02x, DID0=%02x\n", gSpiNfInfo.MID, gSpiNfInfo.DID0);
            gESMT_MID_0xC8  = 1;
            gNeedReMapSpare = 1;
        }

        if (gUseDummyReadId != 0)
        {
            if ((gSpiNfInfo.DID0 & 0xF0u) == (uint8_t)0x90)
            {
                gGD_ECC_8BITS = 1;
            }
            else
            {
                gNeedReMapSpare = 1;
            }
            (void)printf("[Spinfdrv.c]2 MID=%x, gNeedReMapSpare = %x\n", gSpiNfInfo.MID, gNeedReMapSpare);
        }

        if (gSpiNfInfo.DID0 == (uint8_t)0x52)
        {
            gSpiNfInfo.supportBlkLock = 0;
        }

        if ((gSpiNfInfo.DID0 & 0xF0u) == (uint8_t)0x50)
        {
            gSupportEccErrBit = 1;  // enable this function = 0;
        }

        if (gSpiNfInfo.DID0 == (uint8_t)0x55)
        {
            gSupportEccErrBit = 1;  // enable this function = 0;
            mmpAxiSpiSetDrivingMulti(NF_SPI_PORT, NAND_CSN, ITH_GPIO_DRIVING_2);
        }
		
		if ((gSpiNfInfo.DID0 & 0xF0u) == (uint8_t)0xD0)
		{
            gSupportEccErrBit = 1;  // enable this function = 0;
			gEccErrThreshold  = 8;
            if (gSpiNfInfo.DID0 == (uint8_t)0xD1)
            {
				gIsSupportOtp	= 1;	//only GD_5F1GQ4UE support it.
            }
		}
		
		if ((gSpiNfInfo.DID0 & 0xF0u) == (uint8_t)0x90)
		{
            gSupportEccErrBit = 1;  // enable this function = 0;
			gEccErrThreshold  = 8;
		}
    }

    if (gSpiNfInfo.MID == (uint8_t)0x01)
    {
        uint8_t ftrReg[4];
        uint8_t status = 0;

        // do SkyHigh initial flow
        if (spiNf_getFeature(0xA0, &status) == 0)
        {
            (void)printf(" get feature(A0)=%x!!\n", status);
        }

        // if blocks are blocked
        if ((status & 0x78u) != 0)
        {
            uint8_t spiret = 0;
            uint8_t SpiCmd[2];

            SpiCmd[0]   = 0x06;
            spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
            if (spiret != SPINF_OK)
            {
                spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
            }

            (void)spiNf_setFeature(0xA0, 0x02);
            (void)spiNf_setFeature(0xA0, 0x00);

            SpiCmd[0]   = 0x04;
            spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
            if (spiret != SPINF_OK)
            {
                spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
            }

            (void)spiNf_getFeature(0xA0, &status);
        }
        ftrReg[0] = status;

        if (spiNf_getFeature(0xB0, &status) == 0)
        {
            (void)printf(" get feature(B0)=%x!!\n", status);
        }
        if ((status & 0x10u) == 0)
        {
            (void)spiNf_setFeature(0xB0, (status | 0x10u));
        }
        ftrReg[1] = status;

        if (spiNf_getFeature(0xC0, &status) == 0)
        {
            (void)printf(" get feature(C0)=%x!!\n", status);
        }
        ftrReg[2] = status;
        (void)printf(" get feature(A0~C0): %02x, %02x, %02x!!\n", ftrReg[0], ftrReg[1], ftrReg[2]);
    }
	
	if(gSprMapTable == NULL)
	{
		(void)printf("Incorrect Spare Map Table: mpTb = %p\n",gSprMapTable);
		return 0x01;
	}

#ifdef USE_AXISPI_ENGINE
    #ifdef ENABLE_QUAD_OPERATION_MODE
    if(spiNf_setQuadIoOperation(1) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.1\n");
	}
    #else
    if(spiNf_setQuadIoOperation(0) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.2\n");
	}
    #endif
#endif

#ifdef CFG_NAND_RESERVED_SIZE
    if ((gSpiNfInfo.PageSize != 0) && (gSpiNfInfo.PageInBlk != 0))
    {
        uint32_t blkSize = gSpiNfInfo.PageSize * gSpiNfInfo.PageInBlk;
		#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
		gCfgRsvBlkSize = (uint32_t)(g_ftl_reserved_size / blkSize);
		#else
        gCfgRsvBlkSize = (uint32_t)(CFG_NAND_RESERVED_SIZE / blkSize);
		#endif
    }
#endif

	(void)memset(&gAxiInfoBuffer[0], 0, 192);	//for fixing MISRA 8.4

#ifdef ENABLE_WP_RESERVED_AREA
    // set default WP reserved area
    gSpiNfInfo.wpStartBlk   = 0;
    gSpiNfInfo.wpBlkCnt     = gCfgRsvBlkSize;
    gSpiNfInfo.wpMode       = 1;
    (void)printf("	### Trigger Error Base: %x, %x ###\n", gFakeErrorHappen, &gFakeErrorHappen);
    (void)printf("	### EnWpStop Base: %x, %x ###\n", gEnWpStopNfDriver, &gEnWpStopNfDriver);
    (void)printf("	### EnWpReturnFail Error Base: %x, %x ###\n", gEnWpReturnFail, &gEnWpReturnFail);
    #ifdef ENABLE_SAVE_NAND_ERROR_LOG
    (void)printf("	### EnWpStopifLogExist Base: %x, %x ###\n", gEnWpStopNfIfLogBlk, &gEnWpStopNfIfLogBlk);
    (void)printf("	### gDoSaveLog Base: %x, %x ###\n", gDoSaveLog, &gDoSaveLog);
    (void)printf("	### gLogStartBlock Base: %x, %x ###\n", gLogStartBlock, &gLogStartBlock);
    #endif
#endif

#ifdef ENABLE_SAVE_NAND_ERROR_LOG
    if ((gSpiNfInfo.MID != (uint8_t)0xC8) || (gSpiNfInfo.DID0 != (uint8_t)0x52))
    {
        // force to disable save-log if not GD_5F2GQ5UE
        if (gDoSaveLog)
        {
            gDoSaveLog = 0;
        }
    }

    // search block 5~7 for log block
    // if got any log block, STOP NAND driver.
    {
        uint8_t     hdBuf[16]   = {0};
        uint8_t     pgBuf[4096 + 512];
        uint8_t     isLogBlk    = 1;
        uint8_t     res;
        uint32_t    b           = gLogStartBlock;
        uint32_t    i;

        sprintf(hdBuf, "FINE_ITE_LOG");

        while (b < (gLogStartBlock + 4))
        {
            res = spiNf_PageRead(b, 0, pgBuf);

            for (i = 0; i < 12; i++)
            {
                if (pgBuf[i] != hdBuf[i])
                {
                    isLogBlk = 0;
                }
            }
            if (isLogBlk)
            {
                (void)printf("There is a LOG block @ b=%x, %x\n", b, gEnWpStopNfIfLogBlk);
                if (gEnWpStopNfIfLogBlk)
                {
                    (void)printf("There is a LOG block @ b=%x, then STOP it(%x)!!\n", b, gEnWpStopNfIfLogBlk);
                    while (1)
                    {
                    }
                }
            }
            b--;
        }
    }
#endif

    (void)init_power_detect();

    gSpiNfInitFinished = 1;

    return 0x00;
}

// =============================================================================

/**
* Get feature of SPI NAND
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_getFeature (
    uint8_t addr,
    uint8_t * buf)
{
    unsigned char   SpiCmd[2];
    unsigned char   SpiData[4];
    int             spiret;

#ifdef FAST_NAND_READ
    int readTimes = 0;
    // Protection Register
    if (addr == 0xA0)
    {
        if (regA0 != -1)
        {
            *buf = (uint8_t)regA0;
            if (regA0 != 0)
            {
                (void)printf("	get_ftr_A0: %x, %x\n", *buf, regA0);
            }
            return 0;
        }
    }

    // Configuration Register
    if (addr == 0xB0)
    {
        if (regB0 != -1)
        {
            // Suppose not happend of this condition.
            if ((((uint8_t)regB0 & 0x10u) == 0) && (gForceEccEnable != 0))
            {
                (void)printf("[Warning] ECC-en bit is NOT enabled[1]: B0=%x, frcEcc=%x\n", regB0, gForceEccEnable);
                regB0 = -1;
            }
			else
			{
				*buf = (uint8_t)regB0;
				return 0;
			}
        }
    }
	
#if 1
    if (buf == NULL)
    {
		(void)printf("bufffer is NULL!!\n");
        return (4);
    }
	
	while(true)
	{
		SpiCmd[0] = 0x0F;
		SpiCmd[1]   = addr;
		spiret      = spi_read(SpiCmd, 2, &SpiData[0], 1);
		if (spiret == SPINF_OK)
		{
			*buf = SpiData[0];
			if (addr == 0xA0)
			{
				regA0 = *buf;
			}
			if (addr == 0xB0)
			{
				regB0 = (int)*buf;
				if ((((uint8_t)regB0 & 0x10u) == 0) && (gForceEccEnable != 0))
				{
					uint8_t regB0_o = regB0;
					(void)n_SetEcc(ECC_ENABLE);
					(void)printf("[Warning] ECC-en bit is NOT enabled[2]: B0=(%x, %x) fEcc=%x\n", regB0, regB0_o, gForceEccEnable);
					(void)usleep(5000);
					if (readTimes++ >= 10)
					{
						uint32_t    * ptr       = (uint32_t *)0xFFFFFFFFUL;
						int         dummyloop   = 0;
						(void)ithPrintf("[ERROR]The ECC bit cannot be turn on. Force system to reboot....\n");
						for (dummyloop = 0; dummyloop < 4096; dummyloop++)
						{
							asm ("");
						}
						*ptr = 0x1;
					}
					continue;
				}
			}
			break;
		}
		else
		{
			(void)printf("SPI NAND GET feature FAIL.01, addr: %x, result=%x\n", addr, spiret);
			while (n_cmdReset(0) != 0)
			{
				(void)printf("[SPINF ERR] reset commnad fail:xx1, reset cnt = %d\n", readTimes);
				if (readTimes++ >= 10)
				{
					uint32_t    * ptr       = (uint32_t *)0xFFFFFFFFUL;
					int         dummyloop   = 0;
					(void)ithPrintf("[ERROR]The nand reset cmd can not work!. Force system to reboot....\n");
					for (dummyloop = 0; dummyloop < 4096; dummyloop++)
					{
						asm ("");
					}
					*ptr = 0x1;
				}
			}
			(void)usleep(1000 * 1000);
		}
	}
	return 0;
#else
getfeature_cmd:
    SpiCmd[0] = 0x0F;

    if (!buf)
    {
        return (4);
    }

    SpiCmd[1]   = addr;
    spiret      = spi_read(SpiCmd, 2, &SpiData[0], 1);

    if (spiret == SPINF_OK)
    {
        *buf = SpiData[0];
        if (addr == 0xA0)
        {
            regA0 = *buf;
        }
        if (addr == 0xB0)
        {
            regB0 = (int)*buf;

            if (!(regB0 & 0x10) && gForceEccEnable)
            {
                uint8_t regB0_o = regB0;
                (void)n_SetEcc(ECC_ENABLE);
                (void)printf("[Warning] ECC-en bit is NOT enabled[2]: B0=(%x, %x) fEcc=%x\n", regB0, regB0_o, gForceEccEnable);
                if (readTimes++ >= 10)
                {
                    uint32_t    * ptr       = (uint32_t *)0xFFFFFFFFUL;
                    int         dummyloop   = 0;
                    (void)ithPrintf("[ERROR]The ECC bit cannot be turn on. Force system to reboot....\n");
                    for (dummyloop = 0; dummyloop < 4096; dummyloop++)
                    {
                        asm ("");
                    }
                    *ptr = 0x1;
                }
                goto getfeature_cmd;
            }
        }
        // (void)printf("SPI NF GET feature OK, data[%02X]=%02x\n",addr,SpiData[0]);
        return (0);
    }
    else
    {
        (void)printf("SPI NAND GET feature FAIL.01, addr: %x, result=%x\n", addr, spiret);
        // reset 0xFF
        while (n_cmdReset(0))
        {
            (void)printf("[SPINF ERR] reset commnad fail:xx1, reset cnt = %d\n", readTimes);
            if (readTimes++ >= 10)
            {
                uint32_t    * ptr       = (uint32_t *)0xFFFFFFFFUL;
                int         dummyloop   = 0;
                (void)ithPrintf("[ERROR]The nand reset cmd can not work!. Force system to reboot....\n");
                for (dummyloop = 0; dummyloop < 4096; dummyloop++)
                {
                    asm ("");
                }
                *ptr = 0x1;
            }
        }
        (void)usleep(1000 * 1000);
        // goto getfeature_cmd;
        return (5);
    }
	#endif  //test code
#else
    SpiCmd[0] = 0x0F;

    if (!buf)
    {
        return (4);
    }

    SpiCmd[1]   = addr;
    spiret      = spi_read(SpiCmd, 2, &SpiData[0], 1);

    if (spiret == SPINF_OK)
    {
        *buf = SpiData[0];
        // (void)printf("SPI NF GET feature OK, data[%02X]=%02x\n",addr,SpiData[0]);
        return (0);
    }
    else
    {
        (void)printf("SPI NAND GET feature FAIL.02, addr=%x, result=%x\n", addr, spiret);
        // reset 0xFF
        if (n_cmdReset(0))
        {
            uint32_t    * ptr       = (uint32_t *)0xFFFFFFFF;
            int         dummyloop   = 0;
            (void)ithPrintf("[ERROR] The nand reset cmd can not work!! Force system to reboot....\n");
            for (dummyloop = 0; dummyloop < 4096; dummyloop++)
            {
                asm ("");
            }
            *ptr = 0x1;
        }
        (void)usleep(1000 * 1000);
        return (5);
    }
#endif
}

// =============================================================================

/**
* Set feature of SPI NAND
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_setFeature (
    uint8_t addr,
    uint8_t Reg)
{
    unsigned char   SpiCmd[2];
    unsigned char   SpiData[2];
    int             spiret;
    uint8_t         mask[4] = {0xBE, 0xF1, 0x00, 0x60}; // reg[C0] is read only (status)

    SpiCmd[0]   = 0x1F;
    SpiCmd[1]   = addr;

    if (gSpiNfInfo.MID == 0xC8)
    {
        SpiData[0] = (uint8_t)Reg & (uint8_t)mask[(uint8_t)(addr - 0xA0) >> 4];
        if (SpiData[0] != Reg)
        {
            (void)printf("warning: THE VALUE=%x,set=%x, index=%x\n", Reg, SpiData[0], (uint8_t)(addr - 0xA0) >> 4);
        }
    }
    else
    {
        SpiData[0] = Reg;
    }

    spiret = spi_write(SpiCmd, 2, &SpiData[0], 1);

    if (spiret == SPINF_OK)
    {
        // (void)printf("Set Feature OK!!\n");
#ifdef FAST_NAND_READ
        if (addr == 0xA0)
        {
            regA0 = -1;
        }
        if (addr == 0xB0)
        {
            regB0 = -1;
        }
#endif
        return (0);
    }
    else
    {
        (void)printf("SPI NAND SET feature FAIL,result=%x, a=%x, r=%x\n", spiret, addr, Reg);
        return (1);
    }
}

uint8_t
spiNf_ByteRead (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     * dBuf,
    uint32_t    offset,
    uint32_t    rLen)
{
    unsigned char   SpiCmd[4];
    unsigned char   SpiData[3];
    int             spiret;
    uint8_t         status      = 0x01;
    uint32_t        addr;
    uint8_t         result      = 1;
    uint8_t         reMapSpr    = 0;
    uint32_t        oriOffset   = offset;
    uint32_t        oriRdLen    = rLen;
    uint32_t        tsbRdOffset = 0;
    unsigned char   tmpData[256 + 32];
    uint32_t        new_blk     = 0;

    if (gSupportEccErrBit != 0)
    {
        (void)memset(&gEccInf, 0, sizeof(gEccInf));
    }

#if CFG_NAND_READ_CACHE_SIZE > 0
    if (NAND_cache_isByteReadCacheHit(blk, ppo, dBuf, offset, rLen) == true)
    {
        // (void)printf("NAND_ByteReadCache Hit!\n");
        return 0;
    }
#endif
#ifndef FAST_NAND_READ
    if (spiNf_waitReady(&status, 2000))
    {
        goto errBREnd;
    }
#endif

    if (offset >= gSpiNfInfo.PageSize)
    {
        // modified for workaround the axispi 16-bytes alignment issue
        // force to eanble "reMapSpr=1" to read whole spare data(64B/128B/256B) to avoid from this issue
        reMapSpr    = 1;
        oriOffset   = offset;
        offset      = gSpiNfInfo.PageSize;
    }
    else
    {
        if ((rLen > 32) && ((rLen & 0x1Fu)!= 0))
        {
            (void)printf(" Error: size is not alignment, (%x, %x) s=%x \n", blk, ppo, rLen);
            goto errBREnd;
        }
    }

    new_blk = n_getNewBlkIndex(blk);
    addr    = (new_blk * gSpiNfInfo.PageInBlk) + ppo;

#ifdef ENABLE_QUAD_OPERATION_MODE
    if ((gSpiNfInfo.MID == (uint8_t)0xC2) && (gSpiNfInfo.DID0 == (uint8_t)0x12))
    {
        if(spiNf_setQuadIoOperation(1) != 0)
		{
			(void)printf("spiNf_setQuadIoOperation error.3\n");
		}
    }
    else
    {
        if(spiNf_setQuadIoOperation(0) != 0)
		{
			(void)printf("spiNf_setQuadIoOperation error.4\n");
		}
    }
#endif

    // to workaround the toshiba random read page issue
    if ((gSpiNfInfo.MID == 0x98) && (offset >= gSpiNfInfo.PageSize))
    {
        // 1.read the last 4-bytes of last sector(sector 7)
        // 2.and then read the 24-bytes of 0th spare data
        // 3.please reference to datasheet "Definition of 528 bytes Data Pair"
        if (gSpiNfInfo.DID0 == (uint8_t)0xCD)
        {
            tsbRdOffset = 4;    // only Kioxia's NAND need shift the partial read offset
            offset      = gSpiNfInfo.PageSize - tsbRdOffset;
        }
    }

    // if(gSpiNfInfo.MID==0x98) _enHighSpeedMode(0);

    SpiCmd[0]   = 0x13;
    SpiCmd[1]   = (addr >> 16) & 0xFFu;
    SpiCmd[2]   = (addr >> 8) & 0xFFu;
    SpiCmd[3]   = addr & 0xFFu;
    spiret      = spi_write(SpiCmd, 4, &SpiData[0], 0);
    if (spiret != SPINF_OK)
    {
        (void)printf("send cmd 0x13 FAIL4,R1=(%x,%x) addr=%x\n", spiret, SPINF_OK, addr);
        goto errBREnd;
    }

    /* adding this ilde time is for WINBOND issue */
#ifdef FAST_NAND_READ
    if (gSpiNfInfo.MID == 0xEF)
    {
        (void)usleep(50);
    }
#else
    if (gSpiNfInfo.MID == 0xEF)
    {
        (void)usleep(50);
    }
#endif

    if (spiNf_waitReady(&status, 2000) != 0)
    {
        goto errBREnd;
    }

    SpiCmd[0] = 0x03;

    if (gHasDummyByteAddr != 0)
    {
        if (gSpiNfInfo.PageSize == 4096)
        {
            SpiCmd[1] = (unsigned char)((offset >> 8) & 0x1Fu);
        }
        else
        {
            SpiCmd[1] = (unsigned char)((offset >> 8) & 0x0Fu);
        }

        SpiCmd[2]   = (unsigned char)(offset & 0xFFu);
        SpiCmd[3]   = 0x00;
    }
    else
    {
        SpiCmd[1]   = 0x00;
        SpiCmd[2]   = (unsigned char)((offset >> 8) & 0xFFu);
        SpiCmd[3]   = (unsigned char)(offset & 0xFFu); // bit0 must be "0"
    }

    if ((g2PlaneReadCmd != 0) && (g2PlaneNandShift != 0) && ((blk & 0x1u)!=0))
    {
        SpiCmd[1] |= (1u << g2PlaneNandShift);
    }

    if (reMapSpr != 0)
    {
        spiret = spi_read(SpiCmd, 4, tmpData, gSpiNfInfo.SprSize);
    }
    else
    {
        if ((gSpiNfInfo.MID == 0x98) && (tsbRdOffset != 0))
        {
            spiret = spi_read(SpiCmd, 4, tmpData, tsbRdOffset + gSpiNfInfo.FtlSprSize);
        }
        else
        {
            spiret = spi_read(SpiCmd, 4, dBuf, rLen);
        }
    }

#ifndef FAST_NAND_READ
    #ifdef ENABLE_QUAD_OPERATION_MODE
    if(spiNf_setQuadIoOperation(0) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.5\n");
	}
    #endif
#endif

    if (spiret != SPINF_OK)
    {
        (void)printf("send cmd 0x13 FAIL,R2=(%x,%x)\n", spiret, SPINF_OK);
        goto errBREnd;
    }

    // if(gSpiNfInfo.MID==0x98) _enHighSpeedMode(1);

#ifdef USE_MMP_DRIVER // remove this for win32
    #ifdef FAST_NAND_READ
    if ((reMapSpr == 0))
    {
        if ((gSpiNfInfo.MID == 0x98) && (tsbRdOffset != 0))
        {
            (void)memcpy(&dBuf[0], &tmpData[tsbRdOffset + (oriOffset - gSpiNfInfo.PageSize)], oriRdLen);
        }
    }
    #else
    if (reMapSpr)
    {
        ithInvalidateDCacheRange((uint32_t *)tmpData, gSpiNfInfo.SprSize);
    }
    else
    {
        if ((gSpiNfInfo.MID == 0x98) && (tsbRdOffset != 0))
        {
            ithInvalidateDCacheRange((uint32_t *)tmpData, tsbRdOffset + gSpiNfInfo.FtlSprSize);
            (void)memcpy(&dBuf[0], &tmpData[tsbRdOffset + (oriOffset - gSpiNfInfo.PageSize)], oriRdLen);
        }
        else
        {
            ithInvalidateDCacheRange((uint32_t *)&dBuf[0], rLen);
        }
    }
    #endif
#else
    if ((gSpiNfInfo.MID == 0x98) && (tsbRdOffset != 0))
    {
        (void)memcpy(&dBuf[0], &tmpData[tsbRdOffset + (oriOffset - gSpiNfInfo.PageSize)], oriRdLen);
    }
#endif

    if (reMapSpr != 0)
    {
        uint32_t    j;
        uint8_t     tmpBuf[24];
        uint8_t     * ptr;

        offset = oriOffset;

        // re-arrange the spare data
        if (rLen == 24)
        {
            ptr = (uint8_t *)dBuf;
        }
        else
        {
            ptr = (uint8_t *)&tmpBuf[0];
        }

        // read half data
        (void)memset(tmpBuf, 0xFF, 24);

        n_reMapSpare(ptr, tmpData, SPINF_REMAP_SPARE_FOR_READ);

        if (rLen != 24)
        {
            for (j = 0; j < rLen; j++)
            {
                dBuf[j] = ptr[offset - gSpiNfInfo.PageSize + j];
            }
        }
    }

#ifdef FAST_NAND_READ
    result  = n_checkEccStatus(blk, ppo, status);
#else
    result  = n_checkEccStatus(blk, ppo);
#endif

errBREnd:

    return result;
}

// =============================================================================

/**
* SPI NAND read page data
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_PageRead (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     * dBuf)                                             // (uint32_t addr, uint8_t *dBuf)
{
    unsigned char   SpiCmd[8];
    unsigned char   SpiData[3];
    int             spiret;
    uint8_t         status  = 0x01;
    uint32_t        addr    = 0;
    uint8_t         result  = 1;
    uint32_t        new_blk = 0;

    if (gSupportEccErrBit != 0)
    {
        (void)memset(&gEccInf, 0, sizeof(gEccInf));
    }

#if CFG_NAND_READ_CACHE_SIZE > 0
    if (NAND_cache_isPageReadCacheHit(blk, ppo, dBuf) == true)
    {
        // (void)printf("NAND_PageReadCache Hit!\n");
        return 0;
    }
#endif
#ifndef FAST_NAND_READ
    if (spiNf_waitReady(&status, 3000))
    {
        goto errRdEnd;
    }
#endif

    new_blk = n_getNewBlkIndex(blk);
    addr    = (new_blk * gSpiNfInfo.PageInBlk) + ppo;

#ifdef ENABLE_QUAD_OPERATION_MODE
    if(spiNf_setQuadIoOperation(1) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.6\n");
	}
#endif

    SpiCmd[0]   = 0x13;
    SpiCmd[1]   = (addr >> 16) & 0xFFu;
    SpiCmd[2]   = (addr >> 8) & 0xFFu;
    SpiCmd[3]   = addr & 0xFFu;
    spiret      = spi_write(SpiCmd, 4, &SpiData[0], 0);

    if (spiret != SPINF_OK)
    {
        (void)printf("send cmd 0x13 FAIL,R1=(%x,%x)\n", spiret, SPINF_OK);
        goto errRdEnd;
    }

    /* adding this ilde time is for WINBOND issue */
#ifdef FAST_NAND_READ
    if (gSpiNfInfo.MID == 0xEF)
    {
        (void)usleep(50);
    }
#else
    if (gSpiNfInfo.MID == 0xEF)
    {
        (void)usleep(50);
    }
#endif

    if (spiNf_waitReady(&status, 3000) != 0)
    {
        goto errRdEnd;
    }

#ifdef ENABLE_QUAD_OPERATION_MODE
    SpiCmd[0]   = 0x6B;
#else
    SpiCmd[0]   = 0x03;
#endif

    SpiCmd[1]   = 0x00;
    SpiCmd[2]   = 0x00;
    SpiCmd[3]   = 0x00;

    if ((g2PlaneReadCmd != 0) && (g2PlaneNandShift != 0) && ((blk & 0x1u) != 0))
    {
        SpiCmd[1] |= (1u << g2PlaneNandShift);
    }

    /* modified this command flow for axispi dma size alignment */
    spiret = spi_read(SpiCmd, 4, &dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.SprSize);
#ifdef USE_MMP_DRIVER
    #ifndef FAST_NAND_READ
    ithInvalidateDCacheRange((uint32_t *)&dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.SprSize);
    #endif
#endif

    if ((gNeedRawSpare == 0) && (gNeedReMapSpare != 0))
    {
        // read half data
        uint8_t tmpBuf[24];
        (void)memset(tmpBuf, 0xFF, 24);

        n_reMapSpare(&tmpBuf[0], &dBuf[gSpiNfInfo.PageSize], SPINF_REMAP_SPARE_FOR_READ);

        (void)memcpy(&dBuf[gSpiNfInfo.PageSize], &tmpBuf[0], 24);
    }

#ifndef FAST_NAND_READ
    #ifdef USE_AXISPI_ENGINE
    if(spiNf_setQuadIoOperation(0) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.7\n");
	}
    #endif
#endif

    if (spiret != SPINF_OK)
    {
        (void)printf("send cmd 0x13 FAIL,R2=(%x,%x)\n", spiret, SPINF_OK);
        goto errRdEnd;
    }

#ifdef FAST_NAND_READ
    result  = n_checkEccStatus(blk, ppo, status);
#else
    result  = n_checkEccStatus(blk, ppo);
#endif

    if (result != 0)
    {
        static uint8_t * p1 = NULL;

        if (p1 == NULL)
        {
            p1 = (uint8_t *)malloc(gSpiNfInfo.PageSize + 512);
        }

        if (p1 != NULL)
        {
            (void)memcpy(p1, &dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.SprSize);
            ithFlushDCacheRange((void *)p1, gSpiNfInfo.PageSize + 512);
            ithFlushMemBuffer();
        }
        (void)printf("chkEccErr1:(%x)(b=%x, p=%x), dataBase: %p\n", result, blk, ppo, p1);
    }

errRdEnd:

    if (result != 0)
    {
        (void)printf("chkEccErr2:(%x)(b=%x, p=%x)\n", result, blk, ppo);
    }
#if CFG_NAND_READ_CACHE_SIZE > 0
    if (result == 0)
    {
        NAND_cache_addNewEntry(blk, ppo, dBuf);
    }
#endif
    return result;
}

// =============================================================================

/**
* SPI NAND read page data
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_PageReadRaw (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     * dBuf)                                                 // (uint32_t addr, uint8_t *dBuf)
{
    unsigned char   SpiCmd[8];
    unsigned char   SpiData[3];
    int             spiret;
    uint8_t         status  = 0x01;
    uint32_t        addr    = 0;
    uint8_t         result  = 1;
    uint32_t        new_blk = 0;

    if (gSupportEccErrBit != 0)
    {
        (void)memset(&gEccInf, 0, sizeof(gEccInf));
    }

#if CFG_NAND_READ_CACHE_SIZE > 0
    if (NAND_cache_isPageReadCacheHit(blk, ppo, dBuf) == true)
    {
        // (void)printf("NAND_PageReadCache Hit!\n");
        return 0;
    }
#endif

    gNeedRawSpare = 1;

#ifndef FAST_NAND_READ
    if (spiNf_waitReady(&status, 3000))
    {
        goto errRdEnd;
    }
#endif

    new_blk = n_getNewBlkIndex(blk);
    addr    = (new_blk * gSpiNfInfo.PageInBlk) + ppo;

#ifdef ENABLE_QUAD_OPERATION_MODE
    if(spiNf_setQuadIoOperation(1) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.8\n");
	}
#endif

    SpiCmd[0]   = 0x13;
    SpiCmd[1]   = (addr >> 16) & 0xFFu;
    SpiCmd[2]   = (addr >> 8) & 0xFFu;
    SpiCmd[3]   = addr & 0xFFu;
    spiret      = spi_write(SpiCmd, 4, &SpiData[0], 0);

    if (spiret != SPINF_OK)
    {
        (void)printf("send cmd 0x13 FAIL,R1=(%x,%x)\n", spiret, SPINF_OK);
        goto errRdEnd;
    }

    /* adding this ilde time is for WINBOND issue */
#ifdef FAST_NAND_READ
    if (gSpiNfInfo.MID == 0xEF)
    {
        (void)usleep(50);
    }
#else
    if (gSpiNfInfo.MID == 0xEF)
    {
        (void)usleep(50);
    }
#endif

    if (spiNf_waitReady(&status, 3000) != 0)
    {
        goto errRdEnd;
    }

#ifdef ENABLE_QUAD_OPERATION_MODE
    SpiCmd[0]   = 0x6B;
#else
    SpiCmd[0]   = 0x03;
#endif

    SpiCmd[1]   = 0x00;
    SpiCmd[2]   = 0x00;
    SpiCmd[3]   = 0x00;

    /* modified this command flow for axispi dma size alignment */
    spiret      = spi_read(SpiCmd, 4, &dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.SprSize);
#ifdef USE_MMP_DRIVER
    #ifndef FAST_NAND_READ
    ithInvalidateDCacheRange((uint32_t *)&dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.SprSize);
    #endif
#endif

    if ((gNeedRawSpare == 0) && (gNeedReMapSpare != 0))
    {
        // read half data
        uint8_t tmpBuf[24];
        (void)memset(tmpBuf, 0xFF, 24);

        n_reMapSpare(&tmpBuf[0], &dBuf[gSpiNfInfo.PageSize], SPINF_REMAP_SPARE_FOR_READ);

        (void)memcpy(&dBuf[gSpiNfInfo.PageSize], &tmpBuf[0], 24);
    }

#ifndef FAST_NAND_READ
    #ifdef USE_AXISPI_ENGINE
    if(spiNf_setQuadIoOperation(0) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.9\n");
	}
    #endif
#endif

    if (spiret != SPINF_OK)
    {
        (void)printf("send cmd 0x13 FAIL,R2=(%x,%x)\n", spiret, SPINF_OK);
        goto errRdEnd;
    }

#ifdef FAST_NAND_READ
    result  = n_checkEccStatus(blk, ppo, status);
#else
    result  = n_checkEccStatus(blk, ppo);
#endif

    if (result != 0)
    {
        (void)printf("chkEccErr1:(%x)(b=%x, p=%x)\n", result, blk, ppo);
    }

errRdEnd:

    if (result != 0)
    {
        (void)printf("chkEccErr2:(%x)(b=%x, p=%x)\n", result, blk, ppo);
    }
#if CFG_NAND_READ_CACHE_SIZE > 0
    if (result == 0)
    {
        NAND_cache_addNewEntry(blk, ppo, dBuf);
    }
#endif
    gNeedRawSpare = 0;

    return result;
}

// =============================================================================

/**
* SPI NAND program page data
*
* @return 0 If successful. Otherwise, return a nonzero value.
*/

// =============================================================================
uint8_t
spiNf_PageProgram (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     * dBuf)
{
    unsigned char   SpiCmd[8]   = {0};
    int             spiret;
    uint8_t         status      = 0x01;
    uint32_t        addr        = 0;
    uint8_t         wpStatus    = 0x0;
    uint32_t        new_blk     = 0;
	
	if(gAxiInfoBuffer[0] != 0)
	{
		//for fixing MISRA 8.4
		(void)printf("Got AxiinfoBuf data:: %x\n", gAxiInfoBuffer[0]);
	}

#ifdef ENABLE_WP_RESERVED_AREA
    uint32_t ori_EnWpRtrnFail   = gEnWpReturnFail;

    gGlobalBlkIdx = blk;

    if (chkWp(blk) || !gSpiNfInfo.PageInBlk)
    {
        if (!gFakeErrorHappen || (gFakeErrorHappen == 0x55555555))
        {
            uint8_t     b[96];
            uint32_t    k;

            (void)printf("spiNf_PageProgram: S/W WP fail: %x, %x, %x\n", blk, ppo, dBuf, gSpiNfInfo.PageInBlk);

    #ifdef ENABLE_SAVE_NAND_ERROR_LOG
            sprintf(b, "API-pgm:b=%03x,p=%03x,b=%x", blk, ppo, dBuf);

            _save_log(b, 32);
            _save_log(dBuf, 2048);

            sprintf(b, "Spare-Data: b=%03x, p=%03x, b=%x", blk, ppo, dBuf);
            (void)memcpy(&b[32], &dBuf[2048], 64);
            for (k = 0; k < 64; k++)
            {
                (void)printf("%02x ", dBuf[2048 + k]);
                if ((k & 0xF) == 0xF)
                {
                    (void)printf("\n");
                }
            }
            (void)printf("\n");
            _save_log(&b[0], 96);
    #endif

            if (gEnWpStopNfDriver)
            {
                // itpPrintBacktrace();
                while (1)
                {
                }
            }
            if (gEnWpReturnFail)
            {
                // reset command
                n_cmdReset(0);
                (void)usleep(10000);
                return (16);
            }
        }
    }
    else
    {
        if ((blk == 0) && (ppo == 0))
        {
            if (gEnWpReturnFail)
            {
                gEnWpReturnFail = 0;
            }
        }
        if (gDualDieNand && (blk == 0x400) && (ppo == 0))
        {
            if (gEnWpReturnFail)
            {
                gEnWpReturnFail = 0;
            }
        }
    }
#endif

    if (spiNf_getFeature(0xC0, &wpStatus) != 0)
    {
        (void)printf(" get feature.4 [0xC0] = %x!!\n", wpStatus);
    }
    if (((uint8_t)wpStatus & (uint8_t)NF_STATUS_OIP) != 0)
    {
		if (spiNf_waitReady(&wpStatus, 3000) != 0)
		{
			(void)printf(" wait Ready timeout, s = %x!!\n", wpStatus);
		}
    }

    // die select & get local block index
    new_blk = n_getNewBlkIndex(blk);
    addr    = (uint32_t)(new_blk * gSpiNfInfo.PageInBlk) + ppo;

    n_chkRegA0B4WtErs();

    // 06H (write enable)
    if (((uint8_t)wpStatus & (uint8_t)NF_STATUS_WEL) != 0)
    {
        (void)printf("WEL=1, C0=%x, A0=%x\n", wpStatus, status);
    }
    else
    {
        SpiCmd[0]   = 0x06;
        spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
        if (spiret != SPINF_OK)
        {
            spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
        }
    }

#ifdef USE_AXISPI_ENGINE
    #ifdef ENABLE_QUAD_OPERATION_MODE
    if(spiNf_setQuadIoOperation(1) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.10\n");
	}
    #else
    if(spiNf_setQuadIoOperation(0))
	{
		(void)printf("spiNf_setQuadIoOperation error.11\n");
	}
    #endif
#endif

#ifdef USE_MMP_DRIVER
    ithFlushDCacheRange((void *)&dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.FtlSprSize);
    ithFlushMemBuffer();
#endif

    // 02H(program load)
#ifdef ENABLE_QUAD_OPERATION_MODE
    if (g_TOSHIBA_QuadMode != 0)
    {
        SpiCmd[0] = 0x02;                        // Toshiba's 1st gen SPI NAND does NOT support QUAD-IO program
    }
    else
    {
        SpiCmd[0] = 0x32;
    }
#else
    SpiCmd[0]   = 0x02;
#endif

    SpiCmd[1]   = 0x00;

    if (g2PlaneNandShift != 0)
    {
        if ((g2PlaneNandShift != 4) && (g2PlaneNandShift != 5))
        {
            (void)printf("SPI NAND Error: incorrect g2PlaneNandShift value, %x\n", g2PlaneNandShift);
        }
        else
        {
            SpiCmd[2] = ((blk & 0x01u) << g2PlaneNandShift); // if plane mode (0x00 | (blk&0x01)<<g2PlaneNandShift);
        }
    }
    else
    {
        SpiCmd[2] = 0x00;
    }

    SpiCmd[3] = 0x00;

    if (gNeedReMapSpare != 0)
    {
        uint8_t tmpBuf[24];
        (void)memcpy(&tmpBuf[0], &dBuf[gSpiNfInfo.PageSize + 0], 24);
        (void)memset(&dBuf[gSpiNfInfo.PageSize], 0xFF, gSpiNfInfo.SprSize);

        n_reMapSpare(&dBuf[gSpiNfInfo.PageSize], &tmpBuf[0], SPINF_REMAP_SPARE_FOR_WRITE);

#ifdef USE_MMP_DRIVER   // remove this function if WIN32
        ithFlushDCacheRange((void *)&dBuf[gSpiNfInfo.PageSize], gSpiNfInfo.SprSize);
        ithFlushMemBuffer();
#endif
    }

    doPowerDownProtectNand(1);

    spiret = spi_write(SpiCmd, 3, &dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.SprSize);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
    }
	
#ifdef ENABLE_WP_RESERVED_AREA
    if (gFakeErrorHappen == 0x55AA55AA)
    {
        // to trigger the address = 0 issue in AXISPI driver
        (void)printf("	$#$ Set SPI Address = 0 $#$\n");
        addr = 0;
    }
#endif

    // 10H (program execute)
    SpiCmd[0]   = 0x10;
    SpiCmd[1]   = (addr >> 16) & 0xFFu;
    SpiCmd[2]   = (addr >> 8) & 0xFFu;
    SpiCmd[3]   = addr & 0xFFu;
    spiret      = spi_write(SpiCmd, 4, &SpiCmd[0], 0);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
    }
	
#ifdef ENABLE_WP_RESERVED_AREA
    if (gAxiPioGotAddr0 != 0)
    {
        // gAxiPioGotAddr0=3 means addr pointer == NULL
        if ((blk != 0) || (ppo != 0) || (gAxiPioGotAddr0 == 3))
        {
            uint8_t b[96];

            (void)printf("After send 0x10 cmd: b=%03x, p=%03x, b=%x, addr=%x\n", blk, ppo, dBuf, gAxiPioGotAddr0);

    #ifdef ENABLE_SAVE_NAND_ERROR_LOG
            _save_log(gAxiInfoBuffer, 192);
            _save_log(&dBuf[0], 2048);

            sprintf(b, "Spare: b=%03x, p=%02x, b=%x", blk, ppo, dBuf);
            (void)memcpy(&b[32], &dBuf[2048], 64);
            _save_log(&b[0], 96);
    #endif
        }
        (void)printf("gAxiPioGotAddr0 = %x, gFakeErrorHappen = %x!!\n", gAxiPioGotAddr0, gFakeErrorHappen);
        if (gAxiPioGotAddr0 != 0)
        {
            gAxiPioGotAddr0 = 0;
            (void)printf(" Set gAxiPioGotAddr0 as 0 (a = %x)!!\n", gAxiPioGotAddr0);
        }
        if (gFakeErrorHappen == 0x55AA55AA)
        {
            gFakeErrorHappen = 0;
        }
        if (chkWp(blk) == 0))
        {
            if ((blk == 0) && (ppo == 0) && (ori_EnWpRtrnFail != 0))
            {
                gEnWpReturnFail = 1;
            }

            if ((gDualDieNand != 0) && (blk == 0x400) && (ppo == 0))
            {
                if ((ori_EnWpRtrnFail != 0))
                {
                    gEnWpReturnFail = 1;
                }
            }
        }
    #ifdef ENABLE_SAVE_NAND_ERROR_LOG
        (void)printf("End of gAxiPioGotAddr0 = %x, gFakeErrorHappen = %x, gDoSaveLog = %x!!\n", gAxiPioGotAddr0, gFakeErrorHappen, gDoSaveLog);
    #else
        (void)printf("End of gAxiPioGotAddr0 = %x, gFakeErrorHappen = %x!\n", gAxiPioGotAddr0, gFakeErrorHappen);
    #endif
    }
#endif
	if ((gDualDieNand != 0) && (blk == 0x800) && (ppo == 0))
	{
		// additional useless judgment for MISRA C-2012 Rule 8.9
		(void)printf("gDualDieNand = %x\n",gDualDieNand);
	}

    // 0FH (get feature) (wait busy)
    if(spiNf_waitReady(&status, 3000) != 0)
	{
		(void)printf("spiNf_waitReady fail\n");
	}

    if (spiNf_getFeature(0xC0, &wpStatus) != 0)
    {
        (void)printf(" get feature.5 [0xC0] = %x!!\n", wpStatus);
    }

    if ((wpStatus & 0x0Cu) != 0)
    {
        uint8_t s = 0x00;
        (void)printf(" P/E fail.2 [0xC0] = %x, b=%x!!\n", wpStatus, blk);
        if(spiNf_getFeature(0xA0, &s) != 0)
		{
			(void)printf(" get feature.6 [0xA0] = %x!!\n", s);
		}
        (void)printf(" get feature [0xA0] = %x!!\n", s);
    }

#ifdef USE_AXISPI_ENGINE
    #ifndef FAST_NAND_READ
    if(spiNf_setQuadIoOperation(0) != 0)
	{
		(void)printf("spiNf_setQuadIoOperation error.12\n");
	}
    #endif
#endif

    doPowerDownProtectNand(2);

    // 04H (write Disable)
    SpiCmd[0]   = 0x04;
    spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
    }
	
    if (((uint8_t)wpStatus & (uint8_t)NF_STATUS_P_FAIL) != 0)
    {
        if (n_cmdReset(0) != 0)
        {
            uint8_t * p = (uint8_t *)0xFFFFFFFFUL;
            (void)printf("[SPINF ERR] reset commnad fail:1, s=%x\n", status);
            (void)usleep(100 * 1000);
            *p = 1;
        }

        (void)printf("SPI NAND PROGRAM FAIL, status=%x\n", status);

        return (9);
    }
    else
    {
        return (0);
    }
}

#ifdef ENABLE_SAVE_NAND_ERROR_LOG

uint8_t
spiNf_writepage (
    uint32_t    blk,
    uint32_t    ppo,
    uint8_t     * dBuf)
{
    unsigned char   SpiCmd[8]   = {0};
    int             spiret;
    uint8_t         status      = 0x01;
    uint32_t        addr        = 0;
    uint8_t         wpStatus    = 0x0;
    uint32_t        new_blk     = 0;

    (void)printf("spiNf_writepage: %x, %x, %x\n", blk, ppo, dBuf);
    n_cmdReset(0);
    (void)usleep(10000);

    if (spiNf_getFeature(0xC0, &wpStatus) != 0)
    {
        (void)printf(" get feature.6 [0xC0] = %x!!\n", wpStatus);
    }
    if (wpStatus & NF_STATUS_OIP)
    {
        //spiNf_waitReady(&wpStatus, 3000);
		if (spiNf_waitReady(&wpStatus, 3000))
		{
			(void)printf(" wait Ready timeout, s = %x!!\n", wpStatus);
		}
    }

    // die select & get local block index
    new_blk = n_getNewBlkIndex(blk);
    addr    = new_blk * gSpiNfInfo.PageInBlk + ppo;

    n_chkRegA0B4WtErs();

    // 06H (write enable)
    if ((wpStatus & NF_STATUS_WEL) != 0)
    {
        (void)printf("WEL=1, C0=%x, A0=%x\n", wpStatus, status);
    }
    else
    {
        SpiCmd[0]   = 0x06;
        spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
        if (spiret != SPINF_OK)
        {
            spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
        }
    }

    #ifdef USE_AXISPI_ENGINE
        #ifdef ENABLE_QUAD_OPERATION_MODE
        if(spiNf_setQuadIoOperation(1))
		{
			(void)printf("spiNf_setQuadIoOperation error.13\n");
		}
        #else
        if(spiNf_setQuadIoOperation(0))
		{
			(void)printf("spiNf_setQuadIoOperation error.14\n");
		}
        #endif
    #endif

    #ifdef USE_MMP_DRIVER
    ithFlushDCacheRange((void *)&dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.FtlSprSize);
    ithFlushMemBuffer();
    #endif

    // 02H(program load)
    #ifdef ENABLE_QUAD_OPERATION_MODE
    if (g_TOSHIBA_QuadMode)
    {
        SpiCmd[0] = 0x02;                        // Toshiba's 1st gen SPI NAND does NOT support QUAD-IO program
    }
    else
    {
        SpiCmd[0] = 0x32;
    }
    #else
    SpiCmd[0]   = 0x02;
    #endif

    SpiCmd[1]   = 0x00;

    if (g2PlaneNandShift)
    {
        if ((g2PlaneNandShift != 4) && (g2PlaneNandShift != 5))
        {
            (void)printf("SPI NAND Error: incorrect g2PlaneNandShift value, %x\n", g2PlaneNandShift);
        }
        else
        {
            SpiCmd[2] = ((blk & 0x01) << g2PlaneNandShift); // if plane mode (0x00 | (blk&0x01)<<g2PlaneNandShift);
        }
    }
    else
    {
        SpiCmd[2] = 0x00;
    }

    SpiCmd[3] = 0x00;

    if (gNeedReMapSpare)
    {
        uint8_t tmpBuf[24];
        (void)memcpy(&tmpBuf[0], &dBuf[gSpiNfInfo.PageSize + 0], 24);
        (void)memset(&dBuf[gSpiNfInfo.PageSize], 0xFF, gSpiNfInfo.SprSize);

        n_reMapSpare(&dBuf[gSpiNfInfo.PageSize], &tmpBuf[0], SPINF_REMAP_SPARE_FOR_WRITE);

    #ifdef USE_MMP_DRIVER // remove this function if WIN32
        ithFlushDCacheRange((void *)&dBuf[gSpiNfInfo.PageSize], gSpiNfInfo.SprSize);
        ithFlushMemBuffer();
    #endif
    }

    doPowerDownProtectNand(1);

    spiret      = spi_write(SpiCmd, 3, &dBuf[0], gSpiNfInfo.PageSize + gSpiNfInfo.SprSize);

    // 10H (program execute)
    SpiCmd[0]   = 0x10;
    SpiCmd[1]   = (addr >> 16) & 0xFF;
    SpiCmd[2]   = (addr >> 8) & 0xFF;
    SpiCmd[3]   = addr & 0xFF;
    spiret      = spi_write(SpiCmd, 4, &SpiCmd[0], 0);

    // 0FH (get feature) (wait busy)
	if (spiNf_waitReady(&status, 3000))
	{
		(void)printf(" wait Ready timeout, s = %x!!\n", wpStatus);
	}
		
    if (spiNf_getFeature(0xC0, &wpStatus) != 0)
    {
        (void)printf(" get feature.1 [0xC0] = %x!!\n", wpStatus);
    }

    if (wpStatus & 0x0C)
    {
        uint8_t s = 0x00;
        (void)printf(" P/E fail.2 [0xC0] = %x, b=%x!!\n", wpStatus, blk);
        (void)spiNf_getFeature(0xA0, &s);
        (void)printf(" get feature [0xA0] = %x!!\n", s);
    }

    #ifdef USE_AXISPI_ENGINE
        #ifndef FAST_NAND_READ
        if(spiNf_setQuadIoOperation(0))
		{
			(void)printf("spiNf_setQuadIoOperation error.15\n");
		}
        #endif
    #endif

    doPowerDownProtectNand(2);

    // 04H (write Disable)
    SpiCmd[0]   = 0x04;
    spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);

    if (wpStatus & NF_STATUS_P_FAIL)
    {
        if (n_cmdReset(0))
        {
            uint8_t * p = (uint8_t *)0xFFFFFFFF;
            (void)printf("[SPINF ERR] reset commnad fail:1\n", status);
            (void)usleep(100 * 1000);
            *p = 1;
        }

        (void)printf("SPI NAND PROGRAM FAIL, status=%x\n", status);

        return (9);
    }
    else
    {
        return (0);
    }
}
#endif

// =============================================================================

/**
* SPI NAND erase block data
*
* @return 0 If successful. 1 if bus error, 2 if Erase E-fail bit is set.
*/

// =============================================================================
uint8_t
spiNf_BlockErase (
    uint32_t blk)
{
    unsigned char   SpiCmd[4];
    int             spiret;
    uint8_t         status      = 0x01;
    uint8_t         wpStatus    = 0x0;
    uint32_t        new_blk     = 0;

    if (blk < gCfgRsvBlkSize)
    {
        (void)printf("Ers(0x%x), rsvBlkSize = %x\n",blk, gCfgRsvBlkSize);
    }

#ifdef ENABLE_WP_RESERVED_AREA
    if (chkWp(blk))
    {
        if (!gFakeErrorHappen || (gFakeErrorHappen == 0xAAAAAAAA))
        {
            uint8_t b[32];

            (void)printf("spiNf_BlockErase: S/W WP fail: blk = %x\n", blk);

    #ifdef ENABLE_SAVE_NAND_ERROR_LOG
            sprintf(b, "Ers: b = %03x", blk);
            _save_log(b, 32);
    #endif

            if (gEnWpStopNfDriver)
            {
                (void)printf("spiNf_BlockErase: FAIL, Stop it: blk = %x\n", blk);
                while (1)
                {
                }
            }
            if (gEnWpReturnFail)
            {
                // reset command
                (void)printf("spiNf_BlockErase: FAIL, Reset it: blk = %x\n", blk);
                n_cmdReset(0);
                (void)usleep(10000);
                return (1);
            }
        }
    }
#endif

    if (blk > gSpiNfInfo.TotalBlk)
    {
        (void)printf("Erase Error: block Num Out of Range, b=%x, max=%x\n", blk, gSpiNfInfo.TotalBlk);
        return (1);
    }

    if (spiNf_getFeature(0xC0, &status) == 0)
    {
        if (((uint8_t)status & (uint8_t)NF_STATUS_E_FAIL) != (uint8_t)0)
        {
            if (n_cmdReset(0) != (uint8_t)0)
            {
                uint8_t * p = (uint8_t *)0xFFFFFFFFUL;
                (void)printf("[SPINF ERR] reset commnad fail:2, status=%x\n", status);
                (void)usleep(100 * 1000);
                *p = 0x1;
            }
            (void)printf("[SPINF ERR] ERASE fail:2, status=%x\n", status);
        }
        wpStatus = status;
    }

    new_blk = n_getNewBlkIndex(blk);

    n_chkRegA0B4WtErs();

    if (spiNf_getFeature(0xC0, &wpStatus) == 0)
    {
        if (spiNf_waitReady(&status, 3000) != (uint8_t)0)
        {
            (void)printf(" erase FAIL, wait ready timeout(blk=%x)!!\n", blk);
            return (1);
        }
    }

    // 06H (write enable)
    if (((uint8_t)wpStatus & (uint8_t)NF_STATUS_WEL) != (uint8_t)0)
    {
        (void)printf("WEL=1\n");
    }
    else
    {
        SpiCmd[0]   = 0x06;
        spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
        if (spiret != SPINF_OK)
        {
            spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
        }
        // ithDelay(100);
    }

    doPowerDownProtectNand(3);

    // D8H
    SpiCmd[0]   = 0xD8;
    SpiCmd[1]   = ((new_blk * gSpiNfInfo.PageInBlk) >> 16) & (uint8_t)0xFF;
    SpiCmd[2]   = ((new_blk * gSpiNfInfo.PageInBlk) >> 8) & (uint8_t)0xFF;
    SpiCmd[3]   = (new_blk * gSpiNfInfo.PageInBlk) & (uint8_t)0xFF;
    spiret      = spi_write(SpiCmd, 4, &SpiCmd[0], 0);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_ERS_ERR);
    }

    // 0F (get feature)
    if (spiNf_waitReady(&status, 5000) != (uint8_t)0)
    {
        (void)printf(" erase FAIL, wait ready timeout2(blk=%x)!!\n", blk);
        return (1);
    }

    if (spiNf_getFeature(0xC0, &status) != (uint8_t)0)
    {
        (void)printf("Get Erase Register FAIL, don't need Erase it again, [0xC0] = %x!!\n", status);
        wpStatus = 0;
        return (1);
    }
    else
    {
        // (void)printf("Erased & get feature success, [0xC0] = %x!!\n",status);
        wpStatus = status;
    }

    doPowerDownProtectNand(4);

    // 04H (write Disable)
    SpiCmd[0]   = 0x04;
    spiret      = spi_write(SpiCmd, 1, &SpiCmd[0], 0);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_WT_EN_ERR);
    }

    // (void)printf("Ers(%x,%x)\n",blk,wpStatus);

    if (((uint8_t)wpStatus & (uint8_t)NF_STATUS_E_FAIL) != (uint8_t)0)
    {
        uint8_t tmpA0 = 0;
        (void)printf(" erase FAIL(blk=%x, s=%x)!!\n", blk, wpStatus);
        regA0 = -1;
		if (spiNf_getFeature(0xA0, &tmpA0) != (uint8_t)0)
		{
			(void)printf(" reCheck regA0 FAIL!!\n");
		}
        (void)printf(" reCheck regA0 = %x!!\n", tmpA0);
    }
#if CFG_NAND_READ_CACHE_SIZE > 0
    NAND_cache_blockInvalid (blk);
#endif
    if ((wpStatus & (uint8_t)NF_STATUS_E_FAIL) != (uint8_t)0)
    {
        return (2);
    }
    else
    {
        return (0);
    }
}

#if 0
uint32_t
GetNewBlockIndex (
    uint32_t blk)
{
    return n_getNewBlkIndex(blk);
}
#endif

void
spiNf_SetSpiCsCtrl (
    uint32_t csCtrl)
{
    (void)printf("spiNf_SetSpiCsCtrl(%x)\n", csCtrl);
    //gEnSpiCsCtrl = csCtrl;
}

void
spiNf_Reset (
    void)
{
    unsigned char   SpiCmd[1];
    unsigned char   dBuf;
    int             spiret;

    // FFH (reset)
    SpiCmd[0]   = 0xFF;

    spiret      = spi_write(&SpiCmd[0], 1, &dBuf, 0);
    if (spiret != SPINF_OK)
    {
        spinf_error(SPINF_ERROR_CMD_SDS_ERR);
    }
}

void
spiNf_SwitchToDie0InISR (
    void)
{
    if ((gSpiNfInfo.MID == (uint8_t)0xEF) && (gSpiNfInfo.DID0 == (uint8_t)0xAB))
    {
        unsigned char   SpiCmd[2];
        unsigned char   dBuf;

        // (void)ithPrintf("SpiNf_SwitchToDie0InISR: die=%x\n", ds);
        SpiCmd[0]   = 0xC2;
        SpiCmd[1]   = 0;  // die 0
        if(axispi_write_in_isr(SpiCmd, 2, &dBuf, 0) == false)
		{
			(void)printf("axispi_write_in_isr is FAIL\n");
		}
    }
}

/**
 * @brief Switch to die 0 if the nandflash is Winbond M02
 *
 * @return uint8_t 0 if success, otherwise error code.
 */
uint8_t
SpiNf_SwitchToDie0 (
    void)
{
    uint8_t ret = 0;

    if ((gSpiNfInfo.MID == (uint8_t)0xEF) && (gSpiNfInfo.DID0 == (uint8_t)0xAB))
    {
        unsigned char   SpiCmd[2];
        unsigned char   dBuf;

        // (void)ithPrintf("SpiNf_SwitchToDie0InISR: die=%x\n", ds);
        SpiCmd[0]   = 0xC2;
        SpiCmd[1]   = 0;  // die 0
        ret         = (SPI_OK == axispi_write(SpiCmd, 2, &dBuf, 0)) ? SPINF_OK : SPINF_FAIL;
    }

    return ret;
}

/**
 * Set NAND OTP Enable/Disable
 * @param mod: the UID mode(0: disable, 1: enable)
 * @output: the result, 0: success, 1:Not Support OTP, others: fault
 */
uint8_t
spiNf_SetOtpEnableBit(
	uint8_t		mode)
{
	uint8_t	res = 2;
	uint8_t	s = 0;
	uint8_t reg_B0 = 0;
	
	if(gIsSupportOtp == (uint8_t)0u)
	{
		(void)printf("This NAND does Not Support OTP!\n");
		res = 1;
		goto setEnd;
	}
	
	if(mode == 1)
	{
		//Enable OTP_EN bit 
		(void)spiNf_getFeature(0xB0, &s);
		//(void)printf("B0 = %02x\n",s);
		if((s & NF_REG_OTP_EN) == NF_REG_OTP_EN)
		{
			res = 0;
			goto setEnd;
		}
		
		reg_B0 = s | NF_REG_OTP_EN;
		//(void)printf("11.B0 = %02x, %02x\n",s, reg_B0);
		(void)spiNf_setFeature(0xB0, reg_B0);
		(void)spiNf_getFeature(0xB0, &s);
		if((s & NF_REG_OTP_EN) != NF_REG_OTP_EN)
		{
			(void)printf("Set OTP_EN bit of reg B0 FAIL: regB0: %02x, %02x\n",s, reg_B0);
			goto setEnd;
		}
		res = 0;
	}
	
	if(mode == 0)
	{
		//disable OTP
		(void)spiNf_getFeature(0xB0, &s);
		//(void)printf("B0 = %02x\n",s);		
		if((s & NF_REG_OTP_EN) == 0x00)
		{
			res = 0;
			goto setEnd;
		}
		
		reg_B0 = s & ~NF_REG_OTP_EN;	
		//(void)printf("1.B0 = %02x, %02x\n",s, reg_B0);
		(void)spiNf_setFeature(0xB0, reg_B0);
		(void)spiNf_getFeature(0xB0, &s);
		if((s & NF_REG_OTP_EN) != 0x00)
		{
			(void)printf("Set OTP_EN bit of reg B0 FAIL: regB0: %02x, %02x\n",s, reg_B0);
			goto setEnd;
		}		
		res = 0;
	}
	
setEnd:

	if(res == 0u)
	{
		//(void)printf("Set OTP_EN bit success: regB0: %02x, m=%02x\n",s, mode);
	}

	return res;	
}

/**
 * check NAND OTP_PRT bit
 * @output: the OTP_PRT bit status, or 0xFE: Not Support OTP, 0xFF: Bus Error
 */
uint8_t
spiNf_GetOtpProtectStatus (void)
{
	uint8_t	res = 0xFE;	//0xFE: Not Support OTP
	uint8_t	s = 0;
	uint8_t reg_B0 = 0;
	
	//printf("gIsSupportOtp = %x!\n",gIsSupportOtp);

	if(gIsSupportOtp == (uint8_t)1u)
	{
		if (spiNf_getFeature(0xB0, &s) == (uint8_t)0)
		{
			//SPI BUS OK!
			if((s & NF_REG_OTP_PRT) == NF_REG_OTP_PRT)
			{
				res = 1;
			}
			else
			{
				res = 0;
			}			
		}
		else
		{
			res = 0xFF;	//0xFF: Bus Error
		}
	}
	else
	{
		printf("This NAND does Not Support OTP!\n");
	}

	return res;
}

/**
 * Set NAND Write Protection
 * @param blk: the start block index
 * @param bNum: the WP size(in block)
 * @param mod: the WP mode
 * @output: the result, 0: success, others: fault
 */
uint8_t
spiNf_SetWtProtect (
    uint32_t    blk,
    uint32_t    bNum,
    uint8_t     mod)
{
#ifdef ENABLE_WP_RESERVED_AREA
    (void)printf("spiNf_SetWtProtect: start=%x, cnt=%x, mod=%x\n", blk, bNum, mod);
    // (void)printf("   start block: %x\n",blk);
    // (void)printf("   WP block counts: %x\n",bNum);
    // (void)printf("   WP mode: %x\n",mod);

    if (blk > gCfgRsvBlkSize)
    {
        (void)printf("%s[%d]NAND set Wt Protect Fail, start block > reserved size: %x, %x\n", __FILE__, __LINE__, blk, gCfgRsvBlkSize);
        (void)printf("So that Write protect whole reserved area, size = %x\n", gCfgRsvBlkSize);
        gSpiNfInfo.wpStartBlk   = 0;
        gSpiNfInfo.wpBlkCnt     = gCfgRsvBlkSize;
        if (gCfgRsvBlkSize)
        {
            gSpiNfInfo.wpMode = 1;
        }
        else
        {
            gSpiNfInfo.wpMode = 0;
        }

        return 1;
    }

    if (blk + bNum > gCfgRsvBlkSize)
    {
        (void)printf("%s[%d]NAND set Wt Protect Fail, start block + Num > reserved size: %x, %x, %x\n", __FILE__, __LINE__, blk, bNum, gCfgRsvBlkSize);
        (void)printf("So that Write protect whole reserved area, size = %x\n", gCfgRsvBlkSize);
        gSpiNfInfo.wpStartBlk   = 0;
        gSpiNfInfo.wpBlkCnt     = gCfgRsvBlkSize;
        if (gCfgRsvBlkSize)
        {
            gSpiNfInfo.wpMode = 1;
        }
        else
        {
            gSpiNfInfo.wpMode = 0;
        }

        return 2;
    }

    if (mod)
    {
        gSpiNfInfo.wpStartBlk   = blk;
        gSpiNfInfo.wpBlkCnt     = bNum;
        gSpiNfInfo.wpMode       = mod;
    }
    else
    {
        gSpiNfInfo.wpStartBlk   = 0;
        gSpiNfInfo.wpBlkCnt     = 0;
        gSpiNfInfo.wpMode       = 0;
    }

    n_setBlkLockReg(mod);

    return 0;
#else
    return 0;
#endif
}

#ifdef __cplusplus
}
#endif
