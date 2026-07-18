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

/* This is used to pack YAFFS2 tags, not YAFFS1tags. */

#ifndef __YAFFS_PACKEDTAGS2_H__
#define __YAFFS_PACKEDTAGS2_H__

#include "yaffs_guts.h"
#include "yaffs_ecc.h"

//#pragma pack(1)
struct yaffs_packed_tags2_tags_only {
	unsigned seq_number;
	unsigned obj_id;
	unsigned chunk_id;
	unsigned n_bytes;
};

struct yaffs_packed_tags2 {
	struct yaffs_packed_tags2_tags_only t;
	struct yaffs_ecc_other ecc;
};

/* Full packed tags with ECC, used for oob tags */
void yaffs_pack_tags2(struct yaffs_dev *dev,
		      struct yaffs_packed_tags2 *pt,
		      const struct yaffs_ext_tags *t, int tags_ecc);
void yaffs_unpack_tags2(struct yaffs_dev *dev,
			struct yaffs_ext_tags *t, struct yaffs_packed_tags2 *pt,
			int tags_ecc);

/* Only the tags part (no ECC for use with inband tags */
void yaffs_pack_tags2_tags_only(struct yaffs_dev *dev,
				struct yaffs_packed_tags2_tags_only *pt,
				const struct yaffs_ext_tags *t);
void yaffs_unpack_tags2_tags_only(struct yaffs_dev *dev,
				  struct yaffs_ext_tags *t,
				  struct yaffs_packed_tags2_tags_only *pt);
#endif
