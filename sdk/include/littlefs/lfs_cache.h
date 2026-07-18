#ifndef LFS_CACHE_H
#define LFS_CACHE_H

#include <stdint.h>
#include "lfs_flash_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LFS_CACHE_PAGE_ENTRY_TAG
{
    uint32_t writeLastAccessTime;
    uint32_t dirty;
} LFS_CACHE_PAGE_ENTRY;

typedef struct LFS_CACHE_ENTRY_TAG
{
    uint32_t blockNumber;
    uint32_t lastAccessTime;
    LFS_CACHE_PAGE_ENTRY *pageEntry;
    uint32_t dirty;
    uint32_t writeLastAccessTime;
    uint32_t eraseDirty;
    uint8_t *blockBuffer;
} LFS_CACHE_ENTRY;

typedef struct LFS_CACHE_TAG
{
    uint32_t blocksPerCache;
    LFS_CACHE_ENTRY *cacheEntry;
    LFS_FLASH_INFO *flashInfo;
} LFS_CACHE;

extern uint32_t lfs_cache_constructor(uint32_t cache_size, uint32_t page_size, uint32_t block_size, uint32_t spare_size, LFS_CACHE **lfs_cache);
extern uint32_t lfs_cache_destructor(LFS_CACHE **lfs_cache);
extern uint32_t lfs_cache_read_page(uint32_t block_index, uint32_t page_index, uint32_t dst_buf, uint32_t count, uint32_t flash_type, LFS_CACHE *lfs_cache);
extern uint32_t lfs_cache_write_page(uint32_t block_index, uint32_t page_index, uint32_t src_buf, uint32_t count, uint32_t flash_type, LFS_CACHE *lfs_cache);
extern uint32_t lfs_cache_erase_block(uint32_t block_index, uint32_t flash_type, LFS_CACHE *lfs_cache);
extern uint32_t lfs_cache_flush(LFS_CACHE *lfs_cache);
extern void  lfs_cache_set_flashType(uint32_t flash_type, LFS_CACHE *lfs_cache);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
