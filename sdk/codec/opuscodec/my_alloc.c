/*
 * my_alloc.c
 *
 *  Created on: 2019�~10��18��
 *      Author: USER
 */
#include <stddef.h>
#include <string.h>

struct mem_ctrl_block
{
	int is_available;
	int size;
};

char private_data[65536];//65536

void *managed_memory_start = private_data; //heap_start
void *last_valid_address = private_data;   //__attribute__((malloc)) specify no-alias

//void *__attribute__((malloc)) malloc(size_t numbytes)
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
		if (last_valid_address >= managed_memory_start + 65536)//65536
		{
			return (NULL);
		}
		memory_location = last_valid_address;
		last_valid_address += numbytes;
		if (last_valid_address > managed_memory_start + 65536)//65536
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

//void *__attribute__((malloc)) calloc(size_t nitems, size_t size)
void *calloc(size_t nitems, size_t size)
{
	void *memory_location = malloc(nitems * size);
	memset(memory_location, 0, nitems * size);
	return (memory_location);
}

void free(void *__restrict__ firstbyte)
{
	struct mem_ctrl_block *mcb;
	mcb = firstbyte - sizeof(struct mem_ctrl_block);
	mcb->is_available = 1;
	return;
}

void *realloc(void *firstbyte, size_t size)
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