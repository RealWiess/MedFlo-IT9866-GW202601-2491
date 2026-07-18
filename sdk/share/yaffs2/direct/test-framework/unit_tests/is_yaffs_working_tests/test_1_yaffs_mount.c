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


#include "yaffsfs.h"
#define YAFFS_MOUNT_POINT "/yflash2/"
#define FILE_PATH "/yflash2/foo.txt"

int random_seed;
int simulate_power_failure = 0;


int main()
{
	int output = 0;
	int output2 = 0;
	yaffs_start_up();

	printf("\n\n starting test\n");
	yaffs_set_trace(0);
	output = yaffs_mount(YAFFS_MOUNT_POINT);

	if (output>=0){  
		printf("yaffs correctly mounted: %s\n",YAFFS_MOUNT_POINT);
	} else {
		printf("error\n yaffs failed to mount: %s\n with error code %d\n",YAFFS_MOUNT_POINT, yaffs_get_error());

		return (-1);
	}
	//now create a file.
	output = yaffs_open(FILE_PATH,O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
	if (output>=0){  
		printf("yaffs correctly created a the file: %s\n",FILE_PATH);
	} else {
		printf("error\n yaffs failed to create the file: %s\nerror\n",FILE_PATH);
		return (-1);
	}
	output2 = yaffs_close(output);
	if (output2>=0){  
		printf("yaffs correctly closed the file: %s\n",FILE_PATH);
	} else {
		printf("error\n yaffs failed to close the file: %s\nerror\n",FILE_PATH);
		return (-1);
	}
	//unmount and remount the mount point.
	output = yaffs_unmount(YAFFS_MOUNT_POINT);
	if (output>=0){  
		printf("yaffs correctly unmounted: %s\n",YAFFS_MOUNT_POINT);
	} else {
		printf("error\n yaffs failed to unmount: %s\nerror\n",YAFFS_MOUNT_POINT);
		return (-1);
	}
	output = yaffs_mount(YAFFS_MOUNT_POINT);
	if (output>=0){  
		printf("yaffs correctly mounted: %s\n",YAFFS_MOUNT_POINT);
	} else {
		printf("error\n yaffs failed to mount: %s\nerror\n",YAFFS_MOUNT_POINT);
		return (-1);
	}
	//now open the existing file.
	output = yaffs_open(FILE_PATH, O_RDWR, S_IREAD | S_IWRITE);
	if (output>=0){  
		printf("yaffs correctly opened the file: %s\n",FILE_PATH);
	} else {
		printf("error\n yaffs failed to create the file: %s\nerror\n",FILE_PATH);
		return (-1);
	}
	//close the file.
	output2 = yaffs_close(output);
	if (output2>=0){  
		printf("yaffs correctly closed the file: %s\n",FILE_PATH);
	} else {
		printf("error\n yaffs failed to close the file: %s\nerror\n",FILE_PATH);
		return (-1);
	}

	//unmount the mount point.
	output = yaffs_unmount(YAFFS_MOUNT_POINT);
	if (output>=0){  
		printf("yaffs correctly unmounted: %s\n",YAFFS_MOUNT_POINT);
	} else {
		printf("error\n yaffs failed to unmount: %s\nerror\n",YAFFS_MOUNT_POINT);
		return (-1);
	}

	printf("test passed. yay!\n");
	
	return(0);
}
