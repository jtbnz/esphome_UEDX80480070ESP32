#include "../../include/viewe_lcd_driver.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char* TAG = "viewe_lcd";

// Internal driver structure
struct viewe_lcd_driver_t {
    esp_lcd_panel_handle_t lcd_panel;
    uint8_t* framebuffer;
    gpio_num_t backlight_pin;
    uint8_t brightness;
    viewe_lcd_rotation_t rotation;
    size_t buffer_size;
};

// Helper function to rotate coordinates
static void rotate_coordinates(viewe_lcd_driver_t* driver, int* x, int* y) {
    int original_x = *x;
    int original_y = *y;
    
    switch (driver->rotation) {
        case VIEWE_LCD_ROTATION_0:
            // No rotation needed
            break;
            
        case VIEWE_LCD_ROTATION_90:
            *x = original_y;
            *y = VIEWE_LCD_WIDTH - 1 - original_x;
            break;
            
        case VIEWE_LCD_ROTATION_180:
            *x = VIEWE_LCD_WIDTH - 1 - original_x;
            *y = VIEWE_LCD_HEIGHT - 1 - original_y;
            break;
            
        case VIEWE_LCD_ROTATION_270:
            *x = VIEWE_LCD_HEIGHT - 1 - original_y;
            *y = original_x;
            break;
    }
}

// Initialize LCD hardware
static esp_err_t init_lcd_panel(viewe_lcd_driver_t* driver) {
    ESP_LOGI(TAG, "Initializing RGB LCD panel...");
    
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .psram_trans_align = 64,
        .data_width = 16,
        .bits_per_pixel = 16,
        .de_gpio_num = VIEWE_LCD_DE,
        .pclk_gpio_num = VIEWE_LCD_PCLK,
        .vsync_gpio_num = VIEWE_LCD_VSYNC,
        .hsync_gpio_num = VIEWE_LCD_HSYNC,
        .disp_gpio_num = GPIO_NUM_NC,
        .data_gpio_nums = {
            VIEWE_LCD_DATA0, VIEWE_LCD_DATA1, VIEWE_LCD_DATA2, VIEWE_LCD_DATA3,
            VIEWE_LCD_DATA4, VIEWE_LCD_DATA5, VIEWE_LCD_DATA6, VIEWE_LCD_DATA7,
            VIEWE_LCD_DATA8, VIEWE_LCD_DATA9, VIEWE_LCD_DATA10, VIEWE_LCD_DATA11,
            VIEWE_LCD_DATA12, VIEWE_LCD_DATA13, VIEWE_LCD_DATA14, VIEWE_LCD_DATA15,
        },
        .timings = {
            .pclk_hz = 15 * 1000 * 1000,  // 15MHz
            .h_res = VIEWE_LCD_WIDTH,
            .v_res = VIEWE_LCD_HEIGHT,
            .hsync_pulse_width = 48,
            .hsync_back_porch = 40,
            .hsync_front_porch = 88,
            .vsync_pulse_width = 6,
            .vsync_back_porch = 26,
            .vsync_front_porch = 30,
            .flags = {
                .pclk_active_neg = true,
                .de_idle_high = false,
                .pclk_idle_high = false,
            }
        },
        .flags = {
            .fb_in_psram = true,
            .double_fb = false,
            .no_fb = false,
            .bb_invalidate_cache = false,
        },
        .bounce_buffer_size_px = VIEWE_LCD_WIDTH * 10,  // 10 lines
    };
    
    esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &driver->lcd_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RGB panel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_lcd_panel_init(driver->lcd_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RGB panel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "RGB LCD panel initialized successfully");
    return ESP_OK;
}

viewe_lcd_driver_t* viewe_lcd_init(const viewe_lcd_config_t* config) {
    viewe_lcd_driver_t* driver = (viewe_lcd_driver_t*)malloc(sizeof(viewe_lcd_driver_t));
    if (driver == NULL) {
        ESP_LOGE(TAG, "Failed to allocate driver structure");
        return NULL;
    }
    
    memset(driver, 0, sizeof(viewe_lcd_driver_t));
    
    // Apply configuration or use defaults
    if (config != NULL) {
        driver->backlight_pin = config->backlight_pin;
        driver->brightness = config->initial_brightness;
        driver->rotation = config->rotation;
    } else {
        driver->backlight_pin = VIEWE_LCD_BACKLIGHT;
        driver->brightness = 100;
        driver->rotation = VIEWE_LCD_ROTATION_0;
    }
    
    // Initialize backlight GPIO
    if (driver->backlight_pin != GPIO_NUM_NC) {
        gpio_config_t bk_gpio_config = {
            .pin_bit_mask = 1ULL << driver->backlight_pin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&bk_gpio_config);
        gpio_set_level(driver->backlight_pin, driver->brightness > 0 ? 1 : 0);
    }
    
    // Initialize LCD panel
    esp_err_t ret = init_lcd_panel(driver);
    if (ret != ESP_OK) {
        free(driver);
        return NULL;
    }
    
    // Allocate framebuffer in PSRAM
    driver->buffer_size = VIEWE_LCD_WIDTH * VIEWE_LCD_HEIGHT * (VIEWE_LCD_BPP / 8);
    driver->framebuffer = (uint8_t*)heap_caps_malloc(driver->buffer_size, MALLOC_CAP_SPIRAM);
    if (driver->framebuffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer in PSRAM");
        esp_lcd_panel_del(driver->lcd_panel);
        free(driver);
        return NULL;
    }
    
    // Clear framebuffer
    memset(driver->framebuffer, 0, driver->buffer_size);
    
    ESP_LOGI(TAG, "VIEWE LCD driver initialized successfully");
    ESP_LOGI(TAG, "  Resolution: %dx%d", VIEWE_LCD_WIDTH, VIEWE_LCD_HEIGHT);
    ESP_LOGI(TAG, "  Buffer size: %zu bytes", driver->buffer_size);
    ESP_LOGI(TAG, "  Rotation: %d degrees", driver->rotation * 90);
    
    return driver;
}

void viewe_lcd_deinit(viewe_lcd_driver_t* driver) {
    if (driver == NULL) return;
    
    if (driver->lcd_panel != NULL) {
        esp_lcd_panel_del(driver->lcd_panel);
    }
    
    if (driver->framebuffer != NULL) {
        heap_caps_free(driver->framebuffer);
    }
    
    free(driver);
    ESP_LOGI(TAG, "VIEWE LCD driver deinitialized");
}

uint8_t* viewe_lcd_get_framebuffer(viewe_lcd_driver_t* driver) {
    return driver ? driver->framebuffer : NULL;
}

size_t viewe_lcd_get_buffer_size(viewe_lcd_driver_t* driver) {
    return driver ? driver->buffer_size : 0;
}

esp_err_t viewe_lcd_flush(viewe_lcd_driver_t* driver) {
    if (driver == NULL || driver->lcd_panel == NULL || driver->framebuffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return esp_lcd_panel_draw_bitmap(
        driver->lcd_panel,
        0, 0,
        VIEWE_LCD_WIDTH, VIEWE_LCD_HEIGHT,
        driver->framebuffer
    );
}

esp_err_t viewe_lcd_set_pixel(viewe_lcd_driver_t* driver, int x, int y, viewe_color_t color) {
    if (driver == NULL || driver->framebuffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check bounds before rotation
    int width = viewe_lcd_get_width(driver);
    int height = viewe_lcd_get_height(driver);
    
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Apply rotation
    rotate_coordinates(driver, &x, &y);
    
    // Check physical bounds after rotation
    if (x < 0 || x >= VIEWE_LCD_WIDTH || y < 0 || y >= VIEWE_LCD_HEIGHT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Convert to RGB565
    uint16_t rgb565 = viewe_color_to_rgb565(color);
    
    // Calculate position in framebuffer
    size_t pos = (y * VIEWE_LCD_WIDTH + x) * 2;
    if (pos >= driver->buffer_size) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Write pixel (little endian)
    driver->framebuffer[pos] = rgb565 & 0xFF;
    driver->framebuffer[pos + 1] = (rgb565 >> 8) & 0xFF;
    
    return ESP_OK;
}

esp_err_t viewe_lcd_fill(viewe_lcd_driver_t* driver, viewe_color_t color) {
    if (driver == NULL || driver->framebuffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint16_t rgb565 = viewe_color_to_rgb565(color);
    uint16_t* buffer = (uint16_t*)driver->framebuffer;
    size_t pixel_count = VIEWE_LCD_WIDTH * VIEWE_LCD_HEIGHT;
    
    for (size_t i = 0; i < pixel_count; i++) {
        buffer[i] = rgb565;
    }
    
    return ESP_OK;
}

esp_err_t viewe_lcd_fill_rect(viewe_lcd_driver_t* driver, int x, int y, int width, int height, viewe_color_t color) {
    if (driver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            viewe_lcd_set_pixel(driver, x + dx, y + dy, color);
        }
    }
    
    return ESP_OK;
}

esp_err_t viewe_lcd_draw_rect(viewe_lcd_driver_t* driver, int x, int y, int width, int height, viewe_color_t color) {
    if (driver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Top and bottom edges
    for (int dx = 0; dx < width; dx++) {
        viewe_lcd_set_pixel(driver, x + dx, y, color);
        viewe_lcd_set_pixel(driver, x + dx, y + height - 1, color);
    }
    
    // Left and right edges
    for (int dy = 0; dy < height; dy++) {
        viewe_lcd_set_pixel(driver, x, y + dy, color);
        viewe_lcd_set_pixel(driver, x + width - 1, y + dy, color);
    }
    
    return ESP_OK;
}

esp_err_t viewe_lcd_set_brightness(viewe_lcd_driver_t* driver, uint8_t brightness) {
    if (driver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    driver->brightness = brightness > 100 ? 100 : brightness;
    
    if (driver->backlight_pin != GPIO_NUM_NC) {
        // Simple on/off control (PWM can be added later)
        gpio_set_level(driver->backlight_pin, driver->brightness > 0 ? 1 : 0);
    }
    
    return ESP_OK;
}

esp_err_t viewe_lcd_set_rotation(viewe_lcd_driver_t* driver, viewe_lcd_rotation_t rotation) {
    if (driver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    driver->rotation = rotation % 4;
    ESP_LOGI(TAG, "Display rotation set to %d degrees", driver->rotation * 90);
    
    return ESP_OK;
}

int viewe_lcd_get_width(viewe_lcd_driver_t* driver) {
    if (driver == NULL) return 0;
    
    if (driver->rotation == VIEWE_LCD_ROTATION_90 || driver->rotation == VIEWE_LCD_ROTATION_270) {
        return VIEWE_LCD_HEIGHT;
    }
    return VIEWE_LCD_WIDTH;
}

int viewe_lcd_get_height(viewe_lcd_driver_t* driver) {
    if (driver == NULL) return 0;
    
    if (driver->rotation == VIEWE_LCD_ROTATION_90 || driver->rotation == VIEWE_LCD_ROTATION_270) {
        return VIEWE_LCD_WIDTH;
    }
    return VIEWE_LCD_HEIGHT;
}
