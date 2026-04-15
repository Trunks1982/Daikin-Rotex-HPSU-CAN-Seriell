#include "esphome/components/daikin_rotex_can/texts.h"
#include "esphome/components/daikin_rotex_can/utils.h"

namespace esphome {
namespace daikin_rotex_can {

LogFilterText::LogFilterText() {
    this->publish_state("");
}

void LogFilterText::control(const std::string &value) {
    this->publish_state(value);
    Utils::g_log_filter = value;
}

CustomRequestText::CustomRequestText() {
    this->publish_state("");
}

void CustomRequestText::control(const std::string &value) {
    this->publish_state(value);
    this->parent_->custom_request(value);
}

}  // namespace daikin_rotex_can
}  // namespace esphome
