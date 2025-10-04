#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief LED pattern types
 */
typedef enum {
    LED_PATTERN_OFF,           // LED always off
    LED_PATTERN_ON,            // LED always on
    LED_PATTERN_BLINK_SLOW,    // 1 Hz (500ms on, 500ms off)
    LED_PATTERN_BLINK_FAST,    // 4 Hz (125ms on, 125ms off)
    LED_PATTERN_PULSE,         // Breathing effect
    LED_PATTERN_SOS            // Morse code SOS
} led_pattern_t;

/**
 * @brief LED controller configuration
 */
typedef struct {
    gpio_num_t gpio_num;       // GPIO pin number
    bool active_high;          // true = LED on when GPIO high, false = LED on when GPIO low
} led_config_t;

/**
 * @brief Initialize LED controller
 *
 * @param config Pointer to configuration struct
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t led_init(const led_config_t *config);

/**
 * @brief Set LED pattern
 *
 * @param pattern Pattern to display
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if not initialized
 */
esp_err_t led_set_pattern(led_pattern_t pattern);

/**
 * @brief Get current pattern
 *
 * @return Current pattern
 */
led_pattern_t led_get_pattern(void);

/**
 * @brief Deinitialize LED controller and cleanup resources
 */
void led_deinit(void);

#endif // LED_CONTROLLER_H
