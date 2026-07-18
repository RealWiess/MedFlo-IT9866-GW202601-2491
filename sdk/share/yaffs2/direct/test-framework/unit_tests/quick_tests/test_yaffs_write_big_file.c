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

#include "test_yaffs_write_big_file.h"

static int handle=-1;
static char *file_name = NULL;
int test_yaffs_write_big_file(void)
{
	int output=0;
	int error_code=0;
	handle=test_yaffs_open();
	int x=0;
	
	int file_name_length=20000;
	if (handle<0){
		printf("failed to open file\n");
		return -1;
	}
	output=yaffs_lseek(handle,0x7FFFFF00, SEEK_SET);
	if (output<0){
		printf("failed to seek\n");
		return -1;
	}
	file_name = malloc(file_name_length);
	if(!file_name){
		printf("unable to create file text\n");
		return -1;
	}
	
	strcpy(file_name,YAFFS_MOUNT_POINT);
	for (x=strlen(YAFFS_MOUNT_POINT); x<file_name_length -1; x++){
		file_name[x]='a';
	}
	file_name[file_name_length-2]='\0';



	if (handle>=0){
		output= yaffs_write(handle, file_name, file_name_length-1);
		if (output<0){
			error_code=yaffs_get_error();
			//printf("EISDIR def %d, Error code %d\n", ENOTDIR,error_code);
			if (abs(error_code)==EINVAL){
				return 1;
			} else {
				printf("different error than expected\n");
				return -1;
			}
		} else {
			printf("wrote a large amount of text to a file.(which is a bad thing)\n");
			return -1;
		}

	} else {
		printf("error opening file\n");
		return -1;
	}
	
}

int test_yaffs_write_big_file_clean(void)
{
	int output=1;
	if(file_name){
		free(file_name);
		file_name = NULL;
	}

	if (handle>=0){	
		output=yaffs_close(handle);
	}

	
	output= yaffs_truncate(FILE_PATH,FILE_SIZE );	
	if (output>=0){
		output=test_yaffs_write();
		if (output>=0){
			return test_yaffs_write_clean();
		} else {
			printf("failed to write to file\n");
			return -1;
		}
	} else {
		printf("failed to truncate file\n");
		return -1;
	}

}
