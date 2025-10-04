#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "led_controller.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "MAIN";

// Queue handles for inter-task communication
QueueHandle_t sensorQueue;      // Sensor → Processor
QueueHandle_t displayQueue;     // Processor → Display

// Data structures for queue messages
typedef struct {
    float temperature;
    uint32_t timestamp;
} sensor_data_t;

typedef struct {
    float average_temp;
    uint32_t sample_count;
} processed_data_t;

// Task 1: Sensor Task (High Priority)
// Simulates reading a temperature sensor
void sensor_task(void *parameter) {
    sensor_data_t data;
    uint32_t reading_count = 0;

    ESP_LOGI(TAG, "Sensor task started");

    while(1) {
        // Simulate sensor reading (random temperature 15-35°C)
        data.temperature = 15.0 + (esp_random() % 2000) / 100.0;
        data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        reading_count++;

        // Send to processor queue
        if(xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "Sensor [%lu]: %.2f°C", reading_count, data.temperature);
        } else {
            ESP_LOGW(TAG, "Sensor queue full, data lost!");
        }

        // Read sensor every 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task 2: Processor Task (Medium Priority)
// Calculates running average of last 5 readings
void processor_task(void *parameter) {
    sensor_data_t received_data;
    processed_data_t processed;

    float temp_buffer[5] = {0};
    uint8_t buffer_index = 0;
    uint32_t total_samples = 0;

    ESP_LOGI(TAG, "Processor task started");

    while(1) {
        // Wait for data from sensor
        if(xQueueReceive(sensorQueue, &received_data, portMAX_DELAY) == pdPASS) {
            // Store in circular buffer
            temp_buffer[buffer_index] = received_data.temperature;
            buffer_index = (buffer_index + 1) % 5;
            total_samples++;

            // Calculate average
            float sum = 0;
            uint8_t count = (total_samples < 5) ? total_samples : 5;
            for(int i = 0; i < count; i++) {
                sum += temp_buffer[i];
            }

            processed.average_temp = sum / count;
            processed.sample_count = total_samples;

            ESP_LOGI(TAG, "Processor: Avg=%.2f°C (samples: %lu)",
                     processed.average_temp, processed.sample_count);

            // Send to display queue
            xQueueSend(displayQueue, &processed, pdMS_TO_TICKS(100));
        }
    }
}

// Task 3: Display Task (Low Priority)
// Changes LED pattern based on temperature
void display_task(void *parameter) {
    processed_data_t data;
    led_pattern_t current_pattern = LED_PATTERN_OFF;

    ESP_LOGI(TAG, "Display task started");

    while(1) {
        // Wait for processed data
        if(xQueueReceive(displayQueue, &data, portMAX_DELAY) == pdPASS) {
            led_pattern_t new_pattern;

            // Decide pattern based on temperature
            if(data.average_temp < 20.0) {
                new_pattern = LED_PATTERN_BLINK_SLOW;  // Cold
            } else if(data.average_temp < 25.0) {
                new_pattern = LED_PATTERN_ON;          // Comfortable
            } else if(data.average_temp < 30.0) {
                new_pattern = LED_PATTERN_BLINK_FAST;  // Warm
            } else {
                new_pattern = LED_PATTERN_SOS;         // Hot!
            }

            // Only change pattern if different
            if(new_pattern != current_pattern) {
                led_set_pattern(new_pattern);
                current_pattern = new_pattern;
                ESP_LOGI(TAG, "Display: Pattern changed (%.2f°C)", data.average_temp);
            }
        }
    }
}

// Task 4: Stats Task (Lowest Priority)
// Monitors system health
void stats_task(void *parameter) {
    ESP_LOGI(TAG, "Stats task started");

    while(1) {
        // Print system statistics
        ESP_LOGI(TAG, "=== System Stats ===");
        ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "Sensor queue waiting: %d", uxQueueMessagesWaiting(sensorQueue));
        ESP_LOGI(TAG, "Display queue waiting: %d", uxQueueMessagesWaiting(displayQueue));

        // Check stack usage for each task
        ESP_LOGI(TAG, "Task stack high water marks:");
        // Note: You'd need task handles to check individual tasks

        vTaskDelay(pdMS_TO_TICKS(10000));  // Every 10 seconds
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== FreeRTOS Multi-Task Demo ===");

    // Initialize LED controller
    led_config_t led_config = {
        .gpio_num = GPIO_NUM_2,
        .active_high = true
    };

    esp_err_t ret = led_init(&led_config);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LED");
        return;
    }

    // Create queues
    sensorQueue = xQueueCreate(10, sizeof(sensor_data_t));
    displayQueue = xQueueCreate(5, sizeof(processed_data_t));

    if(sensorQueue == NULL || displayQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }

    ESP_LOGI(TAG, "Queues created successfully");

    // Create tasks with different priorities
    xTaskCreate(
        sensor_task,
        "Sensor",
        3072,           // Stack size
        NULL,
        5,              // Priority (highest)
        NULL
    );

    xTaskCreate(
        processor_task,
        "Processor",
        3072,
        NULL,
        4,              // Priority (medium-high)
        NULL
    );

    xTaskCreate(
        display_task,
        "Display",
        3072,
        NULL,
        3,              // Priority (medium)
        NULL
    );

    xTaskCreate(
        stats_task,
        "Stats",
        2048,
        NULL,
        1,              // Priority (lowest)
        NULL
    );

    ESP_LOGI(TAG, "All tasks created. System running...");

    // Main task does nothing - all work in other tasks
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(60000));  // Sleep forever
    }
}
