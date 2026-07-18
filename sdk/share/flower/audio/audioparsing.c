#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/fileheader.h"
#include "ite/audio.h"
#include "flower/flower.h"
#ifdef CFG_BUILD_FFMPEG
    #include "include/audioqueue.h"
    #ifdef CFG_FFMPEG_AUDIO_DECODE
        #include "libavcodec/aac.h"
    #endif
#endif
#include "include/table_def.h"

// #define PARSING_BUFFER_SIZE 80*1024
#define DATA_READ_SIZE 6 * 1024

static unsigned int audioGetFileSize (char * filename);

/**
 * @brief Parse mp3 file and get its duration, sample rate, channel num and
 * bytes per 20ms.
 *
 * @param fp File pointer, can be NULL.
 * @param filename File name.
 * @param ptr Data pointer if fp is NULL.
 * @param length Data length if fp is NULL.
 * @param di DataInfo structure to store the parsed information.
 *
 * @return The duration of the mp3 file in seconds.
 */
int parsing_mp3 (FILE * fp, char * filename, char * ptr, int length,
                 DataInfo * di)
{
    // Macro to swap the byte order of a 32-bit integer.  Used for handling
    // frame counts in some MP3 headers. MP3 files can store multi-byte values
    // in either big-endian or little-endian format. This macro ensures that the
    // frame count is interpreted correctly regardless of the file's byte order.
    // It shifts and combines the bytes to reverse their order.
    // For example, if x = 0xAABBCCDD, then SWAP32(x) will result in 0xDDCCBBAA.
    // This is crucial for correctly reading multi-byte values from the file.
    // The bitwise operations are used to isolate and reposition each byte:
    // - (x & 0x000000FF) extracts the least significant byte and shifts it to
    // the most significant byte position.
    // - (x & 0x0000FF00) extracts the second least significant byte and shifts
    // it to the second most significant byte position.
#define SWAP32(x)                                                              \
    (((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) |                      \
     ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24))

    int time     = 0;
    int nID3size = 0;
    int nSize    = 0;
    // Variables to store parsed information about the MP3 frame.
    int layer, brIdx, srIdx, id, ver, bitRate, sampling_rate, samplesperframe,
        nFileSize, frameCounts, nch;
    // Buffer to read data from the file.
    char readBuffer[DATA_READ_SIZE];
    // Buffers to store header information.
    char header1[4]  = {0};
    char header2[16] = {0};

    // Determine the file size based on whether a file pointer or a data pointer
    // is provided.
    if (fp == NULL)
    {
        // If no file pointer, use the provided length as the file size.
        nFileSize = length;
    }
    else
    {
        // If a file pointer is provided, get the file size using a helper
        // function.
        nFileSize = audioGetFileSize(filename);
        // Get the current file position, which represents the size of any ID3v2
        // tag at the beginning of the file. ID3v2 tags are metadata blocks that
        // can precede the actual audio data.
        nID3size  = ftell(fp);
    }

    // Loop to find the first valid MP3 frame header.
    while (nSize < nFileSize)
    {
        // Flag to indicate if a valid header is found.
        char isFindHdr = 0;
        int  i, size;
        // Read data into the buffer.
        if (fp == NULL)
        {
            // If no file pointer, copy data from the provided pointer.
            // This is used when parsing from a memory buffer instead of a file.
            // The size of the data copied is limited by the size of the
            // readBuffer. This approach is common when dealing with streaming
            // audio or data received over a network. It avoids the need for
            // file I/O operations, making it suitable for real-time or embedded
            // systems.
            memcpy(readBuffer, ptr, sizeof(readBuffer));
            size = sizeof(readBuffer);
        }
        else
        {
            size = fread(readBuffer, 1, sizeof(readBuffer), fp);
        }

        // Iterate through the buffer to find the sync word (0xFF) followed by
        // specific bits indicating a valid MP3 frame header.
        for (i = 0; i + 2 < size; i++)
        {
            // Check for the sync word (0xFF) and the following bits (at least
            // 11 '1's). The sync word is a unique bit pattern that marks the
            // beginning of an MP3 frame. It's always 0xFF followed by at least
            // 11 '1' bits in the next byte. This pattern helps the decoder
            // identify the start of a new frame even if the data stream is
            // corrupted. The bitwise AND operation (readBuffer[i + 1] & 0xe0)
            // checks if the three most significant bits of the next byte are
            // '1'. If this condition is met, it's a potential start of an MP3
            // frame.
            if ((char)readBuffer[i] == (char)0xff &&
                ((readBuffer[i + 1] & 0xe0) == 0xe0))
            {
                // Extract information from the header.
                // The MP3 frame header contains various pieces of information
                // about the audio data, such as the layer, bitrate index, and
                // sampling rate index. These values are encoded within specific
                // bits of the header bytes. Bitwise operations (shifts and
                // ANDs) are used to isolate and extract these values. For
                // example, (readBuffer[i + 1] >> 1) & 0x3 extracts the layer
                // information from bits 1 and 2 of the second header byte. The
                // extracted values are then used to look up further details in
                // tables (defined elsewhere) about the audio encoding.
                layer = 4 - ((readBuffer[i + 1] >> 1) & 0x3);
                brIdx = (readBuffer[i + 2] >> 4) & 0xf;
                srIdx = (readBuffer[i + 2] >> 2) & 0x3;
                if (srIdx == 3 || layer == 4 || brIdx == 15)
                {
                    continue; // illegal frame
                }
                // Set the flag to indicate that a header has been found.
                isFindHdr = 1;
                break;
            }
        }

        // If a header is found, update the size and break the loop.
        if (isFindHdr)
        {
            // Update the size with the offset to the header.
            nSize += i;
            // Adjust the pointer or file position to the beginning of the first
            // valid frame.
            if (fp == NULL)
            {
                // If parsing from a buffer, advance the pointer.
                ptr += nSize;
            }
            else
            {
                // If parsing from a file, seek to the new position.
                // The new position is calculated by adding the ID3v2 tag size
                // (nID3size) and the offset to the header (nSize). This ensures
                // that the file pointer is positioned at the start of the first
                // audio frame, skipping any metadata tags. The SEEK_SET
                // argument specifies that the offset is relative to the
                // beginning of the file.
                (void)fseek(fp, nID3size + nSize, SEEK_SET);
            }
            break;
        }
        nSize += size;
    }

    // If no header is found before reaching the end of the file, return 0.
    if (nSize >= nFileSize)
    {
        printf("found header fail eof \n");
        return time;
    }
    // Adjust the remaining file size after finding the header.
    nFileSize -= nSize;
    // printf("size(%d)(%x)\n",nFileSize,nSize);
    // Read the first frame header (4 bytes) to extract frame information.
    if (fp == NULL)
    {
        // If parsing from a buffer, copy the header from the pointer.
        memcpy(header1, ptr, sizeof(header1));
        // Advance the pointer past the header.
        ptr += sizeof(header1);
    }
    else
    {
        // If parsing from a file, read the header.
        (void)fread(header1, 1, sizeof(header1), fp);
    }
    // Extract frame information from the header.
    // These bitwise operations extract specific fields from the MP3 frame
    // header, such as the MPEG version, channel mode, sampling rate, and
    // bitrate. These fields are encoded within specific bits of the header
    // bytes, and bitwise operations (shifts and ANDs) are used to isolate and
    // extract them. For example, (header1[1] >> 3) & 0x3 extracts the MPEG
    // version from bits 3 and 4 of the second header byte. The extracted values
    // are then used to look up further details in tables (defined elsewhere)
    // about the audio encoding.
    id  = (header1[1] >> 3) & 0x3;
    ver = (id == 0 ? 2 : (id & 0x1 ? 0 : 1));
    // Determine the number of channels (mono or stereo).
    // The number of channels is encoded in bits 6 and 7 of the fourth header
    // byte. If these bits are both set to '1' (0x03), it indicates a single
    // channel (mono). Otherwise, it indicates two channels (stereo).
    if (((header1[3] >> 6) & 0x03) == 0x3)
    {
        nch = 1;
    }
    else
    {
        // If the bits are not both '1', it's stereo.
        nch = 2;
    }

    // Look up the sampling rate, samples per frame, and bitrate from tables
    // based on the extracted indices. The MP3 standard defines tables that map
    // the extracted indices (ver, srIdx, layer, brIdx) to specific audio
    // parameters. These tables contain pre-defined values for sampling rates,
    // samples per frame, and bitrates for different MP3 versions and layers.
    // The code uses these indices to retrieve the corresponding values from the
    // tables, which are essential for decoding the audio data.
    sampling_rate   = mp3samplerateTab[ver][srIdx];
    samplesperframe = samplesPerFrameTab[ver][layer - 1];
    bitRate         = bitrateTab[ver][layer - 1][brIdx] * 1000;
    // printf("SR(%d) FS(%d) BR(%d) ch(%d) ver(%d)\n", sampling_rate,
    // samplesperframe, bitRate, nch, ver);
    di->sr          = sampling_rate;
    di->nch         = nch;
    di->bytes20ms   = 20 * sampling_rate * nch * 2 / 1000;
    // Read additional header information (Xing, Info, or VBRI) to determine the
    // file duration more accurately.

    if (fp == NULL)
    {
        ptr += sideBytesTab[ver][nch - 1];
        memcpy(header2, ptr, sizeof(header2));
    }
    else
    {
        // If parsing from a file, seek past the side information and read the
        // header.
        (void)fseek(fp, sideBytesTab[ver][nch - 1], SEEK_CUR);
        (void)fread(header2, 1, sizeof(header2), fp);
    }

    if (memcmp("Xing", header2, 4) == 0 || memcmp("Info", header2, 4) == 0)
    {
        // printf("Find Xing header:%s\n", header2);
        if ((header2[7] & 0x01) != 0x01)
        {
            printf("Xing header doesn't contain the required information.\n");
            return time;
        }
        // Extract the frame count from the Xing header.
        // The frame count is a 32-bit integer located at a specific offset
        // within the Xing header. The SWAP32 macro is used to handle potential
        // byte order differences. The extracted frame count is then used to
        // calculate the duration of the MP3 file. This method provides a more
        // accurate duration estimate compared to using the bitrate, especially
        // for variable bitrate (VBR) files.
        frameCounts = SWAP32(*((int *)(header2 + 8)));
        // printf("frameCounts(%x)(%d)\n",frameCounts,frameCounts);
        time        = frameCounts * samplesperframe / sampling_rate;
    }
    else if (memcmp("VBRI", header2, 4) == 0)
    {
        // printf("Find VBRI header\n");
        int tmp[16] = {0};
        // Copy the header to a temporary buffer to avoid potential alignment
        // issues. This is a common practice when dealing with headers or
        // structures that might not be naturally aligned in memory. Copying the
        // data to a local buffer ensures that subsequent operations, such as
        // accessing multi-byte values, are performed on aligned data. This can
        // prevent crashes or unexpected behavior, especially on platforms with
        // strict alignment requirements. The size of the temporary buffer (tmp)
        // is chosen to match the size of the header being copied (header2).
        // This ensures that all relevant data is copied without overflow or
        // truncation.
        memcpy(tmp, header2, sizeof(header2));
        frameCounts = SWAP32(*((long int *)(tmp + 14)));
        // printf("frameCounts(%x)(%d)\n",frameCounts,frameCounts);
        time        = frameCounts * samplesperframe / sampling_rate;
    }
    else
    {
        // If no Xing or VBRI header is found, assume CBR (constant bitrate)
        // format and estimate the duration. For CBR files, the duration can be
        // estimated using the file size and bitrate. The formula used is:
        // duration = (file size in bits) / (bitrate in bits per second). This
        // provides a reasonable estimate of the duration, although it might not
        // be as accurate as using frame counts from Xing or VBRI headers. If
        // the bitrate is not available (bitRate is 0), the function attempts to
        // parse the MP3 again, hoping to find a valid bitrate in a subsequent
        // frame. This is a fallback mechanism to handle cases where the initial
        // header might be incomplete or corrupted.
        printf("CBR format\n");
        if (bitRate)
        {
            time = nFileSize * 8 / bitRate;
        }
        else
        {
            if (fp != NULL)
            {
                printf("bitRate = %d :parsing mp3 try again\n", bitRate);
                time = parsing_mp3(fp, filename, (void *)NULL, 0, di);
            }
        }
    }
    // Store the calculated duration in the DataInfo structure.
    di->duration = time;
    // If parsing from a file, seek back to the beginning of the first audio
    // frame.
    if (fp != NULL)
    {
        // The file pointer is moved to the position immediately after the ID3v2
        // tag (if any) and the first MP3 frame header. This ensures that
        // subsequent reads start from the actual audio data, skipping any
        // metadata or header information.
        (void)fseek(fp, nID3size + nSize, SEEK_SET);
    }
    // Return the estimated duration of the MP3 file in seconds.
    return time;
}

/**
 * @brief Calculates the total duration of a WAV file in seconds.
 *
 * This function parses the given WAV file to calculate its total playtime.
 * The duration is calculated by dividing the size of the audio data by the
 * sample rate, number of channels, and bit depth.
 *
 * @param fp The file pointer to the WAV file.
 * @param di A pointer to a DataInfo structure to store parsed audio metadata.
 *
 * @return The total duration of the audio file in seconds.
 * Returns 0 if the file is not a valid WAV file.
 */
int parsing_wav (FILE * fp, DataInfo * di)
{
    double     wavtime;
    // size_t ret;
    char       header1[sizeof(riff_t)];
    char       header2[sizeof(format_t)];
    char       header3[sizeof(data_t)];

    riff_t *   riff_chunk   = (riff_t *)header1;
    format_t * format_chunk = (format_t *)header2;
    data_t *   data_chunk   = (data_t *)header3;

    int        count;
    size_t     read_size;

    read_size = fread(header1, 1, sizeof(header1), fp);
    if (read_size != sizeof(header1))
    {
        printf("read wav header error\n");
        return 0;
    }

    if (0 != strncmp(riff_chunk->id, "RIFF", 4) ||
        0 != strncmp(riff_chunk->type, "WAVE", 4))
    {
        printf("RIFF WAVE head error\n");
        goto not_a_wav;
    }
    else
    {
        (void)fseek(fp, sizeof(riff_t), SEEK_SET);
    }

    read_size = fread(header2, 1, sizeof(header2), fp);
    if (read_size != sizeof(header2))
    {
        printf("read wav format header error\n");
        return 0;
    }
    if (0 != strncmp(format_chunk->id, "fmt ", 4))
    {
        printf("fmt head error\n");
        goto not_a_wav;
    }
    di->sr        = format_chunk->samplerate;
    di->nch       = format_chunk->nchannel;
    di->bitsize   = format_chunk->bit_per_sample;
    di->bytes20ms = 20 * di->sr * di->nch * 2 / 1000;

    if (format_chunk->data_size - 0x10 > 0)
    {
        (void)fseek(fp, (format_chunk->data_size - 0x10), SEEK_CUR);
    }
    read_size = fread(header3, 1, sizeof(header3), fp);
    if (read_size != sizeof(header3))
    {
        printf("read wav data header error\n");
        return 0;
    }

    count = 0;
    while (strncmp(data_chunk->id, "data", 4) != 0 && count < 30)
    {
        printf("skipping chunk=%s len=%i\n", data_chunk->id, data_chunk->size);
        (void)fseek(fp, data_chunk->size, SEEK_CUR);
        count++;
        read_size = fread(header3, 1, sizeof(header3), fp);
        if (read_size != sizeof(header3))
        {
            printf("read wav data header error\n");
            return 0;
        }
    }

    wavtime = (double)data_chunk->size / (double)format_chunk->byte_per_sec;
    di->duration = wavtime;
    return wavtime;

not_a_wav:
    printf("not a wav\n");
    return 0;
}

/**
 * @brief Get the codec type by file extension.
 *
 * @param [in]  filename  file name
 *
 * @return  codec type
 *
 * @retval  -1  unknown file format
 *
 * @example
 * filename = "xxx.wav" => result = ITE_WAV_DECODE
 * filename = "xxx.mp3" => result = ITE_MP3_DECODE
 * filename = "xxx.wma" => result = ITE_WMA_DECODE
 * filename = "xxx.aac" => result = ITE_AAC_DECODE
 * filename = "xxx.m4a" => result = ITE_AAC_DECODE
 * filename = "xxx.m3u8" => result = ITE_AAC_DECODE
 * filename = "xxx.flac" => result = ITE_FLAC_DECODE
 * filename = "xxx.ogg" => result = ITE_VORBIS_DECODE
 * filename = "xxx.opus" => result = ITE_OPUS_DECODE
 *
 * @remark
 * This function is used to get the codec type by file extension.
 * If the file format is not supported, return -1.
 */
int audiomgrCodecType (char * filename)
{
    if (strstr(filename, ".wav"))
    {
        return ITE_WAV_DECODE;
    }
    else if (strstr(filename, ".mp3") || strstr(filename, ".MP3"))
    {
        return ITE_MP3_DECODE;
    }
    else if (strstr(filename, ".wma"))
    {
        return ITE_WMA_DECODE;
    }
    else if (strstr(filename, ".aac"))
    {
        return ITE_AAC_DECODE;
    }
    else if (strstr(filename, ".m4a"))
    {
        return ITE_AAC_DECODE;
    }
    else if (strstr(filename, ".m3u8"))
    {
        return ITE_AAC_DECODE;
    }
    else if (strstr(filename, ".flac"))
    {
        return ITE_FLAC_DECODE;
    }
    else if (strstr(filename, ".ogg"))
    {
        return ITE_OGG_DECODE;
    }
    else if (strstr(filename, ".opus"))
    {
        return ITE_OPUS_DECODE;
    }
    else
    {
        printf("Unsupport file format: %s\n", filename);
        return -1;
    }
    return -1;
}

/**
 * @brief Get typical extension type by sub filename
 *
 * @param [in]  filename  file name
 *
 * @return  extension type
 *
 * @retval  -1  unknown file format
 *
 * @example
 * filename = "xxx.wav" => result = ITE_WAV
 * filename = "xxx.mp3" => result = ITE_MP3
 * filename = "xxx.wma" => result = ITE_WMA
 * filename = "xxx.aac" => result = ITE_AAC
 * filename = "xxx.m4a" => result = ITE_M4A
 * filename = "xxx.m3u8" => result = ITE_M3U8
 * filename = "xxx.flac" => result = ITE_FLAC
 * filename = "xxx.ogg" => result = ITE_VORBIS
 * filename = "xxx.opus" => result = ITE_OPUS
 *
 * @remark
 * This function is used to get typical extension type by sub filename.
 * If the file format is not supported, return -1.
 */
int audiomgrExtensionName (char * filename)
{
    int result = -1;

    do
    {
        if (filename == NULL)
        {
            break;
        }

        if (strstr(filename, ".wav"))
        {
            result = ITE_WAV;
        }
        else if (strstr(filename, ".mp3") || strstr(filename, ".MP3"))
        {
            result = ITE_MP3;
        }
        else if (strstr(filename, ".wma"))
        {
            result = ITE_WMA;
        }
        else if (strstr(filename, ".aac"))
        {
            result = ITE_AAC;
        }
        else if (strstr(filename, ".m4a"))
        {
            result = ITE_M4A;
        }
        else if (strstr(filename, ".m3u8"))
        {
            result = ITE_M3U8;
        }
        else if (strstr(filename, ".flac"))
        {
            result = ITE_FLAC;
        }
        else if (strstr(filename, ".ogg"))
        {
            result = ITE_VORBIS;
        }
        else if (strstr(filename, ".opus"))
        {
            result = ITE_OPUS;
        }
        else
        {
            (void)printf("Unsupport file format: %s\n", filename);
        }
    } while (false);

    return result;
}

/**
 * @brief Checks if the given mp3 file or data contains ID3v2 tags, and if so,
 *        seek to the beginning of the audio data.
 *
 * @param fd The file pointer to the mp3 file.
 * @param inptr The pointer to the beginning of the mp3 data.
 * @param length The length of the mp3 data.
 * @param tag A pointer to an int to store the size of the ID3v2 tag.
 *
 * @return The size of the ID3v2 tag in bytes.
 */
void parsingMp3IsID3v2 (FILE * fd, char * inptr, int length, int * tag)
{
    // Buffer to store the first 1024 bytes of the file or data.
    char pbuf[1024] = {0};
    // Check if a file pointer is provided.
    if (fd == NULL)
    {
        // If no file pointer, check if the provided data length is less than
        // 1024 bytes.
        if (length < 1024)
        {
            // If the data is less than 1024 bytes, print a warning message.
            // This indicates that there might be an issue since ID3v2 tags can
            // be larger than this.
            printf("length < 1024 ,may error!!\n");
        }

        // If a data pointer is provided, copy up to 1024 bytes from the data
        // into the buffer.
        if (inptr != NULL)
        {
            memcpy(pbuf, inptr, 1024);
        }
    }
    else
    {
        // If a file pointer is provided, read up to 1024 bytes from the file
        // into the buffer.
        (void)fread(pbuf, 1, 1024, fd);
    }

    // Initialize the tag size to 0. This will store the size of the ID3v2 tag
    // if found.
    *tag = 0;

    // Check if the first 3 bytes of the buffer match "ID3", which is the
    // identifier for ID3v2 tags.
    if (memcmp("ID3", pbuf, 3) == 0)
    {
        // If "ID3" is found, calculate the size of the ID3v2 tag.
        // The size is encoded in the 4 bytes following the header (bytes 6-9),
        // with each byte representing 7 bits of the size. The size is
        // calculated by shifting and combining these bytes.
        *tag =
            (pbuf[6] << 21) | (pbuf[7] << 14) | (pbuf[8] << 7) | (pbuf[9] << 0);
        // Add 10 bytes to the size, which accounts for the 10-byte ID3v2 header
        // itself.
        *tag += 10;
        // If a file pointer is provided, seek to the end of the ID3v2 tag in
        // the file.
        if (fd != NULL)
        {
            (void)fseek(fd, *tag, SEEK_SET);
        }
        // Print the size of the ID3v2 tag.
        printf("idV3tag (%d)\n", *tag);
    }
    // If no "ID3" tag is found and a file pointer is provided, seek to the
    // beginning of the file (size 0). This is done because the initial read
    // might have moved the file pointer, and we want to ensure subsequent reads
    // start from the correct position.
    else if (fd != NULL)
    {
        fseek(fd, *tag, SEEK_SET);
    }
    // Note: If no ID3 tag is found and fd is NULL, the function does nothing,
    // as there's no file pointer to adjust. The *tag value remains 0,
    // indicating no ID3v2 tag was found.
}

/**
 * Get the file size of the file with given filename.
 *
 * This function retrieves the size of a file specified by its filename.
 * It uses the `access` function to check if the file exists and is accessible.
 * If the file exists, it uses the `stat` function to obtain file metadata,
 * including the file size, which is then returned. If the file does not exist
 * or is not accessible, it prints an error message and returns 0.
 *
 * The `access` function checks the accessibility of the file. The `F_OK` flag
 * checks if the file exists. If `access` returns 0, the file exists and is
 * accessible.
 *
 * @param filename The file name to get the size of.
 *
 * @return The size of the file in bytes if the file exists, 0 otherwise.
 */
static unsigned int audioGetFileSize (char * filename)
{
    struct stat st;
    int         size;

    if (access(filename, F_OK) == 0)
    {
        // size_t ret;
        (void)stat(filename, &st);
        size = st.st_size;
    }
    else
    {
        size = 0;
        (void)printf("%s ot exit\n", filename);
    }
    return size;
}

/**
 * @brief Calculates the total duration of an audio file in seconds.
 *
 * This function determines the file type based on its extension,
 * and processes the file to calculate its total playtime.
 * Supported formats include MP3, WAV, and M4A (with FFMPEG).
 *
 * @param filename The path to the audio file.
 * @param di A pointer to a DataInfo structure to store parsed audio metadata.
 *
 * @return The total duration of the audio file in seconds.
 * Returns 0 if the file does not exist or if the format is unsupported.
 */

int audioGetTotalTime (char * filename, DataInfo * di)
{
    FILE * finput    = NULL;
    int    totalTime = 0;
    int    type;

    do
    {
        if (filename == NULL)
        {
            (void)printf("filename is NULL\n");
            break;
        }

        if (access(filename, 0) < 0)
        {
            (void)printf("%s is not exist! audioGetTotalTime not do\n",
                         filename);
            break;
        }

        type          = audiomgrExtensionName(filename);
        di->codectype = audiomgrCodecType(filename);
        di->bitsize   = 16;

        switch (type)
        {
            case ITE_MP3: // mp3
                {
                    finput = fopen(filename, "rb");
                    if (finput != NULL)
                    {
                        int offset = 0;
                        parsingMp3IsID3v2(finput, (void *)NULL, 0, &offset);
                        // parsing_data_init();
                        totalTime =
                            parsing_mp3(finput, filename, (void *)NULL, 0,
                                        di); // seconds
                        fclose(finput);
                    }
                    break;
                }
            case ITE_WAV: // wav
                {
                    finput = fopen(filename, "rb");
                    if (finput != NULL)
                    {
                        totalTime = parsing_wav(finput, di); // seconds
                        fclose(finput);
                    }
                    break;
                }
            case ITE_M4A:
                {
#ifdef CFG_BUILD_FFMPEG
                    AVFormatContext * ic;
                    int               err;

                    av_register_all();
                    ic  = avformat_alloc_context();
                    err = avformat_open_input(&ic, filename, NULL, NULL);
                    if (err == 0)
                    {
                        totalTime =
                            (double)ic->duration / (1000 * 1000); // seconds
                        avformat_close_input(&ic);
                    }
#endif
                    break;
                }
        }
    } while (false);

    return totalTime;
}

/**
 * @brief Calculates the total duration of an MP3 audio stream in seconds.
 *
 * This function processes the given MP3 audio stream to calculate its total
 * playtime. The stream is expected to be a raw MP3 audio stream, without ID3
 * tags.
 *
 * @param ptr The pointer to the beginning of the MP3 audio stream.
 * @param length The length of the MP3 audio stream in bytes.
 * @param di A pointer to a DataInfo structure to store parsed audio metadata.
 *
 * @return The total duration of the audio stream in seconds.
 */
int parsingMp3Stream (char * ptr, int length, DataInfo * di)
{
    di->codectype = ITE_MP3_DECODE;
    di->bitsize   = 16;
    return parsing_mp3(NULL, NULL, ptr, length, di); // seconds;
}

/**
 * @brief Parses an ID3v2 tag from an MP3 audio stream.
 *
 * This function processes the given MP3 audio stream to parse an ID3v2 tag.
 * The stream is expected to be a raw MP3 audio stream, without ID3 tags.
 * The function will return the size of the ID3v2 tag.
 *
 * @param ptr The pointer to the beginning of the MP3 audio stream.
 * @param length The length of the MP3 audio stream in bytes.
 * @param tag A pointer to an int to store the size of the ID3v2 tag.
 *
 * @return The size of the ID3v2 tag in bytes.
 */
int parsingID3V2Stream (char * ptr, int length, int * tag)
{
    parsingMp3IsID3v2(NULL, ptr, length, tag);
    return *tag;
}

void id3tagParsing (IteFilter * f, rbuf_ite * RingBuf)
{ // filter run parsing
    int         offset = 0, availSize = 0;
    bool        first = true;
    IteQueueblk blk   = {0};

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite * im = blk.datap;
            if (im)
            {
                ite_rbuf_put(RingBuf, im->b_rptr, im->size);
                freemsg_ite(im);
            }
            availSize = ite_rbuf_get_avail_size(RingBuf);
            if (first)
            {
                (void)parsingID3V2Stream((char *)RingBuf->b_rptr, availSize,
                                         &offset); // get id3v2 length;
                first = false;
            }
            else
            {
                if (availSize >= offset)
                {
                    RingBuf->b_rptr += offset;
                    ite_rbuf_rearrange(RingBuf); // memove to header
                    break;
                }
                else
                {
                    ite_rbuf_flush(RingBuf);
                    offset -= availSize;
                }
            }
        }
        usleep(5000);
    }
}

/**
 * @brief Swaps adjacent bytes in a given byte array.
 *
 * This function iterates through the byte array in steps of two, swapping
 * each pair of adjacent bytes. It is designed to operate on arrays with
 * even lengths, as it swaps bytes in pairs.
 *
 * @param bytes A pointer to the byte array to be modified.
 * @param len The length of the byte array. It should be an even number.
 */

void swap_bytes (unsigned char * bytes, int len)
{
    int           i;
    unsigned char tmp;

    for (i = 0; i < len; i += 2)
    {
        tmp          = bytes[i];
        bytes[i]     = bytes[i + 1];
        bytes[i + 1] = tmp;
    }
}

#ifdef CFG_BUILD_FFMPEG
void aacADTSHeader (unsigned char * adtsHeader, AVCodecContext * avctx,
                    int framelength)
{                                          // for aac header add

    unsigned int dwSynWord        = 0xFFF; // 12bit
    unsigned int dwID             = 0;     // 1bit
    unsigned int dwLayer          = 0;     // 2bit    '00'
    unsigned int dwPotectAbsent   = 1;     // 1bit
    unsigned int dwProfile        = 1;     // 2bit
    unsigned int sampleFreIdx     = 3;     // 4bit
    unsigned int dwPrivateBit     = 0;     // 1bit
    // unsigned int  channels = 2;                          // 3bit
    unsigned int dwOriCopy        = 0;               // 1bit
    unsigned int dwHome           = 0;               // 1bit
    unsigned int dwCRIDBit        = 0;               // 1bit
    unsigned int dwCRIDStart      = 0;               // 1bit
    unsigned int dwFrameLength    = framelength + 7; // 13bit
    unsigned int dwBufferFullness = 0x7FF;           // 11bit
    unsigned int dwNoRawData      = 0;               // 2bit
    unsigned int sampleFre        = avctx->sample_rate;
    unsigned int channels         = avctx->channels;
    #ifdef CFG_FFMPEG_AUDIO_DECODE
    AACContext * ac = avctx->priv_data;
    if (ac->m4ac.sbr == 1)
    { // easzy HE_AAC_V2
        if (ac->m4ac.ext_sample_rate > ac->m4ac.sample_rate)
        {
            sampleFre = ac->m4ac.sample_rate;
        }
    }
        // printf("ac->m4ac.ext_sample_rate=%d
        // ac->m4ac.sample_rate=%d\n",ac->m4ac.ext_sample_rate,ac->m4ac.sample_rate);
        // printf("sampleFre=%d channels=%d\n",sampleFre,channels);
    #endif
    switch (sampleFre)
    {
        case 9600:
            sampleFreIdx = 0;
            break;
        case 88200:
            sampleFreIdx = 1;
            break;
        case 64000:
            sampleFreIdx = 2;
            break;
        case 48000:
            sampleFreIdx = 3;
            break;
        case 44100:
            sampleFreIdx = 4;
            break;
        case 32000:
            sampleFreIdx = 5;
            break;
        case 24000:
            sampleFreIdx = 6;
            break;
        case 22050:
            sampleFreIdx = 7;
            break;
        case 16000:
            sampleFreIdx = 8;
            break;
        case 12000:
            sampleFreIdx = 9;
            break;
        case 11025:
            sampleFreIdx = 10;
            break;
        case 8000:
            sampleFreIdx = 11;
            break;
        case 7350:
            sampleFreIdx = 12;
            break;
    }

    adtsHeader[0] = (unsigned char)(dwSynWord >> 4);
    adtsHeader[1] = (unsigned char)(((dwSynWord & 0xF) << 4) | (dwID << 3) |
                                    (dwLayer << 1) | (dwPotectAbsent));
    adtsHeader[2] = (unsigned char)((dwProfile << 6) | (sampleFreIdx << 2) |
                                    (dwPrivateBit << 1) | (channels >> 2));
    adtsHeader[3] = (unsigned char)(((channels & 0x3) << 6) | (dwOriCopy << 5) |
                                    (dwHome << 4) | (dwCRIDBit << 3) |
                                    (dwCRIDStart << 2) | (dwFrameLength >> 11));
    adtsHeader[4] = (unsigned char)((dwFrameLength & 0x7F8) >> 3);
    adtsHeader[5] =
        (unsigned char)(((dwFrameLength & 0x7) << 5) | (dwBufferFullness >> 6));
    adtsHeader[6] =
        (unsigned char)(((dwBufferFullness & 0x3F) << 2) | (dwNoRawData));
}
#endif
