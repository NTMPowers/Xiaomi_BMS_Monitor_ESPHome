#include "xiaomi_bms.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace xiaomi_bms {

static const char *const TAG = "xiaomi_bms";

void XiaomiBMS::set_cell_voltage_sensor(size_t index, sensor::Sensor *sensor) {
  if (index >= this->cell_voltage_sensors_.size()) {
    return;
  }
  this->cell_voltage_sensors_[index] = sensor;
}

void XiaomiBMS::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi BMS:");
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(115200);
}

void XiaomiBMS::update() {
  if (!this->initialized_) {
    this->raw_.fill(0);
    this->initialized_ = true;
  }

  this->try_wake_bms_();
  if (!this->read_cycle_()) {
    ESP_LOGW(TAG, "No complete BMS response cycle received");
    return;
  }

  this->publish_decoded_();
}

void XiaomiBMS::try_wake_bms_() {
  static const uint8_t wake_frame[] = {0x55, 0xAA, 0x03, 0x22, 0x01, 0x30, 0x0C, 0x9D, 0xFF};
  for (uint8_t i = 0; i < 30; i++) {
    this->write_array(wake_frame, sizeof(wake_frame));
    this->flush();
    delay(40);
  }
}

bool XiaomiBMS::read_cycle_() {
  static const ChunkSpec chunks[] = {
      {0x00, 0x20},
      {0x20, 0x20},
      {0x40, 0x20},
      {0x60, 0x20},
      {0x80, 0x22},
  };
  for (const auto &chunk : chunks) {
    if (!this->read_chunk_(chunk.byte_offset, chunk.size)) {
      return false;
    }
  }
  return true;
}

bool XiaomiBMS::read_chunk_(uint8_t byte_offset, uint8_t size) {
  std::vector<uint8_t> packet;
  const uint8_t expected_offset = byte_offset / 2;
  auto request = build_request_(0x01, expected_offset, size);

  while (this->available()) {
    uint8_t drop;
    this->read_byte(&drop);
  }

  this->write_array(request.data(), request.size());
  this->flush();

  if (!this->read_response_for_offset_(expected_offset, packet, 350)) {
    ESP_LOGD(TAG, "Timeout waiting for offset 0x%02X", expected_offset);
    return false;
  }

  if (packet.size() < 8) {
    return false;
  }

  const auto payload_start = static_cast<size_t>(6);
  const auto payload_end = packet.size() - 2;
  const auto payload_len = payload_end - payload_start;
  const auto max_copy = std::min(payload_len, static_cast<size_t>(RAW_SIZE_ - byte_offset));
  std::copy(packet.begin() + payload_start, packet.begin() + payload_start + max_copy, this->raw_.begin() + byte_offset);

  return true;
}

uint16_t XiaomiBMS::calc_crc_(const std::vector<uint8_t> &data) {
  uint16_t crc = 0xFFFF;
  if (data.size() < 4) {
    return crc;
  }
  for (size_t i = 2; i + 2 < data.size(); i++) {
    crc = static_cast<uint16_t>((crc - data[i]) & 0xFFFF);
  }
  return crc;
}

std::vector<uint8_t> XiaomiBMS::build_request_(uint8_t mode, uint8_t offset, uint8_t size) {
  std::vector<uint8_t> frame = {0x55, 0xAA, 0x03, 0x22, mode, offset, size};
  frame.push_back(0x00);
  frame.push_back(0x00);
  const auto crc = calc_crc_(frame);
  frame[7] = static_cast<uint8_t>(crc & 0xFF);
  frame[8] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  return frame;
}

bool XiaomiBMS::check_crc_(const std::vector<uint8_t> &data) {
  if (data.size() < 9) {
    return false;
  }
  if (data[0] != 0x55 || data[1] != 0xAA) {
    return false;
  }
  if (static_cast<size_t>(data[2] + 6) != data.size()) {
    return false;
  }

  const auto crc = calc_crc_(data);
  return data[data.size() - 2] == static_cast<uint8_t>(crc & 0xFF) &&
         data[data.size() - 1] == static_cast<uint8_t>((crc >> 8) & 0xFF);
}

bool XiaomiBMS::read_packet_(std::vector<uint8_t> &out, uint32_t timeout_ms) {
  out.clear();
  uint8_t state = 0;
  const auto start = millis();

  while (millis() - start < timeout_ms) {
    if (!this->available()) {
      delay(1);
      continue;
    }

    uint8_t byte;
    if (!this->read_byte(&byte)) {
      continue;
    }

    if (state == 0) {
      if (byte == 0x55) {
        out.clear();
        out.push_back(byte);
        state = 1;
      }
      continue;
    }

    if (state == 1) {
      if (byte == 0xAA) {
        out.push_back(byte);
        state = 2;
      } else {
        state = 0;
        out.clear();
      }
      continue;
    }

    out.push_back(byte);
    if (out.size() > 8 && static_cast<size_t>(out[2] + 6) == out.size()) {
      return check_crc_(out);
    }
  }

  return false;
}

bool XiaomiBMS::read_response_for_offset_(uint8_t expected_offset, std::vector<uint8_t> &packet, uint32_t timeout_ms) {
  const auto start = millis();
  std::vector<uint8_t> candidate;

  while (millis() - start < timeout_ms) {
    if (!this->read_packet_(candidate, 80)) {
      continue;
    }

    if (candidate.size() >= 6 && candidate[4] == 0x01 && candidate[5] == expected_offset) {
      packet = candidate;
      return true;
    }

    ESP_LOGV(TAG, "Ignoring packet mode=0x%02X offset=0x%02X while waiting for 0x%02X", candidate[4], candidate[5], expected_offset);
  }

  return false;
}

uint16_t XiaomiBMS::unpack_u16_(size_t offset) const {
  return static_cast<uint16_t>(this->raw_[offset] | (this->raw_[offset + 1] << 8));
}

int16_t XiaomiBMS::unpack_i16_(size_t offset) const {
  return static_cast<int16_t>(this->raw_[offset] | (this->raw_[offset + 1] << 8));
}

std::string XiaomiBMS::decode_ascii_(size_t start, size_t end) const {
  std::string out;
  out.reserve(end - start);
  for (size_t i = start; i < end; i++) {
    const char ch = static_cast<char>(this->raw_[i]);
    if (ch == '\0') {
      break;
    }
    out.push_back(ch);
  }
  return out;
}

std::string XiaomiBMS::decode_date_(uint16_t raw) const {
  const uint16_t year = 2000 + ((raw >> 9) & 0x7F);
  const uint16_t month = (raw >> 5) & 0x0F;
  const uint16_t day = raw & 0x1F;

  char buffer[11];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u", year, month, day);
  return buffer;
}

std::string XiaomiBMS::decode_balance_bits_(uint16_t bits) const {
  std::string out;
  for (uint8_t i = 0; i < 15; i++) {
    if ((bits & (1u << i)) == 0) {
      continue;
    }
    if (!out.empty()) {
      out += ", ";
    }
    out += to_string(i + 1);
  }
  if (out.empty()) {
    out = "None";
  }
  return out;
}

std::string XiaomiBMS::decode_status_flags_(uint16_t status) const {
  std::string out;
  bool has_any = false;
  auto append = [&](const char *value) {
    if (has_any) {
      out += ", ";
    }
    out += value;
    has_any = true;
  };

  if (status & (1u << 0)) {
    append("Cfg");
  }
  if (status & (1u << 6)) {
    append("Charging");
  }
  if (status & (1u << 9)) {
    append("OV");
  }
  if (status & (1u << 10)) {
    append("OT");
  }

  uint8_t unknown_count = 0;
  for (uint8_t bit = 0; bit < 16; bit++) {
    if ((status & (1u << bit)) == 0) {
      continue;
    }
    if (bit == 0 || bit == 6 || bit == 9 || bit == 10) {
      continue;
    }
    unknown_count++;
  }
  if (unknown_count > 0) {
    if (has_any) {
      out += ", ";
    }
    out += "Unk:";
    out += to_string(unknown_count);
    has_any = true;
  }

  if (!has_any) {
    out = "None";
  }
  return out;
}

std::string XiaomiBMS::decode_error_bytes_() const {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X %02X %02X %02X %02X %02X", this->raw_[0x42], this->raw_[0x43], this->raw_[0x44],
           this->raw_[0x45], this->raw_[0x46], this->raw_[0x47]);
  return buffer;
}

void XiaomiBMS::publish_decoded_() {
  const uint16_t status = this->unpack_u16_(0x60);
  const uint16_t balance_bits = this->unpack_u16_(0x6C);

  const uint16_t design_capacity = this->unpack_u16_(0x30);
  const uint16_t real_capacity = this->unpack_u16_(0x32);
  const float nominal_voltage = this->unpack_u16_(0x34) / 1000.0f;
  const uint16_t charge_cycles = this->unpack_u16_(0x36);
  const uint16_t charge_count = this->unpack_u16_(0x38);
  const float max_voltage = this->unpack_u16_(0x3A) / 100.0f;
  const float max_discharge_current = this->unpack_u16_(0x3C) / 100.0f;
  const float max_charge_current = this->unpack_u16_(0x3E) / 100.0f;

  const uint16_t remaining_capacity = this->unpack_u16_(0x62);
  const uint16_t state_of_charge = this->unpack_u16_(0x64);
  const float pack_current = this->unpack_i16_(0x66) / 100.0f;
  const float pack_voltage = this->unpack_u16_(0x68) / 100.0f;
  const float temp1 = static_cast<int>(this->raw_[0x6A]) - 20;
  const float temp2 = static_cast<int>(this->raw_[0x6B]) - 20;
  const uint16_t health = this->unpack_u16_(0x76);

  std::array<uint16_t, 15> cells{};
  uint8_t connected_cells = 0;
  uint16_t min_cell = 0xFFFF;
  uint16_t max_cell = 0;

  for (size_t i = 0; i < cells.size(); i++) {
    cells[i] = this->unpack_u16_(0x80 + (i * 2));
    if (cells[i] == 0) {
      continue;
    }
    connected_cells++;
    min_cell = std::min(min_cell, cells[i]);
    max_cell = std::max(max_cell, cells[i]);
  }

  const float cell_min = connected_cells ? min_cell / 1000.0f : 0.0f;
  const float cell_max = connected_cells ? max_cell / 1000.0f : 0.0f;
  const float cell_delta = connected_cells ? (max_cell - min_cell) / 1000.0f : 0.0f;

  if (this->state_of_charge_sensor_ != nullptr) {
    this->state_of_charge_sensor_->publish_state(state_of_charge);
  }
  if (this->health_sensor_ != nullptr) {
    this->health_sensor_->publish_state(health);
  }
  if (this->pack_voltage_sensor_ != nullptr) {
    this->pack_voltage_sensor_->publish_state(pack_voltage);
  }
  if (this->pack_current_sensor_ != nullptr) {
    this->pack_current_sensor_->publish_state(pack_current);
  }
  if (this->remaining_capacity_sensor_ != nullptr) {
    this->remaining_capacity_sensor_->publish_state(remaining_capacity);
  }
  if (this->design_capacity_sensor_ != nullptr) {
    this->design_capacity_sensor_->publish_state(design_capacity);
  }
  if (this->real_capacity_sensor_ != nullptr) {
    this->real_capacity_sensor_->publish_state(real_capacity);
  }
  if (this->nominal_voltage_sensor_ != nullptr) {
    this->nominal_voltage_sensor_->publish_state(nominal_voltage);
  }
  if (this->max_voltage_sensor_ != nullptr) {
    this->max_voltage_sensor_->publish_state(max_voltage);
  }
  if (this->max_charge_current_sensor_ != nullptr) {
    this->max_charge_current_sensor_->publish_state(max_charge_current);
  }
  if (this->max_discharge_current_sensor_ != nullptr) {
    this->max_discharge_current_sensor_->publish_state(max_discharge_current);
  }
  if (this->temp_sensor_1_sensor_ != nullptr) {
    this->temp_sensor_1_sensor_->publish_state(temp1);
  }
  if (this->temp_sensor_2_sensor_ != nullptr) {
    this->temp_sensor_2_sensor_->publish_state(temp2);
  }
  if (this->cell_voltage_min_sensor_ != nullptr) {
    this->cell_voltage_min_sensor_->publish_state(cell_min);
  }
  if (this->cell_voltage_max_sensor_ != nullptr) {
    this->cell_voltage_max_sensor_->publish_state(cell_max);
  }
  if (this->cell_voltage_delta_sensor_ != nullptr) {
    this->cell_voltage_delta_sensor_->publish_state(cell_delta);
  }
  if (this->connected_cells_sensor_ != nullptr) {
    this->connected_cells_sensor_->publish_state(connected_cells);
  }
  if (this->charge_cycles_sensor_ != nullptr) {
    this->charge_cycles_sensor_->publish_state(charge_cycles);
  }
  if (this->charge_count_sensor_ != nullptr) {
    this->charge_count_sensor_->publish_state(charge_count);
  }
  if (this->balance_bits_sensor_ != nullptr) {
    this->balance_bits_sensor_->publish_state(balance_bits);
  }
  if (this->status_flags_sensor_ != nullptr) {
    this->status_flags_sensor_->publish_state(status);
  }

  for (size_t i = 0; i < this->cell_voltage_sensors_.size(); i++) {
    if (this->cell_voltage_sensors_[i] == nullptr) {
      continue;
    }
    this->cell_voltage_sensors_[i]->publish_state(cells[i] / 1000.0f);
  }

  if (this->serial_number_sensor_ != nullptr) {
    this->serial_number_sensor_->publish_state(this->decode_ascii_(0x20, 0x2E));
  }
  if (this->firmware_version_sensor_ != nullptr) {
    char fw[8];
    snprintf(fw, sizeof(fw), "0x%03X", this->unpack_u16_(0x2E));
    this->firmware_version_sensor_->publish_state(fw);
  }
  if (this->pack_date_sensor_ != nullptr) {
    this->pack_date_sensor_->publish_state(this->decode_date_(this->unpack_u16_(0x40)));
  }
  if (this->balancing_cells_sensor_ != nullptr) {
    this->balancing_cells_sensor_->publish_state(this->decode_balance_bits_(balance_bits));
  }
  if (this->status_flags_active_sensor_ != nullptr) {
    this->status_flags_active_sensor_->publish_state(this->decode_status_flags_(status));
  }
  if (this->error_bytes_sensor_ != nullptr) {
    this->error_bytes_sensor_->publish_state(this->decode_error_bytes_());
  }

  if (this->charging_enabled_sensor_ != nullptr) {
    this->charging_enabled_sensor_->publish_state((status & (1u << 6)) != 0);
  }
  if (this->over_voltage_sensor_ != nullptr) {
    this->over_voltage_sensor_->publish_state((status & (1u << 9)) != 0);
  }
  if (this->over_temperature_sensor_ != nullptr) {
    this->over_temperature_sensor_->publish_state((status & (1u << 10)) != 0);
  }
}

}  // namespace xiaomi_bms
}  // namespace esphome