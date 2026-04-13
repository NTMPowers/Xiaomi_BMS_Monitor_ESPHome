import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor

from . import XiaomiBMS

DEPENDENCIES = ["xiaomi_bms"]

CONF_BMS_ID = "bms_id"
CONF_CHARGING_ENABLED = "charging_enabled"
CONF_OVER_VOLTAGE = "over_voltage"
CONF_OVER_TEMPERATURE = "over_temperature"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BMS_ID): cv.use_id(XiaomiBMS),
        cv.Optional(CONF_CHARGING_ENABLED): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_OVER_VOLTAGE): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_OVER_TEMPERATURE): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_BMS_ID])

    if CONF_CHARGING_ENABLED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CHARGING_ENABLED])
        cg.add(parent.set_charging_enabled_sensor(sens))
    if CONF_OVER_VOLTAGE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_OVER_VOLTAGE])
        cg.add(parent.set_over_voltage_sensor(sens))
    if CONF_OVER_TEMPERATURE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_OVER_TEMPERATURE])
        cg.add(parent.set_over_temperature_sensor(sens))
