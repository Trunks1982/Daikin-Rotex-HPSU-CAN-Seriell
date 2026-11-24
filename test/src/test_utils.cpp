#include "esphome/components/daikin_rotex_can/utils.h"
#include "mock_esphome.h"
#include <gtest/gtest.h>

using namespace esphome::daikin_rotex_can;

TEST(UtilsTest, find) {
    EXPECT_TRUE(Utils::find("ab|cde|fg|h|", "ab"));
    EXPECT_TRUE(Utils::find("ab|cde|fg|h|", "cd"));
    EXPECT_TRUE(Utils::find("ab|cde|fg|h|", "cde"));
    EXPECT_TRUE(Utils::find("ab|cde|fg|h|", "fg"));
    EXPECT_TRUE(Utils::find("ab|cde|fg|h|", "h"));
    EXPECT_TRUE(Utils::find("ab|cde|fg|h|", "cde|"));
    EXPECT_TRUE(Utils::find("ab|cde|fg|h|", "cde|fg"));
    EXPECT_FALSE(Utils::find("ab|cde|fg|h|", "cdef"));
}

TEST(UtilsTest, split) {
    auto vec = Utils::split("ab|cde|fg|h|");
    EXPECT_EQ(4, vec.size());
    EXPECT_EQ("ab", vec[0]);
    EXPECT_EQ("cde", vec[1]);
    EXPECT_EQ("fg", vec[2]);
    EXPECT_EQ("h", vec[3]);
}

TEST(UtilsTest, to_hex) {
    EXPECT_EQ("0x01", Utils::to_hex(1));
    EXPECT_EQ("0x05", Utils::to_hex(5));
    EXPECT_EQ("0x09", Utils::to_hex(9));
    EXPECT_EQ("0x0A", Utils::to_hex(10));
    EXPECT_EQ("0x0F", Utils::to_hex(15));
    EXPECT_EQ("0x10", Utils::to_hex(16));
    EXPECT_EQ("0xFF", Utils::to_hex(255));
}

TEST(UtilsTest, to_hex_message) {
    EXPECT_EQ("01 05 09 0C 1B 38 93", Utils::to_hex({1, 5, 9, 12, 27, 56, 147}));
    EXPECT_EQ("00 23 4D 58 63 DE FF", Utils::to_hex({0, 35, 77, 88, 99, 222, 255}));
}

TEST(UtilsTest, str_to_bytes_array8) {
    EXPECT_EQ(TMessage({1, 5, 9, 12, 27, 56, 147}), Utils::str_to_bytes_array8("01 05 09 0C 1B 38 93"));
    EXPECT_EQ(TMessage({0, 35, 77, 88, 99, 222, 255}), Utils::str_to_bytes_array8("00 23 4D 58 63 DE FF"));
}

TEST(UtilsTest, str_to_map) {
    auto map = Utils::str_to_map("0x01:A|0x05:B|0x09:CD|0x0C:Str1|0x1B:str2|0x38:str3|0x93:str 4");
    EXPECT_EQ("A", map.find(0x01)->second);
    EXPECT_EQ("B", map.find(0x05)->second);
    EXPECT_EQ("CD", map.find(0x09)->second);
    EXPECT_EQ("Str1", map.find(0x0C)->second);
    EXPECT_EQ("str2", map.find(0x1B)->second);
    EXPECT_EQ("str3", map.find(0x38)->second);
    EXPECT_EQ("str 4", map.find(0x93)->second);
    EXPECT_TRUE(map.find(0x94) == map.end());
}

TEST(UtilsTest, hex_to_uint16) {
    EXPECT_EQ(1, Utils::hex_to_uint16("01"));
    EXPECT_EQ(5, Utils::hex_to_uint16("0x05"));
    EXPECT_EQ(167, Utils::hex_to_uint16("A7"));
    EXPECT_EQ(43780, Utils::hex_to_uint16("0xAB04"));
}

TEST(UtilsTest, setBytes) {
    TMessage msg = {};
    Utils::setBytes(msg, 7, 0, 1);
    Utils::setBytes(msg, 0x12AF, 1, 2);
    Utils::setBytes(msg, 0x07C1, 4, 2);

    EXPECT_EQ("07 12 AF 00 07 C1 00", Utils::to_hex(msg));
}

TEST(UtilsTest, millis_since) {
    esphome::mock::set_millis(5000);
    EXPECT_EQ(0u, Utils::millis_since(5000));
    EXPECT_EQ(4000u, Utils::millis_since(1000));
    esphome::mock::set_millis(std::nullopt);
}

TEST(UtilsTest, millis_since_across_overflow) {
    // millis() overflows after 49.7 days: 10 ms before the overflow until 5 ms after it
    const uint32_t before = UINT32_MAX - 9;
    esphome::mock::set_millis(5);
    EXPECT_EQ(15u, Utils::millis_since(before));
    // the old form `now > before + interval` would say "not yet" here for the next 49 days
    EXPECT_TRUE(Utils::millis_since(before) > 10u);
    EXPECT_FALSE(Utils::millis_since(before) > 20u);
    esphome::mock::set_millis(std::nullopt);
}
