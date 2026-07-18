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

/* This is used to pack YAFFS1 tags, not YAFFS2 tags. */

#ifndef __YAFFS_PACKEDTAGS1_H__
#define __YAFFS_PACKEDTAGS1_H__

#include "yaffs_guts.h"

struct yaffs_packed_tags1 {
	u32 chunk_id:20;
	u32 serial_number:2;
	u32 n_bytes:10;
	u32 obj_id:18;
	u32 ecc:12;
	u32 deleted:1;
	u32 unused_stuff:1;
	unsigned should_be_ff;

};

void yaffs_pack_tags1(struct yaffs_packed_tags1 *pt,
		      const struct yaffs_ext_tags *t);
void yaffs_unpack_tags1(struct yaffs_ext_tags *t,
			const struct yaffs_packed_tags1 *pt);
#endif
