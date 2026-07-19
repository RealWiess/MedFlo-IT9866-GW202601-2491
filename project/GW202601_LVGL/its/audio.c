#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "ite/audio.h"
#include "ctrlboard.h"

#ifdef __OPENRTOS__
#include "flower.h"
IteFlower *flower = NULL;
extern void flow_set_dac_level(int);
extern void flow_set_adc_level(int);
extern void flow_stop_sound_play(IteFlower *f);
#endif

extern bool                       audioKeySoundPaused;
void AsrStartOnBackGround(bool yesno);
void AudioInit(void)
{
    #ifdef __OPENRTOS__
    audioKeySoundPaused = false;
    
    flower = IteStreamInit();
    flow_set_dac_level(50);
    flow_set_adc_level(50);

    AsrStartOnBackGround(1);
    #endif
}

void AudioExit(void)
{
    
}

int AudioPlayMusic(char* filename, AudioPlayCallback func)
{
    #ifdef __OPENRTOS__
    flow_set_dac_level(theConfig.audiolevel*5);
    //flow_start_quick_play(flower,filename,Normal,func);
    flow_start_sound_play(flower,filename,Normal,(Callback_t)func);
    #endif
    return 0;
}
 
void AudioPlayKeySound(void)
{
    #ifdef __OPENRTOS__
    char filepath[PATH_MAX];
    
    if(audioKeySoundPaused)
        return;
 
    strcpy(filepath, CFG_PRIVATE_DRIVE ":/sounds/");
    strcat(filepath, theConfig.keysound);
    flow_start_sound_play(flower,filepath,Normal,NULL);
    #endif
}

void AudioStop(void)
{
    #ifdef __OPENRTOS__
    flow_stop_sound_play(flower);
    #endif
}
 
void AudioPauseKeySound(void)
{
    #ifdef __OPENRTOS__
    audioKeySoundPaused = true;
    #endif
}
 
void AudioResumeKeySound(void)
{
    audioKeySoundPaused = false;
}
 
void AudioSetKeyLevel(int level)
{
    #ifdef __OPENRTOS__
    theConfig.keylevel = level;
    flow_set_dac_level(theConfig.keylevel);
    #endif
}
 
void AudioMute(void)
{
}
 
void AudioUnMute(void)
{
}
 
bool AudioIsMuted(void)
{
}
 
bool AudioIsPlaying(void)
{
}
 
void AudioSetVolume(int level)
{
    #ifdef __OPENRTOS__
    theConfig.audiolevel = level;
    flow_set_dac_level(theConfig.audiolevel*5);
    #endif
}
 
void AudioSetLevel(int level)
{
    theConfig.audiolevel = level; 
}
 
int AudioGetVolume(void)
{
    return theConfig.audiolevel;
}
 
void AudioSetRecLevel(int level)
{
    #ifdef __OPENRTOS__
    theConfig.audiolevel = level; 
    flow_set_adc_level(theConfig.audiolevel);
    #endif

}
 
void AsrStartOnBackGround(bool yesno){
	/*fAsrCallback need to implement for user deigned*/
 /*   #ifdef __OPENRTOS__
    if(yesno){
        flow_set_adc_level(theConfig.audiolevel);
        //flow_start_asr(flower,fAsrCallback);
		flow_start_asr(flower,NULL);
    }
    else
        flow_stop_asr(flower);
    #endif*/
}