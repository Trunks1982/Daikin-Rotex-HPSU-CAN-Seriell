#include "esphome/components/daikin_rotex_can/entity_manager.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "daikin_rotex_can";

TEntityManager::TEntityManager()
: m_entities()
, m_pCanbus(nullptr)
{
}

void TEntityManager::add(TEntity* pEntity) {
    m_entities.push_back(pEntity);
}

void TEntityManager::removeInvalidRequests() {
    m_entities.erase(
        std::remove_if(
            m_entities.begin(),
            m_entities.end(),
            [](TEntity* pEntity) { return !pEntity->isGetSupported(); }
        ),
        m_entities.end()
    );
}

EntityBase* TEntityManager::get_entity_base(std::string const& id) {
    TEntity const* pEntity = get(id);
    if (pEntity != nullptr) {
        return pEntity->get_entity_base();
    } else {
        ESP_LOGE(TAG, "get_entity_base: Entity not found: %s", id.c_str());
    }
    return nullptr;
}

EntityBase const* TEntityManager::get_entity_base(std::string const& id) const {
    TEntity const* pEntity = get(id);
    if (pEntity != nullptr) {
        return pEntity->get_entity_base();
    } else {
        ESP_LOGE(TAG, "get_entity_base: Entity not found: %s", id.c_str());
    }
    return nullptr;
}

CanSensor* TEntityManager::get_sensor(std::string const& id) {
    EntityBase* pEntity = get_entity_base(id);
    if (CanSensor* pSensor = dynamic_cast<CanSensor*>(pEntity)) {
        return pSensor;
    } else if (pEntity) {
        ESP_LOGE(TAG, "Entity is not a sensor: %s", pEntity->get_name().c_str());
    } else {
        ESP_LOGE(TAG, "Entity is null!");
    }
    return nullptr;
}

CanSensor const* TEntityManager::get_sensor(std::string const& id) const {
    EntityBase const* pEntity = get_entity_base(id);
    if (CanSensor const* pSensor = dynamic_cast<CanSensor const*>(pEntity)) {
        return pSensor;
    } else if (pEntity) {
        ESP_LOGE(TAG, "Const Entity is not a sensor: %s", pEntity->get_name().c_str());
    } else {
        ESP_LOGE(TAG, "Const Entity is null!");
    }
    return nullptr;
}

CanTextSensor* TEntityManager::get_text_sensor(std::string const& id) {
    EntityBase* pEntity = get_entity_base(id);
    if (CanTextSensor* pTextSensor = dynamic_cast<CanTextSensor*>(pEntity)) {
        return pTextSensor;
    } else if (pEntity) {
        ESP_LOGE(TAG, "Entity is not a text sensor: %s", pEntity->get_name().c_str());
    } else {
        ESP_LOGE(TAG, "get_text_sensor() => Entity is null!");
    }
    return nullptr;
}

CanTextSensor const* TEntityManager::get_text_sensor(std::string const& id) const {
    EntityBase const* pEntity = get_entity_base(id);
    if (CanTextSensor const* pTextSensor = dynamic_cast<CanTextSensor const*>(pEntity)) {
        return pTextSensor;
    } else if (pEntity) {
        ESP_LOGE(TAG, "Entity is not a text sensor: %s", pEntity->get_name().c_str());
    } else {
        ESP_LOGE(TAG, "const get_text_sensor() => Entity is null!");
    }
    return nullptr;
}

CanBinarySensor const* TEntityManager::get_binary_sensor(std::string const& id) const {
    EntityBase const* pEntity = get_entity_base(id);
    if (CanBinarySensor const* pBinarySensor = dynamic_cast<CanBinarySensor const*>(pEntity)) {
        return pBinarySensor;
    } else if (pEntity) {
        ESP_LOGE(TAG, "Entity is not a binary sensor: %s", pEntity->get_name().c_str());
    } else {
        ESP_LOGE(TAG, "const get_binary_sensor() => Entity is null!");
    }
    return nullptr;
}

CanSelect* TEntityManager::get_select(std::string const& id) {
    EntityBase* pEntity = get_entity_base(id);
    if (CanSelect* pSelect = dynamic_cast<CanSelect*>(pEntity)) {
        return pSelect;
    } else if (pEntity) {
        ESP_LOGE(TAG, "Entity is not a select: %s", pEntity->get_name().c_str());
    } else {
        ESP_LOGE(TAG, "toSelect() => Entity is null!");
    }
    return nullptr;
}

bool TEntityManager::sendNextPendingGet() {
    TEntity* pEntity = getNextRequestToSend();
    if (pEntity != nullptr) {
        return pEntity->sendGet(m_pCanbus);
    }
    return false;
}

void TEntityManager::sendSet(std::string const& request_name, float value) {
    const auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [&request_name](auto pEntity) { return pEntity->getName() == request_name; }
    );
    if (it != m_entities.end()) {
        (*it)->sendSet(m_pCanbus, value);
    } else {
        ESP_LOGE(TAG, "sendSet: Unknown request: %s", request_name.c_str());
    }
}

void TEntityManager::handle(uint32_t can_id, TMessage const& responseData) {
    bool bHandled = false;
    for (auto pEntity : m_entities) {
        if (pEntity->handle(can_id, responseData)) {
            bHandled = true;
            break;
        }
    }
    if (!bHandled) {
        Utils::log("unhandled", "can_id<%s> data<%s>", Utils::to_hex(can_id).c_str(), Utils::to_hex(responseData).c_str());
    }
}

TEntity const* TEntityManager::get(std::string const& id) const {
    for (auto pEntity: m_entities) {
        if (pEntity->get_id() == id) {
            return pEntity;
        }
    }
    return nullptr;
}

TEntity* TEntityManager::getNextRequestToSend() {
    const uint32_t timestamp = millis();

    for (auto pEntity : m_entities) {
        if (pEntity->is_command_set() && pEntity->isGetInProgress()) {
            return nullptr;
        }
    }

    for (auto pEntity : m_entities) {
        if (pEntity->is_command_set() && pEntity->isGetNeeded()) {
            return pEntity;
        }
    }
    return nullptr;
}


}
}