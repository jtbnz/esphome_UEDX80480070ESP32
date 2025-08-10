import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import touchscreen
from esphome.const import CONF_ID

DEPENDENCIES = ["viewe_display"]

viewe_touchscreen_ns = cg.esphome_ns.namespace("viewe_display")
VieweTouchscreen = viewe_touchscreen_ns.class_(
    "VieweTouchscreen", touchscreen.Touchscreen, cg.Component
)

CONFIG_SCHEMA = touchscreen.TOUCHSCREEN_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(VieweTouchscreen),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await touchscreen.register_touchscreen(var, config)
    await cg.register_component(var, config)