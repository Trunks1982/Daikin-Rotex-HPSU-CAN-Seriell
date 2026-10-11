#include "esphome/components/daikin_rotex_can/texts.h"
#include "esphome/components/daikin_rotex_can/utils.h"
#include "esphome/core/log.h"

#include <algorithm>

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "CanProgramText";

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

/////////////////////// CanProgramText ///////////////////////

CanProgramText::CanProgramText()
: m_current_command(0u)
, m_sent(0u)
, m_received(0u)
, m_collecting(false)
{
    this->publish_state("");
}

void CanProgramText::add_label(std::string const& label) {
    m_labels.push_back(label);
}

void CanProgramText::add_command(uint8_t register_offset) {
    TMessage command = {0x61, 0x00, 0xFA, HZ_PROGRAM_PAGE, register_offset, 0x00, 0x00};
    m_commands.push_back(command);
    m_values.push_back(VALUE_UNCOLLECTED);
}

int CanProgramText::findRegisterIndex(uint8_t register_offset) const {
    for (size_t i = 0; i < m_commands.size(); ++i) {
        if (m_commands[i][4] == register_offset) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void CanProgramText::publish_program() {
    std::string result = "{";

    for (size_t block = 0; block < m_labels.size(); ++block) {
        if (block > 0) {
            result += ", ";
        }
        result += m_labels[block];
        result += ": ";

        for (size_t window = 0; window < WINDOWS_PER_BLOCK; ++window) {
            if (window > 0) {
                result += " ";
            }

            const size_t index = block * WINDOWS_PER_BLOCK + window;
            if (index >= m_values.size() || m_values[index] == VALUE_UNCOLLECTED) {
                result += 'x';
                continue;
            }

            const uint8_t start = static_cast<uint8_t>((m_values[index] >> 8) & 0xFF);
            const uint8_t end = static_cast<uint8_t>(m_values[index] & 0xFF);
            if (start == 0x80 || end == 0x80) {  // sentinel => unused time window
                result += 'x';
            } else {
                result += Utils::format("%u-%u", static_cast<unsigned>(start), static_cast<unsigned>(end));
            }
        }
    }

    result += '}';

    if (result.size() >= 255) {
        ESP_LOGE(TAG, "publish_program() => generated state is invalid (%u chars >= 255); not publishing!",
            static_cast<unsigned>(result.size()));
        return;
    }

    Utils::log("publish_program", "%s", result.c_str());
    this->publish_state(result);
}

bool CanProgramText::sendGet(esphome::esp32_can::ESP32Can* pCanBus) {
    if (pCanBus == nullptr) {
        ESP_LOGE(TAG, "sendGet() => pCanbus is null!");
        return false;
    }
    if (m_commands.empty()) {
        return false;
    }

    const uint32_t can_id = 0x680;
    const bool use_extended_id = false;

    TMessage const& command = m_commands[m_current_command];
    pCanBus->send_data(can_id, use_extended_id, {command.begin(), command.end()});

    Utils::log("sendGet", "name<%s> can_id<%s> data<%s> (received %u/%u)",
        getName().c_str(), Utils::to_hex(can_id).c_str(), Utils::to_hex(command).c_str(),
        static_cast<unsigned>(m_received), static_cast<unsigned>(m_commands.size()));

    m_current_command = (m_current_command + 1) % m_commands.size();
    ++m_sent;
    m_collecting = true;
    m_last_get_timestamp = esphome::millis();
    m_last_handle_timestamp = 0u;   // mark as get in progress until a response is handled
    return true;
}

bool CanProgramText::handle(uint32_t can_id, TMessage const& responseData) {
    if (can_id != m_config.can_id || responseData.size() != 7) {
        return false;
    }
    if (responseData[2] != 0xFA || responseData[3] != HZ_PROGRAM_PAGE) {
        return false;
    }

    const int index = findRegisterIndex(responseData[4]);
    if (index < 0) {
        return false;
    }

    const uint16_t value = (static_cast<uint16_t>(responseData[5]) << 8) | responseData[6];

    if (m_values[index] == VALUE_UNCOLLECTED) {
        m_values[index] = value;
        ++m_received;
    }

    Utils::log("handle", "name<%s> data<%s> value<0x%04X> (received %u/%u)",
        getName().c_str(), Utils::to_hex(responseData).c_str(), value,
        static_cast<unsigned>(m_received), static_cast<unsigned>(m_commands.size()));

    m_last_handle_timestamp = esphome::millis();

    if (m_received >= m_commands.size() && m_commands.size() > 0) {
        publish_program();
        std::fill(m_values.begin(), m_values.end(), VALUE_UNCOLLECTED);
        m_received = 0u;
        m_current_command = 0u;
        m_sent = 0u;
        m_collecting = false;
    }
    return true;
}

bool CanProgramText::isGetNeeded() const {
    if (!is_command_configured() || m_commands.empty()) {
        return false;
    }
    if (isGetInProgress()) {
        return false;
    }
    if (m_collecting) {
        return true;    // mid-cycle: keep rotating until all registers are collected
    }
    return getLastUpdate() == 0 || (esphome::millis() > (getLastUpdate() + get_update_interval()));
}

void CanProgramText::update(uint32_t millis) {
    TEntity::update(millis);

    // If the whole collection cycle did not complete (e.g. registers that never answer),
    // publish the partial result (missing windows are rendered as "x") and start over.
    if (m_collecting && !isGetInProgress() && m_sent >= (MAX_CYCLE_RETRIES * m_commands.size())) {
        ESP_LOGW(TAG, "update() => program collection incomplete (%u/%u); publishing partial state",
            static_cast<unsigned>(m_received), static_cast<unsigned>(m_commands.size()));
        publish_program();
        std::fill(m_values.begin(), m_values.end(), VALUE_UNCOLLECTED);
        m_received = 0u;
        m_current_command = 0u;
        m_sent = 0u;
        m_collecting = false;
    }
}

bool CanProgramText::handleValue(uint16_t value, TVariant& current, TVariant& previous) {
    // Not used: CanProgramText overrides handle() entirely to aggregate multiple registers.
    (void)value;
    (void)current;
    (void)previous;
    return false;
}

void CanProgramText::control(const std::string &value) {
    // Write-back to the HPSU is deferred to a later session. The input is accepted for
    // HA compatibility but nothing is sent on the CAN bus yet, and the HPSU remains the
    // single source of truth (the previously read program is not overwritten).
    ESP_LOGI(TAG, "control(%s) => write-back not implemented yet; state kept from last CAN read.",
        value.c_str());
}

}  // namespace daikin_rotex_can
}  // namespace esphome