#pragma once
// Debug
#define __DEBUG__


// TF CARD - SPI
//#define TF_SUPPORTED
#ifdef TF_SUPPORTED
    #define TFSPI_HOST_ID SPI3_HOST
    #define TF_MISO       GPIO_NUM_38 
    #define TF_MOSI       GPIO_NUM_40
    #define TF_SCLK       GPIO_NUM_39
    #define TF_CS         GPIO_NUM_41
#endif

// Audio - Inter-IC
//#define AUDIO_SUPPORTED
#ifdef AUDIO_SUPPORTED
    #define A_LRCK        GPIO_NUM_35
    #define A_BCLK        GPIO_NUM_36
    #define A_DOUT        GPIO_NUM_37
#endif

// RS485
//#define RS485_SUPPORTED
#ifdef RS485_SUPPORTED
    #define RS485_RXD     GPIO_NUM_1
    #define RS485_RTS     GPIO_NUM_2
    #define RS485_TXD     GPIO_NUM_42
#endif

// EXT IO 8Pin (0-3.3V) 
//#define EXTIO_SUPPORTED
#ifdef EXTIO_SUPPORTED
    #define EXT_01        GPIO_NUM_10
    #define EXT_02        GPIO_NUM_11
    #define EXT_03        GPIO_NUM_12
    #define EXT_04        GPIO_NUM_13
    #define EXT_05        GPIO_NUM_14
    #define EXT_06        GPIO_NUM_21
#endif

// EEPROM
#define EEPROM_SIZE 128
#define EEPROM_ADDR_WIFI_FLAG 0
#define EEPROM_ADDR_WIFI_CREDENTIAL 4

// Portrait
#define TFT_WIDTH   480
#define TFT_HEIGHT  320
#define TFT_BRT_RESTRICT1      180000   // 3min
#define TFT_BRT_RESTRICT2      300000   // 5min
#define TFT_BRT_RESTRICT_OFF   600000   // 10min

// WiFi ESP-NOW
//#define EPSNOW_SUPPORTED

// NTP - timezone : [Asia/Tokyo]
#define NTP_SERVER_NAME1    "ntp1.jst.mfeed.ad.jp"
#define NTP_SERVER_NAME2    "ntp2.jst.mfeed.ad.jp"
#define NTP_SERVER_NAME3    "ntp3.jst.mfeed.ad.jp"
#define NTP_GMT_OFFSET_SEC  9 * 3600L
#define NTP_DAYLIGHT_OFFSET_SEC 0