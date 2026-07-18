/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file
 * system.
 * 
 * Copyright (C) 2002-2021 Aleph One Ltd.
 * 
 * Created by Charles Manning <charles@aleph1.co.uk>
 * 
 * This release of YAFFS is for commercial use by commercially licensed
 * customers only. It may be used under the terms of your agreement with
 * Aleph One Ltd. and is limited to use in products for which the
 * customer is licensee, unless the customer has received an unlimited
 * licence from Aleph One Ltd., in which case it may be used in any
 * product.
 * 
 * In the event that this code is used outside the terms of a current,
 * valid and paid agreement with Aleph One Ltd, the code is subject to
 * the GPL version3 license under which the open source code is
 * distributed.
 * 
 * This release was created on 2025-04-18.
 * 
 */

/*
 * yaffs_ramdisk.c: yaffs ram disk component
 * This provides a ram disk under yaffs.
 * NB this is not intended for NAND emulation.
 * Use this with dev->use_nand_ecc enabled, then ECC overheads are not required.
 */

#include "yportenv.h"
#include "yaffs_trace.h"

#include "yaffs_ramdisk.h"
#include "yaffs_guts.h"
#include "yaffs_packedtags1.h"



#define SIZE_IN_MB 2

#define BLOCK_SIZE (32 * 528)
#define BLOCKS_PER_MEG ((1024*1024)/(32 * 512))





typedef struct
{
	u8 data[528]; // Data + spare
} yramdisk_page;

typedef struct
{
	yramdisk_page page[32]; // The pages in the block

} yramdisk_block;



typedef struct
{
	yramdisk_block **block;
	int nBlocks;
} yramdisk_device;

static yramdisk_device ramdisk;

static int  CheckInit(struct yaffs_dev *dev)
{
	static int initialised = 0;

	int i;
	int fail = 0;
	//int nBlocks;
	int nAllocated = 0;

	if(initialised)
	{
		return YAFFS_OK;
	}

	initialised = 1;


	ramdisk.nBlocks = (SIZE_IN_MB * 1024 * 1024)/(16 * 1024);

	ramdisk.block = malloc(sizeof(yramdisk_block *) * ramdisk.nBlocks);

	if(!ramdisk.block) return 0;

	for(i=0; i <ramdisk.nBlocks; i++)
	{
		ramdisk.block[i] = NULL;
	}

	for(i=0; i <ramdisk.nBlocks && !fail; i++)
	{
		if((ramdisk.block[i] = malloc(sizeof(yramdisk_block))) == 0)
		{
			fail = 1;
		}
		else
		{
			yramdisk_erase(dev,i);
			nAllocated++;
		}
	}

	if(fail)
	{
		for(i = 0; i < nAllocated; i++)
		{
			kfree(ramdisk.block[i]);
		}
		kfree(ramdisk.block);

		yaffs_trace(YAFFS_TRACE_ALWAYS,
			"Allocation failed, could only allocate %dMB of %dMB requested.\n",
			nAllocated/64,ramdisk.nBlocks * 528);
		return 0;
	}


	return 1;
}

int yramdisk_wr_chunk(struct yaffs_dev *dev,int nand_chunk,const u8 *data, const struct yaffs_ext_tags *tags)
{
	int blk;
	int pg;


	CheckInit(dev);

	blk = nand_chunk/32;
	pg = nand_chunk%32;


	if(data)
	{
		memcpy(ramdisk.block[blk]->page[pg].data,data,512);
	}


	if(tags)
	{
		struct yaffs_packed_tags1 pt;

		yaffs_pack_tags1(&pt,tags);
		memcpy(&ramdisk.block[blk]->page[pg].data[512],&pt,sizeof(pt));
	}

	return YAFFS_OK;

}


int yramdisk_rd_chunk(struct yaffs_dev *dev,int nand_chunk, u8 *data, struct yaffs_ext_tags *tags)
{
	int blk;
	int pg;


	CheckInit(dev);

	blk = nand_chunk/32;
	pg = nand_chunk%32;


	if(data)
	{
		memcpy(data,ramdisk.block[blk]->page[pg].data,512);
	}


	if(tags)
	{
		struct yaffs_packed_tags1 pt;

		memcpy(&pt,&ramdisk.block[blk]->page[pg].data[512],sizeof(pt));
		yaffs_unpack_tags1(tags,&pt);

	}

	return YAFFS_OK;
}


int yramdisk_check_chunk_erased(struct yaffs_dev *dev,int nand_chunk)
{
	int blk;
	int pg;
	int i;


	CheckInit(dev);

	blk = nand_chunk/32;
	pg = nand_chunk%32;


	for(i = 0; i < 528; i++)
	{
		if(ramdisk.block[blk]->page[pg].data[i] != 0xFF)
		{
			return YAFFS_FAIL;
		}
	}

	return YAFFS_OK;

}

int yramdisk_erase(struct yaffs_dev *dev, int blockNumber)
{

	CheckInit(dev);

	if(blockNumber < 0 || blockNumber >= ramdisk.nBlocks)
	{
		yaffs_trace(YAFFS_TRACE_ALWAYS,
			"Attempt to erase non-existant block %d\n",
			blockNumber);
		return YAFFS_FAIL;
	}
	else
	{
		memset(ramdisk.block[blockNumber],0xFF,sizeof(yramdisk_block));
		return YAFFS_OK;
	}

}

int yramdisk_initialise(struct yaffs_dev *dev)
{
	(void) dev;

	//dev->use_nand_ecc = 1; // force on use_nand_ecc which gets faked.
						 // This saves us doing ECC checks.

	return YAFFS_OK;
}

