#pragma once

namespace esphome {
namespace daikin_rotex_can {

enum class EntityType {
    SENSOR,
    TEXT_SENSOR,
    BINARY_SENSOR,
    NUMBER,
    SELECT,
    SWITCH,
    UNKNOWN
};

}
}