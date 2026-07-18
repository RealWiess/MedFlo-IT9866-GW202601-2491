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

#include "test_b.h"

void test_b(void *x)
{
	struct bovver_context *bc = (struct bovver_context *)x;
	int n = rand() % 20;
	
	bc->cycle++;

	if(!bc->dirH)
		bc->dirH = yaffs_opendir(bc->baseDir);

	if(!bc->dirH)
		return;

	if(n == 0){
		yaffs_closedir(bc->dirH);
		bc->dirH = NULL;
	} else {
		while(n > 1){
			n--;
			yaffs_readdir(bc->dirH);
		}
	}
}

