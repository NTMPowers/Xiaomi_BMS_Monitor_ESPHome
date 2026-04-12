"""ESPHome Xiaomi BMS component configuration schema."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    ICON_BATTERY,
    ICON_FLASH,
    ICON_THERMOMETER,
    ICON_COUNTER,
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = True

CODEOWNERS = ["@esphome-xiaomi-bms"]

xiaomi_bms_ns = cg.esphome_ns.namespace("xiaomi_bms")
XiaomiBMSComponent = xiaomi_bms_ns.class_(
    "XiaomiBMSComponent", cg.PollingComponent, uart.UARTDevice
)

UNIT_MILLIAMP_HOURS = "mAh"
ICON_COUNTER_CLOCKWISE = "mdi:counter"
ICON_CELL = "mdi:battery-plus-variant"
ICON_DELTA = "mdi:delta"
ICON_BALANCE = "mdi:scale-balance"
ICON_HEALTH = "mdi:heart-pulse"
ICON_SERIAL = "mdi:identifier"

# Sensor keys
CONF_STATE_OF_CHARGE = "state_of_charge"
CONF_HEALTH = "health"
CONF_PACK_VOLTAGE = "pack_voltage"
CONF_PACK_CURRENT = "pack_current"
CONF_REMAINING_CAPACITY = "remaining_capacity"
CONF_DESIGN_CAPACITY = "design_capacity"
CONF_REAL_CAPACITY = "real_capacity"
CONF_NOMINAL_VOLTAGE = "nominal_voltage"
CONF_TEMP_SENSOR_1 = "temp_sensor_1"
CONF_TEMP_SENSOR_2 = "temp_sensor_2"
CONF_CELL_VOLTAGE_MIN = "cell_voltage_min"
CONF_CELL_VOLTAGE_MAX = "cell_voltage_max"
CONF_CELL_VOLTAGE_DELTA = "cell_voltage_delta"
CONF_CONNECTED_CELLS = "connected_cells"
CONF_CHARGE_CYCLES = "charge_cycles"
CONF_CHARGE_COUNT = "charge_count"
CONF_MAX_VOLTAGE = "max_voltage"
CONF_MAX_CHARGE_CURRENT = "max_charge_current"
CONF_MAX_DISCHARGE_CURRENT = "max_discharge_current"
CONF_BALANCE_BITS = "balance_bits"

CONF_CELL_VOLTAGES = "cell_voltages"  # list of up to 15

# Binary sensor keys
CONF_CHARGING_ENABLED = "charging_enabled"
CONF_OVER_VOLTAGE = "over_voltage"
CONF_OVER_TEMPERATURE = "over_temperature"

# Text sensor keys
CONF_SERIAL_NUMBER = "serial_number"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_PACK_DATE = "pack_date"
CONF_BALANCING_CELLS = "balancing_cells"

CELL_VOLTAGE_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_VOLT,
    icon=ICON_CELL,
    accuracy_decimals=3,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiBMSComponent),
            cv.Optional(CONF_UPDATE_INTERVAL, default="5s"): cv.update_interval,
            # Pack-level sensors
            cv.Optional(CONF_STATE_OF_CHARGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_BATTERY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_HEALTH): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_HEALTH,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PACK_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_FLASH,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PACK_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_FLASH,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_REMAINING_CAPACITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_MILLIAMP_HOURS,
                icon=ICON_BATTERY,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DESIGN_CAPACITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_MILLIAMP_HOURS,
                icon=ICON_BATTERY,
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_REAL_CAPACITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_MILLIAMP_HOURS,
                icon=ICON_BATTERY,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_NOMINAL_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_FLASH,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
            ),
            cv.Optional(CONF_MAX_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_FLASH,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
            ),
            cv.Optional(CONF_MAX_CHARGE_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_FLASH,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
            ),
            cv.Optional(CONF_MAX_DISCHARGE_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_FLASH,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
            ),
            cv.Optional(CONF_TEMP_SENSOR_1): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMP_SENSOR_2): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CELL_VOLTAGE_MIN): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_CELL,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CELL_VOLTAGE_MAX): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_CELL,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CELL_VOLTAGE_DELTA): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_DELTA,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CONNECTED_CELLS): sensor.sensor_schema(
                icon="mdi:battery-unknown",
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_CHARGE_CYCLES): sensor.sensor_schema(
                icon=ICON_COUNTER_CLOCKWISE,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CHARGE_COUNT): sensor.sensor_schema(
                icon=ICON_COUNTER_CLOCKWISE,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_BALANCE_BITS): sensor.sensor_schema(
                icon=ICON_BALANCE,
                accuracy_decimals=0,
            ),
            # Individual cell voltages (list of 1..15)
            cv.Optional(CONF_CELL_VOLTAGES): cv.ensure_list(CELL_VOLTAGE_SCHEMA),
            # Binary sensors
            cv.Optional(CONF_CHARGING_ENABLED): binary_sensor.binary_sensor_schema(
                device_class="battery_charging",
            ),
            cv.Optional(CONF_OVER_VOLTAGE): binary_sensor.binary_sensor_schema(
                device_class="problem",
            ),
            cv.Optional(CONF_OVER_TEMPERATURE): binary_sensor.binary_sensor_schema(
                device_class="problem",
            ),
            # Text sensors
            cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(
                icon=ICON_SERIAL,
            ),
            cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(
                icon="mdi:chip",
            ),
            cv.Optional(CONF_PACK_DATE): text_sensor.text_sensor_schema(
                icon="mdi:calendar",
            ),
            cv.Optional(CONF_BALANCING_CELLS): text_sensor.text_sensor_schema(
                icon=ICON_BALANCE,
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("5s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    # Numeric sensors
    for key, setter in [
        (CONF_STATE_OF_CHARGE, "set_state_of_charge_sensor"),
        (CONF_HEALTH, "set_health_sensor"),
        (CONF_PACK_VOLTAGE, "set_pack_voltage_sensor"),
        (CONF_PACK_CURRENT, "set_pack_current_sensor"),
        (CONF_REMAINING_CAPACITY, "set_remaining_capacity_sensor"),
        (CONF_DESIGN_CAPACITY, "set_design_capacity_sensor"),
        (CONF_REAL_CAPACITY, "set_real_capacity_sensor"),
        (CONF_NOMINAL_VOLTAGE, "set_nominal_voltage_sensor"),
        (CONF_MAX_VOLTAGE, "set_max_voltage_sensor"),
        (CONF_MAX_CHARGE_CURRENT, "set_max_charge_current_sensor"),
        (CONF_MAX_DISCHARGE_CURRENT, "set_max_discharge_current_sensor"),
        (CONF_TEMP_SENSOR_1, "set_temp_sensor_1"),
        (CONF_TEMP_SENSOR_2, "set_temp_sensor_2"),
        (CONF_CELL_VOLTAGE_MIN, "set_cell_voltage_min_sensor"),
        (CONF_CELL_VOLTAGE_MAX, "set_cell_voltage_max_sensor"),
        (CONF_CELL_VOLTAGE_DELTA, "set_cell_voltage_delta_sensor"),
        (CONF_CONNECTED_CELLS, "set_connected_cells_sensor"),
        (CONF_CHARGE_CYCLES, "set_charge_cycles_sensor"),
        (CONF_CHARGE_COUNT, "set_charge_count_sensor"),
        (CONF_BALANCE_BITS, "set_balance_bits_sensor"),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))

    # Individual cell voltage sensors
    if CONF_CELL_VOLTAGES in config:
        for idx, cell_cfg in enumerate(config[CONF_CELL_VOLTAGES]):
            sens = await sensor.new_sensor(cell_cfg)
            cg.add(var.set_cell_voltage_sensor(idx, sens))

    # Binary sensors
    for key, setter in [
        (CONF_CHARGING_ENABLED, "set_charging_enabled_binary_sensor"),
        (CONF_OVER_VOLTAGE, "set_over_voltage_binary_sensor"),
        (CONF_OVER_TEMPERATURE, "set_over_temperature_binary_sensor"),
    ]:
        if key in config:
            bs = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(var, setter)(bs))

    # Text sensors
    for key, setter in [
        (CONF_SERIAL_NUMBER, "set_serial_number_text_sensor"),
        (CONF_FIRMWARE_VERSION, "set_firmware_version_text_sensor"),
        (CONF_PACK_DATE, "set_pack_date_text_sensor"),
        (CONF_BALANCING_CELLS, "set_balancing_cells_text_sensor"),
    ]:
        if key in config:
            ts = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(var, setter)(ts))
