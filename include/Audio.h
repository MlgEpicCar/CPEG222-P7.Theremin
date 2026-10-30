#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

void Audio_Init(void);
void Audio_SetFrequency(uint32_t frequency_hz);
void Audio_Stop(void);
void Audio_UpdateSwitch(void);
uint32_t Audio_UpdateDistance(uint32_t distance_hundredths);
int Audio_GetCurrentNote(void);

#endif