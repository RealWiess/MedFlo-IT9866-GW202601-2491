#include <math.h>
#include "include/volumeapi.h"
#include "flower/flower.h"

static const float    maxE = (32768 * 0.3); /*is RMS factor */
static const float    coef = 0.2; /* floating averaging coeff. for energy */
// static const float gain_k = 0.02; /* floating averaging coeff. for gain */
static const float    vol_upramp   = 0.4;
static const float    vol_downramp = 0.45; /* not yet runtime parameterizable */
static const float    ng_lowgain   = 0.005;
static const float    agc_thold    = 0.5;

static inline int16_t saturate (int val)
{
    return (val > 32767) ? 32767 : ((val < -32767) ? -32767 : val);
}

static void renew_energy (int16_t * point, int npoints, Volume * v)
{
    int   i;
    float facc = 0;
    float fen;
    int   tpeak = 0, peak = 0;

    for (i = 0; i < npoints; ++i)
    {
        int s = point[i];
        facc += s * s;

        tpeak = abs(s);
        if (tpeak > peak)
        {
            peak = tpeak;
        }
    }
    fen         = (sqrt(facc / npoints) + 1) / v->max_energy;
    v->fenergy  = (fen * coef) + v->fenergy * (1.0 - coef);
    v->flevpeak = (float)peak / v->max_energy;
    v->finstenergy =
        fen; // currently non-averaged energy seems better (short artefacts)
}

static float volume_agc_process (Volume * v, mblk_ite * om)
{
    float gain_reduce = (agc_thold + v->flevpeak) / 1;
    if (gain_reduce > 1)
    {
        gain_reduce = 1;
    }
    return gain_reduce;
}

static void update_gain (Volume * v, mblk_ite * m, float tgain)
{
    int16_t * point;
    int       accDC = 0;
    int32_t   intgain;
    float     gain;

    if (v->fgain < tgain)
    {
        if (v->fgain < v->fng_lowgain)
        {
            v->fgain = v->fng_lowgain;
        }

        v->fgain *= 1 + (v->fast_upramp ? v->vol_fast_upramp : v->vol_upramp);
        if (v->fgain > tgain)
        {
            v->fgain = tgain;
        }
    }
    else if (v->fgain > tgain)
    {
        v->fgain *= 1 - v->vol_downramp;

        if (v->fgain < tgain)
        {
            v->fgain = tgain;
        }
        v->fast_upramp = false;
    }

    gain    = v->fgain * v->fng_gain;
    intgain = gain * 4096;

    if (v->remove_dc)
    {
        for (point = (int16_t *)m->b_rptr; point < (int16_t *)m->b_wptr;
             ++point)
        {
            accDC += *point;
            *point = saturate(((*point - v->dc_ofs) * intgain) / 4096);
        }
        /* offset smoothing */
        v->dc_ofs = (v->dc_ofs * 7 + accDC * 2 / (m->b_wptr - m->b_rptr)) / 8;
    }
    else if (gain != 1)
    {
        for (point = (int16_t *)m->b_rptr; point < (int16_t *)m->b_wptr;
             ++point)
        {
            *point = saturate(((*point) * intgain) / 4096);
        }
    }
}

static void volume_ng_process (Volume * v, float energy, mblk_ite * om)
{
    int npoints = ((om->b_wptr - om->b_rptr) / 2);
    if (energy > v->fng_thold)
    {
        v->ng_dur = v->ng_cutdur;
        if (v->fng_tgain < .2)
        {
            v->fng_tgain += 0.05;
        }
        else if (v->fng_tgain > .2 && v->fng_tgain < 1.0)
        {
            v->fng_tgain += 0.1;
            v->fng_thold = 0.025;
        }
        else
        {
            v->fng_tgain = 1;
            v->fng_thold = 0.025;
        }
    }
    else
    {

        if (v->ng_dur > 0)
        {
            v->ng_dur -= (npoints * 1000) / v->sr;
            // noise buffer time:;
        }
        else
        {
            if (v->fng_tgain > 0.1)
            {

                v->fng_tgain -= 0.05;
                v->fng_thold = 0.03;
            }
            else
            {
                v->fng_thold = 0.05;
                v->fng_tgain = 0;
            }
        }
    }

    v->fng_gain = v->fng_tgain;
    // v->fng_gain = v->fng_gain*0.75 + v->fng_tgain*0.25;
    // v->fng_gain = v->fng_gain*0.25 + v->fng_tgain*0.75;
}

Volume * VOICE_INIT (void)
{
    Volume * v = (Volume *)ite_new(Volume, 1);
    if (v != NULL)
    {
        v->fenergy      = 0;
        v->flevpeak     = 0;
        v->fstatic_gain = v->fgain = v->ftargain = 1;
        v->dc_ofs                                = 0;
        v->vol_upramp                            = vol_upramp;
        v->vol_fast_upramp                       = vol_upramp * 3;
        v->vol_downramp                          = vol_downramp;
        v->agc_use                               = false;
        v->sr                                    = CFG_AUDIO_SAMPLING_RATE;
        v->npoints                               = 80;
        v->ng_use                                = true;
        v->ng_cutdur   = 100; /*TODO: ng_sustain (milliseconds)*/
        v->ng_dur      = 0;
        v->fng_thold   = 0.05;
        v->vad_cutdur  = 400; /*TODO: vad_sustain (milliseconds)*/
        v->vad_dur     = 0;
        v->fvad_thold  = 0.05;
        v->fng_lowgain = ng_lowgain;
        v->fng_gain    = 1;
        v->fng_tgain   = 0;
        v->remove_dc   = false;
        v->max_energy  = maxE;

        // preprocess
        if (v->fng_lowgain < ng_lowgain)
        {
            v->fng_lowgain = ng_lowgain;
        }
        if (v->ng_use)
        {
            v->fgain = v->ftargain =
                v->fng_lowgain; // start with floorgain (soft start)
        }

        v->npoints     = (int)(0.01 * (float)v->sr);
        v->fast_upramp = true;
    }
    return v;
}

int VOICE_vaddetect (Volume * v, mblk_ite * om)
{
    int                result = 1;
    // int i;
    float              acc    = 0;
    float              energy;
    int16_t *          point;
    static const float maxE    = (32768 * 0.7);
    int                npoints = (om->b_wptr - om->b_rptr) / 2;
    // printf("npoints = %d\n ",npoints);

    for (point = (int16_t *)om->b_rptr; point < (int16_t *)om->b_wptr; ++point)
    {
        acc += (*point) * (*point);
    }
    energy     = (sqrt(acc / npoints) + 1) / maxE;
    v->fenergy = (energy * coef) + v->fenergy * (1.0 - coef);

    if (v->fenergy > v->fvad_thold)
    {
        if (v->vad_dur < v->vad_cutdur)
        {
            v->vad_dur += (npoints * 1000) / v->sr;
            if (v->vad_dur < v->vad_cutdur / 2)
            {
                result = 0;
            }
        }
        else
        {
            v->vad_dur = v->vad_cutdur;
        }
    }
    else
    {
        if (v->vad_dur > 0)
        {
            v->vad_dur -= (npoints * 1000) / v->sr;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

void VOICE_applyProcess (Volume * v, mblk_ite * TxIn)
{

    float ftargain;
    renew_energy((int16_t *)TxIn->b_rptr, (TxIn->b_wptr - TxIn->b_rptr) / 2, v);
    ftargain = v->fstatic_gain;

    if (v->agc_use)
    {
        ftargain /= volume_agc_process(v, TxIn);
    }

    if (v->ng_use)
    {
        volume_ng_process(v, v->finstenergy, TxIn);
    }

    update_gain(v, TxIn, ftargain);
}

void VOICE_UNINIT (Volume * v)
{
    free(v);
}

void VOICE_set_dB_gain (Volume * v, float dBgain)
{
    // set dB gain 0.0~max
    v->fgain = v->fstatic_gain = pow(10, (dBgain) / 10);
    // printf("dBgain=%f,v->fstatic_gain=%f\n",dBgain,v->fstatic_gain);
}

void VOICE_set_agc (Volume * v, bool arg)
{
    v->agc_use = arg;
}

void VOICE_set_noise_gate (Volume * v, bool arg)
{
    v->ng_use = arg;
}

void VOICE_set_vad_threshold (Volume * v, float arg)
{
    v->fvad_thold = arg; // 0~1 default:0.05
}
/*
same as msvolume.c filter
volume_api is light process
------
*init*
------
volume v;
v=VOICE_INIT();
VOICE_set_dB_gain(v,3.0);
VOICE_set_noise_gate(v,1);
VOICE_set_agc(v,1);
---------
*process*
---------
mblk_ite *om;
...
VOICE_applyProcess(v,om);
--------------------------------
--------
*uninit*
--------
VOICE_UNINIT(v)
*/

/*easy fade In/Out during frame*/
void sw_fadeOut (mblk_ite * m)
{
    int i;
    int len = (m->b_wptr - m->b_rptr) / 2;
    for (i = 0; m->b_rptr < m->b_wptr; m->b_rptr += 2, i++)
    {
        *((int16_t *)(m->b_rptr)) =
            ((float)(len - i) / (float)(len)) * ((int)*(int16_t *)m->b_rptr);
    }
    m->b_rptr = m->b_datap;
}

void sw_fadeIn (mblk_ite * m)
{
    int i;
    int len = (m->b_wptr - m->b_rptr) / 2;
    for (i = 0; m->b_rptr < m->b_wptr; m->b_rptr += 2, i++)
    {
        *((int16_t *)(m->b_rptr)) =
            ((float)(i) / (float)(len)) * ((int)*(int16_t *)m->b_rptr);
    }
    m->b_rptr = m->b_datap;
}
