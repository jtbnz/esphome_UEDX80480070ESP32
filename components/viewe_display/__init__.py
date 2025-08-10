import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES,
)
from esphome.components import display, i2c
import esphome.components.touchscreen as touchscreen

DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["touchscreen"]

viewe_display_ns = cg.esphome_ns.namespace("viewe_display")
VieweDisplay = viewe_display_ns.class_(
    "VieweDisplay", display.Display, cg.Component
)
VieweDisplayRef = VieweDisplay.operator("ref")

CONFIG_SCHEMA = cv.All(
    display.DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(VieweDisplay),
            cv.Optional(CONF_LAMBDA): cv.returning_lambda,
            cv.Optional(CONF_PAGES): cv.All(
                cv.ensure_list(
                    {
                        cv.GenerateID(): cv.declare_id(display.DisplayPage),
                        cv.Required(CONF_LAMBDA): cv.lambda_,
                    }
                ),
                cv.Length(min=1),
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_with_esp_idf,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await display.register_display(var, config)
    await cg.register_component(var, config)
    
    # Add required libraries
    cg.add_library("lvgl/lvgl", "8.4.0")
    
    # Add build flags
    cg.add_build_flag("-DLV_CONF_INCLUDE_SIMPLE")
    cg.add_build_flag("-DBOARD_HAS_PSRAM")
    
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(VieweDisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
    
    if CONF_PAGES in config:
        pages = []
        for page_config in config[CONF_PAGES]:
            page = cg.new_Pvariable(page_config[CONF_ID], var)
            pages.append(page)
            lambda_ = await cg.process_lambda(
                page_config[CONF_LAMBDA], [(VieweDisplayRef, "it")], return_type=cg.void
            )
            cg.add(page.set_writer(lambda_))
        cg.add(var.set_pages(pages))