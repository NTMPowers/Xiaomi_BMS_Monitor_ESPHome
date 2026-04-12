#include "xiaomi_bms.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace esphome {
namespace xiaomi_bms {

static const char *const TAG = "xiaomi_bms";

// ─────────────────────────────────────────────────────────────
//  Lifecycle
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Xiaomi BMS…");
  memset(raw_bms_, 0, sizeof(raw_bms_));
  send_wake_();
}

void XiaomiBMSComponent::update() {
  bool all_ok = true;

  for (uint8_t i = 0; i < NUM_CHUNKS; i++) {
    const ChunkSpec &cs = CHUNK_SPECS[i];
    if (!read_chunk_(cs.byte_offset, cs.size)) {
      ESP_LOGW(TAG, "Chunk '%s' (offset 0x%02X) failed – re-waking BMS", cs.label, cs.byte_offset);
      send_wake_();
      all_ok = false;
      break;
    }
  }

  if (all_ok) {
    parse_bms_data_();
  }
}

void XiaomiBMSComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi BMS:");
  LOG_SENSOR("  ", "State of Charge",       state_of_charge_sensor_);
  LOG_SENSOR("  ", "Health",                health_sensor_);
  LOG_SENSOR("  ", "Pack Voltage",          pack_voltage_sensor_);
  LOG_SENSOR("  ", "Pack Current",          pack_current_sensor_);
  LOG_SENSOR("  ", "Remaining Capacity",    remaining_capacity_sensor_);
  LOG_SENSOR("  ", "Design Capacity",       design_capacity_sensor_);
  LOG_SENSOR("  ", "Real Capacity",         real_capacity_sensor_);
  LOG_SENSOR("  ", "Nominal Voltage",       nominal_voltage_sensor_);
  LOG_SENSOR("  ", "Max Voltage",           max_voltage_sensor_);
  LOG_SENSOR("  ", "Max Charge Current",    max_charge_current_sensor_);
  LOG_SENSOR("  ", "Max Discharge Current", max_discharge_current_sensor_);
  LOG_SENSOR("  ", "Temp Sensor 1",         temp_sensor_1_);
  LOG_SENSOR("  ", "Temp Sensor 2",         temp_sensor_2_);
  LOG_SENSOR("  ", "Cell Voltage Min",      cell_voltage_min_sensor_);
  LOG_SENSOR("  ", "Cell Voltage Max",      cell_voltage_max_sensor_);
  LOG_SENSOR("  ", "Cell Voltage Delta",    cell_voltage_delta_sensor_);
  LOG_SENSOR("  ", "Connected Cells",       connected_cells_sensor_);
  LOG_SENSOR("  ", "Charge Cycles",         charge_cycles_sensor_);
  LOG_SENSOR("  ", "Charge Count",          charge_count_sensor_);
  LOG_SENSOR("  ", "Balance Bits",          balance_bits_sensor_);

  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    if (cell_voltage_sensors_[i] != nullptr) {
      ESP_LOGCONFIG(TAG, "  Cell %d Voltage: '%s'", i + 1,
                    cell_voltage_sensors_[i]->get_name().c_str());
    }
  }

  LOG_BINARY_SENSOR("  ", "Charging Enabled", charging_enabled_bs_);
  LOG_BINARY_SENSOR("  ", "Over Voltage",     over_voltage_bs_);
  LOG_BINARY_SENSOR("  ", "Over Temperature", over_temp_bs_);

  LOG_TEXT_SENSOR("  ", "Serial Number",    serial_number_ts_);
  LOG_TEXT_SENSOR("  ", "Firmware Version", firmware_version_ts_);
  LOG_TEXT_SENSOR("  ", "Pack Date",        pack_date_ts_);
  LOG_TEXT_SENSOR("  ", "Balancing Cells",  balancing_cells_ts_);
}

// ─────────────────────────────────────────────────────────────
//  Wake + chunk reading
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::send_wake_() {
  ESP_LOGD(TAG, "Sending wake frames");
  for (uint8_t i = 0; i < WAKE_REPS; i++) {
    write_array(WAKE_FRAME, sizeof(WAKE_FRAME));
    delay(40);
  }
}

// Build a read-request frame:
//   55 AA <len> 22 01 <offset_word> <size_byte> <crc_lo> <crc_hi>
void XiaomiBMSComponent::build_request_(uint8_t offset_word, uint8_t size, std::vector<uint8_t> &frame) {
  frame.clear();
  frame.push_back(0x55);
  frame.push_back(0xAA);
  frame.push_back(0x03);  // length = 2 (fixed bytes) + 1 (size_byte) = 3
  frame.push_back(BMS_ADDR);
  frame.push_back(MODE_READ);
  frame.push_back(offset_word);
  frame.push_back(size);
  frame.push_back(0x00);  // CRC placeholder lo
  frame.push_back(0x00);  // CRC placeholder hi

  uint16_t crc = calc_crc_(frame);
  frame[frame.size() - 2] = crc & 0xFF;
  frame[frame.size() - 1] = (crc >> 8) & 0xFF;
}

uint16_t XiaomiBMSComponent::calc_crc_(const std::vector<uint8_t> &data) {
  // CRC = 0xFFFF − sum(data[2 .. len-3])  (all bytes between header and CRC field)
  uint16_t crc = 0xFFFF;
  for (size_t i = 2; i < data.size() - 2; i++) {
    crc = (crc - data[i]) & 0xFFFF;
  }
  return crc;
}

bool XiaomiBMSComponent::check_crc_(const std::vector<uint8_t> &data) {
  if (data.size() < 9)         return false;
  if (data[0] != 0x55)         return false;
  if (data[1] != 0xAA)         return false;
  if ((size_t)(data[2] + 6) != data.size()) return false;

  uint16_t expected = calc_crc_(data);
  return (data[data.size() - 2] == (expected & 0xFF)) &&
         (data[data.size() - 1] == ((expected >> 8) & 0xFF));
}

// Read one complete, CRC-valid packet within timeout_ms milliseconds.
bool XiaomiBMSComponent::read_packet_(uint32_t timeout_ms, std::vector<uint8_t> &out) {
  out.clear();
  uint8_t state = 0;
  uint32_t deadline = millis() + timeout_ms;

  while (millis() < deadline) {
    if (!available()) {
      yield();
      continue;
    }

    uint8_t byte = read();

    switch (state) {
      case 0:
        if (byte == 0x55) { out = {byte}; state = 1; }
        break;
      case 1:
        if (byte == 0xAA) { out.push_back(byte); state = 2; }
        else              { out.clear(); state = 0; }
        break;
      default:
        out.push_back(byte);
        // Complete when we have received exactly (length_field + 6) bytes
        if (out.size() > 2 && out.size() == (size_t)(out[2] + 6)) {
          if (check_crc_(out)) return true;
          // Bad CRC – restart scan
          out.clear();
          state = 0;
        }
        break;
    }
  }
  return false;
}

bool XiaomiBMSComponent::read_chunk_(uint8_t byte_offset, uint8_t size) {
  uint8_t offset_word = byte_offset / 2;

  std::vector<uint8_t> request;
  build_request_(offset_word, size, request);

  flush();
  write_array(request.data(), request.size());

  uint32_t deadline = millis() + 350;  // 350 ms total window (mirrors Python REQUEST_TIMEOUT)

  while (millis() < deadline) {
    std::vector<uint8_t> pkt;
    uint32_t remaining = deadline - millis();
    if (remaining == 0) break;

    if (!read_packet_(std::min(remaining, (uint32_t)80), pkt)) continue;

    // Accept only response packets (mode == 0x01) at the expected offset
    if (pkt.size() >= 6 && pkt[4] == MODE_READ && pkt[5] == offset_word) {
      // Payload sits between header bytes and the 2 CRC bytes
      const uint8_t *payload = pkt.data() + 6;
      size_t payload_len     = pkt.size() - 8;  // strip 6 header + 2 CRC

      size_t end = std::min((size_t)(byte_offset + payload_len), (size_t)BMS_TOTAL_SIZE);
      memcpy(raw_bms_ + byte_offset, payload, end - byte_offset);
      return true;
    }
  }
  return false;
}

// ─────────────────────────────────────────────────────────────
//  Decode helpers
// ─────────────────────────────────────────────────────────────

std::string XiaomiBMSComponent::decode_date_(uint16_t raw) {
  int year  = 2000 + ((raw >> 9) & 0x7F);
  int month = (raw >> 5) & 0x0F;
  int day   = raw & 0x1F;
  char buf[12];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
  return std::string(buf);
}

std::string XiaomiBMSComponent::decode_ascii_(const uint8_t *p, uint8_t len) {
  std::string s;
  for (uint8_t i = 0; i < len; i++) {
    if (p[i] == 0) break;
    s += (char)p[i];
  }
  return s;
}

std::string XiaomiBMSComponent::decode_balance_bits_(uint16_t bits) {
  std::string result;
  for (uint8_t i = 0; i < 15; i++) {
    if (bits & (1 << i)) {
      if (!result.empty()) result += ",";
      result += std::to_string(i + 1);
    }
  }
  return result.empty() ? "None" : result;
}

// ─────────────────────────────────────────────────────────────
//  Main decoder
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::parse_bms_data_() {
  const uint8_t *d = raw_bms_;

  // ── Text sensors ──────────────────────────────────────────
  if (serial_number_ts_) {
    serial_number_ts_->publish_state(decode_ascii_(d + OFF_SERIAL, 14));
  }
  if (firmware_version_ts_) {
    char buf[8];
    snprintf(buf, sizeof(buf), "0x%03X", u16_le_(d + OFF_FW_VER));
    firmware_version_ts_->publish_state(buf);
  }
  if (pack_date_ts_) {
    pack_date_ts_->publish_state(decode_date_(u16_le_(d + OFF_PACK_DATE)));
  }

  // ── Capacity / static ─────────────────────────────────────
  if (design_capacity_sensor_)
    design_capacity_sensor_->publish_state(u16_le_(d + OFF_DESIGN_CAP));
  if (real_capacity_sensor_)
    real_capacity_sensor_->publish_state(u16_le_(d + OFF_REAL_CAP));
  if (nominal_voltage_sensor_)
    nominal_voltage_sensor_->publish_state(u16_le_(d + OFF_NOM_VOLT) / 1000.0f);
  if (charge_cycles_sensor_)
    charge_cycles_sensor_->publish_state(u16_le_(d + OFF_CHARGE_CYC));
  if (charge_count_sensor_)
    charge_count_sensor_->publish_state(u16_le_(d + OFF_CHARGE_CNT));
  if (max_voltage_sensor_)
    max_voltage_sensor_->publish_state(u16_le_(d + OFF_MAX_VOLT) / 100.0f);
  if (max_discharge_current_sensor_)
    max_discharge_current_sensor_->publish_state(u16_le_(d + OFF_MAX_DIS_CUR) / 100.0f);
  if (max_charge_current_sensor_)
    max_charge_current_sensor_->publish_state(u16_le_(d + OFF_MAX_CHG_CUR) / 100.0f);

  // ── Live data ─────────────────────────────────────────────
  uint16_t status = u16_le_(d + OFF_STATUS);

  if (charging_enabled_bs_)
    charging_enabled_bs_->publish_state(status & STATUS_BIT_CHARGING);
  if (over_voltage_bs_)
    over_voltage_bs_->publish_state(status & STATUS_BIT_OV);
  if (over_temp_bs_)
    over_temp_bs_->publish_state(status & STATUS_BIT_OT);

  if (remaining_capacity_sensor_)
    remaining_capacity_sensor_->publish_state(u16_le_(d + OFF_REMAIN_CAP));
  if (state_of_charge_sensor_)
    state_of_charge_sensor_->publish_state(u16_le_(d + OFF_SOC));
  if (pack_current_sensor_)
    pack_current_sensor_->publish_state(i16_le_(d + OFF_PACK_CUR) / 100.0f);
  if (pack_voltage_sensor_)
    pack_voltage_sensor_->publish_state(u16_le_(d + OFF_PACK_VOLT) / 100.0f);

  // Temperatures: raw byte − 20 = °C
  if (temp_sensor_1_)
    temp_sensor_1_->publish_state((int8_t)(d[OFF_TEMPS + 0]) - 20);
  if (temp_sensor_2_)
    temp_sensor_2_->publish_state((int8_t)(d[OFF_TEMPS + 1]) - 20);

  // Balance
  uint16_t bal = u16_le_(d + OFF_BAL_BITS);
  if (balance_bits_sensor_)
    balance_bits_sensor_->publish_state(bal);
  if (balancing_cells_ts_)
    balancing_cells_ts_->publish_state(decode_balance_bits_(bal));

  // Health
  if (health_sensor_)
    health_sensor_->publish_state(u16_le_(d + OFF_HEALTH));

  // ── Cell voltages ─────────────────────────────────────────
  uint16_t cell_mv[NUM_CELLS];
  uint16_t min_mv = 0xFFFF, max_mv = 0;
  uint8_t connected = 0;

  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    cell_mv[i] = u16_le_(d + OFF_CELLS + i * 2);
    if (cell_mv[i] > 0) {
      connected++;
      if (cell_mv[i] < min_mv) min_mv = cell_mv[i];
      if (cell_mv[i] > max_mv) max_mv = cell_mv[i];
    }
    if (cell_voltage_sensors_[i] != nullptr) {
      cell_voltage_sensors_[i]->publish_state(cell_mv[i] / 1000.0f);
    }
  }

  if (connected_cells_sensor_)
    connected_cells_sensor_->publish_state(connected);

  if (connected > 0) {
    float min_v = min_mv / 1000.0f;
    float max_v = max_mv / 1000.0f;
    if (cell_voltage_min_sensor_)   cell_voltage_min_sensor_->publish_state(min_v);
    if (cell_voltage_max_sensor_)   cell_voltage_max_sensor_->publish_state(max_v);
    if (cell_voltage_delta_sensor_) cell_voltage_delta_sensor_->publish_state(max_v - min_v);
  }

  ESP_LOGD(TAG, "BMS update OK – SoC=%u%% V=%.2fV I=%.2fA T1=%d°C T2=%d°C",
           u16_le_(d + OFF_SOC),
           u16_le_(d + OFF_PACK_VOLT) / 100.0f,
           i16_le_(d + OFF_PACK_CUR)  / 100.0f,
           (int)(d[OFF_TEMPS + 0]) - 20,
           (int)(d[OFF_TEMPS + 1]) - 20);
}

}  // namespace xiaomi_bms
}  // namespace esphome
