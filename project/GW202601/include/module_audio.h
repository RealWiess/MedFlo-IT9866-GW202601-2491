#ifndef MODULE_AUDIO_H
#define MODULE_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif
    typedef enum {
        OnekHz_Audio = 0,
        KillBill,
        Suject3,

        No_Audio
    } eAudioPlayFileIndex;

    typedef enum {
        PLAY_FAIL,
        PLAY_PASS,

        PLAY_MAX
    }eAudioPlayStatus;

    static char* filepath[] = {
        CFG_PRIVATE_DRIVE":/sounds/1kHz_44100Hz_16bit_05sec.wav",
        CFG_PRIVATE_DRIVE":/sounds/killbill.mp3",
        CFG_PRIVATE_DRIVE":/sounds/suject3.mp3",
    };

    int PlayAudio(int song);

    void SetAudioVolume(int volume);

    int SetAudioStop(void);

    bool AudioStatus(void);

    void initAudioIndex(void);

    int getAudioIndex(void);

#ifdef __cplusplus
}
#endif

#endif // MODULE_AUDIO_H

