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

#ifndef __YAFFS_YAFFS2_H__
#define __YAFFS_YAFFS2_H__

#include "yaffs_guts.h"

void yaffs_calc_oldest_dirty_seq(struct yaffs_dev *dev);
void yaffs2_find_oldest_dirty_seq(struct yaffs_dev *dev);
void yaffs2_clear_oldest_dirty_seq(struct yaffs_dev *dev,
				   struct yaffs_block_info *bi);
void yaffs2_update_oldest_dirty_seq(struct yaffs_dev *dev, unsigned block_no,
				    struct yaffs_block_info *bi);
int yaffs_block_ok_for_gc(struct yaffs_dev *dev, struct yaffs_block_info *bi);
u32 yaffs2_find_refresh_block(struct yaffs_dev *dev);
int yaffs2_checkpt_required(struct yaffs_dev *dev);
int yaffs_calc_checkpt_blocks_required(struct yaffs_dev *dev);

void yaffs2_checkpt_invalidate(struct yaffs_dev *dev);
int yaffs2_checkpt_save(struct yaffs_dev *dev);
int yaffs2_checkpt_restore(struct yaffs_dev *dev);

int yaffs2_handle_hole(struct yaffs_obj *obj, loff_t new_size);
int yaffs2_scan_backwards(struct yaffs_dev *dev);

#endif
