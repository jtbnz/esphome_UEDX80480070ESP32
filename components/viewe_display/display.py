import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES,
    CONF_RESET_PIN,
    CONF_WIDTH, 
    CONF_HEIGHT,
)

from . import VieweDisplay, viewe_display_ns

DEPENDENCIES = ["esp32"]

CONF_DE_PIN = "de_pin"
CONF_HSYNC_PIN = "hsync_pin"
CONF_VSYNC_PIN = "vsync_pin"
CONF_PCLK_PIN = "pclk_pin" 
CONF_RED_PINS = "red_pins"
CONF_GREEN_PINS = "green_pins"  
CONF_BLUE_PINS = "blue_pins"
CONF_PIXEL_CLOCK_FREQUENCY = "pixel_clock_frequency"
CONF_HSYNC_PULSE_WIDTH = "hsync_pulse_width"
CONF_HSYNC_BACK_PORCH = "hsync_back_porch"
CONF_HSYNC_FRONT_PORCH = "hsync_front_porch"
CONF_VSYNC_PULSE_WIDTH = "vsync_pulse_width"
CONF_VSYNC_BACK_PORCH = "vsync_back_porch"
CONF_VSYNC_FRONT_PORCH = "vsync_front_porch"
CONF_ENABLE_PIN = "enable_pin"
CONF_BACKLIGHT_PIN = "backlight_pin"
CONF_PCLK_INVERTED = "pclk_inverted"
CONF_HSYNC_IDLE_LOW = "hsync_idle_low"
CONF_VSYNC_IDLE_LOW = "vsync_idle_low"
CONF_DE_IDLE_HIGH = "de_idle_high"

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(VieweDisplay),
            cv.Optional(CONF_WIDTH, default=800): cv.int_,
            cv.Optional(CONF_HEIGHT, default=480): cv.int_,
            cv.Required(CONF_RED_PINS): cv.All(
                cv.ensure_list(cv.int_range(min=0, max=48)), cv.Length(min=5, max=5)
            ),
            cv.Required(CONF_GREEN_PINS): cv.All(
                cv.ensure_list(cv.int_range(min=0, max=48)), cv.Length(min=6, max=6)
            ),
            cv.Required(CONF_BLUE_PINS): cv.All(
                cv.ensure_list(cv.int_range(min=0, max=48)), cv.Length(min=5, max=5)
            ),
            cv.Optional(CONF_DE_PIN, default=40): cv.int_range(min=0, max=48),
            cv.Optional(CONF_PCLK_PIN, default=42): cv.int_range(min=0, max=48),
            cv.Optional(CONF_HSYNC_PIN, default=39): cv.int_range(min=0, max=48),
            cv.Optional(CONF_VSYNC_PIN, default=41): cv.int_range(min=0, max=48),
            cv.Optional(CONF_ENABLE_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_RESET_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_BACKLIGHT_PIN, default=2): cv.int_range(min=0, max=48),
            cv.Optional(CONF_PIXEL_CLOCK_FREQUENCY, default="16MHz"): cv.frequency,
            cv.Optional(CONF_PCLK_INVERTED, default=True): cv.boolean,
            cv.Optional(CONF_HSYNC_IDLE_LOW, default=False): cv.boolean,
            cv.Optional(CONF_VSYNC_IDLE_LOW, default=False): cv.boolean,
            cv.Optional(CONF_DE_IDLE_HIGH, default=False): cv.boolean,
            cv.Optional(CONF_HSYNC_PULSE_WIDTH, default=10): cv.int_,
            cv.Optional(CONF_HSYNC_BACK_PORCH, default=10): cv.int_,
            cv.Optional(CONF_HSYNC_FRONT_PORCH, default=20): cv.int_,
            cv.Optional(CONF_VSYNC_PULSE_WIDTH, default=10): cv.int_,
            cv.Optional(CONF_VSYNC_BACK_PORCH, default=10): cv.int_,
            cv.Optional(CONF_VSYNC_FRONT_PORCH, default=10): cv.int_,
        }
    ).extend(cv.polling_component_schema("16ms")),
    cv.only_with_esp_idf,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await display.register_display(var, config)
    
    # Display dimensions
    cg.add(var.set_width(config[CONF_WIDTH]))
    cg.add(var.set_height(config[CONF_HEIGHT]))
    
    # RGB pins
    red_pins = config[CONF_RED_PINS]
    for i, pin in enumerate(red_pins):
        cg.add(var.set_red_pin(i, pin))
    
    green_pins = config[CONF_GREEN_PINS]
    for i, pin in enumerate(green_pins):
        cg.add(var.set_green_pin(i, pin))
        
    blue_pins = config[CONF_BLUE_PINS]  
    for i, pin in enumerate(blue_pins):
        cg.add(var.set_blue_pin(i, pin))
    
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
    cg.add(var.set_pclk_inverted(config[CONF_PCLK_INVERTED]))
    cg.add(var.set_hsync_idle_low(config[CONF_HSYNC_IDLE_LOW]))
    cg.add(var.set_vsync_idle_low(config[CONF_VSYNC_IDLE_LOW]))
    cg.add(var.set_de_idle_high(config[CONF_DE_IDLE_HIGH]))
    cg.add(var.set_hsync_pulse_width(config[CONF_HSYNC_PULSE_WIDTH]))
    cg.add(var.set_hsync_back_porch(config[CONF_HSYNC_BACK_PORCH]))
    cg.add(var.set_hsync_front_porch(config[CONF_HSYNC_FRONT_PORCH]))
    cg.add(var.set_vsync_pulse_width(config[CONF_VSYNC_PULSE_WIDTH]))
    cg.add(var.set_vsync_back_porch(config[CONF_VSYNC_BACK_PORCH]))
    cg.add(var.set_vsync_front_porch(config[CONF_VSYNC_FRONT_PORCH]))