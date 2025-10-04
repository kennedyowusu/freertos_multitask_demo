# FreeRTOS Multi-Task Demo

A practical demonstration of FreeRTOS concepts: concurrent tasks, queue-based communication, and priority scheduling on ESP32.

## Overview

This project simulates an environmental monitoring system with four independent tasks communicating via queues. It demonstrates professional embedded architecture patterns used in production IoT systems.

## System Architecture

┌─────────────┐
│ Sensor Task │ (Priority 5 - Highest)
│  Reads temp │
└──────┬──────┘
│ Queue: sensor_data_t
↓
┌──────────────┐
│Processor Task│ (Priority 4)
│Calculates avg│
└──────┬───────┘
│ Queue: processed_data_t
↓
┌──────────────┐
│ Display Task │ (Priority 3)
│Controls LED  │
└──────────────┘
┌──────────────┐
│  Stats Task  │ (Priority 1 - Lowest)
│System monitor│
└──────────────┘

## Features

**Four Concurrent Tasks:**

1. **Sensor Task**: Generates random temperature readings (15-35°C) every second
2. **Processor Task**: Calculates running average of last 5 readings
3. **Display Task**: Changes LED pattern based on temperature thresholds
4. **Stats Task**: Monitors system health (heap, queue depths) every 10 seconds

**Queue-Based Communication:**

- No shared global variables
- Thread-safe data passing
- Tasks block when waiting for data (zero CPU usage while idle)

**Temperature-Based LED Patterns:**

- < 20°C: Slow blink (cold)
- 20-25°C: Solid on (comfortable)
- 25-30°C: Fast blink (warm)
- > 30°C: SOS pattern (hot)

## Key Concepts Demonstrated

### 1. Task Creation

```c
xTaskCreate(
    sensor_task,    // Function
    "Sensor",       // Name
    3072,           // Stack size (bytes)
    NULL,           // Parameters
    5,              // Priority
    NULL            // Handle
);
2. Queue Communication
c// Sender
sensor_data_t data = {.temperature = 25.5, .timestamp = now};
xQueueSend(sensorQueue, &data, timeout);

// Receiver
sensor_data_t received;
xQueueReceive(sensorQueue, &received, portMAX_DELAY);
3. Task Priorities

5: Sensor (highest - real-time data collection)
4: Processor (high - immediate data processing)
3: Display (medium - user feedback)
1: Stats (lowest - background monitoring)

4. Blocking Behavior
Tasks use xQueueReceive() with portMAX_DELAY:

Task enters BLOCKED state when queue empty
Zero CPU usage while blocked
Scheduler runs other tasks
Task becomes READY when data arrives

Building and Running
Prerequisites

ESP-IDF v5.5.1
ESP32 development board
Built-in LED on GPIO 2

Build Commands
bash# Set up environment
. ~/.espressif/esp-idf/export.sh

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/tty.SLAB_USBtoUART flash monitor
Expected Output
I (292) MAIN: === FreeRTOS Multi-Task Demo ===
I (298) MAIN: Queues created successfully
I (304) MAIN: All tasks created. System running...
I (310) MAIN: Sensor task started
I (316) MAIN: Processor task started
I (322) MAIN: Display task started
I (328) MAIN: Stats task started
I (1334) MAIN: Sensor [1]: 22.45°C
I (1340) MAIN: Processor: Avg=22.45°C (samples: 1)
I (1346) MAIN: Display: Pattern changed (22.45°C)
I (2344) MAIN: Sensor [2]: 28.12°C
I (2350) MAIN: Processor: Avg=25.29°C (samples: 2)
I (2356) MAIN: Display: Pattern changed (25.29°C)
...
I (10328) MAIN: === System Stats ===
I (10334) MAIN: Free heap: 245632 bytes
I (10340) MAIN: Sensor queue waiting: 0
I (10346) MAIN: Display queue waiting: 0
Project Structure
freertos_multitask_demo/
├── components/
│   └── led_controller/        # Reusable LED component
│       ├── include/
│       │   └── led_controller.h
│       ├── led_controller.c
│       └── CMakeLists.txt
├── main/
│   ├── main.c                 # Application with 4 tasks
│   └── CMakeLists.txt
├── CMakeLists.txt
└── README.md
Code Walkthrough
Data Structures
c// Raw sensor reading
typedef struct {
    float temperature;
    uint32_t timestamp;
} sensor_data_t;

// Processed data
typedef struct {
    float average_temp;
    uint32_t sample_count;
} processed_data_t;
Queue Creation
cQueueHandle_t sensorQueue = xQueueCreate(10, sizeof(sensor_data_t));
QueueHandle_t displayQueue = xQueueCreate(5, sizeof(processed_data_t));
Producer-Consumer Pattern
Producer (Sensor task):
csensor_data_t data;
data.temperature = read_sensor();
xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(100));
Consumer (Processor task):
csensor_data_t received;
xQueueReceive(sensorQueue, &received, portMAX_DELAY);
process(received.temperature);
Experiments to Try
1. Change Task Priorities
Swap sensor and stats priorities - system still works because of blocking behavior.
2. Increase Queue Size
Change xQueueCreate(10, ...) to xQueueCreate(2, ...) - observe queue full warnings.
3. Add Delay to Processor
Add vTaskDelay(pdMS_TO_TICKS(5000)) in processor - sensor queue fills up.
4. Monitor Stack Usage
Add to each task:
cUBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
ESP_LOGI(TAG, "Stack remaining: %d bytes", stackLeft);
5. Simulate Sensor Failure
Comment out sensor task creation - processor and display tasks wait forever (blocked).
Learning Objectives
After studying this project, you should understand:
FreeRTOS Fundamentals:

Task creation and lifecycle
Priority-based preemptive scheduling
Task states (Running, Ready, Blocked)
Stack allocation per task

Inter-Task Communication:

Queue creation and management
Thread-safe data passing
Blocking vs non-blocking operations
Queue full/empty handling

System Design:

Producer-consumer pattern
Event-driven architecture
Priority assignment rationale
Resource monitoring

Debugging:

Reading task logs
Monitoring queue depths
Tracking heap usage
Stack overflow detection

Common Issues
Queue Overflow
Symptom: "Sensor queue full, data lost!" messages
Cause: Producer faster than consumer
Solution: Increase queue size or speed up consumer
Stack Overflow
Symptom: Crashes, ESP32 resets
Cause: Task stack too small
Solution: Increase stack size in xTaskCreate()
Starvation
Symptom: Low priority task never runs
Cause: Higher priority tasks never block
Solution: Ensure all tasks yield/delay periodically
Heap Exhaustion
Symptom: Task/queue creation fails
Cause: Too many tasks/large queues
Solution: Monitor with esp_get_free_heap_size()
Real-World Applications
This pattern scales to production systems:
IoT Sensor Node:

Sensor tasks read multiple sensors
Processing task aggregates data
WiFi task sends to cloud
Display task shows status

Home Automation:

Button task handles user input
Logic task processes commands
Relay task controls devices
Status task updates display

Industrial Controller:

Safety task monitors critical parameters (highest priority)
Control task manages actuators
Logging task records events
UI task handles operator interface

Next Steps
After understanding this project:

Add a fifth task that reads real sensor (DHT22, BME280)
Replace simulated data with actual I2C/SPI communication
Add semaphores to protect shared resources
Implement task notifications for faster signaling
Add WiFi task to send data to cloud

Resources

FreeRTOS Documentation
ESP-IDF FreeRTOS Guide
ESP32 Technical Reference

License
Public Domain / CC0
Author
Part of ESP32 IoT learning roadmap - Week 5-7 material
