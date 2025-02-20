#pragma once

#include <string>

namespace esphome {
namespace daikin_rotex_can {

class IAccessor {
public:
    virtual float get_sensor_value(std::string const& sensor_id) const = 0;
    virtual float get_number_value(std::string const& sensor_id) const = 0;
};

}
}
