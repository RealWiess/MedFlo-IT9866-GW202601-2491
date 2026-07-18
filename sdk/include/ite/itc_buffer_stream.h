// BufferStream
/**
 * Buffer stream is a ring-buffer, memory-based stream.
 */
typedef struct
{
    ITCStream stream;       ///< Base stream definition
    unsigned char* buf;     ///< Memory buffer
    int size;               ///< Buffer size
    int readpos;            ///< Read position
    int writepos;           ///< Written position
    pthread_mutex_t mutex;  ///< Mutex for locking
} ITCBufferStream;

/**
 * Opens a buffer stream.
 *
 * @param bstream Pointer referring to the buffer stream.
 * @param size Buffer size.
 * @return Returns 0 if the buffer stream was successfully opened. A return non-zero value indicates an error.
 */
int itcBufferStreamOpen(ITCBufferStream* bstream, int size);
