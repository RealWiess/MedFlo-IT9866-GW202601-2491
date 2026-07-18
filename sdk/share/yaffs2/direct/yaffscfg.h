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
 * Header file for using yaffs in an application via
 * a direct interface.
 */


#ifndef __YAFFSCFG_H__
#define __YAFFSCFG_H__


#include "yportenv.h"

#ifdef CONFIG_YAFFS_SMALL_RAM
#define YAFFSFS_N_HANDLES	10
#define YAFFSFS_N_DSC		2
#else
#define YAFFSFS_N_HANDLES	100
#define YAFFSFS_N_DSC		20
#endif


struct yaffsfs_DeviceConfiguration {
	const YCHAR *prefix;
	struct yaffs_dev *dev;
};


#endif

