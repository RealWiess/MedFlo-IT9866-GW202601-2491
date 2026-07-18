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

#include "test_yaffs_rename_file_to_dir.h"


int test_yaffs_rename_file_to_dir(void)
{

	int output=0;
	int error_code=0;
	if (yaffs_close(yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE))==-1){
		print_message("failed to create file\n",1);
		return -1;
	}

	if (0 !=  yaffs_access(RENAME_DIR_PATH,0)) {
		output = yaffs_mkdir(RENAME_DIR_PATH,S_IWRITE | S_IREAD);
		if (output < 0) {
			print_message("failed to create directory\n",2);
			return -1;
		}
	}
	output=yaffs_rename(FILE_PATH,RENAME_DIR_PATH);
	if (output==-1){
		error_code=yaffs_get_error();
		if (abs(error_code)==EISDIR){
			return 1;
		} else {
			print_message("different error than expected\n",2);
			return -1;
		}
	} else {
		print_message("renamed a file over a directory (which is a bad thing)\n",2);
		return -1;
	}
	return 1;

}


int test_yaffs_rename_file_to_dir_clean(void)
{
	
	return 1;

}

