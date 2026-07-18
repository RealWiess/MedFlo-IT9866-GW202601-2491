#ifndef LFS_FLASH_INTERFACE_H
#define LFS_FLASH_INTERFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct lfs_flash_info
{
    uint32_t pageSize;
    uint32_t blockSize;
    uint32_t spareSize;
    uint32_t pagesPerBlock;
    uint32_t type;
    uint8_t  *pageBufferInner;
    uint8_t  oob[24];
} LFS_FLASH_INFO;

LFS_FLASH_INFO* lfs_flash_constructor(uint32_t block_size, uint32_t page_size, uint32_t spare_size);
void lfs_flash_destructor(LFS_FLASH_INFO* flash_info);
uint32_t lfs_flash_write_page(LFS_FLASH_INFO* flash_info, uint32_t block, uint32_t page, uint8_t *buffer, uint32_t count);
uint32_t lfs_flash_read_block(LFS_FLASH_INFO* flash_info, uint32_t block, uint8_t *buffer);
uint32_t lfs_flash_erase_block(LFS_FLASH_INFO* flash_info, uint32_t block);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
