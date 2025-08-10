#include "viewe_display.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

#ifdef USE_ESP_IDF

#include <esp_lcd_panel_rgb.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace esphome {
namespace viewe_display {

// Display pins - VIEWE 7-inch specific
#define LCD_PIXEL_CLOCK_HZ (16 * 1000 * 1000)
#define LCD_H_RES 800
#define LCD_V_RES 480
#define LCD_HSYNC_PULSE_WIDTH 10
#define LCD_HSYNC_BACK_PORCH 10  
#define LCD_HSYNC_FRONT_PORCH 20
#define LCD_VSYNC_PULSE_WIDTH 10
#define LCD_VSYNC_BACK_PORCH 10
#define LCD_VSYNC_FRONT_PORCH 10

// RGB pins
#define LCD_DATA0 GPIO_NUM_8   // B0
#define LCD_DATA1 GPIO_NUM_3   // B1
#define LCD_DATA2 GPIO_NUM_46  // B2
#define LCD_DATA3 GPIO_NUM_9   // B3
#define LCD_DATA4 GPIO_NUM_1   // B4
#define LCD_DATA5 GPIO_NUM_5   // G0
#define LCD_DATA6 GPIO_NUM_6   // G1
#define LCD_DATA7 GPIO_NUM_7   // G2
#define LCD_DATA8 GPIO_NUM_15  // G3
#define LCD_DATA9 GPIO_NUM_16  // G4
#define LCD_DATA10 GPIO_NUM_4  // G5
#define LCD_DATA11 GPIO_NUM_45 // R0
#define LCD_DATA12 GPIO_NUM_48 // R1
#define LCD_DATA13 GPIO_NUM_47 // R2
#define LCD_DATA14 GPIO_NUM_21 // R3
#define LCD_DATA15 GPIO_NUM_14 // R4

// Control pins
#define LCD_PCLK GPIO_NUM_42
#define LCD_HSYNC GPIO_NUM_39
#define LCD_VSYNC GPIO_NUM_41
#define LCD_DE GPIO_NUM_40
#define LCD_DISP_EN_GPIO GPIO_NUM_NC
#define LCD_BK_LIGHT GPIO_NUM_2

// Touch pins (GT911)
#define TOUCH_I2C_NUM I2C_NUM_0
#define TOUCH_I2C_SDA GPIO_NUM_19
#define TOUCH_I2C_SCL GPIO_NUM_20
#define TOUCH_RST GPIO_NUM_38
#define TOUCH_INT GPIO_NUM_18
#define GT911_I2C_ADDR 0x5D

// GT911 registers
#define GT911_PRODUCT_ID_REG 0x8140
#define GT911_CONFIG_REG 0x8047
#define GT911_STATUS_REG 0x814E
#define GT911_POINT_INFO_REG 0x814F
#define GT911_POINT1_REG 0x8150

// LVGL buffer size
#define LVGL_BUF_SIZE (LCD_H_RES * 60)

static VieweDisplay *instance = nullptr;
static SemaphoreHandle_t lvgl_mutex = nullptr;

void VieweDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VIEWE 7-inch Display...");
  
  instance = this;
  
  // Initialize backlight
  gpio_config_t bk_gpio_config = {
    .pin_bit_mask = 1ULL << LCD_BK_LIGHT,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&bk_gpio_config);
  gpio_set_level(LCD_BK_LIGHT, 1);
  
  // Initialize LCD
  if (!init_lcd_()) {
    ESP_LOGE(TAG, "Failed to initialize LCD");
    this->mark_failed();
    return;
  }
  
  // Initialize touch
  if (!init_touch_()) {
    ESP_LOGW(TAG, "Failed to initialize touch controller");
  }
  
  // Initialize LVGL
  if (!init_lvgl_()) {
    ESP_LOGE(TAG, "Failed to initialize LVGL");
    this->mark_failed();
    return;
  }
  
  ESP_LOGCONFIG(TAG, "VIEWE Display setup complete");
}

bool VieweDisplay::init_lcd_() {
  ESP_LOGI(TAG, "Initializing RGB LCD panel");
  
  esp_lcd_rgb_panel_config_t panel_config = {};
  panel_config.data_width = 16; // RGB565
  panel_config.psram_trans_align = 64;
  panel_config.num_fbs = 2; // Double buffering
  panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
  panel_config.disp_gpio_num = LCD_DISP_EN_GPIO;
  panel_config.pclk_gpio_num = LCD_PCLK;
  panel_config.vsync_gpio_num = LCD_VSYNC;
  panel_config.hsync_gpio_num = LCD_HSYNC;
  panel_config.de_gpio_num = LCD_DE;
  panel_config.data_gpio_nums[0] = LCD_DATA0;
  panel_config.data_gpio_nums[1] = LCD_DATA1;
  panel_config.data_gpio_nums[2] = LCD_DATA2;
  panel_config.data_gpio_nums[3] = LCD_DATA3;
  panel_config.data_gpio_nums[4] = LCD_DATA4;
  panel_config.data_gpio_nums[5] = LCD_DATA5;
  panel_config.data_gpio_nums[6] = LCD_DATA6;
  panel_config.data_gpio_nums[7] = LCD_DATA7;
  panel_config.data_gpio_nums[8] = LCD_DATA8;
  panel_config.data_gpio_nums[9] = LCD_DATA9;
  panel_config.data_gpio_nums[10] = LCD_DATA10;
  panel_config.data_gpio_nums[11] = LCD_DATA11;
  panel_config.data_gpio_nums[12] = LCD_DATA12;
  panel_config.data_gpio_nums[13] = LCD_DATA13;
  panel_config.data_gpio_nums[14] = LCD_DATA14;
  panel_config.data_gpio_nums[15] = LCD_DATA15;
  
  panel_config.timings.pclk_hz = LCD_PIXEL_CLOCK_HZ;
  panel_config.timings.h_res = LCD_H_RES;
  panel_config.timings.v_res = LCD_V_RES;
  panel_config.timings.hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH;
  panel_config.timings.hsync_back_porch = LCD_HSYNC_BACK_PORCH;
  panel_config.timings.hsync_front_porch = LCD_HSYNC_FRONT_PORCH;
  panel_config.timings.vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH;
  panel_config.timings.vsync_back_porch = LCD_VSYNC_BACK_PORCH;
  panel_config.timings.vsync_front_porch = LCD_VSYNC_FRONT_PORCH;
  panel_config.timings.flags.pclk_active_neg = true;
  
  panel_config.flags.fb_in_psram = true; // Use PSRAM for framebuffer
  
  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &rgb_panel_));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(rgb_panel_));
  ESP_ERROR_CHECK(esp_lcd_panel_init(rgb_panel_));
  
  ESP_LOGI(TAG, "RGB LCD panel initialized successfully");
  return true;
}

bool VieweDisplay::init_touch_() {
  ESP_LOGI(TAG, "Initializing GT911 touch controller");
  
  // Initialize I2C
  i2c_config_t i2c_conf = {};
  i2c_conf.mode = I2C_MODE_MASTER;
  i2c_conf.sda_io_num = TOUCH_I2C_SDA;
  i2c_conf.scl_io_num = TOUCH_I2C_SCL;
  i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_conf.master.clk_speed = 400000;
  
  ESP_ERROR_CHECK(i2c_param_config(TOUCH_I2C_NUM, &i2c_conf));
  ESP_ERROR_CHECK(i2c_driver_install(TOUCH_I2C_NUM, i2c_conf.mode, 0, 0, 0));
  
  // Reset touch controller
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << TOUCH_RST) | (1ULL << TOUCH_INT);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&io_conf);
  
  // Reset sequence
  gpio_set_level(TOUCH_RST, 0);
  gpio_set_level(TOUCH_INT, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(TOUCH_RST, 1);
  vTaskDelay(pdMS_TO_TICKS(100));
  
  // Configure INT pin as input
  io_conf.pin_bit_mask = 1ULL << TOUCH_INT;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
  
  // Read product ID to verify communication
  uint8_t product_id[4];
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, (GT911_PRODUCT_ID_REG >> 8) & 0xFF, true);
  i2c_master_write_byte(cmd, GT911_PRODUCT_ID_REG & 0xFF, true);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_READ, true);
  i2c_master_read(cmd, product_id, 4, I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(TOUCH_I2C_NUM, cmd, pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "GT911 Product ID: %c%c%c%c", product_id[0], product_id[1], product_id[2], product_id[3]);
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to read GT911 Product ID");
    return false;
  }
}

bool VieweDisplay::init_lvgl_() {
  ESP_LOGI(TAG, "Initializing LVGL");
  
  lv_init();
  
  // Create mutex for LVGL
  lvgl_mutex = xSemaphoreCreateMutex();
  
  // Allocate draw buffers in PSRAM
  buf1_ = (lv_color_t *)heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  buf2_ = (lv_color_t *)heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  
  if (!buf1_ || !buf2_) {
    ESP_LOGE(TAG, "Failed to allocate LVGL buffers");
    return false;
  }
  
  lv_disp_draw_buf_init(&disp_buf_, buf1_, buf2_, LVGL_BUF_SIZE);
  
  lv_disp_drv_init(&disp_drv_);
  disp_drv_.hor_res = LCD_H_RES;
  disp_drv_.ver_res = LCD_V_RES;
  disp_drv_.flush_cb = flush_cb;
  disp_drv_.draw_buf = &disp_buf_;
  disp_drv_.user_data = this;
  disp_ = lv_disp_drv_register(&disp_drv_);
  
  // Initialize touch input device
  lv_indev_drv_init(&indev_drv_);
  indev_drv_.type = LV_INDEV_TYPE_POINTER;
  indev_drv_.read_cb = touch_cb;
  indev_drv_.user_data = this;
  indev_ = lv_indev_drv_register(&indev_drv_);
  
  // Create a simple test screen
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x003a57), LV_PART_MAIN);
  
  // Title
  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "VIEWE 7\" Display - ESPHome");
  lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
  
  // Color test bars
  const uint32_t colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF, 0xFFFFFF};
  for(int i = 0; i < 7; i++) {
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, 100, 80);
    lv_obj_align(bar, LV_ALIGN_CENTER, -300 + i * 100, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(colors[i]), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
  }
  
  // Status label
  lv_obj_t *status = lv_label_create(scr);
  lv_label_set_text(status, "RGB565 Full Color Mode Active");
  lv_obj_set_style_text_color(status, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -20);
  
  ESP_LOGI(TAG, "LVGL initialized successfully");
  return true;
}

void VieweDisplay::flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  VieweDisplay *display = (VieweDisplay *)drv->user_data;
  if (!display || !display->rgb_panel_) {
    lv_disp_flush_ready(drv);
    return;
  }
  
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;
  
  esp_lcd_panel_draw_bitmap(display->rgb_panel_, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_p);
  lv_disp_flush_ready(drv);
}

void VieweDisplay::touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  static uint16_t last_x = 0, last_y = 0;
  
  // Read touch status from GT911
  uint8_t touch_points = 0;
  uint8_t point_data[8];
  
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, (GT911_STATUS_REG >> 8) & 0xFF, true);
  i2c_master_write_byte(cmd, GT911_STATUS_REG & 0xFF, true);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_READ, true);
  i2c_master_read_byte(cmd, &touch_points, I2C_MASTER_ACK);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(TOUCH_I2C_NUM, cmd, pdMS_TO_TICKS(10));
  i2c_cmd_link_delete(cmd);
  
  if (ret == ESP_OK && (touch_points & 0x80)) {
    touch_points &= 0x0F;
    
    if (touch_points > 0) {
      // Read first touch point
      cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, (GT911_POINT1_REG >> 8) & 0xFF, true);
      i2c_master_write_byte(cmd, GT911_POINT1_REG & 0xFF, true);
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_READ, true);
      i2c_master_read(cmd, point_data, 8, I2C_MASTER_LAST_NACK);
      i2c_master_stop(cmd);
      i2c_master_cmd_begin(TOUCH_I2C_NUM, cmd, pdMS_TO_TICKS(10));
      i2c_cmd_link_delete(cmd);
      
      last_x = ((point_data[2] & 0x0f) << 8) | point_data[1];
      last_y = ((point_data[4] & 0x0f) << 8) | point_data[3];
      data->state = LV_INDEV_STATE_PR;
    } else {
      data->state = LV_INDEV_STATE_REL;
    }
    
    // Clear touch status
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GT911_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, (GT911_STATUS_REG >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, GT911_STATUS_REG & 0xFF, true);
    i2c_master_write_byte(cmd, 0, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(TOUCH_I2C_NUM, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
  
  data->point.x = last_x;
  data->point.y = last_y;
}

void VieweDisplay::loop() {
  if (lvgl_mutex) {
    if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      lv_timer_handler();
      xSemaphoreGive(lvgl_mutex);
    }
  }
}

void VieweDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "VIEWE Display:");
  ESP_LOGCONFIG(TAG, "  Resolution: 800x480");
  ESP_LOGCONFIG(TAG, "  Color Depth: 16-bit RGB565");
  ESP_LOGCONFIG(TAG, "  Touch: GT911");
}

void VieweDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (!this->framebuffer_) return;
  if (x < 0 || x >= LCD_H_RES || y < 0 || y >= LCD_V_RES) return;
  
  uint16_t col = ((color.red & 0xF8) << 8) | ((color.green & 0xFC) << 3) | (color.blue >> 3);
  this->framebuffer_[y * LCD_H_RES + x] = col;
}

bool VieweDisplay::on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data) {
  return false;
}

}  // namespace viewe_display
}  // namespace esphome

#endif  // USE_ESP_IDF