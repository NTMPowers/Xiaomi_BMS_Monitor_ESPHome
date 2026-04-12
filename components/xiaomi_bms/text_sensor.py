"""ESPHome Xiaomi BMS – text_sensor platform."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from . import XiaomiBMSComponent, CONF_BMS_ID

DEPENDENCIES = ["xiaomi_bms"]

CONF_SERIAL_NUMBER    = "serial_number"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_PACK_DATE        = "pack_date"
CONF_BALANCING_CELLS  = "balancing_cells"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BMS_ID): cv.use_id(XiaomiBMSComponent),

        cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(
            icon="mdi:identifier",
        ),
        cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(
            icon="mdi:chip",
        ),
        cv.Optional(CONF_PACK_DATE): text_sensor.text_sensor_schema(
            icon="mdi:calendar",
        ),
        cv.Optional(CONF_BALANCING_CELLS): text_sensor.text_sensor_schema(
            icon="mdi:scale-balance",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])

    for key, setter in [
        (CONF_SERIAL_NUMBER,    "set_serial_number_text_sensor"),
        (CONF_FIRMWARE_VERSION, "set_firmware_version_text_sensor"),
        (CONF_PACK_DATE,        "set_pack_date_text_sensor"),
        (CONF_BALANCING_CELLS,  "set_balancing_cells_text_sensor"),
    ]:
        if key in config:
            ts = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(hub, setter)(ts))
