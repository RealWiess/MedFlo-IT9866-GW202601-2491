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

#include "yaffs_attribs.h"


void yaffs_load_attribs(struct yaffs_obj *obj, struct yaffs_obj_hdr *oh)
{

#ifdef CONFIG_YAFFS_WINCE
	obj->win_atime[0] = oh->win_atime[0];
	obj->win_ctime[0] = oh->win_ctime[0];
	obj->win_mtime[0] = oh->win_mtime[0];
	obj->win_atime[1] = oh->win_atime[1];
	obj->win_ctime[1] = oh->win_ctime[1];
	obj->win_mtime[1] = oh->win_mtime[1];
#else
	obj->yst_uid = oh->yst_uid;
	obj->yst_gid = oh->yst_gid;

        obj->yst_ctime = yaffs_oh_ctime_fetch(oh);
        obj->yst_mtime = yaffs_oh_mtime_fetch(oh);
        obj->yst_atime = yaffs_oh_atime_fetch(oh);

	obj->yst_rdev = oh->yst_rdev;
#endif
}


void yaffs_load_attribs_oh(struct yaffs_obj_hdr *oh, struct yaffs_obj *obj)
{
#ifdef CONFIG_YAFFS_WINCE
	oh->win_atime[0] = obj->win_atime[0];
	oh->win_ctime[0] = obj->win_ctime[0];
	oh->win_mtime[0] = obj->win_mtime[0];
	oh->win_atime[1] = obj->win_atime[1];
	oh->win_ctime[1] = obj->win_ctime[1];
	oh->win_mtime[1] = obj->win_mtime[1];
#else
	oh->yst_uid = obj->yst_uid;
	oh->yst_gid = obj->yst_gid;

        yaffs_oh_ctime_load(obj, oh);
        yaffs_oh_mtime_load(obj, oh);
        yaffs_oh_atime_load(obj, oh);

	oh->yst_rdev = obj->yst_rdev;
#endif
}

void yaffs_attribs_init(struct yaffs_obj *obj, u32 gid, u32 uid, u32 rdev)
{

#ifdef CONFIG_YAFFS_WINCE
	yfsd_win_file_time_now(obj->win_atime);
	obj->win_ctime[0] = obj->win_mtime[0] = obj->win_atime[0];
	obj->win_ctime[1] = obj->win_mtime[1] = obj->win_atime[1];

#else
	yaffs_load_current_time(obj, 1, 1);
	obj->yst_rdev = rdev;
	obj->yst_uid = uid;
	obj->yst_gid = gid;
#endif
}

void yaffs_load_current_time(struct yaffs_obj *obj, int do_a, int do_c)
{
#ifdef CONFIG_YAFFS_WINCE
	yfsd_win_file_time_now(obj->win_atime);
	obj->win_ctime[0] = obj->win_mtime[0] =
	    obj->win_atime[0];
	obj->win_ctime[1] = obj->win_mtime[1] =
	    obj->win_atime[1];

#else

	obj->yst_mtime = Y_CURRENT_TIME;
	if (do_a)
		obj->yst_atime = obj->yst_mtime;
	if (do_c)
		obj->yst_ctime = obj->yst_mtime;
#endif
}
