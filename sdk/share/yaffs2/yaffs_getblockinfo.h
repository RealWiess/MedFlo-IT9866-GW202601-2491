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

#ifndef __YAFFS_GETBLOCKINFO_H__
#define __YAFFS_GETBLOCKINFO_H__

#include "yaffs_guts.h"
#include "yaffs_trace.h"

/* Function to manipulate block info */
static inline struct yaffs_block_info *yaffs_get_block_info(struct yaffs_dev
							      *dev, int blk)
{
	if (blk < (int)dev->internal_start_block ||
	    blk > (int)dev->internal_end_block) {
		yaffs_trace(YAFFS_TRACE_ERROR,
			"**>> yaffs: get_block_info block %d is not valid",
			blk);
		BUG();
	}
	if(dev->block_info == NULL)
		return 0;
	else
        return &dev->block_info[blk - dev->internal_start_block];
}

#endif
