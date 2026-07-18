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

#include "test_yaffs_rename_dir.h"

/*creates a new directory and renames it*/
int test_yaffs_rename_dir(void)
{
	int output=0;


	if (0 !=  yaffs_access(DIR_PATH,0)) {
		output = yaffs_mkdir(DIR_PATH,S_IREAD|S_IWRITE);
		if (output < 0) {
			print_message("failed to create file\n",2);
			return -1;
		}
	}
	output = yaffs_rename( DIR_PATH , RENAME_DIR_PATH);
	return output;	
}


int test_yaffs_rename_dir_clean(void)
{
	int output = 0;
	if (0 ==  yaffs_access(RENAME_DIR_PATH,0)) {
		output = yaffs_rename(RENAME_DIR_PATH,DIR_PATH);
		if (output < 0) {
			print_message("failed to rename the file\n",2);
			return -1;
		}
	}
	return 1;

}

