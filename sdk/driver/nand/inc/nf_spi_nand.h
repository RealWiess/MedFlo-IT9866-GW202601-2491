#ifndef NF_SPI_NAND__H
#define NF_SPI_NAND__H

#include "configs.h"
#include "ite/mmp_types.h"
#include "spinfdrv.h"


#ifdef NF_HW_ECC

#ifdef __cplusplusxx
extern "C" {
#endif

/********************************************************************
 * 
 ********************************************************************/
 /*
#define	SPINF_ERROR_READ_ID_ERR			1
#define	SPINF_ERROR_GET_FEATURE_ERR		2
#define	SPINF_ERROR_SET_FEATURE_ERR		3

#define	SPINF_ERROR_SEND_RD_CMD1_ERR	4
#define	SPINF_ERROR_SEND_CMD_0X13_ERR	5
#define	SPINF_ERROR_READ_PAGE_ERR		6

#define	SPINF_ERROR_CMD_WT_EN_ERR		7
#define	SPINF_ERROR_CMD_ERS_ERR			8
*/

/********************************************************************
 * enum
 ********************************************************************/
typedef enum {
    SPINF_OTPW_OK = 0,
    SPINF_OTPW_WP = 1,
    SPINF_OTPW_BUS_ERROR = 2,
    SPINF_OTPW_NOT_SUPPORT = 3,
    SPINF_OTPW_BUFFER_NULL = 4,
    SPINF_OTPW_SIZE_ERROR = 5,
    SPINF_OTPW_NOT_EMPTY = 6,
    SPINF_OTPW_WRITE_FAIL = 7
} SPINF_OTPW_STATUS;

typedef enum {
    SPINF_OTPR_OK = 0,
    SPINF_OTPR_FAIL = 1,
    SPINF_OTPR_BUS_ERROR = 2,
    SPINF_OTPR_NOT_SUPPORT = 3,
    SPINF_OTPR_BUFFER_NULL = 4,
    SPINF_OTPR_SIZE_ERROR = 5,
    SPINF_OTPR_EMPTY = 6,
    SPINF_OTPR_ECC_ERROR = 7
} SPINF_OTPR_STATUS;

/********************************************************************
 * public function
 ********************************************************************/
void SpiNf_GetAttribute(MMP_BOOL *NfType, MMP_BOOL *EccStatus);
void SpiNf_Getwear(uint32_t blk, uint8_t *dest);

uint8_t SpiNf_Initialize(uint32_t* pNumOfBlocks, uint8_t* pPagePerBlock, uint32_t* pPageSize);
uint8_t SpiNf_Read(uint32_t pba, uint32_t ppo, uint8_t* pBuf);
uint8_t SpiNf_ReadOneByte(uint32_t pba, uint32_t ppo, uint8_t offset, uint8_t* pByte);
uint8_t SpiNf_ReadPart(uint32_t pba, uint32_t ppo, uint8_t* pBuf, uint8_t index);
uint8_t SpiNf_Write(uint32_t pba, uint32_t ppo, uint8_t* pDtBuf, uint8_t* pSprBuf);
uint8_t SpiNf_WriteDouble(uint32_t pba, uint32_t ppo, uint8_t* pBuf0, uint8_t* pBuf1);
uint8_t SpiNf_Erase(uint32_t pba);

uint8_t SpiNf_IsBadBlock(uint32_t pba);
uint8_t SpiNf_IsBadBlockForBoot(uint32_t pba);

uint8_t SpiNf_LBA_Read(uint32_t pba, uint32_t ppo, uint8_t* pBuf);
uint8_t SpiNf_LBA_Write(uint32_t pba, uint32_t ppo, uint8_t* pDtBuf, uint8_t* pSprBuf);

MMP_UINT8 SpiNf_SetReservedBlocks(unsigned int revBlks);
void SpiNf_SetSpiCsCtrl(uint32_t csCtrl);
void spiNf_Reset(void);
void SpiNf_SwitchToDie0InISR();
uint8_t SpiNf_SwitchToDie0();

uint8_t SpiNf_WriteOtp(uint8_t *dBuf, uint32_t otp_size);
uint8_t SpiNf_ReadOtp(uint8_t *dBuf, uint32_t otp_size);

#ifdef __cplusplusxx
}
#endif

#endif  // End #ifdef NF_HW_ECC

#endif
