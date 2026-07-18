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

#include "test_yaffs_open_ENAMETOOLONG.h"



static int handle = -1;

int test_yaffs_open_ENAMETOOLONG(void)
{
	int x = 0;
	int error_code = 0;
	int file_name_length = 1000000;
	char file_name[file_name_length];

	strcat(file_name,YAFFS_MOUNT_POINT);
	for (x = strlen(YAFFS_MOUNT_POINT); x<file_name_length -1; x++){
		file_name[x] = 'a';
	}
	file_name[file_name_length-2]='\0';
	handle = yaffs_open(file_name, O_CREAT | O_TRUNC| O_RDWR ,FILE_MODE );

	if (handle == -1){
		error_code = yaffs_get_error();

		if (abs(error_code) == ENAMETOOLONG){
			return 1;
		} else {
			print_message("different error than expected\n",2);
			return -1;
		}
	} else {
		//printf("handle %d \n",handle);
		print_message("non existant file opened.(which is a bad thing)\n", 2);
		return -1;
	}
}
int test_yaffs_open_ENAMETOOLONG_clean(void)
{
	if (handle >=0){
		return yaffs_close(handle);
	}
	else {
		return 1;	/* the file failed to open so there is no need to close it*/
	}
}

