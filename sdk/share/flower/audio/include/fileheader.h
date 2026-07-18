#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WORDS_BIGENDIAN
#define le_uint32(a) (swap32((a)))
#define le_uint16(a) (swap16((a)))
#define le_int16(a) ( (int16_t) swap16((uint16_t)((a))) )
#else
#define le_uint32(a) (a)
#define le_uint16(a) (a)
#define le_int16(a) (a)
#endif

#ifndef O_BINARY
	#define O_BINARY 0 
#endif

typedef struct _riff_t {
	char id[4];	// RIFF
	uint32_t  data_size;
	char type[4];//WAVE
} riff_t;


/* The FORMAT chunk */

typedef struct _format_t {
	char  id[4];		//fmt_
	uint32_t   data_size;	//length of FORMAT chunk (0x10)
	uint16_t  code;		// codec type
	uint16_t nchannel;	//Channel numbers (0x01 = mono, 0x02 = stereo)
	uint32_t   samplerate;	// Sample rate (binary, in Hz)
	uint32_t   byte_per_sec;	//Average Bytes Per Second
	uint16_t block_align ;	//number of bytes per sample
	uint16_t bit_per_sample ;	//bits per sample */
} format_t;


/* The DATA chunk */

typedef struct _data_t {
	char id[4];	//data
	int  size;	//length of data
} data_t;


typedef struct _wave_header_t
{
	riff_t riff_block;
	format_t format_block;
	data_t data_block;
} wave_header_t;


typedef enum _PlayerState{
    Closed,
    Paused,
    Playing,
    Subplaying,
    Mixing,
    Eof,
    Dummy
}PlayerState;

typedef enum _RecorderState{
    RecorderClosed,
    RecorderPaused,
    RecorderRunning
}RecorderState;
