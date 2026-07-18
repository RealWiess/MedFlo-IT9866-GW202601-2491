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

#include "yaffs_guts.h"
#include "yaffs_attribs.h"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
#define IATTR_UID ia_uid
#define IATTR_GID ia_gid
#else
#define IATTR_UID ia_uid.val
#define IATTR_GID ia_gid.val
#endif

/*
 * Loading attibs from/to object header assumes the object header
 * is in cpu endian.
 */
void yaffs_load_attribs(struct yaffs_obj *obj, struct yaffs_obj_hdr *oh)
{
	obj->yst_uid = oh->yst_uid;
	obj->yst_gid = oh->yst_gid;

	obj->yst_ctime = yaffs_oh_ctime_fetch(oh);
	obj->yst_mtime = yaffs_oh_mtime_fetch(oh);
	obj->yst_atime = yaffs_oh_atime_fetch(oh);

	obj->yst_rdev = oh->yst_rdev;
}

void yaffs_load_attribs_oh(struct yaffs_obj_hdr *oh, struct yaffs_obj *obj)
{
	oh->yst_uid = obj->yst_uid;
	oh->yst_gid = obj->yst_gid;

	yaffs_oh_ctime_load(obj, oh);
	yaffs_oh_mtime_load(obj, oh);
	yaffs_oh_atime_load(obj, oh);

	oh->yst_rdev = obj->yst_rdev;

}

void yaffs_load_current_time(struct yaffs_obj *obj, int do_a, int do_c)
{
	obj->yst_mtime = Y_CURRENT_TIME;
	if (do_a)
		obj->yst_atime = obj->yst_mtime;
	if (do_c)
		obj->yst_ctime = obj->yst_mtime;
}

void yaffs_attribs_init(struct yaffs_obj *obj, u32 gid, u32 uid, u32 rdev)
{
	yaffs_load_current_time(obj, 1, 1);
	obj->yst_rdev = rdev;
	obj->yst_uid = uid;
	obj->yst_gid = gid;
}

static loff_t yaffs_get_file_size(struct yaffs_obj *obj)
{
	YCHAR *alias = NULL;
	obj = yaffs_get_equivalent_obj(obj);

	switch (obj->variant_type) {
	case YAFFS_OBJECT_TYPE_FILE:
		return obj->variant.file_variant.file_size;
	case YAFFS_OBJECT_TYPE_SYMLINK:
		alias = obj->variant.symlink_variant.alias;
		if (!alias)
			return 0;
		return strnlen(alias, YAFFS_MAX_ALIAS_LENGTH);
	default:
		return 0;
	}
}

int yaffs_set_attribs(struct yaffs_obj *obj, struct iattr *attr)
{
	unsigned int valid = attr->ia_valid;

	if (valid & ATTR_MODE)
		obj->yst_mode = attr->ia_mode;
	if (valid & ATTR_UID)
		obj->yst_uid = attr->IATTR_UID;
	if (valid & ATTR_GID)
		obj->yst_gid = attr->IATTR_GID;

	if (valid & ATTR_ATIME)
		obj->yst_atime = Y_TIME_CONVERT(attr->ia_atime);
	if (valid & ATTR_CTIME)
		obj->yst_ctime = Y_TIME_CONVERT(attr->ia_ctime);
	if (valid & ATTR_MTIME)
		obj->yst_mtime = Y_TIME_CONVERT(attr->ia_mtime);

	if (valid & ATTR_SIZE)
		yaffs_resize_file(obj, attr->ia_size);

	yaffs_update_oh(obj, NULL, 1, 0, 0, NULL);

	return YAFFS_OK;
}

int yaffs_get_attribs(struct yaffs_obj *obj, struct iattr *attr)
{
	unsigned int valid = 0;

	attr->ia_mode = obj->yst_mode;
	valid |= ATTR_MODE;
	attr->IATTR_UID = obj->yst_uid;
	valid |= ATTR_UID;
	attr->IATTR_GID = obj->yst_gid;
	valid |= ATTR_GID;

	Y_TIME_CONVERT(attr->ia_atime) = obj->yst_atime;
	valid |= ATTR_ATIME;
	Y_TIME_CONVERT(attr->ia_ctime) = obj->yst_ctime;
	valid |= ATTR_CTIME;
	Y_TIME_CONVERT(attr->ia_mtime) = obj->yst_mtime;
	valid |= ATTR_MTIME;

	attr->ia_size = yaffs_get_file_size(obj);
	valid |= ATTR_SIZE;

	attr->ia_valid = valid;

	return YAFFS_OK;
}
