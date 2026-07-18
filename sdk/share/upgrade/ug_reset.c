#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <inttypes.h>
#include <string.h>
#include "ite/ug.h"
#include "ug_cfg.h"

static unsigned long GetFileSize(const char *input)
{
    unsigned long ret     = 0;
    FILE *        chkFile = fopen(input, "rb");
    if (chkFile != NULL)
    {
        if (fseek(chkFile, 0L, SEEK_END) == 0)
        {
            ret = ftell(chkFile);
        }
        fclose(chkFile);
    }
    return ret;
}

int ugCopyFile(const char* destPath, const char* srcPath)
{
    int ret = 0;
    FILE *rf = NULL;
    FILE *wf = NULL;
    uint8_t* buf = NULL;
    uint8_t* buf2 = NULL;
    uint32_t filesize, bufsize, readsize, size2, remainsize;
    struct statvfs info;

    filesize = GetFileSize(srcPath);

    UG_LOG_INFO("Copy %s (%ld bytes)", srcPath, filesize);

    // check capacity
    if (statvfs(destPath, &info) == 0)
    {
        uint64_t avail = info.f_bfree * info.f_bsize;
        FILE *   chkFile = fopen(destPath, "rb");
        if (chkFile != NULL)
        {
            if (fseek(chkFile, 0L, SEEK_END) == 0)
            {
                avail += ftell(chkFile);
            }
            fclose(chkFile);
        }

        if (filesize > avail)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of space: %" PRIu32 " > %" PRIu64 "\n", filesize, avail);
            goto end;
        }
    }

    if ((rf = fopen(srcPath, "rb")) == NULL)
    {
        ret = __LINE__;
        UG_LOG_ERR("cannot open file %s: %d\n", srcPath, errno);
        goto end;
    }

    bufsize = CFG_UG_BUF_SIZE < filesize ? CFG_UG_BUF_SIZE : filesize;
    buf = malloc(bufsize);
    if (!buf)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %ld\n", bufsize);
        goto end;
    }

    if ((wf = fopen(destPath, "wb")) == NULL)
    {
        ret = __LINE__;
        UG_LOG_ERR("cannot open file %s: %d\n", destPath, errno);
        goto end;
    }

    remainsize = filesize;
    do
    {
        readsize = (remainsize < bufsize) ? remainsize : bufsize;
        size2 = fread(buf, 1, readsize, rf);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        size2 = fwrite(buf, 1, readsize, wf);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    while (remainsize > 0);

    putchar('\n');

    // verify
    fclose(wf);
    wf = NULL;

    buf2 = malloc(bufsize);
    if (!buf2)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %ld\n", bufsize);
        goto end;
    }

    wf = fopen(destPath, "rb");
    if (!wf)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot open file %s: %d\n", destPath, errno);
        goto end;
    }

    if (fseek(rf, 0, SEEK_SET))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot seek file %d\n", errno);
        goto end;
    }

    UG_LOG_INFO("Verifying");

    remainsize = filesize;
    do
    {
        readsize = (remainsize < bufsize) ? remainsize : bufsize;
        size2 = fread(buf, 1, readsize, rf);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        size2 = fread(buf2, 1, readsize, wf);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        if (memcmp(buf, buf2, readsize))
        {
            ret = __LINE__;
            UG_LOG_ERR("\nVerify failed.\n");
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    } while (remainsize > 0);
    putchar('\n');

end:
    if (rf)
        fclose(rf);

    if (wf)
        fclose(wf);

    if (buf2)
        free(buf2);

    if (buf)
        free(buf);

    return ret;
}

int ugRestoreDir(const char* dest, const char* src)
{
    DIR           *dir;
    struct dirent *ent;
    int ret = 0;

    dir = opendir(src);
    if (dir == NULL)
    {
        UG_LOG_ERR("cannot open directory %s\n", src);
        ret = __LINE__;
        goto end;
    }

    while ((ent = readdir(dir)) != NULL)
    {
        char destPath[PATH_MAX];
        int ret1;

        if (strcmp(ent->d_name, ".") == 0)
            continue;

        if (strcmp(ent->d_name, "..") == 0)
            continue;

        strlcpy(destPath, dest, sizeof(destPath));
        strlcat(destPath, "/", sizeof(destPath));

        if (ent->d_type == DT_DIR)
        {
            if (ent->d_name[0] == '~')
            {
                strlcat(destPath, &ent->d_name[1], sizeof(destPath));
                UG_LOG_INFO("Remove dir %s\n", destPath);
                ugDeleteDirectory(destPath);
            }
            else
            {
                char srcPath[PATH_MAX];

                strlcat(destPath, ent->d_name, sizeof(destPath));

                ret1 = mkdir(destPath, S_IRWXU);
                if (ret1)
                {
                    UG_LOG_WARN("cannot create dir %s: %d\n", destPath, errno);

                    if (ret == 0)
                        ret = ret1;
                }
                else
                {
                    strlcpy(srcPath, src, sizeof(srcPath));
                    strlcat(srcPath, "/", sizeof(srcPath));
                    strlcat(srcPath, ent->d_name, sizeof(srcPath));

                    ret1 = ugRestoreDir(destPath, srcPath);
                    if (ret1)
                    {
                        if (ret == 0)
                            ret = ret1;
                    }
                }
            }
        }
        else
        {
            if (ent->d_name[0] == '~')
            {
                strlcat(destPath, &ent->d_name[1], sizeof(destPath));

                UG_LOG_INFO("Remove file %s\n", destPath);
                ret1 = remove(destPath);
                if (ret1)
                {
                    UG_LOG_WARN("cannot remove file %s: %d\n", destPath, errno);
                }
            }
            else
            {
                char srcPath[PATH_MAX];

                strlcat(destPath, ent->d_name, sizeof(destPath));
                strlcpy(srcPath, src, sizeof(srcPath));
                strlcat(srcPath, "/", sizeof(srcPath));
                strlcat(srcPath, ent->d_name, sizeof(srcPath));

                ret1 = ugCopyFile(destPath, srcPath);
                if (ret1)
                {
                    if (ret == 0)
                        ret = ret1;
                }
            }
        }
    }

end:
    if (dir)
    {
        if (closedir(dir))
            UG_LOG_WARN("cannot closedir (%s)\n", src);
    }
    return ret;
}

int ugResetFactory(void)
{
    DIR           *dir;
    struct dirent *ent;
    int ret = 0;

    dir = opendir(CFG_PRIVATE_DRIVE ":/backup");
    if (dir == NULL)
    {
        UG_LOG_ERR("cannot open directory %s\n", CFG_PRIVATE_DRIVE ":/backup");
        ret = __LINE__;
        goto end;
    }

    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0)
            continue;

        if (strcmp(ent->d_name, "..") == 0)
            continue;

        if (ent->d_type == DT_DIR)
        {
            char destPath[PATH_MAX];
            char srcPath[PATH_MAX];
            int ret1;

            strlcpy(destPath, ent->d_name, sizeof(destPath));
            strlcat(destPath, ":", sizeof(destPath));
            strlcpy(srcPath, CFG_PRIVATE_DRIVE ":/backup/", sizeof(srcPath));
            strlcat(srcPath, ent->d_name, sizeof(srcPath));

            ret1 = ugRestoreDir(destPath, srcPath);
            if (ret1)
            {
                if (ret == 0)
                    ret = ret1;
            }
        }
    }

end:
    if (dir)
    {
        if (closedir(dir))
            UG_LOG_WARN("cannot closedir (%s)\n", CFG_PRIVATE_DRIVE ":/backup");
    }
    return ret;
}
