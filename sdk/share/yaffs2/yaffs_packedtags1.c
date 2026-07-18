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

#include "yaffs_packedtags1.h"
#include "yportenv.h"

static const u8 all_ff[20] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};

void yaffs_pack_tags1(struct yaffs_packed_tags1 *pt,
		      const struct yaffs_ext_tags *t)
{
	pt->chunk_id = t->chunk_id;
	pt->serial_number = t->serial_number;
	pt->n_bytes = t->n_bytes;
	pt->obj_id = t->obj_id;
	pt->ecc = 0;
	pt->deleted = (t->is_deleted) ? 0 : 1;
	pt->unused_stuff = 0;
	pt->should_be_ff = 0xffffffff;
}

void yaffs_unpack_tags1(struct yaffs_ext_tags *t,
			const struct yaffs_packed_tags1 *pt)
{
	if (memcmp(all_ff, pt, sizeof(struct yaffs_packed_tags1))) {
		t->block_bad = 0;
		if (pt->should_be_ff != 0xffffffff)
			t->block_bad = 1;
		t->chunk_used = 1;
		t->obj_id = pt->obj_id;
		t->chunk_id = pt->chunk_id;
		t->n_bytes = pt->n_bytes;
		t->ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
		t->is_deleted = (pt->deleted) ? 0 : 1;
		t->serial_number = pt->serial_number;
	} else {
		memset(t, 0, sizeof(struct yaffs_ext_tags));
	}
}
