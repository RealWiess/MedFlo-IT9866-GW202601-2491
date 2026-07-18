/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL HCC functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/syslimits.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <wchar.h>
#include <time.h>
#include <unistd.h>
#include "fat/api/api_fat.h"
#include "itp_cfg.h"
#include "ssp/mmp_axispi.h"
#include "nor/mmp_nor.h"
#if defined(CFG_A_B_DUAL_BOOT_ENABLE) || defined(CFG_UPGRADE_BACKUP_FAT_MBR)
#include "ite/ug.h"
#endif

#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
#ifndef CFG_SUB_PRIVATE_PARTITION
#define CFG_SUB_PRIVATE_PARTITION 0xFF
#endif
#ifndef CFG_SUB_PUBLIC_PARTITION
#define CFG_SUB_PUBLIC_PARTITION 0xFF
#endif
#endif

#ifdef CFG_UPGRADE_BACKUP_FAT_MBR
static bool MBR_check[ITH_DISK_MAX] = {0};
static bool create_partition[ITH_DISK_MAX] = {0};
#endif

extern long fn_gettaskID(void);
#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
static bool fatInited;
#else
bool fatInited;
#endif

// SD driver
extern F_DRIVER *mmcsd_initfunc(unsigned long driver_param);

struct
{
    int sd;                 // sd0 or sd1
    int removable;          // removable or not
    unsigned long reserved; // reserved size
    unsigned long cacheSize;// cache size
    unsigned long pageSize; // cache page size
} sdDrvParams[2] =
{
    {
        0,
    #ifdef CFG_SD0_STATIC
        0,
        CFG_SD0_RESERVED_SIZE,
    #else
        1,
        0,
    #endif // CFG_SD0_STATIC
    #ifdef CFG_SD0_HOST_CACHE_SIZE
        CFG_SD0_HOST_CACHE_SIZE,
    #else
        0,
    #endif
    #ifdef CFG_SD0_HOST_CACHE_PAGE_SIZE
        CFG_SD0_HOST_CACHE_PAGE_SIZE
    #else
        0
    #endif
    },
    {
        1,
    #ifdef CFG_SD1_STATIC
        0,
        CFG_SD1_RESERVED_SIZE,
    #else
        1,
        0,
    #endif // CFG_SD1_STATIC
    #ifdef CFG_SD1_HOST_CACHE_SIZE
        CFG_SD1_HOST_CACHE_SIZE,
    #else
        0,
    #endif
    #ifdef CFG_SD1_HOST_CACHE_PAGE_SIZE
        CFG_SD1_HOST_CACHE_PAGE_SIZE
    #else
        0
    #endif
    },
};


static int FatFormat(int volume);
static int FatUnmount(ITPDisk disk);

// XD driver
extern F_DRIVER *xd_initfunc(unsigned long driver_param);

// NAND driver
extern F_DRIVER *ftl_initfunc(unsigned long driver_param);

// NOR driver
extern F_DRIVER *nor_initfunc(unsigned long driver_param);

#ifdef CFG_NOR_ENABLE
struct
{
    unsigned long reserved;           // reserved size
    unsigned long cacheSize;          // cache size
    bool          deviceMode;         // device mode
    unsigned long partitionOffset;    // partition offset
} norDrvParam =
{
    CFG_NOR_RESERVED_SIZE,
    CFG_NOR_CACHE_SIZE,
    false,
    0
};
#endif // CFG_NOR_ENABLE

// ms driver
#ifdef CFG_MS_ENABLE
extern F_DRIVER *mspro_initfunc(unsigned long driver_param);
#else
#define mspro_initfunc    NULL
#endif

// usb msc driver
#if defined(CFG_MSC_ENABLE)
extern F_DRIVER *msc_initfunc(unsigned long driver_param);
#elif defined(CFG_USBH_CD_MST)
#include "usbhcc/api/api_mdriver_mst.h"
#define msc_initfunc    mst_initfunc
#else
#define msc_initfunc    NULL
#endif

// RAM disk driver
extern F_DRIVER *ram_initfunc(unsigned long driver_param);

typedef struct
{
    F_DRIVER* driver;
    F_DRIVERINIT initfunc;
    unsigned long param;
} Driver;

static Driver driverTable[ITH_DISK_MAX] =
{
    {
        NULL,
#ifdef CFG_SD0_ENABLE
        mmcsd_initfunc,
        (unsigned long)&sdDrvParams[0]
#else
        NULL,
        0
#endif // CFG_SD0_ENABLE
    },
    {
        NULL,
#ifdef CFG_SD1_ENABLE
        mmcsd_initfunc,

        (unsigned long)&sdDrvParams[1]
#else
        NULL,
        0
#endif
    },
    { NULL, NULL, 0 },
    { NULL, mspro_initfunc, 0 },
    {
        NULL,
#ifdef CFG_XD_ENABLE
        xd_initfunc,
#else
        NULL,
#endif
        0
    },
    {
        NULL,
#ifdef CFG_NAND_ENABLE
#ifdef CFG_HAVE_LFS
        NULL,
        0
#else
        ftl_initfunc,
        CFG_NAND_RESERVED_SIZE
#endif
#else
        NULL,
        0
#endif
    },
    {
        NULL,
#ifdef CFG_NOR_ENABLE
        nor_initfunc,
        (unsigned long)&norDrvParam
#else
        NULL,
        0
#endif
    },
    { NULL, msc_initfunc, 0 },
    { NULL, msc_initfunc, 1 },
    { NULL, msc_initfunc, 2 },
    { NULL, msc_initfunc, 3 },
    { NULL, msc_initfunc, 4 },
    { NULL, msc_initfunc, 5 },
    { NULL, msc_initfunc, 6 },
    { NULL, msc_initfunc, 7 },
    { NULL, msc_initfunc, 8 },
    { NULL, msc_initfunc, 9 },
    { NULL, msc_initfunc, 10 },
    { NULL, msc_initfunc, 11 },
    { NULL, msc_initfunc, 12 },
    { NULL, msc_initfunc, 13 },
    { NULL, msc_initfunc, 14 },
    { NULL, msc_initfunc, 15 },
    {
        NULL,
#if defined(CFG_RAMDISK_ENABLE) || defined(CFG_USBD_CD_MST)
        ram_initfunc,
#else
        NULL,
#endif
        F_AUTO_ASSIGN
    },
};

static F_FILE* openFiles[OPEN_MAX];
#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
static bool mount_info[ITH_DISK_MAX][ITP_MAX_PARTITION] = {0};
static uint8_t mount_flag = 0x0U;
#endif

static int FatOpen(const char* name, int flags, int mode, void* info)
{
    F_FILE* file;
    int i;

#ifdef ITP_FAT_UTF8
    const wchar_t* fmode;
    wchar_t buf[PATH_MAX + 1];

    if (flags & O_APPEND)
    {
        if (flags & O_RDWR)
            fmode = L"a+";
        else
            fmode = L"a";
    }
    else if (flags & O_CREAT)
    {
        if (flags & O_RDWR)
            fmode = L"w+";
        else
            fmode = L"w";
    }
    else
    {
        if (flags & O_RDWR)
            fmode = L"r+";
        else
            fmode = L"r";
    }
    mbstowcs(buf, name, PATH_MAX + 1);
    file = f_wopen(buf, fmode);
#else
    const char* fmode;

    if (flags & O_APPEND)
    {
        if (flags & O_RDWR)
            fmode = "a+";
        else
            fmode = "a";
    }
    else if (flags & O_CREAT)
    {
        if (flags & O_RDWR)
            fmode = "w+";
        else
            fmode = "w";
    }
    else
    {
        if (flags & O_RDWR)
            fmode = "r+";
        else
            fmode = "r";
    }
    file = f_open(name, fmode);
#endif // ITP_FAT_UTF8
    if (!file)
    {
        ITP_LOG_WARN("fat open %s fail: %d\n", name, f_getlasterror() );
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | f_getlasterror();
        return -1;
    }

    // find empty slot
    ithEnterCritical();
    for (i = 0; i < OPEN_MAX; i++)
    {
        if (openFiles[i] == NULL)
        {
            openFiles[i] = file;
            ithExitCritical();
            return i;
        }
    }
    ithExitCritical();

    errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | __LINE__;
    return -1;
}

static int FatClose(int file, void* info)
{
    int ret = f_close(openFiles[file]);

    #ifdef  CFG_NAND_ENABLE
    ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
    #endif

    ithEnterCritical();
    openFiles[file] = NULL;
    ithExitCritical();

    if (ret)
    {
        ITP_LOG_ERR("fat close %d fail: %d\n", file, f_getlasterror() );
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatRead(int file, char *ptr, int len, void* info)
{
    int ret;

#ifdef CFG_DBG_STATS_FAT
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
#endif
    ret = f_read(ptr, 1, len, openFiles[file]);

#ifdef CFG_DBG_STATS_FAT
    gettimeofday(&t2, NULL);

    ithEnterCritical();
    itpStatsFat.readTime += itpTimevalDiff(&t1, &t2);
    itpStatsFat.readSize += len;
    ithExitCritical();

#endif // CFG_DBG_STATS_FAT

    return ret;
}

static int FatWrite(int file, char *ptr, int len, void* info)
{
    int ret;

#ifdef CFG_DBG_STATS_FAT
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
#endif
    ret = f_write(ptr, 1, len, openFiles[file]);

#ifdef CFG_DBG_STATS_FAT
    gettimeofday(&t2, NULL);

    ithEnterCritical();
    itpStatsFat.writeTime += itpTimevalDiff(&t1, &t2);
    itpStatsFat.writeSize += len;
    ithExitCritical();

#endif // CFG_DBG_STATS_FAT

    return ret;
}

static int FatLseek(int file, int ptr, int dir, void* info)
{
    int ret = f_seek(openFiles[file], ptr, dir);
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return f_tell(openFiles[file]);
}

static int FatRemove(const char *path)
{
    int ret;

#ifdef ITP_FAT_UTF8
    wchar_t buf[PATH_MAX + 1];

    mbstowcs(buf, path, PATH_MAX + 1);
    ret = f_wdelete(buf);

#else
    ret = f_delete(path);

#endif // ITP_FAT_UTF8

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatRename(const char *oldname, const char *newname)
{
    int ret;

#ifdef ITP_FAT_UTF8
    wchar_t oldbuf[PATH_MAX + 1], newbuf[PATH_MAX + 1];

    mbstowcs(oldbuf, oldname, PATH_MAX + 1);
    mbstowcs(newbuf, newname, PATH_MAX + 1);
    ret = f_wrename(oldbuf, newbuf);

#else
    ret = f_rename(oldname, newname);

#endif // ITP_FAT_UTF8

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatChdir(const char *path)
{
    int ret;

    if (path[1] == ':')
    {
        ret = f_chdrive(toupper(path[0]) - 'A');
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
    }

#ifdef ITP_FAT_UTF8
    {
        wchar_t buf[PATH_MAX + 1];

        mbstowcs(buf, path, PATH_MAX + 1);
        ret = f_wchdir(buf);
    }
#else
    ret = f_chdir(path);

#endif // ITP_FAT_UTF8

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatChmod(const char *path, mode_t mode)
{
    int ret;
    unsigned char attr = 0;

    if ((mode & S_IREAD) == 0)
        attr |= F_ATTR_HIDDEN;

    if ((mode & S_IWRITE) == 0)
        attr |= F_ATTR_READONLY;

    if ((mode & S_IEXEC) == 0)
        attr |= F_ATTR_ARC;

#ifdef ITP_FAT_UTF8
    {
        wchar_t buf[PATH_MAX + 1];

        mbstowcs(buf, path, PATH_MAX + 1);
        ret = f_wsetattr(buf, attr);
    }
#else
    ret = f_setattr(path, attr);

#endif // ITP_FAT_UTF8

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatMkdir(const char *path, mode_t mode)
{
    int ret;

#ifdef ITP_FAT_UTF8
    wchar_t buf[PATH_MAX + 1];

    mbstowcs(buf, path, PATH_MAX + 1);
    ret = f_wmkdir(buf);

#else
    ret = f_mkdir(path);

#endif // ITP_FAT_UTF8

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatStat(const char *path, struct stat *sbuf)
{
    int ret;
    F_STAT s;
    struct tm t;

#ifdef ITP_FAT_UTF8
    wchar_t buf[PATH_MAX + 1];

    mbstowcs(buf, path, PATH_MAX + 1);
    ret = f_wstat(buf, &s);

#else
    ret = f_stat(path, &s);

#endif // ITP_FAT_UTF8

    if (ret == F_ERR_INVALIDNAME && path && strlen(path) == 3 && path[1] == ':' && path[2] == '/')
    {
        memset(sbuf, 0, sizeof(struct stat));
        sbuf->st_dev = ITP_DEVICE_FAT;
        sbuf->st_mode |= S_IREAD;
        sbuf->st_mode |= S_IWRITE;
        sbuf->st_mode |= S_IFDIR;
        return 0;
    }
    else if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }

    sbuf->st_dev    = ITP_DEVICE_FAT;
    sbuf->st_ino    = s.drivenum;

    sbuf->st_mode   = 0;
    if ((s.attr & F_ATTR_HIDDEN) == 0)
        sbuf->st_mode |= S_IREAD;

    if ((s.attr & F_ATTR_READONLY) == 0)
        sbuf->st_mode |= S_IWRITE;

    if ((s.attr & F_ATTR_ARC) == 0)
        sbuf->st_mode |= S_IEXEC;

    if (s.attr & F_ATTR_DIR)
        sbuf->st_mode |= S_IFDIR;
    else
        sbuf->st_mode |= S_IFREG;

    sbuf->st_size   = s.filesize;

    t.tm_sec        = 0;
    t.tm_min        = 0;
    t.tm_hour       = 0;
    t.tm_mday       = (s.lastaccessdate & F_CDATE_DAY_MASK) >> F_CDATE_DAY_SHIFT;
    t.tm_mon        = ((s.lastaccessdate & F_CDATE_MONTH_MASK) >> F_CDATE_MONTH_SHIFT) - 1;
    t.tm_year       = 1980 - 1900 + ((s.lastaccessdate & F_CDATE_YEAR_MASK) >> F_CDATE_YEAR_SHIFT);
    t.tm_isdst      = -1;
    sbuf->st_atime  = mktime(&t);

    t.tm_sec        = ((s.modifiedtime & F_CTIME_SEC_MASK) >> F_CTIME_SEC_SHIFT) << 1;
    t.tm_min        = (s.modifiedtime & F_CTIME_MIN_MASK) >> F_CTIME_MIN_SHIFT;
    t.tm_hour       = (s.modifiedtime & F_CTIME_HOUR_MASK) >> F_CTIME_HOUR_SHIFT;
    t.tm_mday       = (s.modifieddate & F_CDATE_DAY_MASK) >> F_CDATE_DAY_SHIFT;
    t.tm_mon        = ((s.modifieddate & F_CDATE_MONTH_MASK) >> F_CDATE_MONTH_SHIFT) - 1;
    t.tm_year       = 1980 - 1900 + ((s.modifieddate & F_CDATE_YEAR_MASK) >> F_CDATE_YEAR_SHIFT);
    sbuf->st_mtime  = mktime(&t);

    return 0;
}

static int FatStatvfs(const char *path, struct statvfs *sbuf)
{
    int ret;
    int volume = toupper(path[0]) - 'A';
    F_PHY phy;
    F_SPACE space;
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    Driver* driver;
    uint64_t total, free;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);
    driveStatus = &driveStatusTable[volume];
    driver = &driverTable[driveStatus->disk];

    ret = driver->driver->getphy(driver->driver, &phy);
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }

	if (!phy.bytes_per_sector)
		phy.bytes_per_sector = F_DEF_SECTOR_SIZE;

    ret = f_getfreespace(volume, &space);
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }

    sbuf->f_bsize    = phy.bytes_per_sector;                        /* file system block size */
    sbuf->f_frsize   = phy.bytes_per_sector;                        /* fragment size */
    total = ((uint64_t)(space.total_high) << 32) | space.total;
    sbuf->f_blocks   = (fsblkcnt_t)(total / phy.bytes_per_sector);  /* size of fs in f_frsize units */
    free = ((uint64_t)(space.free_high) << 32) | space.free;
    sbuf->f_bfree    = (fsblkcnt_t)(free / phy.bytes_per_sector);   /* free blocks in fs */
    sbuf->f_bavail   = sbuf->f_bfree;                               /* free blocks avail to non-superuser */
    sbuf->f_files    = -1;                                          /* total file nodes in file system */
    sbuf->f_ffree    = -1;                                          /* free file nodes in fs */
    sbuf->f_favail   = -1;                                          /* avail file nodes in fs */
    sbuf->f_fsid     = ITP_DEVICE_FAT;                              /* file system id */
    sbuf->f_flag     = -1;	                                        /* mount flags */
    sbuf->f_namemax  = FILENAME_MAX;                                /* maximum length of filenames */

    return 0;
}

//extern FN_FILEINT *_f_check_handle(FN_FILE *filehandle);

static int FatFstat(int file, struct stat *st)
{
    int ret;
    F_STAT s;
    struct tm t;

    ret = f_fstat(openFiles[file], &s);
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }

    st->st_dev = ITP_DEVICE_FAT;
    st->st_ino = s.drivenum;

    st->st_mode = 0;
    if ((s.attr & F_ATTR_HIDDEN) == 0)
        st->st_mode |= S_IREAD;

    if ((s.attr & F_ATTR_READONLY) == 0)
        st->st_mode |= S_IWRITE;

    if ((s.attr & F_ATTR_ARC) == 0)
        st->st_mode |= S_IEXEC;

    if (s.attr & F_ATTR_DIR)
        st->st_mode |= S_IFDIR;
    else
        st->st_mode |= S_IFREG;

    st->st_size = s.filesize;
    st->st_blksize = 128 * 1024; // optimized for USB/SD

    t.tm_sec = 0;
    t.tm_min = 0;
    t.tm_hour = 0;
    t.tm_mday = (s.lastaccessdate & F_CDATE_DAY_MASK) >> F_CDATE_DAY_SHIFT;
    t.tm_mon = ((s.lastaccessdate & F_CDATE_MONTH_MASK) >> F_CDATE_MONTH_SHIFT) - 1;
    t.tm_year = 1980 - 1900 + ((s.lastaccessdate & F_CDATE_YEAR_MASK) >> F_CDATE_YEAR_SHIFT);
    t.tm_isdst = -1;
    st->st_atime = mktime(&t);

    t.tm_sec = ((s.modifiedtime & F_CTIME_SEC_MASK) >> F_CTIME_SEC_SHIFT) << 1;
    t.tm_min = (s.modifiedtime & F_CTIME_MIN_MASK) >> F_CTIME_MIN_SHIFT;
    t.tm_hour = (s.modifiedtime & F_CTIME_HOUR_MASK) >> F_CTIME_HOUR_SHIFT;
    t.tm_mday = (s.modifieddate & F_CDATE_DAY_MASK) >> F_CDATE_DAY_SHIFT;
    t.tm_mon = ((s.modifieddate & F_CDATE_MONTH_MASK) >> F_CDATE_MONTH_SHIFT) - 1;
    t.tm_year = 1980 - 1900 + ((s.modifieddate & F_CDATE_YEAR_MASK) >> F_CDATE_YEAR_SHIFT);
    st->st_mtime = mktime(&t);

    return 0;
}

static char* FatGetcwd(char *buf, size_t size)
{
    int ret, drive;

    if (size < 3)
    {
        errno = ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT;
        return NULL;
    }

    drive = f_getdrive();
    buf[0] = 'A' + drive;
    buf[1] = ':';
    size -= 2;

#ifdef ITP_FAT_UTF8
    wchar_t wbuf[PATH_MAX + 1];

    ret = f_wgetcwd(wbuf, size);
    if (ret == 0)
        wcstombs(&buf[2], wbuf, size);

#else
    ret = f_getcwd((&buf[2], size);

#endif // ITP_FAT_UTF8

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return NULL;
    }
    return buf;
}

static int FatRmdir(const char *path)
{
    int ret;

#ifdef ITP_FAT_UTF8
    wchar_t buf[PATH_MAX + 1];

    mbstowcs(buf, path, PATH_MAX + 1);
    ret = f_wrmdir(buf);

#else
    ret = f_rmdir(path);

#endif // ITP_FAT_UTF8

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatClosedir(DIR *dirp)
{
    free(dirp->dd_buf);
    free(dirp);
    return 0;
}

static DIR* FatOpendir(const char *dirname)
{
    DIR* dirp = malloc(sizeof (DIR));
    if (!dirp)
    {
        errno = ENOMEM;
        return NULL;
    }

#ifdef ITP_FAT_UTF8
    {
        F_WFIND* find = malloc(sizeof (F_WFIND) + sizeof (struct dirent) + strlen(dirname) + 1);
        if (!find)
        {
            free(dirp);
            errno = ENOMEM;
            return NULL;
        }

        dirp->dd_fd     = ITP_DEVICE_FAT;
        dirp->dd_size   = sizeof (F_WFIND);
        dirp->dd_buf    = (char*) find;
        dirp->dd_loc    = -1;

        strcpy(&dirp->dd_buf[sizeof(F_WFIND) + sizeof(struct dirent)], dirname);
    }

#else
    {
        FN_FIND* find = malloc(sizeof (FN_FIND) + sizeof (struct dirent) + strlen(dirname) + 1);
        if (!find)
        {
            free(dirp);
            errno = ENOMEM;
            return NULL;
        }

        dirp->dd_fd     = ITP_DEVICE_FAT;
        dirp->dd_size   = sizeof (FN_FIND);
        dirp->dd_buf    = (char*) find;
        dirp->dd_loc    = -1;

        strcpy(&dirp->dd_buf[sizeof(FN_FIND) + sizeof(struct dirent)], dirname);
    }
#endif // ITP_FAT_UTF8

    return dirp;
}

static struct dirent* FatReaddir(DIR *dirp)
{
    struct dirent* d;

    if (dirp->dd_size == sizeof (F_WFIND))
    {
        F_WFIND* find = (F_WFIND*)dirp->dd_buf;

        d = (struct dirent*)(dirp->dd_buf + sizeof (F_WFIND));

        if (dirp->dd_loc >= 0)
        {
            int ret = f_wfindnext(find);
            if (ret)
                return NULL;
        }
        else
        {
            wchar_t buf[PATH_MAX + 1];
            int ret;
            char *dirname = &dirp->dd_buf[sizeof(F_WFIND) + sizeof(struct dirent)];

            mbstowcs(buf, dirname, PATH_MAX + 1);

            if (buf[wcslen(buf) - 1] == L'/')
                wcscat(buf, L"*.*");
            else
                wcscat(buf, L"/*.*");

            ret = f_wfindfirst(buf, find);
            if (ret)
                return NULL;
        }
        d->d_ino    = ++dirp->dd_loc;
        d->d_type   = (find->attr & F_ATTR_DIR) ? DT_DIR : DT_REG;
        d->d_namlen = wcstombs(d->d_name, find->filename, NAME_MAX + 1);
    }
    else
    {
        FN_FIND* find = (FN_FIND*)dirp->dd_buf;

        d = (struct dirent*)(dirp->dd_buf + sizeof (FN_FIND));

        if (dirp->dd_loc >= 0)
        {
            int ret = f_findnext(find);
            if (ret)
                return NULL;
        }
        else
        {
            char buf[PATH_MAX + 1];
            int ret;
            char *dirname = &dirp->dd_buf[sizeof(FN_FIND) + sizeof(struct dirent)];

            strlcpy(buf, dirname, sizeof(buf));
            if (buf[strlen(buf) - 1] == '/')
                strlcat(buf, "*.*", sizeof(buf));
            else
                strlcat(buf, "/*.*", sizeof(buf));

            ret = f_findfirst(buf, find);
            if (ret)
                return NULL;
        }
        d->d_ino    = ++dirp->dd_loc;
        d->d_type   = (find->attr & F_ATTR_DIR) ? DT_DIR : DT_REG;
        d->d_namlen = strlen(find->filename);
        strncpy(d->d_name, find->filename, d->d_namlen);
    }

    return d;
}

static void FatRewinddir(DIR *dirp)
{
    intptr_t ret;

    if (dirp->dd_size == sizeof(F_WFIND))
    {
        F_WFIND* find = (F_WFIND*) dirp->dd_buf;
        wchar_t buf[PATH_MAX + 1];

        mbstowcs(buf, &dirp->dd_buf[sizeof(F_WFIND) + sizeof(struct dirent)], PATH_MAX + 1);

        if (buf[wcslen(buf) - 1] == L'/')
            wcscat(buf, L"*.*");
        else
            wcscat(buf, L"/*.*");

        ret = f_wfindfirst(buf, find);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return;
        }
    }
    else
    {
        FN_FIND* find = (FN_FIND*) dirp->dd_buf;
        char buf[PATH_MAX + 1];

        strlcpy(buf, &dirp->dd_buf[sizeof(FN_FIND) + sizeof(struct dirent)], sizeof(buf));
        if (buf[strlen(buf) - 1] == '/')
            strlcat(buf, "*.*", sizeof(buf));
        else
            strlcat(buf, "/*.*", sizeof(buf));

        ret = f_findfirst(buf, find);
        if (ret == -1)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return;
        }
    }
    dirp->dd_loc  = -1;
}

static long FatFtell(int file)
{
    return f_tell(openFiles[file]);
}

static int FatFflush(int file)
{
    int ret = f_flush(openFiles[file]);

    #ifdef  CFG_NAND_ENABLE
    ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
    #endif

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return EOF;
    }
    return 0;
}

static int FatFeof(int file)
{
    int ret = f_eof(openFiles[file]);
    if (ret)
    {
        return EOF;
    }
    return 0;
}

static int FatTruncate(const char *path, off_t length)
{
    F_FILE* file = f_truncate(path, length);
    if (!file)
    {
        return -1;
    }
    f_close(file);
    return 0;
}

static int FatFtruncate(int file, off_t length)
{
    int ret = f_ftruncate(openFiles[file], length);
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
        return -1;
    }
    return 0;
}

static int FatMount(ITPDisk disk, bool force, bool noexbr)
{
    int ret, i, j;
    ITPDriveStatus* driveStatus = NULL;
    Driver* driver;
    F_PARTITION partitions[ITP_MAX_PARTITION] = {0};
    ITPDriveStatus* driveStatusTable = NULL;
    uint8_t fat_exbr[ITP_FAT_EXBR_SIZE] = {0};
    FILE *f = NULL;
    uint64_t exbr_start = 0;

#ifdef CFG_UPGRADE_BACKUP_FAT_MBR
    uint32_t mbr_pos = 0;
#if defined(CFG_NAND_PARTITION0)
    if (disk == ITP_DISK_NAND)
    {
        mbr_pos = CFG_NAND_RESERVED_SIZE;
    }
#endif
#if defined(CFG_NOR_PARTITION0)
    if (disk == ITP_DISK_NOR)
    {
        mbr_pos = CFG_NOR_RESERVED_SIZE;
    }
#endif
#if defined(CFG_SD0_PARTITION0)
    if (disk == ITP_DISK_SD0)
    {
        mbr_pos = CFG_SD0_RESERVED_SIZE;
    }
#endif
#if defined(CFG_SD1_PARTITION0)
    if (disk == ITP_DISK_SD1)
    {
        mbr_pos = CFG_SD1_RESERVED_SIZE;
    }
#endif

    if ((MBR_check[disk] == false) && (create_partition[disk] == false) && (mbr_pos != 0))
    {
        //check FAT MBR CRC
        ret = ugCheckFatMBR(disk,
                    mbr_pos,
                    0x40000,
                    false,
                    false);
        if (ret == MBR_INUSE_CRC_ERR)
        {
            //rollback backup FAT MBR
            ret = ugCheckFatMBR(disk,
                    mbr_pos,
                    0x40000,
                    false,
                    true);
            if (ret)
            {
                printf("FAT MBR rollback failed!\n");
            }
        }else if (ret == MBR_BACKUP_CRC_ERR)
        {
            //backup FAT MBR again
            ret = ugCheckFatMBR(disk,
                    mbr_pos,
                    0x40000,
                    true,
                    false);
            if (ret)
            {
                printf("FAT MBR backup failed!\n");
            }
        }else if (ret == MBR_BACKUP_INUSE_CRC_ERR)
        {
            printf("FAT MBR both inuse and backup corrupted!!!\n");
        }else if (ret != 0)
        {
            printf("FAT MBR check failed with unexpect errror !!!\n");
        }
        else
        {
            MBR_check[disk] = true;
        }
        ret = 0;
    }
#endif

    // init drives
    driver = &driverTable[disk];

    if (!driver->driver)
    {
        ret = f_createdriver(&driver->driver, driver->initfunc, driver->param);
        if (ret)
        {
            ITP_LOG_ERR("f_createdriver() fail: %d\n", ret );
            return ret;
        }
    }

    // get fat's partitions
    ret = f_getpartition(driver->driver, ITP_MAX_PARTITION, partitions);
    if (ret)
    {
        ITP_LOG_ERR("f_getpartition() fail: %d\n", ret );
        return ret;
    }

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    j = 0;

    //first to mount PART B partition
#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
    uint8_t boot_args = 0x0U;
    if (ugBootargsRead(&boot_args))
    {
        ITP_LOG_ERR("Upgrade read boot args failed.\n");
    }
    if ((mount_flag == 0xBBU) && (boot_args == 0xBBU))
    {
#if defined(CFG_SUB_DATA_NAND)
        ITPDisk sub_disk = ITP_DISK_NAND;
#elif defined(CFG_SUB_DATA_NOR)
        ITPDisk sub_disk = ITP_DISK_NOR;
#elif defined(CFG_SUB_DATA_SD0)
        ITPDisk sub_disk = ITP_DISK_SD0;
#elif defined(CFG_SUB_DATA_SD1)
        ITPDisk sub_disk = ITP_DISK_SD1;
#endif
        if (disk == sub_disk)
        {
    for (i = 0; i < ITP_MAX_PARTITION; i++)
    {
        F_PARTITION* partition = &partitions[i];

        //only allow partition0 with secnum == 0.
        if (partition->secnum == 0 && i)
        {
            break;
        }

                if(mount_info[disk][i] == true)
                {
                    printf("############have mounted(%d, %d)\n", disk, i);
                    continue;
                }
                if((i != CFG_SUB_PRIVATE_PARTITION) && (i != CFG_SUB_PUBLIC_PARTITION))
                {
                    printf("############backup, jump(%d, %d)\n", disk, i);
                    continue;
                }

                printf("############mount part B partition(%d, %d)\n", disk, i);
                // find an empty drive
                for (; j < ITP_MAX_DRIVE; j++)
                {
                    driveStatus = &driveStatusTable[j];

                    if (!driveStatus->avail)
                        break;
                }

                ret = f_initvolumepartition(j, driver->driver, i);
                if (ret && ret != F_ERR_NOTFORMATTED)
                {
                    f_delvolume(j);
                    if (!force)
                    {
                        ITP_LOG_ERR("f_initvolumepartition(%d, 0x%X, %d) fail: %d\n", j, driver->driver, i, ret );
                        return ret;
                    }
                }

                if( (disk==5) && (ret == F_ERR_NOTFORMATTED) )
                {
                    ret = (int)FatFormat((int)j);
                }

                if (ret == F_ERR_NOTFORMATTED && !force)
                {
                    f_delvolume(j);
                    if (partition->secnum == 0)
                    {
                        return F_ERR_NOTFORMATTED;
                    }
                    else
                    {
                        continue;
                    }
                }

                mount_info[disk][i]     = true;
                driveStatus->par_idx    = i;

                driveStatus->disk       = disk;
                driveStatus->device     = ITP_DEVICE_FAT;
                driveStatus->avail      = true;
                driveStatus->name[0]    = 'A' + j;
                driveStatus->name[1]    = ':';
                driveStatus->name[2]    = '/';

                write(ITP_DEVICE_DRIVE, driveStatus, sizeof (ITPDriveStatus));
                if (partition->secnum == 0)
                {
                    break;
                }
            }
        }
    }
#endif

    for (i = 0; i < ITP_MAX_PARTITION; i++)
    {
        F_PARTITION* partition = &partitions[i];

        //only allow partition0 with secnum == 0.
        if (partition->secnum == 0 && i)
        {
            break;
        }

#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
        if (mount_flag == 0xAAU)
        {
#if defined(CFG_MAIN_DATA_NAND)
            ITPDisk main_disk = ITP_DISK_NAND;
#elif defined(CFG_MAIN_DATA_NOR)
            ITPDisk main_disk = ITP_DISK_NOR;
#elif defined(CFG_MAIN_DATA_SD0)
            ITPDisk main_disk = ITP_DISK_SD0;
#elif defined(CFG_MAIN_DATA_SD1)
            ITPDisk main_disk = ITP_DISK_SD1;
#endif

            if(mount_info[disk][i] == true)
            {
                printf("############have mounted(%d, %d)\n", disk, i);
                continue;
            }
            if((disk == main_disk) && ((i == CFG_SUB_PRIVATE_PARTITION) || (i == CFG_SUB_PUBLIC_PARTITION)))
            {
                printf("############backup, jump(%d, %d)\n", disk, i);
                continue;
            }
        }else if (mount_flag == 0xBBU)
        {
#if defined(CFG_SUB_DATA_NAND)
            ITPDisk sub_disk = ITP_DISK_NAND;
#elif defined(CFG_SUB_DATA_NOR)
            ITPDisk sub_disk = ITP_DISK_NOR;
#elif defined(CFG_SUB_DATA_SD0)
            ITPDisk sub_disk = ITP_DISK_SD0;
#elif defined(CFG_SUB_DATA_SD1)
            ITPDisk sub_disk = ITP_DISK_SD1;
#endif

            if(mount_info[disk][i] == true)
            {
                printf("############have mounted(%d, %d)\n", disk, i);
                continue;
            }
            if((disk == sub_disk) && ((i == CFG_PRIVATE_PARTITION) || (i == CFG_PUBLIC_PARTITION)))
            {
                printf("############backup, jump(%d, %d)\n", disk, i);
                continue;
            }
        }
#endif

        // find an empty drive
        for (; j < ITP_MAX_DRIVE; j++)
        {
            driveStatus = &driveStatusTable[j];

            if (!driveStatus->avail)
                break;
        }

        ret = f_initvolumepartition(j, driver->driver, i);
        if (ret && ret != F_ERR_NOTFORMATTED)
        {
            f_delvolume(j);
            if (!force)
            {
                ITP_LOG_ERR("f_initvolumepartition(%d, %p, %d) fail: %d\n", j, driver->driver, i, ret );
                return ret;
            }
        }

        if (ret == F_ERR_NOTFORMATTED && !force)
        {
            f_delvolume(j);
            if (partition->secnum == 0)
            {
                return F_ERR_NOTFORMATTED;
            }
            else
            {
                continue;
            }
        }

#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
        mount_info[disk][i]     = true;
        driveStatus->par_idx    = i;
#endif
        driveStatus->disk       = disk;
        driveStatus->device     = ITP_DEVICE_FAT;
        driveStatus->avail      = true;
        driveStatus->name[0]    = 'A' + j;
        driveStatus->name[1]    = ':';
        driveStatus->name[2]    = '/';

        write(ITP_DEVICE_DRIVE, driveStatus, sizeof (ITPDriveStatus));
        if (partition->secnum == 0)
        {
            break;
        }
    }

    if (noexbr) return 0;

#if defined(CFG_NOR_PARTITION3) && !defined(CFG_FS_LFS)
    if (disk == ITP_DISK_NOR && partitions[3].secnum == 0)
    {
        f = fopen(CFG_PRIVATE_DRIVE ":/" ITP_FAT_EXBR_FILE, "rb");
        if (!f)
        {
            ITP_LOG_ERR("fopen fail\n" );
        }
        else
        {
            if (fread(fat_exbr, 1, ITP_FAT_EXBR_SIZE, f) != ITP_FAT_EXBR_SIZE)
                 ITP_LOG_ERR("fread fail\n" );
            fclose(f);

            memcpy(&exbr_start, fat_exbr, 8);
            if (NorWrite(SPI_0, NOR_CSN, (uint32_t)exbr_start, fat_exbr + 8, ITP_FAT_EXBR_SIZE - 8))
            {
                ITP_LOG_ERR("fwrite fail\n" );
            }
        }

        FatUnmount(disk);
        FatMount(disk, force, noexbr);
    }
#endif

    return 0;
}

static int FatUnmount(ITPDisk disk)
{
    int i, ret1 = -1, ret2;
    ITPDriveStatus* driveStatusTable = NULL;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    // deinit all drives of this disk
    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        ITPDriveStatus* driveStatus = &driveStatusTable[i];
        //printf("i=%d avail=%d disk=%d disk=%d\n", i, driveStatus->avail, driveStatus->disk, disk);
        if (driveStatus->avail && driveStatus->disk == disk && driveStatus->device == ITP_DEVICE_FAT)
        {
            ret2 = f_delvolume(i);
            if (ret2 == 0)
            {
                ret1 = 0;
            }
            else
            {
                ITP_LOG_ERR("f_delvolume(%d) fail: %d\n", i, ret2 );
                if (ret1 <= 0)
                    ret1 = ret2;
            }
            driveStatus->avail  = false;
            write(ITP_DEVICE_DRIVE, driveStatus, sizeof (ITPDriveStatus));
        }
    }

    ret2 = f_releasedriver(driverTable[disk].driver);
    driverTable[disk].driver = NULL;

#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
    printf("Unmount disk %d info status!\n", disk);
    for (i = 0; i < ITP_MAX_PARTITION; i++)
    {
        mount_info[disk][i] = false;
    }
#endif

    if (ret2 > 0)
    {
        ITP_LOG_ERR("f_releasedriver() fail: %d\n", ret1 );
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret1;

        if (ret1 <= 0)
            ret1 = ret2;
    }
    return ret1;
}

static int FatGetPartition(ITPPartition* par)
{
    F_PARTITION partitions[ITP_MAX_PARTITION];
    Driver* driver;
    F_PHY phy;
    int ret, i;

    driver = &driverTable[par->disk];

    if (!driver->driver)
    {
        ret = f_createdriver(&driver->driver, driver->initfunc, driver->param);
        if (ret)
        {
            ITP_LOG_ERR("f_createdriver() fail: %d\n", ret );
            return ret;
        }
    }

    ret = f_getpartition(driver->driver, ITP_MAX_PARTITION, partitions);
    if (ret)
        return ret;

    ret = driver->driver->getphy(driver->driver, &phy);
    if (ret)
        return ret;

	if (!phy.bytes_per_sector)
		phy.bytes_per_sector = F_DEF_SECTOR_SIZE;

    par->count = 0;
    for (i = 0; i < ITP_MAX_PARTITION; i++)
    {
        if (partitions[i].secnum > 0)
        {
            par->count++;
            par->size[i] = (uint64_t)partitions[i].secnum * phy.bytes_per_sector;
        }
        else
            break;
    }

    return 0;
}

#define F_DEF_PART_ALIGN 512             /* default partition alignment */

static int FatCreatePartition(ITPPartition* par)
{
    F_PARTITION partitions[ITP_MAX_PARTITION];
    Driver* driver;
    F_PHY phy;
    int ret, i;
    unsigned long secnum = 0;

    driver = &driverTable[par->disk];

    if (!driver->driver)
    {
        switch (par->disk)
        {
    #ifdef CFG_SD0_STATIC
        case ITP_DISK_SD0:
            sdDrvParams[0].reserved = (unsigned long)par->start[0];
            break;
    #endif // CFG_SD0_STATIC

    #ifdef CFG_SD1_STATIC
        case ITP_DISK_SD1:
            sdDrvParams[1].reserved = (unsigned long)par->start[0];
            break;
    #endif // CFG_SD1_STATIC

    #ifdef CFG_NOR_ENABLE
        case ITP_DISK_NOR:
            norDrvParam.reserved = (unsigned long)par->start[0];
            break;
    #endif // CFG_NOR_ENABLE

        default:
            break;
        }

        ret = f_createdriver(&driver->driver, driver->initfunc, driver->param);
        if (ret)
        {
            ITP_LOG_ERR("f_createdriver() fail: %d\n", ret );
            return ret;
        }
    }

    ret = driver->driver->getphy(driver->driver, &phy);
    if (ret)
        return ret;

	if (!phy.bytes_per_sector)
		phy.bytes_per_sector = F_DEF_SECTOR_SIZE;

    for (i = 0; i < par->count; i++)
    {
        uint64_t size;

        if (par->size[i] > 0)
        {
            partitions[i].secnum = (unsigned long)(par->size[i] / phy.bytes_per_sector);
            secnum += partitions[i].secnum;
        }
        else
        {
            partitions[i].secnum = phy.number_of_sectors - secnum;

            if (i != par->count - 1)
            {
                ITP_LOG_ERR("partition %d/%d has zero size.\n", i, par->count );
                return __LINE__;
            }
        }

        size = partitions[i].secnum * phy.bytes_per_sector;
        ITP_LOG_DBG("partition %d size: %llu\n", i, size );
        if (size < 32 * 1024 * 1024)
        {
            partitions[i].system_indicator = F_SYSIND_DOSFAT16UPTO32MB;
        }
        else if (size < 128 * 1024 * 1024)
        {
            partitions[i].system_indicator = F_SYSIND_DOSFAT16OVER32MB;
        }
        else
        {
            partitions[i].system_indicator = F_SYSIND_DOSFAT32;
        }
    }

    for (;;)
    {
        ret = f_createpartition_align(driver->driver, par->count, partitions, F_DEF_PART_ALIGN);
        if (ret == F_ERR_MEDIATOOSMALL)
        {
            if (partitions[par->count - 1].secnum > F_DEF_PART_ALIGN)
            {
                partitions[par->count - 1].secnum -= F_DEF_PART_ALIGN;
                continue;
            }
            else
            {
                ITP_LOG_ERR("f_createpartition(,%d,) fail: %d\n", par->count, ret );
                return ret;
            }
        }
        else if (ret)
        {
            ITP_LOG_ERR("f_createpartition(,%d,) fail: %d\n", par->count, ret );
            return ret;
        }
        else
        {
            if (par->count > 3)
                par->exbr_start = par->start[par->count - 1] + F_DEF_PART_ALIGN * phy.bytes_per_sector;

            break;
        }
    }
#ifdef CFG_UPGRADE_BACKUP_FAT_MBR
    create_partition[par->disk] = true;
#endif
    return 0;
}

static int FatFormat(int volume)
{
    int ret = f_format(volume, F_FAT16_MEDIA);
    if (ret == F_ERR_MEDIATOOSMALL)
    {
        ret = f_format(volume, F_FAT12_MEDIA);
        if (ret)
        {
            ITP_LOG_ERR("f_format(%d) fail: %d\n", volume, ret );
            return ret;
        }
    }
    else if (ret == F_ERR_MEDIATOOLARGE)
    {
        ret = f_format(volume, F_FAT32_MEDIA);
        if (ret)
        {
            ITP_LOG_ERR("f_format(%d) fail: %d\n", volume, ret );
            return ret;
        }
    }
    return 0;
}

static int FatIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int ret;

    switch (request)
    {
    case ITP_IOCTL_MOUNT:
        ret = FatMount((ITPDisk)ptr, false, false);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
    case ITP_IOCTL_MOUNT_DUAL_AB:
        mount_flag = *(uint8_t*)ptr;
        break;
#endif

    case ITP_IOCTL_FORCE_MOUNT:
        ret = FatMount((ITPDisk)ptr, true, false);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_MOUNT_NOEXBR:
        ret = FatMount((ITPDisk)ptr, false, true);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_FORCE_MOUNT_NOEXBR:
        ret = FatMount((ITPDisk)ptr, true, true);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_UNMOUNT:
        ret = FatUnmount((ITPDisk)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_ENABLE:
        if (!fatInited)
            break;

        ret = f_enterFS();
        if (ret)
        {
            ITP_LOG_ERR("f_enterFS() fail: %d\n", ret );
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_DISABLE:
        if (!fatInited)
            break;

        f_releaseFS();
        break;

    case ITP_IOCTL_GET_PARTITION:
        ret = FatGetPartition((ITPPartition*)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_CREATE_PARTITION:
        ret = FatCreatePartition((ITPPartition*)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_FORMAT:
        ret = FatFormat((int)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_INIT:
        ret = fs_init();
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        ret = fs_start();
        if (ret)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        fatInited = true;
        break;

    case ITP_IOCTL_EXIT:
        fs_stop();
        fs_delete();
        fatInited = false;
        break;

    case ITP_IOCTL_GET_TABLE:
        if ((int)ptr > ITH_DISK_MAX)
        {
            errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | __LINE__;
            return 0;
        }
        return (int)&driverTable[(int)ptr];

    default:
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        return -1;
    }
    return 0;
}

const ITPFSDevice itpFSDeviceFat =
{
    {
        ":fat",
        FatOpen,
        FatClose,
        FatRead,
        FatWrite,
        FatLseek,
        FatIoctl,
        NULL
    },
    FatRemove,
    FatRename,
    FatChdir,
    FatChmod,
    FatMkdir,
    FatStat,
    FatStatvfs,
    FatFstat,
    FatGetcwd,
    FatRmdir,
    FatClosedir,
    FatOpendir,
    FatReaddir,
    FatRewinddir,
    FatFtell,
    FatFflush,
    FatFeof,
    FatTruncate,
    FatFtruncate
};
