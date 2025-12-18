#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// GT911 I2C addresses (can be 0x5D or 0x14 depending on configuration)
#define GT911_I2C_ADDR_1 0x5D
#define GT911_I2C_ADDR_2 0x14

// Default pins for VIEWE hardware
#define GT911_DEFAULT_SDA  GPIO_NUM_19
#define GT911_DEFAULT_SCL  GPIO_NUM_20
#define GT911_DEFAULT_INT  GPIO_NUM_18
#define GT911_DEFAULT_RST  GPIO_NUM_38

// Maximum touch points
#define GT911_MAX_TOUCHES 5

// Touch point structure
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t size;
    uint8_t track_id;
} gt911_touch_point_t;

// Touch data structure
typedef struct {
    uint8_t touch_count;
    gt911_touch_point_t points[GT911_MAX_TOUCHES];
    bool touched;
} gt911_touch_data_t;

// Touch driver handle
typedef struct gt911_driver_t gt911_driver_t;

// Configuration structure
typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    gpio_num_t int_pin;
    gpio_num_t rst_pin;
    uint8_t i2c_addr;
    uint32_t i2c_freq;
    uint16_t width;   // Display width for coordinate mapping
    uint16_t height;  // Display height for coordinate mapping
} gt911_config_t;

// Touch event callback type
typedef void (*gt911_callback_t)(gt911_touch_data_t* data, void* user_data);

/**
 * @brief Initialize GT911 touch controller
 * 
 * @param config Configuration structure (NULL for defaults)
 * @return gt911_driver_t* Driver handle or NULL on failure
 */
gt911_driver_t* gt911_init(const gt911_config_t* config);

/**
 * @brief Deinitialize GT911 touch controller
 * 
 * @param driver Driver handle
 */
void gt911_deinit(gt911_driver_t* driver);

/**
 * @brief Read current touch data
 * 
 * @param driver Driver handle
 * @param data Pointer to touch data structure to fill
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gt911_read_touch(gt911_driver_t* driver, gt911_touch_data_t* data);

/**
 * @brief Register a callback for touch events
 * 
 * @param driver Driver handle
 * @param callback Callback function
 * @param user_data User data to pass to callback
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gt911_register_callback(gt911_driver_t* driver, gt911_callback_t callback, void* user_data);

/**
 * @brief Get default configuration
 * 
 * @param config Pointer to configuration structure to fill
 */
void gt911_get_default_config(gt911_config_t* config);

#ifdef __cplusplus
}
#endif
