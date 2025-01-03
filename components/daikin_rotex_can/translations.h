#pragma once

#include <string>

namespace esphome {
namespace daikin_rotex_can {

// Translate struct
struct Translation {
    const char *key;
    const char *value;
};

// Translate function
std::string translate(const std::string &key);

}  // namespace daikin_rotex_can
}  // namespace esphome
