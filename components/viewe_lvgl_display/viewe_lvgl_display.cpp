#include "viewe_lvgl_display.h"

#ifdef USE_ESP_IDF

namespace esphome {
namespace viewe_lvgl_display {

// GT911 touch controller constants
#define GT911_I2C_ADDR          0x5D
#define GT911_PRODUCT_ID_REG    0x8140
#define GT911_CONFIG_REG        0x8047
#define GT911_COMMAND_REG       0x8040
#define GT911_STATUS_REG        0x814E
#define GT911_POINT_INFO_REG    0x814F
#define GT911_POINT1_REG        0x8150

// Buffer size for LVGL (using two buffers for better performance)
#define LVGL_BUFFER_SIZE (800 * 60)  // 60 lines buffer

void VieweLVGLDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VIEWE LVGL Display...");
  
  // Initialize backlight
  gpio_config_t backlight_cfg = {
    .pin_bit_mask = (1ULL << this->backlight_pin_),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&backlight_cfg);
  gpio_set_level((gpio_num_t)this->backlight_pin_, 1);  // Turn on backlight
  
  // Initialize LCD panel
  if (!this->init_lcd_()) {
    ESP_LOGE(TAG, "Failed to initialize LCD");
    this->mark_failed();
    return;
  }
  
  // Initialize touch controller
  if (!this->init_touch_()) {
    ESP_LOGW(TAG, "Failed to initialize touch controller");
  }
  
  // Initialize LVGL
  if (!this->init_lvgl_()) {
    ESP_LOGE(TAG, "Failed to initialize LVGL");
    this->mark_failed();
    return;
  }
  
  // Create demo widgets
  this->create_demo_widgets_();
  
  ESP_LOGCONFIG(TAG, "VIEWE LVGL Display setup complete");
}

void VieweLVGLDisplay::loop() {
  // LVGL task is handled by FreeRTOS task
  // Just yield here to allow other tasks to run
  delay(5);
}

void VieweLVGLDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "VIEWE LVGL Display:");
  ESP_LOGCONFIG(TAG, "  Width: %d", this->width_);
  ESP_LOGCONFIG(TAG, "  Height: %d", this->height_);
  ESP_LOGCONFIG(TAG, "  Backlight Pin: %d", this->backlight_pin_);
  ESP_LOGCONFIG(TAG, "  Touch SDA Pin: %d", this->touch_sda_pin_);
  ESP_LOGCONFIG(TAG, "  Touch SCL Pin: %d", this->touch_scl_pin_);
}

bool VieweLVGLDisplay::init_lcd_() {
  ESP_LOGI(TAG, "Initializing RGB LCD panel");
  
  esp_lcd_rgb_panel_config_t panel_config = {
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .timings = {
      .pclk_hz = 16000000,  // 16MHz pixel clock
      .h_res = this->width_,
      .v_res = this->height_,
      .hsync_pulse_width = 10,
      .hsync_back_porch = 10,
      .hsync_front_porch = 20,
      .vsync_pulse_width = 10,
      .vsync_back_porch = 10,
      .vsync_front_porch = 10,
      .flags = {
        .hsync_idle_low = 0,
        .vsync_idle_low = 0,
        .de_idle_high = 0,
        .pclk_active_neg = 1,
        .pclk_idle_high = 0,
      },
    },
    .data_width = 16,  // RGB565
    .bits_per_pixel = 16,
    .num_fbs = 2,      // Double buffering
    .bounce_buffer_size_px = 0,
    .sram_trans_align = 0,
    .psram_trans_align = 64,
    .hsync_gpio_num = (gpio_num_t)this->hsync_pin_,
    .vsync_gpio_num = (gpio_num_t)this->vsync_pin_,
    .de_gpio_num = (gpio_num_t)this->de_pin_,
    .pclk_gpio_num = (gpio_num_t)this->pclk_pin_,
    .disp_gpio_num = GPIO_NUM_NC,
    .data_gpio_nums = {
      // RGB565 format: R5G6B5
      (gpio_num_t)this->red_pins_[0],    // R0
      (gpio_num_t)this->red_pins_[1],    // R1
      (gpio_num_t)this->red_pins_[2],    // R2
      (gpio_num_t)this->red_pins_[3],    // R3
      (gpio_num_t)this->red_pins_[4],    // R4
      (gpio_num_t)this->green_pins_[0],  // G0
      (gpio_num_t)this->green_pins_[1],  // G1
      (gpio_num_t)this->green_pins_[2],  // G2
      (gpio_num_t)this->green_pins_[3],  // G3
      (gpio_num_t)this->green_pins_[4],  // G4
      (gpio_num_t)this->green_pins_[5],  // G5
      (gpio_num_t)this->blue_pins_[0],   // B0
      (gpio_num_t)this->blue_pins_[1],   // B1
      (gpio_num_t)this->blue_pins_[2],   // B2
      (gpio_num_t)this->blue_pins_[3],   // B3
      (gpio_num_t)this->blue_pins_[4],   // B4
    },
    .flags = {
      .disp_active_low = 0,
      .relax_on_idle = 0,
      .fb_in_psram = 1,
    },
  };
  
  esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &this->lcd_panel_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create RGB panel: %s", esp_err_to_name(ret));
    return false;
  }
  
  ret = esp_lcd_panel_reset(this->lcd_panel_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to reset panel: %s", esp_err_to_name(ret));
    return false;
  }
  
  ret = esp_lcd_panel_init(this->lcd_panel_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize panel: %s", esp_err_to_name(ret));
    return false;
  }
  
  ESP_LOGI(TAG, "RGB LCD panel initialized successfully");
  return true;
}

bool VieweLVGLDisplay::init_touch_() {
  ESP_LOGI(TAG, "Initializing GT911 touch controller");
  
  // Configure I2C for touch
  i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = (gpio_num_t)this->touch_sda_pin_,
    .scl_io_num = (gpio_num_t)this->touch_scl_pin_,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master = {
      .clk_speed = this->touch_frequency_,
    },
    .clk_flags = 0,
  };
  
  esp_err_t ret = i2c_param_config(I2C_NUM_0, &i2c_conf);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(ret));
    return false;
  }
  
  ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
    return false;
  }
  
  // Configure reset pin
  gpio_config_t reset_cfg = {
    .pin_bit_mask = (1ULL << this->touch_reset_pin_),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&reset_cfg);
  
  // Configure interrupt pin
  gpio_config_t int_cfg = {
    .pin_bit_mask = (1ULL << this->touch_interrupt_pin_),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&int_cfg);
  
  // Reset GT911
  gpio_set_level((gpio_num_t)this->touch_reset_pin_, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level((gpio_num_t)this->touch_reset_pin_, 1);
  vTaskDelay(pdMS_TO_TICKS(100));
  
  return this->gt911_init_();
}

bool VieweLVGLDisplay::init_lvgl_() {
  ESP_LOGI(TAG, "Initializing LVGL");
  
  // Initialize LVGL
  lv_init();
  
  // Create mutex for LVGL
  this->lvgl_mutex_ = xSemaphoreCreateMutex();
  if (!this->lvgl_mutex_) {
    ESP_LOGE(TAG, "Failed to create LVGL mutex");
    return false;
  }
  
  // Allocate buffers
  size_t buffer_size = LVGL_BUFFER_SIZE * sizeof(lv_color_t);
  this->buf1_ = (lv_color_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  this->buf2_ = (lv_color_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  
  if (!this->buf1_ || !this->buf2_) {
    ESP_LOGE(TAG, "Failed to allocate LVGL buffers");
    return false;
  }
  
  // Initialize display buffer
  lv_disp_draw_buf_init(&this->draw_buf_, this->buf1_, this->buf2_, LVGL_BUFFER_SIZE);
  
  // Register display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = this->width_;
  disp_drv.ver_res = this->height_;
  disp_drv.flush_cb = VieweLVGLDisplay::flush_cb_;
  disp_drv.draw_buf = &this->draw_buf_;
  disp_drv.user_data = this;
  this->display_ = lv_disp_drv_register(&disp_drv);
  
  if (!this->display_) {
    ESP_LOGE(TAG, "Failed to register display driver");
    return false;
  }
  
  // Register touch driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = VieweLVGLDisplay::touch_read_cb_;
  indev_drv.user_data = this;
  lv_indev_drv_register(&indev_drv);
  
  // Create LVGL task
  xTaskCreatePinnedToCore(VieweLVGLDisplay::lvgl_task_, "lvgl_task", 8192, this, 5, NULL, 1);
  
  ESP_LOGI(TAG, "LVGL initialized successfully");
  return true;
}

void VieweLVGLDisplay::create_demo_widgets_() {
  ESP_LOGI(TAG, "Creating demo widgets");
  
  // Create a simple test screen
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
  
  // Create a title label
  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "VIEWE 7\" Display Test");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_center(title);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 50);
  
  // Create some colored rectangles
  lv_obj_t *rect1 = lv_obj_create(scr);
  lv_obj_set_size(rect1, 100, 100);
  lv_obj_set_style_bg_color(rect1, lv_color_hex(0xFF0000), LV_PART_MAIN);
  lv_obj_align(rect1, LV_ALIGN_CENTER, -150, 0);
  
  lv_obj_t *rect2 = lv_obj_create(scr);
  lv_obj_set_size(rect2, 100, 100);
  lv_obj_set_style_bg_color(rect2, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_align(rect2, LV_ALIGN_CENTER, 0, 0);
  
  lv_obj_t *rect3 = lv_obj_create(scr);
  lv_obj_set_size(rect3, 100, 100);
  lv_obj_set_style_bg_color(rect3, lv_color_hex(0x0000FF), LV_PART_MAIN);
  lv_obj_align(rect3, LV_ALIGN_CENTER, 150, 0);
  
  // Create a button
  lv_obj_t *btn = lv_btn_create(scr);
  lv_obj_set_size(btn, 120, 50);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -50);
  
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Test Button");
  lv_obj_center(btn_label);
  
  // Create a status label
  lv_obj_t *status = lv_label_create(scr);
  lv_label_set_text(status, "ESPHome LVGL Integration Active");
  lv_obj_set_style_text_color(status, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void VieweLVGLDisplay::flush_cb_(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  VieweLVGLDisplay *display = (VieweLVGLDisplay*)drv->user_data;
  
  if (!display || !display->lcd_panel_) {
    lv_disp_flush_ready(drv);
    return;
  }
  
  int32_t x1 = area->x1;
  int32_t y1 = area->y1;
  int32_t x2 = area->x2;
  int32_t y2 = area->y2;
  
  esp_lcd_panel_draw_bitmap(display->lcd_panel_, x1, y1, x2 + 1, y2 + 1, color_p);
  lv_disp_flush_ready(drv);
}

void VieweLVGLDisplay::touch_read_cb_(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  VieweLVGLDisplay *display = (VieweLVGLDisplay*)drv->user_data;
  
  uint16_t x, y;
  bool pressed;
  
  if (display && display->gt911_read_touch_(&x, &y, &pressed)) {
    data->point.x = x;
    data->point.y = y;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void VieweLVGLDisplay::lvgl_task_(void *param) {
  VieweLVGLDisplay *display = (VieweLVGLDisplay*)param;
  
  while (true) {
    if (xSemaphoreTake(display->lvgl_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
      lv_timer_handler();
      xSemaphoreGive(display->lvgl_mutex_);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

bool VieweLVGLDisplay::gt911_init_() {
  uint8_t product_id[4];
  this->gt911_read_reg_(GT911_PRODUCT_ID_REG, product_id, 4);
  
  ESP_LOGI(TAG, "GT911 Product ID: %c%c%c%c", product_id[0], product_id[1], product_id[2], product_id[3]);
  
  // Simple initialization - the GT911 usually works out of the box
  uint8_t cmd = 0;
  this->gt911_write_reg_(GT911_COMMAND_REG, &cmd, 1);
  
  return true;
}

bool VieweLVGLDisplay::gt911_read_touch_(uint16_t *x, uint16_t *y, bool *pressed) {
  uint8_t status;
  this->gt911_read_reg_(GT911_STATUS_REG, &status, 1);
  
  if (!(status & 0x80)) {
    *pressed = false;
    return true;
  }
  
  uint8_t touch_count = status & 0x0F;
  if (touch_count > 0) {
    uint8_t point_data[8];
    this->gt911_read_reg_(GT911_POINT1_REG, point_data, 8);
    
    *x = (point_data[1] << 8) | point_data[0];
    *y = (point_data[3] << 8) | point_data[2];
    *pressed = true;
  } else {
    *pressed = false;
  }
  
  // Clear status
  uint8_t clear = 0;
  this->gt911_write_reg_(GT911_STATUS_REG, &clear, 1);
  
  return true;
}

void VieweLVGLDisplay::gt911_write_reg_(uint16_t reg, uint8_t *buf, uint8_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg >> 8, true);
  i2c_master_write_byte(cmd, reg & 0xFF, true);
  i2c_master_write(cmd, buf, len, true);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
}

void VieweLVGLDisplay::gt911_read_reg_(uint16_t reg, uint8_t *buf, uint8_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg >> 8, true);
  i2c_master_write_byte(cmd, reg & 0xFF, true);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_READ, true);
  i2c_master_read(cmd, buf, len, I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
}

}  // namespace viewe_lvgl_display
}  // namespace esphome

#endif  // USE_ESP_IDF