/** @defgroup itc_stream Stream
 *  @{
 */
typedef struct ITCStreamTag ITCStream;

typedef struct ITCStreamVTableTag
{
    /**
     * Stream close method
     *
     * @param stream Pointer referring to the opened stream.
     * @return Returns 0 if the stream was successfully closed. A return non-zero value indicates an error.
     */
    int (*Close)(ITCStream* stream);

    /**
     * Stream read method
     *
     * @param stream Pointer referring to the opened stream.
     * @param buf Storage location for data.
     * @param size Maximum number of data unit, in bytes.
     * @return Returns the number of data unit read. A return value of -1 indicates an error.
     */
    int (*Read)(ITCStream* stream, void* buf, int size);

    /**
     * Stream write method
     *
     * @param stream Pointer referring to the opened stream.
     * @param buf Data to be written.
     * @param size Number of data unit, in bytes.
     * @return If successful, it returns the number of data unit actually written. A return value of -1 indicates an error.
     */
    int (*Write)(ITCStream* stream, const void* buf, int size);

    /**
     * Set position in a stream.
     *
     * @param stream Pointer referring to the opened stream.
     * @param offset Number of data unit from origin, in bytes.
     * @param origin Initial position.
     * @return Returns the offset, in data unit, of the new position from the beginning of the stream. The function returns -1 to indicate an error.
     */
    int (*Seek)(ITCStream* stream, long offset, int origin);

    /**
     * Gets the current position of a stream.
     *
     * @param stream Pointer referring to the opened stream.
     * @return Returns the current stream position.
     */
    long (*Tell)(ITCStream* stream);

    /**
     * Gets the current available size of a stream.
     *
     * @param stream Pointer referring to the opened stream.
     * @return Returns the current available size.
     */
    int (*Available)(ITCStream* stream);

    /**
     * Locks internal memory buffer of a stream for reading.
     *
     * @param stream Pointer referring to the opened stream.
     * @param bufptr Internal memory buffer address to retrieve.
     * @param size buffer size to lock.
     * @return If successful, it returns the locked size. A return value of ITC_LOCK_FAIL indicates lock fail error.
     */
    int (*ReadLock)(ITCStream* stream, void** bufptr, int size);

    /**
     * Unlocks internal memory buffer of a stream for reading.
     *
     * @param stream Pointer referring to the opened stream.
     * @param size the locked buffer size.
     */
    void (*ReadUnlock)(ITCStream* stream, int size);

    /**
     * Locks internal memory buffer of a stream for writing.
     *
     * @param stream Pointer referring to the opened stream.
     * @param bufptr Internal memory buffer address to retrieve.
     * @param size buffer size to lock.
     * @return If successful, it returns the locked size. A return value of ITC_LOCK_FAIL indicates lock fail error.
     */
    int (*WriteLock)(ITCStream* stream, void** bufptr, int size);

    /**
     * Unlocks internal memory buffer of a stream for writing.
     *
     * @param stream Pointer referring to the opened stream.
     * @param size the locked buffer size.
     */
    void (*WriteUnlock)(ITCStream* stream, int size);
} ITCStreamVTable;

/**
 * Stream interface definition.
 */
struct ITCStreamTag
{
    ITCStreamVTable const * vptr;   ///< Stream virtual functions table

    int eof;    ///< Indicates the stream is on END-OF-FILE or not.
    int size;   ///< The stream total size.
};

/* virtual functions (late binding) */
/**
 * Closes a opened stream.
 *
 * @param stream Pointer referring to the opened stream.
 * @return Returns 0 if the stream was successfully closed. A return non-zero value indicates an error.
 */
static inline int itcStreamClose(ITCStream* stream) { return stream->vptr->Close(stream); }

/**
 * Reads a opened stream.
 *
 * @param stream Pointer referring to the opened stream.
 * @param buf Storage location for data.
 * @param size Maximum number of data unit, in bytes.
 * @return Returns the number of data unit read. A return value of -1 indicates an error.
 */
static inline int itcStreamRead(ITCStream* stream, void* buf, int size) { return stream->vptr->Read(stream, buf, size); }

/**
 * Writes a opened stream.
 *
 * @param stream Pointer referring to the opened stream.
 * @param buf Data to be written.
 * @param size Number of data unit, in bytes.
 * @return If successful, it returns the number of data unit actually written. A return value of -1 indicates an error.
 */
static inline int itcStreamWrite(ITCStream* stream, const void* buf, int size) { return stream->vptr->Write(stream, buf, size); }

/**
 * Set position in a stream.
 *
 * @param stream Pointer referring to the opened stream.
 * @param offset Number of data unit from origin, in bytes.
 * @param origin Initial position.
 * @return Returns the offset, in data unit, of the new position from the beginning of the stream. The function returns -1 to indicate an error.
 */
static inline int itcStreamSeek(ITCStream* stream, long offset, int origin) { return stream->vptr->Seek(stream, offset, origin); }

/**
 * Gets the current position of a stream.
 *
 * @param stream Pointer referring to the opened stream.
 * @return Returns the current stream position.
 */
static inline long itcStreamTell(ITCStream* stream) { return stream->vptr->Tell(stream); }

/**
 * Gets the current available size of a stream.
 *
 * @param stream Pointer referring to the opened stream.
 * @return Returns the current available size.
 */
static inline int itcStreamAvailable(ITCStream* stream) { return stream->vptr->Available(stream); }

/**
 * Locks internal memory buffer of a stream for reading.
 *
 * @param stream Pointer referring to the opened stream.
 * @param bufptr Internal memory buffer address to retrieve.
 * @param size buffer size to lock.
 * @return If successful, it returns the locked size. A return value of ITC_LOCK_FAIL indicates lock fail error.
 */
static inline int itcStreamReadLock(ITCStream* stream, void** bufptr, int size) { return stream->vptr->ReadLock(stream, bufptr, size); }

/**
 * Unlocks internal memory buffer of a stream for reading.
 *
 * @param stream Pointer referring to the opened stream.
 * @param size the locked buffer size.
 */
static inline void itcStreamReadUnlock(ITCStream* stream, int size) { stream->vptr->ReadUnlock(stream, size); }

/**
 * Locks internal memory buffer of a stream for writing.
 *
 * @param stream Pointer referring to the opened stream.
 * @param bufptr Internal memory buffer address to retrieve.
 * @param size buffer size to lock.
 * @return If successful, it returns the locked size. A return value of ITC_LOCK_FAIL indicates lock fail error.
 */
static inline int itcStreamWriteLock(ITCStream* stream, void** bufptr, int size) { return stream->vptr->WriteLock(stream, bufptr, size); }

/**
 * Unlocks internal memory buffer of a stream for writing.
 *
 * @param stream Pointer referring to the opened stream.
 * @param size the locked buffer size.
 */
static inline void itcStreamWriteUnlock(ITCStream* stream, int size) { stream->vptr->WriteUnlock(stream, size); }

/**
 * Opens a stream.
 *
 * @param stream Pointer referring to the stream.
 */
void itcStreamOpen(ITCStream* stream);

/** @} */ // end of itc_stream
