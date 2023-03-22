// https://github.com/riraosan/ESP32_BT-A2DP_Receiver/blob/master/src/Application.h
#include <cmath>
#include <Arduino.h>
#include <driver/i2s.h>
#include <queue.h>
#include "common.hpp"
#include "helpers/helper_speaker.hpp"

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

enum playType {
    kTypeNull = 0,
    kTypeAudio,
    kTypeBeep,
};

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

static i2s_port_t a_i2s_port_t = I2S_NUM_0;
void speaker_init() {
    if (speakerStatus == Speaker_READY) {
        i2s_driver_uninstall(a_i2s_port_t);
        speakerStatus = Speaker_NONE;
    }

    i2s_pin_config_t pin_config = {
        .bck_io_num   = A_BCLK,
        .ws_io_num    = A_LRCK,
        .data_out_num = A_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE  // Use in i2s_pin_config_t for pins which should not be changed
    };

    i2s_config_t i2s_config = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = 44100,                       // corrected by info from bluetooth
        .bits_per_sample      = (i2s_bits_per_sample_t)16,   // set_bits_per_sample()
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,  // 2-channels
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,   // I2S communication format I2S
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,        // default interrupt priority
        .dma_buf_count        = 8,                           // default
        .dma_buf_len          = 64,                          // default
        .use_apll             = false,                       // I2S using APLL as main I2S clock, enable it to get accurate clock
        .tx_desc_auto_clear   = true                         // I2S auto clear tx descriptor if there is underflow condition
    };

    i2s_driver_install(a_i2s_port_t, &i2s_config, 0, NULL);
    i2s_set_pin(a_i2s_port_t, &pin_config);
    i2s_set_clk(a_i2s_port_t, i2s_config.sample_rate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    speakerStatus = Speaker_READY;
}

size_t playBeep(int __freq, int __timems, int __maxval, bool __modal) {
    size_t writeSize = 0;

    if (speakerStatus != Speaker_READY) {
        return -1;
    }

    // if (__modal == false) {
    //     beepParameters_t *pam =
    //         (beepParameters_t *)malloc(sizeof(beepParameters_t));
    //     pam->freq   = __freq;
    //     pam->time   = __timems;
    //     pam->rate   = _rate;
    //     pam->maxval = __maxval;

    //     i2sQueueMsg_t msg = {.type = kTypeBeep, .dataptr = pam};
    //     xQueueSend(i2sstateQueue, &msg, (TickType_t)0);
    // } else {
        size_t bytes_written = 0;
        size_t count = 0, length = 0;

        double t = (1 / (double)__freq) * (double)_rate;

        if (__timems > 1000) {
            length = _rate * (__timems % 1000) / 1000;
            count  = __timems / 1000;
        } else {
            length = _rate * __timems / 1000;
            count  = 0;
        }
        int rawLength = (count == 0) ? length : _rate;

        uint16_t *raw = (uint16_t *)ps_calloc(rawLength, sizeof(uint16_t));

        for (int i = 0; i < rawLength; i++) {
            int val = 0;
            if (i < 100) {
                val = __maxval * i / 100;
            } else if (i > (rawLength - 1000)) {
                val = __maxval - __maxval * (1000 - (rawLength - i)) / 1000;
            } else {
                val = __maxval;
            }
            raw[i] = (uint16_t)((fastSin(360 / t * i)) * val);
        }

        for (int i = 0; i < count; i++) {
            i2s_write(a_i2s_port_t, raw, _rate, &bytes_written,
                      portMAX_DELAY);
        }
        if (length != 0) {
            i2s_write(a_i2s_port_t, raw, length, &bytes_written,
                      portMAX_DELAY);
        }
        delete raw;
    // }
    return writeSize;
}
// From http://stackoverflow.com/questions/6091837/sin-and-cos-are-slow-is-there-an-alternatve
float fastSin(float theta) {
    auto B = 4.0f / M_PI;
    auto C = -4.0f / (M_PI*M_PI);
    auto P = 0.225f;

    auto y = (B  + C * theta) * theta;
    return P * (y * std::abs(y) - y) + y;
}

// void speaker_beep() {
//     i2s_write(Speak_I2S_NUMBER, buff, DATA_SIZE, &bytes_written, portMAX_DELAY);
// }