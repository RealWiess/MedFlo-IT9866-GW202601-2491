/*
 * Copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 *  NAND flash extern API implementation.
 *
 * @author Joseph
 */

// =============================================================================
//                              Include Files
// =============================================================================
// #include "config.h"
#include "ite/ith.h"
#include "inc/nfdrv.h"
#include "../include/ite/ite_nand.h"

// =============================================================================
//                              Constant Definition
// =============================================================================

#define     BOOT_ROM_RESERVED_SIZE CFG_NAND_RESERVED_SIZE       // 16MB(*2)=32MB(for real nand space, the most small nand size of 2kB/page is 128MB)

#define NF_API
// =============================================================================
//                              Structure Definition
// =============================================================================
enum {
    LL_OK,
    LL_ERASED,
    LL_ERROR,
    LL_REWRITE,
    LL_RETRY
};
// =============================================================================
//                              Private Function Declaration
// =============================================================================

// =============================================================================
//                              Global Data Definition
// =============================================================================

// =============================================================================
//                              Private Function Definition
// =============================================================================
#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
extern unsigned long g_ftl_reserved_size;
#endif

////////////////////////////////////////////////////////////
//
//  Function Implementation
//
////////////////////////////////////////////////////////////
NF_API void
iteNfGetAttribute (
    uint8_t * NfType,
    uint8_t * EccStatus)
{
    MMP_BOOL    type;
    MMP_BOOL    status;

    mmpNFGetAttribute(&type, &status);

    *NfType     = (uint8_t)type;
    *EccStatus  = (uint8_t)status;
}

NF_API void
iteNfGetwear (
    uint32_t    blk,
    uint8_t     * dest)
{
    mmpNFGetwear(blk, dest);
}

NF_API int
iteNfInitialize (
    ITE_NF_INFO * Info)
// iteNfInitialize(int *Info)
{
    int             result  = -1;
    unsigned long   blks    = 0;
    unsigned char   Pgs     = 0;
    unsigned long   PgSize  = 0;
    unsigned long   revBlks = 0;

    // mmpEnableSpiCsCtrl(Info->enSpiCsCtrl);  //**

    if (!mmpNFInitialize(&blks, &Pgs, &PgSize))
    {
        Info->NumOfBlk  = (uint32_t)blks;
        Info->PageInBlk = (uint32_t)Pgs;
        Info->PageSize  = (uint32_t)PgSize;

		#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
		if(g_ftl_reserved_size)
		{
		    Info->BootRomSize = g_ftl_reserved_size;
		}
		#endif
		
        if (Info->BootRomSize)
        {
            uint32_t _blkSize = (uint32_t)(Pgs * PgSize);
            if (_blkSize)
            {
                revBlks = Info->BootRomSize / _blkSize;
                if (Info->BootRomSize % _blkSize)
                {
                    revBlks++;
                }
            }
        }

        if (!mmpNFSetreservedblocks(revBlks))
        {
            uint8_t sprSize = 0;
            if (PgSize == 2048)
#ifdef CFG_SPI_NAND
            {
                Info->SprSize = 24;
            }
#else
            {
                Info->SprSize = 16;
            }
#endif
            else
            {
                Info->SprSize = 24;
            }

            iteNfGetwear(0x55AA55AA, &sprSize); // use this api to get spare size
            Info->AllSprSize = (uint32_t)sprSize * 4;

            // (void)printf("Got Spare size = %x, %x\n",Info->AllSprSize,sprSize);

            Info->Init  = 1;
            result      = true;
        }
    }

    return result;
}

NF_API int
iteNfRead (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   * buffer)
{
    int result = -1;
    int ret;
    ret = mmpNFRead(pba, ppo, buffer);
    if (ret != LL_ERROR)
    {
        if (ret != LL_REWRITE)
        {
            result = 1;
        }
        else
        {
            result = 2;
        }
    }

    return result;
}

NF_API int
iteNfReadEcc (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   * buffer,
    int *ecc_result)
{
    int result = -1;
    int ret;
    ret = mmpNFReadEcc(pba, ppo, buffer, ecc_result);
    if (ret != LL_ERROR)
    {
        if (ret != LL_REWRITE)
        {
            result = 1;
        }
        else
        {
            result = 2;
        }
    }

    return result;
}


NF_API int
iteNfReadOneByte (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   sparepos,
    unsigned char   * ch)
{
    int result = -1;

    if (mmpNFReadOneByte(pba, ppo, sparepos, ch) != LL_ERROR)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfReadPart (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   * buffer,
    unsigned char   index)
{
    int result = -1;

    if (mmpNFReadPart(pba, ppo, buffer, index) != LL_ERROR)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfWrite (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   * pDataBuffer,
    unsigned char   * pSpareBuffer)
{
    int result = -1;

    if (mmpNFWrite(pba, ppo, pDataBuffer, pSpareBuffer) == LL_OK)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfWriteDouble (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   * buffer0,
    unsigned char   * buffer1)
{
    int result = -1;

    if (mmpNFWriteDouble(pba, ppo, buffer0, buffer1) == LL_OK)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfErase (
    unsigned long pba)
{
    int result = -1;

    if (mmpNFErase(pba) == LL_OK)
    {
        result = true;
    }

    return result;
}

/**
 * @brief check if pba is a Bad block
 *
 * LL_OK is Good block, LL_ERROR is Bad Block
 * @return true if block is Good, -1 if block is BAD
 */
NF_API int
iteNfIsBadBlock (
    unsigned long pba)
{
    int result = -1;

    if (mmpNFIsBadBlock(pba) == LL_OK)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfIsBadBlockForBoot (
    unsigned long pba)
{
    int result = -1;

    if (mmpNFIsBadBlockForBoot(pba) == LL_OK)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfLbaRead (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   * buffer)
{
    int result = -1;

    if (mmpNFLbaRead(pba, ppo, buffer) == MMP_TRUE)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfLbaWrite (
    unsigned long   pba,
    unsigned long   ppo,
    unsigned char   * pDataBuffer,
    unsigned char   * pSpareBuffer)
{
    int result = -1;

    if (mmpNFLbaWrite(pba, ppo, pDataBuffer, pSpareBuffer) == LL_OK)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfTerminate (
    void)
{
    return true;
}

NF_API void
iteNfSwitchToDie0InISR ()
{
    mmpNfSwitchToDie0InISR();
}

/**
 * @brief Switch to die 0 if the nandflash is Winbond M02
 *
 * @return true if success, -1 otherwise
 */
NF_API int
iteNfSwitchToDie0 ()
{
    int result = -1;

    if (ithGetCpuMode() == ITH_CPU_SYS)
    {   // the CPU is in NORMAL running mode
        if (mmpNfSwitchToDie0() == LL_OK)
        {
            result = true;
        }
    }
    else
    {   // the CPU is in ISR or exception handler
        mmpNfSwitchToDie0InISR();
        result = true;
    }

    return result;
}

NF_API int
iteNfReadId (
    unsigned char * id)
{
    int result = -1;

    if (mmpNfReadId(id) == LL_OK)
    {
        result = true;
    }

    return result;
}

NF_API int
iteNfDummyReadId (
    unsigned char * id)
{
    int result = -1;

    if (mmpNfDummyReadId(id) == LL_OK)
    {
        result = true;
    }

    return result;
}

/**
 * @brief Set Write Protect for NAND Reserved Area
 *
 * @return true if WP success, -1 otherwise
 */
NF_API int
iteNfSetWtProtect (
    unsigned long   blk,
    unsigned long   bNum,
    unsigned char   mod)
{
    int result = -1;

    if (mmpNfSetWtProtect(blk, bNum, mod) == LL_OK)
    {
        result = true;
    }

    return result;
}

/**
 * @brief Read OTP data
 *
 * @return true if read OK, -1 otherwise
 */
NAND_API int iteNfReadOtp (
	unsigned char	*buf, 
	unsigned long	size)
{
    int result = -1;

    if (mmpNfReadOtp(buf, size) == LL_OK)
    {
        result = true;
    }

    return result;
}

/**
 * @brief Write OTP data
 *
 * @return true if write OK, -1 otherwise
 */
NAND_API int iteNfWriteOtp (
	unsigned char	*buf, 
	unsigned long	size)
{
    int result = -1;

    if (mmpNfWriteOtp(buf, size) == LL_OK)
    {
        result = true;
    }

    return result;
}

/**
 * @brief Get the ECC error threshold value for NAND
 *
 * @return ECC error threshold value
 *
 * @note The ECC error threshold value is used to determine the health 
 *         of a NAND block. If the number of ECC errors in a
 *         block is greater than or equal to the ECC error threshold value,
 *         then the block is considered bad.
 */
NAND_API int iteNfGetEccThr(void)
{
    return (int)mmpNfGetEccThr();
}