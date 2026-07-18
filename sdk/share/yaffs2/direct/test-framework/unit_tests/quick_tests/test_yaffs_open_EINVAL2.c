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

/* yaffs will open a file without an error with the creation mode set to 255.*/

#include "test_yaffs_open_EINVAL2.h"

static int handle = -1;

int test_yaffs_open_EINVAL2(void)
{

	handle=yaffs_open(FILE_PATH, O_CREAT | O_RDWR ,255);

	/* yaffs_open does not check the modes passed into it. This means that the file should open */
	if (handle < 0){
		print_message("file not opened with bad creation mode set (which is a bad thing)\n",2);
		return -1;
	} else {
		/* file opened */
		return 1;
	}
}

int test_yaffs_open_EINVAL2_clean(void)
{
	if (handle >= 0){
		return yaffs_close(handle);
	} else {
		return 1;	/* the file failed to open so there is no need to close it*/
	}
}

