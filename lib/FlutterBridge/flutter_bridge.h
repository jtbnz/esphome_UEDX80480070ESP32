#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "viewe_lcd_driver.h"
#include "gt911_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

// Command types for Flutter communication
typedef enum {
    CMD_PING = 0x01,
    CMD_GET_DISPLAY_INFO = 0x02,
    CMD_SET_PIXEL = 0x03,
    CMD_FILL = 0x04,
    CMD_FILL_RECT = 0x05,
    CMD_DRAW_RECT = 0x06,
    CMD_FLUSH = 0x07,
    CMD_SET_BRIGHTNESS = 0x08,
    CMD_SET_ROTATION = 0x09,
    CMD_GET_TOUCH = 0x0A,
    CMD_DRAW_BITMAP = 0x0B,
} flutter_cmd_t;

// Response types
typedef enum {
    RESP_OK = 0x00,
    RESP_ERROR = 0xFF,
    RESP_DISPLAY_INFO = 0x02,
    RESP_TOUCH_DATA = 0x0A,
} flutter_resp_t;

// Bridge handle
typedef struct flutter_bridge_t flutter_bridge_t;

// Configuration
typedef struct {
    viewe_lcd_driver_t* lcd_driver;
    gt911_driver_t* touch_driver;
    bool enable_serial;
    bool enable_wifi;
    const char* wifi_ssid;
    const char* wifi_password;
    uint16_t tcp_port;
} flutter_bridge_config_t;

/**
 * @brief Initialize Flutter bridge
 * 
 * @param config Configuration structure
 * @return flutter_bridge_t* Bridge handle or NULL on failure
 */
flutter_bridge_t* flutter_bridge_init(const flutter_bridge_config_t* config);

/**
 * @brief Deinitialize Flutter bridge
 * 
 * @param bridge Bridge handle
 */
void flutter_bridge_deinit(flutter_bridge_t* bridge);

/**
 * @brief Process incoming commands (call regularly in main loop)
 * 
 * @param bridge Bridge handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t flutter_bridge_process(flutter_bridge_t* bridge);

/**
 * @brief Send touch event to Flutter
 * 
 * @param bridge Bridge handle
 * @param touch_data Touch data to send
 * @return esp_err_t ESP_OK on success
 */
esp_err_t flutter_bridge_send_touch_event(flutter_bridge_t* bridge, gt911_touch_data_t* touch_data);

/**
 * @brief Get default bridge configuration
 * 
 * @param config Pointer to configuration structure to fill
 */
void flutter_bridge_get_default_config(flutter_bridge_config_t* config);

#ifdef __cplusplus
}
#endif
