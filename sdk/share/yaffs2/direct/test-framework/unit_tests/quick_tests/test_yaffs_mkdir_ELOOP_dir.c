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

#include "test_yaffs_mkdir_ELOOP_dir.h"

static int output = -1;

int test_yaffs_mkdir_ELOOP_dir(void)
{
	int error_code = 0;

	if (set_up_ELOOP()<0){
		print_message("failed to setup symlinks\n",2);
		return -1;
	}

	output = yaffs_mkdir(ELOOP_PATH "/file",O_CREAT | O_RDWR);
	if (output < 0){
		error_code = yaffs_get_error();
		if (abs(error_code) == ELOOP){
			return 1;
		} else {
			print_message("different error than expected\n", 2);
			return -1;
		}
	} else {
		print_message("created a new directory on top of an non-existing directory (which is a bad thing)\n", 2);
		return -1;
	}
}


int test_yaffs_mkdir_ELOOP_dir_clean(void)
{
	if (output >= 0){
		return yaffs_rmdir(DIR_PATH);
	} else {
		return 1;	
	}
}

