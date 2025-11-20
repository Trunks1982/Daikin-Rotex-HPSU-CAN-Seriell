#pragma once

#include "esphome/components/daikin_rotex_can/persistent_value.h"
#include "esphome/components/daikin_rotex_can/entity_manager.h"
#include "esphome/components/daikin_rotex_can/sensors.h"
#include "esphome/components/daikin_rotex_can/pid.h"
#include "esphome/components/esp32_can/esp32_can.h"
#include "esphome/core/component.h"
#include <list>

namespace esphome {
namespace daikin_rotex_can {

class DaikinRotexCanComponent: public Component, public SensorAccessor, public IAccessor {
    struct MaxSpread {
        float tvbh_tv;
        float tvbh_tr;
    };

    struct TvTvBHTrOffset {
        float tv;
        float tvbh;
        float tr;
    };

    class ErrorDetection {
    public:
        ErrorDetection(uint32_t detection_time_ms, bool m_stop_detection_in_good_case);
        bool handle_error_detection(bool is_error_state);
        bool is_good_case_detected() const { return m_good_case_detected; }
        uint32_t get_error_detection_timestamp() const { return m_error_timestamp; }
        void reset_good_case();

    private:
        uint32_t m_error_timestamp;
        uint32_t m_detection_time_ms;
        bool m_good_case_detected;
        bool m_stop_detection_in_good_case;
    };
public:
    using TVoidFunc = std::function<void()>;

    DaikinRotexCanComponent();
    void setup() override;
    void loop() override;
    void dump_config() override;

    void set_canbus(esphome::esp32_can::ESP32Can* pCanbus);
    void set_update_interval(uint16_t seconds) {} // dummy
    void set_project_git_hash(text_sensor::TextSensor* pSensor, std::string const& hash);
    void set_thermal_power_sensor(CanSensor* pSensor);
    void set_thermal_power_sensor_raw(CanSensor* pSensor);
    void set_temperature_spread(CanSensor* pSensor);
    void set_temperature_spread_raw(CanSensor* pSensor);
    void set_tv_tvbh_delta(CanSensor* pSensor);
    void set_tvbh_tr_delta(CanSensor* pSensor);
    void set_vorlauf_soll_tv_delta(CanSensor* pSensor);
    void set_max_spread(float tvbh_tv, float tvbh_tr);
    void set_tv_tvbh_tr_offset(float tv_offset, float tvbh_offset, float tr_offset);
    void add_entity(TEntity* pEntity);
    void set_supply_setpoint_regulated(number::Number* pNumber);

    void on_post_handle(TEntity* pRequest, TEntity::TVariant const& current, TEntity::TVariant const& previous);

    // Texts
    virtual void custom_request(std::string const& value) override;

    // Buttons
    virtual void dhw_run() override;
    virtual void dump() override;

    // CustomCanNumbers
    void on_custom_number(number::Number& number, float value);

    // IAccessor
    virtual float get_sensor_value(std::string const& sensor_id) const override;
    virtual float get_number_value(std::string const& sensor_id) const override;

    void handle(uint32_t can_id, std::vector<uint8_t> const& data);

    void run_dhw_lambdas();
    void call_later(TVoidFunc lambda, uint32_t timeout = 0u) {
        const uint32_t timestamp = millis();
        m_later_calls.push_back({lambda, timestamp + timeout});
    }

    void update_thermal_power();
    void update_temperature_spread();

private:

    using TCanbusAutomation = esphome::Automation<std::vector<uint8_t>, uint32_t, bool>;
    using TCanbusAction = esphome::Action<std::vector<uint8_t>, uint32_t, bool>;

    class MyAction : public TCanbusAction {
    public:
        MyAction(DaikinRotexCanComponent* pParent): m_pParent(pParent) {}
    protected:
        void play(const std::vector<uint8_t> &data, const uint32_t &can_id,
                  const bool &remote_transmission_request) override
        {
            m_pParent->handle(can_id, data);
        }

    private:
        DaikinRotexCanComponent* m_pParent;
    };

    void updateState(std::string const& id);
    bool on_custom_select(std::string const& id, uint8_t value);
    void on_betriebsart(TEntity::TVariant const& current, TEntity::TVariant const& previous);
    void on_betriebsmodus(TEntity::TVariant const& current, TEntity::TVariant const& previous);

    bool is_command_set(TMessage const&);
    static bool is_modus_heating(std::string const& modus);
    std::string recalculate_state(EntityBase* pEntity, std::string const& new_state);
    void update_supply_setpoint_regulated();

    esphome::daikin_rotex_can::TEntityManager m_entity_manager;
    std::shared_ptr<esphome::canbus::CanbusTrigger> m_canbus_trigger;
    std::shared_ptr<TCanbusAutomation> m_canbus_automation;
    std::shared_ptr<MyAction> m_canbus_action;
    std::list<std::pair<TVoidFunc, uint32_t>> m_later_calls;
    std::list<TVoidFunc> m_dhw_run_lambdas;

    PersistentValue<bool> m_optimized_defrosting;
    std::string m_betriebsmodus_before_dhw_and_defrosting;
    text_sensor::TextSensor* m_project_git_hash_sensor;
    std::string m_project_git_hash;
    esphome::esp32_can::ESP32Can* m_pCanbus;

    CanSensor* m_thermal_power_sensor;
    CanSensor* m_thermal_power_raw_sensor;
    CanSensor* m_temperature_spread_sensor;
    CanSensor* m_temperature_spread_raw_sensor;
    CanSensor* m_tv_tvbh_delta_sensor;
    CanSensor* m_tvbh_tr_delta_sensor;
    CanSensor* m_vorlauf_soll_tv_delta;
    MaxSpread m_max_spread;
    TvTvBHTrOffset m_tv_tvbh_tr_offset;
    ErrorDetection m_mixer_error_detection;
    ErrorDetection m_bpv_error_detection;
    ErrorDetection m_spread_error_detection;
    ErrorDetection m_dhw_error_detection;
    number::Number* m_supply_setpoint_regulated;
    uint32_t m_last_supply_setpoint_regulated_ts;
};

inline void DaikinRotexCanComponent::set_canbus(esphome::esp32_can::ESP32Can* pCanbus) {
    m_entity_manager.setCanbus(pCanbus);
    m_pCanbus = pCanbus;

    m_canbus_trigger = std::make_shared<esphome::canbus::CanbusTrigger>(pCanbus, 0, 0, false); // Listen to all can messages
    m_canbus_automation = std::make_shared<TCanbusAutomation>(m_canbus_trigger.get());
    m_canbus_action = std::make_shared<MyAction>(this);
    m_canbus_automation->add_action(m_canbus_action.get());
    pCanbus->add_trigger(m_canbus_trigger.get());
}

inline void DaikinRotexCanComponent::set_project_git_hash(text_sensor::TextSensor* pSensor, std::string const& hash) {
    m_project_git_hash_sensor = pSensor;
    m_project_git_hash = hash;
}

inline void DaikinRotexCanComponent::set_thermal_power_sensor(CanSensor* pSensor) {
    m_thermal_power_sensor = pSensor;
    pSensor->set_smooth(true);
}

inline void DaikinRotexCanComponent::set_thermal_power_sensor_raw(CanSensor* pSensor) {
    m_thermal_power_raw_sensor = pSensor;
}

inline void DaikinRotexCanComponent::set_temperature_spread(CanSensor* pSensor) {
    m_temperature_spread_sensor = pSensor;
    pSensor->set_smooth(true);
    pSensor->set_logging(true);
}

inline void DaikinRotexCanComponent::set_temperature_spread_raw(CanSensor* pSensor) {
    m_temperature_spread_raw_sensor = pSensor;
}

inline void DaikinRotexCanComponent::set_tv_tvbh_delta(CanSensor* pSensor) {
    m_tv_tvbh_delta_sensor = pSensor;
}

inline void DaikinRotexCanComponent::set_tvbh_tr_delta(CanSensor* pSensor) {
    m_tvbh_tr_delta_sensor = pSensor;
}

inline void DaikinRotexCanComponent::set_vorlauf_soll_tv_delta(CanSensor* pSensor) {
    m_vorlauf_soll_tv_delta = pSensor;
}

inline void DaikinRotexCanComponent::set_max_spread(float tvbh_tv, float tvbh_tr) {
    m_max_spread = { tvbh_tv, tvbh_tr };
}

inline void DaikinRotexCanComponent::set_tv_tvbh_tr_offset(float tv_offset, float tvbh_offset, float tr_offset) {
    m_tv_tvbh_tr_offset = { tv_offset, tvbh_offset, tr_offset };
}

inline void DaikinRotexCanComponent::add_entity(TEntity* pEntity) {
    m_entity_manager.add(pEntity);
}

inline void DaikinRotexCanComponent::set_supply_setpoint_regulated(number::Number* pNumber) {
    m_supply_setpoint_regulated = pNumber;
}

} // namespace daikin_rotex_can
} // namespace esphome