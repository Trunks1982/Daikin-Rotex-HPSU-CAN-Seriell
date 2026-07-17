#pragma once

#include "esphome/components/daikin_rotex_can/accessor.h"
#include "esphome/components/daikin_rotex_can/types.h"
#include "esphome/components/daikin_rotex_can/utils.h"
#include "esphome/components/daikin_rotex_can/entity_type.h"
#include "esphome/components/esp32_can/esp32_can.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/hal.h"
#include <functional>
#include <stdint.h>
#include <variant>
#include <list>

namespace esphome {
namespace daikin_rotex_can {

class CanSensor;
class CanTextSensor;
class CanBinarySensor;
class CanNumber;
class CanSelect;
class CanSwitch;

class TEntity {
    static const uint16_t DC = 0xFFFF; // Don't care

public:
    using THandleFunc = std::function<uint16_t(TMessage const&)>;
    using TUpdateFunc = std::function<std::string(IAccessor const&)>;
    using TSetFunc = std::function<void(TMessage&, uint16_t)>;
    using TVariant = std::variant<uint32_t, uint8_t, float, bool, std::string>;
    using TPostHandleLabda = std::function<void(TEntity*, TEntity::TVariant const&, TEntity::TVariant const&)>;

    struct TEntityArguments {
        EntityBase* pEntity;
        EntityType entity_type;
        std::string id;
        uint16_t can_id;
        TMessage command;
        uint8_t data_offset;
        uint8_t data_size;
        float divider;
        bool isSigned;
        std::list<std::string> update_entities;
        uint32_t update_interval;
        THandleFunc handle_lambda;
        TUpdateFunc update_lambda;
        TSetFunc set_lambda;
        bool handle_lambda_set;
        bool update_lambda_set;
        bool set_lambda_set;

        TEntityArguments()
        : pEntity(nullptr)
        , entity_type(EntityType::UNKNOWN)
        , id("")
        , can_id(0x0)
        , command({})
        , data_offset(0)
        , data_size(0)
        , divider(1)
        , isSigned(false)
        , update_entities({})
        , update_interval(1000)
        , handle_lambda([](TMessage const&){ return 0; })
        , update_lambda([](IAccessor const&){ return ""; })
        , set_lambda([](TMessage&, uint16_t){})
        , handle_lambda_set(false)
        , update_lambda_set(false)
        , set_lambda_set(false)
        {
        }

        TEntityArguments(
            EntityBase* _pEntity,
            EntityType _entity_type,
            std::string const& _id,
            uint16_t _can_id,
            std::string const& _command,
            uint8_t _data_offset,
            uint8_t _data_size,
            float _divider,
            bool _isSigned,
            std::list<std::string> const& _update_entities,
            uint32_t _update_interval,
            THandleFunc _handle_lambda,
            TUpdateFunc _update_lambda,
            TSetFunc _set_lambda,
            bool _handle_lambda_set,
            bool _update_lambda_set,
            bool _set_lambda_set
        )
        : pEntity(_pEntity)
        , entity_type(_entity_type)
        , id(_id)
        , can_id(_can_id)
        , command(Utils::str_to_bytes_array8(_command))
        , data_offset(_data_offset)
        , data_size(_data_size)
        , divider(_divider)
        , isSigned(_isSigned)
        , update_entities(_update_entities)
        , update_interval(_update_interval)
        , handle_lambda(_handle_lambda)
        , update_lambda(_update_lambda)
        , set_lambda(_set_lambda)
        , handle_lambda_set(_handle_lambda_set)
        , update_lambda_set(_update_lambda_set)
        , set_lambda_set(_set_lambda_set)
        {}
    };

public:
    TEntity();

    std::string const& get_id() const { return m_config.id; }
    void set_id(std::string const& id) { m_config.id = id; }

    EntityType getType() const { return m_config.pEntity != nullptr ? m_config.entity_type : EntityType::UNKNOWN; }

    std::string getName() const {
        return m_config.pEntity != nullptr ? m_config.pEntity->get_name().str() : "<INVALID>";
    }

    bool isGetSupported() const {
        return m_config.pEntity != nullptr;
    }

    uint32_t getLastUpdate() const {
        return m_last_handle_timestamp;
    }

    uint32_t getLastValueChange() const {
        return m_last_value_change_timestamp;
    }

    bool isChangedInLastMilliseconds(uint32_t milliseconds) const {
        return esphome::millis() > (getLastValueChange() + milliseconds);
    }

    void set_canbus(esphome::esp32_can::ESP32Can* pCanbus) {
        m_pCanbus = pCanbus;
    }

    void set_entity(std::string const& name, TEntityArguments&& arg, IAccessor const* accessor) {
        m_config = std::move(arg);
        m_expected_reponse = TEntity::calculate_reponse(m_config.command);
        m_pAccessor = accessor;
    }

    void set_post_handle(TPostHandleLabda&& func) {
        m_post_handle_lambda = std::move(func);
    }

    std::list<std::string> const& get_update_entities() {
        return m_config.update_entities;
    }

    TEntityArguments const& get_config() const {
        return m_config;
    }

    virtual void update(uint32_t millis);

    bool isMatch(uint32_t can_id, TMessage const& responseData) const;
    bool handle(uint32_t can_id, TMessage const& responseData);

    bool sendGet(esphome::esp32_can::ESP32Can* pCanBus);
    bool sendSet(esphome::esp32_can::ESP32Can* pCanBus, float value);

    bool isGetNeeded() const;
    uint32_t getOverdueTime() const;
    bool is_command_configured() const;

    bool isGetInProgress() const;
    uint32_t get_update_interval() const { return m_config.update_interval; }

    CanSensor* asSensor();
    CanSensor const* asSensor() const;

    CanTextSensor* asTextSensor();
    CanTextSensor const* asTextSensor() const;

    CanBinarySensor* asBinarySensor();
    CanBinarySensor const* asBinarySensor() const;

    CanNumber* asNumber();
    CanNumber const* asNumber() const;

    CanSelect* asSelect();
    CanSelect const* asSelect() const;

    CanSwitch* asSwitch();
    CanSwitch const* asSwitch() const;

    static std::array<uint16_t, 7> calculate_reponse(TMessage const& message);

    std::string string() {
        return Utils::format(
            "TEntity<name: %s, command: %s>",
            getName().c_str(),
            Utils::to_hex(m_config.command).c_str()
        );
    }

protected:
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) = 0;

protected:
    TEntityArguments m_config;
    esphome::esp32_can::ESP32Can* m_pCanbus;

private:
    IAccessor const* m_pAccessor;
    std::array<uint16_t, 7> m_expected_reponse;
    uint32_t m_last_handle_timestamp;
    uint32_t m_last_get_timestamp;
    uint32_t m_last_value_change_timestamp;
    TPostHandleLabda m_post_handle_lambda;
};

inline bool TEntity::isGetNeeded() const {
    if (!is_command_configured()) {
        return false;
    }

    const uint32_t update_interval = get_update_interval();
    return getLastUpdate() == 0 || (esphome::millis() > (getLastUpdate() + update_interval));
}

inline uint32_t TEntity::getOverdueTime() const {
    if (!is_command_configured()) {
        return 0u;
    }

    const uint32_t update_interval = get_update_interval();
    const uint32_t due_time = getLastUpdate() + update_interval;
    const uint32_t now = esphome::millis();

    if (now > due_time) {
        return now - due_time;
    }
    return 0u;
}

inline bool TEntity::is_command_configured() const {
    for (auto& b : m_config.command) {
        if (b != 0x00) {
            return true;
        }
    }
    return false;
}

}
}

