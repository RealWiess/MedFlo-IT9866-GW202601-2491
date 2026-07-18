#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "ite/itc.h"
#include "ite/ith.h"
#include "itc_cfg.h"

static int BufferStreamClose(ITCStream* stream)
{
    int result;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);


    result = pthread_mutex_lock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_lock fail: %d\n", result);
        return result;
    }

    free(bs->buf);
    bs->buf = NULL;

    pthread_mutex_unlock(&bs->mutex);

    result = pthread_mutex_destroy(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_destroy fail: %d\n", result);
    }
    return 0;
}

static int BufferStreamRead(ITCStream* stream, void* buf, int size)
{
    int result, readsize = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);

    result = pthread_mutex_lock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_lock fail: %d\n", result);
        return 0;
    }

    if (stream->eof)
        goto end;

    //if (bs->readpos == bs->size)
    //    bs->readpos = 0;

    if (bs->readpos < bs->writepos)
    {
        readsize = ITH_MIN(size, bs->writepos - bs->readpos);
        memcpy(buf, &bs->buf[bs->readpos], readsize);

        bs->readpos += readsize;
    }
    else // if (bs->readpos >= bs->writepos)
    {
        if (size > bs->size - bs->readpos)
        {
            int size1, size2;

            size1 = bs->size - bs->readpos;
            memcpy(buf, &bs->buf[bs->readpos], size1);

            size -= size1;
            bs->readpos = 0;

            size2 = ITH_MIN(size, bs->writepos - bs->readpos);
            memcpy(((unsigned char*) buf) + size1, &bs->buf[bs->readpos], size2);

            bs->readpos += size2;
            readsize = size1 + size2;
        }
        else // if (size <= bs->size - bs->readpos)
        {
            readsize = size;
            //memcpy(&bs->buf[bs->readpos], buf, readsize);
            memcpy(buf, &bs->buf[bs->readpos], readsize);
            bs->readpos += readsize;
        }
    }

    if (bs->readpos >= bs->size)
    {
        bs->readpos = 0;
    }

    if (readsize > 0 && bs->readpos == bs->writepos)
        stream->eof = 1;

end:

    result = pthread_mutex_unlock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_unlock fail: %d\n", result);
    }
    return readsize;
}

static int BufferStreamWrite(ITCStream* stream, const void* buf, int size)
{
    int result, writesize = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);

    result = pthread_mutex_lock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_lock fail: %d\n", result);
        return 0;
    }

    if (bs->writepos < bs->readpos)
    {
        writesize = ITH_MIN(size, bs->readpos - bs->writepos);
        memcpy(&bs->buf[bs->writepos], buf, writesize);

        bs->writepos += writesize;
    }
    else // if (bs->writepos >= bs->readpos)
    {
        if (size > bs->size - bs->writepos)
        {
            int size1, size2;

            size1 = bs->size - bs->writepos;
            memcpy(&bs->buf[bs->writepos], buf, size1);

            size -= size1;
            bs->writepos = 0;

            size2 = ITH_MIN(size, bs->readpos - bs->writepos);
            memcpy(&bs->buf[bs->writepos], ((unsigned char*) buf) + size1, size2);

            bs->writepos += size2;
            writesize = size1 + size2;
        }
        else // if (size <= bs->size - bs->writepos)
        {
            writesize = size;
            memcpy(&bs->buf[bs->writepos], buf, writesize);

            bs->writepos += writesize;
        }
    }

    if (bs->writepos >= bs->size)
    {
        bs->writepos = 0;
    }

    if (writesize > 0)
        stream->eof = 0;

    result = pthread_mutex_unlock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_unlock fail: %d\n", result);
    }
    return writesize;
}

static int BufferStreamSeek(ITCStream* stream, long offset, int origin)
{
    int pos, result = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);

    result = pthread_mutex_lock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_lock fail: %d\n", result);
        return 0;
    }

    switch (origin)
    {
    case SEEK_SET:
        if (bs->readpos < bs->writepos)
        {
            if (offset <= bs->writepos)
            {
                bs->readpos = offset;
                stream->eof = (bs->readpos == bs->writepos) ? 1 : 0;
            }
            else
            {
                printf("(%d), seek rp: %u, wp: %u, offset: %u\n",
                       __LINE__,
                       bs->readpos,
                       bs->writepos,
                       offset);
                result = -1;
        }
        }
        else // if (bs->readpos >= bs->writepos)
        {
            if (offset < bs->size)
            {
                bs->readpos = offset;
            }
            else
            {
                printf("(%d), seek rp: %u, wp: %u, offset: %u\n",
                       __LINE__,
                       bs->readpos,
                       bs->writepos,
                       offset);
                result = -1;
        }
        }
        break;

    case SEEK_CUR:
        pos = bs->readpos + offset;
        if (bs->readpos < bs->writepos)
        {
            if (pos >= 0 && pos < bs->writepos)
            {
                bs->readpos = pos;
                stream->eof = (bs->readpos == bs->writepos) ? 1 : 0;
            }
            else
            {
                printf("(%d), seek rp: %u, wp: %u, offset: %u\n",
                       __LINE__,
                       bs->readpos,
                       bs->writepos,
                       offset);
                result = -1;
        }
        }
        else // if (bs->readpos >= bs->writepos)
        {
            if (pos >= bs->writepos && pos < bs->size)
            {
                bs->readpos = pos;
            }
            else
                result = -1;
        }
        break;

    case SEEK_END:
        pos = bs->size + offset;
        if (bs->readpos < bs->writepos)
        {
            if (pos >= 0 && pos < bs->writepos)
            {
                bs->readpos = pos;
                stream->eof = (bs->readpos == bs->writepos) ? 1 : 0;
            }
            else
                result = -1;
        }
        else // if (bs->readpos >= bs->writepos)
        {
            if (pos >= bs->writepos && pos < bs->size)
            {
                bs->readpos = pos;
            }
            else
                result = -1;
        }
        break;

    default:
        assert(0);
        result = -1;
        break;
    }

    result = pthread_mutex_unlock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_unlock fail: %d\n", result);
    }
    return result;
}

static long BufferStreamTell(ITCStream* stream)
{
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);

    return bs->readpos;
}

static int BufferStreamAvailable(ITCStream* stream)
{
    int result, avail = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);

    result = pthread_mutex_lock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_lock fail: %d\n", result);
        return 0;
    }

    if (stream->eof)
        goto end;

    if (bs->readpos < bs->writepos)
    {
        avail = bs->writepos - bs->readpos;
    }
    else if (bs->readpos > bs->writepos)
    {
        avail = bs->size - bs->readpos + bs->writepos;
    }

end:
    result = pthread_mutex_unlock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_unlock fail: %d\n", result);
    }
    return avail;
}

static int BufferStreamReadLock(ITCStream* stream, void** bufptr, int size)
{
    int result, readsize = 0, avail = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bufptr);
    assert(bs);

    result = pthread_mutex_lock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_lock fail: %d\n", result);
        return ITC_LOCK_FAIL;
    }

    if (stream->eof)
        goto end;

    if (bs->readpos < bs->writepos)
    {
        avail = bs->writepos - bs->readpos;
    }
    else // if (bs->readpos >= bs->writepos)
    {
        avail = bs->size - bs->readpos;
    }

    readsize = ITH_MIN(size, avail);
    *bufptr = (void*) &bs->buf[bs->readpos];

end:
    return readsize;
}

static void BufferStreamReadUnlock(ITCStream* stream, int size)
{
    int result, readsize = 0, avail = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);

    if (stream->eof)
        goto end;

    if (bs->readpos < bs->writepos)
    {
        avail = bs->writepos - bs->readpos;
    }
    else // if (bs->readpos >= bs->writepos)
    {
        avail = bs->size - bs->readpos;
    }
    readsize = ITH_MIN(size, avail);

    bs->readpos += readsize;

    if (bs->readpos >= bs->size)
    {
        bs->readpos -= bs->size;
    }

    if (readsize > 0 && bs->readpos == bs->writepos)
        stream->eof = 1;

end:
    result = pthread_mutex_unlock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_unlock fail: %d\n", result);
    }
}

static int BufferStreamWriteLock(ITCStream* stream, void** bufptr, int size)
{
    int result, writesize = 0, avail = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bufptr);
    assert(bs);

    result = pthread_mutex_lock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_lock fail: %d\n", result);
        return ITC_LOCK_FAIL;
    }

    *bufptr = (void*) &bs->buf[bs->writepos];

    if (bs->writepos < bs->readpos)
    {
        avail = (bs->readpos - bs->writepos - 1);
    }
    else if (bs->writepos > bs->readpos)
    {
        avail = bs->size - bs->writepos + bs->readpos - 1;
    }
    writesize = ITH_MIN(size, avail);

    return writesize;
}

static void BufferStreamWriteUnlock(ITCStream* stream, int size)
{
    int result, writesize = 0, avail = 0;
    ITCBufferStream* bs = (ITCBufferStream*) stream;
    assert(bs);

    if (bs->writepos < bs->readpos)
    {
        avail = (bs->readpos - bs->writepos - 1);
    }
    else if (bs->writepos > bs->readpos)
    {
        avail = bs->size - bs->writepos + bs->readpos - 1;
        }
    writesize = ITH_MIN(size, avail);

            bs->writepos += writesize;

    if (bs->writepos >= bs->size)
    {
        bs->writepos = 0;
    }

    if (writesize > 0)
        stream->eof = 0;

    result = pthread_mutex_unlock(&bs->mutex);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_unlock fail: %d\n", result);
    }
}

int itcBufferStreamOpen(ITCBufferStream* bstream, int size)
{
    static const ITCStreamVTable vtable = {
        BufferStreamClose,
        BufferStreamRead,
        BufferStreamWrite,
        BufferStreamSeek,
        BufferStreamTell,
        BufferStreamAvailable,
        BufferStreamReadLock,
        BufferStreamReadUnlock,
        BufferStreamWriteLock,
        BufferStreamWriteUnlock
    };

    int result = 0;
    assert(bstream);
    assert(size > 0);

    itcStreamOpen((ITCStream*) bstream);
    bstream->stream.vptr = &vtable;

    bstream->buf = malloc(size);
    if (!bstream->buf)
    {
        ITC_LOG_ERR("Out of memory: %d\n", size);
        result = __LINE__;
        goto end;
    }

    result = pthread_mutex_init(&bstream->mutex, NULL);
    if (result)
    {
        ITC_LOG_ERR("pthread_mutex_init fail: %d\n", result);
        goto end;
    }
    bstream->size = size;
    bstream->readpos = bstream->writepos = 0;
    ((ITCStream*)bstream)->eof = 1;

end:
    return result;
}
