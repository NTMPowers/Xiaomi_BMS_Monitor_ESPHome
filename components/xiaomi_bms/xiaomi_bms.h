#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <array>
#include <vector>

namespace esphome {
namespace xiaomi_bms {

// ─────────────────────────────────────────────────────────────
//  Protocol constants
// ─────────────────────────────────────────────────────────────
static const uint8_t FRAME_SOF0 = 0x55;
static const uint8_t FRAME_SOF1 = 0xAA;
static const uint8_t BMS_ADDR   = 0x22;
static const uint8_t MODE_READ  = 0x01;

// Payload map offsets (byte addresses in the assembled 0xA2-byte block)
static const uint16_t BMS_TOTAL_SIZE   = 0xA2;
static const uint8_t  OFF_SERIAL       = 0x20;
static const uint8_t  OFF_FW_VER       = 0x2E;
static const uint8_t  OFF_DESIGN_CAP   = 0x30;
static const uint8_t  OFF_REAL_CAP     = 0x32;
static const uint8_t  OFF_NOM_VOLT     = 0x34;
static const uint8_t  OFF_CHARGE_CYC   = 0x36;
static const uint8_t  OFF_CHARGE_CNT   = 0x38;
static const uint8_t  OFF_MAX_VOLT     = 0x3A;
static const uint8_t  OFF_MAX_DIS_CUR  = 0x3C;
static const uint8_t  OFF_MAX_CHG_CUR  = 0x3E;
static const uint8_t  OFF_PACK_DATE    = 0x40;
static const uint8_t  OFF_ERROR_BYTES  = 0x42;  // 6 bytes
static const uint8_t  OFF_STATUS       = 0x60;
static const uint8_t  OFF_REMAIN_CAP   = 0x62;
static const uint8_t  OFF_SOC          = 0x64;
static const uint8_t  OFF_PACK_CUR     = 0x66;
static const uint8_t  OFF_PACK_VOLT    = 0x68;
static const uint8_t  OFF_TEMPS        = 0x6A;  // 2 bytes
static const uint8_t  OFF_BAL_BITS     = 0x6C;
static const uint8_t  OFF_HEALTH       = 0x76;
static const uint8_t  OFF_CELLS        = 0x80;  // 15 × uint16_t

// Status flag bits
static const uint16_t STATUS_BIT_CHARGING = (1 << 6);
static const uint16_t STATUS_BIT_OV       = (1 << 9);
static const uint16_t STATUS_BIT_OT       = (1 << 10);

static const uint8_t NUM_CELLS = 15;

// Chunk descriptors: { label, byte_offset, size }
struct ChunkSpec {
  const char *label;
  uint8_t byte_offset;
  uint8_t size;
};

static const ChunkSpec CHUNK_SPECS[] = {
  {"header",   0x00, 0x20},
  {"identity", 0x20, 0x20},
  {"meta",     0x40, 0x20},
  {"live",     0x60, 0x20},
  {"cells",    0x80, 0x22},
};
static const uint8_t NUM_CHUNKS = sizeof(CHUNK_SPECS) / sizeof(CHUNK_SPECS[0]);

// Wake frame – repeated 30× on startup / after missed response
static const uint8_t WAKE_FRAME[] = {0x55, 0xAA, 0x03, 0x22, 0x01, 0x30, 0x0C, 0x9D, 0xFF};
static const uint8_t WAKE_REPS    = 30;

// ─────────────────────────────────────────────────────────────
//  Component
// ─────────────────────────────────────────────────────────────
class XiaomiBMSComponent : public PollingComponent, public uart::UARTDevice {
 public:
  // ── lifecycle ────────────────────────────────────────────
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // ── sensor setters ────────────────────────────────────────
  void set_state_of_charge_sensor(sensor::Sensor *s)      { state_of_charge_sensor_      = s; }
  void set_health_sensor(sensor::Sensor *s)               { health_sensor_               = s; }
  void set_pack_voltage_sensor(sensor::Sensor *s)         { pack_voltage_sensor_         = s; }
  void set_pack_current_sensor(sensor::Sensor *s)         { pack_current_sensor_         = s; }
  void set_remaining_capacity_sensor(sensor::Sensor *s)   { remaining_capacity_sensor_   = s; }
  void set_design_capacity_sensor(sensor::Sensor *s)      { design_capacity_sensor_      = s; }
  void set_real_capacity_sensor(sensor::Sensor *s)        { real_capacity_sensor_        = s; }
  void set_nominal_voltage_sensor(sensor::Sensor *s)      { nominal_voltage_sensor_      = s; }
  void set_max_voltage_sensor(sensor::Sensor *s)          { max_voltage_sensor_          = s; }
  void set_max_charge_current_sensor(sensor::Sensor *s)   { max_charge_current_sensor_   = s; }
  void set_max_discharge_current_sensor(sensor::Sensor *s){ max_discharge_current_sensor_= s; }
  void set_temp_sensor_1(sensor::Sensor *s)               { temp_sensor_1_               = s; }
  void set_temp_sensor_2(sensor::Sensor *s)               { temp_sensor_2_               = s; }
  void set_cell_voltage_min_sensor(sensor::Sensor *s)     { cell_voltage_min_sensor_     = s; }
  void set_cell_voltage_max_sensor(sensor::Sensor *s)     { cell_voltage_max_sensor_     = s; }
  void set_cell_voltage_delta_sensor(sensor::Sensor *s)   { cell_voltage_delta_sensor_   = s; }
  void set_connected_cells_sensor(sensor::Sensor *s)      { connected_cells_sensor_      = s; }
  void set_charge_cycles_sensor(sensor::Sensor *s)        { charge_cycles_sensor_        = s; }
  void set_charge_count_sensor(sensor::Sensor *s)         { charge_count_sensor_         = s; }
  void set_balance_bits_sensor(sensor::Sensor *s)         { balance_bits_sensor_         = s; }

  // Individual cell voltage sensors (index 0‥14 → cell 1‥15)
  void set_cell_voltage_sensor(uint8_t index, sensor::Sensor *s) {
    if (index < NUM_CELLS)
      cell_voltage_sensors_[index] = s;
  }

  // ── binary-sensor setters ─────────────────────────────────
  void set_charging_enabled_binary_sensor(binary_sensor::BinarySensor *s) { charging_enabled_bs_ = s; }
  void set_over_voltage_binary_sensor(binary_sensor::BinarySensor *s)     { over_voltage_bs_     = s; }
  void set_over_temperature_binary_sensor(binary_sensor::BinarySensor *s) { over_temp_bs_        = s; }

  // ── text-sensor setters ───────────────────────────────────
  void set_serial_number_text_sensor(text_sensor::TextSensor *s)   { serial_number_ts_   = s; }
  void set_firmware_version_text_sensor(text_sensor::TextSensor *s){ firmware_version_ts_= s; }
  void set_pack_date_text_sensor(text_sensor::TextSensor *s)        { pack_date_ts_        = s; }
  void set_balancing_cells_text_sensor(text_sensor::TextSensor *s)  { balancing_cells_ts_  = s; }

 protected:
  // ── internal helpers ──────────────────────────────────────
  void send_wake_();
  void drain_rx_();
  bool read_chunk_(uint8_t byte_offset, uint8_t size);
  bool read_packet_(uint32_t timeout_ms, std::vector<uint8_t> &out);
  void build_request_(uint8_t offset_word, uint8_t size, std::vector<uint8_t> &frame);
  bool check_crc_(const std::vector<uint8_t> &data);
  uint16_t calc_crc_(const std::vector<uint8_t> &data);
  void parse_bms_data_();

  static uint16_t u16_le_(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
  static int16_t  i16_le_(const uint8_t *p) { return (int16_t)(p[0] | (p[1] << 8)); }
  static std::string decode_date_(uint16_t raw);
  static std::string decode_ascii_(const uint8_t *p, uint8_t len);
  static std::string decode_balance_bits_(uint16_t bits);

  // ── assembled BMS payload ─────────────────────────────────
  uint8_t raw_bms_[BMS_TOTAL_SIZE]{};

  // ── sensors ───────────────────────────────────────────────
  sensor::Sensor *state_of_charge_sensor_       = nullptr;
  sensor::Sensor *health_sensor_                = nullptr;
  sensor::Sensor *pack_voltage_sensor_          = nullptr;
  sensor::Sensor *pack_current_sensor_          = nullptr;
  sensor::Sensor *remaining_capacity_sensor_    = nullptr;
  sensor::Sensor *design_capacity_sensor_       = nullptr;
  sensor::Sensor *real_capacity_sensor_         = nullptr;
  sensor::Sensor *nominal_voltage_sensor_       = nullptr;
  sensor::Sensor *max_voltage_sensor_           = nullptr;
  sensor::Sensor *max_charge_current_sensor_    = nullptr;
  sensor::Sensor *max_discharge_current_sensor_ = nullptr;
  sensor::Sensor *temp_sensor_1_                = nullptr;
  sensor::Sensor *temp_sensor_2_                = nullptr;
  sensor::Sensor *cell_voltage_min_sensor_      = nullptr;
  sensor::Sensor *cell_voltage_max_sensor_      = nullptr;
  sensor::Sensor *cell_voltage_delta_sensor_    = nullptr;
  sensor::Sensor *connected_cells_sensor_       = nullptr;
  sensor::Sensor *charge_cycles_sensor_         = nullptr;
  sensor::Sensor *charge_count_sensor_          = nullptr;
  sensor::Sensor *balance_bits_sensor_          = nullptr;
  sensor::Sensor *cell_voltage_sensors_[NUM_CELLS]{};

  binary_sensor::BinarySensor *charging_enabled_bs_ = nullptr;
  binary_sensor::BinarySensor *over_voltage_bs_     = nullptr;
  binary_sensor::BinarySensor *over_temp_bs_        = nullptr;

  text_sensor::TextSensor *serial_number_ts_    = nullptr;
  text_sensor::TextSensor *firmware_version_ts_ = nullptr;
  text_sensor::TextSensor *pack_date_ts_         = nullptr;
  text_sensor::TextSensor *balancing_cells_ts_   = nullptr;
};

}  // namespace xiaomi_bms
}  // namespace esphome
