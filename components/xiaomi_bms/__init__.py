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
    "XiaomiBMSComponent", cg.PollingComponent, uart.UARTDevice
)

# Shared constant re-used by the platform files
CONF_BMS_ID = "bms_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiBMSComponent),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("5s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
