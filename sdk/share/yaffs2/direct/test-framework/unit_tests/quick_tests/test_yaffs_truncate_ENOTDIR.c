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

#include "test_yaffs_truncate_ENOTDIR.h"



int test_yaffs_truncate_ENOTDIR(void)
{
	int error=0;
	int output=0;
	if (yaffs_close(yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE))==-1){
		print_message("failed to create file \n",1);
		return -1;
	}

	output= yaffs_truncate(YAFFS_MOUNT_POINT "/test_dir/foo/file",FILE_SIZE_TRUNCATED );
	if (output<0){
		error=yaffs_get_error();
		if (abs(error)==ENOTDIR){
			return 1;
		} else {
			print_message("received a different error than expected\n",2);
			return -1;
		}
	} else{
		print_message("truncated a nonexisting file\n",2);
		return -1;
	}
}

int test_yaffs_truncate_ENOTDIR_clean(void)
{
	return 1;
}
