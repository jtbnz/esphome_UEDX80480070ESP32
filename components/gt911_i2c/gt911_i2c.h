#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace gt911_i2c {

class GT911I2C : public Component, public i2c::I2CDevice {
 public:
  void set_interrupt_pin(GPIOPin *pin) { interrupt_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { reset_pin_ = pin; }
  void set_x_sensor(sensor::Sensor *sensor) { x_sensor_ = sensor; }
  void set_y_sensor(sensor::Sensor *sensor) { y_sensor_ = sensor; }
  void set_touch_binary_sensor(binary_sensor::BinarySensor *sensor) { touch_sensor_ = sensor; }
  
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void reset_();
  bool read_touch_();
  
  GPIOPin *interrupt_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  sensor::Sensor *x_sensor_{nullptr};
  sensor::Sensor *y_sensor_{nullptr};
  binary_sensor::BinarySensor *touch_sensor_{nullptr};
  
  uint16_t x_{0};
  uint16_t y_{0};
  bool touched_{false};
  
  static constexpr uint8_t GT911_I2C_ADDRESS = 0x5D;
  static constexpr uint16_t GT911_READ_XY_REG = 0x814E;
  static constexpr uint16_t GT911_CLEARBUF_REG = 0x814E;
};

}  // namespace gt911_i2c
}  // namespace esphome