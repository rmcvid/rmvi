#include "visual.h"
#define AUDIO_PATH "C:/Users/ryanm/Documents/Rmvi/animation/audio/keyboard/"
#define KEYBOARD_SOUND_COUNT 16
#define SAMPLE_RATE 44100
#include "audio.h"
#include <stdio.h>

static Sound clavier[KEYBOARD_SOUND_COUNT];
static bool clavierOk[KEYBOARD_SOUND_COUNT];
static bool clavierLoaded = false;

void audioInitKeyboard(void){
    if (clavierLoaded) return;
    for (int i = 0; i < KEYBOARD_SOUND_COUNT; i++){
        char path[512];
        snprintf(path, sizeof(path), AUDIO_PATH "/touche_%d.wav", i + 1);
        Wave w = LoadWave(path);
        if (!w.data || w.frameCount <= 0)
        {
            TraceLog(LOG_WARNING, "Keyboard sound invalid: %s", path);
            clavierOk[i] = false;
            if (w.data) UnloadWave(w);
            continue;
        }
        WaveFormat(&w, SAMPLE_RATE, 16, 1);
        clavier[i] = LoadSoundFromWave(w);
        UnloadWave(w);
        clavierOk[i] = (clavier[i].frameCount > 0);
        if (!clavierOk[i])
            TraceLog(LOG_WARNING, "Sound conversion failed: %s", path);
    }
    clavierLoaded = true;
}

void audioPlayKeyboard(int n,float targetDuration){
    if (!clavierLoaded) audioInitKeyboard();
    int idx = (int) Clamp(n-1,0,KEYBOARD_SOUND_COUNT);
    if (clavierOk[idx]) {
        float baseDuration = (float)clavier[idx].frameCount / (float) SAMPLE_RATE;
        float speed = baseDuration / targetDuration;
        SetSoundPitch(clavier[idx],speed);
        PlaySound(clavier[idx]);
        //SetSoundPitch(clavier[idx],1);
    }
}


void rmviAudioRenderBoxesAnimed(AnimText *anim){
    if (!anim || !anim->hasChanged) return;
    char path[256];
    audioPlayKeyboard(anim->letterCount,(anim->letterCount) * ANIM_CLAVIE_SPEED);
    anim->hasChanged = false;
}
