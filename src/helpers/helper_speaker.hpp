
#pragma once

#include <esp_err.h>

#define _rate   44100
esp_err_t speaker_init();
esp_err_t speaker_unload();
//size_t playRAW(const uint8_t *__audioPtr, size_t __size, bool __modal = false, bool freeFlag = true, TickType_t __ticksToWait = portMAX_DELAY);
esp_err_t playDemoAudio();
esp_err_t playLoopAudio();
size_t playBeep(int __freq = 2000, int __timems = 200, int __maxval = 10000, bool __modal = false);
float fastSin(float theta);

typedef struct beepParameters {
    int rate;
    int freq;
    int maxval;
    size_t time;
    // beepParameters() :rate(44100),freq(2000),maxval(10000),time(500){}
    // beepParameters(int rate,int freq,int maxval,size_t time)
    // :rate(rate),freq(freq),maxval(maxval),time(time){}
} beepParameters_t;

typedef struct {
    void *pAudioData;
    int length;
    bool freeFlag;
} audioParameters_t;

typedef enum {
    kTypeNull,
    kTypeAudio,
    kTypeBeep,
} audioPlayStatus_t;

typedef struct {
    int type;
    void *dataptr;
} i2sQueueMsg_t;

typedef struct audioList {
    size_t _num;
    int type;
    void *dataptr;
    audioList *nextPtr;
} audioList_t;

typedef enum {
  Speaker_NONE,
  Speaker_READY,
} Speaker_Status_t;

