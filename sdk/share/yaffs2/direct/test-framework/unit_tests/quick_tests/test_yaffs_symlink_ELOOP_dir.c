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

#include "test_yaffs_symlink_ELOOP_dir.h"

static int output = 0;

int test_yaffs_symlink_ELOOP_dir(void)
{
	int error_code = 0;
	if (set_up_ELOOP()<0){
		print_message("failed to setup symlinks\n",2);
		return -1;
	}

	output = yaffs_symlink(FILE_PATH,ELOOP_PATH "/file");
	if (output<0){ 
		error_code=yaffs_get_error();
		if (abs(error_code)==ELOOP){
			return 1;
		} else {
			print_message("returned error does not match the the expected error\n",2);
			return -1;
		}
	} else {
		print_message("created a symlink in a non-existing directory (which is a bad thing)\n",2);
		return -1;
	}	

}

int test_yaffs_symlink_ELOOP_dir_clean(void)
{
	if (output >= 0){
		return yaffs_unlink(SYMLINK_PATH);
	} else {
		return 1;	/* the file failed to open so there is no need to close it */
	}
}



