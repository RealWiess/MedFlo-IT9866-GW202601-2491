//#include "itp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "audio.h"
#include "module_audio.h"
#include "ctrlboard.h"
#ifdef __OPENRTOS__
#include "flower.h"
#endif

//#define AUDIO_DEBUG

static bool isAudioPlaying = false;
static int songIndex = No_Audio;

static int AudioPlayerPlayCallback(int state, void* arg) {

#ifdef AUDIO_DEBUG
     printf("[%s] line:%d,state=%d\n", __FUNCTION__,__LINE__,state);
#endif
#ifdef __OPENRTOS__

    switch (state) {
    case PLAY_EOF_FILE:
    case Repeat:
#ifdef AUDIO_DEBUG
        printf("[%s]+ line:%d,songIndex=%d,isAudioPlaying=%d\n", __FUNCTION__,__LINE__,songIndex,isAudioPlaying);
#endif
        isAudioPlaying = false;
        songIndex = No_Audio;
        break;
    }
#endif
    return PLAY_PASS;
}

int PlayAudio(int song) {
    if (songIndex < song && isAudioPlaying) {
        return PLAY_FAIL;
    }
#ifdef AUDIO_DEBUG
    printf("[%s] line:%d,song=%d,songIndex=%d,isAudioPlaying=%d\n", __FUNCTION__,__LINE__,song,songIndex,isAudioPlaying);
#endif

    if (!isAudioPlaying) {
        AudioPlayMusic(filepath[song], (AudioPlayCallback)AudioPlayerPlayCallback);
#ifdef _WIN32
        isAudioPlaying = false;
#else
        isAudioPlaying = true;
#endif
        songIndex = song;
    }

    return PLAY_PASS;
}

void SetAudioVolume(int volume) {
    AudioSetVolume(volume);
}

int SetAudioStop(void) {
    AudioStop();
    isAudioPlaying = false;
    songIndex = No_Audio;
    return PLAY_PASS;
}

bool AudioStatus(void) {
    return isAudioPlaying;
}

void initAudioIndex(void) {

    if (!isAudioPlaying) {
        songIndex = No_Audio;
    }
}

int getAudioIndex(void) {
    return songIndex;
}

