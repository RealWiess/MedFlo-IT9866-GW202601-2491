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

#ifndef __NAND_CHIP_H__
#define __NAND_CHIP_H__

#include <stdint.h>

struct nand_chip {
	void *private_data;

	void (*set_ale)(struct nand_chip * this, int high);
	void (*set_cle)(struct nand_chip * this, int high);

	unsigned (*read_cycle)(struct nand_chip * this);
	void (*write_cycle)(struct nand_chip * this, unsigned b);

	int (*check_busy)(struct nand_chip * this);
	void (*idle_fn) (struct nand_chip *this);
	int (*power_check) (struct nand_chip *this);

	int blocks;
	int pages_per_block;
	int data_bytes_per_page;
	int spare_bytes_per_page;
	int bus_width_shift;
};
#endif
