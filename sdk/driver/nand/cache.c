#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <stdint.h>

#include "cache.h"
#include "ite/ith.h"

#define CACHE_MAX_SIZE CFG_NAND_READ_CACHE_SIZE

typedef struct {
	uint64_t    countId;
	uint32_t    blk;
	uint32_t    ppo;
	uint8_t*    cache;
    bool        valid;
} NAND_CACHE_ENTRY;

typedef struct {
    uint64_t              currCntId;
    uint32_t              entrySize; //page size + spr size
    uint32_t              numberOfPages;
    uint32_t              cntEntries;
	NAND_CACHE_ENTRY*     cacheEntries;
} NAND_CACHE;

static NAND_CACHE*	NANDCache = NULL;

static inline uint32_t _find_oldestIdx(NAND_CACHE_ENTRY* cacheEntries, uint32_t size) {
    uint32_t oldestIdx = 0;
    uint32_t i = 0;
    uint64_t min = cacheEntries[0].countId;
    
    for(i = 1; i < size; i++) {
        if(cacheEntries[i].valid == false) {
            oldestIdx = i;
            break;
        } else if(cacheEntries[i].countId < min) {
            min = cacheEntries[i].countId;
            oldestIdx = i;
        }
    }

    return oldestIdx;
}

static inline void _debug_show_entry_content(NAND_CACHE_ENTRY* cacheEntry)
{
#ifdef _DEBUG_
    ithPrintf("[%d,%d,%d,%d]\n", (uint32_t)cacheEntry->countId, cacheEntry->valid, cacheEntry->blk, cacheEntry->ppo);
    ithPrintf("pattern:%d:%d:%d:%d:%d:%d:%d:%d\n", cacheEntry->cache[0],cacheEntry->cache[1],cacheEntry->cache[2],cacheEntry->cache[3],cacheEntry->cache[4],cacheEntry->cache[5],cacheEntry->cache[6],cacheEntry->cache[7]);
#endif
}

int NAND_cache_init (uint32_t entrySize)
{
    if (NANDCache == NULL)
    {
        if(entrySize == 0 || CACHE_MAX_SIZE == 0) return -1;
        NAND_CACHE_ENTRY* cacheEntries = NULL;
        uint32_t i = 0;
        NANDCache = (NAND_CACHE*)malloc(sizeof(NAND_CACHE));
        if (NANDCache == NULL) {
            ithPrintf("NAND_cache_init:malloc NANDCache fail !\n");
            return -1;
        }
        NANDCache->currCntId = NANDCache->cntEntries = 0;
        NANDCache->entrySize = entrySize;
        NANDCache->numberOfPages = CACHE_MAX_SIZE/entrySize;
        
        cacheEntries = (NAND_CACHE_ENTRY*) malloc(sizeof(NAND_CACHE_ENTRY) * NANDCache->numberOfPages);
        if (cacheEntries == NULL) {
            free(NANDCache);
            NANDCache = NULL;
            ithPrintf("NAND_cache_init:malloc cacheEntries fail !\n");
            return -1;
        }
        for (i = 0; i < NANDCache->numberOfPages; i++) {
            cacheEntries[i].countId = 0;
            cacheEntries[i].blk = 0;
            cacheEntries[i].ppo = 0;
            cacheEntries[i].valid = false;
            cacheEntries[i].cache = (uint8_t*) malloc(NANDCache->entrySize);
            if (cacheEntries[i].cache == NULL) {
                for(int j=0; j<i; j++) {
                    free(cacheEntries[j].cache);
                }
                free(NANDCache->cacheEntries);
                free(NANDCache);
                NANDCache = NULL;
                ithPrintf("NAND_cache_init:malloc cacheEntries[%d].cache fail !\n", i);
                return -1;
            }
        }
        NANDCache->cacheEntries = cacheEntries;
    }
    return 0;
}

int NAND_cache_drop (void)
{
    if (NANDCache != NULL) {
        uint32_t i = 0;
        for (i = 0; i < NANDCache->numberOfPages; i++) {
            free(NANDCache->cacheEntries[i].cache);
        }
        free(NANDCache->cacheEntries);
        free(NANDCache);
        NANDCache = NULL;
    }
    return 0;
}

bool NAND_cache_isByteReadCacheHit (uint32_t blk, uint32_t ppo, uint8_t *dBuf, uint32_t offset, uint32_t rLen)
{
    bool isHit = false;
    if (NANDCache != NULL) {
        if(offset+rLen > NANDCache->entrySize) return false;
        for(uint32_t i=0;i<NANDCache->cntEntries;i++) {
            if(blk == NANDCache->cacheEntries[i].blk && ppo == NANDCache->cacheEntries[i].ppo && NANDCache->cacheEntries[i].valid == true)
            {
                memmove(dBuf, NANDCache->cacheEntries[i].cache+offset, rLen);
                NANDCache->cacheEntries[i].countId = NANDCache->currCntId;
                NANDCache->currCntId++;
                isHit = true;
                _debug_show_entry_content(&(NANDCache->cacheEntries[i]));
                break;
            }
        }
    }
    return isHit;
}

bool NAND_cache_isPageReadCacheHit (uint32_t blk, uint32_t ppo, uint8_t *dBuf)
{
    bool isHit = false;
    if (NANDCache != NULL) {
        for(uint32_t i=0;i<NANDCache->cntEntries;i++) {
            if(blk == NANDCache->cacheEntries[i].blk && ppo == NANDCache->cacheEntries[i].ppo && NANDCache->cacheEntries[i].valid == true)
            {
                memmove(dBuf, NANDCache->cacheEntries[i].cache, NANDCache->entrySize);
                NANDCache->cacheEntries[i].countId = NANDCache->currCntId;
                NANDCache->currCntId++;
                isHit = true;
                _debug_show_entry_content(&(NANDCache->cacheEntries[i]));
                break;
            }
        }
    }
    return isHit;
}

void NAND_cache_addNewEntry (uint32_t blk, uint32_t ppo, uint8_t *dBuf)
{
    if (NANDCache != NULL) {
        if(NANDCache->cntEntries < NANDCache->numberOfPages) {
            NANDCache->cacheEntries[NANDCache->cntEntries].countId = NANDCache->currCntId;
            NANDCache->cacheEntries[NANDCache->cntEntries].blk = blk; 
            NANDCache->cacheEntries[NANDCache->cntEntries].ppo = ppo;
            NANDCache->cacheEntries[NANDCache->cntEntries].valid = true;
            memmove(NANDCache->cacheEntries[NANDCache->cntEntries].cache, dBuf, NANDCache->entrySize);
            NANDCache->currCntId++;
            NANDCache->cntEntries++; 
        } else {
            uint32_t oldestIdx = _find_oldestIdx(NANDCache->cacheEntries, NANDCache->cntEntries);
            NANDCache->cacheEntries[oldestIdx].countId = NANDCache->currCntId;
            NANDCache->cacheEntries[oldestIdx].blk = blk; 
            NANDCache->cacheEntries[oldestIdx].ppo = ppo;
            NANDCache->cacheEntries[oldestIdx].valid = true;
            memmove(NANDCache->cacheEntries[oldestIdx].cache, dBuf, NANDCache->entrySize);
            NANDCache->currCntId++;
        }
    }
}

void NAND_cache_blockInvalid (uint32_t blk)
{
    if (NANDCache != NULL) {
        for(uint32_t i=0;i<NANDCache->cntEntries;i++) {
            if(blk == NANDCache->cacheEntries[i].blk)
            {
                 NANDCache->cacheEntries[i].valid = false;
            }
        }
    }
}

void NAND_cache_pageInvalid (uint32_t blk, uint32_t ppo)
{
    if (NANDCache != NULL) {
        for(uint32_t i=0;i<NANDCache->cntEntries;i++) {
            if(blk == NANDCache->cacheEntries[i].blk && ppo == NANDCache->cacheEntries[i].ppo)
            {
                 NANDCache->cacheEntries[i].valid = false;
                 break;
            }
        }
    }
}