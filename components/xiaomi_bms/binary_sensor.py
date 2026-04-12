"""ESPHome Xiaomi BMS – binary_sensor platform."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import XiaomiBMSComponent, CONF_BMS_ID

DEPENDENCIES = ["xiaomi_bms"]

CONF_CHARGING_ENABLED = "charging_enabled"
CONF_OVER_VOLTAGE     = "over_voltage"
CONF_OVER_TEMPERATURE = "over_temperature"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BMS_ID): cv.use_id(XiaomiBMSComponent),

        cv.Optional(CONF_CHARGING_ENABLED): binary_sensor.binary_sensor_schema(
            device_class="battery_charging",
        ),
        cv.Optional(CONF_OVER_VOLTAGE): binary_sensor.binary_sensor_schema(
            device_class="problem",
        ),
        cv.Optional(CONF_OVER_TEMPERATURE): binary_sensor.binary_sensor_schema(
            device_class="problem",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])

    for key, setter in [
        (CONF_CHARGING_ENABLED, "set_charging_enabled_binary_sensor"),
        (CONF_OVER_VOLTAGE,     "set_over_voltage_binary_sensor"),
        (CONF_OVER_TEMPERATURE, "set_over_temperature_binary_sensor"),
    ]:
        if key in config:
            bs = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(hub, setter)(bs))
