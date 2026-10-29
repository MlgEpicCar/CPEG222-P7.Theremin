#ifndef EYES_H
#define EYES_H

#include <stdint.h>

void eyes_Init(void);
void eyes_Trigger(void);
uint32_t eyes_GetPulseWidth(void);
float eyes_GetDistanceCM(void);

#endif