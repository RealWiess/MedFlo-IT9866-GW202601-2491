#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "ite_avienc.h"

#define CHECK_(condition)                                                      \
    if (!(condition))                                                          \
    {                                                                          \
        result = -1;                                                           \
        break;                                                                 \
    }

static int32_t write_short (FILE * out, uint32_t num)
{
    int32_t result = 0;
    uint8_t lilend_num[2];
    size_t  written;

    lilend_num[0] = num & 0xffU;
    lilend_num[1] = (num >> 8U) & 0xffU;

    written       = fwrite(lilend_num, 1, 2, out);
    if (written != 2)
    {
        result = -1;
    }

    return result;
}

static int32_t write_int (FILE * out, uint32_t num)
{
    int32_t result = 0;
    uint8_t lilend_num[4];
    size_t  written;

    lilend_num[0] = num & 0xffU;
    lilend_num[1] = (num >> 8U) & 0xffU;
    lilend_num[2] = (num >> 16U) & 0xffU;
    lilend_num[3] = (num >> 24U) & 0xffU;

    written       = fwrite(lilend_num, 1, 4, out);
    if (written != 4)
    {
        result = -1;
    }

    return result;
}

static int32_t write_avi_header (FILE *                    out,
                                 struct ite_avi_header_t * avi_header)
{
    int32_t result = 0;
    int     marker, t;
    size_t  written;

    do
    {
        written = fwrite("avih", 1, 4, out);
        CHECK_(written == 4);
        marker = ftell(out);
        CHECK_(marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);

        result = write_int(out, avi_header->time_delay);
        CHECK_(result == 0);
        result = write_int(out, avi_header->data_rate);
        CHECK_(result == 0);
        result = write_int(out, avi_header->reserved);
        CHECK_(result == 0);
        result = write_int(out, avi_header->flags);
        CHECK_(result == 0);
        result = write_int(out, avi_header->number_of_frames);
        CHECK_(result == 0);
        result = write_int(out, avi_header->initial_frames);
        CHECK_(result == 0);
        result = write_int(out, avi_header->data_streams);
        CHECK_(result == 0);
        result = write_int(out, avi_header->buffer_size);
        CHECK_(result == 0);
        result = write_int(out, avi_header->width);
        CHECK_(result == 0);
        result = write_int(out, avi_header->height);
        CHECK_(result == 0);
        result = write_int(out, avi_header->time_scale);
        CHECK_(result == 0);
        result = write_int(out, avi_header->playback_data_rate);
        CHECK_(result == 0);
        result = write_int(out, avi_header->starting_time);
        CHECK_(result == 0);
        result = write_int(out, avi_header->data_length);
        CHECK_(result == 0);

        t = ftell(out);
        CHECK_(t >= (marker + 4 * 15));
        result = fseek(out, marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);
    } while (false);

    return result;
}

static int32_t write_stream_header (FILE *                       out,
                                    struct ite_stream_header_t * stream_header)
{
    int32_t result = 0;
    int     marker, t;
    size_t  written;

    do
    {
        written = fwrite("strh", 1, 4, out);
        CHECK_(written == 4);
        marker = ftell(out);
        CHECK_(marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);

        written = fwrite(stream_header->data_type, 1, 4, out);
        CHECK_(written == 4);
        written = fwrite(stream_header->codec, 1, 4, out);
        CHECK_(written == 4);
        result = write_int(out, stream_header->flags);
        CHECK_(result == 0);
        result = write_int(out, stream_header->priority);
        CHECK_(result == 0);
        result = write_int(out, stream_header->initial_frames);
        CHECK_(result == 0);
        result = write_int(out, stream_header->time_scale);
        CHECK_(result == 0);
        result = write_int(out, stream_header->data_rate);
        CHECK_(result == 0);
        result = write_int(out, stream_header->start_time);
        CHECK_(result == 0);
        result = write_int(out, stream_header->data_length);
        CHECK_(result == 0);
        result = write_int(out, stream_header->buffer_size);
        CHECK_(result == 0);
        result = write_int(out, stream_header->quality);
        CHECK_(result == 0);
        result = write_int(out, stream_header->sample_size);
        CHECK_(result == 0);
        result = write_int(out, 0);
        CHECK_(result == 0);
        result = write_int(out, 0);
        CHECK_(result == 0);

        t = ftell(out);
        CHECK_(t >= (marker + 4 * 15));
        result = fseek(out, marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);
    } while (false);

    return 0;
}

static int32_t write_stream_format_v (
    FILE * out, struct ite_stream_format_v_t * stream_format_v)
{
    int32_t result = 0;
    int     marker, t;
    size_t  written;

    do
    {
        written = fwrite("strf", 1, 4, out);
        CHECK_(written == 4);
        marker = ftell(out);
        CHECK_(marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);

        result = write_int(out, stream_format_v->headerSize);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->width);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->height);
        CHECK_(result == 0);
        result =
            write_int(out, stream_format_v->planes +
                               stream_format_v->bits_per_pixel * 256 * 256);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->compression_type);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->imageSize);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->x_pels_per_meter);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->y_pels_per_meter);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->clr_used);
        CHECK_(result == 0);
        result = write_int(out, stream_format_v->clr_important);
        CHECK_(result == 0);

        t = ftell(out);
        CHECK_(t >= (marker + 4 * 11));
        result = fseek(out, marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);
    } while (false);

    return result;
}

static int32_t write_stream_format_a (
    FILE * out, struct ite_stream_format_a_t * stream_format_a)
{
    int32_t result = 0;
    int     marker, t;
    size_t  written;

    do
    {
        written = fwrite("strf", 1, 4, out);
        CHECK_(written == 4);
        marker = ftell(out);
        CHECK_(marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);

        result = write_short(out, stream_format_a->format);
        CHECK_(result == 0);
        result = write_short(out, stream_format_a->channels);
        CHECK_(result == 0);
        result = write_int(out, stream_format_a->sampleRate);
        CHECK_(result == 0);
        result = write_int(out, stream_format_a->bytesPerSecond);
        CHECK_(result == 0);
        result = write_short(out, stream_format_a->block_align);
        CHECK_(result == 0);
        result = write_short(out, stream_format_a->bitsPerSample);
        CHECK_(result == 0);
        result = write_short(out, stream_format_a->size);
        CHECK_(result == 0);

        t = ftell(out);
        CHECK_(t >= (marker + 4 * 3 + 2 * 5));
        result = fseek(out, marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);
    } while (false);

    return result;
}

static int32_t write_junk_chunk (FILE * out)
{
    int32_t result = 0;
    int     marker, t;
    int     r, l;
    size_t  written;
    char *  junk = {"JUNK IN THE CHUNK! "};

    do
    {
        written = fwrite("JUNK", 1, 4, out);
        CHECK_(written == 4);
        marker = ftell(out);
        CHECK_(marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);

        r = 4096 - ftell(out);
        l = strlen(junk);
        t = 0;
        while (t < r)
        {
            if (t + l <= r)
            {
                fwrite(junk, l, 1, out);
            }
            else
            {
                fwrite(junk, r - t, 1, out);
            }
            t = t + l;
        }

        t = ftell(out);
        CHECK_(t != -1);
        result = fseek(out, marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);
    } while (false);

    return result;
}

static int32_t write_avi_header_chunk (struct ite_avi_t * avi)
{
    int32_t result = 0;
    long    marker, t;
    long    sub_marker;
    size_t  written;
    FILE *  out = avi->out;

    do
    {
        written = fwrite("LIST", 1, 4, out);
        CHECK_(written == 4);
        marker = ftell(out);
        CHECK_(marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);
        written = fwrite("hdrl", 1, 4, out);
        CHECK_(written == 4);
        result = write_avi_header(out, &avi->avi_header);
        CHECK_(result == 0);

        written = fwrite("LIST", 1, 4, out);
        CHECK_(written == 4);
        sub_marker = ftell(out);
        CHECK_(sub_marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);
        written = fwrite("strl", 1, 4, out);
        CHECK_(written == 4);
        result = write_stream_header(out, &avi->stream_header_v);
        CHECK_(result == 0);
        result = write_stream_format_v(out, &avi->stream_format_v);
        CHECK_(result == 0);

        t = ftell(out);
        CHECK_(t != -1);
        result = fseek(out, sub_marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - sub_marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);

        if (avi->avi_header.data_streams == 2)
        {
            written = fwrite("LIST", 1, 4, out);
            CHECK_(written == 4);
            sub_marker = ftell(out);
            CHECK_(sub_marker >= 4);
            result = write_int(out, 0);
            CHECK_(result == 0);
            written = fwrite("strl", 1, 4, out);
            CHECK_(written == 4);
            result = write_stream_header(out, &avi->stream_header_a);
            CHECK_(result == 0);
            result = write_stream_format_a(out, &avi->stream_format_a);
            CHECK_(result == 0);

            t = ftell(out);
            CHECK_(t != -1);
            result = fseek(out, sub_marker, SEEK_SET);
            CHECK_(result == 0);
            result = write_int(out, t - sub_marker - 4);
            CHECK_(result == 0);
            result = fseek(out, t, SEEK_SET);
            CHECK_(result == 0);
        }

        t = ftell(out);
        CHECK_(t != -1);
        result = fseek(out, marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);

        result = write_junk_chunk(out);
        CHECK_(result == 0);
    } while (false);

    return result;
}

static int32_t write_index (FILE * out, int count, unsigned int * offsets)
{
    int32_t result = 0;
    int     marker, t;
    size_t  written;
    int     offset = 4;

    do
    {
        CHECK_(offsets != NULL);

        written = fwrite("idx1", 1, 4, out);
        CHECK_(written == 4);
        marker = ftell(out);
        CHECK_(marker >= 4);
        result = write_int(out, 0);
        CHECK_(result == 0);

        for (t = 0; t < count; t++)
        {
            if ((offsets[t] & 0x80000000) == 0)
            {
                fwrite("00dc", 4, 1, out);
            }
            else
            {
                fwrite("01wb", 4, 1, out);
                offsets[t] &= 0x7fffffff;
            }
            result = write_int(out, 0x10);
            CHECK_(result == 0);
            result = write_int(out, offset);
            CHECK_(result == 0);
            result = write_int(out, offsets[t]);
            CHECK_(result == 0);
            offset = offset + offsets[t] + 8;
        }
        CHECK_(result == 0);

        t = ftell(out);
        CHECK_(t != -1);
        result = fseek(out, marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(out, t - marker - 4);
        CHECK_(result == 0);
        result = fseek(out, t, SEEK_SET);
        CHECK_(result == 0);
    } while (false);

    return result;
}

struct ite_avi_t * ite_avi_open (char * filename, int width, int height,
                                 char * fourcc, int fps,
                                 struct ite_avi_audio_t * audio)
{
    int32_t            result = 0;
    struct ite_avi_t * avi    = NULL;
    FILE *             out;
    size_t             written;

    do
    {
        out = fopen(filename, "wb");
        CHECK_(out != NULL);

        avi = (struct ite_avi_t *)malloc(sizeof(struct ite_avi_t));
        CHECK_(avi != NULL);
        memset(avi, 0, sizeof(struct ite_avi_t));

        avi->out         = out;

        avi->offsets_len = 1024;
        avi->offsets     = malloc(avi->offsets_len * sizeof(int));
        CHECK_(avi->offsets != NULL);
        avi->offsets_ptr           = 0;

        /* set avi header */
        avi->avi_header.time_delay = 1000000 / fps;
        avi->avi_header.data_rate  = 0;
        avi->avi_header.flags      = 0x10;
        if (audio == NULL)
        {
            avi->avi_header.data_streams = 1;
        }
        else
        {
            avi->avi_header.data_streams = 2;
        }
        avi->avi_header.number_of_frames = 0; // FIXME - this needs to be reset
        avi->avi_header.width            = width;
        avi->avi_header.height           = height;
        avi->avi_header.buffer_size      = 0;

        /* set stream header */
        strlcpy(avi->stream_header_v.data_type, "vids",
                sizeof(avi->stream_header_v.data_type));
        memcpy(avi->stream_header_v.codec, fourcc, 4);
        avi->stream_header_v.time_scale     = 1;
        avi->stream_header_v.data_rate      = fps;
        avi->stream_header_v.buffer_size    = 0;
        avi->stream_header_v.data_length    = 0;

        /* set stream format */
        avi->stream_format_v.headerSize     = 40;
        avi->stream_format_v.width          = width;
        avi->stream_format_v.height         = height;
        avi->stream_format_v.planes         = 1;
        avi->stream_format_v.bits_per_pixel = 24;
        avi->stream_format_v.compression_type =
            ((uint32_t)fourcc[3] << 24U) + ((uint32_t)fourcc[2] << 16U) +
            ((uint32_t)fourcc[1] << 8U) + ((uint32_t)fourcc[0]);
        avi->stream_format_v.imageSize         = width * height * 3;
        avi->stream_format_v.clr_used          = 0;
        avi->stream_format_v.clr_important     = 0;

        avi->stream_format_v.clr_palette       = 0;
        avi->stream_format_v.clr_palette_count = 0;

        if (avi->avi_header.data_streams == 2)
        {
            /* set stream header */
            strlcpy(avi->stream_header_a.data_type, "auds",
                    sizeof(avi->stream_header_a.data_type));
            avi->stream_header_a.codec[0]   = 1;
            avi->stream_header_a.codec[1]   = 0;
            avi->stream_header_a.codec[2]   = 0;
            avi->stream_header_a.codec[3]   = 0;
            avi->stream_header_a.time_scale = 1;
            avi->stream_header_a.data_rate  = audio->samples_per_second;
            avi->stream_header_a.buffer_size =
                audio->channels * (audio->bits / 8) * audio->samples_per_second;
            avi->stream_header_a.quality = -1;
            avi->stream_header_a.sample_size =
                (audio->bits / 8) * audio->channels;

            /* set stream format */
            avi->stream_format_a.format     = 1;
            avi->stream_format_a.channels   = audio->channels;
            avi->stream_format_a.sampleRate = audio->samples_per_second;
            avi->stream_format_a.bytesPerSecond =
                audio->channels * (audio->bits / 8) * audio->samples_per_second;
            avi->stream_format_a.block_align =
                audio->channels * (audio->bits / 8);

            avi->stream_format_a.bitsPerSample = audio->bits;
            avi->stream_format_a.size          = 0;
        }

        written = fwrite("RIFF", 1, 4, out);
        CHECK_(written == 4);
        result = write_int(out, 0);
        CHECK_(result == 0);
        written = fwrite("AVI ", 1, 4, out);
        CHECK_(written == 4);
        result = write_avi_header_chunk(avi);
        CHECK_(result == 0);

        written = fwrite("LIST", 1, 4, out);
        CHECK_(written == 4);
        avi->marker = ftell(out);
        CHECK_(avi->marker != -1);
        result = write_int(out, 0);
        CHECK_(result == 0);
        written = fwrite("movi", 1, 4, out);
        CHECK_(written == 4);

    } while (false);

    if (result != 0)
    {
        if (out != NULL)
        {
            fclose(out);
        }
        if (avi != NULL)
        {
            if (avi->offsets != NULL)
            {
                free(avi->offsets);
            }
            free(avi);
            avi = NULL;
        }
    }

    return avi;
}

int32_t ite_avi_add_frame (struct ite_avi_t * avi, unsigned char * buffer,
                           int len)
{
    int32_t result = 0;
    int     maxi_pad; /* if your frame is raggin, give it some paddin' */
    char    pad_buffer[4] = {0};
    size_t  written;

    do
    {
        avi->offset_count++;
        avi->stream_header_v.data_length++;

        maxi_pad = len % 4;
        if (maxi_pad > 0)
        {
            maxi_pad = 4 - maxi_pad;
        }

        if (avi->offset_count >= avi->offsets_len)
        {
            avi->offsets_len += 1024;
            avi->offsets =
                realloc(avi->offsets, avi->offsets_len * sizeof(int));
            CHECK_(avi->offsets != NULL);
        }

        avi->offsets[avi->offsets_ptr++] = len + maxi_pad;

        written                          = fwrite("00dc", 1, 4, avi->out);
        CHECK_(written == 4);
        result = write_int(avi->out, len + maxi_pad);
        CHECK_(result == 0);

        written = fwrite(buffer, 1, len, avi->out);
        CHECK_(written == len);

        if (maxi_pad > 0)
        {
            written = fwrite(pad_buffer, 1, maxi_pad, avi->out);
            CHECK_(written == maxi_pad);
        }
    } while (false);

    return result;
}

int32_t ite_avi_add_audio (struct ite_avi_t * avi, unsigned char * buffer,
                           int len)
{
    int32_t result = 0;
    int     maxi_pad; /* incase audio bleeds over the 4 byte boundary  */
    char    pad_buffer[4] = {0};
    size_t  written;

    do
    {
        CHECK_(avi != NULL);
        avi->offset_count++;

        maxi_pad = len % 4;
        if (maxi_pad > 0)
        {
            maxi_pad = 4 - maxi_pad;
        }

        if (avi->offset_count >= avi->offsets_len)
        {
            avi->offsets_len += 1024;
            avi->offsets =
                realloc(avi->offsets, avi->offsets_len * sizeof(int));
            CHECK_(avi->offsets != NULL);
        }

        avi->offsets[avi->offsets_ptr++] = (len + maxi_pad) | 0x80000000;

        written                          = fwrite("01wb", 1, 4, avi->out);
        CHECK_(written == 4);
        result = write_int(avi->out, len + maxi_pad);
        CHECK_(result == 0);
        written = fwrite(buffer, 1, len, avi->out);
        CHECK_(written == len);

        if (maxi_pad > 0)
        {
            written = fwrite(pad_buffer, 1, maxi_pad, avi->out);
            CHECK_(written == maxi_pad);
        }

        avi->stream_header_a.data_length += len + maxi_pad;
    } while (false);

    return result;
}

int32_t ite_avi_close (struct ite_avi_t * avi)
{
    int32_t result = 0;
    long    t;

    do
    {
        CHECK_(avi != NULL);

        t = ftell(avi->out);
        CHECK_(t != -1);
        result = fseek(avi->out, avi->marker, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(avi->out, t - avi->marker - 4);
        CHECK_(result == 0);
        result = fseek(avi->out, t, SEEK_SET);
        CHECK_(result == 0);
        (void)printf("avi->offset_count = %d\n", avi->offset_count);
        result = write_index(avi->out, avi->offset_count, avi->offsets);
        CHECK_(result == 0);

        if (avi->offsets != NULL)
        {
            free(avi->offsets);
        }

        /* reset some avi header fields */
        avi->avi_header.number_of_frames = avi->stream_header_v.data_length;

        t                                = ftell(avi->out);
        CHECK_(t != -1);
        result = fseek(avi->out, 12, SEEK_SET);
        CHECK_(result == 0);
        result = write_avi_header_chunk(avi);
        CHECK_(result == 0);
        result = fseek(avi->out, t, SEEK_SET);
        CHECK_(result == 0);

        t = ftell(avi->out);
        CHECK_(t != -1);
        result = fseek(avi->out, 4, SEEK_SET);
        CHECK_(result == 0);
        result = write_int(avi->out, t - 8);
        CHECK_(result == 0);
        result = fseek(avi->out, t, SEEK_SET);
        CHECK_(result == 0);

        fclose(avi->out);
        free(avi);
    } while (false);

    return result;
}
