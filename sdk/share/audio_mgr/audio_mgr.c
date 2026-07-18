#include "audio_mgr.h"
#include "ite/audio.h"
#include <malloc.h>
#include <string.h>

#define ERRORLOG() printf("no use %s %d\n",__FUNCTION__,__LINE__)

// JL, 10012016
int32_t Audio_Printf(char *strPtr){
    ERRORLOG();
    return 0;
}

void AudioThreadFunc(){
    ERRORLOG();
}
int32_t smtkAudioMgrInitialize(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrTerminate(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrPlayNetwork(SMTK_AUDIO_PARAM_NETWORK *pNetwork){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrPlay(char *filename){
    ERRORLOG();
    return 0;
}
bool smtkAudioGetInsertSoundStatus(void){
    ERRORLOG();
    return 0;
}
void smtkAudioSetInsertSoundStatus(bool pause){
    ERRORLOG();
}
int32_t smtkAudioMgrQuickStop(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrStop(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrPause(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrContinue(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrSetVolume(int32_t nVolume){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrGetVolume(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrMuteOn(void){
    ERRORLOG();
    return 0;
}
int32_t smtkAudioMgrMuteOff(void){
    ERRORLOG();
    return 0;
}
bool smtkAudioMgrIsMuteOn(void){
    ERRORLOG();
    return 0;
}
SMTK_AUDIO_STATE smtkAudioMgrGetState(void){
    ERRORLOG();
    return 0;
}
// get the spectrum information with UINT8[20*5], get 5 frame's spectrum
uint8_t *smtkAudioMgrGetSpectrum(void){
    ERRORLOG();
    return 0;
}
// set spectrum pointer,if null return -1
int32_t smtkAudioMgrSetSpectrum(uint8_t *buffer){
    ERRORLOG();
    return 0;
}
// 0:re-start spectrum, 1:pause/stop spectrum
int32_t smtkAudioMgrPauseSpectrum(int32_t nPause){
    ERRORLOG();
    return 0;
}
uint32_t smtkAudioMgrGetTime(void){
    ERRORLOG();
    return 0;
}
void smtkAudioMgrSetMode(SMTK_AUDIO_MODE mode){
    ERRORLOG();
}
int32_t smtkAudioMgrSetTime(int32_t nTime){
    ERRORLOG();
    return 0;
}
uint32_t smtkAudioMgrSetTotalTime(int32_t nTime){
    ERRORLOG();
    return 0;
}
uint32_t smtkAudioMgrGetTotalTime(uint32_t *time){
    ERRORLOG();
    return 0;
}

int32_t smtkAudioMgrSetDuration(int32_t nDuration){
    ERRORLOG();
    return 0;
}

int32_t smtkAudioMgrGetDuration(){
    ERRORLOG();
    return 0;
}

void smtkAudioMgrSetParsing(bool status){
    ERRORLOG();
}

int32_t smtkAudioMgrShowSpectrum(int32_t nMode){
    ERRORLOG();
    return 0;
}

int32_t smtkAudioMgrOpenEngine(int32_t nEngineType){
    ERRORLOG();
    return 0;
}

void smtkAudioMgrGetNetworkState(int32_t *pType, int32_t *pErr){
    ERRORLOG();
}

void smtkAudioMgrSetNetworkError(int32_t nErr){
    ERRORLOG();
}

int32_t smtkAudioMgrSampleRate(){
    ERRORLOG();
    return 0;
}

int32_t smtkAudioMgrGetType(){
    ERRORLOG();
    return 0;
}

int32_t (*smtkAudioMgrGetCallbackFunction())(int32_t){
    ERRORLOG();
    return 0;
}

void smtkAudioMgrSetCallbackFunction(int32_t (*pApi)(int32_t)){
    ERRORLOG();
}

char *smtkAudioMgrGetFinishName(){
    ERRORLOG();
    return 0;
}

void smtkAudioMgrReloadSBC(void *func){
    ERRORLOG();
}
