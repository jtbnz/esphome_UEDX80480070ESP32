#include "viewe_display.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP_IDF
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <driver/gpio.h>

namespace esphome {
namespace viewe_display {

static const char *const TAG = "viewe_display";

void VieweDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VIEWE RGB Display...");
  ESP_LOGCONFIG(TAG, "Display size: %dx%d", this->width_, this->height_);
  
  this->init_backlight_();
  this->init_lcd_();
  
  // Allocate buffer for drawing
  size_t buffer_size = this->get_buffer_length_();
  ESP_LOGCONFIG(TAG, "Allocating buffer size: %zu bytes", buffer_size);
  this->buffer_ = (uint16_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate buffer for display!");
    this->mark_failed();
    return;
  }
  ESP_LOGCONFIG(TAG, "Buffer allocated at: %p", this->buffer_);
  
  // Initialize buffer to black (will be overwritten by lambda)
  memset(this->buffer_, 0, buffer_size);
  ESP_LOGCONFIG(TAG, "Buffer initialized");
  
  // Force initial display update
  this->display_();
  
  ESP_LOGCONFIG(TAG, "VIEWE RGB Display setup completed");
}

void VieweDisplay::init_backlight_() {
  if (this->backlight_pin_ != 255) {
    ESP_LOGCONFIG(TAG, "Configuring backlight pin: %d", this->backlight_pin_);
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = 1ULL << this->backlight_pin_;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)this->backlight_pin_, 1);
  }
}

void VieweDisplay::init_lcd_() {
  ESP_LOGCONFIG(TAG, "Initializing RGB LCD panel...");
  ESP_LOGCONFIG(TAG, "Panel config - Width: %d, Height: %d", this->width_, this->height_);
  ESP_LOGCONFIG(TAG, "Pixel clock: %lu Hz", this->pixel_clock_frequency_);
  
  esp_lcd_rgb_panel_config_t panel_config = {};
  panel_config.data_width = 16; // RGB565
  panel_config.psram_trans_align = 64;
  panel_config.num_fbs = 1; // Single buffering for now to test
  panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
  
  // Configure pins
  panel_config.disp_gpio_num = (this->enable_pin_ == 255) ? GPIO_NUM_NC : (gpio_num_t)this->enable_pin_;
  panel_config.pclk_gpio_num = (gpio_num_t)this->pclk_pin_;
  panel_config.vsync_gpio_num = (gpio_num_t)this->vsync_pin_;
  panel_config.hsync_gpio_num = (gpio_num_t)this->hsync_pin_;
  panel_config.de_gpio_num = (gpio_num_t)this->de_pin_;
  
  // Configure RGB data pins (16 pins total: 5+6+5)
  int pin_idx = 0;
  // Blue pins (LSB first)
  for (int i = 0; i < 5; i++) {
    panel_config.data_gpio_nums[pin_idx++] = (gpio_num_t)this->blue_pins_[i];
  }
  // Green pins (LSB first)
  for (int i = 0; i < 6; i++) {
    panel_config.data_gpio_nums[pin_idx++] = (gpio_num_t)this->green_pins_[i];
  }
  // Red pins (LSB first)
  for (int i = 0; i < 5; i++) {
    panel_config.data_gpio_nums[pin_idx++] = (gpio_num_t)this->red_pins_[i];
  }
  
  // Configure timing
  panel_config.timings.pclk_hz = this->pixel_clock_frequency_;
  panel_config.timings.h_res = this->width_;
  panel_config.timings.v_res = this->height_;
  panel_config.timings.hsync_pulse_width = this->hsync_pulse_width_;
  panel_config.timings.hsync_back_porch = this->hsync_back_porch_;
  panel_config.timings.hsync_front_porch = this->hsync_front_porch_;
  panel_config.timings.vsync_pulse_width = this->vsync_pulse_width_;
  panel_config.timings.vsync_back_porch = this->vsync_back_porch_;
  panel_config.timings.vsync_front_porch = this->vsync_front_porch_;
  panel_config.timings.flags.pclk_active_neg = this->pclk_inverted_;
  panel_config.timings.flags.hsync_idle_low = this->hsync_idle_low_;
  panel_config.timings.flags.vsync_idle_low = this->vsync_idle_low_;
  panel_config.timings.flags.de_idle_high = this->de_idle_high_;
  
  // Use PSRAM for framebuffer
  panel_config.flags.fb_in_psram = true;
  
  ESP_LOGCONFIG(TAG, "Creating RGB panel...");
  esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &this->panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_lcd_new_rgb_panel failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }
  ESP_LOGCONFIG(TAG, "RGB panel created successfully");
  
  ESP_LOGCONFIG(TAG, "Resetting panel...");
  err = esp_lcd_panel_reset(this->panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(err));
  }
  
  ESP_LOGCONFIG(TAG, "Initializing panel...");
  err = esp_lcd_panel_init(this->panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(err));
  }
  
  ESP_LOGCONFIG(TAG, "RGB LCD panel initialized successfully");
}

void VieweDisplay::update() {
  // Execute the ESPHome display lambda (this calls draw_pixel_at)
  this->do_update_();
  // Now display the updated buffer to the LCD panel
  this->display_();
}

void VieweDisplay::display_() {
  if (this->panel_handle_ != nullptr && this->buffer_ != nullptr) {
    ESP_LOGD(TAG, "Drawing bitmap to display...");
    esp_err_t err = esp_lcd_panel_draw_bitmap(this->panel_handle_, 0, 0, this->width_, this->height_, this->buffer_);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Draw bitmap failed: %s", esp_err_to_name(err));
    } else {
      ESP_LOGD(TAG, "Bitmap drawn successfully");
    }
  } else {
    ESP_LOGW(TAG, "Cannot draw - panel_handle_: %p, buffer_: %p", this->panel_handle_, this->buffer_);
  }
}

void VieweDisplay::draw_pixel_at(int x, int y, Color color) {
  if (x >= this->get_width_internal() || x < 0 || y >= this->get_height_internal() || y < 0) {
    return;
  }
  
  // Convert to RGB565
  uint16_t color565 = ((color.red & 0xF8) << 8) | ((color.green & 0xFC) << 3) | (color.blue >> 3);
  this->buffer_[y * this->width_ + x] = color565;
}

void VieweDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "VIEWE RGB Display:");
  ESP_LOGCONFIG(TAG, "  Width: %d, Height: %d", this->width_, this->height_);
  ESP_LOGCONFIG(TAG, "  Red Pins: [%d,%d,%d,%d,%d]", 
                this->red_pins_[0], this->red_pins_[1], this->red_pins_[2], this->red_pins_[3], this->red_pins_[4]);
  ESP_LOGCONFIG(TAG, "  Green Pins: [%d,%d,%d,%d,%d,%d]", 
                this->green_pins_[0], this->green_pins_[1], this->green_pins_[2], 
                this->green_pins_[3], this->green_pins_[4], this->green_pins_[5]);
  ESP_LOGCONFIG(TAG, "  Blue Pins: [%d,%d,%d,%d,%d]", 
                this->blue_pins_[0], this->blue_pins_[1], this->blue_pins_[2], this->blue_pins_[3], this->blue_pins_[4]);
  ESP_LOGCONFIG(TAG, "  DE Pin: %d", this->de_pin_);
  ESP_LOGCONFIG(TAG, "  PCLK Pin: %d", this->pclk_pin_);
  ESP_LOGCONFIG(TAG, "  HSYNC Pin: %d", this->hsync_pin_);
  ESP_LOGCONFIG(TAG, "  VSYNC Pin: %d", this->vsync_pin_);
  if (this->backlight_pin_ != 255) {
    ESP_LOGCONFIG(TAG, "  Backlight Pin: %d", this->backlight_pin_);
  }
  ESP_LOGCONFIG(TAG, "  Pixel Clock: %.1f MHz", this->pixel_clock_frequency_ / 1e6);
  ESP_LOGCONFIG(TAG, "  PCLK Inverted: %s", this->pclk_inverted_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Color Depth: 16-bit RGB565");
  ESP_LOGCONFIG(TAG, "  Update Interval: %.3fs", this->get_update_interval() / 1000.0f);
}

}  // namespace viewe_display
}  // namespace esphome

#endif  // USE_ESP_IDF