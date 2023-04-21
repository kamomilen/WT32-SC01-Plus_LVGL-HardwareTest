#pragma once

#include <stdio.h>
#include <WString.h>
#include <vector>
#include <esp_err.h>
#include <esp_now.h>
#include "common.hpp"

esp_err_t espnow_init();
esp_err_t onEspNowSendTest(esp_now_peer_info_t slave);
void onEspNowSend(const uint8_t *mac_addr, esp_now_send_status_t status);
void onEspNowReceive(const uint8_t* mac_addr, const uint8_t* data, int data_len);
//std::vector<esp_now_peer_info_t> getEspNowMacAddList();
void trySendCheck();
void trySendCheckTask(void *pvParameters);
