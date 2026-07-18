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
/**   NOR Flash                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define LX_SOURCE_CODE


/* Disable ThreadX error checking.  */

#ifndef LX_DISABLE_ERROR_CHECKING
#define LX_DISABLE_ERROR_CHECKING
#endif


/* Include necessary system files.  */

#include "lx_api.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _lx_nor_flash_open_with_cache                       PORTABLE C      */ 
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This function opens a NOR flash instance and ensures the NOR flash  */ 
/*    is in a coherent state.                                             */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    nor_flash                             NOR flash instance            */ 
/*    name                                  Name of NOR flash instance    */ 
/*    nor_driver_initialize                 Driver initialize             */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    return status                                                       */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    (nor_driver_initialize)               Driver initialize             */ 
/*    _lx_nor_flash_driver_read             Driver read                   */ 
/*    _lx_nor_flash_driver_write            Driver write                  */ 
/*    (lx_nor_flash_driver_block_erased_verify)                           */ 
/*                                          NOR flash verify block erased */ 
/*    _lx_nor_flash_driver_block_erase      Driver block erase            */ 
/*    _lx_nor_flash_logical_sector_find     Find logical sector           */ 
/*    _lx_nor_flash_system_error            System error handler          */ 
/*    tx_mutex_create                       Create thread-safe mutex      */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Application Code                                                    */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     William E. Lamie         Initial Version 6.0           */
/*  09-30-2020     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  11-09-2020     William E. Lamie         Modified comment(s),          */
/*                                            fixed compiler warnings,    */
/*                                            resulting in version 6.1.2  */
/*  12-30-2020     William E. Lamie         Modified comment(s),          */
/*                                            fixed compiler warnings,    */
/*                                            resulting in version 6.1.3  */
/*  06-02-2021     Bhupendra Naphade        Modified comment(s), and      */
/*                                            updated product constants   */
/*                                            resulting in version 6.1.7  */
/*  03-08-2023     Xiuwen Cai               Modified comment(s),          */
/*                                            added new driver interface, */
/*                                            resulting in version 6.2.1  */
/*  10-31-2023     Xiuwen Cai               Modified comment(s),          */
/*                                            added count for minimum     */
/*                                            erased blocks, added        */
/*                                            obsolete count cache,       */
/*                                            avoided clearing user       */
/*                                            extension in flash control  */
/*                                            block,                      */
/*                                            resulting in version 6.3.0  */
/*                                                                        */
/**************************************************************************/
UINT  _lx_nor_flash_open_with_cache(LX_NOR_FLASH  *nor_flash, CHAR *name, UINT (*nor_driver_initialize)(LX_NOR_FLASH *), VOID *memory, ULONG size, VOID *work_buf, ULONG work_buf_size, VOID *mapping_buf, ULONG mapping_buf_size)
{
ULONG           sectors_per_block;
ULONG           sector_map_words;
ULONG           bit_map_words;
ULONG           bit_map_mask;
ULONG           total_header_words;
ULONG           header_sectors;
ULONG           *block_word_ptr;
ULONG           block_word;
ULONG           temp;
ULONG           free_sectors;
ULONG           used_sectors;
ULONG           *new_map_entry;
ULONG           *new_sector_address;
ULONG           erased_count, min_erased_count, max_erased_count, temp_erased_count, min_erased_blocks;
ULONG           j, k, l;    
UINT            status;
#ifdef LX_FREE_SECTOR_DATA_VERIFY
ULONG           *sector_word_ptr;
ULONG           sector_word;
#endif
LX_NOR_FLASH   *tail_ptr;

/*  Cache variables*/
UINT    i;
ULONG   cache_size;
ULONG   *cache_memory;
#ifdef LX_NOR_ENABLE_MAPPING_BITMAP
ULONG   mapping_bitmap_words;
ULONG   mapping_bitmap_word;
ULONG   logical_sector;
ULONG   *mapping_bitmap_ptr;
#endif
#ifdef LX_NOR_ENABLE_OBSOLETE_COUNT_CACHE
ULONG   obsolete_count_words;
ULONG   obsolete_sectors;
UINT    cache_loop_end;
#endif
ULONG phy_block, phy_sec;
LX_INTERRUPT_SAVE_AREA

    /* Determine if memory was specified but with an invalid size (less than one NOR sector).  */
    if (((memory) && (size < LX_NOR_SECTOR_SIZE) ))
    {
    
        /* Error in memory size supplied.  */
        return(LX_ERROR);
    }
    LX_PARAMETER_NOT_USED(name);
    /* Clear the NOR flash control block. User extension is not cleared.  */
    LX_MEMSET(nor_flash, 0, (ULONG)((UCHAR*)&(nor_flash -> lx_nor_flash_open_previous) - (UCHAR*)nor_flash) + sizeof(nor_flash -> lx_nor_flash_open_previous));
   
    /* Call the flash driver's initialization function.  */
    (nor_driver_initialize)(nor_flash);

#ifndef LX_DIRECT_READ

    /* Determine if the driver supplied a RAM buffer for reading the NOR sector if direct read is not
       supported.  */
    if (nor_flash -> lx_nor_flash_sector_buffer == LX_NULL)
    {

        /* Return an error.  */
        return(LX_NO_MEMORY);
    }
#endif
    
    /* Setup the offset to the free bit map.  */
    nor_flash -> lx_nor_flash_block_free_bit_map_offset =  sizeof(LX_NOR_FLASH_BLOCK_HEADER)/sizeof(ULONG);
    
    /* Calculate the number of bits we need in the free physical sector bit map.  Subtract 1 to account for the 
       flash block header itself. The case where multiple physical sectors are needed for certain sized flash 
       devices is handled below.  */
    sectors_per_block =  (nor_flash -> lx_nor_flash_words_per_block / LX_NOR_SECTOR_SIZE) - 1;
    if ((!work_buf) || (work_buf_size < sectors_per_block * sizeof(ULONG)) || mapping_buf_size < nor_flash -> lx_nor_flash_total_blocks * sectors_per_block * sizeof(ULONG))
    {
        /* Error in memory size supplied.  */
        return(LX_ERROR);
    }
    /* Calculate the number of words required for the sector map array.  */
    sector_map_words =  sectors_per_block;
    
    /* Calculate the number of words we need for the free physical sector bit map.  */
    bit_map_words =  (sectors_per_block + 31)/ 32;
    
    /* Save the number of bit map words.  */
    nor_flash -> lx_nor_flash_block_bit_map_words =  bit_map_words;
    
    /* Setup the offset (in words) to the array of physical sector mapping.  */
    nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset =  nor_flash -> lx_nor_flash_block_free_bit_map_offset + bit_map_words;
    
    /* Calculate the total number of words required for the flash block header.  */
    total_header_words =  sizeof(LX_NOR_FLASH_BLOCK_HEADER)/sizeof(ULONG) + bit_map_words + sector_map_words;
    
    /* Determine if more physical sectors are needed, which can happen on large devices.  */
    if (total_header_words <= LX_NOR_SECTOR_SIZE)
    {
    
        /* Round up to the size of 1 physical sector.  */
        total_header_words =  LX_NOR_SECTOR_SIZE;
    }
    else
    {

        /* Otherwise calculate how many header sectors are necessary.  */
        header_sectors =  (total_header_words-1)/LX_NOR_SECTOR_SIZE;
        
        /* Round up to the next sector.  */
        header_sectors++;
        
        /* Compute the total header words, rounding to the next sector.  */
        total_header_words =  header_sectors * LX_NOR_SECTOR_SIZE;
        
        /* Adjust the number of sectors per block.  */
        sectors_per_block =  sectors_per_block - (header_sectors - 1);
    }
    
    /* Save the offset to the sector area.  */
    nor_flash -> lx_nor_flash_block_physical_sector_offset =  total_header_words;
    
    /* Save the physical sectors per block and total physical sectors.  */
    nor_flash -> lx_nor_flash_physical_sectors_per_block =  sectors_per_block;
    nor_flash -> lx_nor_flash_total_physical_sectors =      nor_flash -> lx_nor_flash_total_blocks * sectors_per_block;
    
    /* Build the free bit map mask, for the portion of the bit map that is less than 32 bits.  */
    if ((sectors_per_block % 32) != 0)
    {
        bit_map_mask =  (ULONG)(1 << (sectors_per_block % 32));
        bit_map_mask =  bit_map_mask - 1;
    }
    else
    {
    
        /* Exactly 32 sectors for the bit map mask.  */
        bit_map_mask =  LX_ALL_ONES;
    }

    /* Save the free bit map mask in the control block.  */
    nor_flash -> lx_nor_flash_block_bit_map_mask =  bit_map_mask;
    
    /* Setup default values for the max/min erased counts.  */
    min_erased_count =  LX_ALL_ONES;
    min_erased_blocks = 0;
    max_erased_count =  0;
    
    /* Setup the block word pointer to the first word of the first block, which is effectively the 
       flash base address.  */
    block_word_ptr =  nor_flash -> lx_nor_flash_base_address;
    
    /* Initialize the internal NOR cache.  */
    nor_flash -> lx_nor_flash_extended_cache_entries =  0;

    /* Calculate cache size in words.  */
    cache_size = size/sizeof(ULONG);

    /* Setup cache memory pointer.  */
    cache_memory =  (ULONG *) memory;

#if defined(LX_NOR_ENABLE_MAPPING_BITMAP)

    /* Get the mapping bitmap cache size.  */
    mapping_bitmap_words = (nor_flash -> lx_nor_flash_total_physical_sectors + 31) / 32;
    
    /* Check if the mapping bitmap cache fits in the suppiled cache memory.  */
    if (cache_size < mapping_bitmap_words)
    {

        /* Update the cache size.  */
        mapping_bitmap_words = cache_size;
    }
    
    /* Setup the mapping bitmap cache.  */
    nor_flash -> lx_nor_flash_extended_cache_mapping_bitmap =  cache_memory;

    /* Setup the mapping bitmap cache size.  */
    nor_flash -> lx_nor_flash_extended_cache_mapping_bitmap_max_logical_sector =  mapping_bitmap_words * 32;
    
    /* Clear the mapping bitmap cache.  */
    for (i = 0; i < mapping_bitmap_words; i++)
    {
        cache_memory[i] =  0;
    }

    /* Update the cache memory pointer.  */
    mapping_bitmap_ptr =  cache_memory;

    /* Update the cache size.  */
    cache_size =  cache_size - mapping_bitmap_words;

    /* Update the cache memory pointer.  */
    cache_memory =  cache_memory + mapping_bitmap_words;
#endif
    
#if defined(LX_NOR_ENABLE_OBSOLETE_COUNT_CACHE)

    /* Get the obsolete count cache size.  */
    obsolete_count_words = nor_flash -> lx_nor_flash_total_blocks * sizeof(LX_NOR_OBSOLETE_COUNT_CACHE_TYPE) / 4;
    
    /* Check if the obsolete count cache fits in the suppiled cache memory.  */
    if (cache_size < obsolete_count_words)
    {

        /* Update the cache size.  */
        obsolete_count_words = cache_size;
    }
    
    /* Setup the obsolete count cache.  */
    nor_flash -> lx_nor_flash_extended_cache_obsolete_count =  (LX_NOR_OBSOLETE_COUNT_CACHE_TYPE*)cache_memory;

    /* Setup the obsolete count cache size.  */
    nor_flash -> lx_nor_flash_extended_cache_obsolete_count_max_block =  obsolete_count_words * 4 / sizeof(LX_NOR_OBSOLETE_COUNT_CACHE_TYPE);

    /* Update the cache size.  */
    cache_size =  cache_size - obsolete_count_words;

    /* Update the cache memory pointer.  */
    cache_memory =  cache_memory + obsolete_count_words;

    nor_flash -> lx_nor_flash_mapping_map = mapping_buf;
#endif

    /* Loop through the blocks to determine the minimum and maximum erase count.  */
    for (l = 0; l < nor_flash -> lx_nor_flash_total_blocks; l++)
    {
    
        /* Pickup the first word of the block. If the flash manager has executed before, this word contains the
           erase count for the block. Otherwise, if the word is 0xFFFFFFFF, this flash block was either erased
           or this is the first time it was used.  */
#ifdef LX_DIRECT_READ
        
        /* Read the word directly.  */
        block_word =  *block_word_ptr;
#else

        

        status =  _lx_nor_flash_driver_read(nor_flash, block_word_ptr, &block_word, 1);
        
        /* Check for an error from flash driver. Drivers should never return an error..  */
        if (status)
        {
        
            /* Call system error handler.  */
            _lx_nor_flash_system_error(nor_flash, status);
            /* Return an error.  */
            return(LX_ERROR);
        }
#endif

        /* Is the block erased?  */
        if (((block_word & LX_BLOCK_ERASED) != LX_BLOCK_ERASED) && (block_word != LX_BLOCK_ERASE_STARTED))
        {
        
            /* No, valid block.  Isolate the erased count.  */
            erased_count =  (block_word & LX_BLOCK_ERASE_COUNT_MASK);

            /* Is the erased count the minimum?  */
            if (erased_count == min_erased_count)
            {

                /* Yes, increment the minimum erased block count.  */
                min_erased_blocks++;
            }

            /* Is this the new minimum?  */
            if (erased_count < min_erased_count)
            {
                
                /* Yes, remember the new minimum.  */
                min_erased_count =  erased_count;

                /* Reset the minimum erased block count.  */
                min_erased_blocks =  1;
            }
            
            /* Is this the new maximum?  */
            if (erased_count > max_erased_count)
            {
            
                /* Yes, remember the new maximum.  */
                max_erased_count =  erased_count;
            }
        }
        
        /* Move to the next flash block.  */
        block_word_ptr =  block_word_ptr + (nor_flash -> lx_nor_flash_words_per_block);
    }    
    
    /* If we haven't found any erased counts, we can assume the flash is completely erased and needs to 
       be setup for the first time.  */
    if (min_erased_count == LX_ALL_ONES)
    {
    
        /* Indicate that this is the initial format.  */
        nor_flash -> lx_nor_flash_diagnostic_initial_format =  LX_TRUE;
    
        /* Setup the block word pointer to the first word of the first block, which is effectively the 
           flash base address.  */
        block_word_ptr =  nor_flash -> lx_nor_flash_base_address;
    
        /* Loop through the blocks to setup the flash the fist time.  */
        for (l = 0; l < nor_flash -> lx_nor_flash_total_blocks; l++)
        {

            /* Setup the free bit map that corresponds to the free physical sectors in this
               block. Note that we only need to setup the portion of the free bit map that doesn't 
               have sectors associated with it.  */            
            status =  _lx_nor_flash_driver_write(nor_flash, block_word_ptr+(nor_flash -> lx_nor_flash_block_free_bit_map_offset + (bit_map_words-1)) , &bit_map_mask, 1);
        
            /* Check for an error from flash driver. Drivers should never return an error..  */
            if (status)
            {
        
                /* Call system error handler.  */
                _lx_nor_flash_system_error(nor_flash, status);
                /* Return an error.  */
                return(LX_ERROR);
            }

            /* Setup the initial erase count to 1.  */
            block_word =  ((ULONG) 1);
    
            /* Write the initial erase count for the block.  */            
            status =  _lx_nor_flash_driver_write(nor_flash, block_word_ptr, &block_word, 1);

            /* Check for an error from flash driver. Drivers should never return an error..  */
            if (status)
            {
        
                /* Call system error handler.  */
                _lx_nor_flash_system_error(nor_flash, status);
                /* Return an error.  */
                return(LX_ERROR);
            }

            /* Update the overall minimum and maximum erase count.  */
            nor_flash -> lx_nor_flash_minimum_erase_count =  1;
            nor_flash -> lx_nor_flash_minimum_erased_blocks =  nor_flash -> lx_nor_flash_total_blocks;
            nor_flash -> lx_nor_flash_maximum_erase_count =  1;

            /* Update the number of free physical sectors.  */
            nor_flash -> lx_nor_flash_free_physical_sectors =   nor_flash -> lx_nor_flash_free_physical_sectors + sectors_per_block;
            
            ULONG *mapping_buf_ptr = nor_flash->lx_nor_flash_mapping_map + l * sectors_per_block;
            memset(mapping_buf_ptr, 0xFF, sectors_per_block*sizeof(ULONG));
            /* Move to the next flash block.  */
            block_word_ptr =  block_word_ptr + (nor_flash -> lx_nor_flash_words_per_block);
        }    
    }
    else
    {

        /* At this point, we have a previously managed flash structure. This needs to be traversed to prepare for the 
           current flash operation.  */

        /* Default the flash free sector search to an invalid value.  */
        nor_flash -> lx_nor_flash_free_block_search =  nor_flash -> lx_nor_flash_total_blocks;

        /* Setup the block word pointer to the first word of the first block, which is effectively the 
           flash base address.  */
        block_word_ptr =  nor_flash -> lx_nor_flash_base_address;

        /* Loop through the blocks.  */
        for (l = 0; l < nor_flash -> lx_nor_flash_total_blocks; l++)
        {
         
            /* First, determine if this block has a valid erase count.  */
#ifdef LX_DIRECT_READ
        
            /* Read the word directly.  */
            block_word =  *block_word_ptr;
#else
            status =  _lx_nor_flash_driver_read(nor_flash, block_word_ptr, &block_word, 1);

            /* Check for an error from flash driver. Drivers should never return an error..  */
            if (status)
            {
        
                /* Call system error handler.  */
                _lx_nor_flash_system_error(nor_flash, status);
                /* Return an error.  */
                return(LX_ERROR);
            }
#endif

            /* Is the block erased?  */
            if (((block_word & LX_BLOCK_ERASED) == LX_BLOCK_ERASED) || (block_word == LX_BLOCK_ERASE_STARTED))
            {
                /* This can happen if we were previously in the process of erasing the flash block and a 
                   power interruption occurs.  It should only occur once though. */

                /* Is this the first time?  */
                if (nor_flash -> lx_nor_flash_diagnostic_erased_block)
                {
                            
                    /* No, this is a potential format error, since this should only happen once in a given
                       NOR flash format.  */
                    _lx_nor_flash_system_error(nor_flash, LX_SYSTEM_INVALID_BLOCK);
                    /* Return an error.  */
                    return(LX_ERROR);
                }

                /* Increment the erased block diagnostic.  */
                nor_flash -> lx_nor_flash_diagnostic_erased_block++;

                /* Check to see if the block is erased. */
#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
                status =  (nor_flash -> lx_nor_flash_driver_block_erased_verify)(nor_flash, l);
#else
                status =  (nor_flash -> lx_nor_flash_driver_block_erased_verify)(l);
#endif

                /* Is the block completely erased?  */
                if (status != LX_SUCCESS)
                {
                
                    /* Is this the first time?  */
                    if (nor_flash -> lx_nor_flash_diagnostic_re_erase_block)
                    {
                            
                        /* No, this is a potential format error, since this should only happen once in a given
                           NOR flash format.  */
                        _lx_nor_flash_system_error(nor_flash, LX_SYSTEM_INVALID_BLOCK);
                        /* Return an error.  */
                        return(LX_ERROR);
                    }

                    /* Increment the erased block diagnostic.  */
                    nor_flash -> lx_nor_flash_diagnostic_re_erase_block++;
        
                    /* No, the block is not fully erased, erase it again.  */
                    status =  _lx_nor_flash_driver_block_erase(nor_flash, l, max_erased_count);
                    
                    /* Check for an error from flash driver. Drivers should never return an error..  */
                    if (status)
                    {
        
                        /* Call system error handler.  */
                        _lx_nor_flash_system_error(nor_flash, status);
                        /* Return an error.  */
                        return(LX_ERROR);
                    }
                }
                ULONG *mapping_buf_ptr = nor_flash->lx_nor_flash_mapping_map + l * sectors_per_block;
                memset(mapping_buf_ptr, 0xFF, sectors_per_block*sizeof(ULONG));
                /* Setup the free bit map that corresponds to the free physical sectors in this
                   block. Note that we only need to setup the portion of the free bit map that doesn't 
                   have sectors associated with it.  */            
                status =  _lx_nor_flash_driver_write(nor_flash, block_word_ptr+(nor_flash -> lx_nor_flash_block_free_bit_map_offset + (bit_map_words-1)) , &bit_map_mask, 1);
        
                /* Check for an error from flash driver. Drivers should never return an error..  */
                if (status)
                {
        
                    /* Call system error handler.  */
                    _lx_nor_flash_system_error(nor_flash, status);
                    /* Return an error.  */
                    return(LX_ERROR);
                }
            
                /* Write the initial erase count for the block with upper bit set.  */            
                temp_erased_count =  (max_erased_count | LX_BLOCK_ERASED);
                status =  _lx_nor_flash_driver_write(nor_flash, block_word_ptr, &temp_erased_count, 1);

                /* Check for an error from flash driver. Drivers should never return an error..  */
                if (status)
                {
        
                    /* Call system error handler.  */
                    _lx_nor_flash_system_error(nor_flash, status);
                    /* Return an error.  */
                    return(LX_ERROR);
                }

                /* Write the final initial erase count for the block.  */            
                status =  _lx_nor_flash_driver_write(nor_flash, block_word_ptr, &max_erased_count, 1);

                /* Check for an error from flash driver. Drivers should never return an error..  */
                if (status)
                {
        
                    /* Call system error handler.  */
                    _lx_nor_flash_system_error(nor_flash, status);
                    /* Return an error.  */
                    return(LX_ERROR);
                }

                /* Update the number of free physical sectors.  */
                nor_flash -> lx_nor_flash_free_physical_sectors =   nor_flash -> lx_nor_flash_free_physical_sectors + sectors_per_block;
                
#ifdef LX_NOR_ENABLE_OBSOLETE_COUNT_CACHE
                obsolete_sectors = 0;
#endif
            }
            else
            {
                /* Read this words of the free sector bit map.  */    
                status =  _lx_nor_flash_driver_read(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_free_bit_map_offset), work_buf, bit_map_words);
                
                /* Check for an error from flash driver. Drivers should never return an error..  */
                if (status)
                {
    
                    /* Call system error handler.  */
                    _lx_nor_flash_system_error(nor_flash, status);
                    /* Return an error.  */
                    return(LX_ERROR);
                }           
                /* Calculate the number of free sectors from the free sector bit map.  */
                free_sectors =  0;
                for (j = 0; j < bit_map_words; j++)
                {
#if 0                
                    /* Read this word of the free sector bit map.  */    
#ifdef LX_DIRECT_READ
        
                    /* Read the word directly.  */
                    block_word =  *(block_word_ptr + nor_flash -> lx_nor_flash_block_free_bit_map_offset + j);
#else
                    status =  _lx_nor_flash_driver_read(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_free_bit_map_offset + j), &block_word, 1);

                    /* Check for an error from flash driver. Drivers should never return an error..  */
                    if (status)
                    {
        
                        /* Call system error handler.  */
                        _lx_nor_flash_system_error(nor_flash, status);
                        /* Return an error.  */
                        return(LX_ERROR);
                    }
#endif
#else
                    block_word = *((ULONG*)work_buf + j);
#endif
                    
                    /* Count the number of set bits (free sectors).  */
                    for (k = 0; k < 32; k++)
                    {
                    
                        /* Is this sector free?  */
                        if (block_word & 1)
                        {
                            /* Yes, this sector is free, increment the free sectors count.  */
                            free_sectors++;
                            
                            /* Determine if we need to update the search pointer.  */
                            if (nor_flash -> lx_nor_flash_free_block_search == nor_flash -> lx_nor_flash_total_blocks)
                            {
                            
                                /* Remember the block with free sectors.  */
                                nor_flash -> lx_nor_flash_free_block_search =  l;
                            }
                        }
                        
                        /* Shift down the free sector.  */
                        block_word =  block_word >> 1;
                    }
                }
                    
                /* Update the number of free physical sectors.  */
                nor_flash -> lx_nor_flash_free_physical_sectors =   nor_flash -> lx_nor_flash_free_physical_sectors + free_sectors;

                /* We need to now examine the mapping list.  */
                    
                /* Calculate how many non-free sectors there are - this includes valid and obsolete sectors.  */
                used_sectors =  sectors_per_block - free_sectors;

                /* Now walk the list of logical-physical sector mapping.  */
                ULONG *mapping_buf_ptr = nor_flash->lx_nor_flash_mapping_map + l * sectors_per_block;
                status =  _lx_nor_flash_driver_read(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset), mapping_buf_ptr, sectors_per_block);
                
                /* Check for an error from flash driver. Drivers should never return an error..  */
                if (status)
                {
    
                    /* Call system error handler.  */
                    _lx_nor_flash_system_error(nor_flash, status);
                    /* Return an error.  */
                    return(LX_ERROR);
                }           
#if defined(LX_NOR_ENABLE_OBSOLETE_COUNT_CACHE)

                /* Initialize the obsolete count cache.  */
                obsolete_sectors = 0;
                cache_loop_end = 0;
#endif         
                for (j = 0; j < sectors_per_block; j++)
                {
#if 0                    
                    /* Read this word of the sector mapping list.  */
#ifdef LX_DIRECT_READ
        
                    /* Read the word directly.  */
                    block_word =  *(block_word_ptr + nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset + j);
#else

                    status =  _lx_nor_flash_driver_read(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset + j), &block_word, 1);

                    /* Check for an error from flash driver. Drivers should never return an error..  */
                    if (status)
                    {
        
                        /* Call system error handler.  */
                        _lx_nor_flash_system_error(nor_flash, status);
                        /* Return an error.  */
                        return(LX_ERROR);
                    }
#endif                    
#else
                    block_word = *(mapping_buf_ptr + j);
#endif
                    /* Determine if we are expecting to find a used sector.   */
                    if (used_sectors)
                    {

                        /* Yes, we expect this entry to be used.  */

                        /* Is this sector in-use?  */
                        if ((block_word & LX_NOR_LOGICAL_SECTOR_MASK) != LX_NOR_LOGICAL_SECTOR_MASK)
                        {

                            /* Determine if the valid bit is set and the superceded bit is clear. This indicates the block was 
                               about to become obsolete.  */
                            if ((block_word & LX_NOR_PHYSICAL_SECTOR_VALID) && ((block_word & LX_NOR_PHYSICAL_SECTOR_SUPERCEDED) == 0))
                            {


                                /* Increment the being obsoleted count.  */
                                nor_flash -> lx_nor_flash_diagnostic_sector_being_obsoleted++;

                                /* Save the currently mapped physical sectors.  */
                                temp =  nor_flash -> lx_nor_flash_mapped_physical_sectors;
                                
                                /* Indicate all the physical sectors are mapped for the purpose of this search.  */
                                nor_flash -> lx_nor_flash_mapped_physical_sectors =  nor_flash -> lx_nor_flash_total_physical_sectors;

                                /* Yes, this block was about to become obsolete. Perform a search for a logical sector entry that
                                   has both of these bits set.  */
                                _lx_nor_flash_logical_sector_find(nor_flash, (block_word & LX_NOR_LOGICAL_SECTOR_MASK), LX_TRUE, &new_map_entry, &new_sector_address, &phy_block, &phy_sec);

                                /* Restore the number of mapped physical sectors.  */
                                nor_flash -> lx_nor_flash_mapped_physical_sectors =  temp;

                                /* Determine if the new logical sector entry is present.  */
                                if (new_map_entry)
                                {
                                
                                    /* Yes, make the current entry obsolete in favor of the new entry.  */
                                    block_word =  block_word & ~((ULONG) LX_NOR_PHYSICAL_SECTOR_VALID);
                                    status =  _lx_nor_flash_driver_write(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset + j), &block_word, 1);

                                    /* Check for an error from flash driver. Drivers should never return an error..  */
                                    if (status)
                                    {
        
                                        /* Call system error handler.  */
                                        _lx_nor_flash_system_error(nor_flash, status);
                                        /* Return an error.  */
                                        return(LX_ERROR);
                                    }
                                    *(mapping_buf_ptr + j) = block_word;
                                    /* Is this the first time?  */
                                    if (nor_flash -> lx_nor_flash_diagnostic_sector_obsoleted)
                                    {
                            
                                        /* No, this is a potential format error, since this should only happen once in a given
                                           NOR flash format.  */
                                        _lx_nor_flash_system_error(nor_flash, LX_SYSTEM_INVALID_FORMAT);
                                        /* Return an error.  */
                                        return(LX_ERROR);
                                    }

                                    /* Increment the obsoleted count.  */
                                    nor_flash -> lx_nor_flash_diagnostic_sector_obsoleted++;
                                }
                            }                          
                            
                        }    
                        
                        /* Determine if the sector is free.  */
                        else if (block_word == LX_NOR_PHYSICAL_SECTOR_FREE)
                        {
                        
                            /* A free entry when there are still used sectors implies that the sector was allocated and a power interruption 
                               took place prior to writing the new logical sector number into the list.  */
                            
                            /* Is this the first time?  */
                            if (nor_flash -> lx_nor_flash_diagnostic_mapping_invalidated)
                            {
                            
                                /* No, this is a potential format error, since this should only happen once in a given
                                   NOR flash format.  */
                                _lx_nor_flash_system_error(nor_flash, LX_SYSTEM_INVALID_FORMAT);
                                /* Return an error.  */
                                return(LX_ERROR);
                            }
                            
                            /* Write 0s out to this entry to invalidate the sector entry.  */
                            block_word =  0;
                            status =  _lx_nor_flash_driver_write(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset + j), &block_word, 1);

                            /* Check for an error from flash driver. Drivers should never return an error..  */
                            if (status)
                            {
        
                                /* Call system error handler.  */
                                _lx_nor_flash_system_error(nor_flash, status);
                                /* Return an error.  */
                                return(LX_ERROR);
                            }
                            *(mapping_buf_ptr + j) = block_word;
                            /* Increment the number of mapping invalidates.  */                            
                            nor_flash -> lx_nor_flash_diagnostic_mapping_invalidated++;
                        }
                        
                        /* Yes, now determine if the sector is obsolete.  */
                        if ((block_word & LX_NOR_PHYSICAL_SECTOR_VALID) == 0)
                        {
                                
                            /* Increment the number of obsolete sectors.  */
                            nor_flash -> lx_nor_flash_obsolete_physical_sectors++;
                        }

                        /* Determine if the mapping for this sector isn't yet valid.  */
                        else if (block_word & LX_NOR_PHYSICAL_SECTOR_MAPPING_NOT_VALID)
                        {
                       
                            /* Yes, a power interruption or reset occurred while the sector mapping entry was being written.  */

                            /* Increment the number of obsolete sectors.  */
                            nor_flash -> lx_nor_flash_obsolete_physical_sectors++;
                            
                            /* Increment the interrupted mapping counter.  */                           
                            nor_flash -> lx_nor_flash_diagnostic_mapping_write_interrupted++;

                            /* Invalidate this entry - clearing valid bit, superceded bit and logical sector.  */
                            block_word =  0;
                            status =  _lx_nor_flash_driver_write(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset + j), &block_word, 1);

                            /* Check for an error from flash driver. Drivers should never return an error..  */
                            if (status)
                            {
        
                                /* Call system error handler.  */
                                _lx_nor_flash_system_error(nor_flash, status);
                                /* Return an error.  */
                                return(LX_ERROR);
                            }
                            *(mapping_buf_ptr + j) = block_word;
                                    
                        }
                        else
                        {
                            /* Increment the number of mapped physical sectors.  */
                            nor_flash -> lx_nor_flash_mapped_physical_sectors++;
                        }
                        
                        /* Decrease the number of used sectors.  */
                        used_sectors--;

                        /* Is this entry valid?  */
                        if(block_word == LX_NOR_PHYSICAL_SECTOR_FREE)
                        {
                            cache_loop_end = 1;
                        }
                        if (!cache_loop_end)
                        {                            
                            if ((block_word & (LX_NOR_PHYSICAL_SECTOR_VALID | LX_NOR_PHYSICAL_SECTOR_MAPPING_NOT_VALID)) == LX_NOR_PHYSICAL_SECTOR_VALID)
                            {
    #if defined(LX_NOR_ENABLE_MAPPING_BITMAP)

                                /* Yes, get the logical sector.  */
                                logical_sector = block_word & LX_NOR_LOGICAL_SECTOR_MASK;
                                
                                /* Get the mapping bitmap word.  */
                                mapping_bitmap_word = logical_sector >> 5;
                                
                                /* Check if the mapping bitmap word is within the cache.  */
                                if (mapping_bitmap_word < mapping_bitmap_words)
                                {

                                    /* Set the bit in the mapping bitmap.  */
                                    mapping_bitmap_ptr[mapping_bitmap_word] |=  (ULONG)(1 << (logical_sector & 31));
                                }
    #endif
                                
                            }
                            else
                            {
    #if defined(LX_NOR_ENABLE_OBSOLETE_COUNT_CACHE)

                                /* Increment the obsolete sector count.  */
                                obsolete_sectors++;
    #endif
                            }                        
                        }
                    }
                    else
                    {
                    
                        /* No more used sectors in this flash block.  */
                    
                        /* In this case the entry must be free or there is a serious NOR flash format error present.  */
                        if (block_word != LX_NOR_PHYSICAL_SECTOR_FREE)
                        {
                        
                            /* Increment the sector not free diagnostic.  */
                            nor_flash -> lx_nor_flash_diagnostic_sector_not_free++;

                            /* NOR flash format.  */
                            _lx_nor_flash_system_error(nor_flash, LX_SYSTEM_INVALID_FORMAT);

                            /* Write 0s out to this entry to invalidate the sector entry.  */
                            block_word =  0;
                            status =  _lx_nor_flash_driver_write(nor_flash, (block_word_ptr + nor_flash -> lx_nor_flash_block_physical_sector_mapping_offset + j), &block_word, 1);

                            /* Check for an error from flash driver. Drivers should never return an error..  */
                            if (status)
                            {
        
                                /* Call system error handler.  */
                                _lx_nor_flash_system_error(nor_flash, status);
                                /* Return an error.  */
                                return(LX_ERROR);
                            }
                            *(mapping_buf_ptr + j) = block_word;
                                    
                        }
                        
#ifdef LX_FREE_SECTOR_DATA_VERIFY

                        /* Pickup address of the free sector data area.  */
                        sector_word_ptr =  block_word_ptr + (nor_flash -> lx_nor_flash_block_physical_sector_offset) + (j * LX_NOR_SECTOR_SIZE);

                        /* Determine if the data for this sector is free.  */
                        for (k = 0; k < LX_NOR_SECTOR_SIZE; k++)
                        {

#ifdef LX_DIRECT_READ
        
                            /* Read the word directly.  */
                            sector_word =  *(sector_word_ptr);
#else
                            status =  _lx_nor_flash_driver_read(nor_flash, (sector_word_ptr), &sector_word, 1);

                            /* Check for an error from flash driver. Drivers should never return an error..  */
                            if (status)
                            {
        
                                /* Call system error handler.  */
                                _lx_nor_flash_system_error(nor_flash, status);

                                /* Return an error.  */
                                return(LX_ERROR);
                            }
#endif

                            /* Determine if this word is not available.  */
                            if (sector_word != LX_NOR_PHYSICAL_SECTOR_FREE)
                            {
                            
                                /* Increment the sector data not free diagnostic.  */
                                nor_flash -> lx_nor_flash_diagnostic_sector_data_not_free++;

                                /* This is a format error.  */
                                _lx_nor_flash_system_error(nor_flash, LX_SYSTEM_INVALID_BLOCK);
                               
                                /* Return an error.  */
                                return(LX_ERROR);
                            }

                            /* Move to the next word in the sector.  */
                            sector_word_ptr++;
                        }
#endif
                    }
                }


            }                        
#if defined(LX_NOR_ENABLE_OBSOLETE_COUNT_CACHE)
            /* Check if the block is cached by obsolete count cache.  */
            if (l < nor_flash -> lx_nor_flash_extended_cache_obsolete_count_max_block)
            {

                /* Yes, cache the obsolete sector count.  */
                nor_flash -> lx_nor_flash_extended_cache_obsolete_count[l] = (LX_NOR_OBSOLETE_COUNT_CACHE_TYPE)obsolete_sectors;
            }
#endif          
            /* Move to the next flash block.  */
            block_word_ptr =  block_word_ptr + (nor_flash -> lx_nor_flash_words_per_block);
        }       

        /* Update the overall minimum and maximum erase count.  */
        nor_flash -> lx_nor_flash_minimum_erase_count =  min_erased_count;
        nor_flash -> lx_nor_flash_minimum_erased_blocks =  min_erased_blocks;
        nor_flash -> lx_nor_flash_maximum_erase_count =  max_erased_count;

        /* Determine if we need to update the free sector search pointer.  */
        if (nor_flash -> lx_nor_flash_free_block_search == nor_flash -> lx_nor_flash_total_blocks)
        {
                            
            /* Just start at the beginning.  */
            nor_flash -> lx_nor_flash_free_block_search =  0;
        }
    }

#ifdef LX_THREAD_SAFE_ENABLE

    /* If the thread safe option is enabled, create a ThreadX mutex that will be used in all external APIs 
       in order to provide thread-safe operation.  */
    status =  tx_mutex_create(&nor_flash -> lx_nor_flash_mutex, "NOR Flash Mutex", TX_NO_INHERIT);

    /* Determine if the mutex creation encountered an error.  */
    if (status != LX_SUCCESS)
    {
    
        /* Call system error handler, since this should not happen.  */
        _lx_nor_flash_system_error(nor_flash, LX_SYSTEM_MUTEX_CREATE_FAILED);
    
        /* Return error to caller.  */
        return(LX_ERROR);
    }
#endif

    /* Enable the sector mapping cache.  */
    nor_flash -> lx_nor_flash_sector_mapping_cache_enabled =  LX_TRUE;

    /* Initialize the last found block and sector markers.  */
    nor_flash -> lx_nor_flash_found_block_search =   0;
    nor_flash -> lx_nor_flash_found_sector_search =  0;

    /* Lockout interrupts.  */
    LX_DISABLE

    /* At this point, the NOR flash has been opened successfully.  Place the 
       NOR flash control block on the linked list of currently opened NOR flashes.  */

    /* Set the NOR flash state to open.  */
    nor_flash -> lx_nor_flash_state =  LX_NOR_FLASH_OPENED;

    /* Place the NOR flash control block on the list of opened NOR flashes.  First,
       check for an empty list.  */
    if (_lx_nor_flash_opened_count)
    {

        /* List is not empty - other NOR flashes are open.  */

        /* Pickup tail pointer.  */
        tail_ptr =  _lx_nor_flash_opened_ptr -> lx_nor_flash_open_previous;

        /* Place the new NOR flash control block in the list.  */
        _lx_nor_flash_opened_ptr -> lx_nor_flash_open_previous =  nor_flash;
        tail_ptr -> lx_nor_flash_open_next =                       nor_flash;

        /* Setup this NOR flash's opened links.  */
        nor_flash -> lx_nor_flash_open_previous =  tail_ptr;
        nor_flash -> lx_nor_flash_open_next =      _lx_nor_flash_opened_ptr;   
    }
    else
    {

        /* The opened NOR flash list is empty.  Add the NOR flash to empty list.  */
        _lx_nor_flash_opened_ptr =                 nor_flash;
        nor_flash -> lx_nor_flash_open_next =      nor_flash;
        nor_flash -> lx_nor_flash_open_previous =  nor_flash;
    }

    /* Increment the opened NOR flash counter.  */
    _lx_nor_flash_opened_count++;

    /* Loop through the memory supplied and assign to cache entries.  */
    i =  0;
    while (cache_size >= LX_NOR_SECTOR_SIZE)
    {
    
        /* Setup this cache entry.  */
        if(i < LX_NOR_EXTENDED_CACHE_SIZE)
        {
            nor_flash -> lx_nor_flash_extended_cache[i].lx_nor_flash_extended_cache_entry_sector_address =  LX_NULL;
            nor_flash -> lx_nor_flash_extended_cache[i].lx_nor_flash_extended_cache_entry_sector_memory =   cache_memory;
            nor_flash -> lx_nor_flash_extended_cache[i].lx_nor_flash_extended_cache_entry_access_count =    0;
        }
        /* Move the cache memory forward.   */
        cache_memory =  cache_memory + LX_NOR_SECTOR_SIZE;
        
        /* Decrement the size.  */
        cache_size =  cache_size - LX_NOR_SECTOR_SIZE;
    
        /* Move to next cache entry.  */
        i++;
    }
    
    /* Save the number of cache entries.  */
    if(i > LX_NOR_EXTENDED_CACHE_SIZE)
    {

        nor_flash -> lx_nor_flash_extended_cache_entries =  LX_NOR_EXTENDED_CACHE_SIZE;
    }
    else
    {

        nor_flash -> lx_nor_flash_extended_cache_entries =  i;
    }  
    /* Restore interrupts.  */
    LX_RESTORE
                
    /* Return a successful completion.  */
    return(LX_SUCCESS);
}

