#pragma once

#include <cstdint>
#include <optional>

namespace esphome::mock {

/// Fixes the value esphome::millis() returns; std::nullopt goes back to the real clock.
void set_millis(std::optional<uint32_t> value);

} // namespace esphome::mock
