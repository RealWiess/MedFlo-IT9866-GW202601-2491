#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/syslimits.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "itp_cfg.h"
#include "ite/ite_nand.h"
#include "direct/yaffsfs.h"
#include "direct/yportenv.h"
#include "direct/test-framework/yaffs_fileem2k.h"
#include "itp_yaffs2.h"

struct YaffsContext {
    struct yaffs_dev    * device[ITP_MAX_PARTITION];
    int                 device_count;
};

extern ITE_NF_INFO
itpGetNfInfo ();

int             itp_yaffs_info_block[3]     = {0};
unsigned char   * itp_yaffs_nf_block_buffer = NULL;
int             detect_reading_cycle_issue  = 0;
// static struct YaffsBadBlockCollect bbcollect;
// struct YaffsBadBlockCollect itpGetYaffsBadBlockCollect() {return bbcollect;}

static ITE_NF_INFO          itp_yaffs_nf_info;
static unsigned char        * itp_yaffs_nf_page_buffer = NULL;
static int                  itp_yaffs_flash_page_size;
static int                  itp_yaffs_flash_spare_size;
static int                  itp_yaffs_flash_block_size;
static int                  itp_yaffs_flash_blocks;

static struct YaffsContext  yaffs_context;

static int
YaffsOpen (
    const char  * name,
    int         flags,
    int         mode,
    void        * info)
{
    if ((flags & O_WRONLY) && !(flags & O_APPEND))
    {
        char _name[NAME_MAX + 1];
        strlcpy(_name, name, sizeof(_name));
        strlcat(_name, ".yaffstmp", sizeof(_name));
        return yaffs_open(_name, flags, mode);
    }
    else
    {
        return yaffs_open(name, flags, mode);
    }
}

static int
YaffsClose (
    int     fd,
    void    * info)
{
    return yaffs_close(fd);
}

static int
YaffsRead (
    int     fd,
    char    * ptr,
    int     len,
    void    * info)
{
	if (len == 0)
	{
		return len;
	}
    return yaffs_read(fd, ptr, len);
}

static int
YaffsWrite (
    int     fd,
    char    * ptr,
    int     len,
    void    * info)
{
	if (len == 0)
	{
		return len;
	}
    return yaffs_write(fd, ptr, len);
}

static int
YaffsLseek (
    int     fd,
    int     ptr,
    int     dir,
    void    * info)
{
    return yaffs_lseek(fd, ptr, dir);
}

static int
YaffsRemove (
    const char * path)
{
    return yaffs_unlink(path);
}

static int
YaffsRename (
    const char  * oldname,
    const char  * newname)
{
    return yaffs_rename(oldname, newname);
}

static int
YaffsChdir (
    const char * path)
{
	return 0;
}

static int
YaffsChmod (
    const char  * path,
    mode_t      mode)
{
    return yaffs_chmod(path, mode);
}

static int
YaffsMkdir (
    const char  * path,
    mode_t      mode)
{
    return yaffs_mkdir(path, mode);
}

static int
YaffsStat (
    const char  * path,
    struct stat * sbuf)
{
    int                 ret;
    struct yaffs_stat   yaffs_st;
    ret = yaffs_stat(path, &yaffs_st);
    if (ret != -1)
    {
        sbuf->st_dev        = ITP_DEVICE_YAFFS2;
        sbuf->st_ino        = yaffs_st.st_ino;
        sbuf->st_size       = yaffs_st.st_size;
        sbuf->st_blksize    = yaffs_st.st_blksize;
        sbuf->st_mode       = yaffs_st.st_mode;
        sbuf->st_atime      = yaffs_st.yst_atime;
        sbuf->st_mtime      = yaffs_st.yst_mtime;
    }
    return ret;
}

static int
YaffsStatvfs (
    const char      * path,
    struct statvfs  * sbuf)
{
    memset(sbuf, 0, sizeof(*sbuf));
    sbuf->f_bsize   = itp_yaffs_flash_block_size;
    sbuf->f_frsize  = itp_yaffs_flash_block_size;
    sbuf->f_blocks  = itp_yaffs_flash_blocks;
    sbuf->f_bfree   = yaffs_freespace(path) / itp_yaffs_flash_block_size;
    sbuf->f_bavail  = sbuf->f_bfree;
    sbuf->f_namemax = FILENAME_MAX;
    sbuf->f_files   = -1;
    sbuf->f_ffree   = -1;
    sbuf->f_favail  = -1;
    sbuf->f_flag    = -1;
    sbuf->f_fsid    = ITP_DEVICE_YAFFS2;

    return 0;
}

static int
YaffsFstat (
    int         fd,
    struct stat * st)
{
    int                 ret;
    struct yaffs_stat   yaffs_st;
    ret = yaffs_fstat(fd, &yaffs_st);
    if (ret != -1)
    {
        st->st_dev      = ITP_DEVICE_YAFFS2;
        st->st_ino      = yaffs_st.st_ino;
        st->st_size     = yaffs_st.st_size;
        st->st_blksize  = yaffs_st.st_blksize;
        st->st_mode     = yaffs_st.st_mode;
        st->st_atime    = yaffs_st.yst_atime;
        st->st_mtime    = yaffs_st.yst_mtime;
    }
    return ret;
}

static char *
YaffsGetcwd (
    char    * buf,
    size_t  size)
{
	return NULL;
}

static int
YaffsRmdir (
    const char * path)
{
    return yaffs_rmdir(path);
}

static int
YaffsClosedir (
    DIR * dirp)
{
    int         ret;
    yaffs_DIR   * d = (yaffs_DIR *)dirp->dd_buf;
    ret = yaffs_closedir(d);
    free(dirp);
    return ret;
}

static DIR *
YaffsOpendir (
    const char * path)
{
    yaffs_DIR   * d;
    DIR         * dirp = malloc(sizeof(DIR));
    if (!dirp)
    {
        errno = ENOMEM;
        return NULL;
    }

    d = yaffs_opendir(path);
    if (!d)
    {
        free(dirp);
        return NULL;
    }
    dirp->dd_fd     = ITP_DEVICE_YAFFS2;
    dirp->dd_size   = sizeof(*d);
    dirp->dd_buf    = (char *) d;
    dirp->dd_loc    = -1;

    return dirp;
}

static struct dirent *

YaffsReaddir (
    DIR * dirp)
{
    struct dirent       * de;
    struct yaffs_dirent * yaffs_de;
    yaffs_DIR           * d = (yaffs_DIR *)dirp->dd_buf;

    yaffs_de    = yaffs_readdir(d);
    de          = (struct dirent *)yaffs_de;
    return de;
}

static void
YaffsRewinddir (
    DIR * dirp)
{
    yaffs_DIR * d = (yaffs_DIR *)dirp->dd_buf;
    yaffs_rewinddir(d);
}

static long
YaffsFtell (
    int file)
{
    return yaffs_ftell(file);
}

static int
YaffsFflush (
    int file)
{
    return yaffs_flush(file);
}

static int
YaffsFeof (
    int file)
{
    return yaffs_feof(file);
}

static int
YaffsTruncate (
    const char  * path,
    off_t       length)
{
    return yaffs_truncate(path, length);
}

static int
YaffsFtruncate (
    int     file,
    off_t   length)
{
    return yaffs_ftruncate(file, length);
}

static void
YaffsGetReserveInfoBlock ()
{
    int block   = (CFG_NAND_RESERVED_SIZE - itp_yaffs_flash_block_size) / itp_yaffs_flash_block_size;
    int i;
    int count   = 0;
    int ret;
    for (i = 0; i < ITP_YAFFS_RESERVE_BLOCKS; i++)
    {
        ret = iteNfIsBadBlockForBoot(block - i);
        if (ret > 0)
        {
            itp_yaffs_info_block[count] = block - i;
            //(void)printf("YC: count = %d, reserve info block = %d\n", count, itp_yaffs_info_block[count]);
            if (count == 2)
            {
                break;
            }
            count++;
        }
    }
}

static int
YaffsWriteBadBlockTable (
    struct YaffsPartitionTable * pt,
    bool bProgrammer)
{
    struct YaffsBadBlockTable   bbtable[MAX_BBT_NUM];
    int                         bbtable_page, bbtable_index;
    int                         i, j;
    int                         ret;

    for (i = 0; i < MAX_BBT_NUM; i++)
    {
        bbtable[i].bb_count = 0;
        bbtable[i].bbt      = malloc(sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE);
    }

    // scan bad block
    for (i = 0; i < pt->par_count; i++)
    {
        for (j = pt->partitions[i].block_start; j <= pt->partitions[i].block_end; j++)
        {
        	if (bProgrammer == true)
        	{
            	ret = iteNfRead(j, 0, itp_yaffs_nf_page_buffer);
        	}
			else
			{
				ret = iteNfIsBadBlockForBoot(j);
			}
			
            if (ret < 0)
            {
                (void)printf("bad block: %d\n", j);
                bbtable_page                                = (j - 1) / MAX_BB_NUM_PER_PAGE;
                bbtable_index                               = bbtable[bbtable_page].bb_count;
                bbtable[bbtable_page].bbt[bbtable_index]    = j;
                bbtable[bbtable_page].bb_count++;
            }
        }
    }

    for (i = 0; i < MAX_BBT_NUM; i++)
    {
        memcpy(itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size), &(bbtable[i].bb_count), sizeof(uint32_t));
        memcpy(itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) + sizeof(uint32_t), bbtable[i].bbt, sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE);
    }

    for (i = 0; i < MAX_BBT_NUM; i++)
    {
        if (bbtable[i].bbt)
        {
            free(bbtable[i].bbt);
        }
    }

	return 0;
}

static int
YaffsReadPartitionTable (
    struct YaffsPartitionTable * pt)
{
    int i;
    int n;
    int check_tag_count         = 0;
    int YAFFS_FLASH_TAGADDRESS  = itp_yaffs_info_block[YAFFS_INFO_ADDR0] * itp_yaffs_flash_block_size + (itp_yaffs_nf_info.PageInBlk - 1) * itp_yaffs_flash_page_size;
    int YAFFS2_FLASH_PTADDRESS;

    memset(pt, 0, sizeof(*pt));

    // read tag
READ_TAG:
    n = iteNfRead(
        YAFFS_FLASH_TAGADDRESS / itp_yaffs_flash_block_size,
        (YAFFS_FLASH_TAGADDRESS % itp_yaffs_flash_block_size) / itp_yaffs_flash_page_size,
        itp_yaffs_nf_page_buffer);
    if (n < 0)
    {
        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__,
        YAFFS_FLASH_TAGADDRESS / itp_yaffs_flash_block_size,
        (YAFFS_FLASH_TAGADDRESS % itp_yaffs_flash_block_size) / itp_yaffs_flash_page_size);
        return -1;
    }
    else if (n == 2)
    {
        (void)printf("Detect read cycle issue\n");
        detect_reading_cycle_issue = 1;
    }

    if (!memcmp(itp_yaffs_nf_page_buffer, ITP_YAFFS_TAG, strlen(ITP_YAFFS_TAG)))
    {
        YAFFS2_FLASH_PTADDRESS = YAFFS_FLASH_TAGADDRESS - itp_yaffs_flash_page_size;
    }
    else
    {
        check_tag_count++;
        if (check_tag_count > 1)
        {
            ITP_LOG_ERR("not a valid yaffs partition table\n");
            return -1;
        }
        YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR1] * itp_yaffs_flash_block_size + (itp_yaffs_nf_info.PageInBlk - 1) * itp_yaffs_flash_page_size;
        goto READ_TAG;
    }

    // read partition table
    n = iteNfRead(
        YAFFS2_FLASH_PTADDRESS / itp_yaffs_flash_block_size,
        (YAFFS2_FLASH_PTADDRESS % itp_yaffs_flash_block_size) / itp_yaffs_flash_page_size,
        itp_yaffs_nf_page_buffer);
    if (n < 0)
    {
        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__,
        YAFFS2_FLASH_PTADDRESS / itp_yaffs_flash_block_size,
        (YAFFS2_FLASH_PTADDRESS % itp_yaffs_flash_block_size) / itp_yaffs_flash_page_size);
        return -1;
    }
    else if (n == 2)
    {
        (void)printf("Detect read cycle issue\n");
        detect_reading_cycle_issue = 1;
    }

    memcpy(pt, itp_yaffs_nf_page_buffer + (YAFFS2_FLASH_PTADDRESS % itp_yaffs_flash_page_size), sizeof(*pt));

    if (detect_reading_cycle_issue == 1)
    {
        YaffsRewriteInfoBlock(check_tag_count);
        detect_reading_cycle_issue = 0;
    }

    return 0;
}

static int
YaffsWritePartitionTable (
    struct YaffsPartitionTable * pt)
{
    int page = itp_yaffs_nf_info.PageInBlk - 2;

    memcpy(
        itp_yaffs_nf_block_buffer + page * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
        pt,
        sizeof(struct YaffsPartitionTable));
    return 0;
}

static int
YaffsRecoverBackupBlock ()
{
    int                         i, ret;
    int                         check_tag_count = 0;
    int                         YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR0] * itp_yaffs_flash_block_size + (itp_yaffs_nf_info.PageInBlk - 1) * itp_yaffs_flash_page_size;
    int                         BACKUP_INFO_ADDRESS;
    int                         info_block, info_page;
    int                         new_info_block;
    struct YaffsBackupBlockInfo info;

    // read tag
READ_TAG:
    ret = iteNfRead(
        YAFFS_FLASH_TAGADDRESS / itp_yaffs_flash_block_size,
        (YAFFS_FLASH_TAGADDRESS % itp_yaffs_flash_block_size) / itp_yaffs_flash_page_size,
        itp_yaffs_nf_page_buffer);
    if (ret < 0)
    {
        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__,
        YAFFS_FLASH_TAGADDRESS / itp_yaffs_flash_block_size,
        (YAFFS_FLASH_TAGADDRESS % itp_yaffs_flash_block_size) / itp_yaffs_flash_page_size);
        return -1;
    }

    if (!memcmp(itp_yaffs_nf_page_buffer, ITP_YAFFS_TAG, strlen(ITP_YAFFS_TAG)))
    {
        BACKUP_INFO_ADDRESS = YAFFS_FLASH_TAGADDRESS - 2 * itp_yaffs_flash_page_size;
        info_block          = BACKUP_INFO_ADDRESS / itp_yaffs_flash_block_size;
        info_page           = (BACKUP_INFO_ADDRESS % itp_yaffs_flash_block_size) / itp_yaffs_flash_page_size;
    }
    else
    {
        check_tag_count++;
        if (check_tag_count > 1)
        {
            ITP_LOG_ERR("not a valid info block\n");
            return -1;
        }
        YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR1] * itp_yaffs_flash_block_size + (itp_yaffs_nf_info.PageInBlk - 1) * itp_yaffs_flash_page_size;
        goto READ_TAG;
    }

    // read backup block info
    ret = iteNfRead(
        info_block,
        info_page,
        itp_yaffs_nf_page_buffer);
    if (ret < 0)
    {
        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, info_block, info_page);
        return -1;
    }
    else if (ret == 2)
    {
        (void)printf("Detect read cycle issue\n");
        detect_reading_cycle_issue = 1;
    }
    memcpy(&info, itp_yaffs_nf_page_buffer, sizeof(struct YaffsBackupBlockInfo));

    if (info.need_recover == 1)
    {
        (void)printf("Discover backup block, need to recover\n");
        int backup_block = itp_yaffs_info_block[YAFFS_BACKUP_BLOCK_ADDR];
        for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
        {
            ret = iteNfRead(
                backup_block,
                i,
                itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size));
            if (ret < 0)
            {
                ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, backup_block, i);
                return -1;
            }
        }

        ret = iteNfErase(info.recover_block_index);
        if (ret != true)
        {
            ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, info.recover_block_index);
            return -1;
        }

        for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
        {
            iteNfWrite(
                info.recover_block_index,
                i,
                itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
                itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) + itp_yaffs_flash_page_size);
        }

        for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
        {
            if (i != info_page)
            {
                ret = iteNfRead(
                    info_block,
                    i,
                    itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size));
                if (ret < 0)
                {
                    ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, info_block, i);
                    return -1;
                }
            }
            else
            {
                info.need_recover = 0;
                memcpy(itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
                    &info,
                    sizeof(struct YaffsBackupBlockInfo));
            }
        }

        if (check_tag_count == 0)
        {
            new_info_block = itp_yaffs_info_block[YAFFS_INFO_ADDR1];
        }
        else
        {
            new_info_block = itp_yaffs_info_block[YAFFS_INFO_ADDR0];
        }

        // (void)printf("new_info_block = %d\n", new_info_block);
        ret = iteNfErase(new_info_block);
        if (ret != true)
        {
            ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, new_info_block);
            return -1;
        }

        for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
        {
            ret = iteNfWrite(
                new_info_block,
                i,
                itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
                itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) + itp_yaffs_flash_page_size);
            if (ret < 0)
            {
                ithPrintf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, new_info_block, i);
                return -1;
            }
        }

        ret = iteNfErase(info_block);
        if (ret != true)
        {
            ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, info_block);
            return -1;
        }
    }
    else
    {
        if (detect_reading_cycle_issue == 1)
        {
            YaffsRewriteInfoBlock(check_tag_count);
            detect_reading_cycle_issue = 0;
        }
    }
    return 0;
}

static int
YaffsWriteInfoBlock (
    struct YaffsPartitionTable * pt, 
    bool bProgrammer)
{
    int ret;
    int i;
    int block = itp_yaffs_info_block[YAFFS_INFO_ADDR0];
    (void)printf("%s, block = %d\n", __FUNCTION__, block);

    YaffsWriteBadBlockTable(pt, bProgrammer);
    YaffsWritePartitionTable(pt);

    // write tag
    memcpy(
        itp_yaffs_nf_block_buffer + (itp_yaffs_nf_info.PageInBlk - 1) * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
        ITP_YAFFS_TAG,
        strlen(ITP_YAFFS_TAG));

    ret = iteNfErase(block);
    if (ret != true)
    {
        ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, block);
        return -1;
    }

    for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
    {
        ret = iteNfWrite(
            block,
            i,
            itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
            itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) + itp_yaffs_flash_page_size);
        if (ret < 0)
        {
            ithPrintf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, block, i);
            return -1;
        }
    }

    return 0;
}

static int
YaffsGetPartitions (
    ITPPartition * par)
{
    int                         i;
    struct YaffsPartitionTable  pt;

    switch (par->disk)
    {
        case ITP_DISK_NAND:
            break;

        default:
            ITP_LOG_INFO("unsupport disk: %d\n", par->disk);
            return -1;
    }

	YaffsGetReserveInfoBlock();
    if (YaffsReadPartitionTable(&pt) < 0)
    {
        return -1;
    }

    memset(par->size, 0, sizeof(par->size));
    memset(par->start, 0, sizeof(par->start));

    for (i = 0; i < pt.par_count; i++)
    {
        par->start[i]   = pt.partitions[i].block_start * itp_yaffs_flash_block_size;
        par->size[i]    = pt.partitions[i].blocks * itp_yaffs_flash_block_size;
        // (void)printf("YC: par %d, start = 0x%x, size = 0x%x\n", i, par->start[i], par->size[i]);
    }
    par->count = pt.par_count;
    // (void)printf("YC: par->count = %d\n", par->count);
    return 0;
}

static int
YaffsCreatePartitions (
    ITPPartition * par)
{
    int                         i;
    struct YaffsPartitionTable  pt;

    switch (par->disk)
    {
        case ITP_DISK_NAND:
            break;

        default:
            ITP_LOG_INFO("unsupport disk: %d\n", par->disk);
            return -1;
    }

    memset(&pt, 0, sizeof(pt));
    // memcpy(pt.tag, ITP_YAFFS_TAG, strlen(ITP_YAFFS_TAG));
    pt.par_count = par->count;
    for (i = 0; i < par->count; i++)
    {
        pt.partitions[i].block_start    = (uint32_t)(par->start[i] / itp_yaffs_flash_block_size);
        pt.partitions[i].blocks         = (uint32_t)(par->size[i] / itp_yaffs_flash_block_size);
        pt.partitions[i].block_end      = pt.partitions[i].block_start + pt.partitions[i].blocks - 1;
        if (pt.partitions[i].block_start && !pt.partitions[i].blocks)
        {
            pt.partitions[i].blocks     = itp_yaffs_flash_blocks - pt.partitions[i].block_start;
            pt.partitions[i].block_end  = pt.partitions[i].block_start + pt.partitions[i].blocks - 1;
            break;
        }
    }

    YaffsGetReserveInfoBlock();
    YaffsWriteInfoBlock(&pt, false);
    return 0;
}

static int
YaffsFormat (
    int partition)
{
    int             ret             = 0;
    ITPDriveStatus  * drive_status  = NULL;
    ITPDriveStatus  * driveStatusTable;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    drive_status = driveStatusTable + partition;
    if (drive_status->avail && (drive_status->device == ITP_DEVICE_YAFFS2))
    {
        ret = yaffs_format(drive_status->name, 1, 1, 1);
    }
    return ret;
}

static int
YaffsFormatMount (
    ITPDisk disk)
{
    int                         ret;
    int                         i, j;
    struct YaffsPartitionTable  pt;
    ITPDriveStatus              * drive_status = NULL;
    ITPDriveStatus              * driveStatusTable;

    switch (disk)
    {
        case ITP_DISK_NAND:
            break;

        default:
            ITP_LOG_INFO("unsupport disk: %d\n", disk);
            return -1;
    }

    if (YaffsReadPartitionTable(&pt) < 0)
    {
        return -1;
    }

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    yaffs_context.device_count = 0;
    for (i = j = 0; i < pt.par_count; i++)
    {
        for (; j < ITP_MAX_DRIVE; j++)
        {
            drive_status = driveStatusTable + j;

            if (!drive_status->avail)
            {
                break;
            }
        }
        drive_status->name[0]   = 'A' + j;
        drive_status->name[1]   = ':';
        drive_status->name[2]   = '/';

        // (void)printf("YC: %s, %d, %d, %d, start = 0x%x, end = 0x%x, blocks = %d\n", drive_status->name, itp_yaffs_nf_info.PageInBlk, itp_yaffs_flash_page_size, itp_yaffs_flash_spare_size, pt.partitions[i].block_start, pt.partitions[i].block_end, pt.partitions[i].blocks);
        yaffs_context.device[i] = yflash2_install_drv(drive_status->name,
                                itp_yaffs_nf_info.PageInBlk,
                                itp_yaffs_flash_page_size,
                                itp_yaffs_flash_spare_size,
                                pt.partitions[i].block_start,
                                pt.partitions[i].block_end);

        yaffs_format(drive_status->name, 0, 0, 0);

        ret = yaffs_mount(drive_status->name);
        if (ret >= 0)
        {
            drive_status->disk      = disk;
            drive_status->device    = ITP_DEVICE_YAFFS2;
            drive_status->avail     = true;
            write(ITP_DEVICE_DRIVE, drive_status, sizeof(ITPDriveStatus));
        }
        else
        {
            ithPrintf("[X] mount yaffs %02d %c: off: %08X len: %08X\n\n", i, drive_status->name[0], pt.partitions[i].block_start * itp_yaffs_flash_block_size, pt.partitions[i].blocks * itp_yaffs_flash_block_size);
        }
        yaffs_context.device_count++;
    }

    return 0;
}

static int
YaffsMount (
    ITPDisk disk)
{
    int                         ret;
    int                         i, j;
    struct YaffsPartitionTable  pt;
    ITPDriveStatus              * drive_status = NULL;
    ITPDriveStatus              * driveStatusTable;

    switch (disk)
    {
        case ITP_DISK_NAND:
            break;

        default:
            ITP_LOG_INFO("unsupport disk: %d\n", disk);
            return -1;
    }

    YaffsGetReserveInfoBlock();
    if (YaffsRecoverBackupBlock() < 0)
    {
        return -1;
    }

    if (YaffsReadPartitionTable(&pt) < 0)
    {
        return -1;
    }

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    yaffs_context.device_count = 0;
    for (i = j = 0; i < pt.par_count; i++)
    {
        for (; j < ITP_MAX_DRIVE; j++)
        {
            drive_status = driveStatusTable + j;

            if (!drive_status->avail)
            {
                break;
            }
        }
        drive_status->name[0]   = 'A' + j;
        drive_status->name[1]   = ':';
        drive_status->name[2]   = '/';

        // (void)printf("YC: %s, %d, %d, %d, start = 0x%x, end = 0x%x, blocks = %d\n", drive_status->name, itp_yaffs_nf_info.PageInBlk, itp_yaffs_flash_page_size, itp_yaffs_flash_spare_size, pt.partitions[i].block_start, pt.partitions[i].block_end, pt.partitions[i].blocks);
        yaffs_context.device[i] = yflash2_install_drv(drive_status->name,
                                itp_yaffs_nf_info.PageInBlk,
                                itp_yaffs_flash_page_size,
                                itp_yaffs_flash_spare_size,
                                pt.partitions[i].block_start,
                                pt.partitions[i].block_end);

        ret = yaffs_mount(drive_status->name);
        if (ret >= 0)
        {
            drive_status->disk      = disk;
            drive_status->device    = ITP_DEVICE_YAFFS2;
            drive_status->avail     = true;
            write(ITP_DEVICE_DRIVE, drive_status, sizeof(ITPDriveStatus));
        }
        else
        {
            ithPrintf("[X] mount yaffs %02d %c: off: %08X len: %08X\n\n", i, drive_status->name[0], pt.partitions[i].block_start * itp_yaffs_flash_block_size, pt.partitions[i].blocks * itp_yaffs_flash_block_size);
        }
        yaffs_context.device_count++;
    }
    return 0;
}

static int
YaffsUnmount (
    ITPDisk disk)
{
    int             i, j;
    ITPDriveStatus  * drive_status = NULL;
    ITPDriveStatus  * driveStatusTable;

    switch (disk)
    {
        case ITP_DISK_NAND:
            break;

        default:
            ITP_LOG_INFO("unsupport disk: %d\n", disk);
            return -1;
    }

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    for (i = j = 0; i < yaffs_context.device_count; i++)
    {
        for (; j < ITP_MAX_DRIVE; j++)
        {
            drive_status = driveStatusTable + j;

            if (drive_status->avail && (drive_status->disk == disk) && (drive_status->device == ITP_DEVICE_YAFFS2))
            {
                break;
            }
        }
        yaffs_unmount(drive_status->name);
        drive_status->avail = false;
        write(ITP_DEVICE_DRIVE, drive_status, sizeof(ITPDriveStatus));
        free(yaffs_context.device[i]);
    }

    return 0;
}

static int YaffsCheckFirstBooting()
{
	iteNfRead(
        0,
        0,
        itp_yaffs_nf_page_buffer);

	if (itp_yaffs_nf_page_buffer[20] == 0xAA && itp_yaffs_nf_page_buffer[21] == 0xBB && itp_yaffs_nf_page_buffer[22] == 0xCC )
	{
		printf("####  Check first booting..... ####\n");
		return 1;
	}
	else
	{
		return 0;
	}
}

static void YaffsBuildInfoBlock()
{
	int i;
	int ret;
	struct YaffsPartitionTable pt;
	int block = CFG_NAND_RESERVED_SIZE / itp_yaffs_flash_block_size;
NEXT_BLOCK:
	ret = iteNfRead(
        block,
        0,
        itp_yaffs_nf_page_buffer);

	if (ret < 0)
    {
        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, block, 0);
		block++;
		goto NEXT_BLOCK;
	}
	else
	{
		if (itp_yaffs_nf_page_buffer[0] != 'I' || itp_yaffs_nf_page_buffer[1] != 'N' || itp_yaffs_nf_page_buffer[2] != 'F' || itp_yaffs_nf_page_buffer[3] != 'O')
		{
		    ithPrintf("[X] First booting can't find INFO tag, %x %x %x %x\n", itp_yaffs_nf_page_buffer[0], itp_yaffs_nf_page_buffer[1], itp_yaffs_nf_page_buffer[2], itp_yaffs_nf_page_buffer[3]);
		}
	}
	memcpy(&pt, itp_yaffs_nf_page_buffer+4, sizeof(struct YaffsPartitionTable));
	//printf("YC: pt.par_count = %d\n", pt.par_count);
	//printf("YC: pt.partitions[0].block_start = %d, pt.partitions[0].blocks = %d\n", pt.partitions[0].block_start, pt.partitions[0].blocks);
	//printf("YC: pt.partitions[1].block_start = %d, pt.partitions[1].blocks = %d\n", pt.partitions[1].block_start, pt.partitions[1].blocks);

	YaffsGetReserveInfoBlock();
    YaffsWriteInfoBlock(&pt, true);
	iteNfErase(block);

	//modify first booting tag to original value
	for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
    {
        iteNfRead(
            0,
            i,
            itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size));
    }

	itp_yaffs_nf_block_buffer[20] = 0x34;
	itp_yaffs_nf_block_buffer[21] = 0x35;
	itp_yaffs_nf_block_buffer[22] = 0x36;

	iteNfErase(0);

    for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
    {
        iteNfWrite(
            0,
            i,
            itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
            itp_yaffs_nf_block_buffer + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) + itp_yaffs_flash_page_size);
    }	
}

static int
YaffsInit (
    void)
{
    static int start_up_called = 0;

    if (start_up_called)
    {
        return -1;
    }
    start_up_called             = 1;

    itp_yaffs_nf_info           = itpGetNfInfo();
    itp_yaffs_flash_page_size   = itp_yaffs_nf_info.PageSize;
    itp_yaffs_flash_spare_size  = itp_yaffs_nf_info.AllSprSize;
    itp_yaffs_flash_block_size  = itp_yaffs_nf_info.PageInBlk * itp_yaffs_nf_info.PageSize;
    itp_yaffs_flash_blocks      = itp_yaffs_nf_info.NumOfBlk;
    if (!itp_yaffs_nf_page_buffer)
    {
        itp_yaffs_nf_page_buffer = (unsigned char *)malloc(itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size + 256);
    }
    if (!itp_yaffs_nf_block_buffer)
    {
        itp_yaffs_nf_block_buffer = (unsigned char *)calloc((itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) * itp_yaffs_nf_info.PageInBlk, 1);
    }

    memset(&yaffs_context, 0, sizeof(yaffs_context));

    /*bad block collect init*/
    // memset(&bbcollect, 0, sizeof(bbcollect));

    /* Call the OS initialisation (eg. set up lock semaphore */
    yaffsfs_OSInitialisation();
    return 0;
}

static int
YaffsIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    int ret;

    switch (request)
    {
        case ITP_IOCTL_FORMAT_MOUNT:
            ret = YaffsFormatMount((ITPDisk)ptr);
            if (ret)
            {
                errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
            break;

        case ITP_IOCTL_MOUNT:
            ret = YaffsMount((ITPDisk)ptr);
            if (ret)
            {
                errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
            break;

        case ITP_IOCTL_UNMOUNT:
            ret = YaffsUnmount((ITPDisk)ptr);
            if (ret)
            {
                errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
            break;

        case ITP_IOCTL_GET_PARTITION:
            ret = YaffsGetPartitions((ITPPartition *)ptr);
            if (ret)
            {
                errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
            break;

        case ITP_IOCTL_CREATE_PARTITION:
            ret = YaffsCreatePartitions((ITPPartition *)ptr);
            if (ret)
            {
                errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
            break;

        case ITP_IOCTL_FORMAT:
            ret = YaffsFormat((int)ptr);
            if (ret)
            {
                errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
            break;
			
		case ITP_IOCTL_FIRST_BOOTING:
			if (YaffsCheckFirstBooting() == 1)
			{
				//printf("YC: ############## First booting build info block......................\n");
				YaffsBuildInfoBlock();
			}
			break;

		case ITP_IOCTL_BG_GC_EXIT:
			yaffsfs_bg_gc_exit();
		    break;
			
        case ITP_IOCTL_INIT:
            ret = YaffsInit();
            if (ret)
            {
                errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
            break;
            
        case ITP_IOCTL_GET_TABLE:
            ret = (int)yaffs_context.device;
            break;
		
        default:
            errno = (ITP_DEVICE_YAFFS2 << ITP_DEVICE_ERRNO_BIT) | __LINE__;
            return -1;
    }
    return 0;
}

const ITPFSDevice itpFSDeviceYaffs2 =
{
    {
        ":yaffs2",
        YaffsOpen,
        YaffsClose,
        YaffsRead,
        YaffsWrite,
        YaffsLseek,
        YaffsIoctl,
        NULL
    },
    YaffsRemove,
    YaffsRename,
    YaffsChdir,
    YaffsChmod,
    YaffsMkdir,
    YaffsStat,
    YaffsStatvfs,
    YaffsFstat,
    YaffsGetcwd,
    YaffsRmdir,
    YaffsClosedir,
    YaffsOpendir,
    YaffsReaddir,
    YaffsRewinddir,
    YaffsFtell,
    YaffsFflush,
    YaffsFeof,
    YaffsTruncate,
    YaffsFtruncate
};

int
YaffsRewriteInfoBlock (
    int check_tag_count)
{
    int ret;
    int i;
    int ori_info_block;
    int new_info_block;
    u8  * temp = malloc((itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) * itp_yaffs_nf_info.PageInBlk);

    if (check_tag_count == 0)
    {
        ori_info_block  = itp_yaffs_info_block[YAFFS_INFO_ADDR0];
        new_info_block  = itp_yaffs_info_block[YAFFS_INFO_ADDR1];
    }
    else if (check_tag_count == 1)
    {
        ori_info_block  = itp_yaffs_info_block[YAFFS_INFO_ADDR1];
        new_info_block  = itp_yaffs_info_block[YAFFS_INFO_ADDR0];
    }

    for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
    {
        ret = iteNfRead(
            ori_info_block,
            i,
            temp + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size));
        if (ret < 0)
        {
            ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, ori_info_block, i);
            if (temp)
            {
                free(temp);
            }
            return -1;
        }
    }

    ret = iteNfErase(new_info_block);
    if (ret != true)
    {
        ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, new_info_block);
        if (temp)
        {
            free(temp);
        }
        return -1;
    }

    for (i = 0; i < itp_yaffs_nf_info.PageInBlk; i++)
    {
        ret = iteNfWrite(
            new_info_block,
            i,
            temp + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size),
            temp + i * (itp_yaffs_flash_page_size + itp_yaffs_flash_spare_size) + itp_yaffs_flash_page_size);
        if (ret < 0)
        {
            ithPrintf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, new_info_block, i);
            if (temp)
            {
                free(temp);
            }
            return -1;
        }
    }

    ret = iteNfErase(ori_info_block);
    if (ret != true)
    {
        ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, ori_info_block);
        if (temp)
        {
            free(temp);
        }
        return -1;
    }

    if (temp)
    {
        free(temp);
    }
    return 0;
}
