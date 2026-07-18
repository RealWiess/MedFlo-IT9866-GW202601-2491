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
 * yaffscfg2k.c  The configuration for the "direct" use of yaffs.
 *
 * This file is intended to be modified to your requirements.
 * There is no need to redistribute this file.
 */

#include "yaffscfg.h"
#include "yaffs_guts.h"
#include "yaffsfs.h"
#include "yaffs_fileem2k.h"
#include "yaffs_nandemul2k.h"
#include "yaffs_trace.h"
#include "yaffs_osglue.h"
#include "yaffs_nandsim_file.h"


#include <errno.h>
#ifndef CFG_YAFFS2_LOG
unsigned int yaffs_trace_mask = 0;
#else
unsigned int yaffs_trace_mask =
	YAFFS_TRACE_SCAN |
	/*YAFFS_TRACE_GC |*/
	YAFFS_TRACE_ERASE |
	YAFFS_TRACE_ERROR |
	/*YAFFS_TRACE_TRACING |*/
	YAFFS_TRACE_ALLOCATE |
	YAFFS_TRACE_BAD_BLOCKS |
	YAFFS_TRACE_VERIFY |
	YAFFS_TRACE_CHECKPOINT |
	0;
#endif


/* Configure the devices that will be used */

//#include "yaffs_flashif2.h"
#include "yaffs_fileem2k.h"
#include "yaffs_m18_drv.h"
#include "yaffs_nor_drv.h"
#include "yaffs_nand_drv.h"

int yaffs_start_up(void)
{
	static int start_up_called = 0;

	if(start_up_called)
		return 0;
	start_up_called = 1;

	/* Call the OS initialisation (eg. set up lock semaphore */
	yaffsfs_OSInitialisation();

	/* Install the various devices and their device drivers */
	//YC mark
	yflash2_install_drv("yflash2", 64, 2048, 64, 0, 255); //32mb size
	//yaffs_m18_install_drv("M18-1");
	//yaffs_nor_install_drv("nor");
	yaffs_nandsim_install_drv("nand", "emfile-nand", 256, 4, 1);
	yaffs_nandsim_install_drv("nand128MB", "emfile-nand128MB", 1024, 4, 1);

	return 0;
}



