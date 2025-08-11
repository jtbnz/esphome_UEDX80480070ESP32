import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display, i2c
from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES,
    CONF_RESET_PIN,
    CONF_WIDTH,
    CONF_HEIGHT,
)

DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["display"]

CONF_DE_PIN = "de_pin"
CONF_HSYNC_PIN = "hsync_pin"
CONF_VSYNC_PIN = "vsync_pin"  
CONF_PCLK_PIN = "pclk_pin"
CONF_DATA_PINS = "data_pins"
CONF_PIXEL_CLOCK_FREQUENCY = "pixel_clock_frequency"
CONF_HSYNC_PULSE_WIDTH = "hsync_pulse_width"
CONF_HSYNC_BACK_PORCH = "hsync_back_porch"
CONF_HSYNC_FRONT_PORCH = "hsync_front_porch"
CONF_VSYNC_PULSE_WIDTH = "vsync_pulse_width"
CONF_VSYNC_BACK_PORCH = "vsync_back_porch"
CONF_VSYNC_FRONT_PORCH = "vsync_front_porch"
CONF_ENABLE_PIN = "enable_pin"
CONF_BACKLIGHT_PIN = "backlight_pin"

viewe_rgb_display_ns = cg.esphome_ns.namespace("viewe_rgb").namespace("display")  
VieweRGBDisplay = viewe_rgb_display_ns.class_("VieweRGBDisplay", cg.PollingComponent, display.Display)

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(VieweRGBDisplay),
            cv.Required(CONF_WIDTH): cv.int_,
            cv.Required(CONF_HEIGHT): cv.int_,
            cv.Required(CONF_DATA_PINS): cv.All(
                cv.ensure_list(cv.int_range(min=0, max=48)), cv.Length(min=16, max=16)
            ),
            cv.Required(CONF_DE_PIN): cv.int_range(min=0, max=48),
            cv.Required(CONF_PCLK_PIN): cv.int_range(min=0, max=48),
            cv.Required(CONF_HSYNC_PIN): cv.int_range(min=0, max=48),
            cv.Required(CONF_VSYNC_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_ENABLE_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_RESET_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_BACKLIGHT_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_PIXEL_CLOCK_FREQUENCY, default="16MHz"): cv.frequency,
            cv.Optional(CONF_HSYNC_PULSE_WIDTH, default=10): cv.int_,
            cv.Optional(CONF_HSYNC_BACK_PORCH, default=10): cv.int_,
            cv.Optional(CONF_HSYNC_FRONT_PORCH, default=20): cv.int_,
            cv.Optional(CONF_VSYNC_PULSE_WIDTH, default=10): cv.int_,
            cv.Optional(CONF_VSYNC_BACK_PORCH, default=10): cv.int_,
            cv.Optional(CONF_VSYNC_FRONT_PORCH, default=10): cv.int_,
        }
    ).extend(cv.polling_component_schema("1s")),
    cv.only_with_esp_idf,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await display.register_display(var, config)
    
    # Display dimensions
    cg.add(var.set_width(config[CONF_WIDTH]))
    cg.add(var.set_height(config[CONF_HEIGHT]))
    
    # Data pins (16 pins for RGB565)
    data_pins = config[CONF_DATA_PINS]
    for i, pin in enumerate(data_pins):
        cg.add(var.set_data_pin(i, pin))
    
    # Control pins
    cg.add(var.set_de_pin(config[CONF_DE_PIN]))
    cg.add(var.set_pclk_pin(config[CONF_PCLK_PIN]))
    cg.add(var.set_hsync_pin(config[CONF_HSYNC_PIN]))
    cg.add(var.set_vsync_pin(config[CONF_VSYNC_PIN]))
    
    # Optional pins
    if CONF_ENABLE_PIN in config:
        cg.add(var.set_enable_pin(config[CONF_ENABLE_PIN]))
    if CONF_RESET_PIN in config:
        cg.add(var.set_reset_pin(config[CONF_RESET_PIN]))
    if CONF_BACKLIGHT_PIN in config:
        cg.add(var.set_backlight_pin(config[CONF_BACKLIGHT_PIN]))
    
    # Timing configuration
    cg.add(var.set_pixel_clock_frequency(config[CONF_PIXEL_CLOCK_FREQUENCY]))
    cg.add(var.set_hsync_pulse_width(config[CONF_HSYNC_PULSE_WIDTH]))
    cg.add(var.set_hsync_back_porch(config[CONF_HSYNC_BACK_PORCH]))
    cg.add(var.set_hsync_front_porch(config[CONF_HSYNC_FRONT_PORCH]))
    cg.add(var.set_vsync_pulse_width(config[CONF_VSYNC_PULSE_WIDTH]))
    cg.add(var.set_vsync_back_porch(config[CONF_VSYNC_BACK_PORCH]))
    cg.add(var.set_vsync_front_porch(config[CONF_VSYNC_FRONT_PORCH]))