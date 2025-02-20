#include "esphome/components/daikin_rotex_can/entity.h"
#include "esphome/components/esp32_can/esp32_can.h"
#include "esphome/core/log.h"

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "daikin_rotex_can";

TEntity::TEntity()
: m_config()
, m_pCanbus(nullptr)
, m_pAccessor(nullptr)
, m_expected_reponse()
, m_last_handle_timestamp(0u)
, m_last_get_timestamp(0u)
, m_last_value_change_timestamp(0u)
, m_post_handle_lambda()
{
}

std::array<uint16_t, 7> TEntity::calculate_reponse(TMessage const& message) {
    const uint16_t DC = 0xFFFF;
    std::array<uint16_t, 7> response = {DC, DC, DC, DC, DC, DC, DC};
    if (message[2] == 0xFA) {   // https://github.com/crycode-de/ioBroker.canbus/blob/master/well-known-messages/configs/rotex-hpsu.md
        response[2] = message[2];
        response[3] = message[3];
        response[4] = message[4];
    } else {
        response[2] = message[2];
    }
    return response;
}

bool TEntity::isGetInProgress() const {
    return m_last_get_timestamp > m_last_handle_timestamp && ((millis() - m_last_get_timestamp) < 3*1000); // Consider 3 sek => package is lost
}

bool TEntity::isMatch(uint32_t can_id, TMessage const& responseData) const {
    const bool is_set = (responseData[0] & 0x0F) == 0x00;
    const bool is_response = (responseData[0] & 0x0F) == 0x02;

    const bool is_our_response = can_id == m_config.can_id;                             // Listen for responses caused by our sendGet
    const bool is_rocon_panel_response = can_id == 0x10A && (is_response || is_set);    // Listen for responses and sets caused by RoCon control panel (0x10A)

    const bool is_valid = is_our_response || is_rocon_panel_response;

    if (is_valid) {
        for (uint32_t index = 0; index < responseData.size(); ++index) {
            if (m_expected_reponse[index] != DC && responseData[index] != m_expected_reponse[index]) {
                return false;
            }
        }
        return responseData.size() == 7;
    }
    return false;
}

bool TEntity::handle(uint32_t can_id, TMessage const& responseData) {
    if (isMatch(can_id, responseData)) {

        TEntity::TVariant current, previous;
        bool valid = false;

        if (m_config.data_offset > 0 && (m_config.data_offset + m_config.data_size) <= 7) {
            if (m_config.data_size >= 1 && m_config.data_size <= 2) {
                const uint16_t value = m_config.handle_lambda_set ? m_config.handle_lambda(responseData) :
                    (
                        m_config.data_size == 2 ?
                        (((responseData[m_config.data_offset] << 8) + responseData[m_config.data_offset + 1])) :
                        (responseData[m_config.data_offset])
                    );
                valid = handleValue(value, current, previous);
            } else {
                ESP_LOGE(TAG, "handle: Invalid data size: %d", m_config.data_size);
            }
        } else {
            ESP_LOGE(TAG, "handle: Invalid data_offset: %d", m_config.data_offset);
        }

        if (valid) {
            const bool changed = current != previous;
            if (changed) {
                m_last_value_change_timestamp = millis();
                m_post_handle_lambda(this, current, previous);
            }

            std::string value;
            if (std::holds_alternative<uint32_t>(current)) {
                value = std::to_string(std::get<uint32_t>(current));
            } else if (std::holds_alternative<uint8_t>(current)) {
                value = std::to_string(std::get<uint8_t>(current));
            } else if (std::holds_alternative<float>(current)) {
                value = std::to_string(std::get<float>(current));
            } else if (std::holds_alternative<bool>(current)) {
                value = std::get<bool>(current);
            } else if (std::holds_alternative<std::string>(current)) {
                value = std::get<std::string>(current);
            } else {
                value = "Unsupported value type!";
            }

            Utils::log("handle ", "%s<%s> can_id<%s> data<%s> changed<%d>",
                getName().c_str(), value.c_str(), Utils::to_hex(can_id).c_str(), Utils::to_hex(responseData).c_str(), changed);
        }
        m_last_handle_timestamp = millis();
        return true;
    }
    return false;
}

bool TEntity::sendGet(esphome::esp32_can::ESP32Can* pCanBus) {
    if (pCanBus == nullptr) {
        ESP_LOGE(TAG, "sendGet: pCanbus is null!");
        return false;
    }

    const uint32_t can_id = 0x680;
    const bool use_extended_id = false;

    pCanBus->send_data(can_id, use_extended_id, { m_config.command.begin(), m_config.command.end() });

    Utils::log("sendGet", "%s can_id<%s> command<%s>",
        getName().c_str(), Utils::to_hex(can_id).c_str(), Utils::to_hex(m_config.command).c_str());

    m_last_get_timestamp = millis();
    return true;
}

bool TEntity::sendSet(esphome::esp32_can::ESP32Can* pCanBus, float value) {
    if (pCanBus == nullptr) {
        ESP_LOGE(TAG, "sendSet: pCanbus is null!");
        return false;
    }

    const uint32_t can_id = 0x680;
    const bool use_extended_id = false;

    TMessage command = TMessage(m_config.command);
    command[0] = 0x30;
    command[1] = 0x00;
    if (m_config.set_lambda_set) {
        m_config.set_lambda(command, value);
    } else {
        Utils::setBytes(command, value, m_config.data_offset, m_config.data_size);
    }

    pCanBus->send_data(can_id, use_extended_id, { command.begin(), command.end() });
    Utils::log("sendSet", "name<%s> value<%f> can_id<%s> command<%s>",
        getName().c_str(), value, Utils::to_hex(can_id).c_str(), Utils::to_hex(command).c_str());

    sendGet(pCanBus);

    return true;
}

void TEntity::update(uint32_t millis) {
    if (isGetInProgress() && millis > (m_last_get_timestamp + 5 * 1000)) {
        ESP_LOGE(TAG, "TEntity::update() sedGet timeout! id: %s", m_config.id.c_str());
    }
}

}
}