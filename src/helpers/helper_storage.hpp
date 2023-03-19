#pragma once

#include <driver/sdspi_host.h>
#include <driver/sdmmc_types.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

bool init_sdspi();
bool _SD_Unmount();
void _SD_FileCreate();
void _SDS_RenameFile();
bool isFileExists();
bool _SDS_Format_FATFS();
bool _SDS_RemoveFile();
esp_err_t _SDS_write_file(const char *path, char *data);
esp_err_t _SDS_read_file(const char *path);
esp_err_t esp_vfs_fat_sdcard_format(const char *base_path, sdmmc_card_t *card);

typedef enum {
  TFCARD_UNMOUNT,
  TFCARD_MOUNTED,
} TFCard_Status_t;
static TFCard_Status_t tfcardStatus = TFCARD_UNMOUNT;

