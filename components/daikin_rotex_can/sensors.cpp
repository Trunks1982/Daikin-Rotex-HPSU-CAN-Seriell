#include "esphome/components/daikin_rotex_can/sensors.h"
#include "esphome/components/daikin_rotex_can/entity.h"

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "daikin_rotex_can";

/////////////////////// CanSensor ///////////////////////

CanSensor::CanSensor()
: m_state(std::numeric_limits<float>::quiet_NaN())
, m_range()
, m_pid(0.2, 0.05f, 0.05f, 0.2, 0.2, 0.1f)
, m_smooth(false)
, m_logging(false)
, m_smooth_state(std::numeric_limits<float>::quiet_NaN())
{
}

CanSensor::CanSensor(std::string const& id)
: CanSensor()
{
    m_config.id = id;
}

bool CanSensor::handleValue(uint16_t value, TEntity::TVariant& current, TVariant& previous) {
    previous = state;
    if (m_config.isSigned) {
        current = static_cast<int16_t>(value) / m_config.divider;
    } else {
        current = value / m_config.divider;
    }

    const float float_value = std::get<float>(current);
    const bool valid = !m_range.required() || (float_value >= m_range.min && float_value <= m_range.max);

    if (valid) {
        publish_state(float_value);
    } else {
        ESP_LOGE(TAG, "CanSensor::handleValue() => Sensor<%s> hex<%s> uint16<%d> float<%f> out of range[%f, %f]",
            get_id().c_str(), Utils::to_hex(value).c_str(), value, float_value, m_range.min, m_range.max);
    }

    return valid;
}

void CanSensor::update(uint32_t millis) {
    TEntity::update(millis);

    if (m_smooth) {
        const float dt = (esphome::millis() - m_pid.get_last_update()) / 1000.0f; // seconds
        if (dt > 10.0f) {
            if (std::isnan(m_smooth_state)) {
                m_smooth_state = m_state;
            }

            std::string logstr;
            m_smooth_state += m_pid.compute(m_state, m_smooth_state, dt, logstr);
            Utils::log("PID", "%s: %s, val: %f", get_id().c_str(), logstr.c_str(), m_smooth_state);

            m_smooth_state = std::ceil(m_smooth_state * 100.0) / 100.0;

            publish_state(m_smooth_state);
        }
    }
}

void CanSensor::publish(float state) {
    m_state = state;
    if (!m_smooth) {
        publish_state(state);
    }
}

/////////////////////// CanTextSensor ///////////////////////

bool CanTextSensor::handleValue(uint16_t value, TEntity::TVariant& current, TVariant& previous) {
    previous = state;
    auto it = m_map.findByKey(value);
    current = m_recalculate_state(m_config.pEntity, it != m_map.end() ? it->second : Utils::format("INVALID<%f>", value));
    publish_state(std::get<std::string>(current));
    return true;
}

/////////////////////// CanBinarySensor ///////////////////////

bool CanBinarySensor::handleValue(uint16_t value, TEntity::TVariant& current, TVariant& previous) {
    previous = state;
    current = value > 0;
    publish_state(std::get<bool>(current));
    return true;
}

/////////////////////// CanNumber ///////////////////////

void CanNumber::control(float value) {
    this->publish_state(value);
    sendSet(m_pCanbus, value * get_config().divider);
}

bool CanNumber::handleValue(uint16_t value, TEntity::TVariant& current, TVariant& previous) {
    previous = state;
    if (m_config.isSigned) {
        current = static_cast<int16_t>(value) / m_config.divider;
    } else {
        current = value / m_config.divider;
    }

    publish_state(std::get<float>(current));
    return true;
}

/////////////////////// CanSelect ///////////////////////

void CanSelect::control(const std::string &value) {
    this->publish_state(value);
    const uint16_t key = getKey(value);
    const bool handled = m_custom_select_lambda(get_id(), key);
    if (!handled) {
        sendSet(m_pCanbus, key);
    }
}

uint16_t CanSelect::getKey(std::string const& value) const {
    auto it = m_map.findByValue(value);
    if (it != m_map.end()) {
        return it->first;
    } else {
        ESP_LOGE(TAG, "CanSelect::getValue(%s) => Value not found!", value.c_str());
    }
    return 0;
}

void CanSelect::publish_select_key(uint16_t key) {
    auto it = m_map.findByKey(key);
    if (it != m_map.end()) {
        publish_state(it->second);
    } else {
        ESP_LOGE(TAG, "CanSelect::publish_select_key(%s) => Key not found!", key);
    }
}

bool CanSelect::handleValue(uint16_t value, TEntity::TVariant& current, TVariant& previous) {
    previous = this->current_option();
    current = findNextByKey(value, Utils::format("INVALID<%f>", value));
    publish_state(std::get<std::string>(current));
    return true;
}

}
}