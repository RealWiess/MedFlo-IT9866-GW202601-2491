#include <assert.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "ite/itp.h"
#include "ite/itc.h"
#include "itc_cfg.h"

#define CMD_HEADER_MAX_SIZE (128)

static char* cmdHeaderBuffer;

typedef enum
{
    CLI_STREAM_CMD_READ_FILE,
    CLI_STREAM_CMD_CLOSE_FILE,
    CLI_STREAM_CMD_SIZE,    
} CLI_STREAM_CMD_TYPE;

static const char* cmdHeaderTable[CLI_STREAM_CMD_SIZE] = 
{
    "Read",
    "Close"
};

static int StreamWrite(ITCStream* stream, const void* buf, int size)
{
    assert(stream);
    return 0;
}

static int CliStreamRead(ITCStream* stream, void* buf, int size)
{
    int readSize = 0;
    int totalRead = 0;
    
    ITCCliStream* cs = (ITCCliStream*) stream;
    assert(cs);

    readSize = cs->stream.size - cs->pos > size ? size : cs->stream.size - cs->pos;
       
    sprintf(cmdHeaderBuffer, "%s %d %d\n", cmdHeaderTable[CLI_STREAM_CMD_READ_FILE], cs->pos, readSize);
    write(cs->cliHandler, cmdHeaderBuffer, strlen(cmdHeaderBuffer));
    while(readSize)
    {
        if (ioctl(cs->cliHandler, ITP_IOCTL_IS_CONNECTED, NULL) == 0)
        {
            ITC_LOG_ERR("ERROR: CLI steam %X disconnect\n", cs->cliHandler);
            break;
        }
        int once = read(cs->cliHandler, (char*)buf + totalRead, readSize);
        if (once < 0)
        {
            ITC_LOG_ERR("Read error %d\r\n", totalRead);
            break;
        }
        readSize -= once;
        totalRead += once;
    }
    cs->pos += totalRead;

    return totalRead;
}

static int CliStreamSeek(ITCStream* stream, long offset, int origin)
{
    int pos = 0, result = 0;
    ITCCliStream* cs = (ITCCliStream*) stream;
    assert(cs);
    
    switch (origin)
    {
    case SEEK_SET:
        if (offset < cs->stream.size)
        {
            cs->pos = offset;
        }
        else
            result = -1;

        break;

    case SEEK_CUR:
        pos = cs->pos + offset;
        if (pos >= 0 && pos < cs->stream.size)
        {
            cs->pos = pos;
        }
        else
            result = -1;

        break;

    case SEEK_END:
        pos = cs->stream.size + offset;
        if (pos >= 0 && pos < cs->stream.size)
        {
            cs->pos = pos;
        }
        else
            result = -1;

        break;

    default:
        assert(0);
        result = -1;
        break;
    }
    return result;
}

static long CliStreamTell(ITCStream* stream)
{
    ITCCliStream* cs = (ITCCliStream*) stream;
    assert(cs);

    return cs->pos;
}

static int CliStreamAvailable(ITCStream* stream)
{
    ITCCliStream* cs = (ITCCliStream*) stream;
    assert(cs);

    return cs->stream.size - cs->pos;
}

static int CliStreamCloseStream(ITCStream* stream)
{
    int result;
    ITCCliStream* cs = (ITCCliStream*) stream;
    assert(cs);
    sprintf(cmdHeaderBuffer, "%s\n", cmdHeaderTable[CLI_STREAM_CMD_CLOSE_FILE]);
    write(cs->cliHandler, cmdHeaderBuffer, strlen(cmdHeaderBuffer));
    free(cmdHeaderBuffer);
    return 0;
}

int itcCliStreamOpen(ITCCliStream* cstream, int cliHandler, int fileSize)
{
    static const ITCStreamVTable vtable = {
        CliStreamCloseStream,
        CliStreamRead,
        StreamWrite,
        CliStreamSeek,
        CliStreamTell,
        CliStreamAvailable,
    };
    int ret = 0;
    assert(cstream);
    assert(fileSize > 0);

    itcStreamOpen((ITCStream*) cstream);
    cstream->stream.vptr = &vtable;

    cstream->cliHandler = cliHandler;
    cstream->stream.size = fileSize;
    cstream->pos = 0;
    if (ioctl(cstream->cliHandler, ITP_IOCTL_IS_CONNECTED, NULL) == 0)
    {
        ITC_LOG_ERR("ERROR: CLI steam %X not connect\n", cstream->cliHandler);
        ret = -1;
    }
    else
        cmdHeaderBuffer = malloc(CMD_HEADER_MAX_SIZE);
 
    return ret;
}
