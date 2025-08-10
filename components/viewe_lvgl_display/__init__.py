import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_WIDTH,
    CONF_HEIGHT,
    CONF_SDA,
    CONF_SCL,
    CONF_RESET_PIN,
    CONF_INTERRUPT_PIN,
)

DEPENDENCIES = ["esp32"]
CODEOWNERS = ["@your-username"]

CONF_RGB_PINS = "rgb_pins"
CONF_RED = "red"
CONF_GREEN = "green"
CONF_BLUE = "blue"
CONF_CONTROL_PINS = "control_pins"
CONF_DE_PIN = "de_pin"
CONF_VSYNC_PIN = "vsync_pin"
CONF_HSYNC_PIN = "hsync_pin"
CONF_PCLK_PIN = "pclk_pin"
CONF_BACKLIGHT_PIN = "backlight_pin"
CONF_TOUCH = "touch"
CONF_I2C_FREQUENCY = "i2c_frequency"

viewe_lvgl_display_ns = cg.esphome_ns.namespace("viewe_lvgl_display")
VieweLVGLDisplay = viewe_lvgl_display_ns.class_("VieweLVGLDisplay", cg.Component)

def validate_rgb_pins(value):
    """Validate RGB pin configuration."""
    if not isinstance(value, dict):
        raise cv.Invalid("RGB pins must be a dictionary")
    
    red_pins = value.get(CONF_RED, [])
    green_pins = value.get(CONF_GREEN, [])
    blue_pins = value.get(CONF_BLUE, [])
    
    if len(red_pins) != 5:
        raise cv.Invalid("Red pins must have exactly 5 pins")
    if len(green_pins) != 6:
        raise cv.Invalid("Green pins must have exactly 6 pins") 
    if len(blue_pins) != 5:
        raise cv.Invalid("Blue pins must have exactly 5 pins")
    
    return value

RGB_PINS_SCHEMA = cv.Schema({
    cv.Required(CONF_RED): [cv.int_range(min=0, max=48)],
    cv.Required(CONF_GREEN): [cv.int_range(min=0, max=48)],
    cv.Required(CONF_BLUE): [cv.int_range(min=0, max=48)],
})

CONTROL_PINS_SCHEMA = cv.Schema({
    cv.Required(CONF_DE_PIN): cv.int_range(min=0, max=48),
    cv.Required(CONF_VSYNC_PIN): cv.int_range(min=0, max=48),
    cv.Required(CONF_HSYNC_PIN): cv.int_range(min=0, max=48),
    cv.Required(CONF_PCLK_PIN): cv.int_range(min=0, max=48),
})

TOUCH_SCHEMA = cv.Schema({
    cv.Required(CONF_SDA): cv.int_range(min=0, max=48),
    cv.Required(CONF_SCL): cv.int_range(min=0, max=48),
    cv.Required(CONF_RESET_PIN): cv.int_range(min=0, max=48),
    cv.Required(CONF_INTERRUPT_PIN): cv.int_range(min=0, max=48),
    cv.Optional(CONF_I2C_FREQUENCY, default="400kHz"): cv.frequency,
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(VieweLVGLDisplay),
    cv.Required(CONF_WIDTH): cv.int_range(min=1),
    cv.Required(CONF_HEIGHT): cv.int_range(min=1),
    cv.Required(CONF_RGB_PINS): cv.All(RGB_PINS_SCHEMA, validate_rgb_pins),
    cv.Required(CONF_CONTROL_PINS): CONTROL_PINS_SCHEMA,
    cv.Required(CONF_BACKLIGHT_PIN): cv.int_range(min=0, max=48),
    cv.Optional(CONF_TOUCH): TOUCH_SCHEMA,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Set display dimensions
    cg.add(var.set_width(config[CONF_WIDTH]))
    cg.add(var.set_height(config[CONF_HEIGHT]))
    
    # Set RGB pins
    rgb_pins = config[CONF_RGB_PINS]
    for i, pin in enumerate(rgb_pins[CONF_RED]):
        cg.add(var.set_red_pin(i, pin))
    for i, pin in enumerate(rgb_pins[CONF_GREEN]):
        cg.add(var.set_green_pin(i, pin))
    for i, pin in enumerate(rgb_pins[CONF_BLUE]):
        cg.add(var.set_blue_pin(i, pin))
    
    # Set control pins
    control_pins = config[CONF_CONTROL_PINS]
    cg.add(var.set_de_pin(control_pins[CONF_DE_PIN]))
    cg.add(var.set_vsync_pin(control_pins[CONF_VSYNC_PIN]))
    cg.add(var.set_hsync_pin(control_pins[CONF_HSYNC_PIN]))
    cg.add(var.set_pclk_pin(control_pins[CONF_PCLK_PIN]))
    
    # Set backlight pin
    cg.add(var.set_backlight_pin(config[CONF_BACKLIGHT_PIN]))
    
    # Configure touch if present
    if CONF_TOUCH in config:
        touch_config = config[CONF_TOUCH]
        cg.add(var.set_touch_sda_pin(touch_config[CONF_SDA]))
        cg.add(var.set_touch_scl_pin(touch_config[CONF_SCL]))
        cg.add(var.set_touch_reset_pin(touch_config[CONF_RESET_PIN]))
        cg.add(var.set_touch_interrupt_pin(touch_config[CONF_INTERRUPT_PIN]))
        cg.add(var.set_touch_frequency(touch_config[CONF_I2C_FREQUENCY]))
        cg.add(var.set_touch_enabled(True))
    else:
        cg.add(var.set_touch_enabled(False))
    
    # Add necessary libraries
    cg.add_library("lvgl/lvgl", "8.4.0")
    cg.add_library("SPI", None)
    cg.add_library("Wire", None)
    
    # Add build flags for LVGL
    cg.add_build_flag("-DLV_CONF_INCLUDE_SIMPLE")
    cg.add_build_flag("-DLV_COLOR_DEPTH=16")
    cg.add_build_flag("-DLV_COLOR_16_SWAP=0")  # RGB display, no swap needed