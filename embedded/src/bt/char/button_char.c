#include "esp_timer.h"
#include "bt/char/button_char.h"
#include "common.h"

/* Private variables */

static TaskHandle_t button_indicate_task_handle = NULL;
static const char *TAG = "BUTTON-char";
static volatile uint8_t button_state;
static volatile uint64_t last_isr_time = 0;

/* ISR handler */

static void IRAM_ATTR button_isr(void *arg) {
  uint64_t now = esp_timer_get_time();

  // Debouncing via timer
  if (now - last_isr_time > BUTTON_DEBOUNCE_DELAY_US) {
    // Update button state variable
    button_state = gpio_get_level(BUTTON_GPIO);

    // Resume button indicate task through handle
    xTaskResumeFromISR(button_indicate_task_handle);
  }
}

/* Public functions */

/**
 * @brief Gets current button state.
 * 
 * @return Current button state.
 */
uint8_t get_button_state() { return button_state; }

/**
 * @brief Initializes button characteristic.
 * 
 * @param [out] queue_out Address of pointer to button state ISR queue.
 * 
 * @return Error code. If not ESP_OK, check ESP-IDF documentation.
 */
esp_err_t button_init() {
  ESP_LOGI(TAG, "Initializing button...");

  // Reset pin
  esp_err_t esp_ret;
  esp_ret = gpio_reset_pin(BUTTON_GPIO);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Unable to reset button GPIO (IO%02d)", esp_ret, BUTTON_GPIO);
    return esp_ret;
  }

  // Configure pin
  gpio_config_t io_conf = {
    .intr_type = GPIO_INTR_NEGEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << BUTTON_GPIO),
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_ENABLE
  };
  esp_ret = gpio_config(&io_conf);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Unable to configure button GPIO (IO%02d)", esp_ret, BUTTON_GPIO);
    return esp_ret;
  }
  
  // Create button handler task
  xTaskCreate(button_indicate_task, "button_indicate_task", 4*1024, NULL, 10, &button_indicate_task_handle);

  // Initialize timer for ISR debouncing
  esp_ret = esp_timer_init();
  if (esp_ret == ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "ESP timer already initialized.");
  } else if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Failed to initialize ESP timer.", esp_ret);
    return esp_ret;
  }

  // Attach interrupt to button
  esp_ret = gpio_install_isr_service(0);
  if (esp_ret == ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "ISR service already installed.");
  } else if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Failed to install ISR service.", esp_ret);
    return esp_ret;
  }

  esp_ret = gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Failed to add ISR handler.", esp_ret);
    return esp_ret;
  }

  ESP_LOGI(TAG, "Button initialized! Using GPIO %d.", BUTTON_GPIO);

  return ESP_OK;
}