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

#include "nandsim_file.h"
#include "nandstore_file.h"
#include "nandsim.h"
#include <unistd.h>


struct nand_chip *nandsim_file_init(const char *fname,
				int blocks,
				int pages_per_block,
				int data_bytes_per_page,
				int spare_bytes_per_page,
				int bus_width_shift)
{
	struct nand_store *store = NULL;
	struct nand_chip *chip = NULL;

	store = nandstore_file_init(fname, blocks, pages_per_block,
					data_bytes_per_page,
					spare_bytes_per_page);
	if(store)
		chip = nandsim_init(store, bus_width_shift);

	if(chip)
		return chip;

	if(store){
		/* tear down */
	}
	return NULL;
}
