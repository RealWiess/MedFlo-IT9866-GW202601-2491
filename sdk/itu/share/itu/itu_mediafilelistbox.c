#include <assert.h>
#include <dirent.h>
#include <malloc.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "redblack/redblack.h"
#include "ite/itu.h"
#include "itu_cfg.h"

#pragma warning(disable : 4996)
static const char mmflistboxName[] = "ITUMediaFileListBox";

typedef struct
{
    char * name;
    char * path;
} FileEntry;

static void MediaFileListNameListNewLog (ITUMediaFileListBox * mflistbox, struct dirent ** namelist, int n,
                                         const char * func)
{
    if (namelist)
    {
        /*bool bDebugPrintf = false;
        ITUWidget* widget = (ITUWidget*)mflistbox;
        if (widget && bDebugPrintf)
        {
            if (widget->name)
                (void)printf("[%s][%s]new dirent[%d] namelist %X\n", func, widget->name, n, &namelist);
        }*/
    }
}

static void MediaFileListNameListClear (ITUMediaFileListBox * mflistbox, struct dirent ** namelist, int n,
                                        const char * func)
{
    if (namelist)
    {
        // bool bDebugPrintf = false;
        // const ITUWidget* widget = (ITUWidget*)mflistbox;
        // int count = n;
        while (--n >= 0)
        {
            struct dirent * namel = namelist[n];
            free(namel);
        }
        // if (widget && bDebugPrintf)
        //{
        //	if (widget->name)
        //		(void)printf("[%s][%s]free dirent[%d] namelist %X\n", func, widget->name, count, &namelist);
        // }
        free(namelist);
    }
}

static int MediaFileListCompare (const void * pa, const void * pb, const void * config)
{
    const FileEntry * fea = (FileEntry *)pa;
    const FileEntry * feb = (FileEntry *)pb;
    assert(pa);
    assert(pb);

    return strcmp(fea->path, feb->path);
}

static void MediaFileListDestroy (ITUMediaFileListBox * mflistbox)
{
    assert(mflistbox);

    if (mflistbox->rbtree)
    {
        if (rb_check(mflistbox->rbtree))
        {
            int count = rb_count(mflistbox->rbtree);

            if (count > 0)
            {
                Redblack rb = NULL;
                for (rb = rb_first(mflistbox->rbtree); rb != mflistbox->rbtree; rb = rb_next(rb))
                {
                    FileEntry * val = (FileEntry *)rb->item;
                    if (val)
                    {
                        if (val->path)
                        {
                            free(val->path);
                        }
                        free(val);
                    }
                }
            }

            rb_destroy(mflistbox->rbtree, 0, 0);
        }
        mflistbox->rbtree = NULL;
    }
}

static void MediaFileListBoxParseDirectory (ITUMediaFileListBox * mflistbox, char * dir)
{
    DIR *            pdir = NULL;
    char             path[PATH_MAX];

    struct dirent ** namelist  = NULL;
    int              n         = scandir(dir, &namelist, 0, alphasort);
    bool             use_order = true;

    MediaFileListNameListNewLog(mflistbox, namelist, n, __func__);

    if (use_order && (n > 0))
    {
        int i = 0;
        /*int j = 0;
        for (j = 0; j < n; j++)
        {
            (void)printf("[found_df in %s] %s\n", dir, namelist[j]->d_name);
        }*/
        for (i = 0; i < n; i++)
        {
            FileEntry *fe, *ret;

            if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
            {
                goto end;
            }

            if ((strcmp(namelist[i]->d_name, ".") == 0) || (strcmp(namelist[i]->d_name, "..") == 0))
            {
                continue;
            }

            strlcpy(path, dir, sizeof(path));
            strlcat(path, "/", sizeof(path));
            strlcat(path, namelist[i]->d_name, sizeof(path));

            if (namelist[i]->d_type == DT_DIR)
            {
                MediaFileListBoxParseDirectory(mflistbox, path);
                if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                {
                    goto end;
                }
            }
            else
            {
                bool found = false;

                if (mflistbox->extensions[0] != '\0')
                {
                    char * ptr = strrchr(path, '.');
                    if (ptr)
                    {
                        char  buf[ITU_EXTENSIONS_SIZE];
                        char *ext, *saveptr;

                        ptr++;

                        strlcpy(buf, mflistbox->extensions, sizeof(buf));
                        ext = strtok_r(buf, ";", &saveptr);

                        do
                        {
                            if (stricmp(ptr, ext) == 0)
                            {
                                found = true;
                                break;
                            }
                            ext = strtok_r(NULL, ";", &saveptr);
                        } while (ext);
                    }
                }
                else
                {
                    found = true;
                }
                if (found)
                {
                    fe = malloc(sizeof(FileEntry));
                    if (fe == NULL)
                    {
                        ITU_LOG_ERR("out of memory: %d\n", (int)sizeof(FileEntry));
                        goto end;
                    }
                    fe->path = strdup(path);
                    fe->name = strrchr(fe->path, '/') + 1;

                    ret      = (FileEntry *)(rb_insert_int(mflistbox->rbtree, mflistbox->fileCount, (void *)fe)->item);
                    if (ret == NULL)
                    {
                        free(fe->path);
                        free(fe);
                        ITU_LOG_ERR("out of memory\n");
                        goto end;
                    }
                    mflistbox->fileCount++;
                    if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                    {
                        goto end;
                    }
                }
            }

            if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
            {
                goto end;
            }
        }
        MediaFileListNameListClear(mflistbox, namelist, n, __func__);
        return;
    }
    // Open the directory
    pdir = opendir(dir);
    if (pdir)
    {
        struct dirent * pent;

        if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
        {
            goto end;
        }

        // List the contents of the directory

        while ((pent = readdir(pdir)) != NULL)
        {
            FileEntry *fe, *ret;

            if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
            {
                goto end;
            }

            if ((strcmp(pent->d_name, ".") == 0) || (strcmp(pent->d_name, "..") == 0))
            {
                continue;
            }

            strlcpy(path, dir, sizeof(path));
            strlcat(path, "/", sizeof(path));
            strlcat(path, pent->d_name, sizeof(path));

            if (pent->d_type == DT_DIR)
            {
                MediaFileListBoxParseDirectory(mflistbox, path);
                if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                {
                    goto end;
                }
            }
            else
            {
                bool found = false;

                if (mflistbox->extensions[0] != '\0')
                {
                    char * ptr = strrchr(path, '.');
                    if (ptr)
                    {
                        char  buf[ITU_EXTENSIONS_SIZE];
                        char *ext, *saveptr;

                        ptr++;

                        strlcpy(buf, mflistbox->extensions, sizeof(buf));
                        ext = strtok_r(buf, ";", &saveptr);

                        do
                        {
                            if (stricmp(ptr, ext) == 0)
                            {
                                found = true;
                                break;
                            }
                            ext = strtok_r(NULL, ";", &saveptr);
                        } while (ext);
                    }
                }
                else
                {
                    found = true;
                }
                if (found)
                {
                    fe = malloc(sizeof(FileEntry));
                    if (fe == NULL)
                    {
                        ITU_LOG_ERR("out of memory: %d\n", (int)sizeof(FileEntry));
                        goto end;
                    }
                    fe->path = strdup(path);
                    fe->name = strrchr(fe->path, '/') + 1;

                    ret      = (FileEntry *)(rb_insert_int(mflistbox->rbtree, mflistbox->fileCount, (void *)fe)->item);
                    if (ret == NULL)
                    {
                        free(fe->path);
                        free(fe);
                        ITU_LOG_ERR("out of memory\n");
                        goto end;
                    }
                    mflistbox->fileCount++;
                    if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                    {
                        goto end;
                    }
                }
            }

            if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
            {
                goto end;
            }
        }
    }
    else
    {
        ITU_LOG_WARN("opendir(%s) failed\n", mflistbox->path);
        goto end;
    }

end:
    // Close the directory
    if (pdir)
    {
        closedir(pdir);
    }

    MediaFileListNameListClear(mflistbox, namelist, n, __func__);
}

static void * MediaFileListBoxCreateTask (void * arg)
{
    DIR *                 pdir      = NULL;
    ITUMediaFileListBox * mflistbox = (ITUMediaFileListBox *)arg;
    char                  path[PATH_MAX];

    struct dirent **      namelist  = NULL;
    int                   n         = 0;
    bool                  use_order = true;

    assert(mflistbox);

    if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
    {
        goto end;
    }

    if (strlen(mflistbox->path) == 0)
    {
        ITU_LOG_WARN("empty path\n");
        goto end;
    }
    else
    {
        n = scandir(mflistbox->path, &namelist, 0, alphasort);
        MediaFileListNameListNewLog(mflistbox, namelist, n, __func__);
    }

    MediaFileListDestroy(mflistbox);
    // mflistbox->rbtree = rbinit(MediaFileListCompare, NULL);
    mflistbox->rbtree = (void *)rb_create();
    if (mflistbox->rbtree == NULL)
    {
        ITU_LOG_ERR("insufficient memory\n");
        goto end;
    }

    if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
    {
        goto end;
    }

    // Open the directory
    if (use_order && (n > 0))
    {
        int i = 0;
        /*int j = 0;
        for (j = 0; j < n; j++)
        {
            (void)printf("[found_df in %s] %s\n", mflistbox->path, namelist[j]->d_name);
        }*/

        if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
        {
            goto end;
        }

        mflistbox->fileCount = 0;

        // List the contents of the directory
        for (i = 0; i < n; i++)
        {
            FileEntry *fe, *ret;

            if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
            {
                goto end;
            }

            if ((strcmp(namelist[i]->d_name, ".") == 0) || (strcmp(namelist[i]->d_name, "..") == 0))
            {
                continue;
            }

            strlcpy(path, mflistbox->path, sizeof(path));
            strlcat(path, namelist[i]->d_name, sizeof(path));

            if (namelist[i]->d_type == DT_DIR)
            {
                MediaFileListBoxParseDirectory(mflistbox, path);
                if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                {
                    goto end;
                }
            }
            else
            {
                bool found = false;

                if (mflistbox->extensions[0] != '\0')
                {
                    char * ptr = strrchr(path, '.');
                    if (ptr)
                    {
                        char  buf[ITU_EXTENSIONS_SIZE];
                        char *ext, *saveptr;

                        ptr++;

                        strlcpy(buf, mflistbox->extensions, sizeof(buf));
                        ext = strtok_r(buf, ";", &saveptr);

                        do
                        {
                            if (stricmp(ptr, ext) == 0)
                            {
                                found = true;
                                break;
                            }
                            ext = strtok_r(NULL, ";", &saveptr);
                        } while (ext);
                    }
                }
                else
                {
                    found = true;
                }
                if (found)
                {
                    fe = malloc(sizeof(FileEntry));
                    if (fe == NULL)
                    {
                        ITU_LOG_ERR("out of memory: %d\n", (int)sizeof(FileEntry));
                        goto end;
                    }
                    fe->path = strdup(path);
                    fe->name = strrchr(fe->path, '/') + 1;

                    ret      = (FileEntry *)(rb_insert_int(mflistbox->rbtree, mflistbox->fileCount, (void *)fe)->item);
                    if (ret == NULL)
                    {
                        free(fe->path);
                        free(fe);
                        ITU_LOG_ERR("out of memory\n");
                        goto end;
                    }
                    mflistbox->fileCount++;
                    if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                    {
                        goto end;
                    }
                }
            }

            if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
            {
                goto end;
            }
        }
    }
    else
    {
        pdir = opendir(mflistbox->path);
        if (pdir)
        {
            struct dirent * pent;

            if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
            {
                goto end;
            }

            mflistbox->fileCount = 0;

            // List the contents of the directory
            while ((pent = readdir(pdir)) != NULL)
            {
                FileEntry *fe, *ret;

                if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
                {
                    goto end;
                }

                if ((strcmp(pent->d_name, ".") == 0) || (strcmp(pent->d_name, "..") == 0))
                {
                    continue;
                }

                strlcpy(path, mflistbox->path, sizeof(path));
                strlcat(path, pent->d_name, sizeof(path));

                if (pent->d_type == DT_DIR)
                {
                    MediaFileListBoxParseDirectory(mflistbox, path);
                    if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                    {
                        goto end;
                    }
                }
                else
                {
                    bool found = false;

                    if (mflistbox->extensions[0] != '\0')
                    {
                        char * ptr = strrchr(path, '.');
                        if (ptr)
                        {
                            char  buf[ITU_EXTENSIONS_SIZE];
                            char *ext, *saveptr;

                            ptr++;

                            strlcpy(buf, mflistbox->extensions, sizeof(buf));
                            ext = strtok_r(buf, ";", &saveptr);

                            do
                            {
                                if (stricmp(ptr, ext) == 0)
                                {
                                    found = true;
                                    break;
                                }
                                ext = strtok_r(NULL, ";", &saveptr);
                            } while (ext);
                        }
                    }
                    else
                    {
                        found = true;
                    }
                    if (found)
                    {
                        fe = malloc(sizeof(FileEntry));
                        if (fe == NULL)
                        {
                            ITU_LOG_ERR("out of memory: %d\n", (int)sizeof(FileEntry));
                            goto end;
                        }
                        fe->path = strdup(path);
                        fe->name = strrchr(fe->path, '/') + 1;

                        ret = (FileEntry *)(rb_insert_int(mflistbox->rbtree, mflistbox->fileCount, (void *)fe)->item);
                        if (ret == NULL)
                        {
                            free(fe->path);
                            free(fe);
                            ITU_LOG_ERR("out of memory\n");
                            goto end;
                        }
                        mflistbox->fileCount++;
                        if (mflistbox->fileCount >= ITU_MEDIA_FILE_MAX_COUNT)
                        {
                            goto end;
                        }
                    }
                }

                if (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING)
                {
                    goto end;
                }
            }
        }
    }

end:
    // Close the directory
    if (pdir)
    {
        closedir(pdir);
    }

    MediaFileListNameListClear(mflistbox, namelist, n, __func__);

    mflistbox->mflistboxFlags &= ~ITU_FILELIST_BUSYING;
    mflistbox->mflistboxFlags |= ITU_FILELIST_CREATED;
    return NULL;
}

void ituMediaFileListBoxExit (ITUWidget * widget)
{
    ITUMediaFileListBox * mflistbox = (ITUMediaFileListBox *)widget;
    assert(mflistbox);
    ITU_ASSERT_THREAD();

    if (mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING)
    {
        mflistbox->mflistboxFlags |= ITU_FILELIST_DESTROYING;
    }

    while (mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING)
    {
        (void)usleep(1000);
    }

    MediaFileListDestroy(mflistbox);

    if (mflistbox->randomPlayedArray)
    {
        free(mflistbox->randomPlayedArray);
        mflistbox->randomPlayedArray = NULL;
    }
    ituWidgetExitImpl(widget);
}

bool ituMediaFileListBoxUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool                  result;
    ITUListBox *          listbox   = (ITUListBox *)widget;
    ITUMediaFileListBox * mflistbox = (ITUMediaFileListBox *)widget;
    assert(mflistbox);

    result = ituListBoxUpdate(widget, ev, arg1, arg2, arg3);

    if ((ev == ITU_EVENT_TIMER) && (mflistbox->mflistboxFlags & ITU_FILELIST_CREATED))
    {
        FileEntry * val;
        Redblack    rb   = NULL;
        ITCTree *   node = widget->tree.child;
        int         count;

        mflistbox->mflistboxFlags &= ~ITU_FILELIST_CREATED;

        if (node == NULL)
        {
            return result;
        }

        for (rb = rb_first(mflistbox->rbtree); rb != mflistbox->rbtree; rb = rb_next(rb))
        {
            ITUScrollText * scrolltext = (ITUScrollText *)node;
            val                        = (FileEntry *)rb->item;

            ituScrollTextSetString(scrolltext, val->name);
            // ituWidgetSetCustomData(scrolltext, val->path);
            scrolltext->tmpStr = val->path;

            node               = node->sibling;

            if (node == NULL)
            {
                break;
            }
        }

        for (; node; node = node->sibling)
        {
            ITUScrollText * scrolltext = (ITUScrollText *)node;
            ituScrollTextSetString(scrolltext, "");
        }

        count = itcTreeGetChildCount(widget);
        if (count > 0)
        {
            char buf[32];

            if (mflistbox->fileCount == 0)
            {
                listbox->pageIndex = listbox->pageCount = 0;
            }
            else
            {
                listbox->pageIndex = 1;
                listbox->pageCount = (mflistbox->fileCount + count - 1) / count;
            }

            if (listbox->pageIndex == listbox->pageCount)
            {
                listbox->itemCount = mflistbox->fileCount % count;
                if (listbox->itemCount == 0)
                {
                    listbox->itemCount = count;
                }
            }
            else
            {
                listbox->itemCount = count;
            }
            ituWidgetUpdate(widget, ITU_EVENT_LAYOUT, 0, 0, 0);
            ituListBoxSelect(listbox, -1);

            snprintf(buf, sizeof(buf), "%d %d", listbox->pageIndex, listbox->pageCount);
            ituExecActions((ITUWidget *)listbox, listbox->actions, ITU_EVENT_LOAD, (int)buf);
        }

        result = widget->dirty = true;
    }
    return widget->visible ? result : false;
}

void ituMediaFileListBoxOnLoadPage (ITUListBox * listbox, int pageIndex)
{
    ITUMediaFileListBox * mflistbox = (ITUMediaFileListBox *)listbox;
    ITCTree *             node;
    FileEntry *           val;
    Redblack              rb = NULL;
    int                   i, count;
    assert(mflistbox);
    assert(pageIndex <= listbox->pageCount);

    if ((mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING) || (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING) ||
        !mflistbox->rbtree)
    {
        return;
    }

    count = itcTreeGetChildCount(listbox);

    if (pageIndex == listbox->pageCount)
    {
        listbox->itemCount = mflistbox->fileCount % count;
        if (listbox->itemCount == 0)
        {
            listbox->itemCount = count;
        }
    }
    else
    {
        listbox->itemCount = count;
    }

    if (mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING)
    {
        return;
    }

    node = ((ITCTree *)listbox)->child;
    i    = 0;
    for (rb = rb_first(mflistbox->rbtree); rb != mflistbox->rbtree; rb = rb_next(rb))
    {
        ITUScrollText * scrolltext = (ITUScrollText *)node;

        if (i++ < count * (pageIndex - 1))
        {
            continue;
        }

        val = (FileEntry *)rb->item;

        ituScrollTextSetString(scrolltext, val->name);
        // ituWidgetSetCustomData(scrolltext, val->path);
        scrolltext->tmpStr = val->path;

        node               = node->sibling;

        if (node == NULL)
        {
            break;
        }
    }

    for (; node; node = node->sibling)
    {
        ITUScrollText * scrolltext = (ITUScrollText *)node;
        ituScrollTextSetString(scrolltext, "");
    }
    listbox->pageIndex = pageIndex;
}

#define FILELIST_TASK_STACK_SIZE (32 * 1024)

void ituMediaFileListBoxInit (ITUMediaFileListBox * mflistbox, int width, char * path)
{
    assert(mflistbox);
    ITU_ASSERT_THREAD();

    (void)memset(mflistbox, 0, sizeof(ITUMediaFileListBox));

    ituListBoxInit(&mflistbox->listbox, width);

    ituWidgetSetType(mflistbox, ITU_MEDIAFILELISTBOX);
    ituWidgetSetName(mflistbox, mmflistboxName);
    ituWidgetSetExit(mflistbox, ituMediaFileListBoxExit);
    ituWidgetSetUpdate(mflistbox, ituMediaFileListBoxUpdate);
    ituListBoxSetOnLoadPage(mflistbox, ituMediaFileListBoxOnLoadPage);

    if (path)
    {
        pthread_t      task;
        pthread_attr_t attr;

        strlcpy(mflistbox->path, path, sizeof(mflistbox->path));

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, FILELIST_TASK_STACK_SIZE);
        pthread_create(&task, &attr, MediaFileListBoxCreateTask, mflistbox);
        mflistbox->mflistboxFlags |= ITU_FILELIST_BUSYING;
    }
}

void ituMediaFileListBoxLoad (ITUMediaFileListBox * mflistbox, uint32_t base)
{
    assert(mflistbox);
    ituListBoxLoad(&mflistbox->listbox, base);

    ituWidgetSetExit(mflistbox, ituMediaFileListBoxExit);
    ituWidgetSetUpdate(mflistbox, ituMediaFileListBoxUpdate);
    ituListBoxSetOnLoadPage(mflistbox, ituMediaFileListBoxOnLoadPage);

    if (strlen(mflistbox->path) > 0)
    {
        pthread_t      task;
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, FILELIST_TASK_STACK_SIZE);
        pthread_create(&task, &attr, MediaFileListBoxCreateTask, mflistbox);
        mflistbox->mflistboxFlags |= ITU_FILELIST_BUSYING;
    }
}

void ituMediaFileListSetPath (ITUMediaFileListBox * mflistbox, char * path)
{
    int            len;
    pthread_t      task;
    pthread_attr_t attr;
    assert(mflistbox);
    ITU_ASSERT_THREAD();

    if ((mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING) || (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING))
    {
        return;
    }

    if (path)
    {
        strlcpy(mflistbox->path, path, sizeof(mflistbox->path));
    }
    else
    {
        mflistbox->path[0] = '\0';
    }

    len = strlen(mflistbox->path);
    if (len < 2)
    {
        return;
    }

    if (mflistbox->randomPlayedArray)
    {
        free(mflistbox->randomPlayedArray);
        mflistbox->randomPlayedArray = NULL;
        mflistbox->playIndex         = 0;
        mflistbox->randomPlayedCount = 0;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, FILELIST_TASK_STACK_SIZE);
    pthread_create(&task, &attr, MediaFileListBoxCreateTask, mflistbox);
    mflistbox->mflistboxFlags |= ITU_FILELIST_BUSYING;
}

ITUScrollText * ituMediaFileListPlay (ITUMediaFileListBox * mflistbox)
{
    const ITUWidget * widget  = (ITUWidget *)mflistbox;
    ITUListBox *      listbox = (ITUListBox *)mflistbox;
    assert(mflistbox);
    ITU_ASSERT_THREAD();

    if ((mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING) || (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING) ||
        (mflistbox->fileCount == 0))
    {
        return NULL;
    }

    if (mflistbox->randomPlay)
    {
        time_t t;
        int    i, tryCount;

        if (mflistbox->randomPlayedArray == NULL)
        {
            mflistbox->randomPlayedArray = malloc(mflistbox->fileCount * sizeof(int));
            if (mflistbox->randomPlayedArray == NULL)
            {
                return NULL;
            }
        }
        for (i = 0; i < mflistbox->fileCount; ++i)
        {
            mflistbox->randomPlayedArray[i] = -1;
        }

        mflistbox->playIndex         = 0;
        mflistbox->randomPlayedCount = 0;

        tryCount                     = mflistbox->fileCount * 2;

        srand((unsigned)time(&t));

        do
        {
            if (--tryCount <= 0)
            {
                break;
            }

            mflistbox->playIndex = rand() % mflistbox->fileCount;
        } while (mflistbox->randomPlayedArray[mflistbox->playIndex] >= 0);

        if (tryCount <= 0)
        {
            for (i = 0; i < mflistbox->fileCount; i++)
            {
                if (mflistbox->randomPlayedArray[i] == -1)
                {
                    mflistbox->playIndex = i;
                    break;
                }
            }
        }

        if (mflistbox->playIndex < mflistbox->fileCount)
        {
            int  count;
            char buf[32];

            mflistbox->randomPlayedArray[mflistbox->playIndex] = mflistbox->randomPlayedCount;
            mflistbox->randomPlayedCount++;

            if (widget->type == ITU_SCROLLMEDIAFILELISTBOX)
            {
                count = itcTreeGetChildCount(mflistbox) / 3;
            }
            else
            {
                count = itcTreeGetChildCount(mflistbox);
            }

            listbox->pageIndex = mflistbox->playIndex / count + 1;
            ituListBoxOnLoadPage(listbox, listbox->pageIndex);
            ituWidgetUpdate(listbox, ITU_EVENT_LAYOUT, 0, 0, 0);
            ituListBoxSelect(listbox, mflistbox->playIndex % count);

            snprintf(buf, sizeof(buf), "%d %d", listbox->pageIndex, listbox->pageCount);
            ituExecActions((ITUWidget *)listbox, listbox->actions, ITU_EVENT_LOAD, (int)buf);

            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
    }
    else
    {
        if (listbox->focusIndex >= 0)
        {
            int count;

            if (widget->type == ITU_SCROLLMEDIAFILELISTBOX)
            {
                count = itcTreeGetChildCount(mflistbox) / 3;
            }
            else
            {
                count = itcTreeGetChildCount(mflistbox);
            }

            mflistbox->playIndex = (listbox->pageIndex - 1) * count + listbox->focusIndex;
        }
        else
        {
            char buf[32];

            mflistbox->playIndex = 0;
            listbox->pageIndex   = 1;
            ituListBoxOnLoadPage(listbox, listbox->pageIndex);
            ituWidgetUpdate(listbox, ITU_EVENT_LAYOUT, 0, 0, 0);
            ituListBoxSelect(listbox, 0);

            snprintf(buf, sizeof(buf), "%d %d", listbox->pageIndex, listbox->pageCount);
            ituExecActions((ITUWidget *)listbox, listbox->actions, ITU_EVENT_LOAD, (int)buf);
        }
        return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
    }
    return NULL;
}

ITUScrollText * ituMediaFileListPrev (ITUMediaFileListBox * mflistbox)
{
    const ITUWidget * widget  = (ITUWidget *)mflistbox;
    ITUListBox *      listbox = (ITUListBox *)mflistbox;
    assert(mflistbox);
    ITU_ASSERT_THREAD();

    if (listbox->focusIndex == -1)
    {
        return ituMediaFileListPlay(mflistbox);
    }

    if ((mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING) || (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING) ||
        !mflistbox->rbtree)
    {
        return NULL;
    }

    if (mflistbox->randomPlay)
    {
        if (mflistbox->randomPlayedArray == NULL)
        {
            return ituMediaFileListPlay(mflistbox);
        }

        if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_ONCE || mflistbox->randomPlayedCount <= 1)
        {
            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
        else if (mflistbox->randomPlayedCount > 1)
        {
            int i, index = mflistbox->randomPlayedCount - 2;

            for (i = 0; i < mflistbox->fileCount; i++)
            {
                if (mflistbox->randomPlayedArray[i] == index)
                {
                    mflistbox->randomPlayedArray[mflistbox->playIndex] = -1;
                    mflistbox->randomPlayedCount--;
                    mflistbox->playIndex = i;
                    break;
                }
            }

            if (i < mflistbox->fileCount)
            {
                char buf[32];
                int  count;

                if (widget->type == ITU_SCROLLMEDIAFILELISTBOX)
                {
                    count = itcTreeGetChildCount(mflistbox) / 3;
                }
                else
                {
                    count = itcTreeGetChildCount(mflistbox);
                }

                listbox->pageIndex = mflistbox->playIndex / count + 1;
                ituListBoxOnLoadPage(listbox, listbox->pageIndex);
                ituWidgetUpdate(listbox, ITU_EVENT_LAYOUT, 0, 0, 0);
                ituListBoxSelect(listbox, mflistbox->playIndex % count);

                snprintf(buf, sizeof(buf), "%d %d", listbox->pageIndex, listbox->pageCount);
                ituExecActions((ITUWidget *)listbox, listbox->actions, ITU_EVENT_LOAD, (int)buf);

                return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
            }
        }
    }
    else
    {
        int count;

        if (widget->type == ITU_SCROLLMEDIAFILELISTBOX)
        {
            count = itcTreeGetChildCount(mflistbox) / 3;
        }
        else
        {
            count = itcTreeGetChildCount(mflistbox);
        }

        if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_NONE)
        {
            if (listbox->focusIndex == 0 && listbox->pageIndex == 1)
            {
                return NULL;
            }

            ituListBoxPrev(mflistbox);

            mflistbox->playIndex = (listbox->pageIndex - 1) * count + listbox->focusIndex;

            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
        else if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_ONCE)
        {
            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
        else if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_ALL)
        {
            ituListBoxPrev(mflistbox);

            mflistbox->playIndex = (listbox->pageIndex - 1) * count + listbox->focusIndex;

            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
    }
    return NULL;
}

ITUScrollText * ituMediaFileListNext (ITUMediaFileListBox * mflistbox)
{
    const ITUWidget * widget  = (ITUWidget *)mflistbox;
    ITUListBox *      listbox = (ITUListBox *)mflistbox;
    assert(mflistbox);
    ITU_ASSERT_THREAD();

    if (listbox->focusIndex == -1)
    {
        return ituMediaFileListPlay(mflistbox);
    }

    if ((mflistbox->mflistboxFlags & ITU_FILELIST_BUSYING) || (mflistbox->mflistboxFlags & ITU_FILELIST_DESTROYING) ||
        !mflistbox->rbtree)
    {
        return NULL;
    }

    if (mflistbox->randomPlay)
    {
        if (mflistbox->randomPlayedArray == NULL)
        {
            return ituMediaFileListPlay(mflistbox);
        }

        if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_ONCE)
        {
            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
        else
        {
            int tryCount = mflistbox->fileCount * 2;

            do
            {
                if (--tryCount <= 0)
                {
                    break;
                }

                mflistbox->playIndex = rand() % mflistbox->fileCount;
            } while (mflistbox->randomPlayedArray[mflistbox->playIndex] >= 0);

            if (tryCount <= 0)
            {
                int i = 0;
                for (; i < mflistbox->fileCount; i++)
                {
                    if (mflistbox->randomPlayedArray[i] == -1)
                    {
                        mflistbox->playIndex = i;
                        break;
                    }
                }

                if (i == mflistbox->fileCount)
                {
                    if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_NONE)
                    {
                        return NULL;
                    }
                    else if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_ALL)
                    {
                        return ituMediaFileListPlay(mflistbox);
                    }
                }
            }

            if (mflistbox->playIndex < mflistbox->fileCount)
            {
                char buf[32];
                int  count;

                mflistbox->randomPlayedArray[mflistbox->playIndex] = mflistbox->randomPlayedCount;
                mflistbox->randomPlayedCount++;

                if (widget->type == ITU_SCROLLMEDIAFILELISTBOX)
                {
                    count = itcTreeGetChildCount(mflistbox) / 3;
                }
                else
                {
                    count = itcTreeGetChildCount(mflistbox);
                }

                listbox->pageIndex = mflistbox->playIndex / count + 1;
                ituListBoxOnLoadPage(listbox, listbox->pageIndex);
                ituWidgetUpdate(listbox, ITU_EVENT_LAYOUT, 0, 0, 0);
                ituListBoxSelect(listbox, mflistbox->playIndex % count);

                snprintf(buf, sizeof(buf), "%d %d", listbox->pageIndex, listbox->pageCount);
                ituExecActions((ITUWidget *)listbox, listbox->actions, ITU_EVENT_LOAD, (int)buf);

                return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
            }
        }
    }
    else
    {
        int count;

        if (widget->type == ITU_SCROLLMEDIAFILELISTBOX)
        {
            count = itcTreeGetChildCount(mflistbox) / 3;
        }
        else
        {
            count = itcTreeGetChildCount(mflistbox);
        }

        if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_NONE)
        {
            if (listbox->focusIndex == listbox->itemCount - 1 && listbox->pageIndex == listbox->pageCount)
            {
                return NULL;
            }

            ituListBoxNext(mflistbox);

            mflistbox->playIndex = (listbox->pageIndex - 1) * count + listbox->focusIndex;

            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
        else if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_ONCE)
        {
            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
        else if (mflistbox->repeatMode == ITU_MEDIA_REPEAT_ALL)
        {
            ituListBoxNext(mflistbox);

            mflistbox->playIndex = (listbox->pageIndex - 1) * count + listbox->focusIndex;

            return (ITUScrollText *)ituListBoxGetFocusItem(listbox);
        }
    }
    return NULL;
}
