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

#include "yaffs_tagsmarshall.h"
#include "yaffs_trace.h"
#include "yaffs_packedtags2.h"
#include <ith/ith_utility.h>
#include "yaffs_nand.h"

static int yaffs_tags_marshall_write(struct yaffs_dev *dev,
				    int nand_chunk, const u8 *data,
				    const struct yaffs_ext_tags *tags)
{
	struct yaffs_packed_tags2 pt;
	int retval;

	int packed_tags_size =
	    dev->param.no_tags_ecc ? sizeof(pt.t) : sizeof(pt);
	void *packed_tags_ptr =
	    dev->param.no_tags_ecc ? (void *)&pt.t : (void *)&pt;

	yaffs_trace(YAFFS_TRACE_MTD,
		"yaffs_tags_marshall_write chunk %d data %p tags %p",
		nand_chunk, data, tags);

	/* For yaffs2 writing there must be both data and tags.
	 * If we're using inband tags, then the tags are stuffed into
	 * the end of the data buffer.
	 */
	if (!data || !tags)
		BUG();
	else if (dev->param.inband_tags) {
		struct yaffs_packed_tags2_tags_only *pt2tp;
		pt2tp =
		    (struct yaffs_packed_tags2_tags_only *)(data +
							dev->
							data_bytes_per_chunk);
		yaffs_pack_tags2_tags_only(dev, pt2tp, tags);
	} else {
		yaffs_pack_tags2(dev, &pt, tags, !dev->param.no_tags_ecc);
	}

	retval = dev->drv.drv_write_chunk_fn(dev, nand_chunk,
			data, dev->param.total_bytes_per_chunk,
			(dev->param.inband_tags) ? NULL : packed_tags_ptr,
			(dev->param.inband_tags) ? 0 : packed_tags_size);

	return retval;
}

static int yaffs_tags_marshall_read(struct yaffs_dev *dev,
				   int nand_chunk, u8 *data,
				   struct yaffs_ext_tags *tags)
{
	int retval = 0;
	int local_data = 0;
	u8 spare_buffer[100];
	enum yaffs_ecc_result ecc_result;

	struct yaffs_packed_tags2 pt;

	int packed_tags_size =
	    dev->param.no_tags_ecc ? sizeof(pt.t) : sizeof(pt);
	void *packed_tags_ptr =
	    dev->param.no_tags_ecc ? (void *)&pt.t : (void *)&pt;

	yaffs_trace(YAFFS_TRACE_MTD,
		"yaffs_tags_marshall_read chunk %d data %p tags %p",
		nand_chunk, data, tags);

	if (dev->param.inband_tags) {
		if (!data) {
			local_data = 1;
			data = yaffs_get_temp_buffer(dev);
		}
	}

	if (dev->param.inband_tags || (data && !tags))
		retval = dev->drv.drv_read_chunk_fn(dev, nand_chunk,
					data, dev->param.total_bytes_per_chunk,
					NULL, 0,
					&ecc_result);
	else if (tags)
		retval = dev->drv.drv_read_chunk_fn(dev, nand_chunk,
					data, dev->param.total_bytes_per_chunk,
					spare_buffer, packed_tags_size,
					&ecc_result);
	else
		BUG();


	if (retval == YAFFS_FAIL) {
        if (tags && ecc_result == YAFFS_ECC_RESULT_UNFIXED) {
            tags->ecc_result = YAFFS_ECC_RESULT_UNFIXED;
            dev->n_ecc_unfixed++;
        }
        
        if (tags && ecc_result == YAFFS_ECC_RESULT_FIXED) {
            if (tags->ecc_result <= YAFFS_ECC_RESULT_NO_ERROR)
                tags->ecc_result = YAFFS_ECC_RESULT_FIXED;
            dev->n_ecc_fixed++;
        }

		if (local_data)
			yaffs_release_temp_buffer(dev, data);

		return YAFFS_FAIL;
	}

	if (dev->param.inband_tags) {
		if (tags) {
			struct yaffs_packed_tags2_tags_only *pt2tp;
			pt2tp =
				(struct yaffs_packed_tags2_tags_only *)
				&data[dev->data_bytes_per_chunk];
			yaffs_unpack_tags2_tags_only(dev, tags, pt2tp);
		}
	} else if (tags) {
		memcpy(packed_tags_ptr, spare_buffer, packed_tags_size);
		yaffs_unpack_tags2(dev, tags, &pt, !dev->param.no_tags_ecc);
	}

	if (local_data)
		yaffs_release_temp_buffer(dev, data);

	if (tags && ecc_result == YAFFS_ECC_RESULT_UNFIXED) {
		tags->ecc_result = YAFFS_ECC_RESULT_UNFIXED;
		dev->n_ecc_unfixed++;
	}

	if (tags && ecc_result == YAFFS_ECC_RESULT_FIXED) {
		if (tags->ecc_result <= YAFFS_ECC_RESULT_NO_ERROR)
			tags->ecc_result = YAFFS_ECC_RESULT_FIXED;
		dev->n_ecc_fixed++;
	}

	if (ecc_result < YAFFS_ECC_RESULT_UNFIXED)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

static int yaffs_tags_marshall_query_block(struct yaffs_dev *dev, int block_no,
			       enum yaffs_block_state *state,
			       u32 *seq_number)
{
	int retval;

	yaffs_trace(YAFFS_TRACE_MTD, "yaffs_tags_marshall_query_block %d",
			block_no);

	retval = dev->drv.drv_check_bad_fn(dev, block_no);

	if (retval== YAFFS_FAIL) {
		yaffs_trace(YAFFS_TRACE_MTD, "block is bad");

		*state = YAFFS_BLOCK_STATE_DEAD;
		*seq_number = 0;
	} else {
		struct yaffs_ext_tags t;

		int ret2 = yaffs_tags_marshall_read(dev,
				    block_no * dev->param.chunks_per_block,
				    NULL, &t);

		if (t.chunk_used) {
			*seq_number = t.seq_number;
			*state = YAFFS_BLOCK_STATE_NEEDS_SCAN;
		} else {
			int erased_ok = 1;
			if(!ret2 || t.ecc_result)
			{
				erased_ok = yaffs_erase_block(dev, block_no);
				
			}
			if(!erased_ok)
			{
				dev->n_erase_failures++;
				yaffs_trace(YAFFS_TRACE_ERROR | YAFFS_TRACE_BAD_BLOCKS,
					"query re-erase failed %d", dev->alloc_block_finder);
				*state = YAFFS_BLOCK_STATE_DEAD;
				*seq_number = 0;
			}
			else
			{
				*seq_number = 0;
				
				*state = YAFFS_BLOCK_STATE_EMPTY;
			}
		}
	}

	yaffs_trace(YAFFS_TRACE_MTD,
		"block query returns  seq %d state %d",
		*seq_number, *state);

	return retval;
}

static int yaffs_tags_marshall_mark_bad(struct yaffs_dev *dev, int block_no)
{
	return dev->drv.drv_mark_bad_fn(dev, block_no);

}


void yaffs_tags_marshall_install(struct yaffs_dev *dev)
{
	if (!dev->param.is_yaffs2)
		return;

	if (!dev->tagger.write_chunk_tags_fn)
		dev->tagger.write_chunk_tags_fn = yaffs_tags_marshall_write;

	if (!dev->tagger.read_chunk_tags_fn)
		dev->tagger.read_chunk_tags_fn = yaffs_tags_marshall_read;

	if (!dev->tagger.query_block_fn)
		dev->tagger.query_block_fn = yaffs_tags_marshall_query_block;

	if (!dev->tagger.mark_bad_fn)
		dev->tagger.mark_bad_fn = yaffs_tags_marshall_mark_bad;

}
