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

#include "test_yaffs_stat.h"

int test_yaffs_stat(void)
{
	struct yaffs_stat stat;

	if (yaffs_close(yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE))==-1){
		print_message("failed to create file \n",1);
		return -1;
	}
	return yaffs_stat(FILE_PATH, &stat);
}

int test_yaffs_stat_clean(void)
{
	return 1;
}

int yaffs_test_stat_mode(void)
{
	struct yaffs_stat stat;
	int output=0;
	output=yaffs_stat(FILE_PATH, &stat);
	//printf("output: %d\n",output);
	if (output>=0){
		return stat.st_mode;	
	} else {
		print_message("failed to stat file mode\n",2) ;
		return -1;
	}
}

int yaffs_test_stat_size(void){
	struct yaffs_stat stat;
	int output=0;
	output=yaffs_stat(FILE_PATH, &stat); 
	if (output>=0){
		return stat.st_size;	
	} else {
		print_message("failed to stat file size\n",2) ;
		return -1;
	}
}


