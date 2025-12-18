#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display dimensions
#define VIEWE_LCD_WIDTH  800
#define VIEWE_LCD_HEIGHT 480
#define VIEWE_LCD_BPP    16  // Bits per pixel (RGB565)

// Pin definitions for VIEWE hardware
#define VIEWE_LCD_VSYNC  GPIO_NUM_41
#define VIEWE_LCD_HSYNC  GPIO_NUM_39
#define VIEWE_LCD_DE     GPIO_NUM_40
#define VIEWE_LCD_PCLK   GPIO_NUM_42

// RGB Data pins (16-bit interface)
#define VIEWE_LCD_DATA0  GPIO_NUM_8   // R0
#define VIEWE_LCD_DATA1  GPIO_NUM_3   // R1
#define VIEWE_LCD_DATA2  GPIO_NUM_46  // R2
#define VIEWE_LCD_DATA3  GPIO_NUM_9   // R3
#define VIEWE_LCD_DATA4  GPIO_NUM_1   // R4
#define VIEWE_LCD_DATA5  GPIO_NUM_5   // G0
#define VIEWE_LCD_DATA6  GPIO_NUM_6   // G1
#define VIEWE_LCD_DATA7  GPIO_NUM_7   // G2
#define VIEWE_LCD_DATA8  GPIO_NUM_15  // G3
#define VIEWE_LCD_DATA9  GPIO_NUM_16  // G4
#define VIEWE_LCD_DATA10 GPIO_NUM_4   // G5
#define VIEWE_LCD_DATA11 GPIO_NUM_45  // B0
#define VIEWE_LCD_DATA12 GPIO_NUM_48  // B1
#define VIEWE_LCD_DATA13 GPIO_NUM_47  // B2
#define VIEWE_LCD_DATA14 GPIO_NUM_21  // B3
#define VIEWE_LCD_DATA15 GPIO_NUM_14  // B4

// Default backlight pin
#define VIEWE_LCD_BACKLIGHT GPIO_NUM_2

// Rotation modes
typedef enum {
    VIEWE_LCD_ROTATION_0   = 0,  // 0 degrees
    VIEWE_LCD_ROTATION_90  = 1,  // 90 degrees clockwise
    VIEWE_LCD_ROTATION_180 = 2,  // 180 degrees
    VIEWE_LCD_ROTATION_270 = 3   // 270 degrees clockwise
} viewe_lcd_rotation_t;

// Color structure (RGB888)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} viewe_color_t;

// LCD Driver handle
typedef struct viewe_lcd_driver_t viewe_lcd_driver_t;

// Configuration structure
typedef struct {
    gpio_num_t backlight_pin;
    uint8_t initial_brightness;  // 0-100
    viewe_lcd_rotation_t rotation;
} viewe_lcd_config_t;

/**
 * @brief Initialize the VIEWE LCD driver
 * 
 * @param config Configuration structure (NULL for defaults)
 * @return viewe_lcd_driver_t* Driver handle or NULL on failure
 */
viewe_lcd_driver_t* viewe_lcd_init(const viewe_lcd_config_t* config);

/**
 * @brief Deinitialize and free the LCD driver
 * 
 * @param driver Driver handle
 */
void viewe_lcd_deinit(viewe_lcd_driver_t* driver);

/**
 * @brief Get the framebuffer pointer for direct manipulation
 * 
 * @param driver Driver handle
 * @return uint8_t* Pointer to framebuffer (RGB565 format)
 */
uint8_t* viewe_lcd_get_framebuffer(viewe_lcd_driver_t* driver);

/**
 * @brief Get framebuffer size in bytes
 * 
 * @param driver Driver handle
 * @return size_t Size in bytes
 */
size_t viewe_lcd_get_buffer_size(viewe_lcd_driver_t* driver);

/**
 * @brief Update the display with current framebuffer contents
 * 
 * @param driver Driver handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t viewe_lcd_flush(viewe_lcd_driver_t* driver);

/**
 * @brief Set a single pixel
 * 
 * @param driver Driver handle
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color in RGB888 format
 * @return esp_err_t ESP_OK on success
 */
esp_err_t viewe_lcd_set_pixel(viewe_lcd_driver_t* driver, int x, int y, viewe_color_t color);

/**
 * @brief Fill entire screen with a color
 * 
 * @param driver Driver handle
 * @param color Color in RGB888 format
 * @return esp_err_t ESP_OK on success
 */
esp_err_t viewe_lcd_fill(viewe_lcd_driver_t* driver, viewe_color_t color);

/**
 * @brief Draw a filled rectangle
 * 
 * @param driver Driver handle
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Width in pixels
 * @param height Height in pixels
 * @param color Color in RGB888 format
 * @return esp_err_t ESP_OK on success
 */
esp_err_t viewe_lcd_fill_rect(viewe_lcd_driver_t* driver, int x, int y, int width, int height, viewe_color_t color);

/**
 * @brief Draw a rectangle outline
 * 
 * @param driver Driver handle
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Width in pixels
 * @param height Height in pixels
 * @param color Color in RGB888 format
 * @return esp_err_t ESP_OK on success
 */
esp_err_t viewe_lcd_draw_rect(viewe_lcd_driver_t* driver, int x, int y, int width, int height, viewe_color_t color);

/**
 * @brief Set backlight brightness
 * 
 * @param driver Driver handle
 * @param brightness Brightness level 0-100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t viewe_lcd_set_brightness(viewe_lcd_driver_t* driver, uint8_t brightness);

/**
 * @brief Set display rotation
 * 
 * @param driver Driver handle
 * @param rotation Rotation mode
 * @return esp_err_t ESP_OK on success
 */
esp_err_t viewe_lcd_set_rotation(viewe_lcd_driver_t* driver, viewe_lcd_rotation_t rotation);

/**
 * @brief Get current display width (considering rotation)
 * 
 * @param driver Driver handle
 * @return int Width in pixels
 */
int viewe_lcd_get_width(viewe_lcd_driver_t* driver);

/**
 * @brief Get current display height (considering rotation)
 * 
 * @param driver Driver handle
 * @return int Height in pixels
 */
int viewe_lcd_get_height(viewe_lcd_driver_t* driver);

/**
 * @brief Convert RGB888 to RGB565
 * 
 * @param color Color in RGB888 format
 * @return uint16_t Color in RGB565 format
 */
static inline uint16_t viewe_color_to_rgb565(viewe_color_t color) {
    return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

/**
 * @brief Helper to create a color
 */
static inline viewe_color_t viewe_color(uint8_t r, uint8_t g, uint8_t b) {
    viewe_color_t c = {r, g, b};
    return c;
}

// Predefined colors
#define VIEWE_COLOR_BLACK   viewe_color(0, 0, 0)
#define VIEWE_COLOR_WHITE   viewe_color(255, 255, 255)
#define VIEWE_COLOR_RED     viewe_color(255, 0, 0)
#define VIEWE_COLOR_GREEN   viewe_color(0, 255, 0)
#define VIEWE_COLOR_BLUE    viewe_color(0, 0, 255)
#define VIEWE_COLOR_YELLOW  viewe_color(255, 255, 0)
#define VIEWE_COLOR_CYAN    viewe_color(0, 255, 255)
#define VIEWE_COLOR_MAGENTA viewe_color(255, 0, 255)

#ifdef __cplusplus
}
#endif
