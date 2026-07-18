//================ mp3 =======================


/* indexing = [version][samplerate index]
 * sample rate of frame (Hz)
 */
const static int mp3samplerateTab[3][3] = 
{
    {44100, 48000, 32000},        /* MPEG-1 */
    {22050, 24000, 16000},        /* MPEG-2 */
    {11025, 12000,  8000},        /* MPEG-2.5 */
};

/* indexing = [version][layer]
 * number of samples in one frame (per channel)
 */
const static int samplesPerFrameTab[3][3] = 
{
    {384, 1152, 1152 }, /* MPEG1 */
    {384, 1152,  576 }, /* MPEG2 */
    {384, 1152,  576 }, /* MPEG2.5 */
};

/* indexing = [version][sampleRate][bitRate]
 * for layer3, nSlots = floor(samps/frame * bitRate / sampleRate / 8)
 *   - add one pad slot if necessary
 */
const static int slotTab[3][3][15] = 
{
    {
        /* MPEG-1 */
        { 0, 104, 130, 156, 182, 208, 261, 313, 365, 417, 522, 626, 731, 835,1044 },    /* 44 kHz */
        { 0,  96, 120, 144, 168, 192, 240, 288, 336, 384, 480, 576, 672, 768, 960 },    /* 48 kHz */
        { 0, 144, 180, 216, 252, 288, 360, 432, 504, 576, 720, 864,1008,1152,1440 },    /* 32 kHz */
    },
    {
        /* MPEG-2 */
        { 0,  26,  52,  78, 104, 130, 156, 182, 208, 261, 313, 365, 417, 470, 522 },    /* 22 kHz */
        { 0,  24,  48,  72,  96, 120, 144, 168, 192, 240, 288, 336, 384, 432, 480 },    /* 24 kHz */
        { 0,  36,  72, 108, 144, 180, 216, 252, 288, 360, 432, 504, 576, 648, 720 },    /* 16 kHz */
    },
    {
        /* MPEG-2.5 */
        { 0,  52, 104, 156, 208, 261, 313, 365, 417, 522, 626, 731, 835, 940,1044 },    /* 11 kHz */
        { 0,  48,  96, 144, 192, 240, 288, 336, 384, 480, 576, 672, 768, 864, 960 },    /* 12 kHz */
        { 0,  72, 144, 216, 288, 360, 432, 504, 576, 720, 864,1008,1152,1296,1440 },    /*  8 kHz */
    },
};

/* indexing = [version][layer][bitrate index]
 * bitrate (kbps) of frame
 *   - bitrate index == 0 is "free" mode (bitrate determined on the fly by
 *       counting bits between successive sync words)
 */
const static int bitrateTab[3][3][15] = 
{
    {
        /* MPEG-1 */
        {  0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448}, /* Layer 1 */
        {  0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384}, /* Layer 2 */
        {  0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320}, /* Layer 3 */
    },
    {
        /* MPEG-2 */
        {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256}, /* Layer 1 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 2 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 3 */
    },
    {
        /* MPEG-2.5 */
        {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256}, /* Layer 1 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 2 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 3 */
    },
};

/* indexing = [version][mono/stereo]
 * number of bytes in side info section of bitstream
 */
const static short sideBytesTab[3][2] = {
    {17, 32},   /* MPEG-1:   mono, stereo */
    { 9, 17},   /* MPEG-2:   mono, stereo */
    { 9, 17},   /* MPEG-2.5: mono, stereo */
};
//================ aac =======================

#define NUM_AAC_SAMPLE_RATES    12

/* aac sample rates (table 4.5.1) */
const int aacsampRateTab[NUM_AAC_SAMPLE_RATES] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025,  8000
};
