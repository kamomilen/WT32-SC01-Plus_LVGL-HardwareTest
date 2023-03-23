#pragma once
// Debug
#define __DEBUG__

// SD card Working / enable it below
#define WT32_SC01_PLUS
#define SD_SUPPORTED
#define SOC_I2S_SUPPORTS_ADC

// TF CARD - SPI
#define SDSPI_HOST_ID SPI3_HOST
#define TF_MISO       GPIO_NUM_38 
#define TF_MOSI       GPIO_NUM_40
#define TF_SCLK       GPIO_NUM_39
#define TF_CS         GPIO_NUM_41

// Audio - Inter-IC
//#define A_I2S_PORT    I2S_NUM_0
#define A_LRCK        GPIO_NUM_35
#define A_BCLK        GPIO_NUM_36
#define A_DOUT        GPIO_NUM_37

// RS485
#define RS485_RXD     GPIO_NUM_1
#define RS485_RTS     GPIO_NUM_2
#define RS485_TXD     GPIO_NUM_42

// EXT 8Pin (0-3.3V)
#define EXT_01        GPIO_NUM_10
#define EXT_02        GPIO_NUM_11
#define EXT_03        GPIO_NUM_12
#define EXT_04        GPIO_NUM_13
#define EXT_05        GPIO_NUM_14
#define EXT_06        GPIO_NUM_21

// EEPROM
#define EEPROM_SIZE 128
#define EEPROM_ADDR_WIFI_FLAG 0
#define EEPROM_ADDR_WIFI_CREDENTIAL 4

// Portrait
#define TFT_WIDTH   480
#define TFT_HEIGHT  320

// NTP - timezone : [Asia/Tokyo]
#define NTP_SERVER_NAME1    "ntp1.jst.mfeed.ad.jp"
#define NTP_SERVER_NAME2    "ntp2.jst.mfeed.ad.jp"
#define NTP_SERVER_NAME3    "ntp3.jst.mfeed.ad.jp"
#define NTP_GMT_OFFSET_SEC  9 * 3600L
#define NTP_DAYLIGHT_OFFSET_SEC 0