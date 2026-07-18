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

/* generates a EBADF error by closing a handle twice */

#include "test_yaffs_open.h"

static int handle = -1;

int test_yaffs_close_EBADF(void)
{
	int output = 0;
	int error_code = 0;

	handle = test_yaffs_open();
	if (handle >= 0){
		output = yaffs_close(handle);
		if (output >= 0){
			output = yaffs_close(handle);
			if (output < 0){
				error_code = yaffs_get_error();
				if (abs(error_code) == EBADF){
					handle =-1;
					return 1;
				} else {
					print_message("different error than expected\n",2);
					return -1;
				}
			} else {
				print_message("closed the same handle twice. (which is a bad thing)\n", 2);
				return -1;
			}
		} else {
			print_message("failed to close the handle the firs time\n", 2);
			return -1;
		}	
	} else {
		print_message("failed to open file\n", 2);
		return -1;
	}

}


int test_yaffs_close_EBADF_clean(void)
{
	if (handle >= 0){
		printf("handle %d\n",handle);
		return yaffs_close(handle);
	} else {
		return 1;
	}
}

