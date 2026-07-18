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


#include "yaffs_nandsim_file.h"
#include "yaffs_nand_drv.h"
#include "nandsim_file.h"
#include "nand_chip.h"
#include "yaffs_guts.h"
#include <stddef.h>


struct yaffs_dev *yaffs_nandsim_install_drv(const char *dev_name,
					const char *backing_file_name,
					int n_blocks,
					int n_caches,
					int inband_tags)
{
	struct yaffs_dev *dev;
	char *name_copy = NULL;
	struct yaffs_param *param;
	struct nand_chip *chip = NULL;



	dev = malloc(sizeof(struct yaffs_dev));
	name_copy = strdup(dev_name);

	if(!dev || !name_copy)
		goto fail;

	memset(dev, 0, sizeof(*dev));
	chip = nandsim_file_init(backing_file_name, n_blocks, 64, 2048, 64, 0);
	if(!chip)
		goto fail;

	param = &dev->param;

	param->name = name_copy;

	param->total_bytes_per_chunk = chip->data_bytes_per_page;
	param->chunks_per_block = chip->pages_per_block;
	param->n_reserved_blocks = 5;
	param->start_block = 0; // First block
	param->end_block = n_blocks - 1; // Last block
	param->is_yaffs2 = 1;
	param->use_nand_ecc = 1;
	param->n_caches = n_caches;
	param->stored_endian = 2;
	param->inband_tags = inband_tags;

	if(yaffs_nand_install_drv(dev, chip) != YAFFS_OK)
		goto fail;

	/* The yaffs device has been configured, install it into yaffs */
	yaffs_add_device(dev);

	return dev;

fail:
	free(dev);
	free(name_copy);
	return NULL;
}
