/*
 * Copyright (c) 2024 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL HCC and FileX functions.
 *
 * @author Jack Yu
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
#include "fx_api.h"
#if defined(CFG_BUILD_LEVELX)
#include "lx_api.h"
#endif

#define FILEX_MEDIA_MEMORY_BUFF_SIZE (64*1024)
#define FILEX_FAULT_TOLERANT_MEMORY_BUFF_SIZE (3*1024)
#define LEVELX_EXTERNAL_CACHE_SIZE (128*1024)
#define FILEX_ERR_MEDIATOOSMALL     (-1)
#define FILEX_ERR_MEDIATOOLARGE     (-2)
#define FILEX_TEMP_FILE_EXT         (".filextmp")
//#define ENABLE_FILEX_MEDIA_CHECK

#define FILEX_MEDIA_CHECK_MEMORY_BUFF_SIZE (400*1024)

extern long fn_gettaskID(void);
#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
static bool fatInited;
#else
bool fatInited;
#endif
static bool skipCheckTempFiles = false; // Skip check temp files by default

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


static int FileXFormat(int volume);
static int FileXUnmount(ITPDisk disk);
static void SearchTempFiles(const char *root_dir);
static void ClearTempFiles(const char *root_dir, bool async);

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

typedef struct
{
    FX_MEDIA *media;
    ITPDriveStatus *driveStatus;
    unsigned char *memoryPtr;
    FX_DRV_INFO *drvInfo;
} MEDIA_PARTITION_INFO;

typedef struct
{
    FX_FILE file;
    bool using;
    int lastRet;
} FILE_INFO;

typedef struct
{
  uint32_t  maxSectors;
  uint8_t   sectorPerCluster;
} FAT32_CLUSTER_SIZE;

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

static MEDIA_PARTITION_INFO partitionInfoTable[ITP_MAX_DRIVE];

static FILE_INFO openFiles[OPEN_MAX];
static int currentVolume = 0;

static const FAT32_CLUSTER_SIZE  FAT32_CS[] =
{
  { 0x00010000, 1 }     /* ->32MB */
  , { 0x00020000, 2 }     /* ->64MB */
  , { 0x00040000, 4 }   /* ->128MB */
  , { 0x00080000, 8 }   /* ->256MB */
  , { 0x01000000, 16 }   /* ->8GB */
  , { 0x02000000, 32 }  /* ->16GB */
  , { 0x0ffffff0, 64 }  /* -> ... */
};

static int CheckPathValid(char* name, char **newName)
{

    if(((name[0] >= 'a' && name[0] <= 'z' && name[1] == ':') || (name[0] >= 'A' && name[0] <= 'Z' && name[1] == ':') ) && 
        (name[2] == '\\') || (name[2] == '/'))
    {
        *newName = &name[2];       
        return name[0] <= 'Z' ? name[0] - 'A' : name[0] - 'a';
    }
    return -1;
}

// Function to map FileX error codes to errno codes
static int FxToErrno(UINT fxErrorCode) {
    switch (fxErrorCode) {
        case FX_SUCCESS:
            return 0;  // No error
        
        case FX_BOOT_ERROR:
            return EIO;  // Input/output error
        
        case FX_MEDIA_INVALID:
            return ENXIO;  // No such device or address
        
        case FX_FAT_READ_ERROR:
            return EIO;  // Input/output error
        
        case FX_NOT_FOUND:
            return ENOENT;  // No such file or directory
        
        case FX_NOT_A_FILE:
            return EISDIR;  // Is a directory
        
        case FX_ACCESS_ERROR:
            return EACCES;  // Permission denied
        
        case FX_NOT_OPEN:
            return EBADF;  // Bad file descriptor
        
        case FX_FILE_CORRUPT:
            return EIO;  // Input/output error
        
        case FX_END_OF_FILE:
            return ENODATA;  // No data available (EOF)
        
        case FX_NO_MORE_SPACE:
            return ENOSPC;  // No space left on device
        
        case FX_ALREADY_CREATED:
            return EEXIST;  // File exists
        
        case FX_INVALID_NAME:
            return EINVAL;  // Invalid argument
        
        case FX_INVALID_PATH:
            return ENOENT;  // No such file or directory
        
        case FX_NOT_DIRECTORY:
            return ENOTDIR;  // Not a directory
        
        case FX_NO_MORE_ENTRIES:
            return ENFILE;  // Too many open files in system
        
        case FX_DIR_NOT_EMPTY:
            return ENOTEMPTY;  // Directory not empty
        
        case FX_MEDIA_NOT_OPEN:
            return ENODEV;  // No such device
        
        case FX_INVALID_YEAR:
        case FX_INVALID_MONTH:
        case FX_INVALID_DAY:
        case FX_INVALID_HOUR:
        case FX_INVALID_MINUTE:
        case FX_INVALID_SECOND:
            return EINVAL;  // Invalid argument for date/time
        
        case FX_PTR_ERROR:
            return EFAULT;  // Bad address
        
        case FX_INVALID_ATTR:
            return EINVAL;  // Invalid argument
        
        case FX_CALLER_ERROR:
            return EPERM;  // Operation not permitted
        
        case FX_BUFFER_ERROR:
            return EFAULT;  // Bad address
        
        case FX_NOT_IMPLEMENTED:
            return ENOSYS;  // Function not implemented
        
        case FX_WRITE_PROTECT:
            return EROFS;  // Read-only file system
        
        case FX_INVALID_OPTION:
            return EINVAL;  // Invalid argument
        
        case FX_SECTOR_INVALID:
            return EIO;  // Input/output error
        
        case FX_IO_ERROR:
            return EIO;  // Input/output error
        
        case FX_NOT_ENOUGH_MEMORY:
            return ENOMEM;  // Out of memory
        
        case FX_ERROR_FIXED:
        case FX_ERROR_NOT_FIXED:
            return EIO;  // Input/output error
        
        case FX_NOT_AVAILABLE:
            return EBUSY;  // Device or resource busy
        
        case FX_INVALID_CHECKSUM:
            return EBADMSG;  // Bad message
        
        case FX_READ_CONTINUE:
            return EAGAIN;  // Try again
        
        case FX_INVALID_STATE:
            return EINVAL;  // Invalid argument
        
        default:
            return EINVAL;  // Invalid argument for unrecognized errors
    }
}
static int FileXOpen(const char* name, int flags, int mode, void* info)
{
    int i, mediaIdx, ret = -1;
    char *newName;
    bool fileExist = false;
    UINT openType;
    char _name[NAME_MAX + 1];
    unsigned long oriSize;
    if(!name)
        return -1;
    // find empty slot
    ithEnterCritical();
    for (i = 0; i < OPEN_MAX; i++)
    {
        if (openFiles[i].using == false)
        {
            openFiles[i].using = true;
            openFiles[i].lastRet = 0;
            break;
        }
    }
    ithExitCritical();

    if(i == OPEN_MAX)
    {
        ITP_LOG_WARN("File handler buffer full\r\n");
        return -1;
    }
    mediaIdx = CheckPathValid((char *)name, &newName);
    if(mediaIdx >= 0)
    {
        FX_MEDIA * media_ptr = partitionInfoTable[mediaIdx].media;
        if (partitionInfoTable[mediaIdx].media->fx_media_fault_tolerant_enabled && 
            (flags & (O_WRONLY)) && !(flags & O_APPEND))
        {
            UINT         attributes, year, month, day;
            UINT         hour, minute, second;
            ret = fx_directory_information_get(partitionInfoTable[mediaIdx].media, newName, &attributes, &oriSize,
                                        &year, &month, &day,
                                        &hour, &minute, &second);
            if(!ret)
            {
                strlcpy(_name, newName, sizeof(_name));
                strlcat(_name, FILEX_TEMP_FILE_EXT, sizeof(_name));
                newName = _name;
                flags |= O_CREAT;                
            }
        }
        if (flags & O_CREAT)
        {
            ret = fx_file_create(partitionInfoTable[mediaIdx].media, newName);
            if(ret && ret != FX_ALREADY_CREATED)
            {
                ITP_LOG_WARN("fat create %s fail: %d\n", name, ret );
                errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
                return -1;
            }
            else if(ret == FX_ALREADY_CREATED)
            {
                fileExist = true;
            }
        }
        if (flags & (O_RDWR | O_WRONLY))
            openType = FX_OPEN_FOR_WRITE;
        else
            openType = FX_OPEN_FOR_READ_FAST;
        ret = fx_file_open(partitionInfoTable[mediaIdx].media, &openFiles[i].file, newName, openType);
        if(!ret)
        {
            if(flags & O_APPEND)
            {
                fx_file_relative_seek(&openFiles[i].file, 0, FX_SEEK_END);
            }
            else if(fileExist)
            {
                fx_file_truncate_release(&openFiles[i].file, 0);  // File clear
                fx_file_seek(&openFiles[i].file, 0);
            }
            else fx_file_seek(&openFiles[i].file, 0);
        }
        else
        {
        }
    }
    if (ret)
    {
        ITP_LOG_WARN("fat open %s fail: %d, %d, %s\n", name, ret, mediaIdx, newName);
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }

    return i;
}

static int FileXClose(int file, void* info)
{
    FX_MEDIA * media_ptr = openFiles[file].file.fx_file_media_ptr;
    
    FX_PROTECT
    
    int ret = fx_file_close(&openFiles[file].file);
    char *filePath = openFiles[file].file.fx_file_name_buffer;
    int filePathLen = strlen(filePath);
    int tempFileExtPathLen = strlen(FILEX_TEMP_FILE_EXT);
    if(filePathLen > tempFileExtPathLen)
    {
        if(!strncmp(filePath + filePathLen - tempFileExtPathLen, FILEX_TEMP_FILE_EXT, tempFileExtPathLen))
        {
            char newFilePath[NAME_MAX + 1];
            strncpy(newFilePath, filePath, filePathLen - tempFileExtPathLen);
            newFilePath[filePathLen - tempFileExtPathLen] = '\0';
            if(!openFiles[file].lastRet)
            {
                int ret2 = fx_file_delete(openFiles[file].file.fx_file_media_ptr, newFilePath);
                if(ret2)
                    ITP_LOG_ERR("fx_file_delete %s fail: %d\n", newFilePath, ret2 );
                else
                {
                    ret2 = fx_file_rename(openFiles[file].file.fx_file_media_ptr, filePath, newFilePath);
                    if(ret2)
                        ITP_LOG_ERR("fx_file_rename %s to %s fail: %d\n", filePath, newFilePath, ret2 );
                }
            }
            else
            {
                ITP_LOG_WARN("File %s operate fail: %d, discard\n", filePath, openFiles[file].lastRet);
                int ret2 = fx_file_delete(openFiles[file].file.fx_file_media_ptr, filePath);
            }
        }
    }
    FX_UNPROTECT
    #ifdef  CFG_NAND_ENABLE
    ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
    #endif

    ithEnterCritical();
    openFiles[file].using = false;
    ithExitCritical();

    if (ret)
    {
        ITP_LOG_ERR("fat close %d fail: %d\n", file, ret );
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXRead(int file, char *ptr, int len, void* info)
{
    int ret = -1;
    unsigned long actualSize;
#ifdef CFG_DBG_STATS_FAT
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
#endif
    if(file >= 0 && file < OPEN_MAX && openFiles[file].using)
    {
        ret = fx_file_read(&openFiles[file].file, ptr, len, &actualSize);
        openFiles[file].lastRet = ret;
    }
#ifdef CFG_DBG_STATS_FAT
    gettimeofday(&t2, NULL);

    ithEnterCritical();
    itpStatsFat.readTime += itpTimevalDiff(&t1, &t2);
    itpStatsFat.readSize += len;
    ithExitCritical();

#endif // CFG_DBG_STATS_FAT
    return (ret == 0) ? actualSize : 0;
}

static int FileXWrite(int file, char *ptr, int len, void* info)
{
    int ret = -1;

#ifdef CFG_DBG_STATS_FAT
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
#endif
    if(file >= 0 && file < OPEN_MAX && openFiles[file].using)
    {
        ret = fx_file_write(&openFiles[file].file, ptr, len);
        openFiles[file].lastRet = ret;
    }

#ifdef CFG_DBG_STATS_FAT
    gettimeofday(&t2, NULL);

    ithEnterCritical();
    itpStatsFat.writeTime += itpTimevalDiff(&t1, &t2);
    itpStatsFat.writeSize += len;
    ithExitCritical();

#endif // CFG_DBG_STATS_FAT
    if(ret)
    {
        ITP_LOG_WARN("FileXWrite fail len : %d, ret %d\r\n", len, ret);
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
    }
    return (ret == 0) ? len : 0;    // FileX write no return actual size, only success or fail.
}

static int FileXLseek(int file, int ptr, int dir, void* info)
{
    UINT seek;
    int ret = 0;
    if(file < 0 || file >= OPEN_MAX || !openFiles[file].using)
        return -1;
    switch(dir)
    {
    case SEEK_SET: seek = FX_SEEK_BEGIN ; break;
    case SEEK_CUR: seek = ptr >= 0? FX_SEEK_FORWARD : FX_SEEK_BACK; break;
    case SEEK_END: seek = FX_SEEK_END; break;
    default : 
        ITP_LOG_WARN("FatLseek unknown dir %d\r\n", dir);
        ret = -1;
        break;
    }
    
    if(ret == 0)
        ret = fx_file_relative_seek(&openFiles[file].file, abs(ptr), seek);
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    FX_MEDIA * media_ptr = openFiles[file].file.fx_file_media_ptr;
    FX_PROTECT
    ret = openFiles[file].file.fx_file_current_file_offset;
    FX_UNPROTECT
    return ret;
}

static int FileXRemove(const char *path)
{
    int ret = -1;
    char *newName;
    int mediaIdx = CheckPathValid((char *)path, &newName);
    if(mediaIdx >= 0)
    {
        UINT         attributes, year, month, day;
        ULONG        size;
        UINT         hour, minute, second;
        ret = fx_directory_information_get(partitionInfoTable[mediaIdx].media, newName, &attributes, &size,
                                      &year, &month, &day,
                                      &hour, &minute, &second);    
        if (!ret)
        {         
            if(attributes & FX_DIRECTORY)
                ret = fx_directory_delete(partitionInfoTable[mediaIdx].media, newName);
            else
                ret = fx_file_delete(partitionInfoTable[mediaIdx].media, newName);
        } 
    }
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXRename(const char *oldname, const char *newname)
{
    int ret = -1;
    char *oldNameFx, *newNameFx;
    int mediaIdxOld = CheckPathValid((char *)oldname, &oldNameFx);
    int mediaIdxNew = CheckPathValid((char *)newname, &newNameFx);
    if(mediaIdxOld >= 0 && mediaIdxOld == mediaIdxNew)
    {
        UINT         attributes, year, month, day;
        ULONG        size;
        UINT         hour, minute, second;
        ret = fx_directory_information_get(partitionInfoTable[mediaIdxOld].media, oldNameFx, &attributes, &size,
                                      &year, &month, &day,
                                      &hour, &minute, &second);   
        if (!ret)
        {         
            if(attributes & FX_DIRECTORY)
                ret = fx_directory_rename(partitionInfoTable[mediaIdxOld].media, oldNameFx, newNameFx);
            else
                ret = fx_file_rename(partitionInfoTable[mediaIdxOld].media, oldNameFx, newNameFx);
        }        
    }
    
    if(ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXChdir(const char *path)
{
    int ret = -1;
    char *newName;
    int mediaIdx = CheckPathValid((char *)path, &newName);
    if(mediaIdx >= 0)
    {
        FX_LOCAL_PATH     prevLocalPath;
        ret = fx_directory_local_path_set(partitionInfoTable[mediaIdx].media, &prevLocalPath, newName);
        if(ret == 0) currentVolume = mediaIdx;
    }
    if (ret && ret != FX_NOT_IMPLEMENTED)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXChmod(const char *path, mode_t mode)
{
    int ret = -1;
    UINT attr = 0;
    char *newName;
    int mediaIdx;

    if ((mode & S_IREAD) == 0)
        attr |= FX_HIDDEN;

    if ((mode & S_IWRITE) == 0)
        attr |= FX_READ_ONLY;

    if ((mode & S_IEXEC) == 0)
        attr |= FX_ARCHIVE;

    mediaIdx = CheckPathValid((char *)path, &newName);
    if(mediaIdx >= 0)
        ret = fx_directory_attributes_set(partitionInfoTable[mediaIdx].media, newName, attr);

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXMkdir(const char *path, mode_t mode)
{
    int ret = -1;
    char *newName;
    int mediaIdx = CheckPathValid((char *)path, &newName);
    if(mediaIdx >= 0)
        ret = fx_directory_create(partitionInfoTable[mediaIdx].media, newName);

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXStat(const char *path, struct stat *sbuf)
{
    int ret = -1;
    UINT         attributes, year, month, day;
    ULONG        size;
    UINT         hour, minute, second;
    struct tm t;
    char *newName;

    int mediaIdx = CheckPathValid((char *)path, &newName);
    if(mediaIdx >= 0)
    {
        ret = fx_directory_information_get(partitionInfoTable[mediaIdx].media, newName, &attributes, &size,
                                      &year, &month, &day,
                                      &hour, &minute, &second);
    }
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }

    sbuf->st_dev    = ITP_DEVICE_FAT;
    sbuf->st_ino    = mediaIdx;

    sbuf->st_mode   = 0;
    if ((attributes & FX_HIDDEN) == 0)
        sbuf->st_mode |= S_IREAD;

    if ((attributes & FX_READ_ONLY) == 0)
        sbuf->st_mode |= S_IWRITE;

    if ((attributes & FX_ARCHIVE) == 0)
        sbuf->st_mode |= S_IEXEC;

    if (attributes & FX_DIRECTORY)
        sbuf->st_mode |= S_IFDIR;
    else
        sbuf->st_mode |= S_IFREG;

    sbuf->st_size   = size;

    t.tm_sec        = 0;
    t.tm_min        = 0;
    t.tm_hour       = 0;
    t.tm_mday       = day;
    t.tm_mon        = month;
    t.tm_year       = year;
    t.tm_isdst      = -1;
    sbuf->st_atime  = mktime(&t);

    // Jack TBD, FileX no API to get modification time
    t.tm_sec        = second;
    t.tm_min        = minute;
    t.tm_hour       = hour;
    t.tm_mday       = day;
    t.tm_mon        = month;
    t.tm_year       = year;
    sbuf->st_mtime  = mktime(&t);

    return 0;
}

static int FileXStatvfs(const char *path, struct statvfs *sbuf)
{
    int ret;
    int volume = toupper(path[0]) - 'A';
    F_PHY phy;
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    Driver* driver;
    ULONG64 total, free;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);
    driveStatus = &driveStatusTable[volume];
    driver = &driverTable[driveStatus->disk];

    ret = driver->driver->getphy(driver->driver, &phy);
    if (ret)
    {
        return -1;
    }

	if (!phy.bytes_per_sector)
		phy.bytes_per_sector = F_DEF_SECTOR_SIZE;

    ret = fx_media_extended_space_available(partitionInfoTable[volume].media, &free);
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }

    sbuf->f_bsize    = phy.bytes_per_sector;                        /* file system block size */
    sbuf->f_frsize   = phy.bytes_per_sector;                        /* fragment size */
    sbuf->f_blocks   = partitionInfoTable[volume].media->fx_media_total_clusters * phy.sectors_per_cluster;  /* size of fs in f_frsize units */
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

// FileX could not get information when file opened, only size and mode info
static int FileXFstat(int file, struct stat *st)
{
    int ret, i;

    struct tm t = {0};
    FX_MEDIA *media_ptr;
    if(file < 0 || file >= OPEN_MAX || !openFiles[file].using)
        return -1;
    media_ptr = openFiles[file].file.fx_file_media_ptr;
    for(i = 0 ; i < ITP_MAX_DRIVE; i ++)
    {
        if(partitionInfoTable[i].media == media_ptr)
            break;
    }
    if(i >= ITP_MAX_DRIVE)
    {
        ITP_LOG_ERR("FileXFstat could not find media\r\n");
        return -1;
    }
    st->st_dev = ITP_DEVICE_FAT;
    st->st_ino = i;

    st->st_mode = 0;
    FX_PROTECT
    st->st_size = openFiles[file].file.fx_file_current_file_size;
    if (openFiles[file].file.fx_file_open_mode == FX_OPEN_FOR_WRITE)
        st->st_mode |= S_IWRITE;
    else if (openFiles[file].file.fx_file_open_mode == FX_OPEN_FOR_READ || openFiles[file].file.fx_file_open_mode == FX_OPEN_FOR_READ_FAST)
        st->st_mode |= S_IREAD;
    FX_UNPROTECT
    st->st_blksize = 128 * 1024; // optimized for USB/SD

    st->st_atime = mktime(&t);
    st->st_mtime = mktime(&t);

    return 0;
}

static char* FileXGetcwd(char *buf, size_t size)
{
    int ret = -1;
    char *newName;
    buf[0] = 'A' + currentVolume;
    buf[1] = ':';
    size -= 2;
    if(currentVolume >= 0)
    {
        FX_LOCAL_PATH     prevLocalPath;
        ret = fx_directory_local_path_get_copy(partitionInfoTable[currentVolume].media, buf + 2, size);        
    }
    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return NULL;
    }
    return buf;
}

static int FileXRmdir(const char *path)
{
    int ret = -1;
    char *newName;
    int mediaIdx = CheckPathValid((char *)path, &newName);
    if(mediaIdx >= 0)
    {
        ret = fx_directory_delete(partitionInfoTable[mediaIdx].media, newName);
    }

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXClosedir(DIR *dirp)
{
    free(dirp->dd_buf);
    free(dirp);
    return 0;
}

static DIR* FileXOpendir(const char *dirname)
{
    DIR* dirp = malloc(sizeof (DIR));
    char *name_buf;
    int name_len;
    if (!dirp)
    {
        errno = ENOMEM;
        return NULL;
    }

    {
        FN_FIND* find = malloc(sizeof (FN_FIND) + sizeof (struct dirent) + sizeof (FX_PATH) + strlen(dirname) + 1);
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

        name_buf = &dirp->dd_buf[sizeof(FN_FIND) + sizeof(struct dirent) + sizeof (FX_PATH)];
        strcpy(name_buf, dirname);
        name_len = strlen(name_buf);
        // Check if target dir not root, delete slash for FileX API (fx_directory_default_set/fx_directory_path_set)
        if(name_len > 3 && (name_buf[name_len - 1] == '\\' || name_buf[name_len - 1] == '/'))
            name_buf[name_len - 1] = '\0';

    }
    return dirp;
}

static struct dirent* FileXReaddir(DIR *dirp)
{
    struct dirent* d = NULL;
    char buf[PATH_MAX + 1];
    char full_path_buf[PATH_MAX + 1];
    int ret = -1;
    void *ddBuf = dirp->dd_buf;
    FN_FIND* find = (FN_FIND*)(ddBuf);
    FX_PATH *search_path;
    char *dirname;
    if (dirp->dd_size == sizeof (F_WFIND))
    {
        search_path = (FX_PATH *)(dirp->dd_buf + sizeof (F_WFIND) + sizeof(struct dirent));
        dirname = &dirp->dd_buf[sizeof(F_WFIND) + sizeof(struct dirent) + sizeof (FX_PATH)];
    }
    else
    {
        search_path = (FX_PATH *)(dirp->dd_buf + sizeof (FN_FIND) + sizeof(struct dirent));
        dirname = &dirp->dd_buf[sizeof(FN_FIND) + sizeof(struct dirent) + sizeof (FX_PATH)];
    }
    char *newName;
    int mediaIdx = CheckPathValid(dirname, &newName);
    if(mediaIdx >= 0)
    {
        if (dirp->dd_size == sizeof (F_WFIND))
            d = (struct dirent*)(dirp->dd_buf + sizeof (F_WFIND));       
        else
            d = (struct dirent*)(dirp->dd_buf + sizeof (FN_FIND));       
        
        if (dirp->dd_loc >= 0)
        {            
            ret = fx_directory_path_next_entry_find(partitionInfoTable[mediaIdx].media, search_path, buf);
            if (ret)
                return NULL;
        }
        else
        {            
            if((ret = fx_directory_path_set(partitionInfoTable[mediaIdx].media, search_path, newName)) == 0)
            {                
                ret = fx_directory_path_first_entry_find(partitionInfoTable[mediaIdx].media, search_path, buf);
            }   
            if (ret)
                return NULL;
        }
        UINT         attributes, year, month, day;
        ULONG        size;
        UINT         hour, minute, second;
        strcpy(full_path_buf, newName);
        // Check if dir is not root, concat all path.
        if(strlen(newName) > 1)
            strcat(full_path_buf, "/");
        strcat(full_path_buf, buf);

        ret = fx_directory_information_get(partitionInfoTable[mediaIdx].media, full_path_buf, &attributes, &size,
                                        &year, &month, &day,
                                        &hour, &minute, &second);
        if (ret)
            return NULL;
        d->d_ino    = ++dirp->dd_loc;
        d->d_type   = (attributes & FX_DIRECTORY) ? DT_DIR : DT_REG;
        d->d_namlen = strlen(buf);
        strncpy(d->d_name, buf, d->d_namlen+1);
    }
    return d;
}

static void FileXRewinddir(DIR *dirp)
{
    intptr_t ret;
   
    FN_FIND* find = (FN_FIND*) dirp->dd_buf;
    char *dirname = &dirp->dd_buf[sizeof(FN_FIND) + sizeof(struct dirent) + sizeof (FX_PATH)];
    char *newName; 
    int mediaIdx = CheckPathValid(dirname, &newName);

    if(mediaIdx >= 0)
    {
        dirp->dd_loc  = -1;
    }    
}

static long FileXFtell(int file)
{
    if(file < 0 || file >= OPEN_MAX || !openFiles[file].using)
        return -1;
    FX_MEDIA * media_ptr = openFiles[file].file.fx_file_media_ptr;
    long ret;
    FX_PROTECT
    ret = openFiles[file].file.fx_file_current_file_offset;
    FX_UNPROTECT
    return ret;
}

static int FileXFflush(int file)
{
    if(file < 0 || file >= OPEN_MAX || !openFiles[file].using)
        return -1;
    int ret = fx_media_flush(openFiles[file].file.fx_file_media_ptr);

    #ifdef  CFG_NAND_ENABLE
    ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
    #endif

    if (ret)
    {
        return EOF;
    }
    return 0;
}

static int FileXFeof(int file)
{
    if(file < 0 || file >= OPEN_MAX || !openFiles[file].using)
        return -1;
    FX_MEDIA * media_ptr = openFiles[file].file.fx_file_media_ptr;
    FX_PROTECT
    if(openFiles[file].file.fx_file_current_file_offset >= openFiles[file].file.fx_file_current_file_size)
    {
        FX_UNPROTECT
        return EOF;
    }
    FX_UNPROTECT
    return 0;
}

static int FileXTruncate(const char *path, off_t length)
{
    int ret = -1;
    int file = FileXOpen(path, O_WRONLY, 0, NULL);
    if(file >= 0)
    {
        ret = fx_file_truncate_release(&openFiles[file].file, length);
        FileXClose(file, NULL);
        return ret;
    } 
    return ret;
}

static int FileXFtruncate(int file, off_t length)
{
    int ret = -1;
    if(file >= 0)
    {
        ret = fx_file_truncate_release(&openFiles[file].file, length);
        openFiles[file].lastRet = ret;        
    } 
    return ret;
}

static int FileXMount(ITPDisk disk, bool force, bool noexbr)
{
    int ret, i, j;
    ITPDriveStatus* driveStatus = NULL;
    Driver* driver;
    F_PARTITION partitions[ITP_MAX_PARTITION] = {0};
    ITPDriveStatus* driveStatusTable = NULL;
    uint8_t fat_exbr[ITP_FAT_EXBR_SIZE] = {0};
    FILE *f = NULL;
    uint64_t exbr_start = 0;
    ULONG detectedErrors;
    VOID (*driverFunc)(FX_MEDIA *);
    bool enableFT;
    
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
    for (i = 0; i < ITP_MAX_PARTITION; i++)
    {
        F_PARTITION* partition = &partitions[i];
        unsigned char *faultTolerantMemory;
        unsigned char *sratchMemory;
        int writable = 1;
        //only allow partition0 with secnum == 0.
        if (partition->secnum == 0 && i)
        {
            break;
        }

        // find an empty drive
        for (; j < ITP_MAX_DRIVE; j++)
        {
            driveStatus = &driveStatusTable[j];

            if (!driveStatus->avail)
                break;
        }

        //ret = f_initvolumepartition(j, driver->driver, i);
        if(j >= ITP_MAX_DRIVE)
            return F_ERR_NOTAVAILABLE;
        partitionInfoTable[j].media = malloc(sizeof(FX_MEDIA));
        memset(partitionInfoTable[j].media, 0, sizeof(FX_MEDIA));
        partitionInfoTable[j].drvInfo = malloc(sizeof(FX_DRV_INFO));
        memset(partitionInfoTable[j].drvInfo, 0, sizeof(FX_DRV_INFO));
        partitionInfoTable[j].drvInfo->sectorStart = partition->secstart;
        partitionInfoTable[j].drvInfo->sectorNum = partition->secnum;
        partitionInfoTable[j].drvInfo->driver = driver->driver;
        partitionInfoTable[j].driveStatus = driveStatus;
        partitionInfoTable[j].memoryPtr = malloc(FILEX_MEDIA_MEMORY_BUFF_SIZE + FILEX_FAULT_TOLERANT_MEMORY_BUFF_SIZE);
        if(!partitionInfoTable[j].media || !partitionInfoTable[j].drvInfo || !partitionInfoTable[j].memoryPtr)
        {
            ITP_LOG_ERR("FileX Out of memory\r\n");
            return -1;
        }
#if defined(CFG_BUILD_LEVELX)        
        if(disk == ITP_DISK_NOR && !partition->readonly)
        {
            F_PHY phy;
            driver->driver->getphy(driver->driver, &phy);
            writable = 1;
            partitionInfoTable[j].drvInfo->lxDriverInfo.wearDriver = malloc(sizeof(LX_NOR_FLASH));
            partitionInfoTable[j].drvInfo->lxDriverInfo.wearCache = malloc(LEVELX_EXTERNAL_CACHE_SIZE);
            partitionInfoTable[j].drvInfo->lxDriverInfo.wearCacheSize = LEVELX_EXTERNAL_CACHE_SIZE;
            partitionInfoTable[j].drvInfo->lxDriverInfo.eraseReq = 0;
            partitionInfoTable[j].drvInfo->lxDriverInfo.wearBuf = malloc(phy.bytes_per_sector);
            if(!partitionInfoTable[j].drvInfo->lxDriverInfo.wearDriver || !partitionInfoTable[j].drvInfo->lxDriverInfo.wearCache || !partitionInfoTable[j].drvInfo->lxDriverInfo.wearBuf)
            {
                ITP_LOG_ERR("LevelX Out of memory\r\n");
                return -1;
            }
            driverFunc = fx_nor_flash_lx_driver;
            enableFT = true;
        }
        else if(disk == ITP_DISK_NOR)
        {
            writable = 0;
            driverFunc = fx_driver;
            enableFT = false;
        }
        else
#else
        if(disk == ITP_DISK_NOR)
        {
            writable = 1;
            driverFunc = fx_driver;
            enableFT = false;
        }
        else
#endif      
        {
            writable = 1;
            driverFunc = fx_driver;
            enableFT = true;
        }
        partitionInfoTable[j].drvInfo->writeProtect = writable? 0 : 1;
        faultTolerantMemory = partitionInfoTable[j].memoryPtr  + FILEX_MEDIA_MEMORY_BUFF_SIZE;
        
        ret = fx_media_open(partitionInfoTable[j].media, "FileX", driverFunc, partitionInfoTable[j].drvInfo, partitionInfoTable[j].memoryPtr, FILEX_MEDIA_MEMORY_BUFF_SIZE);
        /*if( (disk==5) && (ret == F_ERR_NOTFORMATTED) )
        {
            ret = (int)FatFormat((int)j);
        }*/

        if (ret && !force)
        {
            if (partition->secnum == 0)
            {
                errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
                printf("F_ERR_NOTFORMATTED %X\r\n", ret);
                return F_ERR_NOTFORMATTED;
            }
            else
            {
                continue;
            }
        }
#if defined(FX_ENABLE_FAULT_TOLERANT)          
        if(!force && enableFT)
        {
            ret = fx_fault_tolerant_enable(partitionInfoTable[j].media, faultTolerantMemory, FILEX_FAULT_TOLERANT_MEMORY_BUFF_SIZE);
            
            if (ret)
            {
                ITP_LOG_ERR("fx_fault_tolerant_enable fail %d %d\r\n", j, ret);
            }
        }
#endif
#if defined(ENABLE_FILEX_MEDIA_CHECK)
        if (!noexbr && !force && disk == ITP_DISK_NOR)
        {
            sratchMemory = malloc(FILEX_MEDIA_CHECK_MEMORY_BUFF_SIZE);
            if(sratchMemory)
            {
                TickType_t time = xTaskGetTickCount();
                ret = fx_media_check(partitionInfoTable[j].media, sratchMemory, FILEX_MEDIA_CHECK_MEMORY_BUFF_SIZE, 
                                    FX_FAT_CHAIN_ERROR | FX_DIRECTORY_ERROR | FX_LOST_CLUSTER_ERROR, &detectedErrors);
                free(sratchMemory);
                if (ret)
                {
                    ITP_LOG_ERR("fx_media_check fail %d %d\r\n", j, ret);
                }
            }
            else
                ITP_LOG_ERR("Alloc sratchMemory fail\r\n");
        }
#endif
#if defined(CFG_BUILD_LEVELX)
        /*if(disk == ITP_DISK_NOR)
        {
            ret = fx_media_flush(partitionInfoTable[j].media);
            if (ret) ITP_LOG_ERR("fx_media_flush fail %d %d\r\n", j, ret);
        }*/
#endif            
        driveStatus->disk       = disk;
        driveStatus->device     = ITP_DEVICE_FAT;
        driveStatus->avail      = true;
        driveStatus->name[0]    = 'A' + j;
        driveStatus->name[1]    = ':';
        driveStatus->name[2]    = '/';
        driveStatus->writable   = writable;

        write(ITP_DEVICE_DRIVE, driveStatus, sizeof (ITPDriveStatus));
        if(!force && enableFT && !skipCheckTempFiles)
        {           
            ClearTempFiles(driveStatus->name, false);
        }
        if (partition->secnum == 0)
        {
            break;
        }
    }

    return 0;
}

static int FileXUnmount(ITPDisk disk)
{
    int i, ret1 = -1, ret2;
    ITPDriveStatus* driveStatusTable = NULL;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    // deinit all drives of this disk
    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        ITPDriveStatus* driveStatus = &driveStatusTable[i];
        if (driveStatus->avail && driveStatus->disk == disk && driveStatus->device == ITP_DEVICE_FAT)
        {
            ret2 = fx_media_close(partitionInfoTable[i].media);
            if (ret2 == 0)
            {
                ret1 = 0;
            }
            else
            {
                ITP_LOG_ERR("fx_media_close(%d) fail: %d\n", i, ret2 );
                if (ret1 <= 0)
                    ret1 = ret2;
            }
#if defined(CFG_BUILD_LEVELX)
            if(disk == ITP_DISK_NOR)
            {   
                if(partitionInfoTable[i].drvInfo->lxDriverInfo.wearDriver)
                {
                    free(partitionInfoTable[i].drvInfo->lxDriverInfo.wearDriver);
                    partitionInfoTable[i].drvInfo->lxDriverInfo.wearDriver = NULL;
                }
                if(partitionInfoTable[i].drvInfo->lxDriverInfo.wearCache)
                {
                    free(partitionInfoTable[i].drvInfo->lxDriverInfo.wearCache);
                    partitionInfoTable[i].drvInfo->lxDriverInfo.wearCache = NULL;
                }
                if(partitionInfoTable[i].drvInfo->lxDriverInfo.wearBuf)
                {
                    free(partitionInfoTable[i].drvInfo->lxDriverInfo.wearBuf);
                    partitionInfoTable[i].drvInfo->lxDriverInfo.wearBuf = NULL;
                }
            }
#endif
            free(partitionInfoTable[i].media);
            free(partitionInfoTable[i].memoryPtr);
            free(partitionInfoTable[i].drvInfo);
            memset(&partitionInfoTable[i].media, 0, sizeof(MEDIA_PARTITION_INFO));

            driveStatus->avail  = false;
            write(ITP_DEVICE_DRIVE, driveStatus, sizeof (ITPDriveStatus));
        }
    }
    ret2 = f_releasedriver(driverTable[disk].driver);
    driverTable[disk].driver = NULL;

    if (ret2 > 0)
    {
        ITP_LOG_ERR("f_releasedriver() fail: %d\n", ret1 );
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret1;

        if (ret1 <= 0)
            ret1 = ret2;
    }
    return ret1;
}

static int FileXFlush(int volume)
{
    if(volume < 0 || volume >= ITP_MAX_DRIVE || !partitionInfoTable[volume].media)
        return -1;
    int ret = fx_media_flush(partitionInfoTable[volume].media);

    if (ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

static int FileXWriteProtect(int volume, int protect)
{
    if(volume < 0 || volume >= ITP_MAX_DRIVE || !partitionInfoTable[volume].media)
        return -1;
    FX_MEDIA * media_ptr = partitionInfoTable[volume].media;        
    FX_PROTECT
    partitionInfoTable[volume].driveStatus->writable = protect? 0 : 1;     
    partitionInfoTable[volume].drvInfo->writeProtect = partitionInfoTable[volume].media->fx_media_driver_write_protect = protect;
    FX_UNPROTECT
    return 0;
}

static int FileXGetPartition(ITPPartition* par)
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
#if defined(CFG_BUILD_LEVELX)
            par->writable[i] = partitions[i].readonly == 0? 1 : 0;
#endif
        }
        else
            break;
    }

    return 0;
}

#define F_DEF_PART_ALIGN 512             /* default partition alignment */

static bool readOnlyTable[] = {
#if defined(CFG_NOR_PARTITION0_READONLY)
    true,
#else
    false,
#endif
#if defined(CFG_NOR_PARTITION1_READONLY)
    true,
#else
    false,
#endif
#if defined(CFG_NOR_PARTITION2_READONLY)
    true,
#else
    false,
#endif
#if defined(CFG_NOR_PARTITION3_READONLY)
    true,
#else
    false,
#endif
#if defined(CFG_NOR_PARTITION4_READONLY)
    true,
#else
    false,
#endif
#if defined(CFG_NOR_PARTITION5_READONLY)
    true,
#else
    false,
#endif              
};      

static int FileXCreatePartition(ITPPartition* par)
{
    F_PARTITION partitions[ITP_MAX_PARTITION] = {0};
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
#ifdef CFG_BUILD_LEVELX        
        if(par->disk == ITP_DISK_NOR)
        {
            if(i < sizeof(readOnlyTable) / sizeof(readOnlyTable[0]))
                partitions[i].readonly = readOnlyTable[i];
        }
#endif
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

        size = (uint64_t)partitions[i].secnum * phy.bytes_per_sector;
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
#if defined(CFG_BUILD_LEVELX)   // LevelX need sure all nor cache are write through
    if(par->disk == ITP_DISK_NOR)
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
    return 0;
}

static int CalcMediaClusterSize(int sectorNum, int fatType)
{
    unsigned char sectorPerCluster = 1;
    if ( fatType == FX_FAT12 )
    {
        sectorPerCluster = 1;
        while ( sectorPerCluster )
        {     /* try FAT12 */
            if ( sectorNum / sectorPerCluster < FX_12_BIT_FAT_SIZE )
            {
                break;
            }

            sectorPerCluster <<= 1;
        }

        if ( !sectorPerCluster )
        {
            return FILEX_ERR_MEDIATOOLARGE;                                          /* fat12 cannot be there */
        }
    }
    else if ( fatType == FX_FAT16 )
    {
        sectorPerCluster = 1;
        while ( sectorPerCluster )
        {     /* try FAT16 */
            if ( sectorNum / sectorPerCluster < FX_16_BIT_FAT_SIZE )
            {
                break;
            }

            sectorPerCluster <<= 1;
        }

        if ( !sectorPerCluster )
        {
            return FILEX_ERR_MEDIATOOLARGE;                                          /* fat16 cannot be there */
        }
    }
    else if ( fatType == FX_FAT32 )
    {
        unsigned int  i;
        for ( i = 0 ; i<( sizeof( FAT32_CS ) / sizeof( FAT32_CLUSTER_SIZE ) ) - 1 && sectorNum > FAT32_CS[i].maxSectors ; i++ )
        {
        }

        sectorPerCluster = FAT32_CS[i].sectorPerCluster;
    }
    return (int)sectorPerCluster;
}
static int CalcMediaType(int sectorNum, int *sectorPerCluster)
{
    int ret;
    ret = CalcMediaClusterSize(sectorNum, FX_FAT16);
    if(ret == FILEX_ERR_MEDIATOOSMALL)  // No use now
    {
        ret = CalcMediaClusterSize(sectorNum, FX_FAT12);
    }
    else if(ret == FILEX_ERR_MEDIATOOLARGE)
    {
        ret = CalcMediaClusterSize(sectorNum, FX_FAT32);
    }
    if(ret > 0)
    {
        *sectorPerCluster = ret;
        return 0;
    }
    return -1;
}

static int FileXCheckMedia(int volume)
{
    int ret;
    ULONG detectedErrors;
    if(volume < 0 || volume >= ITP_MAX_DRIVE || !partitionInfoTable[volume].media)
        return -1;
    unsigned char *sratchMemory = malloc(FILEX_MEDIA_CHECK_MEMORY_BUFF_SIZE);
    if(sratchMemory)
    {
        TickType_t time = xTaskGetTickCount();
        ret = fx_media_check(partitionInfoTable[volume].media, sratchMemory, FILEX_MEDIA_CHECK_MEMORY_BUFF_SIZE, 
                            FX_FAT_CHAIN_ERROR | FX_DIRECTORY_ERROR | FX_LOST_CLUSTER_ERROR, &detectedErrors);
        free(sratchMemory);
        if (ret)
        {
            ITP_LOG_ERR("fx_media_check fail %d %d\r\n", volume, ret);
        }
    }
    else
        ITP_LOG_ERR("Alloc sratchMemory fail\r\n");
    free(sratchMemory);
    return -1;
}

static int FileXFormat(int volume)
{
    F_PHY phy;
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    Driver* driver;
    VOID (*driverFunc)(FX_MEDIA *);
    int ret, fatType, sectorPerCluster;
    bool enableFT = false;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);
    driveStatus = &driveStatusTable[volume];
    driver = &driverTable[driveStatus->disk];

    ret = driver->driver->getphy(driver->driver, &phy);
    fatType = CalcMediaType(partitionInfoTable[volume].drvInfo->sectorNum, &sectorPerCluster);
    ITP_LOG_INFO("fx_media_format %d, start %d, sectors %d, %d %d %d\r\n", volume, partitionInfoTable[volume].drvInfo->sectorStart, 
                                            partitionInfoTable[volume].drvInfo->sectorNum, phy.bytes_per_sector, phy.sectors_per_cluster, sectorPerCluster);
#if defined(CFG_BUILD_LEVELX)
    if (partitionInfoTable[volume].media->fx_media_id == FX_MEDIA_ID)    // opened, need close and re-open
    {
        ret = fx_media_close(partitionInfoTable[volume].media);
    }
    if(driveStatus->disk == ITP_DISK_NOR && driveStatus->writable)
    {
        partitionInfoTable[volume].drvInfo->lxDriverInfo.eraseReq = 1;
        driverFunc = fx_nor_flash_lx_driver;
        // LevelX need reserve at least one block.
        ret = fx_media_format(partitionInfoTable[volume].media, driverFunc, partitionInfoTable[volume].drvInfo, 
                    partitionInfoTable[volume].memoryPtr, FILEX_MEDIA_MEMORY_BUFF_SIZE,
                    "FILEX", 2, 512, partitionInfoTable[volume].drvInfo->sectorStart, partitionInfoTable[volume].drvInfo->sectorNum - phy.sectors_per_cluster,
                    phy.bytes_per_sector, phy.sectors_per_cluster, phy.number_of_heads, phy.sector_per_track, phy.sectors_per_cluster);
        enableFT = driveStatus->writable == 1;
    }
    else
#endif    
    {
        driverFunc = fx_driver;
        ret = fx_media_format(partitionInfoTable[volume].media, driverFunc, partitionInfoTable[volume].drvInfo, 
                    partitionInfoTable[volume].memoryPtr, FILEX_MEDIA_MEMORY_BUFF_SIZE,
                    "FILEX", 2, 512, partitionInfoTable[volume].drvInfo->sectorStart, partitionInfoTable[volume].drvInfo->sectorNum,
                    phy.bytes_per_sector, phy.sectors_per_cluster, phy.number_of_heads, phy.sector_per_track, phy.sectors_per_cluster); 
        enableFT = (driveStatus->writable == 1 && driveStatus->disk != ITP_DISK_NOR);
    }

    if(ret == 0)
    {
        if (partitionInfoTable[volume].media->fx_media_id == FX_MEDIA_ID)    // opened, need close and re-open
        {
            ret = fx_media_close(partitionInfoTable[volume].media);
        }
        ret = fx_media_open(partitionInfoTable[volume].media, "FILEX", driverFunc, partitionInfoTable[volume].drvInfo, 
                            partitionInfoTable[volume].memoryPtr, FILEX_MEDIA_MEMORY_BUFF_SIZE);
        if(ret) ITP_LOG_ERR("fx_media_open fail: %d %X %X\n", ret, (unsigned long)(partitionInfoTable[volume].media), (unsigned long)partitionInfoTable[volume].memoryPtr );
            
#if defined(FX_ENABLE_FAULT_TOLERANT)            
        unsigned char *faultTolerantMemory = partitionInfoTable[volume].memoryPtr  + FILEX_MEDIA_MEMORY_BUFF_SIZE;
        if(enableFT)
        {
            ret = fx_fault_tolerant_enable(partitionInfoTable[volume].media, faultTolerantMemory, FILEX_FAULT_TOLERANT_MEMORY_BUFF_SIZE);
        
            if (ret)
            {
                ITP_LOG_ERR("fx_fault_tolerant_enable fail %d %d\r\n", volume, ret);
            }   
        } 
#endif
    }
    else
        ITP_LOG_ERR("fx_media_format fail: %d\n", ret );
    if(ret)
    {
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | FxToErrno(ret);
        return -1;
    }
    return 0;
}

typedef struct Node 
{
    char *path;
    struct Node *next;
} Node;

typedef struct 
{
    Node *front;
    Node *rear;
} Queue;

static void enqueue(Queue *q, const char *path) 
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("malloc error");
        return;
    }
    newNode->path = strdup(path);
    newNode->next = NULL;

    if (!q->rear) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

static char *dequeue(Queue *q) 
{
    if (!q->front) return NULL;

    Node *temp = q->front;
    char *path = temp->path;

    q->front = q->front->next;
    if (!q->front) q->rear = NULL;

    free(temp);
    return path;
}

static int HandleTempFile(const char *filename) 
{
    const char *extension = FILEX_TEMP_FILE_EXT;
    size_t len_filename = strlen(filename);
    size_t len_extension = strlen(extension);
    int ret = 0;
    if (len_filename > len_extension &&
            strcmp(filename + len_filename - len_extension, extension) == 0)
    {
        char newFilePath[NAME_MAX + 1];
        char *newName;
        char *bakNewName;
        UINT         attributes, year, month, day;
        ULONG        size;
        UINT         hour, minute, second;
        int mediaIdx;
        int filePathLen;
        int tempFileExtPathLen = strlen(FILEX_TEMP_FILE_EXT);

        filePathLen = strlen(filename);
        strncpy(newFilePath, filename, filePathLen - tempFileExtPathLen);
        newFilePath[filePathLen - tempFileExtPathLen] = '\0';
        mediaIdx = CheckPathValid((char *)newFilePath, &newName);
        CheckPathValid((char *)filename, &bakNewName);
              
        if(mediaIdx >= 0)
        {            
            ret = fx_directory_information_get(partitionInfoTable[mediaIdx].media, newName, &attributes, &size,
                                        &year, &month, &day,
                                        &hour, &minute, &second);
            if (!ret)
            {
                // Original file exist, delete bak
                ULONG size2;
                fx_directory_information_get(partitionInfoTable[mediaIdx].media, (char*)filename, &attributes, &size2,
                                        &year, &month, &day,
                                        &hour, &minute, &second);
                ret = fx_file_delete(partitionInfoTable[mediaIdx].media, bakNewName);
                if(ret)
                    ITP_LOG_ERR("fx_file_delete %s fail: %d\n", newFilePath, ret );
            }
            else
            {
                // Original file not exist, rename bak to ori file
                ret = fx_file_rename(partitionInfoTable[mediaIdx].media, bakNewName, newName);
                if(ret)
                    ITP_LOG_ERR("fx_file_rename %s to %s fail: %d\n", bakNewName, newName, ret );
            }
        }
    }
    return ret;
}

static void SearchTempFiles(const char *root_dir) 
{
    Queue queue = {NULL, NULL};
    enqueue(&queue, root_dir);

    char *current_path;
    while ((current_path = dequeue(&queue)) != NULL) {
        DIR *dir = opendir(current_path);
        if (!dir) {
            perror("opendir error");
            free(current_path);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            size_t path_len = strlen(current_path) + strlen(entry->d_name) + 2;
            char *full_path = (char *)malloc(path_len);
            if (!full_path) {
                perror("malloc error");
                continue;
            }
            int needsSlash = (current_path[strlen(current_path) - 1] != '/');
            if(needsSlash)
                snprintf(full_path, path_len, "%s/%s", current_path, entry->d_name);
            else
                snprintf(full_path, path_len, "%s%s", current_path, entry->d_name);
            if ((entry->d_type & DT_DIR) != 0) {
                enqueue(&queue, full_path);
            } else if (HandleTempFile(full_path)) {
                ITP_LOG_WARN("HandleTempFile fail: %d\n", full_path );
            }

            free(full_path);
        }

        closedir(dir);
        free(current_path);
    }
}

static void* ClearTempFilesTask(void* arg)
{
    SearchTempFiles((const char *)arg);
    return NULL;
}

static void ClearTempFiles(const char *root_dir, bool async)
{
    if (async)
    {
        pthread_attr_t attr;
        pthread_t id;        
        pthread_attr_init(&attr);
        attr.stacksize = 128 * 1024;            
        pthread_create(&id, &attr, ClearTempFilesTask, (void*)root_dir);
    }
    else
        SearchTempFiles(root_dir);
}

static int FileXIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int ret = 0;

    switch (request)
    {
    case ITP_IOCTL_MOUNT:
        ret = FileXMount((ITPDisk)ptr, false, false);
        if (ret)
        {
            return -1;
        }
        break;

    case ITP_IOCTL_FORCE_MOUNT:
        ret = FileXMount((ITPDisk)ptr, true, false);
        if (ret)
        {
            return -1;
        }
        break;

    case ITP_IOCTL_MOUNT_NOEXBR:
        ret = FileXMount((ITPDisk)ptr, false, true);
        if (ret)
        {
            return -1;
        }
        break;

    case ITP_IOCTL_FORCE_MOUNT_NOEXBR:
        ret = FileXMount((ITPDisk)ptr, true, true);
        if (ret)
        {
            return -1;
        }
        break;

    case ITP_IOCTL_UNMOUNT:
        ret = FileXUnmount((ITPDisk)ptr);
        if (ret)
        {
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
            return -1;
        }
        break;

    case ITP_IOCTL_DISABLE:
        if (!fatInited)
            break;

        f_releaseFS();
        break;

    case ITP_IOCTL_GET_PARTITION:
        ret = FileXGetPartition((ITPPartition*)ptr);
        if (ret)
        {
            return -1;
        }
        break;

    case ITP_IOCTL_CREATE_PARTITION:
        ret = FileXCreatePartition((ITPPartition*)ptr);
        if (ret)
        {
            return -1;
        }
        break;

    case ITP_IOCTL_FORMAT:
        ret = FileXFormat((int)ptr);
        if (ret)
        {
            return -1;
        }
        break;

    case ITP_IOCTL_INIT:
        ret = fs_init();
        if (ret)
        {
            return -1;
        }
        ret = fs_start();
        if (ret)
        {
            return -1;
        }
#if defined(CFG_BUILD_LEVELX)
        lx_nor_flash_initialize();
#endif
        fx_system_initialize();
        
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
            return 0;
        }
        ret = (int)&driverTable[(int)ptr];
        break;
    case ITP_IOCTL_FLUSH:
        ret = FileXFlush((int)ptr);
        if (ret)
        {
            return -1;
        }
        break;
    case ITP_IOCTL_SET_WRITE_PROTECT:
        {
            ITP_FAT_SET_PARAM *param = (ITP_FAT_SET_PARAM *)ptr;
            FileXWriteProtect(param->volume, param->param1);
        }
        break;
    case ITP_IOCTL_CHK_DSK:
        ret = FileXCheckMedia((int)ptr);
        if (ret)
        {
            return -1;
        }
        break;
    case ITP_IOCTL_SKIP_CHK_TMP_FILE:
        skipCheckTempFiles = (bool)ptr;
        break;
    default:
        errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        return -1;
    }
    return ret;
}

const ITPFSDevice itpFSDeviceFat =
{
    {
        ":FileX",
        FileXOpen,
        FileXClose,
        FileXRead,
        FileXWrite,
        FileXLseek,
        FileXIoctl,
        NULL
    },
    FileXRemove,
    FileXRename,
    FileXChdir,
    FileXChmod,
    FileXMkdir,
    FileXStat,
    FileXStatvfs,
    FileXFstat,
    FileXGetcwd,
    FileXRmdir,
    FileXClosedir,
    FileXOpendir,
    FileXReaddir,
    FileXRewinddir,
    FileXFtell,
    FileXFflush,
    FileXFeof,
    FileXTruncate,
    FileXFtruncate
};
