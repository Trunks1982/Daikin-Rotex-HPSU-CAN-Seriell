#pragma once

#include "esphome/components/daikin_rotex_can/types.h"
#include <functional>
#include <memory>
#include <map>

namespace esphome {
namespace daikin_rotex_can {

class Utils {
    using TVoidFunc = std::function<void()>;
public:
    template<typename... Args>
    static std::string format(const std::string& format, Args... args) {
        const auto size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        const auto buffer = std::make_unique<char[]>(size);

        std::snprintf(buffer.get(), size, format.c_str(), args...);

        return std::string(buffer.get(), buffer.get() + size - 1);
    }

    template<typename First, typename ... T>
    static bool is_in(First &&first, T && ... t)
    {
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
    static void log(std::string const& tag, std::string const& str_format, Args... args);

    static std::string g_log_filter;
};

}
}