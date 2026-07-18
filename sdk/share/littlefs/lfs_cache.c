#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "lfs_cache.h"
#include "lfs_flash_interface.h"
#include "ite/itp.h"
#include "nor/mmp_nor.h"
#ifdef CFG_NAND_ENABLE
#include "ite/ite_nand.h"
#endif
#if defined(CFG_SD0_PARTITION0) || defined(CFG_SD1_PARTITION0)
#include "ite/ite_sd.h"
#endif

#define CACHE_FREE 0xFFFFFFFF
#define ACCESS_TIME_MAX 0xFFFFFFFF
#define FLASH_ERASE_DATA 0xFF

typedef enum CACHE_OPERATION_TAG
{
    CACHE_READ_PAGE,
    CACHE_WRITE_PAGE,
    CACHE_ERASE_BLOCK
} CACHE_OPERATION;

//static LFS_CACHE *g_lfs_cache = NULL;
static pthread_mutex_t lfsInstanceMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pageAccessMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t flushAccessMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static uint32_t accessCounter = 0;

static uint32_t accessTime(LFS_CACHE *lfs_cache)
{
    LFS_CACHE_ENTRY *cacheEntry = NULL;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t k = 0;
    uint32_t updateOperation = 0;
    uint32_t updateBlockIndex = CACHE_FREE;
    uint32_t updatePageIndex = CACHE_FREE;
    uint32_t oldAccessTime = ACCESS_TIME_MAX;
    uint8_t *clastAccessTimeRecord = NULL;
    uint8_t *cwriteLastAccessTimeRecord = NULL;
    uint8_t *pwriteLastAccessTimeRecord = NULL;

    accessCounter++;
    if (accessCounter == ACCESS_TIME_MAX)
    {
        pthread_mutex_lock(&flushAccessMutex);

        printf("[lfs_cache warning] accessTime overflow warning!\n");
        cacheEntry = lfs_cache->cacheEntry;
        accessCounter = 1;

        clastAccessTimeRecord = calloc(lfs_cache->blocksPerCache, 1);
        if (clastAccessTimeRecord == NULL)
        {
            printf("[lfs_cache error] clastAccessTimeRecord calloc error!\n");
            pthread_mutex_unlock(&flushAccessMutex);
            return accessCounter;
        }
        cwriteLastAccessTimeRecord = calloc(lfs_cache->blocksPerCache, 1);
        if (cwriteLastAccessTimeRecord == NULL)
        {
            printf("[lfs_cache error] cwriteLastAccessTimeRecord calloc error!\n");
            pthread_mutex_unlock(&flushAccessMutex);
            return accessCounter;
        }
        pwriteLastAccessTimeRecord = calloc(lfs_cache->blocksPerCache * lfs_cache->flashInfo->pagesPerBlock, 1);
        if (pwriteLastAccessTimeRecord == NULL)
        {
            printf("[lfs_cache error] pwriteLastAccessTimeRecord calloc error!\n");
            pthread_mutex_unlock(&flushAccessMutex);
            return accessCounter;
        }

        for (k = 0; k < lfs_cache->blocksPerCache * lfs_cache->flashInfo->pagesPerBlock + 2 * lfs_cache->blocksPerCache; k++)
        {
            updateBlockIndex = CACHE_FREE;
            updatePageIndex = CACHE_FREE;
            oldAccessTime = ACCESS_TIME_MAX;
            updateOperation = 0;
            for (i = 0; i < lfs_cache->blocksPerCache; i++)
            {
                if (clastAccessTimeRecord[i] == 0 && cacheEntry[i].lastAccessTime != 0 &&
                        oldAccessTime > cacheEntry[i].lastAccessTime)
                {
                    oldAccessTime = cacheEntry[i].lastAccessTime;
                    updateOperation = 1;
                    updateBlockIndex = i;
                }
                if (cwriteLastAccessTimeRecord[i] == 0 && cacheEntry[i].writeLastAccessTime != 0 &&
                        oldAccessTime > cacheEntry[i].writeLastAccessTime)
                {
                    oldAccessTime = cacheEntry[i].writeLastAccessTime;
                    updateOperation = 2;
                    updateBlockIndex = i;
                }
                for (j = 0; j < lfs_cache->flashInfo->pagesPerBlock; j++)
                {
                    if (pwriteLastAccessTimeRecord[i * lfs_cache->blocksPerCache + j] == 0 &&
                            cacheEntry[i].pageEntry[j].writeLastAccessTime != 0 &&
                            oldAccessTime > cacheEntry[i].pageEntry[j].writeLastAccessTime)
                    {
                        oldAccessTime = cacheEntry[i].pageEntry[j].writeLastAccessTime;
                        updateOperation = 3;
                        updateBlockIndex = i;
                        updatePageIndex = j;
                    }
                }
            }

            if (updateOperation == 0)
            {
                break;
            }
            else if (updateOperation == 1)
            {
                cacheEntry[updateBlockIndex].lastAccessTime = accessCounter++;
                clastAccessTimeRecord[updateBlockIndex] = 1;
            }
            else if (updateOperation == 2)
            {
                cacheEntry[updateBlockIndex].writeLastAccessTime = accessCounter++;
                cwriteLastAccessTimeRecord[updateBlockIndex] = 1;
            }
            else
            {
                cacheEntry[updateBlockIndex].pageEntry[updatePageIndex].writeLastAccessTime = accessCounter++;
                pwriteLastAccessTimeRecord[updateBlockIndex * lfs_cache->blocksPerCache + updatePageIndex] = 1;
            }
        }
        if (k == lfs_cache->blocksPerCache * lfs_cache->flashInfo->pagesPerBlock + 2 * lfs_cache->blocksPerCache)
            printf("[lfs_cache warning] accessTime overflow warning!(all time update)\n");

        free(clastAccessTimeRecord);
        free(cwriteLastAccessTimeRecord);
        free(pwriteLastAccessTimeRecord);

        pthread_mutex_unlock(&flushAccessMutex);
    }

    return accessCounter;
}

static void lfs_cache_find_utility_entry(uint32_t block_index,
        LFS_CACHE_ENTRY **blockEntry,
        LFS_CACHE_ENTRY **freeEntry,
        LFS_CACHE_ENTRY **oldEntry,
        LFS_CACHE *lfs_cache)
{
    LFS_CACHE_ENTRY *cacheEntry = NULL;
    uint32_t i = 0;
    uint32_t oldAccessTime = ACCESS_TIME_MAX;

    cacheEntry = lfs_cache->cacheEntry;

    for (i = 0; i < lfs_cache->blocksPerCache; i++)
    {
        if (cacheEntry[i].blockNumber == block_index)
        {
            *blockEntry = &cacheEntry[i];
            return;
        }
        else if (cacheEntry[i].blockNumber == CACHE_FREE)
        {
            *freeEntry = &cacheEntry[i];
            return;
        }

        if (oldAccessTime > cacheEntry[i].lastAccessTime)
        {
            oldAccessTime = cacheEntry[i].lastAccessTime;
            *oldEntry = &cacheEntry[i];
        }
    }
}

static uint32_t lfs_cache_flush_(LFS_CACHE *lfs_cache)
{
    uint32_t result = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t k = 0;
    uint32_t writeOperation = 0;
    uint32_t flushBlockIndex = CACHE_FREE;
    uint32_t flushPageIndex = CACHE_FREE;
    uint32_t oldAccessTime = ACCESS_TIME_MAX;
    uint32_t pageCount = 0;
    uint32_t pageCountIndex = CACHE_FREE;
    uint32_t pageCountAccessTime = ACCESS_TIME_MAX;
    LFS_CACHE_ENTRY *cacheEntry = NULL;

    pthread_mutex_lock(&flushAccessMutex);

    if (lfs_cache == NULL)
    {
        printf("lfs_cache_constructor has not created a instance!\n");
        result = 1;
        goto end;
    }

    cacheEntry = lfs_cache->cacheEntry;

    for (k = 0; k < lfs_cache->blocksPerCache * lfs_cache->flashInfo->pagesPerBlock + lfs_cache->blocksPerCache; k++)
    {
        flushBlockIndex = CACHE_FREE;
        flushPageIndex = CACHE_FREE;
        oldAccessTime = ACCESS_TIME_MAX;
        writeOperation = 0;
        for (i = 0; i < lfs_cache->blocksPerCache; i++)
        {
            if (cacheEntry[i].eraseDirty != 0)
            {
                if (oldAccessTime > cacheEntry[i].writeLastAccessTime)
                {
                    oldAccessTime = cacheEntry[i].writeLastAccessTime;
                    writeOperation = 1;
                    flushBlockIndex = i;
                }
            }
            if (cacheEntry[i].dirty != 0)
            {
                for (j = 0; j < lfs_cache->flashInfo->pagesPerBlock; j++)
                {
                    if (cacheEntry[i].pageEntry[j].dirty != 0)
                    {
                        if (oldAccessTime > cacheEntry[i].pageEntry[j].writeLastAccessTime)
                        {
                            oldAccessTime = cacheEntry[i].pageEntry[j].writeLastAccessTime;
                            writeOperation = 2;
                            flushBlockIndex = i;
                            flushPageIndex = j;
                        }
                    }
                }
            }
        }

        if (writeOperation == 0)
        {
            for (i = 0; i < lfs_cache->blocksPerCache; i++)
            {
                cacheEntry[i].dirty = 0;
            }
            goto end;
        }
        else if (writeOperation == 1)
        {
            if (lfs_flash_erase_block(lfs_cache->flashInfo, cacheEntry[flushBlockIndex].blockNumber) != 0)
            {
                printf("lfs_cache_flush_ error!(lfs_flash_erase_block error)\n");
                result = 1;
                goto end;
            }
            cacheEntry[flushBlockIndex].eraseDirty = 0;
        }
        else
        {
            pageCountAccessTime = oldAccessTime;
            pageCountIndex = flushPageIndex;
            pageCount = 0;
            while (pageCountAccessTime == cacheEntry[flushBlockIndex].pageEntry[pageCountIndex].writeLastAccessTime)
            {
                pageCount++;
                cacheEntry[flushBlockIndex].pageEntry[pageCountIndex].dirty = 0;
                pageCountIndex++;
                if (pageCountIndex == lfs_cache->flashInfo->pagesPerBlock)
                {
                    break;
                }
                pageCountAccessTime++;
                if (pageCountAccessTime == ACCESS_TIME_MAX)
                {
                    pageCountAccessTime = 1;
                }
            }
            if (lfs_flash_write_page(lfs_cache->flashInfo, cacheEntry[flushBlockIndex].blockNumber, flushPageIndex,
                    cacheEntry[flushBlockIndex].blockBuffer + flushPageIndex * lfs_cache->flashInfo->pageSize, pageCount) != 0)
            {
                printf("lfs_cache_flush_ error!(lfs_flash_write_page error)\n");
                result = 1;
                goto end;
            }
        }
    }

end:
    pthread_mutex_unlock(&flushAccessMutex);

    return result;
}

static LFS_CACHE_ENTRY *lfs_cache_find_entry(CACHE_OPERATION cache_operation,
        uint32_t page_index,
        uint32_t block_index,
        uint32_t page_count,
        LFS_CACHE *lfs_cache)
{
    LFS_CACHE_ENTRY *result = NULL;
    LFS_CACHE_ENTRY *blockEntry = NULL;
    LFS_CACHE_ENTRY *freeEntry = NULL;
    LFS_CACHE_ENTRY *oldEntry = NULL;
    uint32_t i = 0;

    if (cache_operation < CACHE_READ_PAGE || cache_operation > CACHE_ERASE_BLOCK)
    {
        printf("lfs_cache_find_entry cache_operation error!\n");
        result = NULL;
        goto end;
    }

    if (cache_operation == CACHE_READ_PAGE)
    {
        lfs_cache_find_utility_entry(block_index, &blockEntry, &freeEntry, &oldEntry, lfs_cache);

        if (blockEntry != NULL)
        {
            result = blockEntry;
            goto end;
        }

        if (freeEntry != NULL)
        {
            freeEntry->blockNumber = block_index;
            if (lfs_flash_read_block(lfs_cache->flashInfo, freeEntry->blockNumber, freeEntry->blockBuffer) != 0)
            {
                printf("lfs_cache_find_entry CACHE_READ_PAGE error!(lfs_flash_read_block error_0)\n");
                result = NULL;
                goto end;
            }
            result = freeEntry;
            goto end;
        }

        if (oldEntry != NULL)
        {
            if (oldEntry->dirty != 0 || oldEntry->eraseDirty != 0)
            {
                if (lfs_cache_flush_(lfs_cache) != 0)
                {
                    printf("lfs_cache_find_entry CACHE_READ_PAGE error!(flush error)\n");
                    result = NULL;
                    goto end;
                }
            }
            oldEntry->blockNumber = block_index;
            if (lfs_flash_read_block(lfs_cache->flashInfo, oldEntry->blockNumber, oldEntry->blockBuffer) != 0)
            {
                printf("lfs_cache_find_entry CACHE_READ_PAGE error!(lfs_flash_read_block error_1)\n");
                result = NULL;
                goto end;
            }
            result = oldEntry;
            goto end;
        }

        printf("lfs_cache_find_entry CACHE_READ_PAGE error!(oldEntry NULL)\n");
        result = NULL;
        goto end;
    }

    if (cache_operation == CACHE_WRITE_PAGE)
    {
        lfs_cache_find_utility_entry(block_index, &blockEntry, &freeEntry, &oldEntry, lfs_cache);

        if (blockEntry != NULL)
        {
            if (blockEntry->dirty != 0)
            {
                for (i = page_index; i < page_index + page_count; i++)
                {
                    if (blockEntry->pageEntry[i].dirty != 0)
                    {
                        if (lfs_cache_flush_(lfs_cache) != 0)
                        {
                            printf("lfs_cache_find_entry CACHE_WRITE_PAGE error!(flush error_0)\n");
                            result = NULL;
                            goto end;
                        }
                        break;
                    }
                }
            }
            result = blockEntry;
            goto end;
        }

        if (freeEntry != NULL)
        {
            freeEntry->blockNumber = block_index;
            if (lfs_flash_read_block(lfs_cache->flashInfo, freeEntry->blockNumber, freeEntry->blockBuffer) != 0)
            {
                printf("lfs_cache_find_entry CACHE_WRITE_PAGE error!(lfs_flash_read_block error_0)\n");
                result = NULL;
                goto end;
            }
            result = freeEntry;
            goto end;
        }

        if (oldEntry != NULL)
        {
            if (oldEntry->dirty != 0 || oldEntry->eraseDirty != 0)
            {
                if (lfs_cache_flush_(lfs_cache) != 0)
                {
                    printf("lfs_cache_find_entry CACHE_WRITE_PAGE error!(flush error_1)\n");
                    result = NULL;
                    goto end;
                }
            }
            oldEntry->blockNumber = block_index;
            if (lfs_flash_read_block(lfs_cache->flashInfo, oldEntry->blockNumber, oldEntry->blockBuffer) != 0)
            {
                printf("lfs_cache_find_entry CACHE_WRITE_PAGE error!(lfs_flash_read_block error_1)\n");
                result = NULL;
                goto end;
            }
            result = oldEntry;
            goto end;
        }

        printf("lfs_cache_find_entry CACHE_WRITE_PAGE error!(oldEntry NULL)\n");
        result = NULL;
        goto end;
    }

    if (cache_operation == CACHE_ERASE_BLOCK)
    {
        lfs_cache_find_utility_entry(block_index, &blockEntry, &freeEntry, &oldEntry, lfs_cache);
        if (blockEntry != NULL)
        {
            if (blockEntry->dirty != 0)
            {
                if (lfs_cache_flush_(lfs_cache) != 0)
                {
                    printf("lfs_cache_find_entry CACHE_ERASE_BLOCK error!(flush error_0)\n");
                    result = NULL;
                    goto end;
                }
            }
            result = blockEntry;
            goto end;
        }

        if (freeEntry != NULL)
        {
            freeEntry->blockNumber = block_index;
            result = freeEntry;
            goto end;
        }

        if (oldEntry != NULL)
        {
            if (oldEntry->dirty != 0 || oldEntry->eraseDirty != 0)
            {
                if (lfs_cache_flush_(lfs_cache) != 0)
                {
                    printf("lfs_cache_find_entry CACHE_ERASE_BLOCK error!(flush error_1)\n");
                    result = NULL;
                    goto end;
                }
            }
            oldEntry->blockNumber = block_index;
            result = oldEntry;
            goto end;
        }

        printf("lfs_cache_find_entry CACHE_ERASE_BLOCK error!(oldEntry NULL)\n");
        result = NULL;
        goto end;
    }

    printf("lfs_cache_find_entry cache_operation error!\n");
    result = NULL;
    goto end;

end:
    
    return result;
}

uint32_t lfs_cache_constructor(uint32_t cache_size, uint32_t page_size, uint32_t block_size, uint32_t spare_size, LFS_CACHE **lfs_cache)
{
    uint32_t result = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    LFS_CACHE *lfs_cache_temp = NULL;
    LFS_CACHE_ENTRY *cacheEntry = NULL;
    uint32_t pagesPerBlock = block_size / page_size;

    if (cache_size == 0 ||
            page_size == 0 ||
            block_size == 0 ||
            cache_size % block_size != 0 ||
            block_size % page_size != 0)
    {
        printf("lfs_cache_constructor parameter error!\n");
        result = 1;
        return result;
    }

    pthread_mutex_lock(&lfsInstanceMutex);

    if (*lfs_cache == NULL)
    {
        lfs_cache_temp = memalign(32, sizeof(LFS_CACHE));
        if (lfs_cache_temp == NULL)
        {
            printf("lfs_cache_constructor lfs_cache_temp memalign error!\n");
            result = 1;
            goto end;
        }
        lfs_cache_temp->blocksPerCache = cache_size / block_size;
        cacheEntry = memalign(32, sizeof(LFS_CACHE_ENTRY) * lfs_cache_temp->blocksPerCache);
        if (cacheEntry == NULL)
        {
            printf("lfs_cache_constructor cacheEntry memalign error!\n");
            result = 1;
            goto end;
        }
        for (i = 0; i < lfs_cache_temp->blocksPerCache; i++)
        {
            cacheEntry[i].blockNumber = CACHE_FREE;
            cacheEntry[i].lastAccessTime = 0;
            cacheEntry[i].writeLastAccessTime = 0;
            cacheEntry[i].dirty = 0;
            cacheEntry[i].eraseDirty = 0;
            cacheEntry[i].pageEntry = memalign(32, sizeof(LFS_CACHE_PAGE_ENTRY) * pagesPerBlock);
            if (cacheEntry[i].pageEntry == NULL)
            {
                printf("lfs_cache_constructor pageEntry memalign error!\n");
                result = 1;
                goto end;
            }
            for (j = 0; j < pagesPerBlock; j++)
            {
                cacheEntry[i].pageEntry[j].writeLastAccessTime = 0;
                cacheEntry[i].pageEntry[j].dirty = 0;
            }
            cacheEntry[i].blockBuffer = memalign(32, block_size);
            if (cacheEntry[i].blockBuffer == NULL)
            {
                printf("lfs_cache_constructor blockBuffer memalign error!\n");
                result = 1;
                goto end;
            }
        }
        lfs_cache_temp->cacheEntry = cacheEntry;
        lfs_cache_temp->flashInfo = lfs_flash_constructor(block_size, page_size, spare_size);
        if(lfs_cache_temp->flashInfo == NULL)
        {
            printf("lfs_cache_constructor flashInfo memalign error!\n");
            result = 1;
            goto end;
        }
        
        *lfs_cache = lfs_cache_temp;
    }
    else
    {
        printf("lfs_cache_constructor has already created a instance!\n");
        result = 1;
        goto end;
    }

end:
    pthread_mutex_unlock(&lfsInstanceMutex);

    return result;
}

uint32_t lfs_cache_destructor(LFS_CACHE **lfs_cache)
{
    uint32_t result = 0;
    uint32_t i = 0;
    LFS_CACHE_ENTRY *cacheEntry = NULL;

    pthread_mutex_lock(&lfsInstanceMutex);

    if (*lfs_cache != NULL)
    {
        cacheEntry = (*lfs_cache)->cacheEntry;
        for (i = 0; i < (*lfs_cache)->blocksPerCache; i++)
        {
            if (cacheEntry[i].pageEntry != NULL)
            {
                free(cacheEntry[i].pageEntry);
            }
            else
            {
                printf("lfs_cache_destructor pageEntry free error!\n");
                result = 1;
                goto end;
            }
            if (cacheEntry[i].blockBuffer != NULL)
            {
                free(cacheEntry[i].blockBuffer);
            }
            else
            {
                printf("lfs_cache_destructor blockBuffer free error!\n");
                result = 1;
                goto end;
            }
        }
        if ((*lfs_cache)->cacheEntry != NULL)
        {
            free((*lfs_cache)->cacheEntry);
        }
        else
        {
            printf("lfs_cache_destructor cacheEntry free error!\n");
            result = 1;
            goto end;
        }
        if ((*lfs_cache)->flashInfo != NULL)
        {
            lfs_flash_destructor((*lfs_cache)->flashInfo);
        }
        else
        {
            printf("lfs_cache_destructor flashInfo free error!\n");
            result = 1;
            goto end;
        }
        if ((*lfs_cache) != NULL)
        {
            free(*lfs_cache);
            *lfs_cache = NULL;
        }
        else
        {
            printf("lfs_cache_destructor g_lfs_cache free error!\n");
            result = 1;
            goto end;
        }
    }
    else
    {
        printf("lfs_cache_constructor has not created a instance!\n");
        result = 1;
        goto end;
    }

end:
    pthread_mutex_unlock(&lfsInstanceMutex);

    return result;
}

uint32_t lfs_cache_read_page(uint32_t block_index, uint32_t page_index, uint32_t dst_buf, uint32_t count, uint32_t flash_type, LFS_CACHE *lfs_cache)
{
    uint32_t result = 0;
    LFS_CACHE_ENTRY *cacheEntry = NULL;

    pthread_mutex_lock(&lfsInstanceMutex);
    pthread_mutex_lock(&pageAccessMutex);

    if (lfs_cache == NULL)
    {
        printf("lfs_cache_constructor has not created a instance!\n");
        result = 1;
        goto end;
    }

    cacheEntry = lfs_cache_find_entry(CACHE_READ_PAGE, page_index, block_index, count, lfs_cache);
    if (cacheEntry == NULL)
    {
        printf("lfs_cache_read_page cacheEntry NULL!\n");
        result = 1;
        goto end;
    }

    memcpy((void *)dst_buf, cacheEntry->blockBuffer + page_index * lfs_cache->flashInfo->pageSize,
            count * lfs_cache->flashInfo->pageSize);
    /*ithDmaMemcpy(dst_buf, (uint32_t)(cacheEntry->blockBuffer + page_index * lfs_cache->flashInfo->pageSize),
            count * lfs_cache->flashInfo->pageSize);*/
    cacheEntry->lastAccessTime = accessTime(lfs_cache);

end:
    pthread_mutex_unlock(&pageAccessMutex);
    pthread_mutex_unlock(&lfsInstanceMutex);

    return result;
}

uint32_t lfs_cache_write_page(uint32_t block_index, uint32_t page_index, uint32_t src_buf, uint32_t count, uint32_t flash_type, LFS_CACHE *lfs_cache)
{
    uint32_t result = 0;
    LFS_CACHE_ENTRY *cacheEntry = NULL;

    pthread_mutex_lock(&lfsInstanceMutex);
    pthread_mutex_lock(&pageAccessMutex);
    pthread_mutex_lock(&flushAccessMutex);

    if (lfs_cache == NULL)
    {
        printf("lfs_cache_constructor has not created a instance!\n");
        result = 1;
        goto end;
    }

    cacheEntry = lfs_cache_find_entry(CACHE_WRITE_PAGE, page_index, block_index, count, lfs_cache);
    if (cacheEntry == NULL)
    {
        printf("lfs_cache_write_page cacheEntry NULL!\n");
        result = 1;
        goto end;
    }

    memcpy(cacheEntry->blockBuffer + page_index * lfs_cache->flashInfo->pageSize,
            (void *)src_buf, count * lfs_cache->flashInfo->pageSize);
    while (count > 0)
    {
        cacheEntry->pageEntry[page_index].writeLastAccessTime = accessTime(lfs_cache);
        cacheEntry->pageEntry[page_index].dirty = 1;
        page_index++;
        count--;
    }
    cacheEntry->lastAccessTime = accessTime(lfs_cache);
    cacheEntry->dirty = 1;

end:
    pthread_mutex_unlock(&flushAccessMutex);
    pthread_mutex_unlock(&pageAccessMutex);
    pthread_mutex_unlock(&lfsInstanceMutex);

    return result;
}

uint32_t lfs_cache_erase_block(uint32_t block_index, uint32_t flash_type, LFS_CACHE *lfs_cache)
{
    uint32_t result = 0;
    LFS_CACHE_ENTRY *cacheEntry = NULL;

    pthread_mutex_lock(&lfsInstanceMutex);
    pthread_mutex_lock(&pageAccessMutex);
    pthread_mutex_lock(&flushAccessMutex);

    if (lfs_cache == NULL)
    {
        printf("lfs_cache_constructor has not created a instance!\n");
        result = 1;
        goto end;
    }

    cacheEntry = lfs_cache_find_entry(CACHE_ERASE_BLOCK, CACHE_FREE, block_index, lfs_cache->flashInfo->pagesPerBlock, lfs_cache);
    if (cacheEntry == NULL)
    {
        printf("lfs_cache_erase_block cacheEntry NULL!\n");
        result = 1;
        goto end;
    }

    memset(cacheEntry->blockBuffer, FLASH_ERASE_DATA, lfs_cache->flashInfo->blockSize);
    cacheEntry->writeLastAccessTime = accessTime(lfs_cache);
    cacheEntry->lastAccessTime = accessTime(lfs_cache);
    cacheEntry->eraseDirty = 1;

end:
    pthread_mutex_unlock(&flushAccessMutex);
    pthread_mutex_unlock(&pageAccessMutex);
    pthread_mutex_unlock(&lfsInstanceMutex);

    return result;
}

uint32_t lfs_cache_flush(LFS_CACHE *lfs_cache)
{
    return lfs_cache_flush_(lfs_cache);
}

void  lfs_cache_set_flashType(uint32_t flash_type, LFS_CACHE *lfs_cache)
{
    lfs_cache->flashInfo->type = flash_type;
}