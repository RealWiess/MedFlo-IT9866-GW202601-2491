// CliStream
/**
 * CLI stream is a read-only, command line based stream. Don't support write related functions.
 */
typedef struct
{
    ITCStream stream;   ///< Base stream definition
    int pos;            ///< Current position
    int cliHandler;
} ITCCliStream;

/**
 * Opens a CLI stream.
 *
 * @param cstream Pointer referring to the CLI stream. 
 * @param cliHandler The CLI device handler.
 * @param fileSize Remote file size.
 * @return Returns 0 if the CLI stream was successfully opened. A return non-zero value indicates an error.
 */
int itcCliStreamOpen(ITCCliStream* cstream, int cliHandler, int fileSize);
