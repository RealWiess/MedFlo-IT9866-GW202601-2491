#define SIGN_BIT    (0x80)  /* Sign bit for a A-law byte. */
#define QUANT_MASK  (0xf)   /* Quantization field mask. */
#define NSEGS       (8)     /* Number of A-law segments. */
#define SEG_SHIFT   (4)     /* Left shift for segment number. */
#define SEG_MASK    (0x70)  /* Segment field mask. */
#define BIAS        (0x84)  /* Bias for linear code. */
#define MAXDECODESIZE (4*1024) //MAXDECODESIZE  < I2SBUFSIZE

unsigned char emptyBuffer[MAXDECODESIZE];

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 */
static short alaw2linear(unsigned char a_val)
{
    short t;
    short seg;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned) a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg) {
    case 0:
        t += 8;
        break;
    case 1:
        t += 0x108;
        break;
    default:
        t += 0x108;
        t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}


/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
static short ulaw2linear(unsigned char u_val)
{
    short t;

    /* Complement to obtain normal u-law value. */
    u_val = ~u_val;

    /*
     * Extract and bias the quantization bits. Then
     * shift up by the segment number and subtract out the bias.
     */
    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned) u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}


/* pcm8 to pcm16 conversion */
static short pcm8_linear(char pcm_val)
{
    return ((short)(pcm_val-128) * (1 << 8)) + (short)pcm_val;
}

static int alawDecode(mblkbuf *m,WaveInfoMode *Info,pcmData *Pcm)
{   //compressing rate 1:2 = alaw : pcm
    unsigned char *start=m->b_rptr;
    int Readsize=m->b_wptr-m->b_rptr;
    unsigned char *end;
    int i;
    if(Readsize > MAXDECODESIZE/2) Readsize=MAXDECODESIZE/2;
    end=m->b_rptr+Readsize;

    for(i=0 ;start<end;start++,i+=2){
        short *s = (short*)&emptyBuffer[i];
        *s=alaw2linear(*start);
    }

    Pcm->data_p = emptyBuffer;
    Pcm->len     = i;
    
    return Readsize;
}

static int ulawDecode(mblkbuf *m,WaveInfoMode *Info,pcmData *Pcm)
{   //compressing rate 1:2 = ulaw : pcm
    unsigned char *start=m->b_rptr;
    int Readsize=m->b_wptr-m->b_rptr;
    unsigned char *end;
    int i;
    if(Readsize > MAXDECODESIZE/2) Readsize=MAXDECODESIZE/2;
    end=m->b_rptr+Readsize;

    for(i=0 ;start<end;start++,i+=2){
        short *s = (short*)&emptyBuffer[i];
        *s=ulaw2linear(*start);
    }

    Pcm->data_p = emptyBuffer;
    Pcm->len     = i;
    
    return Readsize;
}

static int pcmDecode(mblkbuf *m,WaveInfoMode *Info,pcmData *Pcm){
    int Readsize = m->b_wptr-m->b_rptr;
    Pcm->data_p = m->b_rptr;
    Pcm->len    = Readsize;
    return Readsize;
}
