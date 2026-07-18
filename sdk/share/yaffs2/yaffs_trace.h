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

#ifndef __YTRACE_H__
#define __YTRACE_H__

extern unsigned int yaffs_trace_mask;
extern unsigned int yaffs_wr_attempts;

/*
 * Tracing flags.
 * The flags masked in YAFFS_TRACE_ALWAYS are always traced.
 */

#define YAFFS_TRACE_OS			0x00000002
#define YAFFS_TRACE_ALLOCATE		0x00000004
#define YAFFS_TRACE_SCAN		0x00000008
#define YAFFS_TRACE_BAD_BLOCKS		0x00000010
#define YAFFS_TRACE_ERASE		0x00000020
#define YAFFS_TRACE_GC			0x00000040
#define YAFFS_TRACE_WRITE		0x00000080
#define YAFFS_TRACE_TRACING		0x00000100
#define YAFFS_TRACE_DELETION		0x00000200
#define YAFFS_TRACE_BUFFERS		0x00000400
#define YAFFS_TRACE_NANDACCESS		0x00000800
#define YAFFS_TRACE_GC_DETAIL		0x00001000
#define YAFFS_TRACE_SCAN_DEBUG		0x00002000
#define YAFFS_TRACE_MTD			0x00004000
#define YAFFS_TRACE_CHECKPOINT		0x00008000

#define YAFFS_TRACE_VERIFY		0x00010000
#define YAFFS_TRACE_VERIFY_NAND		0x00020000
#define YAFFS_TRACE_VERIFY_FULL		0x00040000
#define YAFFS_TRACE_VERIFY_ALL		0x000f0000

#define YAFFS_TRACE_SYNC		0x00100000
#define YAFFS_TRACE_BACKGROUND		0x00200000
#define YAFFS_TRACE_LOCK		0x00400000
#define YAFFS_TRACE_MOUNT		0x00800000

#define YAFFS_TRACE_ERROR		0x40000000
#define YAFFS_TRACE_BUG			0x80000000
#define YAFFS_TRACE_ALWAYS		0xf0000000

#endif
