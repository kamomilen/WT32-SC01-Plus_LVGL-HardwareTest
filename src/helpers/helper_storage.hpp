#pragma once

#include <driver/sdspi_host.h>
#include <driver/sdmmc_types.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define MOUNT_POINT "/sdcard"
#define READ_MAX_CHAR_SIZE    64

esp_err_t _SD_Mount();
esp_err_t _SD_Unmount();
esp_err_t _SD_IsFileExists(const char * file_path);
esp_err_t _SD_Format_FATFS();
esp_err_t _SD_RemoveFile(const char *path);
esp_err_t _SD_RenameFile(const char *file_from, const char *file_to);
esp_err_t _SD_WriteFile(const char *path, std::string data);
esp_err_t _SD_ReadFile(const char *path, std::string *data);
esp_err_t esp_vfs_fat_sdcard_format(const char *base_path, sdmmc_card_t *card);

typedef enum {
  TFCARD_UNMOUNT,
  TFCARD_MOUNTED,
} TFCard_Status_t;


