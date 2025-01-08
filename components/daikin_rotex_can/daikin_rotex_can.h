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

class DaikinRotexCanComponent: public Component, public SensorAccessor {
    struct MaxSpread {
        float tvbh_tv;
        float tvbh_tr;
    };
public:
    using TVoidFunc = std::function<void()>;

    DaikinRotexCanComponent();
    void setup() override;
    void loop() override;
    void dump_config() override;

    void set_canbus(esphome::esp32_can::ESP32Can* pCanbus);
    void set_update_interval(uint16_t seconds) {} // dummy
    void set_project_git_hash(text_sensor::TextSensor* pSensor, std::string const& hash) { m_project_git_hash_sensor = pSensor; m_project_git_hash = hash; }
    void set_thermal_power_sensor(sensor::Sensor* pSensor) { m_thermal_power_sensor = pSensor; }
    void set_thermal_power_sensor_smooth(sensor::Sensor* pSensor) { m_thermal_power_smooth_sensor = pSensor; }
    void set_max_spread(float tvbh_tv, float tvbh_tr) { m_max_spread = { tvbh_tv, tvbh_tr };}
    void add_entity(EntityBase* pEntity) {
        if (TEntity* pRequest = dynamic_cast<TEntity*>(pEntity)) {
            m_entity_manager.add(pRequest);
        }
    }

    void on_post_handle(TEntity* pRequest, TEntity::TVariant const& current, TEntity::TVariant const& previous);

    // Texts
    virtual void custom_request(std::string const& value) override;

    // Buttons
    virtual void dhw_run() override;
    virtual void dump() override;

    void handle(uint32_t can_id, std::vector<uint8_t> const& data);

    void run_dhw_lambdas();
    void call_later(TVoidFunc lambda, uint32_t timeout = 0u) {
        const uint32_t timestamp = millis();
        m_later_calls.push_back({lambda, timestamp + timeout});
    }

    void update_thermal_power();

private:

    using TCanbusAutomation = esphome::Automation<std::vector<uint8_t>, uint32_t, bool>;
    using TCanbusAction = esphome::Action<std::vector<uint8_t>, uint32_t, bool>;

    class MyAction : public TCanbusAction {
    public:
        MyAction(DaikinRotexCanComponent* pParent): m_pParent(pParent) {}
    protected:
        virtual void play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) override {
            m_pParent->handle(can_id, data);
        }
    private:
        DaikinRotexCanComponent* m_pParent;
    };

    void updateState(std::string const& id);
    bool on_custom_select(std::string const& id, uint8_t value);
    void on_betriebsart(TEntity::TVariant const& current, TEntity::TVariant const& previous);

    float getSensorState(std::string const& name);
    void throwPeriodicError(std::string const& message);
    bool is_command_set(TMessage const&);
    std::string recalculate_state(EntityBase* pEntity, std::string const& new_state) const;

    esphome::daikin_rotex_can::TEntityManager m_entity_manager;
    std::shared_ptr<esphome::canbus::CanbusTrigger> m_canbus_trigger;
    std::shared_ptr<TCanbusAutomation> m_canbus_automation;
    std::shared_ptr<MyAction> m_canbus_action;
    std::list<std::pair<TVoidFunc, uint32_t>> m_later_calls;
    std::list<TVoidFunc> m_dhw_run_lambdas;

    PersistentValue<bool> m_optimized_defrosting;
    text_sensor::TextSensor* m_project_git_hash_sensor;
    std::string m_project_git_hash;
    esphome::esp32_can::ESP32Can* m_pCanbus;

    sensor::Sensor* m_thermal_power_sensor;
    sensor::Sensor* m_thermal_power_smooth_sensor;
    MaxSpread m_max_spread;
    PID m_pid;
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

} // namespace daikin_rotex_can
} // namespace esphome