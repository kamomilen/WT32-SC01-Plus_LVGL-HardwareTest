
#pragma once

#include <stddef.h>

#define _rate   44100
void speaker_init();
//size_t playRAW(const uint8_t *__audioPtr, size_t __size, bool __modal = false, bool freeFlag = true, TickType_t __ticksToWait = portMAX_DELAY);
size_t playBeep(int __freq = 2000, int __timems = 200, int __maxval = 10000, bool __modal = false);
float fastSin(float theta);

typedef enum {
  Speaker_NONE,
  Speaker_READY,
} Speaker_Status_t;
static Speaker_Status_t speakerStatus = Speaker_NONE;
