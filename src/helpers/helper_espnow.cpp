#include "helpers/helper_espnow.hpp"

#ifdef EPSNOW_SUPPORTED
#include <WiFi.h>
#include <esp_err.h>
#include <esp_now.h>

std::vector<esp_now_peer_info_t> espnowMacAddList;
std::vector<esp_now_peer_info_t> espnowMacTestTryList;
std::vector<esp_now_peer_info_t> espnowMacNGList;

//esp_now_peer_info_t slave;

TaskHandle_t ntTrySendCheckTaskHandler;

esp_err_t espnow_init() {
    printf("WiFi Mac:%s\n",WiFi.macAddress().c_str());
    printf("ESP-Now Init ...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(true, true);
    esp_err_t rt = esp_now_init();
    if (rt == ESP_OK) {
        printf("Success\n");
    } else {
        printf("Oops!\n");
        return rt;
    }
    esp_now_register_send_cb(onEspNowSend);
    esp_now_register_recv_cb(onEspNowReceive);
    return rt;
}

esp_err_t onEspNowSendTest(esp_now_peer_info_t slave) {
    uint8_t data[2] = {123, 234};
    esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));
    return result;
}

void onEspNowSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    printf("\n");
    printf("Last Packet Send to: %s\n", macStr);
    printf("Last Packet Send Status: ");
    printf(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void onEspNowReceive(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    printf("\n");
    printf("Last Packet Recv from: %s\n", macStr);
    printf("Last Packet Recv Data(%d): ", data_len);
    for (int i = 0; i < data_len; i++) {
        printf("%d ", data[i]);
    }
    printf("\n");

    bool addFlag = true;
    for (std::vector<esp_now_peer_info_t>::iterator item = espnowMacAddList.begin(); item != espnowMacAddList.end(); ++item) {
        char itemMacStr[18];
        snprintf(itemMacStr, sizeof(itemMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            (*item).peer_addr[0], (*item).peer_addr[1], (*item).peer_addr[2], (*item).peer_addr[3], (*item).peer_addr[4], (*item).peer_addr[5]);
        if (strcmp(macStr, itemMacStr) == 0) {
            addFlag = false;
            break;
        }
    }
    if (addFlag) {
        esp_now_peer_info_t slave;
        memset(&slave, 0, sizeof(slave));
        for (int i = 0; i < 6; ++i) {
            slave.peer_addr[i] = mac_addr[i];
        }
        espnowMacAddList.push_back(slave);
    }
}

std::vector<esp_now_peer_info_t> getEspNowMacAddList() {
    return espnowMacAddList;
}

void trySendCheck() {
  xTaskCreate(trySendCheckTask,
              "trySendCheckTask",
              2048,
              NULL,
              1,
              &ntTrySendCheckTaskHandler);
}

void trySendCheckTask(void *pvParameters) {
    while (1) {
        while (espnowMacTestTryList.size() > 1) {
            esp_now_peer_info_t item = espnowMacTestTryList.back();
            espnowMacTestTryList.pop_back();

            char itemMacStr[18];
            snprintf(itemMacStr, sizeof(itemMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                    item.peer_addr[0], item.peer_addr[1], item.peer_addr[2], item.peer_addr[3], item.peer_addr[4], item.peer_addr[5]);
            printf("EspNow Send Test (%s) ...", itemMacStr);
            if (onEspNowSendTest(item) == ESP_OK) {
                espnowMacAddList.push_back(item);
                printf("OK\n");
            } else {
                espnowMacNGList.push_back(item);
                printf("NG\n");
            }

        }
        // wait 10sec
        vTaskDelay(10000);
    }
}

#endif
