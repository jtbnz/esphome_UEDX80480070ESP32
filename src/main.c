#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "viewe_lcd_driver.h"
#include "gt911_touch.h"
#include "flutter_bridge.h"

static const char* TAG = "main";

// Touch event callback
static void touch_event_callback(gt911_touch_data_t* data, void* user_data) {
    flutter_bridge_t* bridge = (flutter_bridge_t*)user_data;
    
    if (data->touched && data->touch_count > 0) {
        ESP_LOGI(TAG, "Touch detected: count=%d, x=%d, y=%d", 
                 data->touch_count, data->points[0].x, data->points[0].y);
        
        // Send touch event to Flutter
        flutter_bridge_send_touch_event(bridge, data);
    }
}

// Demo graphics task
static void demo_graphics_task(void* pvParameters) {
    viewe_lcd_driver_t* lcd = (viewe_lcd_driver_t*)pvParameters;
    
    ESP_LOGI(TAG, "Starting demo graphics");
    
    // Fill screen with blue
    viewe_lcd_fill(lcd, VIEWE_COLOR_BLUE);
    viewe_lcd_flush(lcd);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Draw some rectangles
    viewe_lcd_fill(lcd, VIEWE_COLOR_BLACK);
    viewe_lcd_fill_rect(lcd, 50, 50, 200, 100, VIEWE_COLOR_RED);
    viewe_lcd_fill_rect(lcd, 300, 50, 200, 100, VIEWE_COLOR_GREEN);
    viewe_lcd_fill_rect(lcd, 550, 50, 200, 100, VIEWE_COLOR_BLUE);
    viewe_lcd_flush(lcd);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Draw rectangle outlines
    viewe_lcd_fill(lcd, VIEWE_COLOR_WHITE);
    viewe_lcd_draw_rect(lcd, 100, 150, 600, 250, VIEWE_COLOR_RED);
    viewe_lcd_draw_rect(lcd, 110, 160, 580, 230, VIEWE_COLOR_GREEN);
    viewe_lcd_draw_rect(lcd, 120, 170, 560, 210, VIEWE_COLOR_BLUE);
    viewe_lcd_flush(lcd);
    
    ESP_LOGI(TAG, "Demo graphics complete - now waiting for Flutter commands");
    
    vTaskDelete(NULL);
}

// Touch polling task
static void touch_polling_task(void* pvParameters) {
    gt911_driver_t* touch = (gt911_driver_t*)pvParameters;
    gt911_touch_data_t touch_data;
    
    while (1) {
        esp_err_t ret = gt911_read_touch(touch, &touch_data);
        if (ret == ESP_OK && touch_data.touched) {
            // Touch event is handled by callback
        }
        
        vTaskDelay(pdMS_TO_TICKS(20));  // Poll every 20ms (50Hz)
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "VIEWE LCD Standalone Driver for Flutter");
    ESP_LOGI(TAG, "ESP32-S3 with 7-inch 800x480 Display");
    
    // Initialize NVS (required for WiFi if enabled later)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize LCD
    ESP_LOGI(TAG, "Initializing LCD driver...");
    viewe_lcd_config_t lcd_config = {
        .backlight_pin = VIEWE_LCD_BACKLIGHT,
        .initial_brightness = 100,
        .rotation = VIEWE_LCD_ROTATION_0,
    };
    
    viewe_lcd_driver_t* lcd = viewe_lcd_init(&lcd_config);
    if (lcd == NULL) {
        ESP_LOGE(TAG, "Failed to initialize LCD driver");
        return;
    }
    
    // Initialize touch controller
    ESP_LOGI(TAG, "Initializing touch controller...");
    gt911_config_t touch_config;
    gt911_get_default_config(&touch_config);
    
    gt911_driver_t* touch = gt911_init(&touch_config);
    if (touch == NULL) {
        ESP_LOGW(TAG, "Failed to initialize touch controller - continuing without touch");
    }
    
    // Initialize Flutter bridge
    ESP_LOGI(TAG, "Initializing Flutter bridge...");
    flutter_bridge_config_t bridge_config;
    flutter_bridge_get_default_config(&bridge_config);
    bridge_config.lcd_driver = lcd;
    bridge_config.touch_driver = touch;
    
    flutter_bridge_t* bridge = flutter_bridge_init(&bridge_config);
    if (bridge == NULL) {
        ESP_LOGE(TAG, "Failed to initialize Flutter bridge");
        return;
    }
    
    // Register touch callback
    if (touch != NULL) {
        gt911_register_callback(touch, touch_event_callback, bridge);
    }
    
    // Start demo graphics task
    xTaskCreate(demo_graphics_task, "demo_graphics", 4096, lcd, 5, NULL);
    
    // Start touch polling task if touch is available
    if (touch != NULL) {
        xTaskCreate(touch_polling_task, "touch_poll", 4096, touch, 5, NULL);
    }
    
    ESP_LOGI(TAG, "System initialized - ready for Flutter communication");
    ESP_LOGI(TAG, "Serial protocol: 115200 baud, 8N1");
    ESP_LOGI(TAG, "Use Flutter app to send commands via serial port");
    
    // Main loop - process Flutter commands
    while (1) {
        flutter_bridge_process(bridge);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
