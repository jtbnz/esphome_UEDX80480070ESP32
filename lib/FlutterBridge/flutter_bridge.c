#include "flutter_bridge.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "flutter_bridge";

#define UART_NUM UART_NUM_0
#define UART_BUF_SIZE 1024
#define CMD_HEADER 0xAA
#define CMD_FOOTER 0x55

// Internal bridge structure
struct flutter_bridge_t {
    viewe_lcd_driver_t* lcd_driver;
    gt911_driver_t* touch_driver;
    bool enable_serial;
    bool enable_wifi;
    uint8_t rx_buffer[UART_BUF_SIZE];
};

// Command packet structure
typedef struct __attribute__((packed)) {
    uint8_t header;
    uint8_t cmd;
    uint16_t length;
    uint8_t data[];
} cmd_packet_t;

void flutter_bridge_get_default_config(flutter_bridge_config_t* config) {
    if (config == NULL) return;
    
    memset(config, 0, sizeof(flutter_bridge_config_t));
    config->enable_serial = true;
    config->enable_wifi = false;
    config->tcp_port = 8888;
}

flutter_bridge_t* flutter_bridge_init(const flutter_bridge_config_t* config) {
    if (config == NULL || config->lcd_driver == NULL) {
        ESP_LOGE(TAG, "Invalid configuration");
        return NULL;
    }
    
    flutter_bridge_t* bridge = (flutter_bridge_t*)malloc(sizeof(flutter_bridge_t));
    if (bridge == NULL) {
        ESP_LOGE(TAG, "Failed to allocate bridge structure");
        return NULL;
    }
    
    memset(bridge, 0, sizeof(flutter_bridge_t));
    bridge->lcd_driver = config->lcd_driver;
    bridge->touch_driver = config->touch_driver;
    bridge->enable_serial = config->enable_serial;
    bridge->enable_wifi = config->enable_wifi;
    
    // Initialize UART for serial communication
    if (bridge->enable_serial) {
        uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        
        esp_err_t ret = uart_param_config(UART_NUM, &uart_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(ret));
            free(bridge);
            return NULL;
        }
        
        ret = uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
            free(bridge);
            return NULL;
        }
        
        ESP_LOGI(TAG, "Serial communication enabled on UART%d at 115200 baud", UART_NUM);
    }
    
    ESP_LOGI(TAG, "Flutter bridge initialized");
    return bridge;
}

void flutter_bridge_deinit(flutter_bridge_t* bridge) {
    if (bridge == NULL) return;
    
    if (bridge->enable_serial) {
        uart_driver_delete(UART_NUM);
    }
    
    free(bridge);
    ESP_LOGI(TAG, "Flutter bridge deinitialized");
}

// Send response packet
static esp_err_t send_response(flutter_bridge_t* bridge, uint8_t resp_type, const uint8_t* data, uint16_t length) {
    if (!bridge->enable_serial) {
        return ESP_OK;
    }
    
    uint8_t header[4] = {CMD_HEADER, resp_type, length & 0xFF, (length >> 8) & 0xFF};
    uart_write_bytes(UART_NUM, (const char*)header, 4);
    
    if (length > 0 && data != NULL) {
        uart_write_bytes(UART_NUM, (const char*)data, length);
    }
    
    uint8_t footer = CMD_FOOTER;
    uart_write_bytes(UART_NUM, (const char*)&footer, 1);
    
    return ESP_OK;
}

// Process a single command
static esp_err_t process_command(flutter_bridge_t* bridge, uint8_t cmd, const uint8_t* data, uint16_t length) {
    esp_err_t ret = ESP_OK;
    
    switch (cmd) {
        case CMD_PING: {
            send_response(bridge, RESP_OK, NULL, 0);
            break;
        }
        
        case CMD_GET_DISPLAY_INFO: {
            uint8_t info[6];
            int width = viewe_lcd_get_width(bridge->lcd_driver);
            int height = viewe_lcd_get_height(bridge->lcd_driver);
            info[0] = width & 0xFF;
            info[1] = (width >> 8) & 0xFF;
            info[2] = height & 0xFF;
            info[3] = (height >> 8) & 0xFF;
            info[4] = 16;  // Bits per pixel
            info[5] = 0;   // Reserved
            send_response(bridge, RESP_DISPLAY_INFO, info, 6);
            break;
        }
        
        case CMD_SET_PIXEL: {
            if (length >= 7) {
                int16_t x = data[0] | (data[1] << 8);
                int16_t y = data[2] | (data[3] << 8);
                viewe_color_t color = {data[4], data[5], data[6]};
                ret = viewe_lcd_set_pixel(bridge->lcd_driver, x, y, color);
                send_response(bridge, ret == ESP_OK ? RESP_OK : RESP_ERROR, NULL, 0);
            }
            break;
        }
        
        case CMD_FILL: {
            if (length >= 3) {
                viewe_color_t color = {data[0], data[1], data[2]};
                ret = viewe_lcd_fill(bridge->lcd_driver, color);
                send_response(bridge, ret == ESP_OK ? RESP_OK : RESP_ERROR, NULL, 0);
            }
            break;
        }
        
        case CMD_FILL_RECT: {
            if (length >= 11) {
                int16_t x = data[0] | (data[1] << 8);
                int16_t y = data[2] | (data[3] << 8);
                int16_t width = data[4] | (data[5] << 8);
                int16_t height = data[6] | (data[7] << 8);
                viewe_color_t color = {data[8], data[9], data[10]};
                ret = viewe_lcd_fill_rect(bridge->lcd_driver, x, y, width, height, color);
                send_response(bridge, ret == ESP_OK ? RESP_OK : RESP_ERROR, NULL, 0);
            }
            break;
        }
        
        case CMD_DRAW_RECT: {
            if (length >= 11) {
                int16_t x = data[0] | (data[1] << 8);
                int16_t y = data[2] | (data[3] << 8);
                int16_t width = data[4] | (data[5] << 8);
                int16_t height = data[6] | (data[7] << 8);
                viewe_color_t color = {data[8], data[9], data[10]};
                ret = viewe_lcd_draw_rect(bridge->lcd_driver, x, y, width, height, color);
                send_response(bridge, ret == ESP_OK ? RESP_OK : RESP_ERROR, NULL, 0);
            }
            break;
        }
        
        case CMD_FLUSH: {
            ret = viewe_lcd_flush(bridge->lcd_driver);
            send_response(bridge, ret == ESP_OK ? RESP_OK : RESP_ERROR, NULL, 0);
            break;
        }
        
        case CMD_SET_BRIGHTNESS: {
            if (length >= 1) {
                ret = viewe_lcd_set_brightness(bridge->lcd_driver, data[0]);
                send_response(bridge, ret == ESP_OK ? RESP_OK : RESP_ERROR, NULL, 0);
            }
            break;
        }
        
        case CMD_SET_ROTATION: {
            if (length >= 1) {
                ret = viewe_lcd_set_rotation(bridge->lcd_driver, (viewe_lcd_rotation_t)data[0]);
                send_response(bridge, ret == ESP_OK ? RESP_OK : RESP_ERROR, NULL, 0);
            }
            break;
        }
        
        case CMD_GET_TOUCH: {
            if (bridge->touch_driver != NULL) {
                gt911_touch_data_t touch_data;
                ret = gt911_read_touch(bridge->touch_driver, &touch_data);
                if (ret == ESP_OK) {
                    uint8_t response[1 + GT911_MAX_TOUCHES * 7];
                    response[0] = touch_data.touch_count;
                    for (int i = 0; i < touch_data.touch_count && i < GT911_MAX_TOUCHES; i++) {
                        int offset = 1 + i * 7;
                        response[offset] = touch_data.points[i].x & 0xFF;
                        response[offset + 1] = (touch_data.points[i].x >> 8) & 0xFF;
                        response[offset + 2] = touch_data.points[i].y & 0xFF;
                        response[offset + 3] = (touch_data.points[i].y >> 8) & 0xFF;
                        response[offset + 4] = touch_data.points[i].size & 0xFF;
                        response[offset + 5] = (touch_data.points[i].size >> 8) & 0xFF;
                        response[offset + 6] = touch_data.points[i].track_id;
                    }
                    send_response(bridge, RESP_TOUCH_DATA, response, 1 + touch_data.touch_count * 7);
                } else {
                    send_response(bridge, RESP_ERROR, NULL, 0);
                }
            } else {
                send_response(bridge, RESP_ERROR, NULL, 0);
            }
            break;
        }
        
        default:
            ESP_LOGW(TAG, "Unknown command: 0x%02X", cmd);
            send_response(bridge, RESP_ERROR, NULL, 0);
            break;
    }
    
    return ret;
}

esp_err_t flutter_bridge_process(flutter_bridge_t* bridge) {
    if (bridge == NULL || !bridge->enable_serial) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int length = uart_read_bytes(UART_NUM, bridge->rx_buffer, UART_BUF_SIZE, pdMS_TO_TICKS(10));
    if (length <= 0) {
        return ESP_OK;
    }
    
    // Simple packet parser
    for (int i = 0; i < length - 4; i++) {
        if (bridge->rx_buffer[i] == CMD_HEADER) {
            uint8_t cmd = bridge->rx_buffer[i + 1];
            uint16_t data_len = bridge->rx_buffer[i + 2] | (bridge->rx_buffer[i + 3] << 8);
            
            if (i + 4 + data_len + 1 <= length && bridge->rx_buffer[i + 4 + data_len] == CMD_FOOTER) {
                process_command(bridge, cmd, &bridge->rx_buffer[i + 4], data_len);
                i += 4 + data_len;
            }
        }
    }
    
    return ESP_OK;
}

esp_err_t flutter_bridge_send_touch_event(flutter_bridge_t* bridge, gt911_touch_data_t* touch_data) {
    if (bridge == NULL || touch_data == NULL || !bridge->enable_serial) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t response[1 + GT911_MAX_TOUCHES * 7];
    response[0] = touch_data->touch_count;
    
    for (int i = 0; i < touch_data->touch_count && i < GT911_MAX_TOUCHES; i++) {
        int offset = 1 + i * 7;
        response[offset] = touch_data->points[i].x & 0xFF;
        response[offset + 1] = (touch_data->points[i].x >> 8) & 0xFF;
        response[offset + 2] = touch_data->points[i].y & 0xFF;
        response[offset + 3] = (touch_data->points[i].y >> 8) & 0xFF;
        response[offset + 4] = touch_data->points[i].size & 0xFF;
        response[offset + 5] = (touch_data->points[i].size >> 8) & 0xFF;
        response[offset + 6] = touch_data->points[i].track_id;
    }
    
    return send_response(bridge, RESP_TOUCH_DATA, response, 1 + touch_data->touch_count * 7);
}
