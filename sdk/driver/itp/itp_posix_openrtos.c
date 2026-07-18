/* scandir.c
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* getdelim.c
 * Copyright 2002, Red Hat Inc. - all rights reserved
 */

/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL Posix OpenRTOS functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <sys/unistd.h>
#include <sys/utime.h>
#include <sys/utsname.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fnmatch.h>
#include <ftw.h>
#include <iconv.h>
#include <limits.h>
#include <malloc.h>
#include <mqueue.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#ifdef CFG_NET_ENABLE
#include <arpa/inet.h>
#endif
#include "openrtos/FreeRTOS.h"
#include "openrtos/semphr.h"
#include "openrtos/task.h"
#include "openrtos/timers.h"
#include "openrtos/event_groups.h"
#include "itp_cfg.h"

#ifdef CFG_NAND_ENABLE
#include "ite/ite_nand.h"
#endif
/**
 * Pthread mutex handle implementation definition.
 */
typedef struct
{
    void* xMutex;   ///< Native mutex.
    int type;       ///< Pthread mutex type.
} itpMutex;

/**
 * Pthread key handle implementation definition.
 */
typedef struct
{
    bool used;              ///< Indicates used or not.
    void (*dtor)(void*);    ///< Destructor callback function.
    void* value;            ///< Key value.
} itpThreadKey;

/**
 * PThread handle implementation definition.
 */
typedef struct
{
    void* xHandle;                          ///< Native thread handle.
    void* xSemaphore;                       ///< Used for joinable thread.
    void* (*start_routine)(void*);          ///< Thread function.
    void* arg;                              ///< Thread function argument.
    void* value_ptr;                        ///< Result value of thread function.
    int detachstate;                        ///< Pthread detach state.
    itpThreadKey keys[PTHREAD_KEYS_MAX];    ///< Thread keys.
    int volume;                             ///< Current disk volume.
    struct _pthread_cleanup_context* cleanup_context;
} itpThread;

/**
 * PThread condition handle implementation definition.
 */
typedef struct
{
    void* xSemaphore;   ///< Native semaphore.
    int count;          ///< condition count.
} itpCondition;

/**
 * POSIX message queue handle implementation definition.
 */
typedef struct
{
    void* xQueue;               ///< Native message queue.
    unsigned long uxItemSize;   ///< Queue item size.
    int oflag;                  ///< Open flags.
} itpQueue;

int _kill(int pid, int sig)
{
    (void)pid;  /* avoid warnings */
    (void)sig;  /* avoid warnings */
    errno = EINVAL;
    return -1;
}

void __libc_fini_array(void);
void  Rmalloc_stat(const char *file);

void _exit(int status)
{
    static bool exiting = false;
    extern void ithDmaWaitAxiSpiIdle(void);
    if (exiting)
    {
        while (true) { }
    }

    exiting = true;

    if (status != 0)
    {
        // backtrace
        itpPrintBacktrace();

        // clock
        ithClockStats();

        // cmdq
    #ifdef CFG_CMDQ_ENABLE
        ithCmdQStats();
    #endif

        // memory usage
        malloc_stats();

        // task list
    #if (configUSE_TRACE_FACILITY == 1)
        {
            static signed char buf[2048];
            vTaskList(buf);
            (void)ithPrintf(buf);
        }
    #endif

    #ifdef CFG_DBG_RMALLOC
        Rmalloc_stat(__FILE__);
    #endif
    }
    else
    {
        __libc_fini_array();
    }

#ifdef CFG_NAND_ENABLE
    iteNfSwitchToDie0InISR();
#endif

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif

#if (CFG_LFS_CACHE_SIZE > 0)
    ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
#endif

    (void)ithPrintf("exit(%d), reboot...\n", status);
    ithEnterCritical();
    ithDmaWaitAxiSpiIdle();
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_SPI_CLK_REG, 1UL << ITH_SPI_RST_BIT, 1UL << ITH_SPI_RST_BIT);
#if 0
    ithExitCritical();
#endif

#ifdef CFG_WATCHDOG_INTR
    ithIntrDisableFiq(ITH_INTR_WD);
#endif // CFG_WATCHDOG_INTR
    ithWatchDogCtrlEnable(ITH_WD_RESET);
    ithWatchDogEnable();
    ithWatchDogSetReload(0);
    ithWatchDogRestart();

    vTaskEndScheduler();
    while (true) { }
}

/*********add for carplay lib*************/
int getentropy(void *buf, size_t buflen)
{
    return -1;
}
/****************************************/

int _getpid(void)
{
    return (int) xTaskGetCurrentTaskHandle();
}

extern uint8_t __heap_start__[];
extern uint8_t __heap_end__[];
static uint8_t* heap_ptr = NULL;

caddr_t _sbrk(int incr)
{
    void* base;

    if (heap_ptr == NULL)
    {
        heap_ptr = (void *)__heap_start__;
    }

    if ((void *)(heap_ptr + incr) <= (void *)(__heap_end__))
    {
        base = heap_ptr;
        heap_ptr += incr;
        return (void *)(base);
    }
    else
    {
#if defined(CFG_BUILD_DEBUG) || defined(CFG_BUILD_DEBUGREL)
    #define BACKTRACE_SIZE 100
        static void * btbuf[BACKTRACE_SIZE];
        int           i, btcount = backtrace(btbuf, BACKTRACE_SIZE, 0);

#endif // defined(CFG_BUILD_DEBUG) || defined(CFG_BUILD_DEBUGREL)

        (void)ithPrintf("Out of memory: start=0x%" PRIx32 " end=0x%" PRIx32 " incr=0x%" PRIx32 " ptr=0x%" PRIx32 "\n",
                  __heap_start__, __heap_end__, incr, heap_ptr);
        malloc_stats();

#ifdef CFG_DBG_RMALLOC
        Rmalloc_stat(__FILE__);
#endif

#if defined(CFG_BUILD_DEBUG) || defined(CFG_BUILD_DEBUGREL)
        // backtrace
        for (i = 0; i < btcount; i++)
        {
            (void)ithPrintf("0x%" PRIx32 " ", btbuf[i]);
        }

        (void)ithPrintf("\n");
#endif // defined(CFG_BUILD_DEBUG) || defined(CFG_BUILD_DEBUGREL)

        errno = ENOMEM;
        return (void *)(-1);
    }
}

int _open(const char* name, int flags, int mode)
{
    int i, subfile;

    if (NULL == name)
    {
        errno = EFAULT;
        return -1;
    }

    if (name[0] == ':')
    {
        char* subname = strchr(&name[1], ':');
        int len = (NULL != subname) ? (subname - name) : strlen(name);

        // try to open custom device
        for (i = ITP_DEVICE_CUSTOM >> ITP_DEVICE_BIT; i < ITP_DEVICE_MAX; i++)
        {
            const ITPDevice* dev = itpDeviceTable[i];

            if ((NULL != dev) && strncmp(name, dev->name, len) == 0)
            {
                subfile = dev->open(subname + 1, flags, mode, dev->info);
                if (subfile == -1)
                {
                    return -1;
                }

                return (i << ITP_DEVICE_BIT) | subfile;
            }
        }
    }
#if defined(CFG_FS_FAT) || defined(CFG_FS_LFS) || defined(CFG_FS_YAFFS2)
    else
    {
        // try to open file system
        itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        ITPDriveStatus* driveStatusTable = NULL;
        ITPDriveStatus* driveStatus;

        ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

        if (name[1] == ':')
        {
            driveStatus = &driveStatusTable[toupper(name[0]) - 'A'];
        }
        else
        {
            driveStatus = &driveStatusTable[t->volume];
        }

        if (driveStatus->avail)
        {
            const ITPDevice* dev = (ITPDevice*) itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
            if (NULL != dev)
            {
                subfile = dev->open(name, flags, mode, dev->info);
                if (subfile != -1)
                {
                    return driveStatus->device | subfile;
                }
                else
                {
                    return -1;
                }
            }
        }
    }
#endif // defined(CFG_FS_FAT)
    errno = ENOENT;
    return -1;
}

int _close(int file)
{
    const ITPDevice* dev;

    if (file < 0)
    {
        ITP_LOG_ERR("_close(0x%X) invalid handle\n", file);
        errno = ENOENT;
        return -1;
    }

    if (file <= STDERR_FILENO)
    {
        return 0;
    }

    if ((((uint32_t)file) & ITP_DEVICE_MASK) == 0U)
    {
        dev = itpDeviceTable[ITP_DEVICE_SOCKET >> ITP_DEVICE_BIT];
    }
    else
    {
        uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
        if (index >= ITP_DEVICE_MAX)
        {
            errno = ENOENT;
            return -1;
        }
        dev = itpDeviceTable[index];
    }

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return dev->close((((uint32_t)file) & ITP_FILE_MASK), dev->info);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int _read(int file, char *ptr, int len)
{
    const ITPDevice* dev;
    uint32_t index;

    if (file < 0)
    {
        ITP_LOG_ERR("_read(0x%X) invalid handle\n", file);
        errno = ENOENT;
        return -1;
    }
    index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }
    dev = itpDeviceTable[index];

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return dev->read((((uint32_t)file) & ITP_FILE_MASK), ptr, len, dev->info);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int itpRead(int file, char *ptr, int len)
{
    const ITPDevice* dev;

    if (file < 0)
    {
        ITP_LOG_ERR("itpRead(0x%X) invalid handle\n", file);
        errno = ENOENT;
        return -1;
    }

    if ((((uint32_t)file) & ITP_DEVICE_MASK) == 0U)
    {
        dev = itpDeviceTable[ITP_DEVICE_SOCKET >> ITP_DEVICE_BIT];
    }
    else
    {
        uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
        if (index >= ITP_DEVICE_MAX)
        {
            errno = ENOENT;
            return -1;
        }
        dev = itpDeviceTable[index];
    }

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return dev->read((((uint32_t)file) & ITP_FILE_MASK), ptr, len, dev->info);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int _write(int file, char *ptr, int len)
{
    const ITPDevice* dev;
    uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;

    if (file < 0)
    {
        ITP_LOG_ERR("_write(0x%X) invalid handle\n", file);
        errno = ENOENT;
        return -1;
    }
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }

    dev = itpDeviceTable[index];

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return dev->write((((uint32_t)file) & ITP_FILE_MASK), (char*)ptr, len, dev->info);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int itpWrite(int file, char *ptr, int len)
{
    const ITPDevice* dev;

    if (file < 0)
    {
        ITP_LOG_ERR("itpWrite(0x%X) invalid handle\n", file);
        errno = ENOENT;
        return -1;
    }

    if ((((uint32_t)file) & ITP_DEVICE_MASK) == 0U)
    {
        dev = itpDeviceTable[ITP_DEVICE_SOCKET >> ITP_DEVICE_BIT];
    }
    else
    {
        uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
        if (index >= ITP_DEVICE_MAX)
        {
            errno = ENOENT;
            return -1;
        }
        dev = itpDeviceTable[index];
    }

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return dev->write((((uint32_t)file) & ITP_FILE_MASK), ptr, len, dev->info);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int _lseek(int file, int ptr, int dir)
{
    const ITPDevice* dev;
    uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;

    if (file < 0)
    {
        ITP_LOG_ERR("_lseek(0x%X) invalid handle\n", file);
        errno = ENOENT;
        return -1;
    }
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }
    dev = itpDeviceTable[index];

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return dev->lseek((((uint32_t)file) & ITP_FILE_MASK), ptr, dir, dev->info);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int ioctl(int file, unsigned long request, void* ptr)
{
    const ITPDevice* dev;

    if (file < 0)
    {
        ITP_LOG_ERR("ioctl(0x%X) invalid handle\n", file);
        errno = ENOENT;
        return -1;
    }

    if ((((uint32_t)file) & ITP_DEVICE_MASK) == 0U)
    {
        dev = itpDeviceTable[ITP_DEVICE_SOCKET >> ITP_DEVICE_BIT];
    }
    else
    {
        uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
        if (index >= ITP_DEVICE_MAX)
        {
            errno = ENOENT;
            return -1;
        }
        dev = itpDeviceTable[index];
    }

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.      */
        return dev->ioctl((((uint32_t)file) & ITP_FILE_MASK), request, ptr, dev->info);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int _unlink(const char *path)
{
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    int ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice* fsdev = (ITPFSDevice*) itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != fsdev)
        {
            ret = fsdev->remove(path);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }
    return ret;
}

int _rename(const char *oldname, const char *newname)
{
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    int ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (oldname[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(oldname[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->rename(oldname, newname);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }
    return ret;
}

int chdir(const char *path)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    int ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        driveStatus = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice* dev = (ITPFSDevice*) itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->chdir(path);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }

    if (0 != ret)
    {
        errno = EINVAL;
    }
    else
    {
        if (strlen(path) > 2 && path[1] == ':')
        {
            t->volume = toupper(path[0]) - 'A';
        }
    }
    return ret;
}

int chmod(const char *path, mode_t mode)
{
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    int ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->chmod(path, mode);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }

    if (0 != ret)
    {
        errno = EINVAL;
    }

    return ret;
}

int mkdir(const char *path, mode_t mode)
{
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus * driveStatus;
    int              ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->mkdir(path, mode);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }

    if (0 != ret)
    {
        errno = EINVAL;
    }

    return ret;
}

int _stat(const char *path, struct stat *buf)
{
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    int ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);
    if (driveStatusTable == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->stat(path, buf);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }

    if (0 != ret)
    {
        errno = EINVAL;
    }

    return ret;
}

int statvfs(const char *path, struct statvfs *buf)
{
    ITPDriveStatus * driveStatusTable = NULL;
    ITPDriveStatus * driveStatus;
    int              ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->statvfs(path, buf);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }
    if (0 != ret)
    {
        errno = EINVAL;
    }

    return ret;
}

int _fstat(int file, struct stat *st)
{
    if ((STDOUT_FILENO == file) || (STDERR_FILENO == file) || (STDIN_FILENO == file))
    {
        st->st_mode = S_IFCHR;
        return 0;
    }
    else
    {
        const ITPFSDevice * dev;
        uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
        if (index >= ITP_DEVICE_MAX)
        {
            errno = ENOENT;
            return -1;
        }
        dev = (ITPFSDevice *)itpDeviceTable[index];
        if (NULL != dev)
        {
            /*  Pass call on to device driver.  Return result.        */
            return dev->fstat((((uint32_t)file) & ITP_FILE_MASK), st);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }
}

char* getcwd(char *buf, size_t size)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    char* dir = NULL;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);
    driveStatus = &driveStatusTable[t->volume];

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            dir = dev->getcwd(buf, size);
        }
        else
        {
            errno = ENOENT;
            return NULL;
        }
    }

    if (dir == NULL)
    {
        errno = EINVAL;
    }

    return dir;
}

int rmdir(const char *path)
{
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    int ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->rmdir(path);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }

    if (0 != ret)
    {
        errno = EINVAL;
    }

    return ret;
}

int closedir(DIR *dirp)
{
    const ITPFSDevice * dev;
    uint32_t index = ((uint32_t)dirp->dd_fd & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }

    dev = (ITPFSDevice *)itpDeviceTable[index];
    if (NULL != dev)
    {
        return dev->closedir(dirp);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

DIR* opendir(const char *dirname)
{
    ITPDriveStatus * driveStatusTable = NULL;
    ITPDriveStatus * driveStatus;
    DIR *            dir = NULL;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (dirname[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(dirname[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            dir = dev->opendir(dirname);
        }
        else
        {
            errno = ENOENT;
            return NULL;
        }
    }

    if (dir == NULL)
    {
        errno = EINVAL;
    }

    return dir;
}

struct dirent* readdir(DIR *dirp)
{
    const ITPFSDevice *dev;
    uint32_t index = ((uint32_t)dirp->dd_fd & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return NULL;
    }
    dev = (ITPFSDevice *)itpDeviceTable[index];
    if (NULL != dev)
    {
        return dev->readdir(dirp);
    }
    else
    {
        errno = ENOENT;
        return NULL;
    }
}

void rewinddir(DIR *dirp)
{
    const ITPFSDevice * dev;
    uint32_t index = ((uint32_t)dirp->dd_fd & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return;
    }
    dev = (ITPFSDevice *)itpDeviceTable[index];
    if (NULL != dev)
    {
        dev->rewinddir(dirp);
    }
}

int scandir(const char *dirname,
   struct dirent *** namelist,
   int (*select)(struct dirent *),
   int (*dcomp)(const struct dirent **, const struct dirent **))
{
    register struct dirent *d, *p, **names;
    register size_t         nitems = 0;
    long                    arraysz;
    DIR *                   dirp;
    bool                    successful = false;
    int                     rc         = 0;

    dirp                               = NULL;

    if ((dirp = opendir(dirname)) == NULL)
    {
        return (-1);
    }

    arraysz = 1;
    names   = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
    if (names == NULL)
    {
        goto cleanup;
    }

    nitems = 0;
    while ((d = readdir(dirp)) != NULL)
    {
        if ((select != NULL) && !(*select)(d))
        {
            continue; /* just selected names */
        }
        /*
         * Make a minimum size copy of the data
         */
        p = (struct dirent *)malloc(sizeof(struct dirent));
        if (p == NULL)
        {
            goto cleanup;
        }
        p->d_ino    = d->d_ino;
        p->d_type   = d->d_type;
        p->d_namlen = d->d_namlen;
        strlcpy(p->d_name, d->d_name, sizeof(p->d_name));

        /*
         * Check to make sure the array has space left and
         * realloc the maximum size.
         */
        if (++nitems >= (size_t)arraysz)
        {
            arraysz = nitems;
            names   = (struct dirent **)realloc((char *)names, arraysz * sizeof(struct dirent *));
            if (names == NULL)
            {
                free(p);
                goto cleanup;
            }
        }
        names[nitems - 1] = p;
    }
    successful = true;
cleanup:
    closedir(dirp);
    if (successful)
    {
        if (nitems && (dcomp != NULL))
        {
            qsort(names, nitems, sizeof(struct dirent *), (void *)dcomp);
        }
        *namelist = names;
        rc        = nitems;
    }
    else
    { /* We were unsuccessful, clean up storage and return -1.  */
        if (NULL != names)
        {
            size_t i;
            for (i = 0; i < nitems; i++)
            {
                if (NULL != names[i])
                {
                    free(names[i]);
                }
            }
            free(names);
        }
        rc = -1;
    }

    return (rc);
}

int alphasort(const struct dirent **d1, const struct dirent **d2)
{
    return(strcmp((*d1)->d_name, (*d2)->d_name));
}

mqd_t mq_open(const char *name, int oflag, ...)
{
    if (0 != (oflag & O_CREAT))
    {
        struct mq_attr* attr;
        va_list ap;
        itpQueue* q = pvPortMalloc(sizeof (itpQueue));
        if (NULL == q)
        {
            errno = ENOMEM;
            return -1;
        }

        va_start(ap, oflag);
        va_arg(ap, mode_t);
        attr = va_arg(ap, struct mq_attr*);
        va_end(ap);

        q->xQueue = xQueueCreate(attr->mq_maxmsg, attr->mq_msgsize);
        if (NULL == q->xQueue)
        {
            vPortFree(q);
            errno = ENOMEM;
            return -1;
        }

        q->uxItemSize   = attr->mq_msgsize;
        q->oflag        = oflag | attr->mq_flags;

        return (mqd_t)q;
    }
    else
    {
        errno = EINVAL;
        return -1;
    }
}

int _times(struct tms *tp)
{
    /* Return a clock that ticks at 100Hz.  */
    clock_t timeval = (clock_t) xTaskGetTickCount() / configTICK_RATE_HZ * CLOCKS_PER_SEC;

    if (NULL != tp)
    {
        tp->tms_utime  = timeval;   /* user time */
        tp->tms_stime  = 0;         /* system time */
        tp->tms_cutime = 0;         /* user time, children */
        tp->tms_cstime = 0;         /* system time, children */
    }
    return timeval;
}

int _gettimeofday(struct timeval *tp, void *tzvp)
{
    struct timezone *tzp = tzvp;
    if (NULL != tp)
    {
    #ifdef CFG_RTC_ENABLE
        ioctl(ITP_DEVICE_RTC, ITP_IOCTL_GET_TIME, (void*)tp);
    #else
        unsigned long ms;

        if (!ithCpuInInterrupt())
        {
            ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        }
        else
        {
            ms = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
        }

        tp->tv_usec = (ms % 1000) * 1000;
        tp->tv_sec  = CFG_RTC_DEFAULT_TIMESTAMP + ms / 1000;
    #endif // CFG_RTC_ENABLE
    }

    if (NULL != tzp)
    {
        tzp->tz_minuteswest = _timezone / 60;
        tzp->tz_dsttime     = _daylight;
    }
    return 0;
}

int settimeofday(const struct timeval *tv , const struct timezone *tz)
{
#ifdef CFG_RTC_ENABLE
    struct timeval t;
#endif

    if (tv == NULL)
    {
        errno = EINVAL;
        return -1;
    }
#ifdef CFG_RTC_ENABLE
    (void)memcpy(&t, tv, sizeof (struct timeval));
    if (NULL != tz)
    {
        t.tv_sec += tz->tz_minuteswest * 60;
    }
    else
    {
        t.tv_sec += _timezone;
    }
    ioctl(ITP_DEVICE_RTC, ITP_IOCTL_SET_TIME, (void*)&t);
#endif
    return 0;
}

int usleep(useconds_t useconds)
{
    TickType_t t = (TickType_t)((uint64_t) useconds / (1000 * portTICK_PERIOD_MS));
    useconds_t r = useconds - ((useconds_t)t * (1000 * portTICK_PERIOD_MS));

    if (t > 0)
    {
        vTaskDelay(t);
    }

    if (r > 0)
    {
        ithDelay(r);
    }

    return 0;
}

unsigned sleep(unsigned seconds)
{
    if (seconds > 0)
    {
        (void)usleep((uint64_t)seconds * 1000000);
    }
    else
    {
        taskYIELD();
    }

    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (req->tv_sec > 0)
    {
        sleep(req->tv_sec);
    }

    if (req->tv_nsec > 0)
    {
        (void)usleep(req->tv_nsec / 1000);
    }

    if (NULL != rem)
    {
        rem->tv_sec = rem->tv_nsec = 0;
    }

    return 0;
}

int pthread_attr_init(pthread_attr_t* attr)
{
    attr->detachstate               = PTHREAD_CREATE_JOINABLE;
    attr->schedpolicy               = SCHED_OTHER;
    attr->schedparam.sched_priority = tskIDLE_PRIORITY + 1;
    attr->inheritsched              = PTHREAD_EXPLICIT_SCHED;
    attr->contentionscope           = PTHREAD_SCOPE_SYSTEM;
    attr->stackaddr                 = NULL;
    attr->stacksize                 = configMINIMAL_STACK_SIZE * sizeof(portSTACK_TYPE);
    attr->is_initialized            = 1;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    attr->is_initialized = 0;
    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
    (void)memcpy(param, &attr->schedparam, sizeof (struct sched_param));
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate)
{
    attr->detachstate = detachstate;
    return 0;
}

int pthread_getschedparam(pthread_t pthread, int* policy, struct sched_param* param)
{
    param->sched_priority = uxTaskPriorityGet((TaskHandle_t) pthread);
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr,
                               const struct sched_param *param)
{
    attr->schedparam.sched_priority = param->sched_priority;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if (stacksize < configMINIMAL_STACK_SIZE * sizeof(portSTACK_TYPE))
    {
        ITP_LOG_DBG("pthread_attr_setstacksize(%p,%d) invalid\n", attr, stacksize);
        return EINVAL;
    }

    if (stacksize > USHRT_MAX * sizeof(portSTACK_TYPE))
    {
        ITP_LOG_ERR("pthread_attr_setstacksize(%p,%d) invalid\n", attr, stacksize);
        return EINVAL;
    }

    attr->stacksize = stacksize;
    return 0;
}

int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope)
{
    attr->contentionscope = contentionscope;
    return 0;
}

static void vTaskCode(void* pvParameters)
{
    itpThread* t = (itpThread*) pvParameters;
    TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();

    if (t->detachstate != PTHREAD_CREATE_JOINABLE)
    {
        t->xHandle = xHandle;
        vTaskSetApplicationTaskTag(xHandle, (TaskHookFunction_t) t);
    }

#ifdef CFG_FS_FAT
    // init fat on this task
    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
#endif

    t->value_ptr = t->start_routine(t->arg);

#ifdef CFG_FS_FAT
    // exit fat on this task
    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_DISABLE, NULL);
#endif

    if (t->detachstate == PTHREAD_CREATE_JOINABLE)
    {
        ITP_LOG_DBG("pthread 0x%" PRIx32 " exit, wait to be joined\n", t);
        xSemaphoreGive(t->xSemaphore);
    }
    else
    {
        int i;

        for (i = 0; i < PTHREAD_KEYS_MAX; i++)
        {
            itpThreadKey * k = &t->keys[i];
            if ((NULL != k->dtor) && k->value)
            {
                k->dtor(k->value);
            }
        }

#if 0
        vTaskPrioritySet(xTaskGetIdleTaskHandle(), uxTaskPriorityGet(xHandle));
#endif
        vPortFree(t);
        vTaskDelete(xHandle);
    }
    while (true)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

int itpPthreadCreate(pthread_t* pthread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg, const char* name)
{
    portBASE_TYPE ret;
    TaskHandle_t handle;
    pthread_attr_t attr_default;
    unsigned short depth;
    itpThread* t = pvPortMalloc(sizeof(itpThread));
    if (t == NULL)
    {
        ITP_LOG_ERR("itpPthreadCreate(%s) nomem\n", name);
        return ENOMEM;
    }

    (void)memset(t, 0, sizeof (itpThread));

    if (NULL == attr)
    {
        pthread_attr_init(&attr_default);
        attr = &attr_default;
    }

    t->start_routine    = start_routine;
    t->arg              = arg;
    t->detachstate      = attr->detachstate;

    if (t->detachstate == PTHREAD_CREATE_JOINABLE)
    {
        t->xSemaphore = xSemaphoreCreateCounting(1, 0);
        if (t->xSemaphore == NULL)
        {
            vPortFree(t);
            ITP_LOG_ERR("itpPthreadCreate(%s) fail\n", name);
            return ENOMEM;
        }
    }
    else
    {
        t->xSemaphore = NULL;
    }

    depth =
        (attr->stacksize / sizeof(portSTACK_TYPE) < USHRT_MAX) ? attr->stacksize / sizeof(portSTACK_TYPE) : USHRT_MAX;
    ret = xTaskCreate(vTaskCode, name, depth, t, attr->schedparam.sched_priority, &handle);
    if (ret != pdPASS)
    {
        if (NULL != t->xSemaphore)
        {
            vSemaphoreDelete(t->xSemaphore);
        }

        vPortFree(t);
        ITP_LOG_ERR("itpPthreadCreate(%s) fail\n", name);
        return EAGAIN;
    }

    if (t->detachstate == PTHREAD_CREATE_JOINABLE)
    {
        portENTER_CRITICAL();
        t->xHandle = handle;
        vTaskSetApplicationTaskTag(handle, (TaskHookFunction_t) t);
        portEXIT_CRITICAL();
    }
    *pthread = (pthread_t) handle;

    return 0;
}

int pthread_cancel(pthread_t pthread)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag((TaskHandle_t) pthread);
    int i;
    TaskHandle_t xHandle;

    for (i = 0; i < PTHREAD_KEYS_MAX; i++)
    {
        itpThreadKey * k = &t->keys[i];
        if ((NULL != k->dtor) && k->value)
        {
            k->dtor(k->value);
        }
    }

#if 0
    xHandle = xTaskGetIdleTaskHandle();
    vTaskPrioritySet(xHandle, tskIDLE_PRIORITY + 1);
#endif

    xHandle = t->xHandle;
    vPortFree(t);
    vTaskDelete(xHandle);

    return 0;
}

void pthread_exit(void* value_ptr)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    t->value_ptr = value_ptr;

#ifdef CFG_FS_FAT
    // exit fat on this task
    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_DISABLE, NULL);
#endif

    if (t->detachstate == PTHREAD_CREATE_JOINABLE)
    {
        ITP_LOG_DBG("pthread 0x%" PRIx32 " exit, wait to be joined\n", t);
        xSemaphoreGive(t->xSemaphore);
    }
    else
    {
        int i;
        TaskHandle_t xHandle;

        for (i = 0; i < PTHREAD_KEYS_MAX; i++)
        {
            itpThreadKey * k = &t->keys[i];
            if ((NULL != k->dtor) && k->value)
            {
                k->dtor(k->value);
            }
        }

#if 0
        xHandle = xTaskGetIdleTaskHandle();
        vTaskPrioritySet(xHandle, tskIDLE_PRIORITY + 1);
#endif

        xHandle = t->xHandle;
        vPortFree(t);
        vTaskDelete(xHandle);
    }
    while (true)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

int pthread_join(pthread_t pthread, void **value_ptr)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag((TaskHandle_t) pthread);
    int i;
    TaskHandle_t xHandle;

    ITP_LOG_DBG("pthread join %p\n", t);

    if (xSemaphoreTake(t->xSemaphore, portMAX_DELAY) == pdFALSE)
    {
        ITP_LOG_ERR("pthread_exit(%p) fail\n", value_ptr);
        return EINVAL;
    }

    for (i = 0; i < PTHREAD_KEYS_MAX; i++)
    {
        itpThreadKey * k = &t->keys[i];
        if ((NULL != k->dtor) && k->value)
        {
            k->dtor(k->value);
        }
    }

    if (value_ptr)
    {
        *value_ptr = t->value_ptr;
    }

    vSemaphoreDelete(t->xSemaphore);

#if 0
    xHandle = xTaskGetIdleTaskHandle();
    vTaskPrioritySet(xHandle, tskIDLE_PRIORITY + 1);
#endif

    xHandle = t->xHandle;
    vPortFree(t);
    vTaskDelete(xHandle);

    return 0;
}

int pthread_detach(pthread_t pthread)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag((TaskHandle_t) pthread);
    t->detachstate = PTHREAD_CREATE_DETACHED;
    if (t->xSemaphore)
    {
        vSemaphoreDelete(t->xSemaphore);
        t->xSemaphore = NULL;
    }
    return 0;
}

int pthread_setschedparam(pthread_t pthread, int policy, struct sched_param* param)
{
    vTaskPrioritySet((TaskHandle_t) pthread, param->sched_priority);
    return 0;
}

pthread_t pthread_self(void)
{
    return (pthread_t) xTaskGetCurrentTaskHandle();
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1 == t2;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    if (once_control->init_executed == 0)
    {
        once_control->init_executed = 1;
        init_routine();
    }
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    attr->type = 0;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind)
{
    attr->type = kind;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr)
{
    pthread_mutexattr_t a;
    itpMutex* m;

    if (mutex == NULL)
    {
        ITP_LOG_ERR("pthread_mutex_init(%p, %p) invalid\n", mutex, attr);
        return EINVAL;
    }

    if (attr == NULL)
    {
        pthread_mutexattr_init(&a);
        attr = &a;
    }

    m = (itpMutex*) pvPortMalloc(sizeof(itpMutex));
    if (m == NULL)
    {
        ITP_LOG_ERR("pthread_mutex_init(%p, %p) nomem\n", mutex, attr);
        return ENOMEM;
    }

    if (attr->type == PTHREAD_MUTEX_RECURSIVE)
    {
        m->xMutex = xSemaphoreCreateRecursiveMutex();
    }
    else
    {
        m->xMutex = xSemaphoreCreateMutex();
    }

    if (m->xMutex == NULL)
    {
        vPortFree(m);
        ITP_LOG_ERR("pthread_mutex_init(%p, %p) fail\n", mutex, attr);
        return ENOMEM;
    }

    m->type = attr->type;
    *mutex = (pthread_mutex_t) m;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    if ((mutex == NULL) || (*mutex == (pthread_mutex_t)NULL))
    {
        return EINVAL;
    }

    if ((*mutex == PTHREAD_MUTEX_INITIALIZER) || (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP))
    {
        *mutex = 0;
    }
    else
    {
        itpMutex* m = (itpMutex*)*mutex;

        vSemaphoreDelete(m->xMutex);
        vPortFree(m);
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    itpMutex* m;
    portBASE_TYPE ret;

    if ((mutex == NULL) || (*mutex == (pthread_mutex_t)NULL))
    {
        ITP_LOG_ERR("pthread_mutex_lock(%p) invalid\n", mutex);
        return EINVAL;
    }

    portENTER_CRITICAL();

    if ((*mutex == PTHREAD_MUTEX_INITIALIZER) || (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP))
    {
        int ret2 = 0;
        pthread_mutexattr_t attr;

        attr.type = (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) ? PTHREAD_MUTEX_RECURSIVE : 0;

        // A static analysis warning indicates that the field "attr.is_initialized" 
        // may be uninitialized at the time of the call to "pthread_mutex_init". 
        // However, this warning is a false positive. A review of the implementation 
        // confirms that "pthread_mutex_init" does not access or rely on the 
        // "attr.is_initialized" field.
        // coverity[UNINIT: FALSE]
        ret2 = pthread_mutex_init(mutex, &attr);
        if (0 != ret2)
        {
            portEXIT_CRITICAL();
            return ret2;
        }
    }
    m = (itpMutex*)*mutex;

    portEXIT_CRITICAL();

    if (m->type == PTHREAD_MUTEX_RECURSIVE)
    {
        ret = xSemaphoreTakeRecursive(m->xMutex, portMAX_DELAY);
    }
    else
    {
        ret = xSemaphoreTake(m->xMutex, portMAX_DELAY);
    }

    if (pdTRUE == ret)
    {
        return 0;
    }
    else
    {
        ITP_LOG_ERR("pthread_mutex_lock(%p) timeout\n", mutex);
        return EBUSY;
    }
}

int itpMutexLockTimeout(pthread_mutex_t* mutex, unsigned long ms)
{
    itpMutex* m;
    portBASE_TYPE ret;

    if ((mutex == NULL) || (*mutex == (pthread_mutex_t)NULL))
    {
        ITP_LOG_ERR("itpMutexLockTimeout(%p) invalid\n", mutex);
        return EINVAL;
    }

    portENTER_CRITICAL();

    if ((*mutex == PTHREAD_MUTEX_INITIALIZER) || (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP))
    {
        int ret2 = 0;
        pthread_mutexattr_t attr;

        attr.type = (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) ? PTHREAD_MUTEX_RECURSIVE : 0;

        // A static analysis warning indicates that the field "attr.is_initialized" 
        // may be uninitialized at the time of the call to "pthread_mutex_init". 
        // However, this warning is a false positive. A review of the implementation 
        // confirms that "pthread_mutex_init" does not access or rely on the 
        // "attr.is_initialized" field.
        // coverity[UNINIT: FALSE]
        ret2 = pthread_mutex_init(mutex, &attr);
        if (0 != ret2)
        {
            portEXIT_CRITICAL();
            return ret2;
        }
    }
    m = (itpMutex*)*mutex;

    portEXIT_CRITICAL();

    if (m->type == PTHREAD_MUTEX_RECURSIVE)
    {
        ret = xSemaphoreTakeRecursive(m->xMutex, ms * portTICK_PERIOD_MS);
    }
    else
    {
        ret = xSemaphoreTake(m->xMutex, ms * portTICK_PERIOD_MS);
    }

    if (pdTRUE == ret)
    {
        return 0;
    }
    else
    {
        ITP_LOG_ERR("itpMutexLockTimeout(%p) timeout\n", mutex);
        return EBUSY;
    }
}

int pthread_mutex_trylock(pthread_mutex_t * mutex)
{
    itpMutex* m;
    portBASE_TYPE ret;

    if ((mutex == NULL) || (*mutex == (pthread_mutex_t)NULL))
    {
        ITP_LOG_ERR("pthread_mutex_trylock(%p) invalid\n", mutex);
        return EINVAL;
    }

    portENTER_CRITICAL();

    if ((*mutex == PTHREAD_MUTEX_INITIALIZER) || (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP))
    {
        int ret2;
        pthread_mutexattr_t attr;

        attr.type = (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) ? PTHREAD_MUTEX_RECURSIVE : 0;

        // A static analysis warning indicates that the field "attr.is_initialized" 
        // may be uninitialized at the time of the call to "pthread_mutex_init". 
        // However, this warning is a false positive. A review of the implementation 
        // confirms that "pthread_mutex_init" does not access or rely on the 
        // "attr.is_initialized" field.
        // coverity[UNINIT: FALSE]
        ret2 = pthread_mutex_init(mutex, &attr);
        if (0 != ret2)
        {
            portEXIT_CRITICAL();
            return ret2;
        }
    }
    m = (itpMutex*)*mutex;

    portEXIT_CRITICAL();

    if (m->type == PTHREAD_MUTEX_RECURSIVE)
    {
        ret = xSemaphoreTakeRecursive(m->xMutex, 0);
    }
    else
    {
        ret = xSemaphoreTake(m->xMutex, 0);
    }

    if (pdTRUE == ret)
    {
        return 0;
    }
    else
    {
        ITP_LOG_ERR("pthread_mutex_trylock(%p) timeout\n", mutex);
        return EBUSY;
    }
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    if ((mutex == NULL) || (*mutex == (pthread_mutex_t)NULL))
    {
        ITP_LOG_ERR("pthread_mutex_unlock(%p) invalid\n", mutex);
        return EINVAL;
    }

    if ((*mutex != PTHREAD_MUTEX_INITIALIZER) && (*mutex != PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP))
    {
        itpMutex * m = (itpMutex *)*mutex;
        if (m->type == PTHREAD_MUTEX_RECURSIVE)
        {
            xSemaphoreGiveRecursive(m->xMutex);
        }
        else
        {
            xSemaphoreGive(m->xMutex);
        }
    }
    return 0;
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    itpCondition* c;
    if (cond == NULL)
    {
        ITP_LOG_ERR("pthread_cond_init(%p, %p) invalid\n", cond, attr);
        return EINVAL;
    }

    c = (itpCondition*) pvPortMalloc(sizeof(itpCondition));
    if (c == NULL)
    {
        ITP_LOG_ERR("pthread_cond_init(%p, %p) nomem\n", cond, attr);
        return ENOMEM;
    }

    c->xSemaphore = xSemaphoreCreateCounting(32, 0);
    if (c->xSemaphore == NULL)
    {
        vPortFree(c);
        ITP_LOG_ERR("pthread_cond_init(%p, %p) fail\n", cond, attr);
        return ENOMEM;
    }
    c->count = 0;

    *cond = (pthread_cond_t) c;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    if ((cond == NULL) || (*cond == (pthread_cond_t)NULL))
    {
        ITP_LOG_ERR("pthread_cond_destroy(%p) invalid\n", cond);
        return EINVAL;
    }

    if (*cond == PTHREAD_COND_INITIALIZER)
    {
        *cond = (pthread_cond_t)NULL;
    }
    else
    {
        itpCondition* c = (itpCondition*) *cond;
        vSemaphoreDelete(c->xSemaphore);
        vPortFree(c);
    }
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    itpCondition* c;

    if ((cond == NULL) || (*cond == (pthread_cond_t)NULL))
    {
        ITP_LOG_ERR("pthread_cond_wait(%p, %p) invalid\n", cond, mutex);
        return EINVAL;
    }

    portENTER_CRITICAL();

    if (*cond == PTHREAD_COND_INITIALIZER)
    {
        int ret = pthread_cond_init(cond, NULL);
        if (0 != ret)
        {
            portEXIT_CRITICAL();
            return ret;
        }
    }

    c = (itpCondition*) *cond;
    c->count++;

    portEXIT_CRITICAL();

    if (pthread_mutex_unlock(mutex))
    {
        return EINVAL;
    }

    if (xSemaphoreTake(c->xSemaphore, portMAX_DELAY) == pdFALSE)
    {
        portENTER_CRITICAL();
        c->count--;
        portEXIT_CRITICAL();

        ITP_LOG_ERR("pthread_cond_wait(%p, %p) fail\n", cond, mutex);
        return EINVAL;
    }

    if (pthread_mutex_lock(mutex))
    {
        return EINVAL;
    }

    return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
    itpCondition* c;
    unsigned long ms;
    struct timeval tv = {0};
    uint64_t start, finish;

    if ((cond == NULL) || (*cond == (pthread_cond_t)NULL) || (abstime == NULL))
    {
        ITP_LOG_ERR("pthread_cond_timedwait(%p, %p, %p) invalid\n", cond, mutex, abstime);
        return EINVAL;
    }

#ifdef CFG_RTC_ENABLE
    ioctl(ITP_DEVICE_RTC, ITP_IOCTL_GET_TIME, (void*)&tv);
    start = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

#else
    ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    tv.tv_usec = (ms % 1000) * 1000;
    tv.tv_sec  = CFG_RTC_DEFAULT_TIMESTAMP + ms / 1000;
    start = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

#endif // CFG_RTC_ENABLE

    finish = (uint64_t)abstime->tv_sec * 1000 + abstime->tv_nsec / 1000000;
    ms = (long)(finish - start);

    portENTER_CRITICAL();

    if (*cond == PTHREAD_COND_INITIALIZER)
    {
        int ret = pthread_cond_init(cond, NULL);
        if (0 != ret)
        {
            portEXIT_CRITICAL();
            return ret;
        }
    }

    c = (itpCondition*) *cond;
    c->count++;

    portEXIT_CRITICAL();

    if (pthread_mutex_unlock(mutex))
    {
        return EINVAL;
    }

    if (xSemaphoreTake(c->xSemaphore, ms * portTICK_PERIOD_MS) == pdFALSE)
    {
        portENTER_CRITICAL();
        c->count--;
        portEXIT_CRITICAL();

        ITP_LOG_DBG("xSemaphoreTake(%p, %d) timeout\n", c->xSemaphore, ms * portTICK_PERIOD_MS);
        return ETIMEDOUT;
    }

    if (pthread_mutex_lock(mutex))
    {
        return EINVAL;
    }

    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    itpCondition* c;

    if ((cond == NULL) || (*cond == (pthread_cond_t)NULL))
    {
        ITP_LOG_ERR("pthread_cond_signal(%p) invalid\n", cond);
        return EINVAL;
    }

    portENTER_CRITICAL();

    if (*cond == PTHREAD_COND_INITIALIZER)
    {
        int ret = pthread_cond_init(cond, NULL);
        if (0 != ret)
        {
            portEXIT_CRITICAL();
            return ret;
        }
    }
    c = (itpCondition*) *cond;

    if (c->count == 0)
    {
        portEXIT_CRITICAL();
        return 0;
    }
    c->count--;

    portEXIT_CRITICAL();

    if (xSemaphoreGive(c->xSemaphore) == pdFALSE)
    {
        portENTER_CRITICAL();
        c->count++;
        portEXIT_CRITICAL();

        ITP_LOG_ERR("pthread_cond_signal(%p) timeout\n", cond);
        return EINVAL;
    }
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    itpCondition* c;

    if ((cond == NULL) || (*cond == (pthread_cond_t)NULL))
    {
        ITP_LOG_ERR("pthread_cond_broadcast(%p) invalid\n", cond);
        return EINVAL;
    }

    portENTER_CRITICAL();

    if (*cond == PTHREAD_COND_INITIALIZER)
    {
        int ret = pthread_cond_init(cond, NULL);
        if (0 != ret)
        {
            portEXIT_CRITICAL();
            return ret;
        }
    }
    c = (itpCondition*) *cond;

    portEXIT_CRITICAL();

    for (;;)
    {
        portENTER_CRITICAL();

        if (c->count == 0)
        {
            portEXIT_CRITICAL();
            return 0;
        }
        c->count--;

        portEXIT_CRITICAL();

        if (xSemaphoreGive(c->xSemaphore) == pdFALSE)
        {
            portENTER_CRITICAL();
            c->count++;
            portEXIT_CRITICAL();

            ITP_LOG_ERR("pthread_cond_broadcast(%p) fail\n", cond);
            return EINVAL;
        }
    }
    return 0;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)( void * ))
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    int i;

    if (NULL == key)
    {
        ITP_LOG_ERR("pthread_key_create(%p, %p) invalid\n", key, destructor);
        return EINVAL;
    }

    for (i = 0; i < PTHREAD_KEYS_MAX; i++)
    {
        itpThreadKey* k = &t->keys[i];

        if (!k->used)
        {
            k->used     = true;
            k->dtor     = destructor;
            k->value    = NULL;
            *key        = (pthread_key_t) i;
            return 0;
        }
    }
    ITP_LOG_ERR("pthread_key_create(%p, %p) fail: %d\n", key, destructor, i);
    return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    itpThreadKey* k = &t->keys[(unsigned int)key];

    if (!k->used)
    {
        ITP_LOG_ERR("pthread_key_delete(0x%" PRIx32 ") invalid\n", key);
        return EINVAL;
    }

    k->used     = false;
    k->value    = NULL;
    return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    itpThreadKey* k = &t->keys[(unsigned int)key];

    if ((unsigned int)key >= PTHREAD_KEYS_MAX)
    {
        ITP_LOG_ERR("pthread_setspecific(0x%" PRIx32 ", %p) invalid\n", key, value);
        return EINVAL;
    }

    k->used  = true;
    k->value = (void*) value;
    return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    itpThreadKey* k = &t->keys[(unsigned int)key];

    if (((unsigned int)key >= PTHREAD_KEYS_MAX) || (!k->used))
    {
        ITP_LOG_ERR("pthread_getspecific(0x%" PRIx32 ") invalid\n", key);
        return NULL;
    }
    return k->value;
}

int sched_get_priority_max(int policy)
{
    return configTIMER_TASK_PRIORITY - 1;
}

int sched_get_priority_min(int  policy)
{
    return tskIDLE_PRIORITY + 1;
}

int sched_yield(void)
{
    taskYIELD();
    return 0;
}

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (NULL == sem)
    {
        ITP_LOG_ERR("sem_init(%p, %d, %d) invalid\n", sem, pshared, value);
        return -1;
    }

    sem->__sem_lock = (void *)xSemaphoreCreateCounting(SEM_VALUE_MAX, value);
    if (sem->__sem_lock)
    {
        return 0;
    }
    else
    {
        ITP_LOG_ERR("sem_init(%p, %d, %d) fail\n", sem, pshared, value);
        return -1;
    }
}

int sem_destroy(sem_t *sem)
{
    if (NULL == sem)
    {
        ITP_LOG_ERR("sem_destroy(%p) invalid\n", sem);
        return -1;
    }

    vSemaphoreDelete(sem->__sem_lock);
    return 0;
}

int sem_wait(sem_t *sem)
{
    if (NULL == sem)
    {
        ITP_LOG_ERR("sem_wait(%p) invalid\n", sem);
        return -1;
    }

    if (pdTRUE == xSemaphoreTake(sem->__sem_lock, portMAX_DELAY))
    {
        return 0;
    }
    else
    {
        ITP_LOG_ERR("sem_wait(%p) fail\n", sem);
        return -1;
    }
}

int sem_trywait(sem_t *sem)
{
    if (NULL == sem)
    {
        ITP_LOG_ERR("sem_trywait(%p) invalid\n", sem);
        return -1;
    }

    if (pdTRUE == xSemaphoreTake(sem->__sem_lock, 0))
    {
        return 0;
    }
    else
    {
#if 0
        ITP_LOG_DBG("sem_trywait(%p) fail\n", sem);
#endif
        return -1;
    }
}

int sem_post(sem_t *sem)
{
    if (NULL == sem)
    {
        ITP_LOG_ERR("sem_post(%p) invalid\n", sem);
        return -1;
    }

    if (!ithCpuInInterrupt())
    {
        if (pdTRUE == xSemaphoreGive(sem->__sem_lock))
        {
            return 0;
        }
        else
        {
            ITP_LOG_DBG("sem_post(%p) fail\n", sem);
            return -1;
        }
    }
    else
    {
        itpSemPostFromISR(sem);
        return 0;
    }
}

int sem_getvalue(sem_t *sem, int *sval)
{
    if (NULL == sem)
    {
        ITP_LOG_ERR("sem_getvalue(%p, %p) invalid\n", sem, sval);
        return -1;
    }

    if (!ithCpuInInterrupt())
    {
        *sval = uxQueueMessagesWaiting(sem->__sem_lock);
    }
    else
    {
        *sval = uxQueueMessagesWaitingFromISR(sem->__sem_lock);
    }

    return 0;
}

int timer_create(clockid_t clock_id, struct sigevent *evp, timer_t *timerid)
{
    itpTimer* t = (itpTimer*) pvPortMalloc(sizeof (itpTimer));
    if (t == NULL)
    {
        ITP_LOG_ERR("timer_create(%" PRIu32 ", %p, %p) nomem\n", clock_id, evp, timerid);
        return -1;
    }

    t->pxTimer  = NULL;
    t->routine  = NULL;
    t->arg      = 0;
    *timerid    = (timer_t) t;
    return 0;
}

#if configUSE_TIMERS == 1

int timer_delete(timer_t timerid)
{
    itpTimer* t = (itpTimer*)timerid;

    if (t->pxTimer)
    {
        if (xTimerDelete(t->pxTimer, 0) == pdFAIL)
        {
            ITP_LOG_ERR("timer_delete(%" PRIu32 ") fail\n", timerid);
            return -1;
        }
    }

    vPortFree(t);
    return 0;
}

int timer_connect(timer_t timerid, VOIDFUNCPTR routine, int arg)
{
    ((itpTimer*)timerid)->routine  = routine;
    ((itpTimer*)timerid)->arg      = arg;
    return 0;
}

static void vTimerCallback(TimerHandle_t pxTimer)
{
    itpTimer* t = (itpTimer*) pvTimerGetTimerID(pxTimer);
    t->routine((timer_t)t, t->arg);
}

int timer_settime(timer_t timerid, int flags, const struct itimerspec *value, struct itimerspec *ovalue)
{
    itpTimer* t = (itpTimer*)timerid;
    TickType_t tick;
    unsigned portBASE_TYPE uxAutoReload;

    if (value->it_value.tv_sec == 0 && value->it_value.tv_nsec == 0)
    {
        if (t->pxTimer)
        {
            xTimerStop(t->pxTimer, 0);
        }

        return 0;
    }
    else if (value->it_interval.tv_sec > 0 || value->it_interval.tv_nsec > 0)
    {
        tick = value->it_interval.tv_sec * configTICK_RATE_HZ;
        tick += (uint64_t)value->it_interval.tv_nsec * configTICK_RATE_HZ / 1000000000;
        uxAutoReload = pdTRUE;
    }
    else
    {
        tick = value->it_value.tv_sec * configTICK_RATE_HZ;
        tick += (uint64_t)value->it_value.tv_nsec * configTICK_RATE_HZ / 1000000000;
        uxAutoReload = pdFALSE;
    }

    if (t->pxTimer && (uxAutoReload == pdFALSE))
    {
        if (xTimerChangePeriod(t->pxTimer, tick, 0) == pdFAIL)
        {
            ITP_LOG_WARN("timer_settime(%" PRIu32 ", %d, %p, %p) fail\n", timerid, flags, value, ovalue);
            xTimerDelete(t->pxTimer, 0);
        }
        else
        {
            return 0;
        }
    }
    else if(t->pxTimer)
    {
        xTimerDelete(t->pxTimer, 0);
    }

    t->pxTimer = xTimerCreate("Timer", tick, uxAutoReload, (void*)t, vTimerCallback);
    if (NULL == t->pxTimer)
    {
        ITP_LOG_ERR("timer_settime(%" PRIu32 ", %d, %p, %p) fail\n", timerid, flags, value, ovalue);
        return -1;
    }

    if (xTimerStart(t->pxTimer, 0) == pdFAIL)
    {
        ITP_LOG_ERR("timer_settime(%" PRIu32 ", %d, %p, %p) fail\n", timerid, flags, value, ovalue);
        return -1;
    }
    return 0;
}

#endif // configUSE_TIMERS == 1

int mq_close(mqd_t msgid)
{
    itpQueue* q = (itpQueue*) msgid;
    vQueueDelete(q->xQueue);
    vPortFree(q);
    return 0;
}

int mq_send(mqd_t msgid, const char *msg, size_t msg_len, unsigned int msg_prio)
{
    itpQueue* q = (itpQueue*) msgid;
    TickType_t xTicksToWait = (q->oflag & O_NONBLOCK) ? 0 : portMAX_DELAY;

    if (xQueueSend(q->xQueue, (const void*) msg, xTicksToWait) != pdTRUE)
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int mq_timedsend(mqd_t msgid, const char *msg, size_t msg_len, unsigned msg_prio, const struct timespec *abs_timeout)
{
    itpQueue* q = (itpQueue*) msgid;
    TickType_t xTicksToWait;

    if (0 != (q->oflag & O_NONBLOCK))
    {
        xTicksToWait = 0;
    }
    else
    {
        unsigned long ms;
        struct timeval tv = {0};
        uint64_t start, finish;

    #ifdef CFG_RTC_ENABLE
        ioctl(ITP_DEVICE_RTC, ITP_IOCTL_GET_TIME, (void*)&tv);
        start = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    #else
        ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        tv.tv_usec = (ms % 1000) * 1000;
        tv.tv_sec  = CFG_RTC_DEFAULT_TIMESTAMP + ms / 1000;
        start = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    #endif // CFG_RTC_ENABLE

        finish = (uint64_t)abs_timeout->tv_sec * 1000 + abs_timeout->tv_nsec / 1000000;
        ms = (long)(finish - start);
        xTicksToWait = ms * portTICK_PERIOD_MS;
    }

    if (xQueueSend(q->xQueue, (const void*) msg, xTicksToWait) != pdTRUE)
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

ssize_t mq_receive(mqd_t msgid, char *msg, size_t msg_len, unsigned int *msg_prio)
{
    itpQueue* q = (itpQueue*) msgid;
    TickType_t xTicksToWait = (q->oflag & O_NONBLOCK) ? 0 : portMAX_DELAY;

    if (xQueueReceive(q->xQueue, msg, xTicksToWait) != pdTRUE)
    {
        errno = EINVAL;
        return -1;
    }
    return q->uxItemSize;
}

ssize_t mq_timedreceive(mqd_t msgid, char *msg, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout)
{
    itpQueue* q = (itpQueue*) msgid;
    TickType_t xTicksToWait;

    if (0 != (q->oflag & O_NONBLOCK))
    {
        xTicksToWait = 0;
    }
    else
    {
        unsigned long ms;
        struct timeval tv = {0};
        uint64_t start, finish;

    #ifdef CFG_RTC_ENABLE
        ioctl(ITP_DEVICE_RTC, ITP_IOCTL_GET_TIME, (void*)&tv);
        start = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    #else
        ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        tv.tv_usec = (ms % 1000) * 1000;
        tv.tv_sec  = CFG_RTC_DEFAULT_TIMESTAMP + ms / 1000;
        start = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    #endif // CFG_RTC_ENABLE

        finish = (uint64_t)abs_timeout->tv_sec * 1000 + abs_timeout->tv_nsec / 1000000;
        ms = (long)(finish - start);
        xTicksToWait = ms * portTICK_PERIOD_MS;
    }

    if (xQueueReceive(q->xQueue, msg, xTicksToWait) != pdTRUE)
    {
        errno = EINVAL;
        return -1;
    }
    return q->uxItemSize;
}

int mq_getattr(mqd_t msgid, struct mq_attr *mqstat)
{
    itpQueue* q = (itpQueue*) msgid;

    mqstat->mq_flags = q->oflag;
    mqstat->mq_msgsize = q->uxItemSize;
    mqstat->mq_curmsgs = uxQueueMessagesWaiting(q->xQueue);
    mqstat->mq_maxmsg = uxQueueSpacesAvailable(q->xQueue) + mqstat->mq_curmsgs;

    return 0;
}

int _isatty(int file)
{
    return (file == STDIN_FILENO  ||
            file == STDERR_FILENO ||
            file == STDOUT_FILENO) ? 1 : 0;
}

int lstat(const char *path, struct stat *buf)
{
    return _stat(path, buf);
}

int dup(int fildes)
{
    return fildes;
}

int dup2(int fildes, int fildes2)
{
    return fildes2;
}

int _link(char *old, char *new)
{
    errno = ENOENT;
    return -1;
}

mode_t umask(mode_t mask)
{
    return 0;
}

long sysconf(int __name)
{
    switch (__name)
    {
    case _SC_PAGE_SIZE:
        return 4096U;

    default:
        return -1;
    }
}

long pathconf(const char *__path, int __name)
{
    switch (__name)
    {
    case _PC_NAME_MAX:
        return PATH_MAX;

    default:
        return -1;
    }
}

int getpagesize()
{
    return sysconf(_SC_PAGESIZE);
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    struct timeval now;
    int            ret = gettimeofday(&now, NULL);

    if (ret != 0)
    {
        return ret;
    }

    tp->tv_sec  = now.tv_sec;
    tp->tv_nsec = now.tv_usec * 1000;
    return 0;
}

char* basename(const char *path)
{
    char * p;
    if ((path == NULL) || (*path == '\0'))
    {
        return ".";
    }
    p = (char *)(path + strlen(path) - 1);
    while (*p == '/')
    {
        if (p == path)
        {
            return (char *)path;
        }
        *p-- = '\0';
    }
    while ((p >= path) && (*p != '/'))
    {
        p--;
    }
    return p + 1;
}

char *dirname(char *path)
{
    char * p;
    if ((path == NULL) || (*path == '\0'))
    {
        return ".";
    }
    p = path + strlen(path) - 1;
    while (*p == '/')
    {
        if (p == path)
        {
            return path;
        }
        *p-- = '\0';
    }
    while ((p >= path) && (*p != '/'))
    {
        p--;
    }
    return p < path ? "." : p == path ? "/" : (*p = '\0', path);
}

int fsync(int fd)
{
    if ((STDOUT_FILENO == fd) || (STDERR_FILENO == fd) || (STDIN_FILENO == fd))
    {
        errno = EINVAL;
        return  -1;
    }
    else
    {
        const ITPFSDevice* dev;
        uint32_t index = ((uint32_t)fd & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
        if (index >= ITP_DEVICE_MAX)
        {
            errno = ENOENT;
            return -1;
        }
        dev = (ITPFSDevice*) itpDeviceTable[index];

        if (NULL != dev)
        {
            struct stat st = {0};
            int ret = dev->fstat((((uint32_t)fd) & ITP_FILE_MASK), &st);
            if (ret == 0)
            {
                ITPDriveStatus* driveStatusTable = NULL;
                ITPDriveStatus* driveStatus;

                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);
                driveStatus = &driveStatusTable[st.st_ino];

                if (driveStatus->avail && driveStatus->disk == ITP_DISK_NOR)
                {
                    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
                }

#if (CFG_LFS_CACHE_SIZE > 0)
                if (driveStatus->avail &&
                        (driveStatus->disk == ITP_DISK_NOR ||
                        driveStatus->disk == ITP_DISK_NAND ||
                        driveStatus->disk == ITP_DISK_SD0 ||
                        driveStatus->disk == ITP_DISK_SD1))
                {
                    ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
                }
#endif

                return 0;
            }
        }
        errno = EINVAL;
        return  -1;
    }
}

int fdatasync(int fd)
{
    return fsync(fd);
}

int waitpid(pid_t pid, int *status, int options)
{
    assert(0);
    return -1;
}

int seteuid(uid_t uid)
{
    return 0;
}

int chroot(const char *path)
{
    return 0;
}

int setgid(gid_t gid)
{
    return 0;
}

int setegid(gid_t egid)
{
    return 0;
}

uid_t getuid(void)
{
    return 0;
}

gid_t getgid(void)
{
    return 0;
}

uid_t geteuid(void)
{
    return 0;
}

gid_t getegid(void)
{
    return 0;
}

int sigaction(int sig, const struct sigaction * act, struct sigaction * oact)
{
    return 0;
}

int creat(const char *path, mode_t mode)
{
    assert(0);
    return -1;
}

int execv(const char *__path, char * const __argv[])
{
    assert(0);
    return -1;
}

int utime(const char *__file, const struct utimbuf *__times)
{
    assert(0);
    return -1;
}

pid_t getppid(void)
{
    assert(0);
    return -1;
}

int _execve(const char *path, char *const argv[], char *const envp[])
{
    assert(0);
    return -1;
}

pid_t _fork(void)
{
    return 0;
}

pid_t _wait(int *status)
{
    assert(0);
    return -1;
}

int execlp(const char *path, const char *argv0, ...)
{
    assert(0);
    return -1;
}

iconv_t iconv_open(const char *__tocode, const char *__fromcode)
{
    assert(0);
    return NULL;
}

size_t iconv(iconv_t __cd, char **__inbuf, size_t *__inbytesleft, char **__outbuf, size_t *__outbytesleft)
{
    assert(0);
    return 0;
}

int iconv_close(iconv_t __cd)
{
    assert(0);
    return -1;
}

int uname(struct utsname *name)
{
    (void)memset(name, 0, sizeof (struct utsname));
    strlcpy(name->sysname, CFG_SYSTEM_NAME, sizeof(name->sysname));
    strlcpy(name->version, CFG_VERSION_MAJOR_STR "." CFG_VERSION_MINOR_STR "." CFG_VERSION_PATCH_STR "." CFG_VERSION_CUSTOM_STR, sizeof(name->version));
    strlcpy(name->release, CFG_VERSION_TWEAK_STR, sizeof(name->release));
    return 0;
}

int daemon(int nochdir, int noclose)
{
    return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
    return -1;
}

void openlog(char *ident, int option ,int facility)
{
}

void syslog(int priority, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void vsyslog(int pri, const char *format, va_list ap)
{
}

int getdtablesize(void)
{
    return 0;
}

pid_t setsid(void)
{
    return (pid_t)-1;
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    return -1;
}

void itpSemPostFromISR(sem_t* sem)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(sem->__sem_lock, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int itpSemGetValueFromISR(sem_t *sem, int *sval)
{
    if (NULL == sem)
    {
        return -1;
    }

    *sval = uxQueueMessagesWaitingFromISR(sem->__sem_lock);
    return 0;
}

int itpSemWaitTimeout(sem_t *sem, unsigned long ms)
{
    return (pdTRUE == xSemaphoreTake(sem->__sem_lock, ms * portTICK_PERIOD_MS)) ? 0 : -1;
}

int itpMessageQueueSendFromISR(mqd_t msgid, const char *msg)
{
    itpQueue* q = (itpQueue*) msgid;
    BaseType_t xHigherPriorityTaskWoken, xResult;

    xHigherPriorityTaskWoken = pdFALSE;

    xResult                  = xQueueSendFromISR(q->xQueue, (const void *)msg, &xHigherPriorityTaskWoken);

    if (xResult == pdTRUE)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
        return -1;
    }

    return 0;
}

void itpTaskNotifyGive(pthread_t pthread, int index)
{
    xTaskNotifyGiveIndexed((TaskHandle_t) pthread, index);
}

void itpTaskNotifyGiveFromISR(pthread_t pthread, int index)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveIndexedFromISR((TaskHandle_t) pthread, index, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int itpTaskNotifyTake(bool clearCountOnExit, long msToWait, int index)
{
    uint32_t ret = ulTaskNotifyTakeIndexed(index, clearCountOnExit ? pdTRUE : pdFALSE,
                                           msToWait == -1 ? portMAX_DELAY : msToWait * portTICK_PERIOD_MS);
    return (ret > 0) ? 0 : -1;
}

int itpTaskGetVolume(void)
{
    itpThread* t = (itpThread*) xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
    return t->volume;
}

long itpTimevalDiff(struct timeval *starttime, struct timeval *finishtime)
{
    uint64_t start = (uint64_t)starttime->tv_sec * 1000 + starttime->tv_usec / 1000;
    uint64_t finish = (uint64_t)finishtime->tv_sec * 1000 + finishtime->tv_usec / 1000;
    return (long)(finish - start);
}

uint32_t itpGetTickCount(void)
{
    if (!ithCpuInInterrupt())
    {
        return xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
    else
    {
        return xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    }
}

uint32_t itpGetTickDuration(uint32_t tick)
{
    uint32_t diff, t = itpGetTickCount();

    if (t >= tick)
    {
        diff = t - tick;
    }
    else
    {
        diff = 0xFFFFFFFF - tick + t;
    }
    return diff;
}

size_t itpFread(void* buf, size_t size, size_t count, FILE* fp)
{
    const ITPDevice* dev ;
    uint32_t index = (((uint32_t)fp->_file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }
    dev = itpDeviceTable[index];

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        if (size > 0)
        {
            return dev->read(((uint32_t)fp->_file) & ITP_FILE_MASK, (char *)buf, size * count, dev->info) / size;
        }

        return 0;
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

size_t itpFwrite(const void* buf, size_t size, size_t count, FILE* fp)
{
    const ITPDevice* dev;
    uint32_t index = (((uint32_t)fp->_file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }
    dev = itpDeviceTable[index];

    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        if (size > 0)
        {
            return dev->write(((uint32_t)fp->_file) & ITP_FILE_MASK, (char *)buf, size * count, dev->info) / size;
        }

        return 0;
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int itpFseek(FILE* fp, long offset, int whence)
{
    const ITPDevice* dev;
    uint32_t index = (((uint32_t)fp->_file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }

    dev = itpDeviceTable[index];
    if (NULL != dev)
    {
        /*  Pass call on to device driver.  Return result.        */
        if (dev->lseek(((uint32_t)fp->_file) & ITP_FILE_MASK, offset, whence, dev->info) == -1)
            return -1;

        return 0;
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

long itpFtell(FILE* fp)
{
    const ITPFSDevice* fsdev;
    uint32_t index = (((uint32_t)fp->_file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }
    fsdev = (ITPFSDevice*)itpDeviceTable[index];
    if (NULL != fsdev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return fsdev->ftell(((uint32_t)fp->_file) & ITP_FILE_MASK);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

extern int    _fwalk_reent (struct _reent *, int (*)(struct _reent *, FILE *));

int itpFflush(FILE* fp)
{
    uint32_t index;
    const ITPFSDevice * fsdev;

    if (fp == NULL)
    {
        return _fwalk_reent(_GLOBAL_REENT, _fflush_r);
    }

    if ((fp == stdin) || (fp == stdout) || (fp == stderr))
    {
        return _fflush_r(_REENT, fp);
    }

    index = (((uint32_t)fp->_file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }
    fsdev = (ITPFSDevice *)itpDeviceTable[index];
    if (NULL != fsdev)
    {
        /*  Pass call on to device driver.  Return result.        */
        return fsdev->fflush(((uint32_t)fp->_file) & ITP_FILE_MASK);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int itpFeof(FILE* fp)
{
    const ITPFSDevice* fsdev;
    uint32_t index = (((uint32_t)fp->_file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
    if (index >= ITP_DEVICE_MAX)
    {
        errno = ENOENT;
        return -1;
    }
    fsdev = (ITPFSDevice*)itpDeviceTable[index];
    if (NULL != fsdev)
    {
        #ifdef feof
        #undef feof
        #endif
        /*  Pass call on to device driver.  Return result.        */
        return fsdev->feof(((uint32_t)fp->_file) & ITP_FILE_MASK);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int ftruncate(int file, off_t length)
{
    if ((STDOUT_FILENO == file) || (STDERR_FILENO == file) || (STDIN_FILENO == file))
    {
        return 0;
    }
    else
    {
        const ITPFSDevice* dev;
        uint32_t index = (((uint32_t)file) & ITP_DEVICE_MASK) >> ITP_DEVICE_BIT;
        if (index >= ITP_DEVICE_MAX)
        {
            errno = ENOENT;
            return -1;
        }
        dev = (ITPFSDevice*) itpDeviceTable[index];

        if (NULL != dev)
        {
            /*  Pass call on to device driver.  Return result.        */
            return dev->ftruncate((((uint32_t)file) & ITP_FILE_MASK), length);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }
}

int truncate(const char *path, off_t length)
{
    ITPDriveStatus* driveStatusTable = NULL;
    ITPDriveStatus* driveStatus;
    int ret = -1;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    if (path[1] == ':')
    {
        driveStatus = &driveStatusTable[toupper(path[0]) - 'A'];
    }
    else
    {
        itpThread * t = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());
        driveStatus   = &driveStatusTable[t->volume];
    }

    if (driveStatus->avail)
    {
        const ITPFSDevice * dev = (ITPFSDevice *)itpDeviceTable[driveStatus->device >> ITP_DEVICE_BIT];
        if (NULL != dev)
        {
            ret = dev->truncate(path, length);
        }
        else
        {
            errno = ENOENT;
            return -1;
        }
    }

    if (0 != ret)
    {
        errno = EINVAL;
    }

    return ret;
}

ssize_t getline(char **bufptr, size_t *n, FILE *fp)
{
#define MIN_LINE_SIZE 4
#define DEFAULT_LINE_SIZE 128

    char *buf;
    char *ptr;
    size_t newsize, numbytes;
    int pos;
    int ch;
    bool cont;

    if ((fp == NULL) || (bufptr == NULL) || (n == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    buf = *bufptr;
    if ((buf == NULL) || (*n < MIN_LINE_SIZE))
    {
        buf = (char *)realloc (*bufptr, DEFAULT_LINE_SIZE);
        if (buf == NULL)
        {
            return -1;
        }
        *bufptr = buf;
        *n = DEFAULT_LINE_SIZE;
    }

    numbytes = *n;
    ptr = buf;

    cont = true;

    while (cont)
    {
        /* fill buffer - leaving room for nul-terminator */
        while (--numbytes > 0)
        {
            if ((ch = getc (fp)) == EOF)
            {
                cont = false;
                break;
            }
            else
            {
                *ptr++ = ch;
                if (ch == '\n')
                {
                  cont = false;
                  break;
                }
            }
        }

        if (cont)
        {
            /* Buffer is too small so reallocate a larger buffer.  */
            pos = ptr - buf;
            newsize = (*n << 1);
            buf = realloc (buf, newsize);
            if (buf == NULL)
            {
                break;
            }

            /* After reallocating, continue in new buffer */
            *bufptr = buf;
            *n = newsize;
            ptr = buf + pos;
            numbytes = newsize - pos;
        }
    }

    /* if no input data, return failure */
    if (ptr == buf)
    {
        return -1;
    }

    /* otherwise, nul-terminate and return number of bytes read */
    *ptr = '\0';
    return (ssize_t)(ptr - buf);
}

int gethostname(char *name, size_t len)
{
    strncpy(name, "lwip", len);
    return 0;
}

#ifdef CFG_NET_ENABLE
int getnameinfo(const struct sockaddr *sa, socklen_t addrlen,
                   char *host, socklen_t hostlen,
                   char *serv, socklen_t servlen,
                   unsigned int flags) 
{

    if (sa->sa_family == AF_INET)
    {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        char ip[16];

        inet_ntop(AF_INET, &(sin->sin_addr), ip, sizeof(ip));

        if (flags & NI_NUMERICHOST) 
        {
            strncpy(host, ip, hostlen);
        }

        if (flags & NI_NUMERICSERV)
        {
            snprintf(serv, servlen, "%d", ntohs(sin->sin_port));
        } else
        {
            struct servent *se = getservbyport(sin->sin_port, "tcp");
            if (se == NULL)
            {

                return EAI_FAIL; 
            }
            strncpy(serv, se->s_name, servlen);
        }

    }else
    {

        return EAI_FAIL;
    }

    return 0;
}
#else
int getnameinfo(const struct sockaddr *sa, socklen_t addrlen, char *host,
         socklen_t hostlen, char *serv, socklen_t servlen, unsigned int flags)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return EAI_FAIL;
}
#endif //CFG_NET_ENABLE

const char *gai_strerror(int ecode)
{
    switch(ecode) {
    case EAI_FAIL   : return "A non-recoverable error occurred";
#if 0
    case EAI_FAMILY : return "The address family was not recognized or the address length was invalid for the specified family";
#endif
    case EAI_NONAME : return "The name does not resolve for the supplied parameters";
    case 201 : return "EAI_SERVICE";
    case 203 : return "EAI_MEMORY";
    case 210 : return "HOST_NOT_FOUND";
    case 211 : return "NO_DATA";
    case 212 : return "NO_RECOVERY";
    case 213 : return "TRY_AGAIN";
    }

    return "Unknown error";
}

struct servent *getservbyport(int port, const char *proto)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

struct hostent *gethostbyaddr(const void *addr,
                              socklen_t len, int type)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

struct passwd* getpwuid(uid_t uid)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

struct group* getgrgid(gid_t gid)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

struct passwd* getpwnam(const char *name)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

struct group* getgrnam(const char *name)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

extern caddr_t __mmap_start__;
extern caddr_t __mmap_end__;

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    static unsigned int mmap_top = 0U;

    if (mmap_top > (unsigned int)&__mmap_end__)
    {
        return MAP_FAILED;
    }

    // Let's only allocate in increments of a cache line.
    if (mmap_top == 0)
    {
        mmap_top = (unsigned int)&__mmap_start__;
    }

    unsigned int ret = mmap_top;
    mmap_top += ((len + 31) & (~(0x1F)));
    if ((mmap_top > (unsigned int)&__mmap_end__) || (mmap_top < (unsigned int)&__mmap_start__))
    {
        return MAP_FAILED;
    }
    else
    {
        return ((void *)ret);
    }
}

int munmap(void *addr, size_t len)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return 0;
}

int mprotect(void *addr, size_t len, int prot)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return 0;
}

int msync(void *addr, size_t len, int flags)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return 0;
}

int mlock(const void *addr, size_t len)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return 0;
}

int munlock(const void *addr, size_t len)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return 0;
}

int setpriority(int which, id_t who, int value)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return 0;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

char *realpath(const char *path,char *resolved_path)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

int nftw(const char *dirpath,
        int (*fn) (const char *fpath, const struct stat *sb,
                   int typeflag, struct FTW *ftwbuf),
        int nopenfd, int flags)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

FILE *popen(const char *command, const char *type)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return NULL;
}

int pclose(FILE *stream)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

int setuid(uid_t uid)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

int fchmod(int fildes, mode_t mode)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    errno = EIO;
    return -1;
}

int symlink(const char *name1, const char *name2)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    errno = EIO;
    return -1;
}

int utimes(const char *filename, const struct timeval times[2])
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

uint32_t itpGetFreeHeapSize(void)
{
    struct mallinfo mi = mallinfo();
    return (unsigned int)__heap_end__ - (unsigned int)__heap_start__ - mi.uordblks;
}

unsigned alarm(unsigned __secs)
{
    return 0;
}

int	pthread_setcancelstate(int __state, int *__oldstate)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

void _pthread_cleanup_push(struct _pthread_cleanup_context *_context, void (*_routine)(void *), void *_arg)
{
    itpThread * self    = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());

    _context->_routine  = _routine;
    _context->_arg      = _arg;
    _context->_previous = self->cleanup_context;

    if ((_context->_previous != NULL) && ((char *)_context >= (char *)_context->_previous))
    {
        _context->_previous = NULL;
    }

    self->cleanup_context = _context;
}

void pthread_testcancel(void)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
}

void _pthread_cleanup_pop(struct _pthread_cleanup_context *_context, int _execute)
{
    itpThread * self = (itpThread *)xTaskGetApplicationTaskTag(xTaskGetCurrentTaskHandle());

    if (_execute)
    {
        _context->_routine(_context->_arg);
    }

    self->cleanup_context = _context->_previous;
}

int	pthread_condattr_init(pthread_condattr_t *__attr)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

int	pthread_condattr_destroy(pthread_condattr_t *__attr)
{
    ITP_LOG_DBG("%s NO IMPL\n", __FUNCTION__);
    return -1;
}

#ifndef CFG_NET_ENABLE
#ifdef __BSD_VISIBLE

int fcntl(int fd, int cmd, ...)
{
    return 0;
}

int select (int __n, fd_set * __readfds, fd_set * __writefds, fd_set * __exceptfds, struct timeval * __timeout)
{
    return 0;
}

int pipe(int __fildes[2])
{
    return 0;
}

#else

int itpSocketFcntl(int fd, int cmd, int arg)
{
    return 0;
}

int itpSocketSelect(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
                struct timeval *timeout)
{
    return 0;
}
#endif // __BSD_VISIBLE
#endif // !CFG_NET_ENABLE

ITPEvent* itpEventCreate(void)
{
    ITPEvent * ev = calloc(1, sizeof(ITPEvent));
    if (NULL == ev)
    {
        return NULL;
    }

    ev->pxEvent = xEventGroupCreate();
    if (NULL == ev->pxEvent)
    {
        free(ev);
        return NULL;
    }
    return ev;
}

void itpEventDestroy(ITPEvent* event)
{
    vEventGroupDelete(event->pxEvent);
    free(event);
}

unsigned long itpEventWaitBits(ITPEvent* event, unsigned long bits, bool clear, bool waitall, unsigned long ms)
{
    EventBits_t result;
    TickType_t  xTicksToWait;

    if (ms == (unsigned long)-1)
    {
        xTicksToWait = portMAX_DELAY;
    }
    else
    {
        xTicksToWait = ms * portTICK_PERIOD_MS;
    }

    result =
        xEventGroupWaitBits(event->pxEvent, bits, clear ? pdTRUE : pdFALSE, waitall ? pdTRUE : pdFALSE, xTicksToWait);
    if (result == 0)
    {
        ITP_LOG_WARN("WAIT EVENT TIMEOUT\n");
    }
    return result;
}

int itpEventSetBits(ITPEvent* event, unsigned long bits)
{
    if (!ithCpuInInterrupt())
    {
        // EventBits_t uxBits = xEventGroupSetBits(event->pxEvent, bits);
        xEventGroupSetBits(event->pxEvent, bits);
    }
    else
    {
        BaseType_t xHigherPriorityTaskWoken, xResult;

        xHigherPriorityTaskWoken = pdFALSE;

        xResult                  = xEventGroupSetBitsFromISR(event->pxEvent, bits, &xHigherPriorityTaskWoken);

        if (xResult != pdFAIL)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

void itpTaskSuspend(pthread_t pthread)
{
    vTaskSuspend((TaskHandle_t) pthread);
}

void itpTaskResume(pthread_t pthread)
{
    if (!ithCpuInInterrupt())
    {
        vTaskResume((TaskHandle_t)pthread);
    }
    else
    {
        BaseType_t xYieldRequired = xTaskResumeFromISR((TaskHandle_t)pthread);
        if (xYieldRequired == pdTRUE)
        {
            vTaskSwitchContext();
        }
    }
}

#ifdef __riscv

int access(const char *fn, int flags)
{
    struct stat s;
    if (stat(fn, &s))
    {
        return -1;
    }
    if (s.st_mode & S_IFDIR)
    {
        return 0;
    }
    if (flags & W_OK)
    {
        if (s.st_mode & S_IWRITE)
        {
            return 0;
        }
        return -1;
    }
    return 0;
}

#endif // __riscv
 