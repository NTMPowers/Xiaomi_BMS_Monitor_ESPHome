"""ESPHome Xiaomi BMS – component hub registration."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = True

CODEOWNERS = ["@esphome-xiaomi-bms"]

xiaomi_bms_ns = cg.esphome_ns.namespace("xiaomi_bms")
XiaomiBMSComponent = xiaomi_bms_ns.class_(
    "XiaomiBMSComponent", cg.Component, uart.UARTDevice
)

CONF_BMS_ID = "bms_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiBMSComponent),
            cv.Optional(CONF_UPDATE_INTERVAL, default="5s"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
