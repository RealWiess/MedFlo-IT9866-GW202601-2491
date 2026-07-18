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

/*this is no longer relevent because it is not possiable to read -1 bytes*/

#include "test_yaffs_read_EINVAL.h"
#include "test_yaffs_write.h"

static int handle = -1;
static char *file_name = NULL;

int test_yaffs_read_EINVAL(void)
{
	int error_code = 0;
	char text[2000000];
	int output=0;	
	/*if (yaffs_close(yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE))==-1){
		print_message("failed to create file before remounting\n",1);
		return -1;
	}*/
	handle=yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE);

	if (handle<0){
		print_message("could not open file\n",2);
		return -1;
	}	

	/*there needs a large amout of test in the file in order to trigger EINVAL */
	output=test_yaffs_read_EINVAL_init();
	if (output<0){
		print_message("could not write text to the file\n",2);
		return -1; 
	}

	if (handle>=0){
		output=yaffs_read(handle, text, -1);
		if (output<0){ 
			error_code=yaffs_get_error();
			if (abs(error_code)== EINVAL){
				return 1;
			} else {
				print_message("different error than expected\n",2);
				return -1;
			}
		} else{
			print_message("read a negative number of bytes (which is a bad thing)\n",2);
			return -1;
		}
	} else {
		print_message("error opening file\n",2);
		return -1;
	}
}

int test_yaffs_read_EINVAL_clean(void)
{
	int output=0;
	if (handle>=0){
		if(file_name){
			free(file_name);
			file_name = NULL;
		}

		
		output= yaffs_truncate(FILE_PATH,FILE_SIZE );	
		if (output>=0){
			output=test_yaffs_write();
			if (output<0){
				print_message("failed to write to file\n",2);
				return -1;
			} else {
				output=test_yaffs_write_clean();
				if (output<0){
					print_message("failed to clean the write_to_file function\n",2);
				}
			}
		} else {
			print_message("failed to truncate file\n",2);
			return -1;
		}

		if(output>=0){
			output=yaffs_close(handle);
			if (output>=0){
				return 1;
			} else {
				print_message("could not close the handle\n",2);
				return -1;
			}
		} else {
			print_message("failed to fix the file\n",2);
			return -1;
		}
	} else {
		print_message("no open handle\n",2);
		return -1;	
	}
}

int test_yaffs_read_EINVAL_init(void)
{
	int output=0;
	int x=0;
	
	int file_name_length=1000000;

	file_name = malloc(file_name_length);
	if(!file_name){
		print_message("unable to create file text\n",2);
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
			print_message("could not write text to file\n",2);
			return -1;
		} else {
			
			return 1;
		}

	} else {
		print_message("error opening file\n",2);
		return -1;
	}
	
}


