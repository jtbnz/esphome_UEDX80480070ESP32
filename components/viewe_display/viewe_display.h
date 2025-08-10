#pragma once

#include "esphome/core/component.h"
#include "esphome/components/display/display.h"

#ifdef USE_ESP_IDF

#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <driver/gpio.h>
#include <lvgl.h>

namespace esphome {
namespace viewe_display {

static const char *const TAG = "viewe_display";

class VieweDisplay : public display::Display, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_height_internal() override { return 480; }
  int get_width_internal() override { return 800; }
  size_t get_buffer_length_() { return 800 * 480 * 2; } // RGB565
  
  void display_();
  
  bool init_lcd_();
  bool init_touch_();
  bool init_lvgl_();
  
  esp_lcd_panel_handle_t lcd_panel_{nullptr};
  esp_lcd_panel_handle_t rgb_panel_{nullptr};
  
  lv_disp_draw_buf_t disp_buf_;
  lv_disp_drv_t disp_drv_;
  lv_disp_t *disp_{nullptr};
  lv_indev_drv_t indev_drv_;
  lv_indev_t *indev_{nullptr};
  
  lv_color_t *buf1_{nullptr};
  lv_color_t *buf2_{nullptr};
  
  uint16_t *framebuffer_{nullptr};
  
  static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
  static void touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
  static bool on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data);
};

}  // namespace viewe_display
}  // namespace esphome

#endif  // USE_ESP_IDF