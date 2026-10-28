#ifndef BEHOLDER_AUDIO_H
#define BEHOLDER_AUDIO_H
void AudioInit(void);
void AudioStop(void);
void AudioToggle(void);
void AudioCycle(void);
void AudioEffect(unsigned char kind);
void AudioEnding(void);
void AudioRestart(void);
void AudioSelect(unsigned char index);
extern volatile unsigned char music_backend,music_found,music_enabled,music_loops;
extern volatile unsigned int music_clock;
#define FX_MOVE 1
#define FX_BLOCK 2
#define FX_HIT 3
#define FX_KILL 4
#define FX_KEY 5
#define FX_DOOR 6
#define FX_HEAL 7
#define FX_WIN 8
#define FX_DEATH 9
#endif
