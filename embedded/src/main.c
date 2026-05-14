/* Includes */
#include <string.h>

#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "common.h"
#include "gap.h"

/* Private variables */

static const char *TAG = "BLE-main";

/* Private function prototypes */

static void on_stack_reset(int reason);
static void on_stack_sync();
static void nimble_host_config_init();
static void nimble_host_task(void *param);

// I don't know why this function has to be declared, but it has to be declared.
void ble_store_config_init();

/* Private functions */

static void on_stack_reset(int reason) {
  ESP_LOGI(TAG, "NimBLE stack reset.");
  ESP_LOGI(TAG, "  Reset reason: %d", reason);
}

static void on_stack_sync() {
  // When sync, initialize advertisement.
  advertisement_init();
}

static void nimble_host_config_init() {
  // Set host callbacks
  ble_hs_cfg.reset_cb = on_stack_reset;
  ble_hs_cfg.sync_cb = on_stack_sync;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  // Store host configuration
  ble_store_config_init();
}

/* Tasks */

static void nimble_host_task(void *param) {
  ESP_LOGI(TAG, "NimBLE host task has been started!");

  // This function doesn't return until nimble_stop_port is executed
  nimble_port_run();

  // Cleanup
  vTaskDelete(NULL);
}

/* Main task */
void app_main() {
  esp_err_t esp_ret;

    // Initialize NVS flash
  esp_ret = nvs_flash_init(); // First attempt

  // If no space or new version, erase and try again
  if (esp_ret == ESP_ERR_NVS_NO_FREE_PAGES || esp_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    esp_ret = nvs_flash_init(); // Second attampt
  }
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Failed to initialize NVS flash.", esp_ret);
    abort();
  }

  // Initialize NimBLE host stack
  esp_ret = nimble_port_init();
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Failed to initialize NimBLE stack.", esp_ret);
    abort();
  }

  // Initialize GAP service
  esp_ret = gap_init();
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Failed to initialize GAP service.", esp_ret);
    abort();
  }

  // Initialize NimBLE configuration
  nimble_host_config_init();

  // Initialize application
  xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);

  // Run thread

  while (1) {
    // ESP_LOGD(TAG, "This thread is empty. Go back to development.");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}