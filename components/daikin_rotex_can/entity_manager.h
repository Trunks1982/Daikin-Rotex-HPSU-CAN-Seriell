#pragma once

#include "esphome/components/daikin_rotex_can/sensors.h"
#include "esphome/components/daikin_rotex_can/entity.h"

namespace esphome {
namespace daikin_rotex_can {

class TEntityManager {
public:
    TEntityManager();
    void add(TEntity* pRequest);

    void removeInvalidRequests();

    void setCanbus(esphome::esp32_can::ESP32Can* pCanbus);
    esphome::esp32_can::ESP32Can* getCanbus() const;

    uint32_t size() const;
    TEntity const* get(uint32_t index) const;
    TEntity const* get(std::string const& id) const;

    EntityBase* get_entity_base(std::string const& id);
    EntityBase const* get_entity_base(std::string const& id) const;

    const std::vector<TEntity*>& get_entities() const { return m_entities; }

    CanSensor* get_sensor(std::string const& id);
    CanSensor const* get_sensor(std::string const& id) const;

    CanTextSensor* get_text_sensor(std::string const& id);
    CanTextSensor const* get_text_sensor(std::string const& id) const;

    CanBinarySensor const* get_binary_sensor(std::string const& id) const;

    CanSelect* get_select(std::string const& id);

    bool sendNextPendingGet();
    void sendSet(std::string const& request_name, float value);
    void handle(uint32_t can_id, TMessage const& responseData);

private:
    TEntity* getNextRequestToSend();

    std::vector<TEntity*> m_entities;
    esphome::esp32_can::ESP32Can* m_pCanbus;
};

inline void TEntityManager::setCanbus(esphome::esp32_can::ESP32Can* pCanbus) {
    m_pCanbus = pCanbus;
}

inline esphome::esp32_can::ESP32Can* TEntityManager::getCanbus() const {
    return m_pCanbus;
}

inline uint32_t TEntityManager::size() const {
    return m_entities.size();
}

inline TEntity const* TEntityManager::get(uint32_t index) const {
    return (index < m_entities.size()) ? m_entities[index] : nullptr;
}

}
}