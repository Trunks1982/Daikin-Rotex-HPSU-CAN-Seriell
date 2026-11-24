#pragma once

#include "esphome/components/canbus/canbus.h"

#include <deque>
#include <map>
#include <string>
#include <vector>

namespace esphome::hpsu_sim {

// Simulated HPSU for host tests. See canbus.py for the register model.
class HpsuSim : public canbus::Canbus {
 public:
  void add_register(std::vector<uint8_t> key, std::vector<uint8_t> data, bool limited, int32_t min, int32_t max,
                    bool is_signed);

  // The HPSU changes a value on its own (e.g. operating state heating -> defrosting).
  // Nothing is sent: the ESP sees it with its next read.
  void hpsu_set(std::string const &key_hex, std::string const &data_hex);

  // The RoCon control panel writes a value: the frame from 0x10A reaches the ESP as well.
  void rocon_set(std::string const &key_hex, std::string const &data_hex);

  canbus::CanStatus get_status() override { return {}; }

 protected:
  bool setup_internal() override { return true; }
  canbus::Error send_message(canbus::CanFrame *frame) override;
  canbus::Error read_message(canbus::CanFrame *frame) override;

 private:
  struct Register {
    std::vector<uint8_t> data;
    bool limited{false};
    int32_t min{0};
    int32_t max{0};
    bool is_signed{false};
  };

  static std::vector<uint8_t> key_of(uint8_t const *frame);
  static std::vector<uint8_t> parse_hex(std::string const &hex);
  static std::string to_hex(uint8_t const *data, size_t len);
  static std::string to_hex(std::vector<uint8_t> const &data) { return to_hex(data.data(), data.size()); }

  // Applies a set frame; returns false when the register is unknown or the value is dropped.
  bool apply_set(uint8_t const *frame, char const *source);
  void enqueue(uint32_t can_id, std::vector<uint8_t> const &data);

  std::map<std::vector<uint8_t>, Register> registers_;
  std::deque<canbus::CanFrame> rx_queue_;
};

}  // namespace esphome::hpsu_sim
