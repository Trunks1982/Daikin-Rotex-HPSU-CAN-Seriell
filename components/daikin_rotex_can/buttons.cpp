#include "esphome/components/daikin_rotex_can/buttons.h"

namespace esphome {
namespace daikin_rotex_can {

void DHWRunButton::press_action() {
    this->parent_->dhw_run();
}

void DumpButton::press_action() {
    this->parent_->dump();
}

void CustomNumber::control(float value) {
    this->parent_->on_custom_number(*this, value);
}

}  // namespace seeed_mr24hpc1
}  // namespace esphome
