#ifndef _NANDCACHE_H
#define _NANDCACHE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool NAND_cache_isPageReadCacheHit (uint32_t blk, uint32_t ppo, uint8_t *dBuf);
bool NAND_cache_isByteReadCacheHit (uint32_t blk, uint32_t ppo, uint8_t *dBuf, uint32_t offset, uint32_t rLen);
void NAND_cache_addNewEntry (uint32_t blk, uint32_t ppo, uint8_t *dBuf);
void NAND_cache_blockInvalid (uint32_t blk);
void NAND_cache_pageInvalid (uint32_t blk, uint32_t ppo);
int NAND_cache_init (uint32_t entrySize);
int NAND_cache_drop (void);

#endif // _NANDCACHE_H