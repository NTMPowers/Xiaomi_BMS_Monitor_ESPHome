import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor"]
DEPENDENCIES = ["uart"]

xiaomi_bms_ns = cg.esphome_ns.namespace("xiaomi_bms")
XiaomiBMS = xiaomi_bms_ns.class_("XiaomiBMS", cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiBMS),
        }
    )
    .extend(cv.polling_component_schema("2s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
