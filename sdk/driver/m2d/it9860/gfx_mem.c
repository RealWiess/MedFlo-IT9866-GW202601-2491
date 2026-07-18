/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file gfx_mem.c
 *  GFX memory API header file.
 *
 * @author Awin Huang
 * @version 1.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ite/itp.h"
#include "gfx_mem.h"
#include "nedmalloc.h"

//=============================================================================
//                              Compile Option
//=============================================================================

//=============================================================================
//                              Extern Reference
//=============================================================================
extern uint8_t __heap_start__[];
extern uint8_t __heap_end__[];
//=============================================================================
//                              Macro Definition
//=============================================================================

//=============================================================================
//                              Structure Definition
//=============================================================================

//=============================================================================
//                              Global Data Definition
//=============================================================================
#ifdef CFG_M2D_MEMPOOL_ENABLE
static nedpool* m2dMemPool; 
#endif
//=============================================================================
//                              Private Function Declaration
//=============================================================================

//=============================================================================
//                              Public Function Definition
//=============================================================================
uint8_t *
gfxVmemAlloc(
    uint32_t sizeInByte)
{
#ifdef CFG_M2D_MEMPOOL_ENABLE
    if (!m2dMemPool)
        m2dMemPool = nedcreatepool(CFG_M2D_MEMPOOL_SIZE, 0);
        
    return nedpmemalign(m2dMemPool, 32, sizeInByte);
#elif defined (CFG_M2D_RESERVE_MEM_ENABLE)    
	uint8_t *ptr = (uint8_t *)itpWTM2DAlloc(sizeInByte);
    if (!ptr) {
		ptr = (uint8_t *)itpVmemAlloc(sizeInByte);
    }
	else
	{
		itpWTM2DStats();
	}
	return ptr;
#else    
    return (uint8_t *)itpVmemAlloc(sizeInByte);
#endif // CFG_M2D_MEMPOOL_ENABLE
}

void
gfxVmemFree(
    uint8_t *ptr)
{
#ifdef CFG_M2D_MEMPOOL_ENABLE
    nedpfree(m2dMemPool, ptr);
#elif defined (CFG_M2D_RESERVE_MEM_ENABLE)      
    if ((ptr >= __heap_start__) && (ptr <= __heap_end__))
    {
        itpVmemFree((uint32_t)ptr);
    }
    else
    {
        itpWTM2DFree((uint32_t)ptr);
    }
#else    
    itpVmemFree((uint32_t)ptr);
#endif // CFG_M2D_MEMPOOL_ENABLE    
}

//=============================================================================
//                              Private Function Definition
//=============================================================================