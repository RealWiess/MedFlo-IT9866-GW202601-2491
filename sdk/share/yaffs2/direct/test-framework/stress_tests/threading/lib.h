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

#ifndef __lib_h__
#define __lib_h__

#define BOVVER_HANDLES 10
#include "yaffsfs.h"
struct bovver_context {
	int bovverType;
	int threadId;
	char baseDir[200];
	int h[BOVVER_HANDLES];
	yaffs_DIR *dirH;
	int opMax;
	int op;
	int cycle;
};

int get_counter(int x);
void set_counter(int x, unsigned int value);
void init_counter(unsigned int size_of_counter);
void number_of_threads(unsigned int num);
unsigned int get_num_of_threads(void);

#endif
