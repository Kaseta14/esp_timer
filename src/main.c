#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

static const char *TAG = "MOTOR_CONTROL";

#define MOTOR_GPIO_PIN GPIO_NUM_9
#define LED_GPIO_PIN GPIO_NUM_10

#define TIME_ON (5ULL * 1000000ULL) // 15 microseconds 
#define TIME_OFF (45ULL * 1000000ULL) // 45 microseconds

typedef enum {
    STATE_ON,
    STATE_OFF
} motor_state_t;

static motor_state_t current_state = STATE_OFF;
static esp_timer_handle_t motor_timer = NULL;

void init_hardware(void) {

        gpio_config_t io_conf = {
            .pin_bit_mask = ((1ULL << MOTOR_GPIO_PIN) | (1ULL << LED_GPIO_PIN)),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        gpio_set_level(MOTOR_GPIO_PIN, 0);
        gpio_set_level(LED_GPIO_PIN, 0);
}
void pause_play_callback(void* arg) {
    if (current_state == STATE_OFF) {
        current_state = STATE_ON;
        gpio_set_level(MOTOR_GPIO_PIN, 1);
        gpio_set_level(LED_GPIO_PIN, 1);
        ESP_LOGI(TAG, "Motor is turned on for 15 seconds");
        esp_timer_start_once(motor_timer, TIME_ON);
    }
    else {
        current_state = STATE_OFF;
        gpio_set_level(MOTOR_GPIO_PIN, 0);
        gpio_set_level(LED_GPIO_PIN, 0);
        ESP_LOGI(TAG,"Motor is turned off for 45 seconds");
        esp_timer_start_once(motor_timer, TIME_OFF);
    }
}

void app_main() {
    // --- НАЛАШТУВАННЯ WATCHDOG ---
    // Створюємо конфігурацію: якщо собаку не погодувати 5 секунд, плата перезавантажиться
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 5000,
        .idle_core_mask = (1 << 0), // Моніторимо нульове ядро
        .trigger_panic = true       // Викликати паніку (перезавантаження) при зависанні
    };
    esp_task_wdt_init(&wdt_config);
    
    // Підписуємо нашу поточну таску (app_main) на моніторинг
    esp_task_wdt_add(NULL);

    ESP_LOGI(TAG, "Launch control motor system");

    init_hardware();

    const esp_timer_create_args_t timer_args = {
        .callback = &pause_play_callback,
        .name = "motor_hardware_timer"
    };
    esp_timer_create(&timer_args, &motor_timer);
    esp_timer_start_once(motor_timer, 0);
    ESP_LOGI(TAG, "The timer was started independently, leaving the main thread free");

    while(1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}