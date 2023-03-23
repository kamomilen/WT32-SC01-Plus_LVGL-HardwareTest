#include <stdio.h>
#include <string.h>
#include <vector>
#include "time.h"
#include "WiFi.h"
#include <EEPROM.h>
#include <LovyanGFX.hpp> // main library
#include <lvgl.h>
#include "lv_conf.h"
#include "main.hpp"
#include "common.hpp"
#include "helpers/helper_storage.hpp"
#include "helpers/helper_speaker.hpp"
#include "helpers/helper_display.hpp"

/*** Enum declaration ***/
typedef enum {
  NONE,
  NETWORK_SEARCHING,
  NETWORK_CONNECTED_POPUP,
  NETWORK_CONNECTED,
  NETWORK_CONNECT_FAILED
} Network_Status_t;
Network_Status_t networkStatus = NONE;

typedef enum {
  SCREEN_ON_BRT_RESTRICT,
  SCREEN_ON_BRT_MAX,
  SCREEN_OFF
} Screen_Status_t;
Screen_Status_t screenStatus = SCREEN_ON_BRT_MAX;
static lv_timer_t *screenLastTouchTime = 0;

static TFCard_Status_t tfcardStatus = TFCARD_UNMOUNT;
static Speaker_Status_t speakerStatus = Speaker_NONE;
static audioPlayStatus_t audioPlayStatus = kTypeNull;
//static LGFX tft;

//const char *ntpServer = "ntp.nict.jp";
//const long gmtOffset_sec = 9 * 3600L;  // Set your timezone here [Asia/Tokyo]
//const int daylightOffset_sec = 0;
const char * slider_brightness_prefix = "";
const char * slider_brightness_postfix = "%";
#define _UI_TEMPORARY_STRING_BUFFER_SIZE 32

/*** Setup screen resolution for LVGL ***/
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_style_t label_style;
static lv_obj_t *headerLabel;

static lv_style_t border_style;
static lv_style_t popupBox_style;
static lv_style_t custom_contents_style;
static lv_obj_t *timeLabel;
static lv_obj_t *settings;
static lv_obj_t *settingBtn;
static lv_obj_t *settingCloseBtn;
static lv_obj_t *settingWiFiSwitch;
static lv_obj_t *wfList;
static lv_obj_t *settinglabel;
static lv_obj_t *mboxConnect;
static lv_obj_t *mboxTitle;
static lv_obj_t *mboxPassword;
static lv_obj_t *mboxConnectBtn;
static lv_obj_t *mboxCloseBtn;
static lv_obj_t *keyboard;
static lv_obj_t *popupBox;
static lv_obj_t *popupBoxCloseBtn;
static lv_timer_t *timer;

//custom
static lv_obj_t *label_brt_value;
//static lv_obj_t *label_hum_value;
static lv_obj_t *slider_brightness;
static lv_obj_t *label_slider_brightness_value;
static lv_obj_t *label_exec_msg;

static int16_t tft_max_brightness_value = 0;
static int foundNetworks = 0;
unsigned long networkTimeout = 10 * 1000;
String ssidName, ssidPW;

TaskHandle_t ntScanTaskHandler, ntConnectTaskHandler;
std::vector<String> foundWifiList;


// Variables for touch x,y
#ifdef DRAW_ON_SCREEN
static int32_t x, y;
#endif

void setup(void)
{

  Serial.begin(115200); /* prepare for possible serial debug */

  tft.init(); // Initialize LovyanGFX
  lv_init();  // Initialize lvgl

  // Setting display to landscape
  if (tft.width() < tft.height())
    tft.setRotation(tft.getRotation() ^ 1);

  /* LVGL : Setting up buffer to use for display */
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  /*** LVGL : Setup & Initialize the display device driver ***/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = display_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*** LVGL : Setup & Initialize the input device driver ***/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read;
  lv_indev_drv_register(&indev_drv);

  setStyle();
  makeKeyboard();
  buildStatusBar();
  buildPWMsgBox();
  buildCustomContents();
  buildSettings();
  tryPreviousNetwork();

}

void loop()
{
  if (speakerStatus == Speaker_READY && audioPlayStatus == kTypeAudio) {
    playLoopAudio();
  }
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}

  /*** Display callback to flush the buffer to screen ***/
  void display_flush(lv_disp_drv_t * disp, const lv_area_t *area, lv_color_t *color_p)
  {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    //tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.pushPixels((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
  }

  /*** Touchpad callback to read the touchpad ***/
void touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);

  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;

    // printf("Touch (x,y): (%03d,%03d)\n",touchX,touchY );
  }
}


void tryPreviousNetwork() {

  if (!EEPROM.begin(EEPROM_SIZE)) {
    delay(1000);
    ESP.restart();
  }

  loadWIFICredentialEEPROM();
}

void saveWIFICredentialEEPROM(int flag, String ssidpw) {
  EEPROM.writeInt(EEPROM_ADDR_WIFI_FLAG, flag);
  EEPROM.writeString(EEPROM_ADDR_WIFI_CREDENTIAL, flag == 1 ? ssidpw : "");
  EEPROM.commit();
}

void loadTFCard() {
  switch(tfcardStatus) {
    case TFCARD_UNMOUNT:
      _TF_Mount();
      tfcardStatus = TFCARD_MOUNTED;
      break;
  }
}

void unloadTFCard() {
  switch(tfcardStatus) {
    case TFCARD_MOUNTED:
      _TF_Unmount();
      tfcardStatus = TFCARD_UNMOUNT;
      break;
  }
}

void loadWIFICredentialEEPROM() {
  int wifiFlag = EEPROM.readInt(EEPROM_ADDR_WIFI_FLAG);
  String wifiCredential = EEPROM.readString(EEPROM_ADDR_WIFI_CREDENTIAL);

  if (wifiFlag == 1 && wifiCredential.length() != 0 && wifiCredential.indexOf(" ") != -1) {
    char preSSIDName[30], preSSIDPw[30];
    if (sscanf(wifiCredential.c_str(), "%s %s", preSSIDName, preSSIDPw) == 2) {

      lv_obj_add_state(settingWiFiSwitch, LV_STATE_CHECKED);
      lv_event_send(settingWiFiSwitch, LV_EVENT_VALUE_CHANGED, NULL);

      //popupMsgBox("Welcome Back!", "Attempts to reconnect to the previously connected network.");
      ssidName = String(preSSIDName);
      ssidPW = String(preSSIDPw);
      networkConnector();
    } else {
      saveWIFICredentialEEPROM(0, "");
    }
  }
}

void setStyle() {
  lv_style_init(&border_style);
  lv_style_set_border_width(&border_style, 2);
  lv_style_set_border_color(&border_style, lv_color_black());

  lv_style_init(&popupBox_style);
  lv_style_set_radius(&popupBox_style, 10);
  lv_style_set_bg_opa(&popupBox_style, LV_OPA_COVER);
  lv_style_set_border_color(&popupBox_style, lv_palette_main(LV_PALETTE_BLUE));
  lv_style_set_border_width(&popupBox_style, 5);

  // custom font used.
  lv_style_init(&custom_contents_style);
  lv_style_set_border_width(&custom_contents_style, 2);
  lv_style_set_border_color(&custom_contents_style, lv_color_black());
  lv_style_set_text_font(&custom_contents_style, &lv_font_notosansjp_regular_custom_16_2);
}

 void buildStatusBar() {

  static lv_style_t style_btn;
  lv_style_init(&style_btn);
  lv_style_set_bg_color(&style_btn, lv_color_hex(0xC5C5C5));
  lv_style_set_bg_opa(&style_btn, LV_OPA_50);

  lv_obj_t *statusBar = lv_obj_create(lv_scr_act());
  lv_obj_set_size(statusBar, tft.width(), 30);
  lv_obj_align(statusBar, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_remove_style(statusBar, NULL, LV_PART_SCROLLBAR | LV_STATE_ANY);

  timeLabel = lv_label_create(statusBar);
  lv_obj_set_size(timeLabel, tft.width() - 50, 30);

  lv_label_set_text(timeLabel, "WiFi Not Connected!    " LV_SYMBOL_CLOSE);
  lv_obj_align(timeLabel, LV_ALIGN_LEFT_MID, 8, 4);

  settingBtn = lv_btn_create(statusBar);
  lv_obj_set_size(settingBtn, 30, 30);
  lv_obj_align(settingBtn, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_obj_add_event_cb(settingBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_t *label = lv_label_create(settingBtn); /*Add a label to the button*/
  lv_label_set_text(label, LV_SYMBOL_SETTINGS);  /*Set the labels text*/
  lv_obj_center(label);
}

 void btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED) {
    if (btn == settingBtn) {
      lv_obj_clear_flag(settings, LV_OBJ_FLAG_HIDDEN);
    } else if (btn == settingCloseBtn) {
      lv_obj_add_flag(settings, LV_OBJ_FLAG_HIDDEN);
    } else if (btn == mboxConnectBtn) {
      ssidPW = String(lv_textarea_get_text(mboxPassword));

      networkConnector();
      lv_obj_move_background(mboxConnect);
      popupMsgBox("Connecting!", "Attempting to connect to the selected network.");
    } else if (btn == mboxCloseBtn) {
      lv_obj_move_background(mboxConnect);
    } else if (btn == popupBoxCloseBtn) {
      lv_obj_move_background(popupBox);
    }

  } else if (code == LV_EVENT_VALUE_CHANGED) {
    if (btn == settingWiFiSwitch) {

      if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {

        if (ntScanTaskHandler == NULL) {
          networkStatus = NETWORK_SEARCHING;
          networkScanner();
          timer = lv_timer_create(timerForNetwork, 1000, wfList);
          lv_list_add_text(wfList, "WiFi: Looking for Networks...");
        }

      } else {

        if (ntScanTaskHandler != NULL) {
          networkStatus = NONE;
          vTaskDelete(ntScanTaskHandler);
          ntScanTaskHandler = NULL;
          lv_timer_del(timer);
          lv_obj_clean(wfList);
        }

        if (WiFi.status() == WL_CONNECTED) {
          WiFi.disconnect(true);
          lv_label_set_text(timeLabel, "WiFi Not Connected!    " LV_SYMBOL_CLOSE);
        }
      }
    }
  }
}
void slider_event_brightness(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * target = lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    int16_t val1 = lv_slider_get_value(target);
    int16_t val2 =  int16_t(((int)tft_max_brightness_value * 100 * (int)val1) / 10000);
    tft.setBrightness((uint8_t)val2);
    //tft.setBrightness((uint8_t)val);
    char buf1[8];
    lv_snprintf(buf1, sizeof(buf1), "%d/%d", (int)val2, (int)tft_max_brightness_value);
    lv_label_set_text(label_brt_value, (const char *)buf1);
    set_slider_text_value(label_slider_brightness_value, val1, (char *)slider_brightness_prefix, (char *)slider_brightness_postfix);
    // char buf2[8];
    // lv_snprintf(buf2, sizeof(buf2), "%d", val2);
    // lv_label_set_text(label_hum_value, buf2);
  }
}

void set_slider_text_value(lv_obj_t * trg, int16_t val, char * prefix, char * postfix)
{
    char buf[_UI_TEMPORARY_STRING_BUFFER_SIZE];
    lv_snprintf(buf, sizeof(buf), "%s%d%s", prefix, (int)val, postfix);
    lv_label_set_text(trg, buf);
}

void timerForNetwork(lv_timer_t *timer) {
  LV_UNUSED(timer);

  switch (networkStatus) {

    case NETWORK_SEARCHING:
      showingFoundWiFiList();
      break;

    case NETWORK_CONNECTED_POPUP:
      //popupMsgBox("WiFi Connected!", "Now you'll get the current time soon.");
      networkStatus = NETWORK_CONNECTED;
      configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER_NAME1, NTP_SERVER_NAME2, NTP_SERVER_NAME3);
      break;

    case NETWORK_CONNECTED:

      showingFoundWiFiList();
      updateLocalTime();
      break;

    case NETWORK_CONNECT_FAILED:
      networkStatus = NETWORK_SEARCHING;
      popupMsgBox("Oops!", "Please check your wifi password and try again.");
      break;

    default:
      break;
  }
}

void showingFoundWiFiList() {
  if (foundWifiList.size() == 0 || foundNetworks == foundWifiList.size())
    return;

  lv_obj_clean(wfList);
  lv_list_add_text(wfList, foundWifiList.size() > 1 ? "WiFi: Found Networks" : "WiFi: Not Found!");

  for (std::vector<String>::iterator item = foundWifiList.begin(); item != foundWifiList.end(); ++item) {
    lv_obj_t *btn = lv_list_add_btn(wfList, LV_SYMBOL_WIFI, (*item).c_str());
    lv_obj_add_event_cb(btn, list_event_handler, LV_EVENT_CLICKED, NULL);
    delay(1);
  }

  foundNetworks = foundWifiList.size();
}

lv_obj_t * lv_label_create_custom(lv_obj_t * parent, lv_align_t align, lv_coord_t x, lv_coord_t y, const char * text) {  
  lv_obj_t *labelCustom = lv_label_create(parent);
  lv_obj_set_align(labelCustom, align);
  lv_obj_set_x(labelCustom, x);
  lv_obj_set_y(labelCustom, y);
  lv_label_set_text(labelCustom, text);
  return labelCustom;
}

lv_obj_t * lv_slider_create_custom(lv_obj_t * parent, lv_align_t align, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, int32_t value) {
  lv_obj_t *sliderCustom = lv_label_create(parent);
  lv_obj_set_align(sliderCustom, align);
  lv_obj_set_width(sliderCustom, w);
  lv_obj_set_height(sliderCustom, h);
  lv_obj_set_x(sliderCustom, x);
  lv_obj_set_y(sliderCustom, y);
  lv_slider_set_value(slider_brightness, value, LV_ANIM_OFF);
  return sliderCustom;
}

void buildCustomContents() {
  // Make Body Area
  lv_obj_t *bodyScreen = lv_obj_create(lv_scr_act());
  lv_obj_add_style(bodyScreen, &custom_contents_style, 0);
  lv_obj_set_size(bodyScreen, tft.width(), tft.height() - 34);
  lv_obj_align(bodyScreen, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Label1
  //lv_obj_t *label1 = lv_label_create_custom(bodyScreen, LV_ALIGN_CENTER, 0, -35 * 3, "WT32 SC01 PLUS Sample App Test");
  lv_obj_t *label1 = lv_label_create(bodyScreen);
  lv_obj_set_x(label1, 0);
  lv_obj_set_y(label1, -35 * 3);
  lv_obj_set_align(label1, LV_ALIGN_CENTER);
  lv_label_set_text(label1, "WT32 SC01 PLUS Sample App Test");

  // MaxBRT
  //lv_obj_t *label_temp = lv_label_create_custom(bodyScreen, LV_ALIGN_CENTER, -35, -35 * 2, "MaxBRT:");
  //label_temp_value = lv_label_create_custom(bodyScreen, LV_ALIGN_CENTER, 35, 35 * 2, "-");
  lv_obj_t *label_temp = lv_label_create(bodyScreen);
  lv_obj_set_x(label_temp, -35);
  lv_obj_set_y(label_temp, -35 * 2);
  lv_obj_set_align(label_temp, LV_ALIGN_CENTER);
  lv_label_set_text(label_temp, "BRT:");
  label_brt_value = lv_label_create(bodyScreen);
  lv_obj_set_x(label_brt_value, 35);
  lv_obj_set_y(label_brt_value, -35 * 2);
  lv_obj_set_align(label_brt_value, LV_ALIGN_CENTER);
  lv_label_set_text(label_brt_value, "-");

  // Slider BRT
  //slider_brightness = lv_slider_create_custom(bodyScreen, LV_ALIGN_CENTER, 0, 0, 200, 20, 100);
  //label_slider_brightness_value = lv_label_create_custom(bodyScreen, LV_ALIGN_CENTER, 150, 0, "100%");
  slider_brightness = lv_slider_create(bodyScreen);
  lv_obj_set_width(slider_brightness, 200);
  lv_obj_set_height(slider_brightness, 20);
  lv_obj_set_x(slider_brightness, 0);
  lv_obj_set_y(slider_brightness, -35 * 1);
  lv_obj_set_align(slider_brightness, LV_ALIGN_CENTER);
  lv_slider_set_value(slider_brightness, 100, LV_ANIM_OFF);//100%
  label_slider_brightness_value = lv_label_create(bodyScreen);
  lv_obj_set_x(label_slider_brightness_value, 150);
  lv_obj_set_y(label_slider_brightness_value, -35 * 1);
  lv_obj_set_align(label_slider_brightness_value, LV_ALIGN_CENTER);
  lv_label_set_text(label_slider_brightness_value, "100%");
  lv_obj_add_event_cb(slider_brightness, slider_event_brightness, LV_EVENT_ALL, NULL);

  // Test Button
  int btn_x1 = -180;
  int btn_x2 = btn_x1 + (100 * 1);
  int btn_x3 = btn_x1 + (100 * 2);
  int btn_x4 = btn_x1 + (100 * 3);
  int btn_y1 = 35 * 1;
  int btn_y2 = 35 * 2;
  int btn_y3 = 35 * 3;
  lv_obj_t *button_TF_test1 = lv_btn_create(bodyScreen);
  lv_obj_align(button_TF_test1, LV_ALIGN_CENTER, btn_x1, btn_y1);
  lv_obj_set_size(button_TF_test1, LV_SIZE_CONTENT, 35);
  lv_obj_t *button_label1 = lv_label_create(button_TF_test1);
  lv_label_set_text(button_label1, "SD Test");
  lv_obj_center(button_label1);
  lv_obj_add_event_cb(button_TF_test1, button1_event_handler, LV_EVENT_ALL, NULL);

  lv_obj_t *button_TF_test2 = lv_btn_create(bodyScreen);
  lv_obj_align(button_TF_test2, LV_ALIGN_CENTER, btn_x2, btn_y1);
  lv_obj_set_size(button_TF_test2, LV_SIZE_CONTENT, 35);
  lv_obj_t *button_label2 = lv_label_create(button_TF_test2);
  lv_label_set_text(button_label2, "SPK Test");
  lv_obj_center(button_label2);
  lv_obj_add_event_cb(button_TF_test2, button2_event_handler, LV_EVENT_ALL, NULL);

  lv_obj_t *button_TF_test3 = lv_btn_create(bodyScreen);
  lv_obj_align(button_TF_test3, LV_ALIGN_CENTER, btn_x3, btn_y1);
  lv_obj_set_size(button_TF_test3, LV_SIZE_CONTENT, 35);
  lv_obj_t *button_label3 = lv_label_create(button_TF_test3);
  lv_label_set_text(button_label3, "  -  ");
  lv_obj_center(button_label3);
  lv_obj_add_event_cb(button_TF_test3, button3_event_handler, LV_EVENT_ALL, NULL);

  lv_obj_t *button_TF_test4 = lv_btn_create(bodyScreen);
  lv_obj_align(button_TF_test4, LV_ALIGN_CENTER, btn_x4, btn_y1);
  lv_obj_set_size(button_TF_test4, LV_SIZE_CONTENT, 35);
  lv_obj_t *button_label4 = lv_label_create(button_TF_test4);
  lv_label_set_text(button_label4, "Reset");
  lv_obj_center(button_label4);
  lv_obj_add_event_cb(button_TF_test4, button4_event_handler, LV_EVENT_ALL, NULL);

  label_exec_msg = lv_label_create(bodyScreen);
  lv_obj_set_x(label_exec_msg, 0);
  lv_obj_set_y(label_exec_msg, btn_y2);
  lv_obj_set_align(label_exec_msg, LV_ALIGN_CENTER);
  lv_label_set_text(label_exec_msg, "");
}

void buildSettings() {
  settings = lv_obj_create(lv_scr_act());
  lv_obj_add_style(settings, &border_style, 0);
  lv_obj_set_size(settings, tft.width() - 100, tft.height() - 40);
  lv_obj_align(settings, LV_ALIGN_TOP_RIGHT, -20, 20);

  settinglabel = lv_label_create(settings);
  lv_label_set_text(settinglabel, "Settings " LV_SYMBOL_SETTINGS);
  lv_obj_align(settinglabel, LV_ALIGN_TOP_LEFT, 0, 0);

  settingCloseBtn = lv_btn_create(settings);
  lv_obj_set_size(settingCloseBtn, 30, 30);
  lv_obj_align(settingCloseBtn, LV_ALIGN_TOP_RIGHT, 0, -10);
  lv_obj_add_event_cb(settingCloseBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_t *btnSymbol = lv_label_create(settingCloseBtn);
  lv_label_set_text(btnSymbol, LV_SYMBOL_CLOSE);
  lv_obj_center(btnSymbol);

  settingWiFiSwitch = lv_switch_create(settings);
  lv_obj_add_event_cb(settingWiFiSwitch, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align_to(settingWiFiSwitch, settinglabel, LV_ALIGN_TOP_RIGHT, 60, -10);
  lv_obj_add_flag(settings, LV_OBJ_FLAG_HIDDEN);

  wfList = lv_list_create(settings);
  lv_obj_set_size(wfList, tft.width() - 140, 210);
  lv_obj_align_to(wfList, settinglabel, LV_ALIGN_TOP_LEFT, 0, 30);

  // Set MaxBRT
  tft_max_brightness_value = tft.getBrightness();
  char buf1[8];
  lv_snprintf(buf1, sizeof(buf1), "%d/%d", (int)tft_max_brightness_value, (int)tft_max_brightness_value);
  lv_label_set_text(label_brt_value, buf1);
}

void button1_event_handler(lv_event_t *e) {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (tfcardStatus != TFCARD_UNMOUNT) {
    return;
  }

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    int ret;

    // [1 - Mount]
    printf("[1 - Mount] Start.\n");
    loadTFCard();
    if (tfcardStatus == TFCARD_UNMOUNT) {
      printf("[1 - Mount] UnMount.\n");
      lv_label_set_text(label_exec_msg, "[1 - Mount] UnMount.");
      return;
    } else if (tfcardStatus == TFCARD_MOUNTED) {
      printf("[1 - Mount] Mount.\n");
      tfcardStatus = TFCARD_MOUNTED;
    }
    ESP_LOGI("TEST", "[1 - Mount] Mount.");

    // [2 - SD Format] Test
    // printf("[2 - SD Format] Start.\n");
    // ret = _TF_Format_FATFS();
    // if (ret != ESP_OK) {
    //   printf("[2 - SD Format] NG.\n");
    //   lv_label_set_text(label_exec_msg, "[2 - SD Format] NG.");
    //   return;
    // }
    // ESP_LOGI("TEST", "[2 - SD Format] OK.");
    // printf("[2 - SD Format] OK.\n");

    // [3 - SD Create File]
    const char *test_file_path = MOUNT_POINT"/testfile.txt";
    std::string test_file_data = "abcdefg\nABCDEFG\n1234567\n";
    printf("[3 - SD Create File] Start.\n");
    ret = _TF_WriteFile(test_file_path, test_file_data.c_str());
    if (ret != ESP_OK) {
      printf("[3 - SD Create File] NG.\n");
      lv_label_set_text(label_exec_msg, "[3 - SD Create File] NG.");
      return;
    }
    ESP_LOGI("TEST", "[3 - SD Create File] OK.");
    printf("[3 - SD Create File] OK.\n");

    // [4 - SD File Exists]
    printf("[4 - SD File Exists] Start.\n");
    ret = _TF_IsFileExists(test_file_path);
    if (ret != ESP_OK) {
      printf("[4 - SD File Exists] NG.");
      lv_label_set_text(label_exec_msg, "[4 - SD File Exists] NG.");
      return;
    }
    ESP_LOGI("TEST", "[4 - SD File Exists] OK.");
    printf("[4 - SD File Exists] OK.\n");

    // [5 - SD File Read] Test
    printf("[5 - SD File Read] Start.\n");
    std::string test_file_data_2;
    ret = _TF_ReadFile(test_file_path, &test_file_data_2);
    if (ret != ESP_OK) {
      printf("[5 - SD File Read] File NG.\n");
      lv_label_set_text(label_exec_msg, "[5 - SD File Read] File NG.");
      return;
    } else {
      // printf("1:%s\n", test_file_data.c_str());
      // printf("2:%s\n", test_file_data_2.c_str());
      if (test_file_data != test_file_data_2) {
        printf("[5 - SD File Read] File Diff NG.\n");
        lv_label_set_text(label_exec_msg, "[5 - SD File Read] File Diff NG.");
        return;
      }
    }
    ESP_LOGI("TEST", "[5 - SD File Read] OK.");
    printf("[5 - SD File Read] OK.\n");

    // [6 - SD File Remove]
    const char *test_file_remove_path = MOUNT_POINT"/remove.txt";
    printf("[6 - SD File Remove] Start.\n");
    ret = _TF_RenameFile(test_file_path, test_file_remove_path);
    if (ret != ESP_OK) {
      printf("[6 - SD File Remove] NG.\n");
      lv_label_set_text(label_exec_msg, "[6 - SD File Remove] NG.");
      return;
    }
    ESP_LOGI("TEST", "[6 - SD File Remove] OK.");
    printf("[6 - SD File Remove] OK.\n");

    // [7 - SD File Delete]
    printf("[7 - SD File Delete] Start.\n");
    ret = _TF_RemoveFile(test_file_remove_path);
    if (ret != ESP_OK) {
      printf("[7 - SD File Delete] NG.\n");
      lv_label_set_text(label_exec_msg, "[7 - SD File Delete] NG.");
      return;
    }
    ESP_LOGI("TEST", "[7 - SD File Delete] OK.");
    printf("[7 - SD File Delete] OK.\n");

    // [8 - SD Unmount]
    printf("[8 - SD Unmount] Start.\n");
    ret = _TF_Unmount();
    if (ret != ESP_OK) {
      printf("[8 - SD Unmount] NG.\n");
      lv_label_set_text(label_exec_msg, "[8 - SD Unmount] NG.");
      return;
    }
    ESP_LOGI("TEST", "[8 - SD Unmount] OK.");
    printf("[8 - SD Unmount] OK.\n");

    lv_label_set_text(label_exec_msg, "SDCard Test All OK.");
  }
}

void button2_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    int ret;
    // if (speakerStatus == Speaker_READY) {
    //     printf("[1 - Unload] Start.\n");
    //     speaker_unload();
    //     speakerStatus = Speaker_NONE;
    //     printf("[1 - Unload] End.\n");
    // }

    printf("[1 - Init] Start.\n");
    ret = speaker_init();
    if (ret != ESP_OK) {
      printf("[1 - Init] ERR.\n");
      return;
    }
    speakerStatus = Speaker_READY;
    printf("[1 - Init] OK.\n");

    // printf("[2 - Beep] Start.\n");
    // if (speakerStatus == Speaker_READY) {
    //   ret = playBeep(2500, 2000, 1000);
    //   if (ret != ESP_OK) {
    //     printf("[2 - Beep] Exec ERR.\n");
    //     return;
    //   }
    // } else {
    //   printf("[2 - Beep] Status Error.\n");
    //   return;
    // }
    // printf("[2 - Beep] OK.\n");
    printf("[2 - AudioPlay] Start.\n");
    ret = playDemoAudio();
    if (ret != ESP_OK) {
      printf("[2 - AudioPlay] Exec ERR.\n");
      return;
    }
    printf("[2 - AudioPlay] OK.\n");

    // printf("[3 - Unmount] Start.\n");
    // ret = speaker_unload();
    // if (ret != ESP_OK) {
    //   printf("[3 - Unmount] ERR.\n");
    //   return;
    // }
    // speakerStatus = Speaker_NONE;
    // printf("[3 - Unmount] OK.\n");

    lv_label_set_text(label_exec_msg, "exec speaker_test. All OK");
  }
}
void button3_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {

  }
}
void button4_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    ESP.restart();
  }
}

void list_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);


  if (code == LV_EVENT_CLICKED) {

    String selectedItem = String(lv_list_get_btn_text(wfList, obj));
    for (int i = 0; i < selectedItem.length() - 1; i++) {
      if (selectedItem.substring(i, i + 2) == " (") {
        ssidName = selectedItem.substring(0, i);
        lv_label_set_text_fmt(mboxTitle, "Selected WiFi SSID: %s", ssidName);
        lv_obj_move_foreground(mboxConnect);
        break;
      }
    }
  }
}

/*
 * NETWORK TASKS
 */

void networkScanner() {
  xTaskCreate(scanWIFITask,
              "ScanWIFITask",
              4096,
              NULL,
              1,
              &ntScanTaskHandler);
}

void networkConnector() {
  xTaskCreate(beginWIFITask,
              "beginWIFITask",
              2048,
              NULL,
              1,
              &ntConnectTaskHandler);
}

void scanWIFITask(void *pvParameters) {
  while (1) {
    foundWifiList.clear();
    int n = WiFi.scanNetworks();
    vTaskDelay(10);
    for (int i = 0; i < n; ++i) {
      String item = WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") " + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      foundWifiList.push_back(item);
      vTaskDelay(10);
    }
    vTaskDelay(5000);
  }
}


void beginWIFITask(void *pvParameters) {

  unsigned long startingTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  vTaskDelay(100);

  WiFi.begin(ssidName.c_str(), ssidPW.c_str());
  while (WiFi.status() != WL_CONNECTED && (millis() - startingTime) < networkTimeout) {
    vTaskDelay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    networkStatus = NETWORK_CONNECTED_POPUP;
    saveWIFICredentialEEPROM(1, ssidName + " " + ssidPW);
  } else {
    networkStatus = NETWORK_CONNECT_FAILED;
    saveWIFICredentialEEPROM(0, "");
  }

  vTaskDelete(NULL);
}

void buildPWMsgBox() {

  mboxConnect = lv_obj_create(lv_scr_act());
  lv_obj_add_style(mboxConnect, &border_style, 0);
  lv_obj_set_size(mboxConnect, tft.width() * 2 / 3, tft.height() / 2);
  lv_obj_center(mboxConnect);

  mboxTitle = lv_label_create(mboxConnect);
  lv_label_set_text(mboxTitle, "Selected WiFi SSID: ThatProject");
  lv_obj_align(mboxTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  mboxPassword = lv_textarea_create(mboxConnect);
  lv_obj_set_size(mboxPassword, tft.width() / 2, 40);
  lv_obj_align_to(mboxPassword, mboxTitle, LV_ALIGN_TOP_LEFT, 0, 30);
  lv_textarea_set_placeholder_text(mboxPassword, "Password?");
  lv_obj_add_event_cb(mboxPassword, text_input_event_cb, LV_EVENT_ALL, keyboard);

  mboxConnectBtn = lv_btn_create(mboxConnect);
  lv_obj_add_event_cb(mboxConnectBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(mboxConnectBtn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_t *btnLabel = lv_label_create(mboxConnectBtn);
  lv_label_set_text(btnLabel, "Connect");
  lv_obj_center(btnLabel);

  mboxCloseBtn = lv_btn_create(mboxConnect);
  lv_obj_add_event_cb(mboxCloseBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(mboxCloseBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_t *btnLabel2 = lv_label_create(mboxCloseBtn);
  lv_label_set_text(btnLabel2, "Cancel");
  lv_obj_center(btnLabel2);
}

void text_input_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    lv_obj_move_foreground(keyboard);
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  }

  if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  }
}

void makeKeyboard() {
  keyboard = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

void popupMsgBox(String title, String msg) {

  if (popupBox != NULL) {
    lv_obj_del(popupBox);
  }

  popupBox = lv_obj_create(lv_scr_act());
  lv_obj_add_style(popupBox, &popupBox_style, 0);
  lv_obj_set_size(popupBox, tft.width() * 2 / 3, tft.height() / 2);
  lv_obj_center(popupBox);

  lv_obj_t *popupTitle = lv_label_create(popupBox);
  lv_label_set_text(popupTitle, title.c_str());
  lv_obj_set_width(popupTitle, tft.width() * 2 / 3 - 50);
  lv_obj_align(popupTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *popupMSG = lv_label_create(popupBox);
  lv_obj_set_width(popupMSG, tft.width() * 2 / 3 - 50);
  lv_label_set_text(popupMSG, msg.c_str());
  lv_obj_align(popupMSG, LV_ALIGN_TOP_LEFT, 0, 40);

  popupBoxCloseBtn = lv_btn_create(popupBox);
  lv_obj_add_event_cb(popupBoxCloseBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(popupBoxCloseBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_t *btnLabel = lv_label_create(popupBoxCloseBtn);
  lv_label_set_text(btnLabel, "Okay");
  lv_obj_center(btnLabel);
}

void updateLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  char hourMin[6];
  strftime(hourMin, 6, "%H:%M", &timeinfo);
  String hourMinWithSymbol = String(hourMin);
  hourMinWithSymbol += "   ";
  hourMinWithSymbol += LV_SYMBOL_WIFI;
  if (tfcardStatus == TFCARD_MOUNTED) {
    hourMinWithSymbol += "   ";
    hourMinWithSymbol += LV_SYMBOL_SD_CARD;
  }
  if (speakerStatus == Speaker_READY && audioPlayStatus == kTypeAudio) {
    hourMinWithSymbol += "   ";
    hourMinWithSymbol += LV_SYMBOL_AUDIO;
    hourMinWithSymbol += " (vol.10/27) ";
  }
  lv_label_set_text(timeLabel, hourMinWithSymbol.c_str());
}


