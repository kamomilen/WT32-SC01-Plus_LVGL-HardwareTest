#pragma once

#include <string.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>

/*** Function declaration ***/
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void tryPreviousNetwork();
void setStyle();
void showingFoundWiFiList();
lv_obj_t * lv_label_create_custom(lv_obj_t * parent, lv_align_t align, lv_coord_t x, lv_coord_t y, const char * text);
lv_obj_t * lv_slider_create_custom(lv_obj_t * parent, lv_align_t align, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, int32_t value);
void loadWIFICredentialEEPROM();
void popupMsgBox();
void networkScanner();
void networkConnector();
void LCDSleepTask() ;
void btn_event_cb();
void timerForNetwork(lv_timer_t *timer);
void btn_event_cb(lv_event_t *e);
void updateLocalTime();
void popupMsgBox(String title, String msg);
void button1_event_handler(lv_event_t *e);
void button2_event_handler(lv_event_t *e);
void button3_event_handler(lv_event_t *e);
void button4_event_handler(lv_event_t *e);
void list_event_handler(lv_event_t *e);
void beginWIFITask(void *pvParameters);
void text_input_event_cb(lv_event_t *e);
void buildSettings();
void makeKeyboard();
void buildStatusBar();
void buildPWMsgBox();
void buildCustomContents();
void LCDSleep(void *pvParameters);
void scanWIFITask(void *pvParameters);
void set_slider_text_value(lv_obj_t * trg, int16_t val, char * prefix, char * postfix);
void loadTFCard();
void onReceive(const uint8_t* mac_addr, const uint8_t* data, int data_len);


