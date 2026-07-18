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

#include "test_yaffs_dup.h"

static int handle = -1;
static int handle2 = -1;
int test_yaffs_dup(void)
{
	handle = yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE);
	if (handle >=0){
		handle2 =yaffs_dup(handle);
		return handle2;
	}
	return handle;	
}


int test_yaffs_dup_clean(void)
{
	int output1=1;
	int output2=1;
	if (handle >= 0){
		output1= yaffs_close(handle);
	} 
	if (handle2 >= 0){
		output2 =yaffs_close(handle2);
	}
	return (output1 && output2);	
}
