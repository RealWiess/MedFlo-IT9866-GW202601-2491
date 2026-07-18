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

#ifndef __YAFFS_VERIFY_H__
#define __YAFFS_VERIFY_H__

#include "yaffs_guts.h"

void yaffs_verify_blk(struct yaffs_dev *dev, struct yaffs_block_info *bi,
		      int n);
void yaffs_verify_collected_blk(struct yaffs_dev *dev,
				struct yaffs_block_info *bi, int n);
void yaffs_verify_blocks(struct yaffs_dev *dev);

void yaffs_verify_oh(struct yaffs_obj *obj, struct yaffs_obj_hdr *oh,
		     struct yaffs_ext_tags *tags, int parent_check);
void yaffs_verify_file(struct yaffs_obj *obj);
void yaffs_verify_link(struct yaffs_obj *obj);
void yaffs_verify_symlink(struct yaffs_obj *obj);
void yaffs_verify_special(struct yaffs_obj *obj);
void yaffs_verify_obj(struct yaffs_obj *obj);
void yaffs_verify_objects(struct yaffs_dev *dev);
void yaffs_verify_obj_in_dir(struct yaffs_obj *obj);
void yaffs_verify_dir(struct yaffs_obj *directory);
void yaffs_verify_free_chunks(struct yaffs_dev *dev);

int yaffs_verify_file_sane(struct yaffs_obj *obj);

int yaffs_skip_verification(struct yaffs_dev *dev);

#endif
