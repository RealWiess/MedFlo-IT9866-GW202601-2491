#include <unistd.h>     // for usleep()
#include <stdio.h>      // for (void)printf()
#include <string.h>     // for strcpy()
#include <malloc.h>

#include "ite/ith.h"
#include "ite/itp.h"    // for itpVmemAlloc()
#include "inc/configs.h"
#include "inc/nf_spi_nand.h"

////////////////////////////////////////////////////////////
//    GLOBAL VARIABLE
////////////////////////////////////////////////////////////
static uint8_t * gNfDataBuf = NULL;

#define    SNF_PAGE_SIZE CFG_NAND_PAGE_SIZE
////////////////////////////////////////////////////////////
// Compile Option
////////////////////////////////////////////////////////////
// #define    SHOW_RWE_MSG
// #define    ENABLE_ERASE_ALL_BLOCKS

// #define    ENABLE_DEBUG_FTLWTERR_ISSUE
/********************************************************************
* enum/type define
********************************************************************/
#define    SPINF_INVALID_REVBLK_SIZE (0xFFFFFFFF)

typedef uint8_t (*BLOCK_CHECK_METHOD) (
    uint32_t pba);

enum {
    LL_OK,
    LL_ERASED,
    LL_ERROR
};

typedef enum {
    NAND_MAKER_TOSHIBA = 0,                                                 // 0
    NAND_MAKER_SAMSUNG,                                                     // 1
    NAND_MAKER_HYNIX,                                                       // 2
    NAND_MAKER_NUMONYX,                                                     // 3
    NAND_MAKER_POWERFLASH,                                                  // 4
    NAND_MAKER_GPOWER,                                                      // 5
    NAND_MAKER_WINBOND,                                                     // 6
    NAND_MAKER_XTX,                                                         // 7
    NAND_MAKER_PARAGON,                                                     // 8
    NAND_MAKER_GIGADEVICE,                                                  // 9
    NAND_MAKER_ESMT,                                                        // 10
    NAND_MAKER_SKYHIGH,                                                     // 11
    NAND_UNKNOWN_MAKER
} NAND_MAKER;

typedef enum {
    NAND_TOSHIBA_METHOD_01 = 0,                                             // Check byte 0 in data & 1st bytes in spare of 1st & 2nd page
    NAND_TOSHIBA_METHOD_02,                                                 // Check byte 0 in data & 1st bytes in spare of last page
    NAND_TOSHIBA_METHOD_03,                                                 // Check byte 0 in data & 1st bytes in spare of 1st page & last page
    NAND_SAMSUNG_METHOD_01,                                                 // Check 1st bytes in spare of 1st & 2nd page
    NAND_SAMSUNG_METHOD_02,                                                 // Check 1st bytes in spare of last page
    NAND_HYNIX_METHOD_01,                                                   // Check 1st bytes in spare of 1st & 2nd page
    NAND_HYNIX_METHOD_02,                                                   // Check 1st bytes in spare of last page & (last-2) page
    NAND_NUMONYX_METHOD_01,                                                 // Check 1st & 6th byte in spare of 1st page
    NAND_NUMONYX_METHOD_02,                                                 // Check 1st byte in spare of last page
    NAND_POWERFLASH_METHOD_01,
    NAND_GPOWER_METHOD_01,                                                  // Check 1st byte in spare of first page
    NAND_UNKNOWN_METHOD
} NAND_BLOCK_CHECK_METHOD;

typedef struct
{
    unsigned long   wear;                                                   /* spare area 32-bit Wear Leveling Counter */
    unsigned char   dummy[8];                                               /* 8 bytes allocated for any structure below */
    unsigned long   ecc;                                                    /* space for ECC for lower layer calculation */
    unsigned long   goodCode;
    unsigned long   revBlkNum;
    unsigned char   dummy32[104];                                           /* other 128-24 = 104 dummy byte */
} SPINF_ST_SPARE;

/********************************************************************
* global variable
********************************************************************/
static SPI_NF_INFO      g_NfAttrib;
static unsigned long    g_ReservedBlockNum  = SPINF_INVALID_REVBLK_SIZE;    // it's from Kconfig setting
static unsigned long    g_SprRsvBlockNum    = SPINF_INVALID_REVBLK_SIZE;    // it's from SPI NAND

static unsigned char    gOtpWriteProtection = 0;
static unsigned char    gOtpEnable = 0;

#if 0                                                                       // def  CFG_CPU_BIG_ENDIAN  find USB driver for ENDIAN setitng
static const MMP_UINT32 WRITE_SYMBOL        = 0x574C5446;
#else
static const MMP_UINT32 WRITE_SYMBOL        = 0x46544C57;
#endif

#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
extern uint8_t gIsCfgAllowAdjustRsvSize;               
extern uint8_t gBootImgExist;
extern unsigned long g_ftl_reserved_size;
extern unsigned long g_pkg_reserved_size;
extern uint8_t gDoFtlReformatFlow;
extern uint8_t gPkgUpgrading;

uint8_t PkgUgMgcNum = 0x96;
#endif

extern unsigned char gForcePioMode;
extern unsigned char gIsSupportOtp;
/********************************************************************
* private function
********************************************************************/
void 
SpiNf_MarkAsBadBlk (
    uint32_t pba);

uint8_t
_checkEcc (
    void)
{
    uint8_t status;

    LOG_INFO "[LEAVE]SpiNf_Read: \n" LOG_END
    return (0);
}

uint8_t
_calBitCntDW (
    uint32_t * p)
{
    uint8_t     cnt = 0;
    uint32_t    s   = 0;
    uint32_t    i   = 0;
    uint32_t    tmp = *p;

    // (void)printf("input: %08X\n", tmp);

    for (i = 0; i < 32; i++)
    {
        s = (uint32_t)1 << i;
        // (void)printf("   s= %x, %x\n", s, tmp & s);
        if (tmp & s)
        {
            cnt++;
        }
    }
    // (void)printf("   bit cnt=%x\n", cnt);

    return cnt;
}

uint8_t
_checkSprGoodBlkMark (
    uint32_t * p)
{
    uint8_t     status  = 0;
    uint32_t    tmp     = *p;

    if (tmp == WRITE_SYMBOL)
    {
        status = 1;                     // return 1 for good
    }
    else
    {
        uint32_t dwDif = tmp ^ WRITE_SYMBOL;

        // (void)printf("XOR p(%08X, %08X) r(%08X) \n", tmp, WRITE_SYMBOL, dwDif);

        if (_calBitCntDW(&dwDif) < 3)
        {
            // (void)printf("mark bit < 3, mrk=%08X, WtSym=%08X) d=%08X\n");
            status = 1; // return 1 for good
        }
        /*
        else
        {
            (void)printf("XOR > 3, \n");
            (void)printf("XOR mrk=%08X, WtSym=%08X) d=%08X\n", tmp, WRITE_SYMBOL, dwDif);
        }
        */
    }

    // (void)printf("chlRes: %x, %x, %x\n", tmp, WRITE_SYMBOL, status);

    return status;
}

static uint8_t
_checkOriBadBlock (
    uint32_t pba)
{
    uint8_t rst     = LL_ERROR;
    uint8_t spiRst;
    uint8_t Byte    = 0;

    // disable NAND ECC
    // SpiNf_InternalEcc(0);

    switch (g_NfAttrib.BBM_type)
    {
        case 1: // for GigaDevice or ATO or Foresee BBM_TYPE_GD1
            // (void)printf("BBM type1\n");
            spiRst = SpiNf_ReadOneByte(pba, 0, 0, &Byte);
            if (Byte == 0xFF)
            {
                rst = LL_OK;
            }
            break;

        case 2: // for BBM_TYPE_MXIC1
                // (void)printf("BBM type2\n");
            spiRst = SpiNf_ReadOneByte(pba, 0, 12, &Byte);
            if (Byte == 0xFF)
            {
                spiRst = SpiNf_ReadOneByte(pba, 1, 12, &Byte);
                if (Byte == 0xFF)
                {
                    rst = LL_OK;
                }
            }
            break;

        case 3: // for BBM_TYPE_WINBOND1
                // (void)printf("BBM type3\n");
                // check the 0th byte of data area at first
            spiRst = spiNf_ByteRead(pba, 0, gNfDataBuf, 0, 16);
            if (gNfDataBuf[0] == 0xFF)
            {
                // check the 0th byte of spare area
                // however, the function "SpiNf_ReadOneByte()" has exchange the data to 16th byte of spare area
                spiRst = SpiNf_ReadOneByte(pba, 0, 16, &Byte);
                if (Byte == 0xFF)
                {
                    rst = LL_OK;
                }
            }
            break;

        case 4: // for BBM_TYPE_XTX
                // (void)printf("BBM type4\n");
            spiRst = SpiNf_ReadOneByte(pba, 0, 0, &Byte);
            if (Byte == 0xFF)
            {
                // check the 0th byte of spare area
                // however, the function "SpiNf_ReadOneByte()" has exchange the data to 0th byte of spare area
                spiRst = SpiNf_ReadOneByte(pba, 1, 0, &Byte);
                if (Byte == 0xFF)
                {
                    rst = LL_OK;
                }
            }
            break;

        case 5: // for BBM_TYPE_TOSHOBA
                // (void)printf("BBM type5\n");
            spiRst = SpiNf_ReadOneByte(pba, 0, 0, &Byte);
            if (Byte == 0xFF)
            {
                // check the 0th byte of spare area
                spiRst = SpiNf_ReadOneByte(pba, 1, 0, &Byte);
                if (Byte == 0xFF)
                {
                    rst = LL_OK;
                }
            }
            break;

        case 6: // for BBM_TYPE_ESMT(see page:39 of F50L2G41XA(2B).pdf)
                // (void)printf("BBM type6\n");
            spiRst = SpiNf_ReadOneByte(pba, 0, 12, &Byte);
            if (Byte == 0xFF)
            {
                spiRst = SpiNf_ReadOneByte(pba, 1, 12, &Byte);
                if (Byte == 0xFF)
                {
                    rst = LL_OK;
                }
            }
            break;

        case 7: // for
                // (void)printf("BBM type7\n");
            spiRst = SpiNf_ReadOneByte(pba, 0, 16, &Byte);
            if (Byte == 0xFF)
            {
                spiRst = SpiNf_ReadOneByte(pba, 1, 16, &Byte);
                if (Byte == 0xFF)
                {
                    rst = LL_OK;
                }
            }
            break;

        case 8: // for NAND_MAKER_SKYHIGH(read page 0, 1 and last)
                // (void)printf("BBM type8\n");
            spiRst = SpiNf_ReadOneByte(pba, 0, 0, &Byte);
            if (Byte == 0xFF)
            {
                spiRst = SpiNf_ReadOneByte(pba, 1, 0, &Byte);
                if (Byte == 0xFF)
                {
                    spiRst = SpiNf_ReadOneByte(pba, g_NfAttrib.PageInBlk - 1, 0, &Byte);
                    if (Byte == 0xFF)
                    {
                        rst = LL_OK;
                    }
                }
            }
            break;

        default:    // for GigaDevice or default_BBM_TYPE
            spiRst = SpiNf_ReadOneByte(pba, 0, 0, &Byte);
            if (Byte == 0xFF)
            {
                rst = LL_OK;
            }
            break;
    }

    // enable NAND ECC
    // SpiNf_InternalEcc(1);

#ifdef SHOW_INFO_MSG
    (void)printf("_chkbadBlk=%x,%x\n", pba, rst);
#endif

    return rst;
}

#ifdef ENABLE_DEBUG_FTLWTERR_ISSUE

uint32_t
_calBit (
    uint8_t Byte)
{
    uint32_t    k       = 0;
    uint32_t    c       = 0;
    uint8_t     tmpB    = 0x01;

    for (k = 0; k < 8; k++)
    {
        if (Byte & (tmpB << k))
        {
            c++;
        }
    }
    return c;
}

/*
  check FTL's Spare format(16-bytes)
  return: 0: OK, others: false
*/
static uint8_t
chkErrFtlSpareFormat (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     * spr,
    uint8_t     m)
{
    uint8_t     * b         = (uint8_t *)spr;
    uint32_t    * bf32      = (uint32_t *)b;
    uint32_t    lba         = bf32[2];
    uint8_t     isAll0xFF   = 1;
    uint8_t     isAll0x00   = 1;
    uint8_t     res         = 0;
    uint32_t    k           = 0;
    uint32_t    total_bit   = 0;

    // patch for ftlWtErr2 issue
    // if log block's lba > total block, then return error to FTL

    for (k = 0; k < 16; k++)
    {
        if (b[k] != 0xFF)
        {
            isAll0xFF = 0;
        }
        total_bit = _calBit(b[k]);
    }
    if (isAll0xFF && (total_bit >= 606))
    {
        (void)printf("Got Bit Error Case(%x, %x, %x)\n", pba, ppo, m);
        isAll0xFF = 1;
        // while(1);
    }

    if (isAll0xFF == 0)
    {
        for (k = 0; k < 16; k++)
        {
            if (b[k])
            {
                isAll0x00 = 0;
            }
        }
    }

    if ((isAll0xFF == 0) && (isAll0x00 == 0))
    {
        if (((b[4] & 0xF0) == 0x30) && ((b[4] & 0x07) < 4))
        {
            // it's a FTL's map block
            // (void)printf("it's a FTL's map block: b=%x, p=%x, m=%x\n", pba, ppo, m);
            // ithPrintVram8((uint32_t)spr, 16);
            if (pba < g_ReservedBlockNum)
            {
                // reserved area has FTL's map block
                (void)printf("\n");
                (void)printf("chkFtlSpareFormatErr6: (%x, %x, %x, %x)\n", pba, ppo, g_ReservedBlockNum, m);
                ithPrintVram8((uint32_t)spr, 16);
                (void)printf("\n");
                res = 1;
            }
        }
        else
        {
            uint8_t isFF = 1;
            for (k = 0; k < 5; k++)
            {
                if (b[k] != 0xFF)
                {
                    isFF = 0;
                }
            }
            for (k = 12; k < 16; k++)
            {
                if (b[k] != 0xFF)
                {
                    isFF = 0;
                }
            }

            // it's a FTL's log or data block  0X0F: log block, 0x30~0x33 & 0x38~0x3B is Map block

            if ((b[4] & 0xF0) != 0x00)
            {
                if (isFF == 0)
                {
                    // it's incorrect FTL log block type
                    (void)printf("chkFtlSpareFormatErr2: (%x, %x, %x)\n", pba, ppo, m);
                    ithPrintVram8((uint32_t)spr, 16);
                    res = 1;
                }
                else
                {
                    // if isFF = 1, then need to return ok
                    goto chkEnd;
                }
            }

            if ((b[4] & 0x0F) != 0x0F)
            {
                // it's incorrect FTL log block type
                (void)printf("chkFtlSpareFormatErr3: (%x, %x, %x)\n", pba, ppo, m);
                ithPrintVram8((uint32_t)spr, 16);
                res = 1;
            }

            if (bf32[2] > g_NfAttrib.TotalBlk)
            {
                uint8_t caseA   = 0;
                uint8_t caseB   = 1;

                if ((b[2] == 0) && (b[3] == 0))
                {
                    caseA = 1;
                }

                for (k = 6; k < 16; k++)
                {
                    if (b[k] != 0xFF)
                    {
                        caseB = 0;
                    }
                }

                if (!caseA || !caseB)
                {
                    (void)printf("chkFtlSpareFormatErr5: (%x, %x) errBitCnt = %d, m=%x\n", pba, ppo, total_bit, m);
                    ithPrintVram8((uint32_t)spr, 16);
                    res = 1;
                }
            }
        }
    }

chkEnd:

    if (res)
    {
        (void)printf("Check Spare Error!!!\n");
    }
    return res;
}
#endif

/*
  To get the FTL's Reserved size Via read 1st F/W spare data
  return: the FTL's Reserved size(in Bytes, Not Block Counts)
*/
#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
uint32_t 
_getFwImgRsvSize(
    void)
{
	uint8_t		buf[4096+256];
	uint32_t	b = 0;
	uint32_t	tmpSize = (uint32_t)CFG_NAND_RESERVED_SIZE;
	uint32_t	imgAddr = (uint32_t)CFG_UPGRADE_IMAGE_POS;
	uint32_t	blk_size = g_NfAttrib.PageInBlk * g_NfAttrib.PageSize;
	uint8_t 	index = LL_RP_SPARE;
	uint8_t		res = 0;
	SPINF_ST_SPARE *pSpr;
	
	if(blk_size)
	{		
		b = imgAddr / blk_size;
		(void)printf("RdPart: b=%x, buf=%x, blkSize:%x, img=%x\n", b, &buf[0], blk_size, imgAddr);
		res = SpiNf_ReadPart(b, 0, &buf[0], index);		
		
	}
	
	pSpr = (SPINF_ST_SPARE *)&buf[0];
	
	if(_checkSprGoodBlkMark((uint32_t*)&pSpr->goodCode))
	{
        if(pSpr->revBlkNum)
        {
			if((pSpr->revBlkNum != 0xFFFFFFFF) && (pSpr->revBlkNum * blk_size) > CFG_UPGRADE_IMAGE_POS)
			{
				(void)printf("_getFwImgRsvSize(): Got reserved size from NAND spare data0: b=%x, p=%x, rsvB=%x, img=%x, en1=%x\n", b, 0, pSpr->revBlkNum, imgAddr, gIsCfgAllowAdjustRsvSize);
				tmpSize = pSpr->revBlkNum * blk_size;
			}
			else
			{
				if(pSpr->revBlkNum == 0xFFFFFFFF)
				{
					(void)printf("Got No Boot-Image: b=%x, p=%x, rsvB=%x, img=%x, en1=%x\n", b, 0, pSpr->revBlkNum, imgAddr, gIsCfgAllowAdjustRsvSize);
					//tmpSize = pSpr->revBlkNum * blk_size;
					gBootImgExist = 0;
				}
				else
				{
					(void)printf("Incorrect reserved size(boot-image): %x\n",pSpr->revBlkNum);
				}
			}
        }
	}
	return tmpSize;
}
#endif

/********************************************************************
* public function
********************************************************************/
void
SpiNf_GetAttribute (
    MMP_BOOL    * NfType,
    MMP_BOOL    * EccStatus)
{
    LOG_INFO  "[ENTER]SpiNf_GetAttribute: \n"  LOG_END

    * NfType    = MMP_TRUE;
    *EccStatus  = MMP_TRUE;

    LOG_INFO "[LEAVE]SpiNf_GetAttribute: \n" LOG_END
}

void
SpiNf_Getwear (
    uint32_t    blk,
    uint8_t     * dest)
{
    if (blk == 0x55AA55AA)
    {
        // wrap to get the NAND spare size info
        *dest = (uint8_t)(g_NfAttrib.SprSize / 4);
    }
    else
    {
        *dest = 0;
    }

    LOG_INFO  "[ENTER]SpiNf_Getwear: \n"  LOG_END
    LOG_INFO "[LEAVE]SpiNf_Getwear: \n" LOG_END
}

uint8_t
SpiNf_Initialize (
    uint32_t    * pNumOfBlocks,
    uint8_t     * pPagePerBlock,
    uint32_t    * pPageSize)
{
    SPI_NF_INFO dev;

#ifdef SHOW_INFO_MSG
    (void)printf("spiNf init:1\n");
#endif

    memset(&g_NfAttrib,    0,  sizeof(SPI_NF_INFO));
    memset(&dev,           0,  sizeof(SPI_NF_INFO));

    spiNf_Initial(&dev);

    (void)printf("    NfDev.Init=%x\n",      dev.Init);
    (void)printf("    NfDev.TotalBlk=%x\n",  dev.TotalBlk);
    (void)printf("    NfDev.PageInBlk=%x\n", dev.PageInBlk);
    (void)printf("    NfDev.PageSize=%x\n",  dev.PageSize);
    (void)printf("    NfDev.SprSize=%x\n",   dev.FtlSprSize);

    if (dev.PageSize != SNF_PAGE_SIZE)
    {
        /* To prevent from incorrect setting of Kconfig */
        (void)printf("[ERROR] Incorrect NAND page size in Kconfig, STOP it(this.page=%d, kcfg.page=%d)!!\n", dev.PageSize, SNF_PAGE_SIZE);
        dev.Init = 0;
    }

    if (!dev.Init)
    {
        return 1;
    }

    memcpy(&g_NfAttrib, &dev, sizeof(SPI_NF_INFO));

    *pNumOfBlocks   = g_NfAttrib.TotalBlk;
    *pPagePerBlock  = g_NfAttrib.PageInBlk;
    *pPageSize      = g_NfAttrib.PageSize;

    /* malloc read/write buffer */
    if (gNfDataBuf == NULL)
    {
        gNfDataBuf = (uint8_t *)malloc((uint32_t)g_NfAttrib.PageSize + g_NfAttrib.SprSize + 32);
        (void)printf("gNfDataBuf = %p\n", gNfDataBuf);
    }

#ifdef ENABLE_ERASE_ALL_BLOCKS
    if (1)
    {
        uint32_t i;
        spiNf_SetWtProtect(0, 0x80, 0);

        for (i = 0; i < 0x80; i++)
        {
            SpiNf_Erase(i);
        }
        for (i = 0; i < g_NfAttrib.TotalBlk; i++)
        {
            if (SpiNf_IsBadBlockForBoot(i) == LL_OK)
            {
                SpiNf_Erase(i);
            }
        }

        spiNf_SetWtProtect(0, 0x80, 1);
        (void)printf("Erase All Blocks OK!!!\n");
        while (1)
        {
        }
    }
#endif

#ifdef SHOW_INFO_MSG
    (void)printf("spiNf init:4\n");
#endif

	#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
	(void)printf("Alow to change the Reserved Size\n");	
	gIsCfgAllowAdjustRsvSize = 0x5A;
	//also need to get the reserved size(in F/W image spare)
	// get(read F/W image 1st spare) & set g_ftl_reserved_size value...
	if(g_ftl_reserved_size == 0)
	{
		g_ftl_reserved_size = _getFwImgRsvSize();
		(void)printf("NAND got FTL's Reserved Size0 = %x, kcfg: %x\n", g_ftl_reserved_size, CFG_NAND_RESERVED_SIZE);
	}
	else
	{
		unsigned long oriSize = g_ftl_reserved_size;
		g_ftl_reserved_size = _getFwImgRsvSize();
		(void)printf("NAND got FTL's Reserved Size1 = %x, ori = %x, kcfg: %x\n", g_ftl_reserved_size, oriSize, CFG_NAND_RESERVED_SIZE);
	}
	#endif
    return LL_OK;
}

uint8_t
SpiNf_Erase (
    uint32_t pba)
{
    uint8_t     status      = 1;
    uint32_t    ErsRetryCnt = 0;

    LOG_INFO  "[ENTER]SpiNf_Erase: \n"  LOG_END

	if(gOtpEnable && gOtpWriteProtection)
	{
		printf("[NAND ERROR] OTP ENABLE & OTP WP, so that skip ERASE, and return OK!\n");
		//it's impossible case for OTP enable & erase block at the same time.
		return (LL_OK);
	}

    while (status)
    {
        status = spiNf_BlockErase(pba);
        if (status == 2)
        {
            // erase Fail, need mark as bad
            (void)printf("erase Fail, need mark as bad\n");
            break;
        }
        if (ErsRetryCnt > 10)
        {
            uint8_t * p = (uint8_t *)0xFFFFFFFF;
            (void)printf("AXISPI bus MAYBE unstable.\n");
            // (void)printf("[SPINF ERR] reset commnad fail:1\n",status);
            usleep(100 * 1000);
            *p = 1;
        }
        if (status == 1)
        {
            (void)printf("Erase fail, maybe bus issue, do Erase again, b=%x, s=%x, r=%x\n", pba, status, ErsRetryCnt);
            ErsRetryCnt++;
        }
        // if (status == 0) then Erase success

        // if (status == 1) it's SPI BUS issue, retry Erase!
    }

    LOG_INFO "[LEAVE]SpiNf_Erase: \n" LOG_END

#ifdef SHOW_RWE_MSG
        (void) printf("Ers[%x,%x]\n", pba, status);
#endif

    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        return (LL_OK);
    }
}

uint8_t
SpiNf_Write (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     * pDtBuf,
    uint8_t     * pSprBuf)
{
    uint8_t status;

    LOG_INFO  "[ENTER]SpiNf_Write: \n"  LOG_END

#ifdef SHOW_RWE_MSG
        (void) printf("W1[%x,%x][%x,%x]\n", pba, ppo, pDtBuf, pSprBuf);
#endif

	if(gOtpEnable && gOtpWriteProtection)
	{
		printf("[NAND ERROR] OTP ENABLE & OTP WP, so that skip write, and return OK!\n");
		//it's impossible case for OTP enable + write OTP + OTP WP at the same time.
		//it needs to skip incorrect write operation.
		return (LL_OK);
	}

#ifdef ENABLE_DEBUG_FTLWTERR_ISSUE
    if (chkErrFtlSpareFormat(pba, ppo, (uint8_t *)&pSprBuf[0], 0))
    {
        // check Write spare format error
        (void)printf("check Write spare format error: b=%x, p=%x, spr=%x\n", pba, ppo, pSprBuf);
        return (LL_ERROR);
    }
#endif

#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
	if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading == PkgUgMgcNum))
	{
		uint32_t	tmpBlkSize = g_NfAttrib.PageSize * g_NfAttrib.PageInBlk;		
		
		if(g_pkg_reserved_size != g_ftl_reserved_size && ((g_ReservedBlockNum * tmpBlkSize) != g_pkg_reserved_size))
		{
			uint32_t	tmpRsvBlkNum = g_ReservedBlockNum;
			if(tmpBlkSize)
			{				
				g_ReservedBlockNum = g_pkg_reserved_size / tmpBlkSize;
			}			
			(void)printf("Modify the g_ReservedBlockNum as PkgRsvSize: %x, %x, %x (%x, %x)\n", g_ReservedBlockNum, g_pkg_reserved_size, g_ftl_reserved_size, gIsCfgAllowAdjustRsvSize, gPkgUpgrading);
			(void)printf("oriRsvBlkNum = %x, blkSize = %x\n",tmpRsvBlkNum, tmpBlkSize);
		}
	}
#endif

    memcpy(&gNfDataBuf[0],                         pDtBuf,                 g_NfAttrib.PageSize);
    memcpy(&gNfDataBuf[g_NfAttrib.PageSize],       pSprBuf,                16); // g_NfAttrib.FtlSprSize);
    memcpy(&gNfDataBuf[g_NfAttrib.PageSize + 16],  &WRITE_SYMBOL,          4);
    memcpy(&gNfDataBuf[g_NfAttrib.PageSize + 20],  &g_ReservedBlockNum,    4);
    memset(&gNfDataBuf[g_NfAttrib.PageSize + 24], 0xFF, g_NfAttrib.SprSize - g_NfAttrib.FtlSprSize);

    if (g_SprRsvBlockNum == SPINF_INVALID_REVBLK_SIZE)
    {
        uint32_t * rsv = (uint32_t *)&gNfDataBuf[g_NfAttrib.PageSize + 20];
        if (g_ReservedBlockNum && (g_ReservedBlockNum != SPINF_INVALID_REVBLK_SIZE))
        {
            g_SprRsvBlockNum = g_ReservedBlockNum;
        }
        else
        {
			#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
    		(void)printf("[warning WtPg] incorrect reserved size: g_SprRsvBlockNum=%x, spr=%x, en1=%x\n", g_SprRsvBlockNum, *rsv, gIsCfgAllowAdjustRsvSize);
			#else
            (void)printf("[warning WtPg] incorrect reserved size: g_SprRsvBlockNum=%x, spr=%x\n", g_SprRsvBlockNum, *rsv);
			#endif
        }
    }

    if ((g_SprRsvBlockNum != SPINF_INVALID_REVBLK_SIZE) && (g_SprRsvBlockNum != g_ReservedBlockNum))
    {
		#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
        // To prevent from different reserved size setting in the same NAND	
		if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading==PkgUgMgcNum))
		{
			/* if write rsv area, then force change the g_SprRsvBlockNum as g_ReservedBlockNum */
			g_SprRsvBlockNum = g_ReservedBlockNum;
			(void)printf("PKG upgrading, Allow return OK(w1) pba=%x, ppo=%x, sprRsv=%x, kcfgRsv=%x, en1=%x)\n", pba, ppo, g_SprRsvBlockNum, g_ReservedBlockNum, gIsCfgAllowAdjustRsvSize);
		}
		else
		{
			if(gOtpEnable == 0u)
			{
				//Don't return ERROR if gOtpEnable=1(during OTP Read/Write)			
				(void)printf("[NAND ERROR] Incorrect Reserved Size(w1): ori=%x, kcfg=%x)\n", g_SprRsvBlockNum, g_ReservedBlockNum);
				(void)printf("             Maybe the NAND reserved size has been changed in Kconfig!!\n");
				return (LL_ERROR);
			}
		}
		#else
        // To prevent from different reserved size setting in the same NAND
        (void)printf("[NAND ERROR] Incorrect Reserved Size(w1): ori=%x, kcfg=%x)\n", g_SprRsvBlockNum, g_ReservedBlockNum);
        (void)printf("             Maybe the NAND reserved size has been changed in Kconfig!!\n");
		if(gOtpEnable == 0u)
		{
			return (LL_ERROR);
		}
		#endif
    }

    status = spiNf_PageProgram(pba, ppo, gNfDataBuf);

    LOG_INFO "[LEAVE]SpiNf_Write: \n" LOG_END

    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        return (LL_OK);
    }
}

uint8_t
SpiNf_WriteDouble (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     * pBuf0,
    uint8_t     * pBuf1)
{
    uint8_t status;

    LOG_INFO  "[ENTER]SpiNf_WriteDouble: \n"  LOG_END

#ifdef SHOW_RWE_MSG
        (void) printf("W2[%x,%x][%x,%x]\n", pba, ppo, pBuf0, pBuf1);
#endif

	if(gOtpEnable && gOtpWriteProtection)
	{
		(void)printf("[NAND ERROR] OTP ENABLE & OTP WP, so that skip write double, and return OK!\n");
		//It's impossible case for OTP enable + write OTP + OTP WP at the same time.
		//It needs to skip incorrect write operation.
		return (LL_OK);
	}
	
#ifdef ENABLE_DEBUG_FTLWTERR_ISSUE
    if (chkErrFtlSpareFormat(pba, ppo, (uint8_t *)&pBuf1[g_NfAttrib.PageSize / 2], 1))
    {
        // check Write spare format error
        (void)printf("check WriteDouble spare format error: b=%x, p=%x, spr=%x\n", pba, ppo, &pBuf1[g_NfAttrib.PageSize / 2]);
        return (LL_ERROR);
    }
#endif

    memcpy(&gNfDataBuf[0],                         pBuf0,                  g_NfAttrib.PageSize / 2);
    memcpy(&gNfDataBuf[g_NfAttrib.PageSize / 2],   pBuf1,                  (g_NfAttrib.PageSize / 2) + 16); // g_NfAttrib.FtlSprSize);
    memcpy(&gNfDataBuf[g_NfAttrib.PageSize + 16],  &WRITE_SYMBOL,          4);
    memcpy(&gNfDataBuf[g_NfAttrib.PageSize + 20],  &g_ReservedBlockNum,    4);
    memset(&gNfDataBuf[g_NfAttrib.PageSize + 24], 0xFF, g_NfAttrib.SprSize - g_NfAttrib.FtlSprSize);

    if (g_SprRsvBlockNum == SPINF_INVALID_REVBLK_SIZE)
    {
        uint32_t * rsv = (uint32_t *)&gNfDataBuf[g_NfAttrib.PageSize + 20];
        if (g_ReservedBlockNum && (g_ReservedBlockNum != SPINF_INVALID_REVBLK_SIZE))
        {
            g_SprRsvBlockNum = g_ReservedBlockNum;
        }
        else
        {
            (void)printf("[warning WtDb] incorrect reserved size: g_SprRsvBlockNum=%x, spr=%x\n", g_SprRsvBlockNum, *rsv);
        }
    }

    if ((g_SprRsvBlockNum != SPINF_INVALID_REVBLK_SIZE) && (g_SprRsvBlockNum != g_ReservedBlockNum))
    {
		#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
        // To prevent from different reserved size setting in the same NAND		
		if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading == PkgUgMgcNum))
		{
			(void)printf("PKG upgrading, Allow return OK(w2), sprRsv=%x, kcfgRsv=%x, en1=%x)\n", g_SprRsvBlockNum, g_ReservedBlockNum, gIsCfgAllowAdjustRsvSize);
		}
		else
		{
			(void)printf("[NAND ERROR] Incorrect Reserved Size(w2): ori=%x, kcfg=%x, en1=%x)\n", g_SprRsvBlockNum, g_ReservedBlockNum, gIsCfgAllowAdjustRsvSize);
			(void)printf("             Maybe the NAND reserved size has been changed in Kconfig!!\n");		
			return (LL_ERROR);
		}
		#else
        (void)printf("[NAND ERROR] Incorrect reserved size(w2): ori=%x, kcfg=%x)\n", g_SprRsvBlockNum, g_ReservedBlockNum);
        (void)printf("             Maybe the NAND reserved size has been changed in Kconfig!!\n");
        return (LL_ERROR);
		#endif
    }

    status = spiNf_PageProgram(pba, ppo, gNfDataBuf);

    LOG_INFO "[LEAVE]SpiNf_WriteDouble: \n" LOG_END

    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        return (LL_OK);
    }
}

uint8_t
SpiNf_LBA_Write (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     * pDtBuf,
    uint8_t     * pSprBuf)
{
    uint8_t status;

    LOG_INFO  "[ENTER]SpiNf_LBA_Write: \n"  LOG_END
    memcpy (
        &gNfDataBuf[0],
        pDtBuf,
        g_NfAttrib.PageSize);
    memcpy(&gNfDataBuf[g_NfAttrib.PageSize], pSprBuf, g_NfAttrib.FtlSprSize);

    status = spiNf_PageProgram(pba, ppo, gNfDataBuf);

    LOG_INFO "[LEAVE]SpiNf_LBA_Write: \n" LOG_END

    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        return (LL_OK);
    }
}

uint8_t
SpiNf_Read (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     * pBuf)
{
    uint8_t     status, rst = LL_ERASED;
    uint32_t    i;

    LOG_INFO  "[ENTER]SpiNf_Read: \n"  LOG_END

#ifdef SHOW_RWE_MSG
        (void) printf("R1[%x,%x]\n", pba, ppo);
#endif

#ifdef FAST_NAND_READ
    status = spiNf_PageRead(pba, ppo, pBuf);

    if (g_SprRsvBlockNum == SPINF_INVALID_REVBLK_SIZE)
    {
        SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&pBuf[g_NfAttrib.PageSize];
        if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode))
        {
            if (pSpr->revBlkNum)
            {
                g_SprRsvBlockNum = pSpr->revBlkNum;
                (void)printf("Got reserved size from NAND spare data: b=%x, p=%x, rsvB = (%x, %x)\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum);
            }
        }
    }
    else
    {
        SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&pBuf[g_NfAttrib.PageSize];
        if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode) && (g_SprRsvBlockNum != pSpr->revBlkNum))
        {
			#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
			if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading==PkgUgMgcNum))
			{
				//(void)printf("[Read] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x), en1=%x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize);
			}
			else
			{
				(void)printf("[warning.read] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x) en1=%x, pkg=%x\n", pba, ppo, g_ReservedBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize, gPkgUpgrading);
			}						
			#else
            (void)printf("[warning] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x)\n", pba, ppo, g_ReservedBlockNum, pSpr->revBlkNum);
			#endif
        }
    }

    for (i = 0; i < g_NfAttrib.FtlSprSize; i++)
    {
        if (pBuf[g_NfAttrib.PageSize + i] != 0xFF)
        {
            rst = LL_ERROR;
        }
    }

    if (rst == LL_ERASED)
    {
        uint32_t    * p32   = (uint32_t *)&pBuf[0];
        uint32_t    chkCnt  = g_NfAttrib.PageSize / 4;
        for (i = 0; i < chkCnt; i++)
        {
            if (p32[i] != 0xFFFFFFFF)
            {
                // (void)printf("Got a Fake Erased page(%x, %x)(%x, %x)\n",pba, ppo, i, p32[i]);
                rst = LL_ERROR;
                break;
            }
        }
    }
#else
    status = spiNf_PageRead(pba, ppo, gNfDataBuf);

    memcpy(pBuf, &gNfDataBuf[0], g_NfAttrib.PageSize + g_NfAttrib.FtlSprSize);

    if (g_SprRsvBlockNum == SPINF_INVALID_REVBLK_SIZE)
    {
        SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[g_NfAttrib.PageSize];
        if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode))
        {
            if (pSpr->revBlkNum)
            {
                g_SprRsvBlockNum = pSpr->revBlkNum;
                (void)printf("Got reserved size from NAND spare data: b=%x, p=%x, rsvB = (%x, %x)\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum);
            }
        }
    }
    else
    {
        SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[g_NfAttrib.PageSize];
        if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode) && (g_SprRsvBlockNum != pSpr->revBlkNum))
        {
            (void)printf("[warning] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x)\n", pba, ppo, g_ReservedBlockNum, pSpr->revBlkNum);
        }
    }

    for (i = 0; i < g_NfAttrib.FtlSprSize; i++)
    {
        if (gNfDataBuf[g_NfAttrib.PageSize + i] != 0xFF)
        {
            rst = LL_ERROR;
        }
    }

    if (rst == LL_ERASED)
    {
        uint32_t    * p32   = (uint32_t *)&pBuf[0];
        uint32_t    chkCnt  = g_NfAttrib.PageSize / 4;
        for (i = 0; i < chkCnt; i++)
        {
            if (p32[i] != 0xFFFFFFFF)
            {
                // (void)printf("Got a Fake Erased page(%x, %x)(%x, %x)\n",pba, ppo, i, p32[i]);
                rst = LL_ERROR;
                break;
            }
        }
    }
#endif

#ifdef ENABLE_DEBUG_FTLWTERR_ISSUE
    if (!status)
    {
        status = chkErrFtlSpareFormat(pba, ppo, (uint8_t *)&pBuf[g_NfAttrib.PageSize], 2);
    }
#endif
    LOG_INFO "[LEAVE]SpiNf_Read: \n" LOG_END

    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        if (rst == LL_ERASED)
        {
            return (LL_ERASED);
        }
        else
        {
            return (LL_OK);
        }
    }
}

uint8_t
SpiNf_LBA_Read (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     * pBuf)
{
    uint8_t status;

    LOG_INFO  "[ENTER]SpiNf_LBA_Read: \n"  LOG_END

#ifdef SHOW_RWE_MSG
        (void) printf("R4[%x,%x]\n", pba, ppo);
#endif

    status = spiNf_PageRead(pba, ppo, gNfDataBuf);

    memcpy(pBuf, &gNfDataBuf[0], g_NfAttrib.PageSize + g_NfAttrib.FtlSprSize);

    LOG_INFO "[LEAVE]SpiNf_LBA_Read: \n" LOG_END

    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        return (LL_OK);
    }
}

/*
read spare data(has been remapped)
return the data of spare[offset]
*/
uint8_t
SpiNf_ReadOneByte (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     offset,
    uint8_t     * pByte)
{
    uint8_t status;

    LOG_INFO  "[ENTER]SpiNf_ReadOneByte: \n"  LOG_END

    /*
    if call this function & pba = rsv_blk --> gForcePioMode = 1
    */
    if (pba == g_ReservedBlockNum)
    {
        gForcePioMode = 1;
    }

    status = spiNf_ByteRead(pba, ppo, gNfDataBuf, g_NfAttrib.PageSize, g_NfAttrib.FtlSprSize);
    if (!status)
    {
        *pByte = gNfDataBuf[offset];
    }

    if (g_SprRsvBlockNum == SPINF_INVALID_REVBLK_SIZE)
    {
        SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[0];
        if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode))
        {
            if (pSpr->revBlkNum)
            {
                g_SprRsvBlockNum = pSpr->revBlkNum;
                (void)printf("[Rd1B] auto set reserved size: %x , %x\n", g_SprRsvBlockNum, pSpr->revBlkNum);
            }
        }
    }
    else
    {
        SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[0];
        if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode) && (g_SprRsvBlockNum != pSpr->revBlkNum))
        {
			#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
			if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading == PkgUgMgcNum))
			{
				//(void)printf("[Rd1B] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x), en1=%x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize);
			}
			else
			{
				(void)printf("[warning Rd1B] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x), en1=%x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize);
				ithPrintVram8((uint32_t)&gNfDataBuf[0], 24);
            	if (pSpr->revBlkNum)
            	{
             	    g_SprRsvBlockNum = pSpr->revBlkNum;
            	}
			}						
			#else
            (void)printf("[warning Rd1B] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x)\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum);
            ithPrintVram8((uint32_t)&gNfDataBuf[0], 24);
            if (pSpr->revBlkNum)
            {
                g_SprRsvBlockNum = pSpr->revBlkNum;
            }
			#endif
        }
    }

    /*
    if call this function & pba =  --> gForcePioMode = 1
    */
    if (pba == (g_NfAttrib.TotalBlk - 1))
    {
        gForcePioMode = 0;
    }

#ifdef SHOW_RWE_MSG
    (void)printf("R3[%x,%x,%x]=%02X, r=%x\n", pba, ppo, offset, *pByte, status);
#endif

    LOG_INFO "[LEAVE]SpiNf_Write: \n" LOG_END

    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        return (LL_OK);
    }
}

uint8_t
SpiNf_ReadPart (
    uint32_t    pba,
    uint32_t    ppo,
    uint8_t     * pBuf,
    uint8_t     index)
{
    uint8_t     status, rst = LL_ERASED;
    uint32_t    * dBuf32;
    uint32_t    i;

    LOG_INFO  "[ENTER]SpiNf_ReadPart: \n"  LOG_END

#ifdef SHOW_RWE_MSG
        (void) printf("R2[%x, %x, %x, %x]\n", pba, ppo, pBuf, index);
#endif

    dBuf32 = (uint32_t *)gNfDataBuf;

    switch (index)
    {
        case LL_RP_1STHALF: // 0x01
            status = spiNf_ByteRead(pba, ppo, &gNfDataBuf[0], 0, g_NfAttrib.PageSize / 2);
            for (i = 0; i < (g_NfAttrib.PageSize / 8); i++)
            {
                if (dBuf32[i] != 0xFFFFFFFF)
                {
                    rst = LL_ERROR;
                    break;
                }
            }
            if (rst == LL_ERASED)
            {
                status = spiNf_ByteRead(pba, ppo, &gNfDataBuf[g_NfAttrib.PageSize], g_NfAttrib.PageSize, g_NfAttrib.FtlSprSize);
                for (i = (g_NfAttrib.PageSize / 4); i < (g_NfAttrib.PageSize + g_NfAttrib.FtlSprSize) / 4; i++)
                {
                    if (dBuf32[i] != 0xFFFFFFFF)
                    {
                        rst = LL_ERROR;
                    }
                }
            }
            break;

        case LL_RP_2NDHALF: // 0x00
            status = spiNf_ByteRead(pba, ppo, &gNfDataBuf[g_NfAttrib.PageSize / 2], g_NfAttrib.PageSize / 2, g_NfAttrib.PageSize / 2);
            for (i = (g_NfAttrib.PageSize / 8); i < (g_NfAttrib.PageSize / 4); i++)
            {
                if (dBuf32[i] != 0xFFFFFFFF)
                {
                    rst = LL_ERROR;
                    break;
                }
            }
            if (rst == LL_ERASED)
            {
                status = spiNf_ByteRead(pba, ppo, &gNfDataBuf[g_NfAttrib.PageSize], g_NfAttrib.PageSize, g_NfAttrib.FtlSprSize);
                for (i = (g_NfAttrib.PageSize / 4); i < (g_NfAttrib.PageSize + g_NfAttrib.FtlSprSize) / 4; i++)
                {
                    if (dBuf32[i] != 0xFFFFFFFF)
                    {
                        rst = LL_ERROR;
                    }
                }
            }
            break;

        case LL_RP_DATA: // 0xFE
            status = spiNf_PageRead(pba, ppo, gNfDataBuf);
            {
                if (g_SprRsvBlockNum == SPINF_INVALID_REVBLK_SIZE)
                {
                    SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[g_NfAttrib.PageSize];
                    if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode))
                    {
                        if (pSpr->revBlkNum)
                        {
                            (void)printf("[RdPt1] auto set reserved size: b&p=(%x, %x), rsv=(%x,%x)\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum);
                            g_SprRsvBlockNum = pSpr->revBlkNum;
                            ithPrintVram8((uint32_t)pSpr, 24);
                        }
                    }
                }
                else
                {
                    SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[g_NfAttrib.PageSize];
                    if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode) && (g_SprRsvBlockNum != pSpr->revBlkNum))
                    {
						#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
						if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading == PkgUgMgcNum))
						{
							//(void)printf("reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x), en1=%x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize);
						}
						else
						{
							if(gOtpEnable == 0u)
							{
								//Do not show warning if gOtpEnable=1
								(void)printf("[warning RdPt1] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x), en1=%x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize);
							}
						}						
						#else
						if(gOtpEnable == 0u)
                        {
							//Do not show warning if gOtpEnable=1(during OTP Read/Write)
							(void)printf("[warning RdPt1] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x)\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum);
						}
                        #endif
                    }
#ifdef ENABLE_DEBUG_FTLWTERR_ISSUE
                    if (!status)
                    {
                        status = chkErrFtlSpareFormat(pba, ppo, (uint8_t *)pSpr, 3);
                    }
#endif
                }
            }

            for (i = 0; i < (g_NfAttrib.PageSize + g_NfAttrib.FtlSprSize) / 4; i++)
            {
                if (dBuf32[i] != 0xFFFFFFFF)
                {
                    rst = LL_ERROR;
                    break;
                }
            }
            break;

        case LL_RP_SPARE: // 0xFF
            status = spiNf_ByteRead(pba, ppo, &gNfDataBuf[g_NfAttrib.PageSize], g_NfAttrib.PageSize, g_NfAttrib.FtlSprSize);
            {
                if (g_SprRsvBlockNum == SPINF_INVALID_REVBLK_SIZE)
                {
                    SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[g_NfAttrib.PageSize];
                    if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode))
                    {
                        if (pSpr->revBlkNum)
                        {
                            g_SprRsvBlockNum = pSpr->revBlkNum;
                            (void)printf("[RdPt2] auto set reserved size:(%x,%x) = %x, %x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum);
                        }
                    }
                }
                else
                {
                    SPINF_ST_SPARE * pSpr = (SPINF_ST_SPARE *)&gNfDataBuf[g_NfAttrib.PageSize];
                    if (_checkSprGoodBlkMark((uint32_t *)&pSpr->goodCode) && (g_SprRsvBlockNum != pSpr->revBlkNum))
                    {
					    #ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
					    if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading == PkgUgMgcNum))
					    {
					    	//(void)printf("reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x), en1=%x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize);
					    }
					    else
					    {
					    	(void)printf("[warning RdPt2] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x), en1=%x\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum, gIsCfgAllowAdjustRsvSize);
					    }
					    #else
                        (void)printf("[warning RdPt2] reserved size has changed: b=%x, p=%x, RsvBlkNum:(%x, %x)\n", pba, ppo, g_SprRsvBlockNum, pSpr->revBlkNum);
					    #endif
                    }
                }
            }
            for (i = (g_NfAttrib.PageSize / 4); i < (g_NfAttrib.PageSize + g_NfAttrib.FtlSprSize) / 4; i++)
            {
                if (dBuf32[i] != 0xFFFFFFFF)
                {
                    rst = LL_ERROR;
                }
            }
#ifdef ENABLE_DEBUG_FTLWTERR_ISSUE
            if (!status)
            {
                status = chkErrFtlSpareFormat(pba, ppo, (uint8_t *)&gNfDataBuf[g_NfAttrib.PageSize], 4);
            }
#endif
            break;

        default:
            // error read part index, do return error.
            (void)printf("SPI NAND SpiNf_ReadPart() error, incorrect index(%x)\n", index);
            return 111;
            break;
    }

    if (!status)
    {
        switch (index)
        {
            case LL_RP_1STHALF:
#ifdef SHOW_RWE_MSG
                (void)printf("R2.1[%x,%x,%x]\n", pBuf, &gNfDataBuf[0], g_NfAttrib.PageSize / 2);
#endif
                memcpy(pBuf, &gNfDataBuf[0], g_NfAttrib.PageSize / 2);
                break;

            case LL_RP_2NDHALF:
#ifdef SHOW_RWE_MSG
                (void)printf("R2.2[%x,%x,%x]\n", pBuf, &gNfDataBuf[g_NfAttrib.PageSize / 2], g_NfAttrib.PageSize / 2);
#endif
                memcpy(pBuf, &gNfDataBuf[g_NfAttrib.PageSize / 2], g_NfAttrib.PageSize / 2);
                break;

            case LL_RP_DATA:
#ifdef SHOW_RWE_MSG
                (void)printf("R2.3[%x,%x,%x]\n", pBuf, &gNfDataBuf[0], g_NfAttrib.PageSize);
#endif
                memcpy(pBuf, &gNfDataBuf[0], g_NfAttrib.PageSize);
                break;

            case LL_RP_SPARE:
#ifdef SHOW_RWE_MSG
                (void)printf("R2.4[%x,%x,%x]\n", pBuf, &gNfDataBuf[g_NfAttrib.PageSize], g_NfAttrib.FtlSprSize);
#endif
                memcpy(pBuf, &gNfDataBuf[g_NfAttrib.PageSize], g_NfAttrib.FtlSprSize);
                break;

            default: // coverity:deadcode
                (void)printf("[SPI NAND] error: unknown index=%x\n", index);
                break;
        }
    }

    LOG_INFO "[LEAVE]SpiNf_ReadPart: \n" LOG_END

#ifdef SHOW_RWE_MSG
        (void) printf("R2.f, R=[%x,%x,%x]\n", status, rst, LL_ERASED);  // 0 2 1
#endif

    if (status)
    {
        (void)printf("R2.f, R=[%x,%x,%x]\n", status, rst, LL_ERASED);   // 0 2 1
    }
    if (status)
    {
        return (LL_ERROR);
    }
    else
    {
        if (rst == LL_ERASED)
        {
            return (LL_ERASED);
        }
        else
        {
            return (LL_OK);
        }
    }
}

/*
   return:: LL_OK: GOOD block;  LL_ERROR: BAD block(or Reserved Block)
 */
uint8_t
SpiNf_IsBadBlock (
    uint32_t pba)
{
    uint8_t blockStatus = LL_ERROR;

    LOG_INFO "[ENTER]SpiNf_IsBadBlock: pba(%d), realAddress = 0x%08x\n", pba, ((pba * g_NfAttrib.PageInBlk) * SNF_PAGE_SIZE) LOG_END

    // ----------------------------------------
    // 0. PRECONDITION
    if (pba >= g_NfAttrib.TotalBlk)
    {
        (void)printf("\tLB_HWECC_IsBadBlock: ERROR! pba(%u) >= g_NfAttrib.numOfBlocks(%u)\n", pba, g_NfAttrib.TotalBlk);
        return LL_ERROR;
    }

    // Reserve boot section
    if ((pba < g_ReservedBlockNum) && (g_ReservedBlockNum != SPINF_INVALID_REVBLK_SIZE))
    {
		#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
		if(gDoFtlReformatFlow != 0xA5)
		{
			(void)printf("IBB[2] is BAD block for reserved block,(%x,%x)\n", pba, g_ReservedBlockNum);
			return blockStatus;			
		}
		#else
        (void)printf("IBB[2] is BAD block for reserved block,(%x,%x)\n", pba, g_ReservedBlockNum);
		return blockStatus;	
		#endif
    }

    // ----------------------------------------
    // 2. Read first page
    {
        MMP_UINT8       status;
        SPINF_ST_SPARE  pSpare;

        status = spiNf_PageRead(pba, 0, gNfDataBuf);

        memcpy(&pSpare, &gNfDataBuf[g_NfAttrib.PageSize], 24);

        if (_checkSprGoodBlkMark((uint32_t *)&pSpare.goodCode))
        {
            blockStatus = LL_OK;
        }
        else
        {
            blockStatus = _checkOriBadBlock(pba);
        }

        if ((blockStatus != LL_OK) && (pSpare.goodCode != 0xFFFFFFFF))
        {
            (void)printf("    [0]Spr.code[%03x] = %08x,%08x\n", pba, pSpare.goodCode, WRITE_SYMBOL);
        }
    }

#ifdef SHOW_RWE_MSG
    (void)printf("IBB[%x]=%x\n", pba, blockStatus);
#endif

    LOG_INFO "[LEAVE]SpiNf_IsBadBlock: \n" LOG_END
    return blockStatus;
}

/*
   return:: LL_OK: GOOD block;  LL_ERROR: BAD block;
 */
uint8_t
SpiNf_IsBadBlockForBoot (
    uint32_t pba)
{
    uint8_t blockStatus = LL_ERROR;

    LOG_INFO "[ENTER]SpiNf_IsBadBlockForBoot: pba(%d), realAddress = 0x%08x\n", pba, ((pba * g_NfAttrib.PageInBlk) * SNF_PAGE_SIZE) LOG_END

    // ----------------------------------------
    // 0. PRECONDITION
    if (pba >= g_NfAttrib.TotalBlk)
    {
        (void)printf("\tSpiNf_IsBadBlockForBoot: ERROR! pba(%u) >= g_NfAttrib.numOfBlocks(%u)\n", pba, g_NfAttrib.TotalBlk);
        return LL_ERROR;
    }

    // ----------------------------------------
    // 2. Read first page
    {
        MMP_UINT8       status;
        SPINF_ST_SPARE  pSpare;

        status = spiNf_PageRead(pba, 0, gNfDataBuf);

        memcpy(&pSpare, &gNfDataBuf[g_NfAttrib.PageSize], 24);

        if (_checkSprGoodBlkMark((uint32_t *)&pSpare.goodCode))
        {
            blockStatus = LL_OK;
        }
        else
        {
            uint8_t * p = (uint8_t *)gNfDataBuf;

            if (!pba)   // check block0 only
            {
                uint8_t     isAll0xFF = 1;
                uint32_t    i;

                p = (uint8_t *)&pSpare;
                for (i = 0; i < g_NfAttrib.FtlSprSize; i++)
                {
                    if (p[i] != 0xFF)
                    {
                        isAll0xFF = 0;
                        break;
                    }
                }
                if (isAll0xFF)
                {
                    blockStatus = LL_OK;
                }
            }
            else if ((g_ReservedBlockNum != SPINF_INVALID_REVBLK_SIZE) && (pba < g_ReservedBlockNum) &&
                (p[0] == 0x46) && (p[1] == 0x49) && (p[2] == 0x4e) && (p[3] == 0x45))
            {
                // (void)printf("checkBadForBoot: got FINE mark at block %x, rsv=%x!!\n", pba, g_ReservedBlockNum);
                blockStatus = LL_OK;
            }
            else
            {
                blockStatus = _checkOriBadBlock(pba);
            }
        }

        if ((blockStatus != LL_OK) && (pSpare.goodCode != 0xFFFFFFFF))
        {
            (void)printf("    [1]Spr.code[%03x] = %08x,%08x\n", pba, pSpare.goodCode, WRITE_SYMBOL);
        }

        /* patched the TRAC #2411 issue(force to return OK if block0) */
        if ((pba == 0) && (blockStatus != LL_OK))
        {
            uint32_t    i;
            uint8_t     * p = (uint8_t *)&pSpare;
            (void)printf("Got Block0 is NOT a GOOD block, spare data as follow:\n");
            for (i = 0; i < g_NfAttrib.FtlSprSize; i++)
            {
                (void)printf("%02X ", p[i]);
                if ((i & 0xF) == 0xF)
                {
                    (void)printf("\n");
                }
            }
            (void)printf("\n");
            blockStatus = LL_OK;
        }
    }

#ifdef SHOW_RWE_MSG
    (void)printf("IBB fb2[%x]=%x\n", pba, blockStatus);
#else
    if (blockStatus != LL_OK)
    {
        (void)printf("    IBB fb1[%x]=%x\n", pba, blockStatus);
    }
#endif

    LOG_INFO "[LEAVE]SpiNf_IsBadBlockForBoot: \n" LOG_END
    return blockStatus;
}

MMP_UINT8
SpiNf_SetReservedBlocks (
    unsigned int revBlks)
{
    uint8_t rst = LL_ERROR;

    LOG_INFO  "[ENTER]SpiNf_SetReservedBlocks: \n"  LOG_END
        (void) printf("SpiNf_SetReservedBlocks::%x\n", revBlks);

	#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
	(void)printf("SpiNf_SetReservedBlocks: rsvN0: k=%x, o=%x, s=%x, en=%x\n", g_ReservedBlockNum, g_SprRsvBlockNum, revBlks, gIsCfgAllowAdjustRsvSize);
	if((gIsCfgAllowAdjustRsvSize == 0x5A) && (gPkgUpgrading == PkgUgMgcNum))
	{
		rst = LL_OK;
	}
	#else
	(void)printf("SpiNf_SetReservedBlocks::%x\n", revBlks);
	#endif

    if (g_ReservedBlockNum == SPINF_INVALID_REVBLK_SIZE)
    {
        g_ReservedBlockNum  = revBlks;
        rst                 = LL_OK;
    }
    else
    {
        if (g_ReservedBlockNum == revBlks)
        {
            rst = LL_OK;
        }
        else
        {
            (void)printf("SPINF error: Nf_revBlkSize: ori=%x, tgt=%x\n", g_ReservedBlockNum, revBlks);
        }
    }

    (void)printf("SpiNf_SetReservedBlocks:%x,%x\n", g_ReservedBlockNum, revBlks);

    LOG_INFO "[LEAVE]SpiNf_SetReservedBlocks: \n" LOG_END
    return rst;
}

void
SpiNf_MarkAsBadBlk (
    uint32_t pba)
{
    uint8_t     dBuf[4096 + 512]    = {0};
    uint8_t     rbBuf[4096 + 512]   = {0};
    uint8_t     res;
    uint32_t    i;

    (void)printf("    ### mark as bad, b=%x (%x,%x)###\n", pba, g_NfAttrib.PageSize, g_NfAttrib.FtlSprSize);

    res = spiNf_PageProgram(pba, 0, dBuf);

    res = spiNf_PageRead(pba, 0, rbBuf);

    // for(i=0; i<(g_NfAttrib.PageSize+g_NfAttrib.FtlSprSize); i++)
    /*
    for (i = 0; i < (32); i++)
    {
        (void)printf("%02x ", rbBuf[i]);
        if ((i & 0x0F) == 0x0F) (void)printf("\n");
        if ((i & 0x3FF) == 0x3FF) (void)printf("\n");
    }
    (void)printf("\n");
    */
}

/**
 * Write data to NAND OTP area
 * @param dBuf: data buffer
 * @param otp_size: the otp write size
 * @output: 0: ok, others: fail
 */
uint8_t
SpiNf_WriteOtp (
    uint8_t 	*dBuf,
	uint32_t 	otp_size)
{
	SPINF_OTPW_STATUS ret = SPINF_OTPW_OK;
    uint32_t i,j,k;
	uint8_t  res = 0;
	uint8_t  otpWpS = 0;	
	uint8_t  *rdBuf = NULL;
	uint8_t  *ptnBuf = NULL;
	uint8_t  idx = LL_RP_DATA;
	uint32_t otpPg = 0;
	
	if(gIsSupportOtp == 0u)
	{
		(void)printf("Not support OTP(write): s=%x\n",gIsSupportOtp);
		ret = SPINF_OTPW_NOT_SUPPORT;
		goto otp_end;
	}
	
	if(gOtpWriteProtection)
	{
		(void)printf("	OTP has Enable Write Protection: s=%x\n",gOtpWriteProtection);
		ret = SPINF_OTPW_WP;
		goto otp_end;
	}	
	
	if(dBuf == NULL)
	{
		(void)printf("	OTP's buffer is NULL\n");
		ret = SPINF_OTPW_BUFFER_NULL;
		goto otp_end;
	}
		
	if(otp_size == 0)
	{
		(void)printf("	OTP's size == 0\n");
		ret = SPINF_OTPW_SIZE_ERROR;
		goto otp_end;
	}
	
	if(otp_size > 512)
	{
		(void)printf("	OTP's size > 512\n");
		ret = SPINF_OTPW_SIZE_ERROR;
		goto otp_end;
	}
	
	otpWpS = spiNf_GetOtpProtectStatus();	
	if(otpWpS == 0xFF)
	{
		(void)printf("	SPI BUS ERROR: s=%x\n",otpWpS);
		ret = SPINF_OTPW_BUS_ERROR;
		goto otp_end;
	}
	else
	{
		if(otpWpS == 0xFE)
		{
			(void)printf("	Not support OTP: ws=%x\n",otpWpS);
			ret = SPINF_OTPW_NOT_SUPPORT;
			goto otp_end;
		}
		else
		{
			//(void)printf("OTP_PRT: s=%x\n",otpWpS);
		}
	}
	
	if(otpWpS == 0u)
	{
		//locate memory for temp buffer
		rdBuf = (uint8_t*)malloc(g_NfAttrib.PageSize + g_NfAttrib.SprSize);
		if(rdBuf == NULL)
		{
			(void)printf("	out of memory: rdBuf is NULL\n");
			ret = SPINF_OTPW_BUFFER_NULL;
			goto otp_end;
		}	
		memset(rdBuf, 0, g_NfAttrib.PageSize + g_NfAttrib.SprSize);		
		
		ptnBuf = (uint8_t*)malloc(g_NfAttrib.PageSize + g_NfAttrib.SprSize);
		if(ptnBuf == NULL)
		{
			(void)printf("	out of memory: ptnBuf is NULL\n");
			ret = SPINF_OTPW_BUFFER_NULL;
			goto otp_end;
		}
			
		res = spiNf_SetOtpEnableBit(1);
		if(res)
		{
			//0: success, 1:Not Support OTP, others: fault
			(void)printf("	Enable OTP bit Error, R=%x\n", ret);
			ret = SPINF_OTPW_NOT_SUPPORT;
			goto otp_end;
		}
		gOtpEnable = 1;
		res = SpiNf_ReadPart(0, otpPg, rdBuf, idx);
		if(res == LL_ERASED)
		{
			//check page erased(empty), then do program
			//(void)printf("It Can Write OTP page %x: r=%x\n", otpPg, res);
			memset(ptnBuf, 0xFF, g_NfAttrib.PageSize + g_NfAttrib.SprSize);	
			memcpy(&ptnBuf[0], dBuf, otp_size);
			
			res = SpiNf_Write(0, otpPg, ptnBuf, &ptnBuf[g_NfAttrib.PageSize]);
			if(res == LL_OK)
			{
				res = SpiNf_ReadPart(0, otpPg, rdBuf, idx);
				if(res == LL_OK)
				{
					//(void)printf("show read result: r=%x\n",res);
					//(void)printf("show OTP data:\n");
					//ithPrintVram8((uint32_t)&rdBuf[0], otp_size);
					//(void)printf("\n");			
					
					if(memcmp(&rdBuf[0], ptnBuf, otp_size) != 0u)
					{
						(void)printf("	Write OTP Fail(compare fail)\n");
						ret = SPINF_OTPW_WRITE_FAIL;
						//goto otp_end; //don't do goto when OTP_EN=1
					}
				}
				else
				{
					(void)printf("	Write OTP Fail, r=%x\n", res);
					ret = SPINF_OTPW_WRITE_FAIL;
					//goto otp_end; //don't do goto when OTP_EN=1
				}
				gOtpWriteProtection = 1;
			}
			else
			{
				
				(void)printf("	OTP write Error, result r=%x\n", res);
				ret = SPINF_OTPW_WRITE_FAIL;
				//goto otp_end; //don't do goto when OTP_EN=1
			}
		}
		else
		{
			if(res == LL_OK)
			{
				(void)printf("	OTP has written, page=%d\n", otpPg);
			}
			
			if(res == LL_ERROR)
			{
				(void)printf("	OTP was Error, page=%d\n", otpPg);
			}
			(void)printf("	Can Not Write OTP:%x, r=%x\n", otpPg, res);
			ret = SPINF_OTPW_NOT_EMPTY;
			gOtpWriteProtection = 1;
		}
		gOtpEnable = 0;
		res = spiNf_SetOtpEnableBit(0);
		if(res > 1)
		{
			//0: success, 1:Not Support OTP, others: fault
			(void)printf("	Disable OTP bit Error, R=%x\n", ret);
			/* NAND will not work normal if OTP_EN bit is Error */
			ret = SPINF_OTPW_BUS_ERROR;
			gOtpEnable = 1;
			gOtpWriteProtection = 1;
		}
	}
	else
	{			
		(void)printf("	OTP Write Protection(otp_prt=1), r=%x \n",otpWpS);
		gOtpWriteProtection = 1;
		ret = SPINF_OTPW_WP;
	}
	
otp_end:
	
	if(rdBuf != NULL)
	{
		free(rdBuf);
		rdBuf = NULL;
	}
	
	if(ptnBuf != NULL)
	{
		free(ptnBuf);
		ptnBuf = NULL;
	}
	
	if(ret > 1)
	{
		(void)printf("	Write OTP FAIL, r=%x \n",ret);
	}
	
	return (uint8_t)ret;
}

/**
 * Read data rom NAND OTP area
 * @param dBuf: data buffer
 * @param otp_size: the otp write size
 * @output: 0: ok, 1:fail, 2:BUS error, 3:Not support OTP, 4:buffe NULL, 5:size error, 6:Empty, 7:ECC error
 */
uint8_t
SpiNf_ReadOtp (
    uint8_t 	*dBuf,
	uint32_t 	otp_size)
{
	SPINF_OTPW_STATUS ret = SPINF_OTPR_OK;	
	uint8_t  res = 0;
	uint8_t  otpWpS = 0;	
	uint8_t  *buf8 = NULL;
	uint8_t  idx = 0;
	uint32_t otpPg = 0;
	
	if(gIsSupportOtp == 0u)
	{
		(void)printf("	Not support OTP(read): s=%x\n",gIsSupportOtp);
		ret = SPINF_OTPR_NOT_SUPPORT;
		goto r_end;
	}
	
	if(dBuf == NULL)
	{
		(void)printf("	OTP's buffer is NULL\n");
		ret = SPINF_OTPR_BUFFER_NULL;
		goto r_end;
	}
		
	if(otp_size == 0)
	{
		(void)printf("	OTP's size == 0\n");
		ret = SPINF_OTPR_SIZE_ERROR;
		goto r_end;
	}
	
	if(otp_size > 512)
	{
		(void)printf("	OTP's size > 512\n");
		ret = SPINF_OTPR_SIZE_ERROR;
		goto r_end;
	}
	
	otpWpS = spiNf_GetOtpProtectStatus();	
	if(otpWpS == 0xFF)
	{
		(void)printf("	SPI BUS ERROR: s=%x\n",otpWpS);
		ret = SPINF_OTPR_BUS_ERROR;
		goto r_end;
	}
	else
	{
		if(otpWpS == 0xFE)
		{
			(void)printf("	Not support OTP: rs=%x\n",otpWpS);
			ret = SPINF_OTPR_NOT_SUPPORT;
			goto r_end;
		}
	}
	
	if(otpWpS == 0)
	{
		gOtpWriteProtection = 0;
	}
	
	if(otpWpS == 1)
	{
		gOtpWriteProtection = 1;
	}
	
	//locate memory for temp buffer
	buf8 = (uint8_t*)malloc(g_NfAttrib.PageSize + g_NfAttrib.SprSize);
	if(buf8 == NULL)
	{
		(void)printf("	out of memory: buf8 is NULL\n");
		ret = SPINF_OTPR_BUFFER_NULL;
		goto r_end;
	}	
	memset(buf8, 0, g_NfAttrib.PageSize + g_NfAttrib.SprSize);
	
	res = spiNf_SetOtpEnableBit(1);
	gOtpEnable = 1;
	if(res != 0)
	{
		//0: success, 1:Not Support OTP, others: fault
		(void)printf("	Enable OTP bit Error, R=%x\n", res);
		ret = SPINF_OTPR_NOT_SUPPORT;
		goto r_end;
	}
	
	idx = LL_RP_DATA;
	res = SpiNf_ReadPart(0, otpPg, buf8, idx);
	if(res == LL_OK)
	{
		//(void)printf("It Can Read OTP page %x: r=%x\n", 1, res);
		memcpy(&dBuf[0], &buf8[0], otp_size);
	}
	else
	{
		if(res == LL_ERASED)
		{
			(void)printf("	OTP page is empty, r=%x\n", res);
			memcpy(&dBuf[0], &buf8[0], otp_size);
			//ret = SPINF_OTPR_EMPTY;	//return OK if empty
		}
		else
		{
			(void)printf("	OTP page ECC error, r=%x\n", res);
			ret = SPINF_OTPR_ECC_ERROR;
		}
	}
	gOtpEnable = 0;
	res = spiNf_SetOtpEnableBit(0);
	if(res > 1)
	{
		//0: success, 1:Not Support OTP, others: fault
		(void)printf("	Disable OTP bit Error, R=%x\n", res);
		/* NAND will not work normally if OTP_EN bit is Error */
		ret = SPINF_OTPR_BUS_ERROR;
		gOtpEnable = 1;
		gOtpWriteProtection = 1;
	}

r_end:
	
	if(buf8 != NULL)
	{
		free(buf8);
	}
	
	if(ret)
	{
		(void)printf("	Read OTP FAIL, r=%x \n",ret);
	}
	
	return (uint8_t)ret;
}
	
void
SpiNf_SetSpiCsCtrl (
    uint32_t csCtrl)
{
    spiNf_SetSpiCsCtrl(csCtrl);
}
