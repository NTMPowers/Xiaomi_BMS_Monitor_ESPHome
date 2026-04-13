#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace xiaomi_bms {

class XiaomiBMS : public PollingComponent, public uart::UARTDevice {
 public:
  void update() override;
  void dump_config() override;

  void set_state_of_charge_sensor(sensor::Sensor *sensor) { this->state_of_charge_sensor_ = sensor; }
  void set_health_sensor(sensor::Sensor *sensor) { this->health_sensor_ = sensor; }
  void set_pack_voltage_sensor(sensor::Sensor *sensor) { this->pack_voltage_sensor_ = sensor; }
  void set_pack_current_sensor(sensor::Sensor *sensor) { this->pack_current_sensor_ = sensor; }
  void set_remaining_capacity_sensor(sensor::Sensor *sensor) { this->remaining_capacity_sensor_ = sensor; }
  void set_design_capacity_sensor(sensor::Sensor *sensor) { this->design_capacity_sensor_ = sensor; }
  void set_real_capacity_sensor(sensor::Sensor *sensor) { this->real_capacity_sensor_ = sensor; }
  void set_nominal_voltage_sensor(sensor::Sensor *sensor) { this->nominal_voltage_sensor_ = sensor; }
  void set_max_voltage_sensor(sensor::Sensor *sensor) { this->max_voltage_sensor_ = sensor; }
  void set_max_charge_current_sensor(sensor::Sensor *sensor) { this->max_charge_current_sensor_ = sensor; }
  void set_max_discharge_current_sensor(sensor::Sensor *sensor) { this->max_discharge_current_sensor_ = sensor; }
  void set_temp_sensor_1_sensor(sensor::Sensor *sensor) { this->temp_sensor_1_sensor_ = sensor; }
  void set_temp_sensor_2_sensor(sensor::Sensor *sensor) { this->temp_sensor_2_sensor_ = sensor; }
  void set_cell_voltage_min_sensor(sensor::Sensor *sensor) { this->cell_voltage_min_sensor_ = sensor; }
  void set_cell_voltage_max_sensor(sensor::Sensor *sensor) { this->cell_voltage_max_sensor_ = sensor; }
  void set_cell_voltage_delta_sensor(sensor::Sensor *sensor) { this->cell_voltage_delta_sensor_ = sensor; }
  void set_connected_cells_sensor(sensor::Sensor *sensor) { this->connected_cells_sensor_ = sensor; }
  void set_charge_cycles_sensor(sensor::Sensor *sensor) { this->charge_cycles_sensor_ = sensor; }
  void set_charge_count_sensor(sensor::Sensor *sensor) { this->charge_count_sensor_ = sensor; }
  void set_balance_bits_sensor(sensor::Sensor *sensor) { this->balance_bits_sensor_ = sensor; }
  void set_status_flags_sensor(sensor::Sensor *sensor) { this->status_flags_sensor_ = sensor; }
  void set_cell_voltage_sensor(size_t index, sensor::Sensor *sensor);

  void set_serial_number_sensor(text_sensor::TextSensor *sensor) { this->serial_number_sensor_ = sensor; }
  void set_firmware_version_sensor(text_sensor::TextSensor *sensor) { this->firmware_version_sensor_ = sensor; }
  void set_pack_date_sensor(text_sensor::TextSensor *sensor) { this->pack_date_sensor_ = sensor; }
  void set_balancing_cells_sensor(text_sensor::TextSensor *sensor) { this->balancing_cells_sensor_ = sensor; }
  void set_status_flags_active_sensor(text_sensor::TextSensor *sensor) { this->status_flags_active_sensor_ = sensor; }
  void set_error_bytes_sensor(text_sensor::TextSensor *sensor) { this->error_bytes_sensor_ = sensor; }

  void set_charging_enabled_sensor(binary_sensor::BinarySensor *sensor) { this->charging_enabled_sensor_ = sensor; }
  void set_over_voltage_sensor(binary_sensor::BinarySensor *sensor) { this->over_voltage_sensor_ = sensor; }
  void set_over_temperature_sensor(binary_sensor::BinarySensor *sensor) { this->over_temperature_sensor_ = sensor; }

 protected:
  static constexpr size_t RAW_SIZE_ = 0xA2;

  struct ChunkSpec {
    uint8_t byte_offset;
    uint8_t size;
  };

  std::array<uint8_t, RAW_SIZE_> raw_{};
  bool initialized_{false};

  sensor::Sensor *state_of_charge_sensor_{nullptr};
  sensor::Sensor *health_sensor_{nullptr};
  sensor::Sensor *pack_voltage_sensor_{nullptr};
  sensor::Sensor *pack_current_sensor_{nullptr};
  sensor::Sensor *remaining_capacity_sensor_{nullptr};
  sensor::Sensor *design_capacity_sensor_{nullptr};
  sensor::Sensor *real_capacity_sensor_{nullptr};
  sensor::Sensor *nominal_voltage_sensor_{nullptr};
  sensor::Sensor *max_voltage_sensor_{nullptr};
  sensor::Sensor *max_charge_current_sensor_{nullptr};
  sensor::Sensor *max_discharge_current_sensor_{nullptr};
  sensor::Sensor *temp_sensor_1_sensor_{nullptr};
  sensor::Sensor *temp_sensor_2_sensor_{nullptr};
  sensor::Sensor *cell_voltage_min_sensor_{nullptr};
  sensor::Sensor *cell_voltage_max_sensor_{nullptr};
  sensor::Sensor *cell_voltage_delta_sensor_{nullptr};
  sensor::Sensor *connected_cells_sensor_{nullptr};
  sensor::Sensor *charge_cycles_sensor_{nullptr};
  sensor::Sensor *charge_count_sensor_{nullptr};
  sensor::Sensor *balance_bits_sensor_{nullptr};
  sensor::Sensor *status_flags_sensor_{nullptr};
  std::array<sensor::Sensor *, 15> cell_voltage_sensors_{};

  text_sensor::TextSensor *serial_number_sensor_{nullptr};
  text_sensor::TextSensor *firmware_version_sensor_{nullptr};
  text_sensor::TextSensor *pack_date_sensor_{nullptr};
  text_sensor::TextSensor *balancing_cells_sensor_{nullptr};
  text_sensor::TextSensor *status_flags_active_sensor_{nullptr};
  text_sensor::TextSensor *error_bytes_sensor_{nullptr};

  binary_sensor::BinarySensor *charging_enabled_sensor_{nullptr};
  binary_sensor::BinarySensor *over_voltage_sensor_{nullptr};
  binary_sensor::BinarySensor *over_temperature_sensor_{nullptr};

  void try_wake_bms_();
  bool read_cycle_();
  bool read_chunk_(uint8_t byte_offset, uint8_t size);

  static uint16_t calc_crc_(const std::vector<uint8_t> &data);
  static std::vector<uint8_t> build_request_(uint8_t mode, uint8_t offset, uint8_t size);
  static bool check_crc_(const std::vector<uint8_t> &data);
  bool read_packet_(std::vector<uint8_t> &out, uint32_t timeout_ms);
  bool read_response_for_offset_(uint8_t expected_offset, std::vector<uint8_t> &packet, uint32_t timeout_ms);

  uint16_t unpack_u16_(size_t offset) const;
  int16_t unpack_i16_(size_t offset) const;
  std::string decode_ascii_(size_t start, size_t end) const;
  std::string decode_date_(uint16_t raw) const;
  std::string decode_balance_bits_(uint16_t bits) const;
  std::string decode_status_flags_(uint16_t status) const;
  std::string decode_error_bytes_() const;
  void publish_decoded_();
};

}  // namespace xiaomi_bms
}  // namespace esphome