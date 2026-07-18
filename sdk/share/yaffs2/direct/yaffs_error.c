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

#include "yaffsfs.h"

struct error_entry {
	int code;
	const char *text;
};

static const struct error_entry error_list[] = {
	{ ENOMEM , "ENOMEM" },
	{ EBUSY , "EBUSY"},
	{ ENODEV , "ENODEV"},
	{ EINVAL , "EINVAL"},
	{ EBADF , "EBADF"},
	{ EACCES , "EACCES"},
	{ EXDEV , "EXDEV" },
	{ ENOENT , "ENOENT"},
	{ ENOSPC , "ENOSPC"},
	{ ERANGE , "ERANGE"},
	{ ENODATA, "ENODATA"},
	{ ENOTEMPTY, "ENOTEMPTY"},
	{ ENAMETOOLONG, "ENAMETOOLONG"},
	{ ENOMEM , "ENOMEM"},
	{ EEXIST , "EEXIST"},
	{ ENOTDIR , "ENOTDIR"},
	{ EISDIR , "EISDIR"},
	{ ENFILE, "ENFILE"},
	{ EROFS, "EROFS"},
	{ EFAULT, "EFAULT"},
	{ 0, NULL }
};

const char *yaffs_error_to_str(int err)
{
	const struct error_entry *e = error_list;

	if (err < 0)
		err = -err;

	while (e->code && e->text) {
		if (err == e->code)
			return e->text;
		e++;
	}
	return "Unknown error code";
}
