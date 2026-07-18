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


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>


#include "yaffsfs.h"

#include "yaffs_guts.h" /* Only for dumping device innards */
#include "yaffs_endian.h" /*For testing the swap_u64 macro */

extern int yaffs_trace_mask;



int call_all_reldev(struct yaffs_dev *dev)
{
	struct yaffs_stat buf;
	struct yaffs_utimbuf utime;
	unsigned char xbuffer[20];
	char cbuffer[20];

	yaffs_mount_reldev(dev);
	yaffs_open_sharing_reldev(dev, "foo", 0, 0, 0);
	yaffs_open_reldev(dev, "foo", 0, 0);
	yaffs_truncate_reldev(dev, "foo", 99);
	yaffs_unlink_reldev(dev, "foo");
	yaffs_rename_reldev(dev, "foo", "foo_new");
	yaffs_stat_reldev(dev, "foo", &buf);
	yaffs_lstat_reldev(dev, "foo", &buf);
	yaffs_utime_reldev(dev, "foo", &utime);
	yaffs_setxattr_reldev(dev, "foo", "name", xbuffer, 20, 0);
	yaffs_lsetxattr_reldev(dev, "foo", "name", xbuffer, 20, 0);
	yaffs_getxattr_reldev(dev, "foo", "name", xbuffer, 20);
	yaffs_lgetxattr_reldev(dev, "foo", "name", xbuffer, 20);

	yaffs_listxattr_reldev(dev, "foo", cbuffer, 20);
	yaffs_llistxattr_reldev(dev, "foo", cbuffer, 20);
	yaffs_removexattr_reldev(dev, "foo", "name");
	yaffs_lremovexattr_reldev(dev, "foo", "name");

	yaffs_access_reldev(dev, "foo", 0);
	yaffs_chmod_reldev(dev, "foo", 0);
	yaffs_mkdir_reldev(dev, "foo", 0);
	yaffs_rmdir_reldev(dev, "foo");


	yaffs_opendir_reldev(dev, "foo");

	//yaffs_symlink_reldev(dev, "foo", "foolink");
	//yaffs_readlink_reldev(dev, "foo", cbuffer, 20);
	//yaffs_link_reldev(dev, "foo", "foo_new");

	yaffs_mknod_reldev(dev, "foo", 0, 0);
	yaffs_freespace_reldev(dev);
	yaffs_totalspace_reldev(dev);

	yaffs_sync_reldev(dev);
	yaffs_sync_files_reldev(dev);

	yaffs_unmount_reldev(dev);
	yaffs_unmount2_reldev(dev, 1);
	yaffs_remount_reldev(dev, 1, 1);

	return 0;
}


int random_seed;
int simulate_power_failure;

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	call_all_reldev(NULL);

	return 0;
}
