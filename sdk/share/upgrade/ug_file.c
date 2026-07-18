#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include "ite/ug.h"
#include "ug_cfg.h"

#define PASUCLSTR "ucl-"
#define PASDIFFSTR "ucl-diff-"

#pragma pack(1)
typedef struct
{
    uint32_t filename_size;
} file_t;

static const uint32_t enckey = CFG_UPGRADE_ENC_KEY;

static size_t enc_fread(void *buffer, size_t size, size_t count, ITCStream *stream)
{
    if (enckey == 0)
    {
        return itcStreamRead((ITCStream*)stream, buffer, size * count);
    }
    else
    {
        uint8_t* ptr = (uint8_t*)buffer;
        size_t ret, i, bufsize, alignsize;
        uint32_t key32;
        uint8_t* buf;

        bufsize = size * count;

        buf = malloc(bufsize);
        if (buf == NULL)
        {
            UG_LOG_ERR("Out of memory: %ld\n", bufsize);
            return 0;
        }

        alignsize = bufsize & ~(sizeof(uint32_t) - 1);

        key32 = (((uint32_t) enckey) << 24) |
            (((uint32_t) enckey) << 16) |
            (((uint32_t) enckey) << 8) |
            ((uint32_t) enckey);

        ret = itcStreamRead((ITCStream*)stream, buf, size * count);

        for (i = 0; i < alignsize; i += sizeof(uint32_t))
            *(uint32_t*)&ptr[i] = *(uint32_t*)&buf[i] ^ key32;

        for (; i < bufsize; i++)
            ptr[i] = buf[i] ^ enckey;

        free(buf);

        return ret;
    }
}

int ugUpgradeFile(ITCStream *f, char* drive)
{
    int ret = 0;
    file_t file;
    char filename[PATH_MAX];
    char* buf = NULL;
    char* buf2 = NULL;
    uint32_t filesize, bufsize, readsize, size2, remainsize;
    FILE* outfile = NULL;
    struct statvfs info;

    // read file
    readsize = itcStreamRead((ITCStream*)f, &file, sizeof(file_t));
    if (readsize != sizeof(file_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(file_t));
        goto end;
    }

    file.filename_size = itpLetoh32(file.filename_size);
    UG_LOG_DBG("filename_size=%ld\n", file.filename_size);

    strlcpy(filename, drive, sizeof(filename));

    readsize = itcStreamRead((ITCStream*)f, &filename[2], file.filename_size);
    if (readsize != file.filename_size)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, file.filename_size);
        goto end;
    }

    readsize = itcStreamRead((ITCStream*)f, &filesize, sizeof (uint32_t));
    if (readsize != sizeof (uint32_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof (uint32_t));
        goto end;
    }

    filesize = itpLetoh32(filesize);

    bufsize = CFG_UG_BUF_SIZE < filesize ? CFG_UG_BUF_SIZE : filesize;
    buf = malloc(bufsize);
    if (!buf)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %ld\n", bufsize);
        goto end;
    }

    // check capacity
    if (statvfs(drive, &info) == 0)
    {
        uint64_t avail = info.f_bfree * info.f_bsize;
        FILE *   chkFile = fopen(filename, "rb");
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

    // upgrade
    outfile = fopen(filename, "wb");
    if (!outfile)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot open file %s: %d\n", filename, errno);
        goto end;
    }

    UG_LOG_INFO("[%d%%] Writing %s, %ld bytes", ugGetProrgessPercentage(), filename, filesize);

    remainsize = filesize;
    do
    {
        readsize = (remainsize < bufsize) ? remainsize : bufsize;
        size2 = enc_fread(buf, 1, readsize, f);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        size2 = fwrite(buf, 1, readsize, outfile);
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
    fclose(outfile);
    outfile = NULL;

    buf2 = malloc(bufsize);
    if (!buf2)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %ld\n", bufsize);
        goto end;
    }

    outfile = fopen(filename, "rb");
    if (!outfile)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot open file %s: %d\n", filename, errno);
        goto end;
    }

    if (itcStreamSeek((ITCStream*)f, -(long)filesize, SEEK_CUR))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot seek file %d\n", errno);
        goto end;
    }

    UG_LOG_INFO("[%d%%] Verifying", ugGetProrgessPercentage());

    remainsize = filesize;
    do
    {
        readsize = (remainsize < bufsize) ? remainsize : bufsize;
        size2 = enc_fread(buf, 1, readsize, f);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        size2 = fread(buf2, 1, readsize, outfile);
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
    }
    while (remainsize > 0);
    putchar('\n');

end:
    if (outfile)
        fclose(outfile);

    if (buf2)
        free(buf2);

    if (buf)
        free(buf);

    return ret;
}

int ugUpgradeFile_diff(ITCStream *f, char* drive)
{
    int ret = 0, pos=0;
    bool isDeleteMode = false, isAddMode = false, isDiffMode = false;
    file_t file;
    char filename[PATH_MAX], real_filename[PATH_MAX];
    char* buf = NULL;
    char* buf2 = NULL;
    uint8_t *old_buf = NULL, *new_buf = NULL, *ucl_buf = NULL, *diff_buf = NULL;
    uint32_t filesize, bufsize, readsize, size2, remainsize, crc32val = 0;;
    FILE* outfile = NULL;
    struct statvfs info;

    // read file
    readsize = itcStreamRead((ITCStream*)f, &file, sizeof(file_t));
    if (readsize != sizeof(file_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file size: %u\n", readsize);
        goto end;
    }

    file.filename_size = itpLetoh32(file.filename_size);
    UG_LOG_DBG("filename_size=%u\n", file.filename_size);

    strlcpy(filename, drive, sizeof(filename));

    readsize = itcStreamRead((ITCStream*)f, &filename[2], file.filename_size);
    if (readsize != file.filename_size)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %u != %u\n", readsize, file.filename_size);
        goto end;
    }

    //parse real file name to check remove or diff
    while(pos < strlen(filename)){
        if(filename[pos]== '~'){
            isDeleteMode = true;
            break;
        }
        if(!strncmp(&filename[pos], PASDIFFSTR, strlen(PASDIFFSTR)))
        {
            memcpy(real_filename,filename ,pos);
            snprintf(&real_filename[pos], PATH_MAX - strlen(PASDIFFSTR) - pos, "%s", &filename[pos + strlen(PASDIFFSTR)]);
            isDiffMode = true;
            UG_LOG_DBG("get real diff file name%s\n", real_filename);
            break;
        }
        else if(!strncmp(&filename[pos], PASUCLSTR, strlen(PASUCLSTR)))
        {
            memcpy(real_filename,filename ,pos);
            snprintf(&real_filename[pos], PATH_MAX - strlen(PASUCLSTR) - pos, "%s", &filename[pos + strlen(PASUCLSTR)]);
            isAddMode = true;
            UG_LOG_DBG("get real add file name%s\n", real_filename);
            break;
        }
        pos++;
    }

    if(isDeleteMode)  //remove file
    {
        //find file delete tag
        UG_LOG_DBG("pos=%d \n", pos);
        memcpy(real_filename,filename ,pos);
        snprintf(&real_filename[pos],PATH_MAX-pos,"%s", &filename[pos+1]);
        UG_LOG_INFO("delete file: %s\n", real_filename);
        readsize = itcStreamRead((ITCStream*)f, &filesize, sizeof (uint32_t));//move pos
        if (readsize != sizeof (uint32_t))
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file size: %u\n", readsize);
            goto end;
        }
        filesize = itpLetoh32(filesize);

        itcStreamSeek((ITCStream*)f,filesize,SEEK_CUR);

        if( 0 != remove(real_filename) ){
            UG_LOG_ERR("Remove Fail \n");
        }
    }
    else if(isDiffMode)  //patch from diff file
    {
        //open original file to do diff patch
        uint32_t old_size = 0, new_size = 0, diff_size = 0, check_size = 0, ucl_size = 0, ucl_pos = 0;
        outfile = fopen(real_filename, "rb");
        if (!outfile)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot open file %s: %d\n", real_filename, errno);
            goto end;
        }
        fseek(outfile, 0, SEEK_END);
        old_size = ftell(outfile);
        UG_LOG_DBG("read original file size(%u)\n", old_size);
        old_buf= malloc(old_size);
        if (!old_buf)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of memory: %lu\n", old_size);
            goto end;
        }
        fseek(outfile, 0, SEEK_SET);
        check_size = fread(old_buf, 1, old_size, outfile);
        if (check_size != old_size)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read original file: %u != %u\n", check_size, old_size);
            goto end;
        }
        fclose(outfile);
        outfile = NULL;

        //read ucl compress file size and data
        readsize = itcStreamRead((ITCStream*)f, &ucl_size, sizeof (uint32_t));
        if (readsize != sizeof (uint32_t))
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file size: %u\n", readsize);
            goto end;
        }

        ucl_size = itpLetoh32(ucl_size);

        bufsize = CFG_UG_BUF_SIZE < ucl_size ? CFG_UG_BUF_SIZE : ucl_size;
        buf = malloc(bufsize);
        if (!buf)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of memory: %u\n", bufsize);
            goto end;
        }

        ucl_buf= malloc(ucl_size);
        remainsize = ucl_size;
        do
        {
            readsize = (remainsize < bufsize) ? remainsize : bufsize;
            size2 = enc_fread(buf, 1, readsize, f);
            if (size2 != readsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %u != %u\n", size2, readsize);
                goto end;
            }
            memcpy(ucl_buf + ucl_pos, buf, readsize);
            ucl_pos += readsize;
            remainsize -= readsize;

            putchar('.');
            fflush(stdout);
        }
        while (remainsize > 0);

        UG_LOG_DBG("read ucl file size(%u)\n", ucl_size);
#ifndef _WIN32
        //to do ucl decompress and diff patch new file
        diff_buf = hw_ucl_decompress(ucl_buf, ucl_size, &diff_size);
        if (!diff_buf)
        {
            ret = __LINE__;
            UG_LOG_ERR("ucl decompress error!!!\n");
            goto end;
        }
        new_size = ugpatch_getsize(diff_buf);
        new_buf = ugpatch_getbuf(old_buf, old_size, diff_buf);
        if (!new_buf)
        {
            ret = __LINE__;
            UG_LOG_ERR("diff patch error!!!\n");
            goto end;
        }
#endif
        crc32val = (diff_buf[diff_size - 1] << 24) | (diff_buf[diff_size - 2] << 16) | (diff_buf[diff_size - 3] << 8) | diff_buf[diff_size - 4];
        printf("check CRC---0x%x(0x%x)\n", crc32val, ugCalcBufferCrc(new_buf, new_size));
        if(ugCalcBufferCrc(new_buf, new_size) != crc32val)
        {
            ret = __LINE__;
            UG_LOG_ERR("crc check error!!!\n");
            goto end;
        }
        //outfile = fopen("A:/diff_patch.bin", "wb");
        //fwrite(new_buf, 1, new_size, outfile);
        //fclose(outfile);
        UG_LOG_DBG("get new file size(%u)\n", new_size);

        //==============================

        // check capacity
        if (statvfs(drive, &info) == 0)
        {
            uint64_t avail = info.f_bfree * info.f_bsize;
            struct stat buffer;

            if (stat(real_filename, &buffer) == 0)
                avail += buffer.st_size;

            if (new_size > avail)
            {
                ret = __LINE__;
                UG_LOG_ERR("Out of space: %u > %llu\n", new_size, avail);
                goto end;
            }
        }

        // upgrade
        outfile = fopen(real_filename, "wb");
        if (!outfile)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot open file %s: %d\n", real_filename, errno);
            goto end;
        }

        UG_LOG_INFO("[%d%%] Writing %s, %u bytes", ugGetProrgessPercentage(), real_filename, new_size);

        size2 = fwrite(new_buf, 1, new_size, outfile);
        if (size2 != new_size)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %u != %u\n", size2, new_size);
            goto end;
        }
        putchar('.');
        fflush(stdout);

        putchar('\n');

        fclose(outfile);
        outfile = NULL;

        // verify
        buf2 = malloc(new_size);
        if (!buf2)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of memory: %u\n", new_size);
            goto end;
        }

        outfile = fopen(real_filename, "rb");
        if (!outfile)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot open file %s: %d\n", real_filename, errno);
            goto end;
        }

        UG_LOG_INFO("[%d%%] Verifying", ugGetProrgessPercentage());

        size2 = fread(buf2, 1, new_size, outfile);
        if (size2 != new_size)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %u != %u\n", size2, new_size);
            goto end;
        }

        if (memcmp(new_buf, buf2, new_size))
        {
            ret = __LINE__;
            UG_LOG_ERR("\nVerify failed.\n");
            goto end;
        }

        putchar('.');
        fflush(stdout);

        putchar('\n');
    }else if(isAddMode)  //do ucl decompress and add new file
    {
        uint32_t new_size = 0, ucl_size = 0, ucl_pos = 0;

        //read ucl compress file size and data
        readsize = itcStreamRead((ITCStream*)f, &ucl_size, sizeof (uint32_t));
        if (readsize != sizeof (uint32_t))
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file size: %u\n", readsize);
            goto end;
        }

        ucl_size = itpLetoh32(ucl_size);

        bufsize = CFG_UG_BUF_SIZE < ucl_size ? CFG_UG_BUF_SIZE : ucl_size;
        buf = malloc(bufsize);
        if (!buf)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of memory: %u\n", bufsize);
            goto end;
        }

        ucl_buf= malloc(ucl_size);
        remainsize = ucl_size;
        do
        {
            readsize = (remainsize < bufsize) ? remainsize : bufsize;
            size2 = enc_fread(buf, 1, readsize, f);
            if (size2 != readsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %u != %u\n", size2, readsize);
                goto end;
            }
            memcpy(ucl_buf + ucl_pos, buf, readsize);
            ucl_pos += readsize;
            remainsize -= readsize;

            putchar('.');
            fflush(stdout);
        }
        while (remainsize > 0);

        UG_LOG_DBG("read ucl file size(%u)\n", ucl_size);
#ifndef _WIN32
        //do ucl decompress to new file
        new_buf = hw_ucl_decompress(ucl_buf, ucl_size, &new_size);
        if (!new_buf)
        {
            ret = __LINE__;
            UG_LOG_ERR("ucl decompress error!!!\n");
            goto end;
        }
#endif
        //outfile = fopen("A:/diff_patch.bin", "wb");
        //fwrite(new_buf, 1, new_size, outfile);
        //fclose(outfile);
        UG_LOG_INFO("get new file size(%u)\n", new_size);

        //==============================

        // check capacity
        if (statvfs(drive, &info) == 0)
        {
            uint64_t avail = info.f_bfree * info.f_bsize;
            FILE *   chkFile = fopen(real_filename, "rb");
            if (chkFile != NULL)
            {
                if (fseek(chkFile, 0L, SEEK_END) == 0)
                {
                    avail += ftell(chkFile);
                }
                fclose(chkFile);
            }

            if (new_size > avail)
            {
                ret = __LINE__;
                UG_LOG_ERR("Out of space: %u > %llu\n", new_size, avail);
                goto end;
            }
        }

        // upgrade
        outfile = fopen(real_filename, "wb");
        if (!outfile)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot open file %s: %d\n", real_filename, errno);
            goto end;
        }

        UG_LOG_INFO("[%d%%] Writing %s, %u bytes", ugGetProrgessPercentage(), real_filename, new_size);

        size2 = fwrite(new_buf, 1, new_size, outfile);
        if (size2 != new_size)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %u != %u\n", size2, new_size);
            goto end;
        }
        putchar('.');
        fflush(stdout);

        putchar('\n');

        fclose(outfile);
        outfile = NULL;

        // verify
        buf2 = malloc(new_size);
        if (!buf2)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of memory: %u\n", new_size);
            goto end;
        }

        outfile = fopen(real_filename, "rb");
        if (!outfile)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot open file %s: %d\n", real_filename, errno);
            goto end;
        }

        UG_LOG_INFO("[%d%%] Verifying", ugGetProrgessPercentage());

        size2 = fread(buf2, 1, new_size, outfile);
        if (size2 != new_size)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %u != %u\n", size2, new_size);
            goto end;
        }

        if (memcmp(new_buf, buf2, new_size))
        {
            ret = __LINE__;
            UG_LOG_ERR("\nVerify failed.\n");
            goto end;
        }

        putchar('.');
        fflush(stdout);

        putchar('\n');
    }else
    {
        UG_LOG_INFO(">>>>>>>>>>>>> %s\n", filename);
        readsize = itcStreamRead((ITCStream*)f, &filesize, sizeof (uint32_t));
        if (readsize != sizeof (uint32_t))
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file size: %u\n", readsize);
            goto end;
        }

        filesize = itpLetoh32(filesize);

        bufsize = CFG_UG_BUF_SIZE < filesize ? CFG_UG_BUF_SIZE : filesize;
        buf = malloc(bufsize);
        if (!buf)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of memory: %u\n", bufsize);
            goto end;
        }

        // check capacity
        if (statvfs(drive, &info) == 0)
        {
            uint64_t avail = info.f_bfree * info.f_bsize;
            FILE *   chkFile = fopen(filename, "rb");
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
                UG_LOG_ERR("Out of space: %u > %llu\n", filesize, avail);
                goto end;
            }
        }

        // upgrade
        outfile = fopen(filename, "wb");
        if (outfile == NULL)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot open file %s: %d\n", filename, errno);
            goto end;
        }

        UG_LOG_INFO("[%d%%] Writing %s, %u bytes", ugGetProrgessPercentage(), filename, filesize);

        remainsize = filesize;
        do
        {
            readsize = (remainsize < bufsize) ? remainsize : bufsize;
            size2 = enc_fread(buf, 1, readsize, f);
            if (size2 != readsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %u != %u\n", size2, readsize);
                goto end;
            }

            size2 = fwrite(buf, 1, readsize, outfile);
            if (size2 != readsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot write file: %u != %u\n", size2, readsize);
                goto end;
            }

            remainsize -= readsize;

            putchar('.');
            fflush(stdout);
        }
        while (remainsize > 0);

        putchar('\n');

        // verify
        fclose(outfile);
        outfile = NULL;

        buf2 = malloc(bufsize);
        if (!buf2)
        {
            ret = __LINE__;
            UG_LOG_ERR("Out of memory: %u\n", bufsize);
            goto end;
        }

        outfile = fopen(filename, "rb");
        if (!outfile)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot open file %s: %d\n", filename, errno);
            goto end;
        }

        if (itcStreamSeek((ITCStream*)f, -(long)filesize, SEEK_CUR))
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot seek file %d\n", errno);
            goto end;
        }

        UG_LOG_INFO("[%d%%] Verifying", ugGetProrgessPercentage());

        remainsize = filesize;
        do
        {
            readsize = (remainsize < bufsize) ? remainsize : bufsize;
            size2 = enc_fread(buf, 1, readsize, f);
            if (size2 != readsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %u != %u\n", size2, readsize);
                goto end;
            }

            size2 = fread(buf2, 1, readsize, outfile);
            if (size2 != readsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %u != %u\n", size2, readsize);
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
        }
        while (remainsize > 0);
        putchar('\n');
    }
    //=================================================



end:
    if (outfile)
        fclose(outfile);

    if (buf2)
        free(buf2);

    if (buf)
        free(buf);

    if (diff_buf)
        free(diff_buf);

    if (ucl_buf)
        free(ucl_buf);

    if (old_buf)
        free(old_buf);

    if (new_buf)
        free(new_buf);

    return ret;
}
