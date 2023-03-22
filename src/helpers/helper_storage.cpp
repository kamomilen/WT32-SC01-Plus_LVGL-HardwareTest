// SD Card on SPI bus

//#include <SD.h>
//#include <SPI.h>
#include <driver/sdspi_host.h>
#include <driver/sdmmc_host.h>
#include <driver/sdmmc_types.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include "common.hpp"
#include "main.hpp"
#include "helper_storage.hpp"

static sdmmc_host_t sdmmc_host;
static sdmmc_card_t* sdcard;
const char *TAG = "sdcard";

esp_err_t _SD_Mount()
{
    sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    device_config.host_id = SDSPI_HOST_ID;
    device_config.gpio_cs = SD_CS;

    ESP_LOGI(TAG, "Initializing SD card");
    sdmmc_host_t _sdmmc_host = SDSPI_HOST_DEFAULT();
    _sdmmc_host.slot = device_config.host_id;
    //sdmmc_host = _sdmmc_host;

    esp_vfs_fat_mount_config_t mount_config =
    {
        //.format_if_mount_failed = true,
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "Initializing SPI BUS");
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };
    esp_err_t ret = spi_bus_initialize(SDSPI_HOST_ID, &bus_cfg, SDSPI_DEFAULT_DMA);
    //sdmmc_host = _sdmmc_host;
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, (const sdmmc_host_t *)&_sdmmc_host, &device_config, &mount_config, &sdcard);
    sdmmc_host = _sdmmc_host;
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, enable above in mount_config.");
            return ESP_FAIL;
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
            return ESP_FAIL;
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, sdcard);

    return ESP_OK;
}

esp_err_t _SD_Unmount() {

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, sdcard);
    ESP_LOGI(TAG, "Card unmounted");

    //deinitialize the bus after all devices are removed
    esp_err_t ret = spi_bus_free((spi_host_device_t)sdmmc_host.slot);
    if (ret != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

// SDCard File Write
esp_err_t _SD_WriteFile(const char *path, std::string data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data.c_str());
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

// SDCard File Read
esp_err_t _SD_ReadFile(const char *path, std::string *data)
{
    std::string str;
    char buffer[READ_MAX_CHAR_SIZE];
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    while(fgets(buffer, READ_MAX_CHAR_SIZE, fp) != NULL) {
        if(*buffer && buffer[strlen(buffer) - 1] == '\n'){
            buffer[strlen(buffer) - 1] = 0;
            str += buffer;
            str += "\n";
        } else {
            str += buffer;
        }
    }
    fclose(fp);
    *data = str.c_str();

    return ESP_OK;
}

// SDCard File existence check
esp_err_t _SD_IsFileExists(const char *file_path) {
    // Open 出来ればファイルが存在すると判定することにする
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    fclose(f);
    return ESP_OK;
}

// SDCard FAT Format[format not implemented]
esp_err_t _SD_Format_FATFS() {
    // Format FATFS
    esp_err_t ret = esp_vfs_fat_sdcard_format(MOUNT_POINT, sdcard);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    return ESP_OK;
}
// [format not implemented]
esp_err_t esp_vfs_fat_sdcard_format(const char *base_path, sdmmc_card_t *card) {
    return ESP_OK;
}

// SDCard File Remove
esp_err_t _SD_RemoveFile(const char *file_from, const char *file_to) {
    ESP_LOGI(TAG, "Renaming file %s to %s", file_from, file_to);
    if (rename(file_from, file_to) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// not implemented
esp_err_t _SD_DeleteFile(const char *path) {
    return ESP_OK;
}


