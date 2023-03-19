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

#define MOUNT_POINT "/sdcard"
#define EXAMPLE_MAX_CHAR_SIZE    64

// SPI - ESP32-S3
// #define SPI_HOST_ID SPI3_HOST
// #define SD_MISO GPIO_NUM_38 
// #define SD_MOSI GPIO_NUM_40
// #define SD_SCLK GPIO_NUM_39
// #define SD_CS   GPIO_NUM_41

static sdmmc_host_t sdmmc_host;
static sdmmc_card_t* sdcard;
//static sdmmc_command_t* sdcard_cmd;
static const char *TAG = "sdcard";
struct stat st;

const char *file_hello = MOUNT_POINT"/hello.txt";
const char *file_foo = MOUNT_POINT"/foo.txt";

bool init_sdspi()
{
    sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    //device_config.host_id = SPI_HOST_ID;
    device_config.host_id = SDSPI_HOST_ID;
    device_config.gpio_cs = SD_CS;  

    ESP_LOGI(TAG, "Initializing SD card");
    sdmmc_host_t _sdmmc_host = SDSPI_HOST_DEFAULT();
    sdmmc_host = _sdmmc_host;
    _sdmmc_host.slot = device_config.host_id;

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
    //sp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    esp_err_t ret = spi_bus_initialize((spi_host_device_t)sdmmc_host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, (const sdmmc_host_t *)&sdmmc_host, &device_config, &mount_config, &sdcard);
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

bool _SD_Unmount() {
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


esp_err_t _SDS_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

esp_err_t _SDS_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

// テキストファイルを書き込む
void _SD_FileCreate() {
    const char *file_hello = MOUNT_POINT"/hello.txt";
    char data[EXAMPLE_MAX_CHAR_SIZE];
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", sdcard->cid.name);
    esp_err_t ret = _SDS_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return;
    }
}

void _SDS_RenameFile() {


    // 既に存在するファイルは削除する
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
    }

    // リネームの開始
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }
}

// ファイルの存在チェック
bool isFileExists() {
    const char *file_foo = MOUNT_POINT"/foo.txt";
    // if (stat(file_foo, &st) == 0) {
    // }
    esp_err_t ret = _SDS_read_file(file_foo);
    if (ret != ESP_OK) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

// フォーマットは実装されていない
esp_err_t esp_vfs_fat_sdcard_format(const char *base_path, sdmmc_card_t *card) {
    return ESP_OK;
}

// フォーマットは実装されていない
bool _SDS_Format_FATFS() {
    // Format FATFS
    esp_err_t ret = esp_vfs_fat_sdcard_format(MOUNT_POINT, sdcard);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool _SDS_RemoveFile() {
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}



