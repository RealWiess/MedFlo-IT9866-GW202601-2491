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
 * yaffs_ramdisk.h: yaffs ram disk component
 */

#ifndef __YAFFS_RAMDISK_H__
#define __YAFFS_RAMDISK_H__


#include "yaffs_guts.h"
int yramdisk_erase(struct yaffs_dev *dev, int blockNumber);
int yramdisk_wr_chunk(struct yaffs_dev *dev,int nand_chunk,const u8 *data, const struct yaffs_ext_tags *tags);
int yramdisk_rd_chunk(struct yaffs_dev *dev,int nand_chunk, u8 *data, struct yaffs_ext_tags *tags);
int yramdisk_initialise(struct yaffs_dev *dev);
int yramdisk_mark_block_bad(struct yaffs_dev *dev,int blockNumber);
int yramdisk_query_block(struct yaffs_dev *dev, int block_no, enum yaffs_block_state *state, int *seq_number);
#endif
