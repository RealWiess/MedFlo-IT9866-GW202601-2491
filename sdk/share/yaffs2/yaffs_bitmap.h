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
 * Chunk bitmap manipulations
 */

#ifndef __YAFFS_BITMAP_H__
#define __YAFFS_BITMAP_H__

#include "yaffs_guts.h"

void yaffs_verify_chunk_bit_id(struct yaffs_dev *dev, int blk, int chunk);
void yaffs_clear_chunk_bits(struct yaffs_dev *dev, int blk);
void yaffs_clear_chunk_bit(struct yaffs_dev *dev, int blk, int chunk);
void yaffs_set_chunk_bit(struct yaffs_dev *dev, int blk, int chunk);
int yaffs_check_chunk_bit(struct yaffs_dev *dev, int blk, int chunk);
int yaffs_still_some_chunks(struct yaffs_dev *dev, int blk);
int yaffs_count_chunk_bits(struct yaffs_dev *dev, int blk);

#endif
