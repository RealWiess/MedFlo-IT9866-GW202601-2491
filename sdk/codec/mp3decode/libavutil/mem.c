/*
 * default memory allocator for libavutil
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * default memory allocator for libavutil
 */

#define _XOPEN_SOURCE 600

#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_MALLOC_H
//#include <malloc.h>
#endif

#include "avutil.h"
#include "mem.h"

#define ALIGN 16

/* You can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically. */

#define MAX_MALLOC_SIZE INT_MAX // iclai: should be reduced to the upper bound of the max memory size on our EV Board

struct mem_ctrl_block
{
	int is_available;
	int size;
};

char private_data[65536];

void *managed_memory_start = private_data; //heap_start
void *last_valid_address = private_data;   //__attribute__((malloc)) specify no-alias

void *malloc(size_t numbytes)
{
	void *current_location;
	struct mem_ctrl_block *current_location_mcb;
	void *memory_location = NULL;
	numbytes += sizeof(struct mem_ctrl_block);
	current_location = managed_memory_start;
	while (last_valid_address != current_location)
	{
		current_location_mcb = (struct mem_ctrl_block *)current_location;
		if (current_location_mcb->is_available)
		{
			if (current_location_mcb->size >= numbytes)
			{
				current_location_mcb->is_available = 0;
				memory_location = current_location;
				break;
			}
		}
		current_location += current_location_mcb->size;
	}
	if (!memory_location)
	{
		if (last_valid_address >= managed_memory_start + 65536)
		{
			return (NULL);
		}
		memory_location = last_valid_address;
		last_valid_address += numbytes;
		if (last_valid_address > managed_memory_start + 65536)
		{
			last_valid_address -= numbytes;
			return (NULL);
		}
		else
		{
			current_location_mcb = memory_location;
			current_location_mcb->is_available = 0;
			current_location_mcb->size = numbytes;
		}
	}

	memory_location += sizeof(struct mem_ctrl_block);
	return (memory_location);
}


void free(void *firstbyte)
{
	struct mem_ctrl_block *mcb;
	mcb = firstbyte - sizeof(struct mem_ctrl_block);
	mcb->is_available = 1;
	return;
}

void * realloc(void *firstbyte, size_t size)
{
	struct mem_ctrl_block *mcb;
	void *memory_location = NULL;
	if(firstbyte == NULL)
	{
		memory_location = malloc(size);
		return memory_location;
	}
	mcb = firstbyte - sizeof(struct mem_ctrl_block);
	if(mcb->is_available == 1)
	{
		memory_location = malloc(size);
		return memory_location;
	}
	if(size == 0 && firstbyte != NULL)
	{
		free(firstbyte);
		return NULL;
	}
	if(mcb->size < size)
	{
		free(firstbyte);
		memory_location = malloc(size);
		return memory_location;
	}		
	else
	{
		memory_location = firstbyte;
		return memory_location;
	}
}

void *av_malloc(size_t size)
{
    void *ptr = NULL;

    /* let's disallow possible ambiguous cases */
    if (size > (MAX_MALLOC_SIZE-32))
        return NULL;

    //ptr = memalign(ALIGN,size);
	ptr = malloc(size);

    if(!ptr && !size)
        ptr= av_malloc(1);

    return ptr;
}

void *av_realloc(void *ptr, size_t size)
{
    /* let's disallow possible ambiguous cases */
    if (size > (MAX_MALLOC_SIZE-16))
        return NULL;


    return realloc(ptr, size + !size);

}

void *av_realloc_f(void *ptr, size_t nelem, size_t elsize)
{
    size_t size;
    void *r;

    if (av_size_mult(elsize, nelem, &size)) {
        av_free(ptr);
        return NULL;
    }
    r = av_realloc(ptr, size);
    if (!r && size)
        av_free(ptr);
    return r;
}

void av_free(void *ptr)
{
    if (ptr)
        free(ptr);
}

void av_freep(void *arg)
{
    void **ptr= (void**)arg;
    av_free(*ptr);
    *ptr = NULL;
}

void *av_mallocz(size_t size)
{
    void *ptr = av_malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void *av_calloc(size_t nmemb, size_t size)
{
    if (size <= 0 || nmemb >= INT_MAX / size)
        return NULL;
    return av_mallocz(nmemb * size);
}

char *av_strdup(const char *s)
{
    char *ptr= NULL;
    if(s){
        int len = strlen(s) + 1;
        ptr = av_malloc(len);
        if (ptr)
            memcpy(ptr, s, len);
    }
    return ptr;
}

/* add one element to a dynamic array */
void av_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem)
{
    /* see similar ffmpeg.c:grow_array() */
    int nb, nb_alloc;
    intptr_t *tab;

    nb = *nb_ptr;
    tab = *(intptr_t**)tab_ptr;
    if ((nb & (nb - 1)) == 0) {
        if (nb == 0)
            nb_alloc = 1;
        else
            nb_alloc = nb * 2;
        tab = av_realloc(tab, nb_alloc * sizeof(intptr_t));
        *(intptr_t**)tab_ptr = tab;
    }
    tab[nb++] = (intptr_t)elem;
    *nb_ptr = nb;
}
