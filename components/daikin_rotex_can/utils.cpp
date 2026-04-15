#include "esphome/components/daikin_rotex_can/utils.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <iomanip>
//#include <cctype>

namespace esphome {
namespace daikin_rotex_can {

std::string Utils::g_log_filter = "";
static const char* TAG = "Utils";

bool Utils::find(std::string const& haystack, std::string const& needle) {
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return (it != haystack.end());
}

std::vector<std::string> Utils::split(std::string const& str) {
    std::string segment;
    std::istringstream iss(str);
    std::vector<std::string> result;

    while (std::getline(iss, segment, '|')) {
        if (!segment.empty()) {
            result.push_back(segment);
        }
    }

    return result;
}

std::string Utils::to_hex(uint32_t value) {
    char hex_string[20];
    sprintf(hex_string, "0x%02X", value);
    return std::string(hex_string);
}

std::string Utils::to_hex(TMessage const& data) {
    std::stringstream str;
    str.setf(std::ios_base::hex, std::ios::basefield);
    str.setf(std::ios_base::uppercase);
    str.fill('0');

    bool first = true;
    for (uint8_t chr : data)
    {
        if (first) {
            first = false;
        } else {
            str << " ";
        }
        str << std::setw(2) << (unsigned short)(std::byte)chr;
    }
    return str.str();
}

TMessage Utils::str_to_bytes(const std::string& str) {
    TMessage bytes = {0};
    std::stringstream ss(str);
    std::string byteStr;

    uint8_t index = 0;
    while (ss >> byteStr) {
        const uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        bytes[index++] = byte;
    }

    return bytes;
}

TMessage Utils::str_to_bytes_array8(const std::string& str) {
    TMessage byte_array{};

    std::string cleaned_str;
    bool prev_space = false;
    for (char c : str) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            cleaned_str += c;
            prev_space = false;
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!prev_space) {
                cleaned_str += ' ';
                prev_space = true;
            }
        }
    }
    if (!cleaned_str.empty() && cleaned_str.back() == ' ') {
        cleaned_str.pop_back();
    }

    std::stringstream ss(cleaned_str);
    std::string byte_str;
    size_t index = 0;

    while (ss >> byte_str && index < byte_array.size()) {
        byte_array[index++] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
    }

    return byte_array;
}

std::map<uint16_t, std::string> Utils::str_to_map(const std::string& input) {
    std::map<uint16_t, std::string> result;
    std::stringstream ss(input);
    std::string pair;

    while (std::getline(ss, pair, '|')) {
        size_t pos = pair.find(':');
        if (pos != std::string::npos) {
            std::string keyStr = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);

            uint16_t key = static_cast<uint16_t>(std::strtoul(keyStr.c_str(), nullptr, 16));

            result[key] = value;
        }
    }

    return result;
}

uint16_t Utils::hex_to_uint16(const std::string& hexStr) {
    uint16_t result;
    std::stringstream ss;

    ss << std::hex << hexStr;
    ss >> result;

    return result;
}

void Utils::setBytes(TMessage& data, uint16_t value, uint8_t offset, uint8_t len) {
    if (len == 1) {
        data[offset] = value & 0xFF;
    } else if (len == 2) {
        data[offset] = (value >> 8) & 0xFF;
        data[offset + 1] = value & 0xFF;
    } else {
        ESP_LOGE("write", "Invalid len: %d", len);
    }
}

void Utils::log_impl(std::string const& tag, std::string const& formatted) {
    const std::string log_filter = g_log_filter;
    bool found = log_filter.empty();
    if (!found) {
        for (auto segment : Utils::split(log_filter)) {
            if (Utils::find(tag, segment) || Utils::find(formatted, segment)) {
                found = true;
                break;
            }
        }
    }
    if (found) {
        std::string final_log = Utils::format("millis: %d|", esphome::millis()) + formatted;
        ESP_LOGI(tag.c_str(), "%s", final_log.c_str());
    }
}

}
}