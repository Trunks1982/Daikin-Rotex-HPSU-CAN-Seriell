#include "hpsu_sim.h"
#include "esphome/core/log.h"

#include <cstdio>
#include <sstream>

namespace esphome::hpsu_sim {

static const char *const TAG = "hpsu_sim";
static const uint32_t ROCON_CAN_ID = 0x10A;
static const uint8_t RESPONSE = 0xD2;

void HpsuSim::add_register(std::vector<uint8_t> key, std::vector<uint8_t> data, bool limited, int32_t min,
                           int32_t max, bool is_signed) {
  Register reg;
  reg.data = std::move(data);
  reg.limited = limited;
  reg.min = min;
  reg.max = max;
  reg.is_signed = is_signed;
  this->registers_[std::move(key)] = std::move(reg);
}

std::vector<uint8_t> HpsuSim::key_of(uint8_t const *frame) {
  if (frame[2] == 0xFA) {
    return {frame[2], frame[3], frame[4]};
  }
  return {frame[2]};
}

std::vector<uint8_t> HpsuSim::parse_hex(std::string const &hex) {
  std::vector<uint8_t> result;
  std::istringstream iss(hex);
  std::string token;
  while (iss >> token) {
    result.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
  }
  return result;
}

std::string HpsuSim::to_hex(uint8_t const *data, size_t len) {
  std::string result;
  char buf[4];
  for (size_t i = 0; i < len; ++i) {
    std::snprintf(buf, sizeof(buf), i == 0 ? "%02X" : " %02X", data[i]);
    result += buf;
  }
  return result;
}

void HpsuSim::enqueue(uint32_t can_id, std::vector<uint8_t> const &data) {
  canbus::CanFrame frame{};
  frame.can_id = can_id;
  frame.can_data_length_code = static_cast<uint8_t>(data.size());
  for (size_t i = 0; i < data.size() && i < canbus::CAN_MAX_DATA_LENGTH; ++i) {
    frame.data[i] = data[i];
  }
  this->rx_queue_.push_back(frame);
  ESP_LOGI(TAG, "tx 0x%03X: %s", static_cast<unsigned>(can_id), to_hex(data).c_str());
}

bool HpsuSim::apply_set(uint8_t const *frame, char const *source) {
  const auto key = key_of(frame);
  auto it = this->registers_.find(key);
  if (it == this->registers_.end()) {
    ESP_LOGW(TAG, "set %s from %s: unknown register", to_hex(key).c_str(), source);
    return false;
  }
  Register &reg = it->second;
  const size_t offset = 2 + key.size();
  std::vector<uint8_t> data(frame + offset, frame + offset + reg.data.size());

  if (reg.limited) {
    const uint16_t raw = static_cast<uint16_t>((data[0] << 8) | data[1]);
    const int32_t value = reg.is_signed ? static_cast<int32_t>(static_cast<int16_t>(raw)) : static_cast<int32_t>(raw);
    if (value < reg.min || value > reg.max) {
      ESP_LOGI(TAG, "set %s = %s from %s: dropped (%d outside %d..%d)", to_hex(key).c_str(), to_hex(data).c_str(),
               source, static_cast<int>(value), static_cast<int>(reg.min), static_cast<int>(reg.max));
      return false;
    }
  }
  reg.data = data;
  ESP_LOGI(TAG, "set %s = %s from %s: accepted", to_hex(key).c_str(), to_hex(data).c_str(), source);
  return true;
}

canbus::Error HpsuSim::send_message(canbus::CanFrame *frame) {
  ESP_LOGI(TAG, "rx 0x%03X: %s", static_cast<unsigned>(frame->can_id),
           to_hex(frame->data, frame->can_data_length_code).c_str());
  if (frame->can_data_length_code != 7) {
    ESP_LOGW(TAG, "unexpected frame length %u", frame->can_data_length_code);
    return canbus::ERROR_OK;
  }

  const uint8_t kind = frame->data[0] & 0x0F;
  if (kind == 0x00) {  // set
    this->apply_set(frame->data, "esp");
  } else if (kind == 0x01) {  // read
    const auto key = key_of(frame->data);
    auto it = this->registers_.find(key);
    if (it == this->registers_.end()) {
      ESP_LOGW(TAG, "read %s: unknown register", to_hex(key).c_str());
      return canbus::ERROR_OK;
    }
    std::vector<uint8_t> response{RESPONSE, 0x00};
    response.insert(response.end(), key.begin(), key.end());
    response.insert(response.end(), it->second.data.begin(), it->second.data.end());
    // reply id follows the request type: 0x31 -> 0x180, 0x61 -> 0x300, 0xA1 -> 0x500
    this->enqueue(static_cast<uint32_t>(frame->data[0] >> 4) * 0x80, response);
  }
  return canbus::ERROR_OK;
}

canbus::Error HpsuSim::read_message(canbus::CanFrame *frame) {
  if (this->rx_queue_.empty()) {
    return canbus::ERROR_NOMSG;
  }
  *frame = this->rx_queue_.front();
  this->rx_queue_.pop_front();
  return canbus::ERROR_OK;
}

void HpsuSim::hpsu_set(std::string const &key_hex, std::string const &data_hex) {
  const auto key = parse_hex(key_hex);
  auto it = this->registers_.find(key);
  if (it == this->registers_.end()) {
    ESP_LOGW(TAG, "hpsu_set %s: unknown register", key_hex.c_str());
    return;
  }
  it->second.data = parse_hex(data_hex);
  ESP_LOGI(TAG, "hpsu changed %s = %s", to_hex(key).c_str(), to_hex(it->second.data).c_str());
}

void HpsuSim::rocon_set(std::string const &key_hex, std::string const &data_hex) {
  std::vector<uint8_t> frame{0x30, 0x00};
  const auto key = parse_hex(key_hex);
  const auto data = parse_hex(data_hex);
  frame.insert(frame.end(), key.begin(), key.end());
  frame.insert(frame.end(), data.begin(), data.end());
  if (frame.size() != 7) {
    ESP_LOGW(TAG, "rocon_set %s %s: frame needs 7 bytes", key_hex.c_str(), data_hex.c_str());
    return;
  }
  if (this->apply_set(frame.data(), "rocon")) {
    this->enqueue(ROCON_CAN_ID, frame);
  }
}

void HpsuSim::send_frame(uint32_t can_id, std::string const &data_hex) {
  this->enqueue(can_id, parse_hex(data_hex));
}

}  // namespace esphome::hpsu_sim
