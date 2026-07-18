/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** LevelX Component                                                      */ 
/**                                                                       */
/**   NOR Flash driver                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary files.  */

#include "lx_api.h"
#include "fx_api.h"
#include <assert.h>
#include "nor/mmp_nor.h"
#include "ssp/mmp_spi.h"
#include "ite/itp.h"
/* Define constants for the NOR flash. */

/* This configuration is for one physical sector of overhead.  */

UINT  ite_lx_nor_flash_driver_initialize(LX_NOR_FLASH *nor_flash);
UINT  ite_lx_nor_flash_driver_erase_all(LX_NOR_FLASH *nor_flash);
#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  ite_lx_nor_flash_driver_read(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *destination, ULONG words);
UINT  ite_lx_nor_flash_driver_write(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *source, ULONG words);
UINT  ite_lx_nor_flash_driver_block_erase(LX_NOR_FLASH *nor_flash, ULONG block, ULONG erase_count);
UINT  ite_lx_nor_flash_driver_block_erased_verify(LX_NOR_FLASH *nor_flash, ULONG block);
UINT  ite_lx_nor_flash_driver_system_error(LX_NOR_FLASH *nor_flash, UINT error_code, ULONG block, ULONG sector);
#else
UINT  ite_lx_nor_flash_driver_read(ULONG *flash_address, ULONG *destination, ULONG words);
UINT  ite_lx_nor_flash_driver_write(ULONG *flash_address, ULONG *source, ULONG words);
UINT  ite_lx_nor_flash_driver_block_erase(ULONG block, ULONG erase_count);
UINT  ite_lx_nor_flash_driver_block_erased_verify(ULONG block);
UINT  ite_lx_nor_flash_driver_system_error(UINT error_code, ULONG block, ULONG sector);
#endif


UINT  ite_lx_nor_flash_driver_initialize(LX_NOR_FLASH *nor_flash)
{

    /* Setup the base address of the flash memory.  */
    FX_DRV_INFO*   nor_driver = (FX_DRV_INFO*)nor_flash->lx_media_driver_info;
    F_PHY phy;
    UINT ret = 0;
    nor_driver->driver->getphy(nor_driver->driver, &phy);
    nor_flash -> lx_nor_flash_base_address =                (ULONG*)(nor_driver->sectorStart * phy.bytes_per_sector);

    /* Setup geometry of the flash.  */
    nor_flash -> lx_nor_flash_words_per_block =             phy.sectors_per_cluster * phy.bytes_per_sector / sizeof(ULONG);
    nor_flash -> lx_nor_flash_total_blocks =                nor_driver->sectorNum / phy.sectors_per_cluster;
    
    /* Setup function pointers for the NOR flash services.  */
    nor_flash -> lx_nor_flash_driver_read =                 ite_lx_nor_flash_driver_read;
    nor_flash -> lx_nor_flash_driver_write =                ite_lx_nor_flash_driver_write;
    nor_flash -> lx_nor_flash_driver_block_erase =          ite_lx_nor_flash_driver_block_erase;
    nor_flash -> lx_nor_flash_driver_block_erased_verify =  ite_lx_nor_flash_driver_block_erased_verify;

    /* Setup local buffer for NOR flash operation. This buffer must be the sector size of the NOR flash memory.  */
    nor_flash -> lx_nor_flash_sector_buffer =  nor_driver->lxDriverInfo.wearBuf;
    
    if(nor_driver->lxDriverInfo.eraseReq)
    {
        ret = ite_lx_nor_flash_driver_erase_all(nor_flash);
        nor_driver->lxDriverInfo.eraseReq = 0;
    }
    else
    {
        unsigned long blockSize = phy.sectors_per_cluster * phy.bytes_per_sector;
        uint8_t     * checkBufA    = (uint8_t *)itpVmemAlignedAlloc(32, blockSize);
        uint8_t     * checkBufB    = (uint8_t *)itpVmemAlignedAlloc(32, blockSize);
        int i, j, diffCnt;
        for(i = 0 ; i < nor_flash->lx_nor_flash_total_blocks ; i ++)
        {
            NorRead(SPI_0, NOR_CSN, (phy.reserved_sectors + nor_driver->sectorStart + i * phy.sectors_per_cluster) * phy.bytes_per_sector, checkBufA, blockSize);
            NorRead(SPI_0, NOR_CSN, (phy.reserved_sectors + nor_driver->sectorStart + i * phy.sectors_per_cluster) * phy.bytes_per_sector, checkBufB, blockSize);
            diffCnt = 0;
            for(j = 0; j < blockSize ; j ++)
            {
                if(checkBufA[j]!=checkBufB[j])
                {
                    diffCnt++;
                    if(diffCnt > 1)
                        break;                   
                }
            }
            if(diffCnt > 1) //Occasionally there will be 1 bit error, which needs to be ignored.
            {
                printf("Block %X %X flip, erase\r\n", nor_driver->sectorStart, i);
                ret = nor_driver->driver->eraseblock(nor_driver->driver, i + nor_driver->sectorStart/phy.sectors_per_cluster);
                if(ret)
                {
                    printf("eraseblock error, block 0x%X start 0x%X, cluster 0x%X result 0x%X\r\n", 
                            i, nor_driver->sectorStart, phy.sectors_per_cluster, ret);
                }
            }
        }
        itpVmemFree((uint32_t)checkBufA);
        itpVmemFree((uint32_t)checkBufB);
    }
    /* Return success.  */
    return ret == 0? (LX_SUCCESS) : LX_ERROR;
}

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  ite_lx_nor_flash_driver_read(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *destination, ULONG words)
#else
UINT  ite_lx_nor_flash_driver_read(ULONG *flash_address, ULONG *destination, ULONG words)
#endif
{
    uint32_t norResult = 0;
    FX_DRV_INFO*   nor_driver = nor_flash->lx_media_driver_info;
#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nor_flash);
#endif
    if(!destination)
    {
        printf("_lx_nor_flash_driver_read buffer error\r\n");    // The known cause is that the external cache is too small
        return LX_ERROR;
    }
    
    norResult = nor_driver->driver->readbytes(nor_driver->driver, (void*)destination, ((unsigned long)flash_address ), words * sizeof(ULONG));
    return norResult == 0 ? (LX_SUCCESS) : LX_ERROR;
}


#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  ite_lx_nor_flash_driver_write(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *source, ULONG words)
#else
UINT  ite_lx_nor_flash_driver_write(ULONG *flash_address, ULONG *source, ULONG words)
#endif
{
    uint32_t norResult = 0;
    FX_DRV_INFO*   nor_driver = nor_flash->lx_media_driver_info;
#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nor_flash);
#endif
    norResult = nor_driver->driver->writebytes(nor_driver->driver, (void*)source, ((unsigned long)flash_address ), words * sizeof(ULONG));
       
    return norResult == 0 ? (LX_SUCCESS) : LX_ERROR;
}

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  ite_lx_nor_flash_driver_block_erase(LX_NOR_FLASH *nor_flash, ULONG block, ULONG erase_count)
#else
UINT  ite_lx_nor_flash_driver_block_erase(ULONG block, ULONG erase_count)
#endif
{

uint32_t norResult = 0;
FX_DRV_INFO*   nor_driver = nor_flash->lx_media_driver_info;
F_PHY phy;
#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nor_flash);
#endif
    LX_PARAMETER_NOT_USED(erase_count);
    nor_driver->driver->getphy(nor_driver->driver, &phy);
    norResult = nor_driver->driver->eraseblock(nor_driver->driver, block + nor_driver->sectorStart/phy.sectors_per_cluster);
    if(norResult)
    {
        printf("_lx_nor_flash_driver_block_erase error, block 0x%X start 0x%X, cluster 0x%X result 0x%X\r\n", 
                block, nor_driver->sectorStart, phy.sectors_per_cluster, norResult);
        return (LX_ERROR);
    }
    return(LX_SUCCESS);
}


UINT  ite_lx_nor_flash_driver_erase_all(LX_NOR_FLASH *nor_flash)
{

uint32_t norResult = 0;
ULONG block;
FX_DRV_INFO*   nor_driver = nor_flash->lx_media_driver_info;
F_PHY phy;
    
    nor_driver->driver->getphy(nor_driver->driver, &phy);
    for(block = 0 ; block < nor_flash->lx_nor_flash_total_blocks ; block ++)
    {
        norResult = nor_driver->driver->eraseblock(nor_driver->driver, block + nor_driver->sectorStart/phy.sectors_per_cluster);
        if(norResult)
        {
            printf("_lx_nor_flash_driver_erase_all error, block 0x%X start 0x%X, cluster 0x%X result 0x%X\r\n", 
                    block, nor_driver->sectorStart, phy.sectors_per_cluster, norResult);
            return (LX_ERROR);
        }
    }

    return(LX_SUCCESS);
}


#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  ite_lx_nor_flash_driver_block_erased_verify(LX_NOR_FLASH *nor_flash, ULONG block)
#else
UINT  ite_lx_nor_flash_driver_block_erased_verify(ULONG block)
#endif
{

ULONG   addr, sec;
ULONG   *wordPtr;
uint32_t norResult = 0, i;
//static uint8_t verifyBuf[128 * 128 * sizeof(ULONG)]; 
#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nor_flash);
#endif
FX_DRV_INFO*   nor_driver = nor_flash->lx_media_driver_info;
F_PHY phy;
    /* Determine if the block is completely erased.  */
    nor_driver->driver->getphy(nor_driver->driver, &phy);
    for(sec = 0 ; sec < phy.sectors_per_cluster ; sec ++)
    {
        addr = (nor_driver->sectorStart + block * phy.sectors_per_cluster + sec) * phy.bytes_per_sector;
        norResult = nor_driver->driver->readbytes(nor_driver->driver, (void*)nor_flash->lx_nor_flash_sector_buffer, addr, phy.bytes_per_sector);
        if(norResult)
        {
            printf("_lx_nor_flash_driver_block_erased_verify read error, block 0x%X addr 0x%X, size 0x%X result 0x%X\r\n", block, addr, phy.bytes_per_sector, norResult);
            return (LX_ERROR);
        }
        wordPtr = nor_flash->lx_nor_flash_sector_buffer;
        for(i = 0 ; i < phy.bytes_per_sector / sizeof(ULONG) ; i ++)
        {
            if (*wordPtr++ != 0xFFFFFFFF)
            {
                return(LX_ERROR);
            }
        }
    }
    /* Return success.  */
    return(LX_SUCCESS);
}

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  ite_lx_nor_flash_driver_system_error(LX_NOR_FLASH *nor_flash, UINT error_code, ULONG block, ULONG sector)
#else
UINT  ite_lx_nor_flash_driver_system_error(UINT error_code, ULONG block, ULONG sector)
#endif
{

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nor_flash);
#endif
    LX_PARAMETER_NOT_USED(error_code);
    LX_PARAMETER_NOT_USED(block);
    LX_PARAMETER_NOT_USED(sector);

    /* Custom processing goes here...  all errors are fatal.  */
    return(LX_ERROR);
}
