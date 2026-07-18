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

#include "test_yaffs_inodecount_NULL.h"



int test_yaffs_inodecount_NULL(void)
{
	int output=0;
	int error=0;
	output = yaffs_inodecount(NULL);
	if (output<0){
		error=yaffs_get_error();
		if (abs(error)==EFAULT){
			return 1;
		} else {
			print_message("different error than expected\n",2);
			return -1;
		}
	} else {
		print_message("got the inodecount of a non existing mountpoint (which is a bad thing)\n",2);
		return -1;
	}
}


int test_yaffs_inodecount_NULL_clean(void)
{
	return 1;
}

