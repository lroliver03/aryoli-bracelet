/**
 * @file led_char.c
 * @author Lucas Oliver
 * @brief BLE LED characteristic code file
 * @version 0.1
 * @date 2026-05-15
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "bt/char/led_char.h"
#include "common.h"

/* Private variables */

static const char *TAG = "LED-char";
static uint8_t led_state;

/* Public functions */

/**
 * @brief Gets current LED state.
 * 
 * @return Current LED state.
 */
uint8_t get_led_state() { return led_state; }

/**
 * @brief Turns on LED and sets LED state.
 */
void set_led() {
  ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, true));
  led_state = true;
}

/**
 * @brief Turns off LED and clears LED state.
 */
void clr_led() {
  ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, false));
  led_state = false;
}

/**
 * @brief Initializes LED characteristic.
 * 
 * @return Error code. If not ESP_OK, check ESP-IDF documentation.
 */
esp_err_t led_init() {
  ESP_LOGI(TAG, "Initializing LED...");

  esp_err_t esp_ret;
  esp_ret = gpio_reset_pin(LED_GPIO);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Unable to reset LED GPIO (IO%02d)", esp_ret, LED_GPIO);
    return esp_ret;
  }

  esp_ret = gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Unable to set direction of LED GPIO (IO%02d)", esp_ret, LED_GPIO);
    return esp_ret;
  }

  gpio_set_level(LED_GPIO, false);
  led_state = false;

  ESP_LOGI(TAG, "LED initialized! Using GPIO %d.", LED_GPIO);

  return ESP_OK;
}