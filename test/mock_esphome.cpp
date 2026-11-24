#include "mock_esphome.h"

#include <chrono>
#include <cstdint>
#include <iostream>

namespace esphome {

namespace {
std::optional<uint32_t> g_fixed_millis;
}

namespace mock {
void set_millis(std::optional<uint32_t> value) {
    g_fixed_millis = value;
}
} // namespace mock

uint32_t millis() {
    if (g_fixed_millis) {
        return *g_fixed_millis;
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void delay(uint32_t ms) {
}

} // namespace esphome
