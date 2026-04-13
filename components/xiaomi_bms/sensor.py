import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
)

from . import XiaomiBMS

DEPENDENCIES = ["xiaomi_bms"]

CONF_BMS_ID = "bms_id"
CONF_STATE_OF_CHARGE = "state_of_charge"
CONF_HEALTH = "health"
CONF_PACK_VOLTAGE = "pack_voltage"
CONF_PACK_CURRENT = "pack_current"
CONF_REMAINING_CAPACITY = "remaining_capacity"
CONF_DESIGN_CAPACITY = "design_capacity"
CONF_REAL_CAPACITY = "real_capacity"
CONF_NOMINAL_VOLTAGE = "nominal_voltage"
CONF_MAX_VOLTAGE = "max_voltage"
CONF_MAX_CHARGE_CURRENT = "max_charge_current"
CONF_MAX_DISCHARGE_CURRENT = "max_discharge_current"
CONF_TEMP_SENSOR_1 = "temp_sensor_1"
CONF_TEMP_SENSOR_2 = "temp_sensor_2"
CONF_CELL_VOLTAGE_MIN = "cell_voltage_min"
CONF_CELL_VOLTAGE_MAX = "cell_voltage_max"
CONF_CELL_VOLTAGE_DELTA = "cell_voltage_delta"
CONF_CONNECTED_CELLS = "connected_cells"
CONF_CHARGE_CYCLES = "charge_cycles"
CONF_CHARGE_COUNT = "charge_count"
CONF_BALANCE_BITS = "balance_bits"
CONF_STATUS_FLAGS = "status_flags"
CONF_CELL_VOLTAGES = "cell_voltages"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BMS_ID): cv.use_id(XiaomiBMS),
        cv.Optional(CONF_STATE_OF_CHARGE): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_HEALTH): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_PACK_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement="V",
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_PACK_CURRENT): sensor.sensor_schema(
            unit_of_measurement="A",
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_REMAINING_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="mAh",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DESIGN_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="mAh",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_REAL_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="mAh",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_NOMINAL_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement="V",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_MAX_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement="V",
            accuracy_decimals=2,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_MAX_CHARGE_CURRENT): sensor.sensor_schema(
            unit_of_measurement="A",
            accuracy_decimals=2,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_MAX_DISCHARGE_CURRENT): sensor.sensor_schema(
            unit_of_measurement="A",
            accuracy_decimals=2,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_TEMP_SENSOR_1): sensor.sensor_schema(
            unit_of_measurement="C",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMP_SENSOR_2): sensor.sensor_schema(
            unit_of_measurement="C",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_MIN): sensor.sensor_schema(
            unit_of_measurement="V",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_MAX): sensor.sensor_schema(
            unit_of_measurement="V",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_DELTA): sensor.sensor_schema(
            unit_of_measurement="V",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CONNECTED_CELLS): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CHARGE_CYCLES): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_CHARGE_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_BALANCE_BITS): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_STATUS_FLAGS): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_CELL_VOLTAGES): cv.All(
            cv.ensure_list(sensor.sensor_schema(unit_of_measurement="V", accuracy_decimals=3, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT)),
            cv.Length(min=1, max=15),
        ),
        cv.Optional(CONF_UPDATE_INTERVAL): cv.invalid(
            "Set update_interval under the xiaomi_bms hub component, not in the sensor platform."
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_BMS_ID])

    if CONF_STATE_OF_CHARGE in config:
        sens = await sensor.new_sensor(config[CONF_STATE_OF_CHARGE])
        cg.add(parent.set_state_of_charge_sensor(sens))
    if CONF_HEALTH in config:
        sens = await sensor.new_sensor(config[CONF_HEALTH])
        cg.add(parent.set_health_sensor(sens))
    if CONF_PACK_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_PACK_VOLTAGE])
        cg.add(parent.set_pack_voltage_sensor(sens))
    if CONF_PACK_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_PACK_CURRENT])
        cg.add(parent.set_pack_current_sensor(sens))
    if CONF_REMAINING_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_REMAINING_CAPACITY])
        cg.add(parent.set_remaining_capacity_sensor(sens))
    if CONF_DESIGN_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_DESIGN_CAPACITY])
        cg.add(parent.set_design_capacity_sensor(sens))
    if CONF_REAL_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_REAL_CAPACITY])
        cg.add(parent.set_real_capacity_sensor(sens))
    if CONF_NOMINAL_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_NOMINAL_VOLTAGE])
        cg.add(parent.set_nominal_voltage_sensor(sens))
    if CONF_MAX_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_MAX_VOLTAGE])
        cg.add(parent.set_max_voltage_sensor(sens))
    if CONF_MAX_CHARGE_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_MAX_CHARGE_CURRENT])
        cg.add(parent.set_max_charge_current_sensor(sens))
    if CONF_MAX_DISCHARGE_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_MAX_DISCHARGE_CURRENT])
        cg.add(parent.set_max_discharge_current_sensor(sens))
    if CONF_TEMP_SENSOR_1 in config:
        sens = await sensor.new_sensor(config[CONF_TEMP_SENSOR_1])
        cg.add(parent.set_temp_sensor_1_sensor(sens))
    if CONF_TEMP_SENSOR_2 in config:
        sens = await sensor.new_sensor(config[CONF_TEMP_SENSOR_2])
        cg.add(parent.set_temp_sensor_2_sensor(sens))
    if CONF_CELL_VOLTAGE_MIN in config:
        sens = await sensor.new_sensor(config[CONF_CELL_VOLTAGE_MIN])
        cg.add(parent.set_cell_voltage_min_sensor(sens))
    if CONF_CELL_VOLTAGE_MAX in config:
        sens = await sensor.new_sensor(config[CONF_CELL_VOLTAGE_MAX])
        cg.add(parent.set_cell_voltage_max_sensor(sens))
    if CONF_CELL_VOLTAGE_DELTA in config:
        sens = await sensor.new_sensor(config[CONF_CELL_VOLTAGE_DELTA])
        cg.add(parent.set_cell_voltage_delta_sensor(sens))
    if CONF_CONNECTED_CELLS in config:
        sens = await sensor.new_sensor(config[CONF_CONNECTED_CELLS])
        cg.add(parent.set_connected_cells_sensor(sens))
    if CONF_CHARGE_CYCLES in config:
        sens = await sensor.new_sensor(config[CONF_CHARGE_CYCLES])
        cg.add(parent.set_charge_cycles_sensor(sens))
    if CONF_CHARGE_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_CHARGE_COUNT])
        cg.add(parent.set_charge_count_sensor(sens))
    if CONF_BALANCE_BITS in config:
        sens = await sensor.new_sensor(config[CONF_BALANCE_BITS])
        cg.add(parent.set_balance_bits_sensor(sens))
    if CONF_STATUS_FLAGS in config:
        sens = await sensor.new_sensor(config[CONF_STATUS_FLAGS])
        cg.add(parent.set_status_flags_sensor(sens))

    if CONF_CELL_VOLTAGES in config:
        for idx, cell_cfg in enumerate(config[CONF_CELL_VOLTAGES]):
            sens = await sensor.new_sensor(cell_cfg)
            cg.add(parent.set_cell_voltage_sensor(idx, sens))
