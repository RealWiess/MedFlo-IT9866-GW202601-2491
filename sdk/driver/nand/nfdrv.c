#include "../ith/ith_cfg.h"

#include "inc/configs.h"
#include "inc/nfdrv.h"
#include "inc/spinfdrv.h"

#ifdef CFG_SPI_NAND

    #include "inc/nf_spi_nand.h"

    #define NFInitialize        SpiNf_Initialize
    #define NFGetwear           SpiNf_Getwear
    #define NFErase             SpiNf_Erase
    #define NFWrite             SpiNf_Write
    #define NFLbaWrite          SpiNf_LBA_Write
    #define NFWriteDouble       SpiNf_WriteDouble
    #define NFRead              SpiNf_Read
    #define NFLbaRead           SpiNf_LBA_Read
    #define NFReadPart          SpiNf_ReadPart
    #define NFIsBadBlock        SpiNf_IsBadBlock
    #define NFIsBadBlockForBoot SpiNf_IsBadBlockForBoot
    #define NFReadOneByte       SpiNf_ReadOneByte
    #define NFGetAttribute      SpiNf_GetAttribute
    #define NFSetreservedblocks SpiNf_SetReservedBlocks
    #define NFSetSpiCsCtrl      SpiNf_SetSpiCsCtrl
#else

    #include "inc/nf_lb_hwecc.h"

    #ifdef NF_LARGEBLOCK_2KB

        #define NFInitialize        LB_HWECC_Initialize
        #define NFGetwear           LB_HWECC_Getwear
        #define NFErase             LB_HWECC_Erase
        #define NFWrite             LB_HWECC_Write
        #define NFLbaWrite          LB_HWECC_LBA_Write
        #define NFWriteDouble       LB_HWECC_WriteDouble
        #define NFRead              LB_HWECC_Read
        #define NFLbaRead           LB_HWECC_LBA_Read
        #define NFReadPart          LB_HWECC_ReadPart
        #define NFIsBadBlock        LB_HWECC_IsBadBlock
        #define NFIsBadBlockForBoot LB_HWECC_IsBadBlockForBoot
        #define NFReadOneByte       LB_HWECC_ReadOneByte
        #define NFGetAttribute      LB_HWECC_GetAttribute
        #define NFSetSpiCsCtrl      LB_HWECC_SetSpiCsCtrl
    #endif  //#ifdef NF_LARGEBLOCK_8KB
#endif

#define ithStorgeNand               ITH_STOR_NAND

#ifdef CFG_SPI_NAND_USE_AXISPI
    #define USE_ITH_STOR_MUTEX
#endif

#ifdef CFG_SPI_NAND
    #define NFSetreservedblocks     SpiNf_SetReservedBlocks
#else
    #ifdef  NF_PITCH_RESERVED_BLOCK_ISSUE
        #define NFSetreservedblocks LB_HWECC_Setreservedblocks
    #endif
#endif
////////////////////////////////////////////////////////////
//
//  Global Variable
//
////////////////////////////////////////////////////////////
static void * NF_Semaphore = MMP_NULL;

////////////////////////////////////////////////////////////
//
//  Const variable or Marco
//
////////////////////////////////////////////////////////////
#define NFDRV_ECC_THRESHOLD_VALUE (4)

enum {
    LL_OK,
    LL_ERASED,
    LL_ERROR,
    LL_REWRITE,
    LL_RETRY
};

extern NF_ECC_INFO gEccInf;

extern uint8_t SpiNf_WriteOtp(uint8_t *dBuf, uint32_t otp_size);
extern uint8_t SpiNf_ReadOtp(uint8_t *dBuf, uint32_t otp_size);
////////////////////////////////////////////////////////////
//
//  Function Implementation
//
////////////////////////////////////////////////////////////

void
nfLockMutex (
    void * mtx)
{
#ifdef USE_ITH_STOR_MUTEX
    ithLockMutex(ithStorMutex);
#endif
}

void
nfUnlockMutex (
    void * mtx)
{
#ifdef USE_ITH_STOR_MUTEX
    ithUnlockMutex(ithStorMutex);
#endif
}

MMP_UINT8
mmpNFInitialize (
    unsigned long   * pNumOfBlocks,
    unsigned char   * pPagePerBlock,
    unsigned long   * PageSize)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = NFInitialize(pNumOfBlocks, pPagePerBlock, PageSize);

    nfUnlockMutex(ithStorMutex);

    return result;
}

void
mmpNFGetwear (
    MMP_UINT32  blk,
    MMP_UINT8   * dest)
{
    NFGetwear(blk, dest);
}

#ifdef  NF_PITCH_RESERVED_BLOCK_ISSUE

MMP_UINT8
mmpNFSetreservedblocks (
    unsigned int blk)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    result = NFSetreservedblocks(blk);

    nfUnlockMutex(ithStorMutex);

    return result;
}
#endif

MMP_UINT8
mmpNFErase (
    MMP_UINT32 pba)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = NFErase(pba);

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFWrite (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   * pDataBuffer,
    MMP_UINT8   * pSpareBuffer)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = NFWrite(pba, ppo, pDataBuffer, pSpareBuffer);

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFWriteDouble (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   * buffer0,
    MMP_UINT8   * buffer1)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = NFWriteDouble(pba, ppo, buffer0, buffer1);

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFRead (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   * buffer)
{
    MMP_UINT8   result, RetryCount = 0;
    NF_ECC_INFO ecc;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    do
    {
        result = NFRead(pba, ppo, buffer);

        // (void)printf("mmpR:R=%x.\r\n",result);

        RetryCount++;
        if (RetryCount == 3)
        {
            (void)printf("mmpR:R=ERROR\r\n");
            result = LL_ERROR;
            break;
        }
    } while (result == LL_RETRY);

    if (gEccInf.enableEccBit)
    {
        if ((gEccInf.block == pba) && (gEccInf.page == ppo))
        {
            uint8_t tmp = 0;

            if (gEccInf.EccErrThreshold)
            {
                tmp = gEccInf.EccErrThreshold;
            }
            else
            {
                tmp = NFDRV_ECC_THRESHOLD_VALUE;
            }

            if (gEccInf.EccErrBit > (tmp - 2))
            {
                // (void)printf("mR1:(%x, %x)(%x,%x)\n",pba,ppo,gEccInf.enableEccBit,gEccInf.EccErrBit);
                if (gEccInf.EccErrBit >= tmp)
                {
                    (void)printf("	mR1.reWt: %x, %x, r=%x\n", gEccInf.enableEccBit, gEccInf.EccErrBit, result);
					if(result != LL_ERROR)
					{
						result = LL_REWRITE;
					}
                }
            }
        }
        else
        {
            (void)printf("mR1.err:(%x, %x)(%x,%x)\n", pba, ppo, gEccInf.enableEccBit, gEccInf.EccErrBit);
            while (1)
            {
            }
        }
    }

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFReadEcc (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   * buffer,
    int *ecc_result)
{
    MMP_UINT8   result, RetryCount = 0;
    NF_ECC_INFO ecc;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    do
    {
        result = NFRead(pba, ppo, buffer);

        RetryCount++;
        if (RetryCount == 3)
        {
            (void)printf("mmpR:R=ERROR\r\n");
            result = LL_ERROR;
            break;
        }
    } while (result == LL_RETRY);

    if (gEccInf.enableEccBit)
    {
        if ((gEccInf.block == pba) && (gEccInf.page == ppo))
        {
            uint8_t tmp = 0;

            if (gEccInf.EccErrThreshold)
            {
                tmp = gEccInf.EccErrThreshold;
            }
            else
            {
                tmp = NFDRV_ECC_THRESHOLD_VALUE;
            }

            if (gEccInf.EccErrBit > (tmp - 2))
            {
                // (void)printf("mR1:(%x, %x)(%x,%x)\n",pba,ppo,gEccInf.enableEccBit,gEccInf.EccErrBit);
                if (gEccInf.EccErrBit >= tmp)
                {
                    (void)printf("	mR2.reWt: %x, %x, r=%x\n", gEccInf.enableEccBit, gEccInf.EccErrBit, result);
					if(result != LL_ERROR)
					{
						result = LL_REWRITE;
					}
                }
            }            
            *ecc_result = gEccInf.EccErrBit;
        }
        else
        {
            (void)printf("mR1.err:(%x, %x)(%x,%x)\n", pba, ppo, gEccInf.enableEccBit, gEccInf.EccErrBit);
            while (1)
            {
            }
        }
    }
    else
        *ecc_result = 0;

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFReadPart (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   * buffer,
    MMP_UINT8   index)
{
    MMP_UINT8 result, RetryCount = 0;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    do
    {
        result = NFReadPart(pba, ppo, buffer, index);

        // (void)printf("mmpRp:R=%x.\r\n",result);

        RetryCount++;
        if (RetryCount == 3)
        {
            (void)printf("mmpR:Rp=ERROR\r\n");
            result = LL_ERROR;
            break;
        }
    } while (result == LL_RETRY);

    if (gEccInf.enableEccBit)
    {
        if ((gEccInf.block == pba) && (gEccInf.page == ppo))
        {
            uint8_t tmp = 0;

            if (gEccInf.EccErrThreshold)
            {
                tmp = gEccInf.EccErrThreshold;
            }
            else
            {
                tmp = NFDRV_ECC_THRESHOLD_VALUE;
            }

            if (gEccInf.EccErrBit > (tmp - 2))
            {
                // (void)printf("mR2:(%x, %x, %x)(%x, %x)\n",pba,ppo,index,gEccInf.enableEccBit,gEccInf.EccErrBit);
                if (gEccInf.EccErrBit >= tmp)
                {
                    (void)printf("	mR3.reWt: %x, %x, r=%x\n", gEccInf.enableEccBit, gEccInf.EccErrBit, result);
                    if (index != 0xFF)
                    {
						if(result != LL_ERROR)
						{
							result = LL_REWRITE;                    /* skip read spare case */
						}
                    }
                }
            }
        }
        else
        {
            (void)printf("mR2.err:(%x, %x, %x)(%x, %x)\n", pba, ppo, index, gEccInf.enableEccBit, gEccInf.EccErrBit);
            while (1)
            {
            }
        }
    }

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFIsBadBlock (
    MMP_UINT32 pba)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = NFIsBadBlock(pba);

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFIsBadBlockForBoot (
    MMP_UINT32 pba)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = NFIsBadBlockForBoot(pba);

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFReadOneByte (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   sparepos,
    MMP_UINT8   * ch)
{
    MMP_UINT8 result, RetryCount = 0;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    do
    {
        result = NFReadOneByte(pba, ppo, sparepos, ch);

        // (void)printf("mmpR1b:R=%x,RT=%x.\r\n",result,LL_RETRY);

        RetryCount++;
        if (RetryCount == 3)
        {
            (void)printf("mmpR1b:R=ERROR\r\n");
            result = LL_ERROR;
            break;
        }
    } while (result == LL_RETRY);

    if (gEccInf.enableEccBit)
    {
        if ((gEccInf.block == pba) && (gEccInf.page == ppo))
        {
            if (gEccInf.EccErrBit > 2)
            {
                // (void)printf("mR3:(%x, %x, %x)(%x, %x)\n",pba,ppo,sparepos,gEccInf.enableEccBit,gEccInf.EccErrBit);
                if (gEccInf.EccErrBit > NFDRV_ECC_THRESHOLD_VALUE)
                {
                    (void)printf("	mR3.reWt: %x, %x\n", gEccInf.enableEccBit, gEccInf.EccErrBit);
                }
            }
        }
        else
        {
            (void)printf("mR3.err:(%x, %x, %x)(%x, %x)\n", pba, ppo, sparepos, gEccInf.enableEccBit, gEccInf.EccErrBit);
            while (1)
            {
            }
        }
    }

    nfUnlockMutex(ithStorMutex);

    return result;
}

void
mmpNFGetAttribute (
    MMP_BOOL    * NfType,
    MMP_BOOL    * EccStatus)
{
    NFGetAttribute(NfType, EccStatus);
}

MMP_UINT8
mmpNFLbaRead (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   * buffer)
{
    MMP_UINT8 result, RetryCount = 0;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    do
    {
        result = NFLbaRead(pba, ppo, buffer);

        // (void)printf("mmpR:R=%x.\r\n",result);

        RetryCount++;
        if (RetryCount == 3)
        {
            (void)printf("mmpR:R=ERROR\r\n");
            result = LL_ERROR;
            break;
        }
    } while (result == LL_RETRY);

    nfUnlockMutex(ithStorMutex);

    return result;
}

MMP_UINT8
mmpNFLbaWrite (
    MMP_UINT32  pba,
    MMP_UINT32  ppo,
    MMP_UINT8   * pDataBuffer,
    MMP_UINT8   * pSpareBuffer)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = NFLbaWrite(pba, ppo, pDataBuffer, pSpareBuffer);

    nfUnlockMutex(ithStorMutex);

    return result;
}

DLLAPI void
mmpEnableSpiCsCtrl (
    MMP_UINT32 EnCtrl)
{
    NFSetSpiCsCtrl(EnCtrl);
}

DLLAPI MMP_INT32
mmpNfSetWtProtect (
    MMP_UINT32  blk,
    MMP_UINT32  bNum,
    MMP_UINT8   mod)
{
    MMP_INT32 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = (MMP_INT32)spiNf_SetWtProtect(blk, bNum, mod);

    nfUnlockMutex(ithStorMutex);

    return result;
}

DLLAPI void
mmpNfSwitchToDie0InISR ()
{
    spiNf_SwitchToDie0InISR();
}

DLLAPI MMP_UINT8
mmpNfSwitchToDie0 ()
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = (MMP_UINT8)SpiNf_SwitchToDie0();

    nfUnlockMutex(ithStorMutex);

    return result;
}

DLLAPI MMP_UINT8
mmpNfReadId (
    MMP_UINT8 * id)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = spiNf_ReadId(id);

    nfUnlockMutex(ithStorMutex);

    return result;
}

DLLAPI MMP_UINT8
mmpNfDummyReadId (
    MMP_UINT8 * id)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = spiNf_DummyReadId(id);

    nfUnlockMutex(ithStorMutex);

    return result;
}

DLLAPI MMP_UINT8
mmpNfReadOtp (
	MMP_UINT8   * pDataBuffer,
    MMP_UINT32  size)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = SpiNf_ReadOtp(pDataBuffer, size);

    nfUnlockMutex(ithStorMutex);

    return result;
}


DLLAPI MMP_UINT8
mmpNfWriteOtp (
	MMP_UINT8   * pDataBuffer,
    MMP_UINT32  size)
{
    MMP_UINT8 result;

    nfLockMutex(ithStorMutex);

    ithStorageSelect(ithStorgeNand);

    result = SpiNf_WriteOtp(pDataBuffer, size);

    nfUnlockMutex(ithStorMutex);

    return result;
}

DLLAPI MMP_UINT8
mmpNfGetEccThr (void)
{
    MMP_UINT8 tmp;
    if (gEccInf.EccErrThreshold)
    {
        tmp = gEccInf.EccErrThreshold;
    }
    else
    {
        tmp = NFDRV_ECC_THRESHOLD_VALUE;
    }
    return tmp;
}