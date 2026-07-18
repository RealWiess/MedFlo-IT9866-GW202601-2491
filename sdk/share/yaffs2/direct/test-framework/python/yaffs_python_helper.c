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

/*
 * These are some dangly bits that need to be built to wrap up the rest of the yaffs test harness.
 *
 *
 * This is also the place where extra debugging stuff might end up.
 *
 */
 
#include "yaffsfs.h"
#include "yaffs_trace.h"

int simulate_power_failure;
int random_seed;

const char * yaffs_error_to_str(int err);	/*this is not part of yaffs. it is a specialy built file for converting error codes to text*/

int yaffs_print_constants(void)
{
	printf( "O_CREAT........%d\n",O_CREAT);
	printf( "O_RDONLY.......%d\n",O_RDONLY);
	printf( "O_WRONLY.......%d\n",O_WRONLY);
	printf( "O_RDWR.........%d\n",O_RDWR);
	printf( "O_TRUNC........%d\n",O_TRUNC);

	printf( "sizeof(off_t)..%d\n",sizeof(off_t)); 
	return 0;
}

int yaffs_O_CREAT(void) { return O_CREAT;}
int yaffs_O_RDONLY(void) { return O_RDONLY;}
int yaffs_O_WRONLY(void) { return O_WRONLY;}
int yaffs_O_RDWR(void) { return O_RDWR;}
int yaffs_O_TRUNC(void) { return O_TRUNC;}


int yaffs_S_IFMT(void){return S_IFMT;}
int yaffs_S_IFLNK(void){return S_IFLNK;}
int yaffs_S_IFDIR(void){return S_IFDIR;}
int yaffs_S_IFREG(void){return S_IFREG;}
int yaffs_S_IREAD(void){return S_IREAD;}
int yaffs_S_IWRITE(void){return S_IWRITE;}
int yaffs_S_IEXEC(void){return S_IEXEC;}
int yaffs_XATTR_CREATE(void){return XATTR_CREATE;}
int yaffs_XATTR_REPLACE(void){return XATTR_REPLACE;}
