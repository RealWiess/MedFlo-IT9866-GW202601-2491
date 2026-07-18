#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "littlefs/lfs.h"
#include "lfs_flash_interface.h"
#include "ite/itp.h"
#include "nor/mmp_nor.h"
#ifdef CFG_NAND_ENABLE
#include "ite/ite_nand.h"
#endif
#if defined(CFG_SD0_PARTITION0) || defined(CFG_SD1_PARTITION0)
#include "ite/ite_sd.h"
static unsigned char erase_data[65536] = {0};
#endif

/*#ifdef CFG_NOR_USE_DPUAES
static uint8_t  cipher_key[16] = {
    0xff, 0x00, 0x11, 0x22,
    0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa,
    0xbb, 0xcc, 0xdd, 0xee
};
#endif*/

LFS_FLASH_INFO* lfs_flash_constructor(uint32_t block_size, uint32_t page_size, uint32_t spare_size)
{
    LFS_FLASH_INFO *flash_Info = memalign(32, sizeof(LFS_FLASH_INFO));
    if (flash_Info == NULL)
    {
        printf("lfs_flash_constructor flash_Info memalign error!\n");
        return NULL;
    }
    flash_Info->blockSize = block_size;
    flash_Info->pageSize = page_size;
    flash_Info->spareSize = spare_size;
    flash_Info->pagesPerBlock = block_size / page_size;
    flash_Info->type = (uint32_t)ITH_DISK_MAX;
    flash_Info->pageBufferInner = (unsigned char *)malloc(page_size+spare_size+256);
    if (flash_Info->pageBufferInner == NULL)
    {
        printf("lfs_flash_constructor pageBufferInner memalign error!\n");
        free(flash_Info);
        return NULL;
    }
    return flash_Info;
}

void lfs_flash_destructor(LFS_FLASH_INFO* flash_info)
{
    free(flash_info->pageBufferInner);
    free(flash_info);
}

uint32_t lfs_flash_write_page(LFS_FLASH_INFO* flash_info, uint32_t block, uint32_t page, uint8_t *buffer, uint32_t count)
{
    uint32_t result = 0;
    int n = 0;
    int sector = 0;

    switch (flash_info->type)
    {
    case ITP_DISK_SD0:
#if defined(CFG_SD0_PARTITION0)
        sector = block * flash_info->pagesPerBlock + page;
        if (iteSdWriteMultipleSectorEx(SD_0, sector, count, buffer))
        {
            printf("[X] %s iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__, sector, count);
            result = 1;
            goto end;
        }
#else
        printf("%s type SD0 but CFG_SD0_PARTITION0 disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_SD1:
#if defined(CFG_SD1_PARTITION0)
        sector = block * flash_info->pagesPerBlock + page;
        if (iteSdWriteMultipleSectorEx(SD_1, sector, count, buffer))
        {
            printf("[X] %s iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__, sector, count);
            result = 1;
            goto end;
        }
#else
        printf("%s type SD1 but CFG_SD1_PARTITION0 disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_NAND:
#if defined(CFG_NAND_ENABLE)
        while (count > 0)
        {
            n = iteNfWrite(block, page, buffer, flash_info->oob);
            if (n != true)
            {
                printf("[X] %s iteNfWrite(%d, %d)\n", __FUNCTION__, block, page);
            }
            page++;
            count--;
            buffer += flash_info->pageSize;
        }
#else
        printf("%s type Nand but CFG_NAND_ENABLE disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_NOR:
#if defined(CFG_NOR_ENABLE)
    /*#ifdef CFG_NOR_USE_DPUAES
        pthread_mutex_lock(&DPU_AES_MUTEX);
        
        n = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);
        
        if (n == 0)
            n = NorWriteWithoutEraseAES(SPI_0, SPI_CSN_0, block * flash_info->blockSize + page * flash_info->pageSize,
                    buffer, flash_info->pageSize);
        
        pthread_mutex_unlock(&DPU_AES_MUTEX);
        
        if (n)
        {
            printf("[X] %s NorWriteWithoutErase(%d, %d)\n", __FUNCTION__,
                    block * flash_info->blockSize + page * flash_info->pageSize,
                    flash_info->pageSize);
            result = 1;
            goto end;
        }
    #else*/
        n = NorWriteWithoutErase(SPI_0, NOR_CSN, block * flash_info->blockSize + page * flash_info->pageSize,
                buffer, flash_info->pageSize * count);
        if (n)
        {
            printf("[X] %s NorWriteWithoutErase(%d, %d)\n", __FUNCTION__,
                    block * flash_info->blockSize + page * flash_info->pageSize,
                    flash_info->pageSize);
            result = 1;
            goto end;
        }
    /*#endif*/
#else
        printf("%s type Nor but CFG_NOR_ENABLE disable\n", __FUNCTION__);
#endif
        break;
    default:
        printf("%s unsupport flash: %d\n", __FUNCTION__, flash_info->type );
        return -1;
    }
end:
    return result;
}

uint32_t lfs_flash_read_block(LFS_FLASH_INFO* flash_info, uint32_t block, uint8_t *buffer)
{
    uint32_t result = 0;
    uint32_t count = 0;
    int n = 0;

    switch (flash_info->type)
    {
    case ITP_DISK_SD0:
#if defined(CFG_SD0_PARTITION0)
        if (iteSdReadMultipleSectorEx(SD_0, block * flash_info->pagesPerBlock, (int)flash_info->pagesPerBlock, buffer))
        {
            printf("[X] %s iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__, block * flash_info->pagesPerBlock,
                    (int)flash_info->pagesPerBlock);
            result = 1;
            goto end;
        }
#else
        printf("%s type SD0 but CFG_SD0_PARTITION0 disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_SD1:
#if defined(CFG_SD1_PARTITION0)
        if (iteSdReadMultipleSectorEx(SD_1, block * flash_info->pagesPerBlock, (int)flash_info->pagesPerBlock, buffer))
        {
            printf("[X] %s iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__, block * flash_info->pagesPerBlock,
                    (int)flash_info->pagesPerBlock);
            result = 1;
            goto end;
        }
#else
        printf("%s type SD1 but CFG_SD1_PARTITION0 disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_NAND:
#if defined(CFG_NAND_ENABLE)
        for (count = 0; count < flash_info->pagesPerBlock; count++)
        {
            n = iteNfRead(block, count, flash_info->pageBufferInner);
            if (n != true)
            {
                printf("[X] %s iteNfRead(%d, %d)\n", __FUNCTION__, block, count);
            }
            memcpy(buffer + count * flash_info->pageSize, flash_info->pageBufferInner, flash_info->pageSize);
        }
#else
        printf("%s type Nand but CFG_NAND_ENABLE disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_NOR:
#if defined(CFG_NOR_ENABLE)
    /*#ifdef CFG_NOR_USE_DPUAES
        pthread_mutex_lock(&DPU_AES_MUTEX);
        
        n = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);
        
        if (n == 0)
            n = NorReadAES(SPI_0, SPI_CSN_0, block * flash_info->blockSize, buffer, flash_info->blockSize);
        
        pthread_mutex_unlock(&DPU_AES_MUTEX);
        
        if (n)
        {
            printf("[X] %s NorReadAES(%d, %d)\n", __FUNCTION__, block * flash_info->blockSize,
                    flash_info->blockSize);
            result = 1;
            goto end;
        }
    #else*/
        n = NorRead(SPI_0, SPI_CSN_0, block * flash_info->blockSize, buffer, flash_info->blockSize);
        if (n)
        {
            printf("[X] %s NorRead(%d, %d)\n", __FUNCTION__, block * flash_info->blockSize,
                    flash_info->blockSize);
            result = 1;
            goto end;
        }
    /*#endif*/
#else
        printf("%s type Nor but CFG_NOR_ENABLE disable\n", __FUNCTION__);
#endif
        break;
    default:
        printf("%s unsupport flash: %d\n", __FUNCTION__, flash_info->type );
        return -1;
    }
end:
    return result;
}

uint32_t lfs_flash_erase_block(LFS_FLASH_INFO* flash_info, uint32_t block)
{
    uint32_t result = 0;
    int n = 0;
    int sector = 0;

    switch (flash_info->type)
    {
    case ITP_DISK_SD0:
#if defined(CFG_SD0_PARTITION0)
        sector = block * flash_info->pagesPerBlock;
        if (iteSdWriteMultipleSectorEx(SD_0, sector, flash_info->pagesPerBlock, erase_data))
        {
            printf("[X] %s iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__, sector, flash_info->pagesPerBlock);
            result = 1;
        }
#else
        printf("%s type SD0 but CFG_SD0_PARTITION0 disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_SD1:
#if defined(CFG_SD1_PARTITION0)
        sector = block * flash_info->pagesPerBlock;
        if (iteSdWriteMultipleSectorEx(SD_1, sector, flash_info->pagesPerBlock, erase_data))
        {
            printf("[X] %s iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__, sector, flash_info->pagesPerBlock);
            result = 1;
        }
#else
        printf("%s type SD1 but CFG_SD1_PARTITION0 disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_NAND:
#if defined(CFG_NAND_ENABLE)
        n = iteNfErase(block);
        if (n != true)
        {
            printf("[X] %s iteNfErase(%d)\n", __FUNCTION__, block);
        }
#else
        printf("%s type Nand but CFG_NAND_ENABLE disable\n", __FUNCTION__);
#endif
        break;
    case ITP_DISK_NOR:
#if defined(CFG_NOR_ENABLE)
        n = NorErase(SPI_0, NOR_CSN, block * flash_info->blockSize, flash_info->blockSize);
        if (n != true)
        {
            printf("[X] %s NorErase(%d)\n", __FUNCTION__, block);
            result = 1;
        }
#else
        printf("%s type Nor but CFG_NOR_ENABLE disable\n", __FUNCTION__);
#endif
        break;
    default:
        printf("%s unsupport flash: %d\n", __FUNCTION__, flash_info->type );
        return -1;
    }
    return result;
}