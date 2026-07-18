// ArrayStream
/**
 * Array stream is a read-only, memory-based stream. Don't support write related functions.
 */
typedef struct
{
    ITCStream stream;   ///< Base stream definition
    const char* array;  ///< Array buffer
    int pos;            ///< Current position
} ITCArrayStream;

/**
 * Opens a array stream.
 *
 * @param astream Pointer referring to the array stream.
 * @param array Array buffer.
 * @param size Array size.
 * @return Returns 0 if the array stream was successfully opened. A return non-zero value indicates an error.
 */
int itcArrayStreamOpen(ITCArrayStream* astream, const char* array, int size);
