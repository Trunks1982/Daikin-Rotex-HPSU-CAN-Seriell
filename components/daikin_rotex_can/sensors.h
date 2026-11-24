#pragma once

#include "esphome/components/daikin_rotex_can/sensor_accessor.h"
#include "esphome/components/daikin_rotex_can/bidi_map.h"
#include "esphome/components/daikin_rotex_can/entity.h"
#include "esphome/components/daikin_rotex_can/pid.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include <limits>

namespace esphome {
namespace daikin_rotex_can {

/////////////////////// CanSensor ///////////////////////

class CanSensor : public sensor::Sensor, public TEntity, public Parented<SensorAccessor> {
    struct Range {
        float min;
        float max;

        // [0, 0] means "no range" (default in __init__.py); a range may start or end at 0
        bool required() const { return !(min == 0 && max == 0); }
    };

public:
    CanSensor();
    CanSensor(std::string const& id);
    void set_range(Range const& range) { m_range = range; }
    /// Raw value the HPSU sends when it has no value for this register: published as unknown (NaN).
    void set_no_value(uint16_t raw) { m_no_value = raw; m_has_no_value = true; }
    void set_smooth(bool smooth) { m_smooth = smooth; }
    void set_logging(bool logging) { m_logging = logging; m_pid.set_logging(logging); }
    /// Read the value every poll but publish it only every publish_interval_ms, together with the
    /// highest value read since the previous publish on max_sensor (e.g. tv / tv_max).
    void set_max_sensor(sensor::Sensor* max_sensor, uint32_t publish_interval_ms) {
        m_max_sensor = max_sensor;
        m_publish_interval = publish_interval_ms;
    }
    virtual void update(uint32_t millis) override;
    void publish(float state);
protected:
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) override;
private:
    float m_state;
    Range m_range;
    PID m_pid;
    bool m_smooth;
    bool m_logging;
    float m_smooth_state;
    sensor::Sensor* m_max_sensor = nullptr;
    uint32_t m_publish_interval = 0;
    uint32_t m_last_publish = 0;
    bool m_published = false;
    float m_window_max = std::numeric_limits<float>::quiet_NaN();
    uint16_t m_no_value = 0;
    bool m_has_no_value = false;
};

/////////////////////// CanTextSensor ///////////////////////

class CanTextSensor : public text_sensor::TextSensor, public TEntity, public Parented<SensorAccessor> {
public:
    using TRecalculateState = std::function<std::string(EntityBase*, std::string const&)>;

    CanTextSensor() = default;
    void set_map(std::string const& str_map) { m_map = Utils::str_to_map(str_map); }
    void set_recalculate_state(TRecalculateState&& lambda) { m_recalculate_state = std::move(lambda); }
protected:
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) override;
private:
    BidiMap m_map;
    TRecalculateState m_recalculate_state;
};

/////////////////////// CanBinarySensor ///////////////////////

class CanBinarySensor : public binary_sensor::BinarySensor, public TEntity, public Parented<SensorAccessor> {
public:
    CanBinarySensor() = default;

protected:
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) override;
};

/////////////////////// CanNumber ///////////////////////

class CanNumber : public number::Number, public TEntity, public Parented<SensorAccessor> {
public:
    CanNumber() = default;
protected:
    void control(float value) override;
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) override;
};

/////////////////////// CanSelect ///////////////////////

class CanSelect : public select::Select, public TEntity, public Parented<SensorAccessor> {
    using TCustomSelectLambda = std::function<bool(std::string const& id, uint16_t key)>;
public:
    CanSelect() = default;
    void set_map(std::string const& str_map) { m_map = Utils::str_to_map(str_map); }
    void set_custom_select_lambda(TCustomSelectLambda&& lambda) { m_custom_select_lambda = std::move(lambda); }

    std::string findNextByKey(uint16_t value, std::string const& fallback) const;
    uint16_t getKey(std::string const& value) const;

    void publish_select_key(uint16_t key);
protected:
    void control(const std::string &value) override;
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) override;

private:
    BidiMap m_map;
    TCustomSelectLambda m_custom_select_lambda;
};

inline std::string CanSelect::findNextByKey(uint16_t value, std::string const& fallback) const { 
    auto it = m_map.findNextByKey(value);
    return it != m_map.end() ? it->second : fallback;
};

/////////////////////// CanSwitch ///////////////////////

class CanSwitch : public switch_::Switch, public TEntity, public Parented<SensorAccessor> {
    using TCustomSelectLambda = std::function<bool(std::string const& id, uint16_t key)>;
public:
    CanSwitch() = default;
protected:
    virtual void write_state(bool state) override;
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) override;
};

}  // namespace ld2410
}  // namespace esphome
