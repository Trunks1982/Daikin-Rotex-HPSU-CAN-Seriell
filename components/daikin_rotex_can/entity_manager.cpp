#include "esphome/components/daikin_rotex_can/entity_manager.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "daikin_rotex_can";

TEntityManager::TEntityManager()
: m_entities()
, m_pCanbus(nullptr)
, m_last_handle(0u)
, m_delay_between_requests(250)
, m_set_request_queue()
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

CanSensor* TEntityManager::get_sensor(std::string const& id) {
    TEntity* pEntity = get(id);
    if (pEntity == nullptr) {
        ESP_LOGE(TAG, "Entity not found: %s", id.c_str());
        return nullptr;
    }

    if (CanSensor* pSensor = pEntity->asSensor()) {
        return pSensor;
    } else {
        ESP_LOGE(TAG, "Entity is not a sensor: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

CanSensor const* TEntityManager::get_sensor(std::string const& id) const {
    TEntity const* pEntity = get(id);
    if (pEntity == nullptr) {
        ESP_LOGE(TAG, "Const Entity not found: %s", id.c_str());
        return nullptr;
    }

    if (CanSensor const* pSensor = pEntity->asSensor()) {
        return pSensor;
    } else {
        ESP_LOGE(TAG, "Const Entity is not a sensor: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

CanTextSensor* TEntityManager::get_text_sensor(std::string const& id) {
    TEntity* pEntity = get(id);
    if (pEntity == nullptr) {
        ESP_LOGE(TAG, "get_text_sensor() => Entity not found: %s", id.c_str());
        return nullptr;
    }

    if (CanTextSensor* pTextSensor = pEntity->asTextSensor()) {
        return pTextSensor;
    } else {
        ESP_LOGE(TAG, "Entity is not a text sensor: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

CanTextSensor const* TEntityManager::get_text_sensor(std::string const& id) const {
    TEntity const* pEntity = get(id);
    if (pEntity == nullptr) {
        ESP_LOGE(TAG, "const get_text_sensor() => Entity not found: %s", id.c_str());
        return nullptr;
    }

    if (CanTextSensor const* pTextSensor = pEntity->asTextSensor()) {
        return pTextSensor;
    } else {
        ESP_LOGE(TAG, "Entity is not a text sensor: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

CanBinarySensor const* TEntityManager::get_binary_sensor(std::string const& id) const {
    TEntity const* pEntity = get(id);
    if (pEntity == nullptr) {
        ESP_LOGE(TAG, "const get_binary_sensor() => Entity not found: %s", id.c_str());
        return nullptr;
    }

    if (CanBinarySensor const* pBinarySensor = pEntity->asBinarySensor()) {
        return pBinarySensor;
    } else {
        ESP_LOGE(TAG, "Entity is not a binary sensor: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

CanNumber const* TEntityManager::get_number(std::string const& id, bool log_missing) const {
    TEntity const *pEntity = get(id);
    if (pEntity == nullptr) {
        if (log_missing) {
            ESP_LOGE(TAG, "const get_number() => Entity not found: %s", id.c_str());
        }
        return nullptr;
    }

    if (CanNumber const* pNumber = pEntity->asNumber()) {
        return pNumber;
    } else if (log_missing) {
        ESP_LOGE(TAG, "Entity is not a number: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

CanSelect* TEntityManager::get_select(std::string const& id) {
    TEntity *pEntity = get(id);
    if (pEntity == nullptr) {
        ESP_LOGE(TAG, "toSelect() => Entity not found: %s", id.c_str());
        return nullptr;
    }

    if (CanSelect* pSelect = pEntity->asSelect()) {
        return pSelect;
    } else {
        ESP_LOGE(TAG, "Entity is not a select: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

CanSelect const* TEntityManager::get_select(std::string const& id) const {
    TEntity const *pEntity = get(id);
    if (pEntity == nullptr) {
        ESP_LOGE(TAG, "const get_select() => Entity not found: %s", id.c_str());
        return nullptr;
    }

    if (CanSelect const* pSelect = pEntity->asSelect()) {
        return pSelect;
    } else {
        ESP_LOGE(TAG, "Entity is not a select: %s", pEntity->getName().c_str());
    }
    return nullptr;
}

bool TEntityManager::sendNextPendingRequest() {
    if (m_set_request_queue.empty() == false && is_allowed_by_delay_between_requests()) {
        auto request = m_set_request_queue.front();
        m_set_request_queue.pop_front();
        ESP_LOGI(TAG, "sendNextPendingRequest: Process queued set request => name: %s, value: %f", request.first.c_str(), request.second);
        return sendSet(request.first, request.second);
    }

    TEntity* pEntity = getNextGetRequestToSend();
    if (pEntity != nullptr) {
        return pEntity->sendGet(m_pCanbus);
    }
    return false;
}

bool TEntityManager::sendSet(std::string const& request_name, float value) {
    const uint32_t now = esphome::millis();

    if (!is_allowed_by_delay_between_requests()) {
        ESP_LOGE(TAG, "sendSet: Add to queue => name: %s, value: %f", request_name.c_str(), value);
        m_set_request_queue.push_back(std::make_pair(request_name, value));
        return false;
    }

    const auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [&request_name](auto pEntity) { return pEntity->getName() == request_name; }
    );
    bool result = false;
    if (it != m_entities.end()) {
        result = (*it)->sendSet(m_pCanbus, value * (*it)->get_config().divider);
        m_last_handle = esphome::millis();
    } else {
        ESP_LOGE(TAG, "sendSet: Unknown request: %s", request_name.c_str());
    }
    return result;
}

void TEntityManager::handle(uint32_t can_id, TMessage const& responseData) {
    bool bHandled = false;
    for (auto pEntity : m_entities) {
        if (pEntity->handle(can_id, responseData)) {
            bHandled = true;
            break;
        }
    }
    m_last_handle = esphome::millis();
    if (!bHandled) {
        Utils::log("unhandled", "can_id<%s> data<%s>", Utils::to_hex(can_id).c_str(), Utils::to_hex(responseData).c_str());
    }
}

TEntity* TEntityManager::get(std::string const& id) {
    for (auto pEntity: m_entities) {
        if (pEntity->get_id() == id) {
            return pEntity;
        }
    }
    return nullptr;
}

TEntity const* TEntityManager::get(std::string const& id) const {
    for (auto pEntity: m_entities) {
        if (pEntity->get_id() == id) {
            return pEntity;
        }
    }
    return nullptr;
}

TEntity* TEntityManager::getNextGetRequestToSend() {
    const uint32_t now = esphome::millis();

    if (!is_allowed_by_delay_between_requests()) {
        return nullptr;
    }

    for (auto pEntity : m_entities) {
        if (pEntity->isGetInProgress()) {
            return nullptr;
        }
    }

    TEntity* pNext = nullptr;
    for (auto pEntity : m_entities) {
        if (pEntity->isGetNeeded()) {
            if (pNext == nullptr || pEntity->getOverdueTime() > pNext->getOverdueTime()) {
                pNext = pEntity;
            }
        }
    }
    return pNext;
}


}
}