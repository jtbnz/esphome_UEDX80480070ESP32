#include "gt911_touch.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "gt911_touch";

// GT911 register addresses
#define GT911_REG_CONFIG_DATA   0x8047
#define GT911_REG_PRODUCT_ID    0x8140
#define GT911_REG_STATUS        0x814E
#define GT911_REG_TOUCH_DATA    0x814F

// Internal driver structure
struct gt911_driver_t {
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    gpio_num_t int_pin;
    gpio_num_t rst_pin;
    uint16_t width;
    uint16_t height;
    gt911_callback_t callback;
    void* user_data;
};

// I2C write helper
static esp_err_t gt911_write_reg(gt911_driver_t* driver, uint16_t reg, const uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (driver->i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg >> 8, true);
    i2c_master_write_byte(cmd, reg & 0xFF, true);
    if (len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(driver->i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// I2C read helper
static esp_err_t gt911_read_reg(gt911_driver_t* driver, uint16_t reg, uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (driver->i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg >> 8, true);
    i2c_master_write_byte(cmd, reg & 0xFF, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (driver->i2c_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(driver->i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Reset GT911
static esp_err_t gt911_reset(gt911_driver_t* driver) {
    if (driver->rst_pin == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "Reset pin not configured, skipping reset");
        return ESP_OK;
    }
    
    gpio_set_level(driver->rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(driver->rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return ESP_OK;
}

void gt911_get_default_config(gt911_config_t* config) {
    if (config == NULL) return;
    
    config->i2c_port = I2C_NUM_0;
    config->sda_pin = GT911_DEFAULT_SDA;
    config->scl_pin = GT911_DEFAULT_SCL;
    config->int_pin = GT911_DEFAULT_INT;
    config->rst_pin = GT911_DEFAULT_RST;
    config->i2c_addr = GT911_I2C_ADDR_1;
    config->i2c_freq = 100000;  // 100kHz
    config->width = 800;
    config->height = 480;
}

gt911_driver_t* gt911_init(const gt911_config_t* config) {
    gt911_driver_t* driver = (gt911_driver_t*)malloc(sizeof(gt911_driver_t));
    if (driver == NULL) {
        ESP_LOGE(TAG, "Failed to allocate driver structure");
        return NULL;
    }
    
    memset(driver, 0, sizeof(gt911_driver_t));
    
    // Apply configuration or use defaults
    gt911_config_t default_config;
    if (config == NULL) {
        gt911_get_default_config(&default_config);
        config = &default_config;
    }
    
    driver->i2c_port = config->i2c_port;
    driver->i2c_addr = config->i2c_addr;
    driver->int_pin = config->int_pin;
    driver->rst_pin = config->rst_pin;
    driver->width = config->width;
    driver->height = config->height;
    
    // Initialize I2C
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = config->i2c_freq,
    };
    
    esp_err_t ret = i2c_param_config(driver->i2c_port, &i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(ret));
        free(driver);
        return NULL;
    }
    
    ret = i2c_driver_install(driver->i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        free(driver);
        return NULL;
    }
    
    // Initialize interrupt pin if configured
    if (driver->int_pin != GPIO_NUM_NC) {
        gpio_config_t int_config = {
            .pin_bit_mask = 1ULL << driver->int_pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&int_config);
    }
    
    // Initialize reset pin if configured
    if (driver->rst_pin != GPIO_NUM_NC) {
        gpio_config_t rst_config = {
            .pin_bit_mask = 1ULL << driver->rst_pin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&rst_config);
        gpio_set_level(driver->rst_pin, 1);
    }
    
    // Reset GT911
    gt911_reset(driver);
    
    // Read product ID to verify communication
    uint8_t product_id[4];
    ret = gt911_read_reg(driver, GT911_REG_PRODUCT_ID, product_id, 4);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "GT911 Product ID: %c%c%c%c", 
                 product_id[0], product_id[1], product_id[2], product_id[3]);
    } else {
        ESP_LOGW(TAG, "Failed to read product ID, trying alternate address");
        // Try alternate address
        driver->i2c_addr = (driver->i2c_addr == GT911_I2C_ADDR_1) ? GT911_I2C_ADDR_2 : GT911_I2C_ADDR_1;
        ret = gt911_read_reg(driver, GT911_REG_PRODUCT_ID, product_id, 4);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "GT911 Product ID: %c%c%c%c (alternate address)", 
                     product_id[0], product_id[1], product_id[2], product_id[3]);
        } else {
            ESP_LOGE(TAG, "Failed to communicate with GT911");
            i2c_driver_delete(driver->i2c_port);
            free(driver);
            return NULL;
        }
    }
    
    ESP_LOGI(TAG, "GT911 touch controller initialized");
    return driver;
}

void gt911_deinit(gt911_driver_t* driver) {
    if (driver == NULL) return;
    
    i2c_driver_delete(driver->i2c_port);
    free(driver);
    ESP_LOGI(TAG, "GT911 touch controller deinitialized");
}

esp_err_t gt911_read_touch(gt911_driver_t* driver, gt911_touch_data_t* data) {
    if (driver == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(data, 0, sizeof(gt911_touch_data_t));
    
    // Read status register
    uint8_t status;
    esp_err_t ret = gt911_read_reg(driver, GT911_REG_STATUS, &status, 1);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Check if touch data is ready
    if ((status & 0x80) == 0) {
        data->touched = false;
        return ESP_OK;
    }
    
    // Get number of touch points
    uint8_t touch_count = status & 0x0F;
    if (touch_count > GT911_MAX_TOUCHES) {
        touch_count = GT911_MAX_TOUCHES;
    }
    
    data->touch_count = touch_count;
    data->touched = (touch_count > 0);
    
    // Read touch data
    if (touch_count > 0) {
        uint8_t touch_data[8];
        for (uint8_t i = 0; i < touch_count; i++) {
            ret = gt911_read_reg(driver, GT911_REG_TOUCH_DATA + (i * 8), touch_data, 8);
            if (ret != ESP_OK) {
                return ret;
            }
            
            data->points[i].track_id = touch_data[0];
            data->points[i].x = touch_data[1] | (touch_data[2] << 8);
            data->points[i].y = touch_data[3] | (touch_data[4] << 8);
            data->points[i].size = touch_data[5] | (touch_data[6] << 8);
            
            // Clamp to display bounds
            if (data->points[i].x >= driver->width) {
                data->points[i].x = driver->width - 1;
            }
            if (data->points[i].y >= driver->height) {
                data->points[i].y = driver->height - 1;
            }
        }
    }
    
    // Clear status register
    uint8_t clear = 0;
    gt911_write_reg(driver, GT911_REG_STATUS, &clear, 1);
    
    return ESP_OK;
}

esp_err_t gt911_register_callback(gt911_driver_t* driver, gt911_callback_t callback, void* user_data) {
    if (driver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    driver->callback = callback;
    driver->user_data = user_data;
    
    return ESP_OK;
}
