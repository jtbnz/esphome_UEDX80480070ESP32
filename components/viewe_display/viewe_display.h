#pragma once

#include "esphome/core/component.h"
#include "esphome/components/display/display.h"

#ifdef USE_ESP_IDF
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <driver/gpio.h>

namespace esphome {
namespace viewe_display {

class VieweDisplay : public display::Display {
 public:
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
  void set_pclk_pin(uint8_t pin) { this->pclk_pin_ = pin; }
  void set_hsync_pin(uint8_t pin) { this->hsync_pin_ = pin; }
  void set_vsync_pin(uint8_t pin) { this->vsync_pin_ = pin; }
  void set_enable_pin(uint8_t pin) { this->enable_pin_ = pin; }
  void set_reset_pin(uint8_t pin) { this->reset_pin_ = pin; }
  void set_backlight_pin(uint8_t pin) { this->backlight_pin_ = pin; }
  
  void set_pixel_clock_frequency(uint32_t freq) { this->pixel_clock_frequency_ = freq; }
  void set_pclk_inverted(bool inverted) { this->pclk_inverted_ = inverted; }
  void set_hsync_idle_low(bool idle_low) { this->hsync_idle_low_ = idle_low; }
  void set_vsync_idle_low(bool idle_low) { this->vsync_idle_low_ = idle_low; }
  void set_de_idle_high(bool idle_high) { this->de_idle_high_ = idle_high; }
  void set_hsync_pulse_width(uint16_t width) { this->hsync_pulse_width_ = width; }
  void set_hsync_back_porch(uint16_t porch) { this->hsync_back_porch_ = porch; }
  void set_hsync_front_porch(uint16_t porch) { this->hsync_front_porch_ = porch; }
  void set_vsync_pulse_width(uint16_t width) { this->vsync_pulse_width_ = width; }
  void set_vsync_back_porch(uint16_t porch) { this->vsync_back_porch_ = porch; }
  void set_vsync_front_porch(uint16_t porch) { this->vsync_front_porch_ = porch; }
  
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

  void draw_pixel_at(int x, int y, Color color) override;

 protected:
  int get_height_internal() override { return this->height_; }
  int get_width_internal() override { return this->width_; }
  size_t get_buffer_length_() { return this->width_ * this->height_ * 2; } // RGB565
  
  void init_lcd_();
  void init_backlight_();
  void display_();
  
  uint16_t width_{800};
  uint16_t height_{480};
  uint8_t red_pins_[5];
  uint8_t green_pins_[6];
  uint8_t blue_pins_[5];
  uint8_t de_pin_{40};
  uint8_t pclk_pin_{42};
  uint8_t hsync_pin_{39};
  uint8_t vsync_pin_{41};
  uint8_t enable_pin_{255};  // GPIO_NUM_NC
  uint8_t reset_pin_{255};   // GPIO_NUM_NC
  uint8_t backlight_pin_{2};
  
  uint32_t pixel_clock_frequency_{16000000};
  bool pclk_inverted_{true};
  bool hsync_idle_low_{false};
  bool vsync_idle_low_{false};
  bool de_idle_high_{false};
  uint16_t hsync_pulse_width_{10};
  uint16_t hsync_back_porch_{10};
  uint16_t hsync_front_porch_{20};
  uint16_t vsync_pulse_width_{10};
  uint16_t vsync_back_porch_{10};
  uint16_t vsync_front_porch_{10};
  
  esp_lcd_panel_handle_t panel_handle_{nullptr};
  uint16_t *buffer_{nullptr};
  bool need_update_{false};
};

}  // namespace viewe_display
}  // namespace esphome

#endif  // USE_ESP_IDF