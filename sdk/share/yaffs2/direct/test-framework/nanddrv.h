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

#ifndef __NAND_DRIVER_H__
#define __NAND_DRIVER_H__
#include "nand_chip.h"

struct nanddrv_transfer {
	unsigned char *buffer;
	int offset;
	int nbytes;
};

int nanddrv_read_tr(struct nand_chip *this, int page,
		struct nanddrv_transfer *tr, int n_tr);
int nanddrv_write_tr(struct nand_chip *this, int page,
		struct nanddrv_transfer *tr, int n_tr);
int nanddrv_erase(struct nand_chip *this, int block);

#endif

