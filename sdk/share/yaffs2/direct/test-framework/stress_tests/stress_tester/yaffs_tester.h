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

#ifndef __YAFFS_TESTER_H__
	#define __YAFFS_TESTER_H__

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "yaffsfs.h"	/* it is in "yaffs2/direct/" link it in the Makefile */
#include "message_buffer.h"
#include "error_handler.h"
	
#define MAX_NUMBER_OF_OPENED_HANDLES 70
#define MAX_FILE_NAME_SIZE 51

typedef struct handle_regster_template{
	int handle[MAX_NUMBER_OF_OPENED_HANDLES];
	char path[MAX_NUMBER_OF_OPENED_HANDLES][100];
	int number_of_open_handles;
}handle_regster;


void init(char *yaffs_test_dir,char *yaffs_mount_dir,int argc, char *argv[]);	/*sets up yaffs and mounts yaffs */
void test(char *yaffs_test_dir);				/*contains the test code*/
void generate_random_string(char *ptr,int length_of_str);				/*generates a random string of letters to be used for a name*/
void join_paths(char *path1,char *path2,char *newpath );
void copy_array(char *from,char *to, unsigned int from_offset,unsigned int to_offset);
void stat_file(char *path);
void write_to_random_file(handle_regster *P_open_handles_array);
void close_random_file(handle_regster *P_open_handles_array);

void truncate_random_file(handle_regster *P_open_handles_array);
#endif
