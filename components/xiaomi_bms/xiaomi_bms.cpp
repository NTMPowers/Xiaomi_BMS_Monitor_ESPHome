#include "xiaomi_bms.h"
#include "esphome/core/log.h"

#include <cstring>
#include <algorithm>

namespace esphome {
namespace xiaomi_bms {

static const char *const TAG = "xiaomi_bms";

// Number of wake frames to send at the start of each poll cycle.
// Keep this LOW - the BMS responds to each wake frame (20 bytes/response)
// and the ESP8266 HW UART buffer is only 128 bytes. 3 frames = 60 bytes,
// leaving room for the buffer to be drained before overflow.
static const uint8_t  WAKE_COUNT        = 3;
static const uint32_t WAKE_INTERVAL_MS  = 40;
// Settle time after last wake frame - enough for responses to arrive
// so we can drain them, but not so long the BMS sleeps again
static const uint32_t WAKE_SETTLE_MS    = 200;
// How long to wait for a response to each chunk request
static const uint32_t CHUNK_TIMEOUT_MS  = 500;

// ─────────────────────────────────────────────────────────────
//  Lifecycle
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Xiaomi BMS");
  memset(raw_bms_, 0, sizeof(raw_bms_));
  trigger_poll_();
}

void XiaomiBMSComponent::loop() {
  uint32_t now = millis();

  switch (state_) {

    // ── IDLE: wait for next poll interval ───────────────────
    case State::IDLE:
      if (now - last_poll_ms_ >= update_interval_ms_) {
        trigger_poll_();
      }
      break;

    // ── WAKING: drip-feed wake frames without blocking ──────
    case State::WAKING:
      // Continuously drain RX during wake to prevent buffer overflow.
      // The BMS responds to each wake frame with a 20-byte packet.
      while (available()) read();

      if (wake_reps_sent_ < WAKE_COUNT) {
        if (now >= wake_next_ms_) {
          write_array(WAKE_FRAME, sizeof(WAKE_FRAME));
          wake_reps_sent_++;
          wake_next_ms_ = now + WAKE_INTERVAL_MS;
        }
      } else {
        // All wake frames sent – wait settle time, keep draining, then start chunks
        if (now >= state_deadline_) {
          // Final drain pass
          while (available()) read();
          chunk_idx_ = 0;
          send_chunk_request_(chunk_idx_);
          state_ = State::READING_RESPONSE;
        }
      }
      break;

    // ── READING_RESPONSE: assemble packet byte by byte ──────
    case State::READING_RESPONSE:
      // Drain all available bytes each loop() call
      while (available()) {
        uint8_t b = read();
        ESP_LOGD(TAG, "RX byte: 0x%02X (rx_state=%d buf_size=%d)", b, rx_state_, (int)rx_buf_.size());

        switch (rx_state_) {
          case 0:
            if (b == 0x55) { rx_buf_.clear(); rx_buf_.push_back(b); rx_state_ = 1; }
            break;
          case 1:
            if (b == 0xAA) { rx_buf_.push_back(b); rx_state_ = 2; }
            else           { rx_buf_.clear(); rx_state_ = 0; }
            break;
          default:
            rx_buf_.push_back(b);
            if (rx_buf_.size() > 2 && rx_buf_.size() == (size_t)(rx_buf_[2] + 6)) {
              // Full packet received
              if (check_crc_(rx_buf_)) {
                process_received_packet_();
              } else {
                std::string bad;
                for (auto x : rx_buf_) { char t[4]; snprintf(t, sizeof(t), "%02X ", x); bad += t; }
                ESP_LOGW(TAG, "Bad CRC: %s", bad.c_str());
              }
              rx_buf_.clear();
              rx_state_ = 0;
            }
            break;
        }
      }

      // Check timeout
      if (millis() >= chunk_deadline_) {
        if (!rx_buf_.empty()) {
          std::string partial;
          for (auto x : rx_buf_) { char t[4]; snprintf(t, sizeof(t), "%02X ", x); partial += t; }
          ESP_LOGW(TAG, "Chunk '%s' timeout, partial (%d bytes): %s",
                   CHUNK_SPECS[chunk_idx_].label, (int)rx_buf_.size(), partial.c_str());
        } else {
          ESP_LOGW(TAG, "Chunk '%s' timeout, no bytes received",
                   CHUNK_SPECS[chunk_idx_].label);
        }
        // Retry from beginning with fresh wake
        trigger_poll_();
      }
      break;

    case State::PARSE:
      parse_bms_data_();
      state_ = State::IDLE;
      break;

    default:
      break;
  }
}

// ─────────────────────────────────────────────────────────────
//  State transitions
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::trigger_poll_() {
  ESP_LOGD(TAG, "Starting poll cycle – sending wake frames");
  last_poll_ms_    = millis();
  wake_reps_sent_  = 0;
  wake_next_ms_    = millis();
  // deadline = time after all wake frames + settle
  state_deadline_  = millis() + (WAKE_COUNT * WAKE_INTERVAL_MS) + WAKE_SETTLE_MS;
  state_           = State::WAKING;
}

void XiaomiBMSComponent::send_chunk_request_(uint8_t chunk_idx) {
  const ChunkSpec &cs = CHUNK_SPECS[chunk_idx];
  uint8_t offset_word = cs.byte_offset / 2;
  expected_offset_    = offset_word;

  std::vector<uint8_t> frame;
  build_request_(offset_word, cs.size, frame);

  std::string hex;
  for (auto b : frame) { char buf[4]; snprintf(buf, sizeof(buf), "%02X ", b); hex += buf; }
  ESP_LOGD(TAG, "TX chunk '%s': %s", cs.label, hex.c_str());

  // Flush any stale RX bytes before sending
  while (available()) read();
  rx_buf_.clear();
  rx_state_ = 0;

  write_array(frame.data(), frame.size());
  chunk_deadline_ = millis() + CHUNK_TIMEOUT_MS;
}

void XiaomiBMSComponent::process_received_packet_() {
  if (rx_buf_.size() < 6) return;

  // Log received packet
  std::string hex;
  for (auto b : rx_buf_) { char buf[4]; snprintf(buf, sizeof(buf), "%02X ", b); hex += buf; }
  ESP_LOGD(TAG, "RX chunk '%s': %s", CHUNK_SPECS[chunk_idx_].label, hex.c_str());

  uint8_t mode   = rx_buf_[4];
  uint8_t offset = rx_buf_[5];

  if (mode != MODE_READ || offset != expected_offset_) {
    ESP_LOGD(TAG, "Ignoring packet: mode=0x%02X offset=0x%02X (want mode=0x%02X offset=0x%02X)",
             mode, offset, MODE_READ, expected_offset_);
    return;  // keep waiting
  }

  // Copy payload into raw_bms_ buffer
  const uint8_t *payload = rx_buf_.data() + 6;
  size_t payload_len     = rx_buf_.size() - 8;
  uint8_t byte_offset    = CHUNK_SPECS[chunk_idx_].byte_offset;
  size_t end = std::min((size_t)(byte_offset + payload_len), (size_t)BMS_TOTAL_SIZE);
  memcpy(raw_bms_ + byte_offset, payload, end - byte_offset);

  ESP_LOGD(TAG, "Chunk '%s' OK (%d payload bytes)", CHUNK_SPECS[chunk_idx_].label, (int)payload_len);

  // Advance to next chunk
  chunk_idx_++;
  if (chunk_idx_ < NUM_CHUNKS) {
    send_chunk_request_(chunk_idx_);
    // Stay in READING_RESPONSE state
  } else {
    // All chunks received
    state_ = State::PARSE;
  }
}

// ─────────────────────────────────────────────────────────────
//  Protocol helpers
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::build_request_(uint8_t offset_word, uint8_t size, std::vector<uint8_t> &frame) {
  frame.clear();
  frame.push_back(0x55);
  frame.push_back(0xAA);
  frame.push_back(0x03);   // LEN = 2 + 1 data byte
  frame.push_back(BMS_ADDR);
  frame.push_back(MODE_READ);
  frame.push_back(offset_word);
  frame.push_back(size);
  frame.push_back(0x00);   // CRC lo placeholder
  frame.push_back(0x00);   // CRC hi placeholder
  uint16_t crc = calc_crc_(frame);
  frame[7] = crc & 0xFF;
  frame[8] = (crc >> 8) & 0xFF;
}

uint16_t XiaomiBMSComponent::calc_crc_(const std::vector<uint8_t> &data) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 2; i < data.size() - 2; i++)
    crc = (crc - data[i]) & 0xFFFF;
  return crc;
}

bool XiaomiBMSComponent::check_crc_(const std::vector<uint8_t> &data) {
  if (data.size() < 9)                          return false;
  if (data[0] != 0x55 || data[1] != 0xAA)      return false;
  if ((size_t)(data[2] + 6) != data.size())     return false;
  uint16_t crc = calc_crc_(data);
  return data[data.size()-2] == (crc & 0xFF) &&
         data[data.size()-1] == ((crc >> 8) & 0xFF);
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
//  Sensor publishing
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::parse_bms_data_() {
  const uint8_t *d = raw_bms_;

  if (serial_number_ts_)
    serial_number_ts_->publish_state(decode_ascii_(d + OFF_SERIAL, 14));
  if (firmware_version_ts_) {
    char buf[8]; snprintf(buf, sizeof(buf), "0x%03X", u16_le_(d + OFF_FW_VER));
    firmware_version_ts_->publish_state(buf);
  }
  if (pack_date_ts_)
    pack_date_ts_->publish_state(decode_date_(u16_le_(d + OFF_PACK_DATE)));

  if (design_capacity_sensor_)     design_capacity_sensor_->publish_state(u16_le_(d + OFF_DESIGN_CAP));
  if (real_capacity_sensor_)       real_capacity_sensor_->publish_state(u16_le_(d + OFF_REAL_CAP));
  if (nominal_voltage_sensor_)     nominal_voltage_sensor_->publish_state(u16_le_(d + OFF_NOM_VOLT) / 1000.0f);
  if (charge_cycles_sensor_)       charge_cycles_sensor_->publish_state(u16_le_(d + OFF_CHARGE_CYC));
  if (charge_count_sensor_)        charge_count_sensor_->publish_state(u16_le_(d + OFF_CHARGE_CNT));
  if (max_voltage_sensor_)         max_voltage_sensor_->publish_state(u16_le_(d + OFF_MAX_VOLT) / 100.0f);
  if (max_discharge_current_sensor_) max_discharge_current_sensor_->publish_state(u16_le_(d + OFF_MAX_DIS_CUR) / 100.0f);
  if (max_charge_current_sensor_)  max_charge_current_sensor_->publish_state(u16_le_(d + OFF_MAX_CHG_CUR) / 100.0f);

  uint16_t status = u16_le_(d + OFF_STATUS);
  if (charging_enabled_bs_) charging_enabled_bs_->publish_state(status & STATUS_BIT_CHARGING);
  if (over_voltage_bs_)     over_voltage_bs_->publish_state(status & STATUS_BIT_OV);
  if (over_temp_bs_)        over_temp_bs_->publish_state(status & STATUS_BIT_OT);

  if (remaining_capacity_sensor_)  remaining_capacity_sensor_->publish_state(u16_le_(d + OFF_REMAIN_CAP));
  if (state_of_charge_sensor_)     state_of_charge_sensor_->publish_state(u16_le_(d + OFF_SOC));
  if (pack_current_sensor_)        pack_current_sensor_->publish_state(i16_le_(d + OFF_PACK_CUR) / 100.0f);
  if (pack_voltage_sensor_)        pack_voltage_sensor_->publish_state(u16_le_(d + OFF_PACK_VOLT) / 100.0f);
  if (temp_sensor_1_)              temp_sensor_1_->publish_state((int8_t)(d[OFF_TEMPS])     - 20);
  if (temp_sensor_2_)              temp_sensor_2_->publish_state((int8_t)(d[OFF_TEMPS + 1]) - 20);

  uint16_t bal = u16_le_(d + OFF_BAL_BITS);
  if (balance_bits_sensor_)   balance_bits_sensor_->publish_state(bal);
  if (balancing_cells_ts_)    balancing_cells_ts_->publish_state(decode_balance_bits_(bal));
  if (health_sensor_)         health_sensor_->publish_state(u16_le_(d + OFF_HEALTH));

  uint16_t min_mv = 0xFFFF, max_mv = 0;
  uint8_t  connected = 0;

  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    uint16_t mv = u16_le_(d + OFF_CELLS + i * 2);
    if (mv > 0) {
      connected++;
      if (mv < min_mv) min_mv = mv;
      if (mv > max_mv) max_mv = mv;
    }
    if (cell_voltage_sensors_[i])
      cell_voltage_sensors_[i]->publish_state(mv / 1000.0f);
  }

  if (connected_cells_sensor_) connected_cells_sensor_->publish_state(connected);
  if (connected > 0) {
    float min_v = min_mv / 1000.0f, max_v = max_mv / 1000.0f;
    if (cell_voltage_min_sensor_)   cell_voltage_min_sensor_->publish_state(min_v);
    if (cell_voltage_max_sensor_)   cell_voltage_max_sensor_->publish_state(max_v);
    if (cell_voltage_delta_sensor_) cell_voltage_delta_sensor_->publish_state(max_v - min_v);
  }

  ESP_LOGI(TAG, "BMS OK – SoC=%u%% V=%.2fV I=%.2fA T1=%d°C T2=%d°C",
           u16_le_(d + OFF_SOC),
           u16_le_(d + OFF_PACK_VOLT) / 100.0f,
           i16_le_(d + OFF_PACK_CUR)  / 100.0f,
           (int)(d[OFF_TEMPS])     - 20,
           (int)(d[OFF_TEMPS + 1]) - 20);
}

// ─────────────────────────────────────────────────────────────
//  dump_config
// ─────────────────────────────────────────────────────────────

void XiaomiBMSComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi BMS:");
  ESP_LOGCONFIG(TAG, "  Update interval: %ums", update_interval_ms_);
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
    if (cell_voltage_sensors_[i])
      ESP_LOGCONFIG(TAG, "  Cell %d Voltage: '%s'", i + 1, cell_voltage_sensors_[i]->get_name().c_str());
  }
  LOG_BINARY_SENSOR("  ", "Charging Enabled", charging_enabled_bs_);
  LOG_BINARY_SENSOR("  ", "Over Voltage",     over_voltage_bs_);
  LOG_BINARY_SENSOR("  ", "Over Temperature", over_temp_bs_);
  LOG_TEXT_SENSOR("  ", "Serial Number",    serial_number_ts_);
  LOG_TEXT_SENSOR("  ", "Firmware Version", firmware_version_ts_);
  LOG_TEXT_SENSOR("  ", "Pack Date",        pack_date_ts_);
  LOG_TEXT_SENSOR("  ", "Balancing Cells",  balancing_cells_ts_);
}

}  // namespace xiaomi_bms
}  // namespace esphome
