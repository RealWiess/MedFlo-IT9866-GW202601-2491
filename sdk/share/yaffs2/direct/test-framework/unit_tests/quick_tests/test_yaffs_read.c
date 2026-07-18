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

#include "test_yaffs_read.h"

static int handle=-1;

int test_yaffs_read(void)
{
	char text[2000] = "\0";
	int output=0;

	handle = test_yaffs_open();
	if (handle>=0){
		output=yaffs_read(handle, text, FILE_TEXT_NBYTES);
		if (output>=0){ 
			return 1;
		} else{
			print_message("error reading file\n", 2);
			return -1;
		}
	} else {
		print_message("error opening file\n", 2);
		return -1;
	}
}

int test_yaffs_read_clean(void)
{
	if (handle>=0){
		return yaffs_close(handle);
	}
	else {
		return 1; /* no handle was opened so there is no need to close a handle */
	}	
}
