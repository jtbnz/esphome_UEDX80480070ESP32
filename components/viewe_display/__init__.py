import esphome.codegen as cg
from esphome.components import display

DEPENDENCIES = ["esp32"]
CODEOWNERS = ["@viewe"]

viewe_display_ns = cg.esphome_ns.namespace("viewe_display")
VieweDisplay = viewe_display_ns.class_(
    "VieweDisplay", display.DisplayBuffer
)