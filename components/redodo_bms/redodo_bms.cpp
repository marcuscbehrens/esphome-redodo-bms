#include "redodo_bms.h"

namespace esphome {
namespace redodo_bms {

static const char *TAG = "redodo_bms";

void RedodoBMS::update() {
  if (this->parent()->connected() != true) {
    this->clear_sensor_values();
    return;
  }

  auto *chr = this->parent()->get_characteristic(SERVICE_UUID, WRITE_CHAR_UUID);
  if (chr == nullptr) {
    ESP_LOGW(TAG, "Write characteristic not found");
    this->clear_sensor_values();
    return;
  }

  chr->write_value(cmd_query_battery_status, sizeof(cmd_query_battery_status), ESP_GATT_WRITE_TYPE_NO_RSP);
}

void RedodoBMS::gattc_event_handler(esp_gattc_cb_event_t event,
                                    esp_gatt_if_t gattc_if,
                                    esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_CONNECT_EVT:
      ESP_LOGI(TAG, "BLE connected");
      break;

    case ESP_GATTC_DISCONNECT_EVT:
      ESP_LOGW(TAG, "BLE disconnected");
      break;

    case ESP_GATTC_SEARCH_CMPL_EVT: {
      ESP_LOGI(TAG, "Service discovery complete");
      auto *chr = this->parent()->get_characteristic(SERVICE_UUID, NOTIFY_CHAR_UUID);
      if (chr == nullptr) {
        ESP_LOGW(TAG, "Notify characteristic not found");
        this->clear_sensor_values();
        break;
      }
      esp_ble_gattc_register_for_notify(this->parent()->get_gattc_if(), this->parent()->get_remote_bda(), chr->handle);
      break;
    }

    case ESP_GATTC_NOTIFY_EVT:
      ESP_LOGD(TAG, "Notification received (%d bytes)", param->notify.value_len);
      ESP_LOGD(TAG, "Hex: %s", format_hex_pretty(param->notify.value, param->notify.value_len).c_str());
      this->parse_notification_(param->notify.value, param->notify.value_len);
      break;

    default:
      break;
  }
}

static uint32_t read_u32_le(const uint8_t *p) {
  return (uint32_t(p[3]) << 24) | (uint32_t(p[2]) << 16) | (uint32_t(p[1]) << 8) | uint32_t(p[0]);
}

void RedodoBMS::parse_notification_(const uint8_t *pData, size_t length) {
  if (length < 104) {
    this->clear_sensor_values();
    ESP_LOGW(TAG, "Notification too short (%d bytes)", length);
    return;
  }

  // Voltage & current
  bms_data.battery_voltage = read_u32_le(&pData[8]) / 1000.0f;
  bms_data.battery_current = (int32_t) read_u32_le(&pData[48]) / 1000.0f;

  // Temperatures
  bms_data.cell_temp   = (int16_t)((pData[53] << 8) | pData[52]);
  bms_data.mosfet_temp = (int16_t)((pData[55] << 8) | pData[54]);

  // Capacity & SOC
  bms_data.battery_remaining_ah = ((pData[63] << 8) | pData[62]) / 100.0f;
  bms_data.battery_soc          = ((pData[91] << 8) | pData[90]);

  // Individual cell voltages (bytes 16–47, 16 cells × 2 bytes each)
  float min_v = 100.0f, max_v = 0.0f;
  bool found = false;
  for (uint8_t i = 0; i < MAX_CELLS; i++) {
    uint16_t mv = (pData[16 + i * 2 + 1] << 8) | pData[16 + i * 2];
    if (mv == 0) {
      bms_data.cell_voltage[i] = NAN;
      continue;
    }
    bms_data.cell_voltage[i] = mv / 1000.0f;
    min_v = std::min(min_v, bms_data.cell_voltage[i]);
    max_v = std::max(max_v, bms_data.cell_voltage[i]);
    found = true;
  }
  bms_data.cell_delta_voltage = found ? (max_v - min_v) : NAN;

  // Balancing bitmask — bytes 84–87, each bit = one cell
  uint32_t bal = read_u32_le(&pData[84]);
  for (uint8_t i = 0; i < MAX_CELLS; i++) {
    bms_data.balancing[i] = (bal >> i) & 0x01;
  }

  // Protection flags
  uint32_t protection = read_u32_le(&pData[76]);
  bms_data.problem = (protection != 0);
  if (bms_data.problem) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Flags: 0x%08X", protection);
    bms_data.problem_description = buf;
  } else {
    bms_data.problem_description = "OK";
  }

  ESP_LOGD(TAG, "V=%.2fV I=%.2fA ΔV=%.3fV SOC=%.0f%% bal=0x%08X problem=%d",
           bms_data.battery_voltage, bms_data.battery_current,
           bms_data.cell_delta_voltage, bms_data.battery_soc, bal, bms_data.problem);

  set_sensor_values();
}

void RedodoBMS::set_sensor_values() {
  if (battery_voltage_sensor_)      battery_voltage_sensor_->publish_state(bms_data.battery_voltage);
  if (battery_current_sensor_)      battery_current_sensor_->publish_state(bms_data.battery_current);
  if (battery_soc_sensor_)          battery_soc_sensor_->publish_state(bms_data.battery_soc);
  if (battery_remaining_ah_sensor_) battery_remaining_ah_sensor_->publish_state(bms_data.battery_remaining_ah);
  if (cell_delta_voltage_sensor_)   cell_delta_voltage_sensor_->publish_state(bms_data.cell_delta_voltage);
  if (cell_temp_sensor_)            cell_temp_sensor_->publish_state(bms_data.cell_temp);
  if (mosfet_temp_sensor_)          mosfet_temp_sensor_->publish_state(bms_data.mosfet_temp);

  for (uint8_t i = 0; i < MAX_CELLS; i++) {
    if (cell_voltage_sensor_[i])  cell_voltage_sensor_[i]->publish_state(bms_data.cell_voltage[i]);
    if (balancing_sensor_[i])     balancing_sensor_[i]->publish_state(bms_data.balancing[i]);
  }

  if (problem_binary_sensor_)              problem_binary_sensor_->publish_state(bms_data.problem);
  if (problem_description_text_sensor_)    problem_description_text_sensor_->publish_state(bms_data.problem_description);
}

void RedodoBMS::clear_sensor_values() {
  bms_data.battery_voltage = NAN;
  bms_data.battery_current = NAN;
  bms_data.battery_remaining_ah = NAN;
  bms_data.cell_delta_voltage = NAN;
  bms_data.battery_soc = NAN;
  bms_data.mosfet_temp = NAN;
  bms_data.cell_temp = NAN;
  for (uint8_t i = 0; i < MAX_CELLS; i++) {
    bms_data.cell_voltage[i] = NAN;
    bms_data.balancing[i] = false;
  }
  bms_data.problem = false;
  bms_data.problem_description = "";
  set_sensor_values();
}

}  // namespace redodo_bms
}  // namespace esphome