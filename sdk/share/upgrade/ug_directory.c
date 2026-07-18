#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "ite/ug.h"
#include "ug_cfg.h"

#pragma pack(1)
typedef struct
{
    uint32_t dirname_size;
} directory_t;

int ugUpgradeDirectory(ITCStream *f, char* drive)
{
    int ret;
    directory_t dir;
    char buf[PATH_MAX];
    char *p = NULL;
    size_t len, readsize;

    // read directory
    readsize = itcStreamRead((ITCStream*)f, &dir, sizeof(directory_t));
    if (readsize != sizeof(directory_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(directory_t));
        return ret;
    }

    dir.dirname_size = itpLetoh32(dir.dirname_size);
    UG_LOG_DBG("dirname_size=%d\n", dir.dirname_size);

    strlcpy(buf, drive, sizeof(buf));

    readsize = itcStreamRead((ITCStream*)f, &buf[2], dir.dirname_size);
    if (readsize != dir.dirname_size)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, dir.dirname_size);
        return ret;
    }

    UG_LOG_INFO("[%d%%] Create directory %s\n", ugGetProrgessPercentage(), buf);
#if !defined(CFG_FS_LFS) && !defined(CFG_FS_YAFFS2)
    if (chdir(drive))
    {
        ret = __LINE__;
        UG_LOG_ERR("chdir %s fail: %d\n", drive, errno);
        return ret;
    }
#endif
    len = strlen(buf);
    if (buf[len - 1] == '/')
           buf[len - 1] = 0;

    ret = mkdir(buf, S_IRWXU);
    if (ret == -1)
    {
        UG_LOG_WARN("mkdir %s fail: %d\n", buf, errno);
    }

    return 0;
}

int ugCopyDirectory(const char* dest, const char* src)
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

        if (access(dest, W_OK))
        {
            ret1 = mkdir(destPath, S_IRWXU);
            if (ret1)
            {
                UG_LOG_WARN("cannot create dir %s: %d\n", destPath, errno);

                if (ret == 0)
                    ret = ret1;
            }
        }
        strlcat(destPath, "/", sizeof(destPath));

        if (ent->d_type == DT_DIR)
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

                ret1 = ugCopyDirectory(destPath, srcPath);
                if (ret1)
                {
                    if (ret == 0)
                        ret = ret1;
                }
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

end:
    if (dir)
    {
        if (closedir(dir))
            UG_LOG_WARN("cannot closedir (%s)\n", src);
    }
    return ret;
}

void ugDeleteDirectory(char* dirname)
{
    DIR           *dir;
    struct dirent *ent;

    dir = opendir(dirname);
    if (dir == NULL)
        return;

    while ((ent = readdir(dir)) != NULL)
    {
        char path[PATH_MAX];

        if (strcmp(ent->d_name, ".") == 0)
            continue;

        if (strcmp(ent->d_name, "..") == 0)
            continue;

        strlcpy(path, dirname, sizeof(path));
        strlcat(path, "/", sizeof(path));
        strlcat(path, ent->d_name, sizeof(path));

        if (ent->d_type == DT_DIR)
        {
            ugDeleteDirectory(path);
        }
        else
        {
            remove(path);
        }
    }
    closedir(dir);
    rmdir(dirname);
}
