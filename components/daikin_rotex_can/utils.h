#pragma once

#include "esphome/components/daikin_rotex_can/types.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/hal.h"
#include "esphome/core/version.h"
#include <functional>
#include <memory>
#include <map>

namespace esphome {
namespace daikin_rotex_can {

class Utils {
    friend class LogFilterText;

    using TVoidFunc = std::function<void()>;
public:
    /// Name of an entity. EntityBase::get_name() is deprecated from ESPHome 2026.11, its replacement
    /// write_name_to() does not exist before that version.
    static std::string entity_name(EntityBase const& entity) {
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 11, 0)
        char buffer[ENTITY_NAME_BUF_SIZE];
        return std::string(buffer, entity.write_name_to(buffer, sizeof(buffer)));
#else
        return entity.get_name().str();
#endif
    }

    template<typename... Args>
    static std::string format(const std::string& format, Args... args) {
        const auto size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        const auto buffer = std::make_unique<char[]>(size);

        std::snprintf(buffer.get(), size, format.c_str(), args...);

        return std::string(buffer.get(), buffer.get() + size - 1);
    }

    template<typename First, typename ... T>
    static bool is_in(First &&first, T && ... t) {
        return ((first == t) || ...);
    }

    static std::string to_hex(TMessage const& data);
    static bool find(std::string const& haystack, std::string const& needle);
    static std::vector<std::string> split(std::string const& str);
    static std::string to_hex(uint32_t value);
    static TMessage str_to_bytes(const std::string& str);
    static TMessage str_to_bytes_array8(const std::string& str);
    static std::map<uint16_t, std::string> str_to_map(const std::string& input);
    static uint16_t hex_to_uint16(const std::string& hexStr);
    static void setBytes(TMessage& data, uint16_t value, uint8_t offset, uint8_t len);

    template<typename... Args>
    static void log(std::string const& tag, std::string const& str_format, Args... args) {
        log_impl(tag, Utils::format(str_format, args...));
    }

    static bool equals(float a, float b, float epsilon);

    /// Milliseconds since `since` (an earlier esphome::millis() value). The unsigned subtraction stays
    /// correct across the millis() overflow after 49.7 days, `millis() > since + interval` does not.
    static uint32_t millis_since(uint32_t since) { return esphome::millis() - since; }

private:
    static void log_impl(std::string const& tag, std::string const& formatted);

    static std::string g_log_filter;
};

inline bool Utils::equals(float a, float b, float epsilon) {
    return std::abs(a - b) <= epsilon;
}

}
}