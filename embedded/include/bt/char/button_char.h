#ifndef BUTTON_CHAR_H
#define BUTTON_CHAR_H

/* Includes */
// ESP APIs
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Defines */

#define BUTTON_GPIO CONFIG_BUTTON_GPIO
#define BUTTON_DEBOUNCE_DELAY_US CONFIG_BUTTON_DEBOUNCE_DELAY_US

/* Public function prototypes */

uint8_t get_button_state();
esp_err_t button_init();
void button_indicate_task(void *arg); // Defined in "bt/gatt_svc.c"
void gatt_svr_button_subscribe_reset(); // Defined in "bt/gatt_svc.c"

#endif