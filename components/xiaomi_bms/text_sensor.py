import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from . import XiaomiBMS

DEPENDENCIES = ["xiaomi_bms"]

CONF_BMS_ID = "bms_id"
CONF_SERIAL_NUMBER = "serial_number"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_PACK_DATE = "pack_date"
CONF_BALANCING_CELLS = "balancing_cells"
CONF_STATUS_FLAGS_ACTIVE = "status_flags_active"
CONF_ERROR_BYTES = "error_bytes"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BMS_ID): cv.use_id(XiaomiBMS),
        cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_PACK_DATE): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_BALANCING_CELLS): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_STATUS_FLAGS_ACTIVE): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_ERROR_BYTES): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_BMS_ID])

    if CONF_SERIAL_NUMBER in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SERIAL_NUMBER])
        cg.add(parent.set_serial_number_sensor(sens))
    if CONF_FIRMWARE_VERSION in config:
        sens = await text_sensor.new_text_sensor(config[CONF_FIRMWARE_VERSION])
        cg.add(parent.set_firmware_version_sensor(sens))
    if CONF_PACK_DATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_PACK_DATE])
        cg.add(parent.set_pack_date_sensor(sens))
    if CONF_BALANCING_CELLS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BALANCING_CELLS])
        cg.add(parent.set_balancing_cells_sensor(sens))
    if CONF_STATUS_FLAGS_ACTIVE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STATUS_FLAGS_ACTIVE])
        cg.add(parent.set_status_flags_active_sensor(sens))
    if CONF_ERROR_BYTES in config:
        sens = await text_sensor.new_text_sensor(config[CONF_ERROR_BYTES])
        cg.add(parent.set_error_bytes_sensor(sens))
