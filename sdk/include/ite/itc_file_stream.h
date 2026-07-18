// FileStream
/**
 * File stream is a file-based stream. Don't support lock related functions.
 */
typedef struct
{
    ITCStream stream;   ///< Base stream definition
    FILE* file;         ///< File handle
} ITCFileStream;

/**
 * Opens a file stream.
 *
 * @param fstream Pointer referring to the file stream.
 * @param filename The file path to open.
 * @param write Indicates the stream is for reading or writting.
 * @return Returns 0 if the file stream was successfully opened. A return non-zero value indicates an error.
 */
int itcFileStreamOpen(ITCFileStream* fstream, const char* filename, bool write);

/**
 * @brief Closes the specified file stream.
 *
 * This function closes the file stream associated with the given ITCFileStream pointer,
 * releasing any resources allocated for the stream.
 *
 * @param fs Pointer to the ITCFileStream to be closed.
 * @return 0 on success, or a negative error code on failure.
 */
int itcFileStreamClose(ITCFileStream* fs);
