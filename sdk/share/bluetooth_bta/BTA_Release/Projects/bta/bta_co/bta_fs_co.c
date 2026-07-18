/*****************************************************************************
**
**  Name:           bta_fs_co.c
**
**  Description:    This file contains the file system call-out
**                  function implementation for Insight.
**
**  Copyright (c) 2003-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <errno.h>
#include <stdlib.h>
//#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#include "gki.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"

#define ALIGN32(value)     (((value) + (0x1f)) & ~(0x1f))

extern UINT8 appl_trace_level;
#include <stdio.h>
#include "ite/itp.h"

static UINT8 *pPbLeft = NULL;
static UINT16 pbLeftSize = 0;

void freeMemThorough(void *target, UINT16 *size)
{
	if (!target)
	{
		printf("target content is NULL\n");
		return;
	}
	memset(target, 0, *size);
	free(target);
	target = NULL;
	*size = 0;
}

struct node{
	UINT8 *data;
	UINT16 dataSize;
	UINT16 hash;
	struct node *next;
};
typedef struct node Node;

Node *gpHashNodeStart = NULL;
//#define HF_DBG
void node_add(Node **head, UINT8 *name, UINT16 nameSize, UINT16 hash)
{
	Node **indirect = head;

	Node *newNode = (Node*)malloc(sizeof(Node));
	newNode->data = (UINT8*)malloc(sizeof(UINT8) * nameSize);

	memset(newNode->data, 0, nameSize);
	memcpy(newNode->data, name, nameSize - 1); // set pb data

	newNode->dataSize = nameSize;
	newNode->hash = hash;
	newNode->next = NULL;
#ifdef HF_DBG
	printf("###Name: %s\n", newNode->data);
#endif
	while (*indirect)
		indirect = &((*indirect)->next);

	*indirect = newNode;
}

void node_del_all(Node **head)
{
	Node* next;

	while (*head != NULL)
	{
		next = (*head)->next;

		freeMemThorough((*head)->data, &((*head)->dataSize));

		free(*head);
		*head = next;
	}
}

/*****************************************************************************
**  Function Declarations
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_fs_convert_oflags
**
** Description      This function converts the open flags from BTA into MFS.
**
** Returns          BTA FS status value.
**
*******************************************************************************/
int bta_fs_convert_bta_oflags(int bta_oflags)
{
    int oflags = O_BINARY; /* Initially read only */

    /* Only one of these can be set: Read Only, Read/Write, or Write Only */
    if (bta_oflags & BTA_FS_O_RDWR)
        oflags |= O_RDWR;
    else if (bta_oflags & BTA_FS_O_WRONLY)
        oflags |= O_WRONLY;

    /* OR in any other flags that are set by BTA */
    if (bta_oflags & BTA_FS_O_CREAT)
        oflags |= O_CREAT;

    if (bta_oflags & BTA_FS_O_EXCL)
        oflags |= O_EXCL;

    if (bta_oflags & BTA_FS_O_TRUNC)
        oflags |= O_TRUNC;

    return (oflags);
}

/*******************************************************************************
**
** Function         bta_fs_co_open
**
** Description      This function is executed by BTA when a file is opened.
**                  The phone uses this function to open
**                  a file for reading or writing.
**
** Parameters       p_path  - Fully qualified path and file name.
**                  oflags  - permissions and mode (see constants above)
**                  size    - size of file to put (0 if unavailable or not applicable)
**                  evt     - event that must be passed into the call-in function.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, a file descriptor (int),
**                        if successful, and an error code (tBTA_FS_CO_STATUS)
**                        are returned in the call-in function, bta_fs_ci_open().
**
*******************************************************************************/
void bta_fs_co_open(const char *p_path, int oflags, UINT32 size, UINT16 evt,
                    UINT8 app_id)
{
    tBTA_FS_CO_STATUS  status = BTA_FS_CO_OK;
    UINT32  file_size = 0;
    struct  stat file_stat;
	int fd, err = 0;

	// free gpHashNodeStart
	node_del_all(&gpHashNodeStart);

    APPL_TRACE_DEBUG4("[CO] bta_fs_co_open: handle:%d err:%d, flags:%x, app id:%d",
                      fd, err, oflags, app_id);
    APPL_TRACE_DEBUG1("file=%s", p_path);

	bta_fs_ci_open(fd, status, file_size, evt);
}

/*******************************************************************************
**
** Function         bta_fs_co_close
**
** Description      This function is called by BTA when a connection to a
**                  client is closed.
**
** Parameters       fd      - file descriptor of file to close.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                      [BTA_FS_CO_OK if successful],
**                      [BTA_FS_CO_FAIL if failed  ]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_close(int fd, UINT8 app_id)
{
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;
    int err;

	if (pbLeftSize)
		freeMemThorough(pPbLeft, &pbLeftSize);

	APPL_TRACE_DEBUG2("[CO] bta_fs_co_close: handle:%d, app id:%d", fd, app_id);
    /*if ((status = fclose(fd)) < 0)
    {
        err = errno;
        status = BTA_FS_CO_FAIL;
        APPL_TRACE_WARNING3("[CO] bta_fs_co_close: handle:%d error=%d app_id:%d", fd, err, app_id);
    }*/

    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_read
**
** Description      This function is called by BTA to read in data from the
**                  previously opened file on the phone.
**
** Parameters       fd      - file descriptor of file to read from.
**                  p_buf   - buffer to read the data into.
**                  nbytes  - number of bytes to read into the buffer.
**                  evt     - event that must be passed into the call-in function.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, bta_fs_ci_read() is
**                        called with the buffer of data, along with the number
**                        of bytes read into the buffer, and a status.  The
**                        call-in function should only be called when ALL requested
**                        bytes have been read, the end of file has been detected,
**                        or an error has occurred.
**
*******************************************************************************/
void bta_fs_co_read(int fd, UINT8 *p_buf, UINT16 nbytes, UINT16 evt, UINT8 ssn, UINT8 app_id)
{
    tBTA_FS_CO_STATUS  status = BTA_FS_CO_OK;
    INT32   num_read;
    int     err;

	bta_fs_ci_read(fd, (UINT16)nbytes, status, evt);
}

/*******************************************************************************
**
** Function         bta_fs_co_write
**
** Description      This function is called by io to send file data to the
**                  phone.
**
** Parameters       fd      - file descriptor of file to write to.
**                  p_buf   - buffer to read the data from.
**                  nbytes  - number of bytes to write out to the file.
**                  evt     - event that must be passed into the call-in function.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, bta_fs_ci_write() is
**                        called with the file descriptor and the status.  The
**                        call-in function should only be called when ALL requested
**                        bytes have been written, or an error has been detected,
**
*******************************************************************************/
void bta_fs_co_write(int fd, const UINT8 *p_buf, UINT16 nbytes, UINT16 evt,
                     UINT8 ssn, UINT8 app_id)
{
    tBTA_FS_CO_STATUS  status = BTA_FS_CO_OK;
    INT32   num_written = 0;
    int     err=0;

	UINT8 *pPbStart, *pPbEnd, *pName, *pHash = NULL;
	UINT8 *pWriteData = NULL, *_p_buf;
	UINT16 i = 0, sLen, hash, _nbytes = nbytes + 1;// +1 for make totalSize end with 0
	UINT16 totalSize = pbLeftSize + _nbytes;

	_nbytes = ALIGN32(_nbytes);
	totalSize = ALIGN32(totalSize);

	_p_buf = (UINT8 *)malloc(sizeof(UINT8) * _nbytes); // +1 make sure _p_buf end with 0
	memcpy(_p_buf, p_buf, nbytes);

	pWriteData = (UINT8 *)malloc(sizeof(UINT8) * totalSize);
	memset(pWriteData, 0, totalSize);
	
	if (pbLeftSize)
	{
		memcpy(pWriteData, pPbLeft, pbLeftSize); // copy last time left pb data
		strlcat(pWriteData, _p_buf, totalSize); // copy new data behind last data

		freeMemThorough(pPbLeft, &pbLeftSize);
	}
	else
		memcpy(pWriteData, _p_buf, _nbytes);

	freeMemThorough(_p_buf, &_nbytes);
	
	pPbStart = pWriteData;

	while (pPbStart = strstr(pPbStart, "FN:"))
	{
		pPbEnd = pPbStart;
		if (pPbEnd = strstr(pPbStart, "END:"))
		{
			pName = pPbStart + 3; // set name start
			pPbEnd = strstr(pPbStart, "\r\n");
			sLen = pPbEnd - pName + 1;

			pPbStart = strstr(pPbStart, "TEL");
			pHash = strstr(pPbStart, ":") + 1;
			pPbEnd = strstr(pPbStart, "\r\n");

			hash = 0;
			for (i = 0; i < (pPbEnd - pHash); i++)
				hash = *(pHash + i) + (hash << 6) + (hash << 16) - hash;

			node_add(&gpHashNodeStart, pName, sLen, hash);
		}
		else 
			break;
	}

	// save _p_buf left
	if (!pHash) // in case there's no any "FN:" could been found
		pHash = pPbStart;

	pbLeftSize = totalSize - (UINT16)(pHash - pWriteData);

	pbLeftSize = ALIGN32(pbLeftSize);

	pPbLeft = (UINT8 *)malloc(sizeof(UINT8) * pbLeftSize);

	memset(pPbLeft, 0, pbLeftSize);
	strcpy(pPbLeft, pHash);
	// save _p_buf left

	freeMemThorough(pWriteData, &totalSize);

	APPL_TRACE_DEBUG3("[CO] bta_fs_co_write: handle:%d error=%d, num_written:%d", fd, err, num_written);

	bta_fs_ci_write(fd, status, evt);
}

/*******************************************************************************
**
** Function         bta_fs_co_seek
**
** Description      This function is called by io to move the file pointer
**                  of a previously opened file to the specified location for
**                  the next read or write operation.
**
** Parameters       fd      - file descriptor of file.
**                  offset  - Number of bytes from origin.
**                  origin  - Initial position.
**
** Returns          void
**
*******************************************************************************/
void bta_fs_co_seek (int fd, INT32 offset, INT16 origin, UINT8 app_id)
{
    lseek(fd, offset, origin);
}

/*******************************************************************************
**
** Function         bta_fs_co_access
**
** Description      This function is called to check the existence of
**                  a file or directory, and return whether or not it is a
**                  directory or length of the file.
**
** Parameters       p_path   - (input) file or directory to access (fully qualified path).
**                  mode     - (input) [BTA_FS_ACC_EXIST, BTA_FS_ACC_READ, or BTA_FS_ACC_RDWR]
**                  p_is_dir - (output) returns TRUE if p_path specifies a directory.
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                   [BTA_FS_CO_OK if it exists]
**                   [BTA_FS_CO_EACCES if permissions are wrong]
**                   [BTA_FS_CO_FAIL if it does not exist]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_access(const char *p_path, int mode, BOOLEAN *p_is_dir,
                                   UINT8 app_id)
{
    int err;
    int os_mode = 0;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;
    struct stat buffer;

    *p_is_dir = FALSE;

    if (mode == BTA_FS_ACC_RDWR)
        os_mode = 6;
    else if (mode == BTA_FS_ACC_READ)
        os_mode = 4;

    if ((access (p_path, os_mode)) == 0)
    {
        /* Determine if the object is a file or directory */
        //if (_stat(p_path, &buffer) == 0)
		if (stat(p_path, &buffer) == 0)
            if (S_ISDIR(buffer.st_mode))
                *p_is_dir = TRUE;
    }
    else
    {
        err = errno;
        status = (err == EACCES) ? BTA_FS_CO_EACCES : BTA_FS_CO_FAIL;
    }

    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_mkdir
**
** Description      This function is called to create a directory with
**                  the pathname given by path. The pathname is a null terminated
**                  string. All components of the path must already exist.
**
** Parameters       p_path   - (input) name of directory to create (fully qualified path).
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                  [BTA_FS_CO_OK if successful]
**                  [BTA_FS_CO_FAIL if unsuccessful]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_mkdir(const char *p_path, UINT8 app_id)
{
    int err;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;

    if ((mkdir (p_path, 0666)) != 0)
    {
        err = errno;
        status = BTA_FS_CO_FAIL;
        APPL_TRACE_WARNING3("[CO] bta_fs_co_mkdir: error=%d, path [%s] app_id:%d",
                            err, p_path, app_id);
    }
    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_rmdir
**
** Description      This function is called to remove a directory whose
**                  name is given by path. The directory must be empty.
**
** Parameters       p_path   - (input) name of directory to remove (fully qualified path).
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                      [BTA_FS_CO_OK if successful]
**                      [BTA_FS_CO_EACCES if read-only]
**                      [BTA_FS_CO_ENOTEMPTY if directory is not empty]
**                      [BTA_FS_CO_FAIL otherwise]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_rmdir(const char *p_path, UINT8 app_id)
{
    int     err;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;

    if ((rmdir (p_path)) != 0)
    {
        err = errno;
        if (err == EACCES)
            status = BTA_FS_CO_EACCES;
        else if (err == ENOTEMPTY)
            status = BTA_FS_CO_ENOTEMPTY;
        else
            status = BTA_FS_CO_FAIL;
    }
    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_unlink
**
** Description      This function is called to remove a file whose name
**                  is given by p_path.
**
** Parameters       p_path   - (input) name of file to remove (fully qualified path).
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                      [BTA_FS_CO_OK if successful]
**                      [BTA_FS_CO_EACCES if read-only]
**                      [BTA_FS_CO_FAIL otherwise]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_unlink(const char *p_path, UINT8 app_id)
{
    int     err;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;

    if ((unlink (p_path)) != 0)
    {
        err = errno;
        if (err == EACCES)
            status = BTA_FS_CO_EACCES;
        else
            status = BTA_FS_CO_FAIL;
    }
    return (status);

}

/*******************************************************************************
**
** Function         bta_fs_co_getdirentry
**
** Description      This function is called to get a directory entry for the
**                  specified p_path.  The first/next directory should be filled
**                  into the location specified by p_entry.
**
** Parameters       p_path      - directory to search (Fully qualified path)
**                  first_item  - TRUE if first search, FALSE if next search
**                                      (p_cur contains previous)
**                  p_entry (input/output) - Points to last entry data (valid when
**                                           first_item is FALSE)
**                  evt     - event that must be passed into the call-in function.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, the status is passed
**                        in the bta_fs_ci_direntry() call-in function.
**                        BTA_FS_CO_OK is returned when p_entry is valid,
**                        BTA_FS_CO_EODIR is returned when no more entries [finished]
**                        BTA_FS_CO_FAIL is returned if an error occurred
**
*******************************************************************************/
void bta_fs_co_getdirentry(const char *p_path, BOOLEAN first_item,
                           tBTA_FS_DIRENTRY *p_entry, UINT16 evt, UINT8 app_id)
{
    tBTA_FS_CO_STATUS    co_status = BTA_FS_CO_FAIL;
    int                  status = -1;    /* '0' - success, '-1' - fail */
    struct tm           *p_tm;
    char                *p_temppath = NULL;
    DIR *dir;
    struct dirent *dirent;
    struct stat buf;

    /* First item is to be retrieved */
    if (first_item)
    {
        /* Buffer must be large enough to hold the maximum path size for platform */
        if ((p_temppath = (char *)GKI_getbuf(GKI_MAX_BUF_SIZE)) != NULL)
        {
            sprintf(p_temppath, "%s\\*", p_path);

            dir = opendir(p_temppath);
            if( !(dirent = readdir(dir)))
            {
                p_entry->refdata = (void *) dir;     /* Save this for future searches */
                status = 0;
            }
            GKI_freebuf(p_temppath);
        }
    }
    else    /* Get the next entry based on the p_ref data from previous search */
    {
        //if (!(dirent = readdir((long)p_entry->refdata)))
		if (!(dirent = readdir((DIR*)(p_entry->refdata))))
        {
            /* Close the search if there are no more items */
            //closedir( (long) p_entry->refdata);
			closedir( (DIR*) (p_entry->refdata));
            co_status = BTA_FS_CO_EODIR;
        }
    }

    if (status == 0)
    {
        /* Load new values into the return structure (refdata is left untouched) */
        //stat(dirent, &buf);
		stat(dirent->d_name, &buf);
        p_entry->filesize = buf.st_size;
        p_entry->mode = 0; /* Default is normal read/write file access */

        if (buf.st_mode & S_IRUSR)
            p_entry->mode |= BTA_FS_A_RDONLY;
        if (S_ISDIR(buf.st_mode))
            p_entry->mode |= BTA_FS_A_DIR;

        strcpy(p_entry->p_name, dirent->d_name);

		//if ((p_tm = gmtime(buf.st_mtime)) != NULL)
        if ((p_tm = gmtime(&(buf.st_mtime))) != NULL)
        {
            sprintf(p_entry->crtime, "%04d%02d%02dT%02d%02d%02dZ",
                    p_tm->tm_year + 1900,   /* Base Year ISO 6201 */
                    p_tm->tm_mon,
                    p_tm->tm_mday,
                    p_tm->tm_hour,
                    p_tm->tm_min,
                    p_tm->tm_sec);
        }
        else
            p_entry->crtime[0] = '\0';  /* No valid time */

        co_status = BTA_FS_CO_OK;
    }

    bta_fs_ci_direntry(co_status, evt);
}
