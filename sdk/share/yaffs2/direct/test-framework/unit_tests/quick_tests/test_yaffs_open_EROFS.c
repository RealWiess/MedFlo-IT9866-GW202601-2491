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

#include "test_yaffs_open_EROFS.h"

static int handle = -1;

int test_yaffs_open_EROFS(void)
{
	int error_code = 0;
	yaffs_unlink(FILE_PATH);
	EROFS_setup();

	handle = yaffs_open(FILE_PATH, O_CREAT  ,S_IREAD);
	if (handle == -1){
		error_code = yaffs_get_error();
		if (abs(error_code) == EROFS){
			return 1;
		} else {
			print_message("different error than expected\n",2);
			return -1;
		}
	} else {
		print_message("non existant file opened.(which is a bad thing)\n",2);
		return -1;
	}
}

int test_yaffs_open_EROFS_clean(void)
{
	int output =1;
	if (handle >=0){
		handle= yaffs_close(handle);
	} 
	return (EROFS_clean() && output);
	
}

