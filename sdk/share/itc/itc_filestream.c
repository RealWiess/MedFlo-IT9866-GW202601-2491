#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include "ite/itc.h"
#include "itc_cfg.h"

int itcFileStreamClose(ITCFileStream* fs)
{    
    int result = 0;
    assert(fs);

    if (fs->file != NULL)
    {
        result = fclose(fs->file);
        fs->file = NULL;
    }
    return result;
}

int FileStreamClose(ITCStream* stream)
{
    return itcFileStreamClose((ITCFileStream*) stream);
}

static int FileStreamRead(ITCStream* stream, void* buf, int size)
{
    ITCFileStream* fs = (ITCFileStream*) stream;
    assert(fs);

    return fread(buf, 1, size, fs->file);
}

static int FileStreamWrite(ITCStream* stream, const void* buf, int size)
{
    ITCFileStream* fs = (ITCFileStream*) stream;
    assert(fs);

    return fwrite(buf, 1, size, fs->file);
}

static int FileStreamSeek(ITCStream* stream, long offset, int origin)
{
    ITCFileStream* fs = (ITCFileStream*) stream;
    assert(fs);

    return fseek(fs->file, offset, origin);
}

static long FileStreamTell(ITCStream* stream)
{
    ITCFileStream* fs = (ITCFileStream*) stream;
    assert(fs);

    return ftell(fs->file);
}

static int FileStreamAvailable(ITCStream* stream)
{
    ITCFileStream* fs = (ITCFileStream*) stream;
    long current_pos;

    assert(fs);

    current_pos = ftell(fs->file);
    if (current_pos < 0)
    {
        // handle ftell error
        ITC_LOG_ERR("FileStreamAvailable: ftell failed\n");
        return 0;
    }

    // ensure no integer overflow
    if (current_pos > stream->size)
    {
        return 0;
    }

    return stream->size - current_pos;
}

int itcFileStreamOpen(ITCFileStream* fstream, const char* filename, bool write)
{
    struct stat sb;
    static const ITCStreamVTable vtable = {
        FileStreamClose,
        FileStreamRead,
        FileStreamWrite,
        FileStreamSeek,
        FileStreamTell,
        FileStreamAvailable
    };
    assert(fstream);
    assert(filename);

    itcStreamOpen(&fstream->stream);
    fstream->stream.vptr = &vtable;

    fstream->file = fopen(filename, write ? "wb" : "rb");
    if (NULL == fstream->file)
    {
        ITC_LOG_ERR("itcFileStreamOpen %s failed\n", filename);
        return __LINE__;
    }

    if (fstat(fileno(fstream->file), &sb) == -1)
    {
        fclose(fstream->file);
        fstream->file = NULL;
        ITC_LOG_ERR("fstat %s failed\n", filename);
        return __LINE__;
    }

    fstream->stream.size = (int)sb.st_size;

    return 0;
}
