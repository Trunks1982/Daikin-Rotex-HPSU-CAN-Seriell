#pragma once

#include "esphome/components/daikin_rotex_can/sensors.h"
#include "esphome/components/daikin_rotex_can/entity.h"
#include <deque>

namespace esphome {
namespace daikin_rotex_can {

class TEntityManager {
public:
    TEntityManager();
    void add(TEntity* pRequest);

    void removeInvalidRequests();

    void setCanbus(esphome::canbus::Canbus* pCanbus);
    esphome::canbus::Canbus* getCanbus() const;

    uint32_t size() const;
    TEntity const* get(uint32_t index) const;
    TEntity* get(std::string const& id);
    TEntity const* get(std::string const& id) const;

    const std::vector<TEntity*>& get_entities() const { return m_entities; }
    void set_delay_between_requests(uint16_t milliseconds);

    CanSensor* get_sensor(std::string const& id);
    CanSensor const* get_sensor(std::string const& id) const;

    CanTextSensor* get_text_sensor(std::string const& id);
    CanTextSensor const* get_text_sensor(std::string const& id) const;

    CanBinarySensor const* get_binary_sensor(std::string const& id) const;

    CanNumber const* get_number(std::string const& id, bool log_missing = true) const;

    CanSelect* get_select(std::string const& id);
    CanSelect const* get_select(std::string const& id) const;

    bool is_allowed_by_delay_between_requests() const;

    bool sendNextPendingRequest();
    // value in entity units, scaled by the entity's divider before sending
    bool sendSet(std::string const& request_name, float value);
    // raw value as sent on the bus (e.g. a select map key), not scaled
    bool sendSetRaw(std::string const& request_name, uint16_t raw);
    void handle(uint32_t can_id, TMessage const& responseData);

private:
    struct SetRequest {
        std::string name;
        float value;
        bool raw;
    };

    TEntity* getNextGetRequestToSend();
    bool sendSet(std::string const& request_name, float value, bool raw);

    std::vector<TEntity*> m_entities;
    esphome::canbus::Canbus* m_pCanbus;
    uint32_t m_last_handle;
    uint16_t m_delay_between_requests;
    std::deque<SetRequest> m_set_request_queue;
};

inline void TEntityManager::setCanbus(esphome::canbus::Canbus* pCanbus) {
    m_pCanbus = pCanbus;
}

inline esphome::canbus::Canbus* TEntityManager::getCanbus() const {
    return m_pCanbus;
}

inline uint32_t TEntityManager::size() const {
    return m_entities.size();
}

inline TEntity const* TEntityManager::get(uint32_t index) const {
    return (index < m_entities.size()) ? m_entities[index] : nullptr;
}

inline void TEntityManager::set_delay_between_requests(uint16_t milliseconds) {
    m_delay_between_requests = milliseconds;
}

inline bool TEntityManager::is_allowed_by_delay_between_requests() const {
    const uint32_t now = esphome::millis();
    return now >= (m_last_handle + m_delay_between_requests);
}

}
}