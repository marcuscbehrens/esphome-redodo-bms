#pragma once
#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace redodo_bms {

static const auto SERVICE_UUID = esphome::esp32_ble::ESPBTUUID::from_raw("0000ffe0-0000-1000-8000-00805f9b34fb");
static const auto NOTIFY_CHAR_UUID = esphome::esp32_ble::ESPBTUUID::from_raw("0000ffe1-0000-1000-8000-00805f9b34fb");
static const auto WRITE_CHAR_UUID = esphome::esp32_ble::ESPBTUUID::from_raw("0000ffe2-0000-1000-8000-00805f9b34fb");

static uint8_t cmd_query_battery_status[] = {0x00, 0x00, 0x04, 0x01, 0x13, 0x55, 0xAA, 0x17};

static const uint8_t MAX_CELLS = 16;

struct RedodoBMSData {
  float battery_voltage = NAN;
  float battery_current = NAN;
  float battery_remaining_ah = NAN;
  float cell_delta_voltage = NAN;
  float battery_soc = NAN;
  float mosfet_temp = NAN;
  float cell_temp = NAN;
  float cell_voltage[MAX_CELLS] = {};
  bool balancing[MAX_CELLS] = {};
  bool problem = false;
  std::string problem_description;
};

class RedodoBMS : public PollingComponent, public ble_client::BLEClientNode {
 public:
  void update() override;
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;
  void parse_notification_(const uint8_t *pData, size_t length);
  void set_sensor_values();
  void clear_sensor_values();

  void set_battery_voltage_sensor(sensor::Sensor *s)      { battery_voltage_sensor_ = s; }
  void set_battery_current_sensor(sensor::Sensor *s)      { battery_current_sensor_ = s; }
  void set_battery_soc_sensor(sensor::Sensor *s)          { battery_soc_sensor_ = s; }
  void set_battery_remaining_ah_sensor(sensor::Sensor *s) { battery_remaining_ah_sensor_ = s; }
  void set_cell_delta_voltage_sensor(sensor::Sensor *s)   { cell_delta_voltage_sensor_ = s; }
  void set_cell_temp_sensor(sensor::Sensor *s)            { cell_temp_sensor_ = s; }
  void set_mosfet_temp_sensor(sensor::Sensor *s)          { mosfet_temp_sensor_ = s; }
  void set_cell_voltage_sensor(uint8_t i, sensor::Sensor *s) {
    if (i < MAX_CELLS) cell_voltage_sensor_[i] = s;
  }
  void set_balancing_binary_sensor(uint8_t i, binary_sensor::BinarySensor *bs) {
    if (i < MAX_CELLS) balancing_sensor_[i] = bs;
  }
  void set_problem_binary_sensor(binary_sensor::BinarySensor *bs) { problem_binary_sensor_ = bs; }
  void set_problem_description_text_sensor(text_sensor::TextSensor *ts) { problem_description_text_sensor_ = ts; }

 protected:
  RedodoBMSData bms_data;

  sensor::Sensor *battery_voltage_sensor_{nullptr};
  sensor::Sensor *battery_current_sensor_{nullptr};
  sensor::Sensor *battery_soc_sensor_{nullptr};
  sensor::Sensor *battery_remaining_ah_sensor_{nullptr};
  sensor::Sensor *cell_delta_voltage_sensor_{nullptr};
  sensor::Sensor *cell_temp_sensor_{nullptr};
  sensor::Sensor *mosfet_temp_sensor_{nullptr};
  sensor::Sensor *cell_voltage_sensor_[MAX_CELLS]{};

  binary_sensor::BinarySensor *balancing_sensor_[MAX_CELLS]{};
  binary_sensor::BinarySensor *problem_binary_sensor_{nullptr};

  text_sensor::TextSensor *problem_description_text_sensor_{nullptr};
};

}  // namespace redodo_bms
}  // namespace esphome