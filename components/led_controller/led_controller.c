#include "led_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "LED_CTRL";

// Private state
static led_config_t led_cfg;
static bool initialized = false;
static bool current_state = false;
static led_pattern_t current_pattern = LED_PATTERN_OFF;
static TaskHandle_t led_task_handle = NULL;

// Private function to control LED hardware
static void led_set_state(bool on) {
    uint32_t level;
    if (led_cfg.active_high) {
        level = on ? 1 : 0;
    } else {
        level = on ? 0 : 1;
    }
    gpio_set_level(led_cfg.gpio_num, level);
    current_state = on;
}

// Pattern execution task
static void led_task(void *arg) {
    TickType_t dot_duration = pdMS_TO_TICKS(200);  // For SOS pattern
    TickType_t dash_duration = pdMS_TO_TICKS(600);

    while (1) {
        switch (current_pattern) {
            case LED_PATTERN_OFF:
                led_set_state(false);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_PATTERN_ON:
                led_set_state(true);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_PATTERN_BLINK_SLOW:
                led_set_state(true);
                vTaskDelay(pdMS_TO_TICKS(500));
                led_set_state(false);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_PATTERN_BLINK_FAST:
                led_set_state(true);
                vTaskDelay(pdMS_TO_TICKS(125));
                led_set_state(false);
                vTaskDelay(pdMS_TO_TICKS(125));
                break;

            case LED_PATTERN_PULSE:
                // Simple pulse: fade in and out
                for (int i = 0; i < 10; i++) {
                    led_set_state(true);
                    vTaskDelay(pdMS_TO_TICKS(20));
                    led_set_state(false);
                    vTaskDelay(pdMS_TO_TICKS(80));
                }
                for (int i = 0; i < 10; i++) {
                    led_set_state(true);
                    vTaskDelay(pdMS_TO_TICKS(80));
                    led_set_state(false);
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                break;

            case LED_PATTERN_SOS:
                // S (3 dots)
                for (int i = 0; i < 3; i++) {
                    led_set_state(true);
                    vTaskDelay(dot_duration);
                    led_set_state(false);
                    vTaskDelay(dot_duration);
                }
                vTaskDelay(dot_duration); // Space between letters

                // O (3 dashes)
                for (int i = 0; i < 3; i++) {
                    led_set_state(true);
                    vTaskDelay(dash_duration);
                    led_set_state(false);
                    vTaskDelay(dot_duration);
                }
                vTaskDelay(dot_duration); // Space between letters

                // S (3 dots)
                for (int i = 0; i < 3; i++) {
                    led_set_state(true);
                    vTaskDelay(dot_duration);
                    led_set_state(false);
                    vTaskDelay(dot_duration);
                }

                vTaskDelay(pdMS_TO_TICKS(2000)); // Long pause before repeating
                break;
        }
    }
}

// Public API implementation
esp_err_t led_init(const led_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Store configuration
    led_cfg = *config;

    // Configure GPIO
    gpio_reset_pin(led_cfg.gpio_num);
    gpio_set_direction(led_cfg.gpio_num, GPIO_MODE_OUTPUT);
    led_set_state(false);

    // Create background task
    BaseType_t result = xTaskCreate(
        led_task,
        "led_task",
        2048,           // Stack size
        NULL,
        5,              // Priority
        &led_task_handle
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return ESP_FAIL;
    }

    initialized = true;
    ESP_LOGI(TAG, "Initialized on GPIO %d", led_cfg.gpio_num);
    return ESP_OK;
}

esp_err_t led_set_pattern(led_pattern_t pattern) {
    if (!initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    current_pattern = pattern;
    ESP_LOGI(TAG, "Pattern changed to %d", pattern);
    return ESP_OK;
}

led_pattern_t led_get_pattern(void) {
    return current_pattern;
}

void led_deinit(void) {
    if (!initialized) {
        return;
    }

    // Delete task
    if (led_task_handle != NULL) {
        vTaskDelete(led_task_handle);
        led_task_handle = NULL;
    }

    // Turn off LED
    led_set_state(false);
    gpio_reset_pin(led_cfg.gpio_num);

    initialized = false;
    ESP_LOGI(TAG, "Deinitialized");
}
