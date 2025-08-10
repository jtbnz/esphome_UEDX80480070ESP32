#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// LVGL includes
#include "lvgl.h"

namespace esphome {
namespace viewe_lvgl_display {

static const char *const TAG = "viewe_lvgl_display";

class VieweLVGLDisplay : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Configuration setters
  void set_width(uint16_t width) { this->width_ = width; }
  void set_height(uint16_t height) { this->height_ = height; }
  
  void set_red_pin(uint8_t index, uint8_t pin) { 
    if (index < 5) this->red_pins_[index] = pin; 
  }
  void set_green_pin(uint8_t index, uint8_t pin) { 
    if (index < 6) this->green_pins_[index] = pin; 
  }
  void set_blue_pin(uint8_t index, uint8_t pin) { 
    if (index < 5) this->blue_pins_[index] = pin; 
  }
  
  void set_de_pin(uint8_t pin) { this->de_pin_ = pin; }
  void set_vsync_pin(uint8_t pin) { this->vsync_pin_ = pin; }
  void set_hsync_pin(uint8_t pin) { this->hsync_pin_ = pin; }
  void set_pclk_pin(uint8_t pin) { this->pclk_pin_ = pin; }
  void set_backlight_pin(uint8_t pin) { this->backlight_pin_ = pin; }
  
  void set_touch_sda_pin(uint8_t pin) { this->touch_sda_pin_ = pin; }
  void set_touch_scl_pin(uint8_t pin) { this->touch_scl_pin_ = pin; }
  void set_touch_reset_pin(uint8_t pin) { this->touch_reset_pin_ = pin; }
  void set_touch_interrupt_pin(uint8_t pin) { this->touch_interrupt_pin_ = pin; }
  void set_touch_frequency(uint32_t freq) { this->touch_frequency_ = freq; }

  // Get LVGL display object
  lv_disp_t *get_display() { return this->display_; }

 protected:
  // Display configuration
  uint16_t width_{800};
  uint16_t height_{480};
  uint8_t red_pins_[5];
  uint8_t green_pins_[6];
  uint8_t blue_pins_[5];
  uint8_t de_pin_;
  uint8_t vsync_pin_;
  uint8_t hsync_pin_;
  uint8_t pclk_pin_;
  uint8_t backlight_pin_;
  
  // Touch configuration
  uint8_t touch_sda_pin_;
  uint8_t touch_scl_pin_;
  uint8_t touch_reset_pin_;
  uint8_t touch_interrupt_pin_;
  uint32_t touch_frequency_{400000};
  
  // Internal objects
  esp_lcd_panel_handle_t lcd_panel_{nullptr};
  lv_disp_t *display_{nullptr};
  lv_disp_draw_buf_t draw_buf_;
  lv_color_t *buf1_{nullptr};
  lv_color_t *buf2_{nullptr};
  SemaphoreHandle_t lvgl_mutex_{nullptr};
  
  // Internal methods
  bool init_lcd_();
  bool init_touch_();
  bool init_lvgl_();
  void create_demo_widgets_();
  
  // Static callbacks
  static void flush_cb_(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
  static void touch_read_cb_(lv_indev_drv_t *drv, lv_indev_data_t *data);
  static void lvgl_task_(void *param);
  
  // Touch helpers
  bool gt911_read_touch_(uint16_t *x, uint16_t *y, bool *pressed);
  bool gt911_init_();
  void gt911_write_reg_(uint16_t reg, uint8_t *buf, uint8_t len);
  void gt911_read_reg_(uint16_t reg, uint8_t *buf, uint8_t len);
};

}  // namespace viewe_lvgl_display
}  // namespace esphome

#endif  // USE_ESP_IDF