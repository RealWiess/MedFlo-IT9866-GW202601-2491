// BlockStream
/**
* Block stream is a block-based stream. Don't support lock related functions.
*/
typedef struct
{
    ITCStream stream;        ///< Base stream definition
    int fd;                  ///< Device handle
    int start;               ///< Start address
    int blockSize;           ///< Block size
    int gapSize;             ///< Gap size
    int offset;              ///< Block offset of start address
    int pos;                 ///< Current position
    char* cache;             ///< Cache blocks
    int cachePos;            ///< Cache position
    int cacheSize;           ///< Cache size
    int cacheBlockPos;       ///< Cache block position
    int cacheBlockCount;     ///< Cache block count
    int write;               ///< Reading or writting
} ITCBlockStream;

/**
* Opens a block stream.
*
* @param bstream Pointer referring to the block stream.
* @param devname The device name to open.
* @param start The address to read or write.
* @param size The size to read or write.
* @param write Indicates the stream is for reading or writting.
* @return Returns 0 if the block stream was successfully opened. A return non-zero value indicates an error.
*/
int itcBlockStreamOpen(ITCBlockStream* bstream, const char* devname, uint32_t start, int size, bool write);
