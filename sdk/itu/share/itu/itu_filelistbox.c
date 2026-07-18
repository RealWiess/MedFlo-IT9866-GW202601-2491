#include <assert.h>
#include <dirent.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "redblack/redblack.h"
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char flistboxName[] = "ITUFileListBox";

static void FileListDestroy(ITUFileListBox* flistbox)
{
    assert(flistbox);

    if (flistbox->rbtree)
    {
        if (rb_check(flistbox->rbtree))
        {
            if (rb_count(flistbox->rbtree) > 0)
            {
                rb_destroy(flistbox->rbtree, 0, 1);
            }
        }
        flistbox->rbtree = NULL;
    }
}

static void* FileListBoxCreateTask(void* arg)
{
    DIR *            pdir     = NULL;
    ITUFileListBox * flistbox = (ITUFileListBox *)arg;
    assert(flistbox);

    if (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING)
    {
        flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
        return NULL;
    }

    if (strlen(flistbox->path) == 0)
    {
        ITU_LOG_WARN("empty path\n");
        flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
        return NULL;
    }

    FileListDestroy(flistbox);

    flistbox->rbtree = (void *)rb_create();
    if (flistbox->rbtree == NULL)
    {
        ITU_LOG_ERR("insufficient memory\n");
        flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
        return NULL;
    }

    if (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING)
    {
        flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
        return NULL;
    }

    // Open the directory
    pdir = opendir(flistbox->path);
    if (pdir != NULL)
    {
        struct dirent * pent;

        if (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING)
        {
            closedir(pdir);
            flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
            return NULL;
        }

        flistbox->fileCount = 0;

        // List the contents of the directory
        while ((pent = readdir(pdir)) != NULL)
        {
            struct dirent* dir = NULL;
            struct dirent* ret = NULL;
            Redblack rb = NULL;

            if (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING)
            {
                closedir(pdir);
                flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
                return NULL;
            }

            if ((strcmp(pent->d_name, ".") == 0) || (strcmp(pent->d_name, "..") == 0))
            {
                continue;
            }

            dir = malloc(sizeof(struct dirent));
            if (dir == NULL)
            {
                ITU_LOG_ERR("out of memory: %d\n", (int)sizeof(struct dirent));
                closedir(pdir);
                flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
                return NULL;
            }

            if (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING)
            {
                closedir(pdir);
                flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
                return NULL;
            }

            (void)memcpy(dir, pent, sizeof(struct dirent));
            rb = rb_insert_int(flistbox->rbtree, flistbox->fileCount, (void*)dir);
            if (rb != NULL)
            {
                ret = (struct dirent*)(rb->item);
            }
            if (ret == NULL)
            {
                ITU_LOG_ERR("out of memory: %d\n", (int)sizeof(struct dirent));
                free(dir);
                closedir(pdir);
                flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
                return NULL;
            }

            flistbox->fileCount++;

            if (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING)
            {
                closedir(pdir);
                flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
                return NULL;
            }
        }
        flistbox->flistboxFlags |= (unsigned int)ITU_FILELIST_CREATED;
    }
    else
    {
        ITU_LOG_WARN("opendir(%s) failed\n", flistbox->path);
        flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
        return NULL;
    }

    // Close the directory (final check again)
    if (pdir != NULL)
    {
        closedir(pdir);
    }

    flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_BUSYING;
    return NULL;
}

void ituFileListBoxExit(ITUWidget* widget)
{
    ITUFileListBox* flistbox = (ITUFileListBox*) widget;
    int max_loop = 0;
    assert(flistbox);
    ITU_ASSERT_THREAD();

    if (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_BUSYING)
    {
        flistbox->flistboxFlags |= (unsigned int)ITU_FILELIST_DESTROYING;
    }

    while ((flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_BUSYING) && (max_loop < 3000))
    {
        (void)usleep(1000);
        max_loop++;
    }

    FileListDestroy(flistbox);
    ituWidgetExitImpl(widget);
}

bool ituFileListBoxUpdate (ITUWidget * widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
    bool             result;
    ITUListBox *     listbox  = (ITUListBox *)widget;
    ITUFileListBox * flistbox = (ITUFileListBox *)widget;
    assert(flistbox);

    result = ituListBoxUpdate(widget, ev, arg1, arg2, arg3);

    if ((ev == ITU_EVENT_TIMER) && (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_CREATED))
    {
        ITCTree * node = widget->tree.child;
        int       count;
        Redblack  rb = NULL;

        flistbox->flistboxFlags &= ~(unsigned int)ITU_FILELIST_CREATED;

        if (node == NULL)
        {
            return result;
        }

        for (rb = rb_first(flistbox->rbtree); rb != flistbox->rbtree; rb = rb_next(rb))
        {
            ITUScrollText * scrolltext = (ITUScrollText *)node;
            struct dirent * val        = (struct dirent *)rb->item;

            if (val)
            {
                if (scrolltext)
                {
                    if (val->d_type == DT_DIR)
                    {
                        char buf[NAME_MAX + 10];
                        strlcpy(buf, "<", sizeof(buf));
                        strlcat(buf, val->d_name, sizeof(buf));
                        strlcat(buf, ">", sizeof(buf));
                        ituScrollTextSetString(scrolltext, buf);
                    }
                    else
                    {
                        ituScrollTextSetString(scrolltext, val->d_name);
                    }
                }
            }

            node = node->sibling;

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

            if (flistbox->fileCount == 0)
            {
                listbox->pageIndex = listbox->pageCount = 0;
            }
            else
            {
                listbox->pageIndex = 1;

                if (flistbox->fileCount <= count)
                {
                    listbox->pageCount = 1;
                }
                else
                {
                    listbox->pageCount = flistbox->fileCount / count + 1;
                }
            }
            if (listbox->pageIndex == listbox->pageCount)
            {
                listbox->itemCount = flistbox->fileCount % count;
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

            snprintf(buf, sizeof(buf), "%d %d", listbox->pageIndex, listbox->pageCount);
            ituExecActions((ITUWidget*)listbox, listbox->actions, ITU_EVENT_LOAD, (int)buf);
        }
        result = widget->dirty = true;
    }
    return widget->visible ? result : false;
}

static void FileListBoxBackPage(ITUFileListBox* flistbox)
{
    ITUListBox* listbox = (ITUListBox*) flistbox;
    int len;
    char* ptr;
    pthread_t task;
    pthread_attr_t attr;
    assert(flistbox);

    if ((flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_BUSYING) || (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING))
    {
        return;
    }

    len = strlen(flistbox->path);
    if (len <= 3)
    {
        return;
    }

    if (flistbox->path[len - 1] == '/')
    {
        flistbox->path[len - 1] = '\0';
    }

    ptr = strrchr(flistbox->path, '/');
    if (ptr)
    {
        *ptr = '\0';
    }

    ituListBoxSelect(listbox, -1);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&task, &attr, FileListBoxCreateTask, flistbox);
    flistbox->flistboxFlags |= (unsigned int)ITU_FILELIST_BUSYING;
}

void ituFileListBoxOnAction (ITUWidget * widget, ITUActionType action, char * param)
{
    assert(widget);

    switch (action)
    {
    case ITU_ACTION_BACK:
        FileListBoxBackPage((ITUFileListBox*)widget);
        break;

    default:
        ituListBoxOnAction(widget, action, param);
        break;
    }
}

void ituFileListBoxOnLoadPage(ITUListBox* listbox, int pageIndex)
{
    ITUFileListBox* flistbox = (ITUFileListBox*) listbox;
    ITCTree* node;
    struct dirent* val;
    Redblack rb = NULL;
    int i, count;
    assert(flistbox);
    assert(pageIndex <= listbox->pageCount);

    if ((flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_BUSYING) || (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING) ||
        !flistbox->rbtree)
    {
    return;
    }

    count = itcTreeGetChildCount(listbox);

    if ((pageIndex == listbox->pageCount) && (count > 0))
    {
        listbox->itemCount = flistbox->fileCount % count;
        if (listbox->itemCount == 0)
        {
            listbox->itemCount = count;
        }
    }
    else
    {
        listbox->itemCount = count;
    }

    if ((flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_BUSYING) || (count == 0))
    {
        return;
    }

    node = ((ITCTree*)listbox)->child;
    i = 0;
    for (rb = rb_first(flistbox->rbtree); rb != flistbox->rbtree; rb = rb_next(rb))
    {
        ITUScrollText * scrolltext = (ITUScrollText *)node;

        if (i++ < count * (pageIndex - 1))
        {
            continue;
        }

        val = (struct dirent*)rb->item;
        if (val)
        {
            if (scrolltext)
            {
                if (val->d_type == DT_DIR)
                {
                    char buf[NAME_MAX + 10];
                    strlcpy(buf, "<", sizeof(buf));
                    strlcat(buf, val->d_name, sizeof(buf));
                    strlcat(buf, ">", sizeof(buf));
                    ituScrollTextSetString(scrolltext, buf);
                }
                else
                {
                    ituScrollTextSetString(scrolltext, val->d_name);
                }
            }
        }

        node = node->sibling;

        if (node == NULL)
        {
            break;
        }
    }

    for (; node; node = node->sibling)
    {
        ITUScrollText* scrolltext = (ITUScrollText*) node;
        ituScrollTextSetString(scrolltext, "");
    }
    listbox->pageIndex = pageIndex;
}

void ituFileListOnSelection(ITUListBox* listbox, ITUScrollText* item, bool confirm)
{
    pthread_t        task;
    pthread_attr_t   attr;
    ITUFileListBox * flistbox = (ITUFileListBox *)listbox;
    assert(listbox);

    if (!confirm)
    {
        return;
    }

    if ((flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_BUSYING) || (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING))
    {
        return;
    }

    if (item->text.string[0] != '<')
    {
        return;
    }

    if (flistbox->path[strlen(flistbox->path) - 1] != '/')
    {
        strlcat(flistbox->path, "/", sizeof(flistbox->path));
    }

    strlcat(flistbox->path, &item->text.string[1], sizeof(flistbox->path));
    flistbox->path[strlen(flistbox->path) - 1] = '\0';

    ituListBoxSelect(listbox, -1);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&task, &attr, FileListBoxCreateTask, flistbox);
    flistbox->flistboxFlags |= (unsigned int)ITU_FILELIST_BUSYING;
}

void ituFileListBoxInit(ITUFileListBox* flistbox, int width, char* path)
{
    assert(flistbox);
    ITU_ASSERT_THREAD();

    (void)memset(flistbox, 0, sizeof (ITUFileListBox));

    ituListBoxInit(&flistbox->listbox, width);

    ituWidgetSetType(flistbox, ITU_FILELISTBOX);
    ituWidgetSetName(flistbox, flistboxName);
    ituWidgetSetExit(flistbox, ituFileListBoxExit);
    ituWidgetSetUpdate(flistbox, ituFileListBoxUpdate);
    ituWidgetSetOnAction(flistbox, ituFileListBoxOnAction);
    ituListBoxSetOnLoadPage(flistbox, ituFileListBoxOnLoadPage);
    ituListBoxSetOnSelection(flistbox, ituFileListOnSelection);

    if (path)
    {
        pthread_t task;
        pthread_attr_t attr;

        strlcpy(flistbox->path, path, sizeof(flistbox->path));

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&task, &attr, FileListBoxCreateTask, flistbox);
        flistbox->flistboxFlags |= (unsigned int)ITU_FILELIST_BUSYING;
    }
}
#define FILELIST_TASK_STACK_SIZE    (12 * 1024)
void ituFileListBoxLoad(ITUFileListBox* flistbox, uint32_t base)
{
    assert(flistbox);

    ituListBoxLoad(&flistbox->listbox, base);

    ituWidgetSetExit(flistbox, ituFileListBoxExit);
    ituWidgetSetUpdate(flistbox, ituFileListBoxUpdate);
    ituWidgetSetOnAction(flistbox, ituFileListBoxOnAction);
    ituListBoxSetOnLoadPage(flistbox, ituFileListBoxOnLoadPage);
    ituListBoxSetOnSelection(flistbox, ituFileListOnSelection);

    if (strlen(flistbox->path) > 0)
    {
        pthread_t task;
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, FILELIST_TASK_STACK_SIZE);
        pthread_create(&task, &attr, FileListBoxCreateTask, flistbox);
        flistbox->flistboxFlags |= (unsigned int)ITU_FILELIST_BUSYING;
    }
}

void ituFileListSetPath(ITUFileListBox* flistbox, char* path)
{
    int len;
    pthread_t task;
    pthread_attr_t attr;
    assert(flistbox);
    ITU_ASSERT_THREAD();

    if ((flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_BUSYING) || (flistbox->flistboxFlags & (unsigned int)ITU_FILELIST_DESTROYING))
    {
        return;
    }

    if (path)
    {
        strlcpy(flistbox->path, path, sizeof(flistbox->path));
    }
    else
    {
        flistbox->path[0] = '\0';
    }

    len = strlen(flistbox->path);
    if (len < 2)
    {
        return;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&task, &attr, FileListBoxCreateTask, flistbox);
    flistbox->flistboxFlags |= (unsigned int)ITU_FILELIST_BUSYING;
}
