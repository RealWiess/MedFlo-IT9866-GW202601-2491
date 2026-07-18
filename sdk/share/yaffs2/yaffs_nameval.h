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

#ifndef __NAMEVAL_H__
#define __NAMEVAL_H__

#include "yportenv.h"

struct yaffs_dev;

int nval_del(struct yaffs_dev *dev, char *xb, int xb_size, const YCHAR * name);
int nval_set(struct yaffs_dev *dev,
	     char *xb, int xb_size, const YCHAR * name, const char *buf,
	     int bsize, int flags);
int nval_get(struct yaffs_dev *dev,
	     const char *xb, int xb_size, const YCHAR * name, char *buf,
	     int bsize);
int nval_list(struct yaffs_dev *dev,
	      const char *xb, int xb_size, char *buf, int bsize);
int nval_hasvalues(struct yaffs_dev *dev, const char *xb, int xb_size);
#endif
