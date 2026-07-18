#include <stdio.h>
#include "flower/flower.h"

typedef struct Volume{
	float fenergy;
	float flevpeak;
	float finstenergy;
	float fgain; 				/**< the one really applied, smoothed target_gain version*/
	float fstatic_gain;	/**< the one fixed by the user */
	int   dc_ofs;
	//float gain_k;
	float vol_upramp;
	float vol_fast_upramp;
	float vol_downramp;
	float ftargain; /*the target gain choosed by echo limiter and noise gate*/
    float max_energy;
	int sr;
	int npoints;
	int ng_cutdur; /*noise gate cut time, after last speech detected*/
	int ng_dur;
    int vad_cutdur;
    int vad_dur;
	float fng_thold;
    float fvad_thold;
	float fng_lowgain;
	float fng_gain;
    float fng_tgain;
	bool agc_use;
	bool ng_use;
	bool remove_dc;
	bool fast_upramp;
}Volume;

//init
Volume* VOICE_INIT(void);
void VOICE_set_dB_gain(Volume *v,float dBgain);
void VOICE_set_max_energy(Volume *v,float coef);
void VOICE_set_agc(Volume *v,bool arg);
void VOICE_set_noise_gate(Volume *v,bool arg);
//process
void VOICE_applyProcess(Volume *v,mblk_ite *TxIn);
//uninit
void VOICE_UNINIT(Volume *v);
