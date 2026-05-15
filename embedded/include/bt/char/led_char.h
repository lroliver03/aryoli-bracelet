#ifndef LED_CHAR_H
#define LED_CHAR_H

/* Includes */
// ESP APIs
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Defines */
#define LED_GPIO CONFIG_LED_GPIO

/* Public function prototypes */
uint8_t get_led_state();
void set_led();
void clr_led();
esp_err_t led_init();

#endif