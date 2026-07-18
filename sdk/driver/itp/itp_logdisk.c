/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL Log to disk functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#define ITP_FAT_ACCEL
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "itp_cfg.h"

static FILE* itpLogDiskFile;

static int LogDiskPutchar(int c)
{
    if (itpLogDiskFile)
    {
        char ch = (char)c;
        int ret = fwrite(&ch, 1, 1, itpLogDiskFile);
        fflush(itpLogDiskFile);
        return (ret == 1) ? c : EOF;
    }
    return EOF;
}

void LogDiskInit(const char* path)
{
    itpLogDiskFile = fopen(path, "w");
#if defined(CFG_FS_FAT) || defined(CFG_FS_LFS) || defined(CFG_FS_YAFFS2)
		if (!itpLogDiskFile)
		{
		    // init fat on this task
		    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
		    itpLogDiskFile = fopen(path, "w");
		}
#endif

    if (itpLogDiskFile)
	    ithPutcharFunc = LogDiskPutchar;
}

static int LogDiskWrite(int file, char *ptr, int len, void* info)
{
    if (itpLogDiskFile)
    {
        int ret = fwrite(ptr, 1, len, itpLogDiskFile);
        fflush(itpLogDiskFile);
        return ret;
    }
    return EOF;
}

static int LogDiskIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
	case ITP_IOCTL_INIT:
        LogDiskInit((const char*)ptr);
        break;

    default:
        errno = (ITP_DEVICE_LOGDISK << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceLogDisk =
{
    ":logdisk",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    LogDiskWrite,
    itpLseekDefault,
    LogDiskIoctl,
    NULL
};
