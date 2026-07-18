#include <assert.h>
#include "ite/itp.h"
#include "ite/itc.h"

static int StreamClose(struct ITCStreamTag* stream)
{
    assert(stream);
    return 0;
}

static int StreamRead(struct ITCStreamTag* stream, void* buf, int size)
{
    assert(stream);
    return 0;
}

static int StreamWrite(struct ITCStreamTag* stream, const void* buf, int size)
{
    assert(stream);
    return 0;
}

static int StreamSeek(struct ITCStreamTag* stream, long offset, int origin)
{
    assert(stream);
    return 0;
}

static long StreamTell(struct ITCStreamTag* stream)
{
    assert(stream);
    return 0;
}

static int StreamAvailable(struct ITCStreamTag* stream)
{
    assert(stream);
    return 0;
}

void itcStreamOpen(ITCStream* stream)
{
    static const ITCStreamVTable vtable = {
        StreamClose,
        StreamRead,
        StreamWrite,
        StreamSeek,
        StreamTell,
        StreamAvailable,
    };
    assert(stream);

    stream->vptr        = &vtable;
    stream->eof         = 0;
    stream->size        = 0;
}
