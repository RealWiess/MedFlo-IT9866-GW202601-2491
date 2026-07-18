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

#ifndef __YAFFS_ATTRIBS_H__
#define __YAFFS_ATTRIBS_H__

#include "yaffs_guts.h"

void yaffs_load_attribs(struct yaffs_obj *obj, struct yaffs_obj_hdr *oh);
void yaffs_load_attribs_oh(struct yaffs_obj_hdr *oh, struct yaffs_obj *obj);
void yaffs_attribs_init(struct yaffs_obj *obj, u32 gid, u32 uid, u32 rdev);
void yaffs_load_current_time(struct yaffs_obj *obj, int do_a, int do_c);
int yaffs_set_attribs(struct yaffs_obj *obj, struct iattr *attr);
int yaffs_get_attribs(struct yaffs_obj *obj, struct iattr *attr);

#endif
