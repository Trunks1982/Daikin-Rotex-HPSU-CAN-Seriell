#include <gtest/gtest.h>
#include "esphome/components/daikin_rotex_can/scheduler.h"
#include "esphome/components/daikin_rotex_can/bidi_map.h"

using namespace esphome::daikin_rotex_can;

TEST(BidiMapTest, findNextByKey) {
    std::map<uint16_t, std::string> map =  {{1, "a"}, {3, "c"}, {5, "e"}, {7, "g"}};
    BidiMap bidi_map(map);

    EXPECT_EQ("a", bidi_map.findNextByKey(0)->second);
    EXPECT_EQ("a", bidi_map.findNextByKey(1)->second);
    EXPECT_EQ("a", bidi_map.findNextByKey(2)->second);
    EXPECT_EQ("c", bidi_map.findNextByKey(3)->second);
    EXPECT_EQ("c", bidi_map.findNextByKey(4)->second);
    EXPECT_EQ("e", bidi_map.findNextByKey(5)->second);
    EXPECT_EQ("e", bidi_map.findNextByKey(6)->second);
    EXPECT_EQ("g", bidi_map.findNextByKey(7)->second);
    EXPECT_EQ("g", bidi_map.findNextByKey(8)->second);
}

TEST(BidiMapTest, findByKey) {
    std::map<uint16_t, std::string> map =  {{1, "a"}, {3, "c"}, {5, "e"}, {7, "g"}};
    BidiMap bidi_map(map);

    EXPECT_TRUE(bidi_map.findByKey(0) == bidi_map.end());
    EXPECT_EQ("a", bidi_map.findByKey(1)->second);
    EXPECT_TRUE(bidi_map.findByKey(2) == bidi_map.end());
    EXPECT_EQ("c", bidi_map.findByKey(3)->second);
    EXPECT_TRUE(bidi_map.findByKey(4) == bidi_map.end());
    EXPECT_EQ("e", bidi_map.findByKey(5)->second);
    EXPECT_TRUE(bidi_map.findByKey(6) == bidi_map.end());
    EXPECT_EQ("g", bidi_map.findByKey(7)->second);
    EXPECT_TRUE(bidi_map.findByKey(8) == bidi_map.end());
}

TEST(BidiMapTest, findByValue) {
    std::map<uint16_t, std::string> map =  {{1, "a"}, {3, "c"}, {5, "e"}, {7, "g"}};
    BidiMap bidi_map(map);

    EXPECT_TRUE(bidi_map.findByValue("") == bidi_map.end());
    EXPECT_EQ(1, bidi_map.findByValue("a")->first);
    EXPECT_TRUE(bidi_map.findByValue("b") == bidi_map.end());
    EXPECT_EQ(3, bidi_map.findByValue("c")->first);
    EXPECT_TRUE(bidi_map.findByValue("d") == bidi_map.end());
    EXPECT_EQ(5, bidi_map.findByValue("e")->first);
    EXPECT_TRUE(bidi_map.findByValue("f") == bidi_map.end());
    EXPECT_EQ(7, bidi_map.findByValue("g")->first);
    EXPECT_TRUE(bidi_map.findByValue("h") == bidi_map.end());
}

TEST(BidiMapTest, getKey) {
    std::map<uint16_t, std::string> map =  {{1, "a"}, {3, "c"}, {5, "e"}, {7, "g"}};
    BidiMap bidi_map(map);

    EXPECT_EQ(0, bidi_map.getKey(""));
    EXPECT_EQ(1, bidi_map.getKey("a"));
    EXPECT_EQ(0, bidi_map.getKey("b"));
    EXPECT_EQ(3, bidi_map.getKey("c"));
    EXPECT_EQ(0, bidi_map.getKey("d"));
    EXPECT_EQ(5, bidi_map.getKey("e"));
    EXPECT_EQ(0, bidi_map.getKey("f"));
    EXPECT_EQ(7, bidi_map.getKey("g"));
    EXPECT_EQ(0, bidi_map.getKey("h"));
}

TEST(BidiMapTest, copy) {
    std::map<uint16_t, std::string> map =  {{1, "a"}, {3, "c"}, {5, "e"}, {7, "g"}};
    BidiMap bidi_map(map);
    BidiMap bidi_map2(bidi_map);

    EXPECT_EQ("a", bidi_map2.findNextByKey(0)->second);
    EXPECT_EQ("a", bidi_map2.findNextByKey(1)->second);
    EXPECT_EQ("a", bidi_map2.findNextByKey(2)->second);
    EXPECT_EQ("c", bidi_map2.findNextByKey(3)->second);
    EXPECT_EQ("c", bidi_map2.findNextByKey(4)->second);
    EXPECT_EQ("e", bidi_map2.findNextByKey(5)->second);
    EXPECT_EQ("e", bidi_map2.findNextByKey(6)->second);
    EXPECT_EQ("g", bidi_map2.findNextByKey(7)->second);
    EXPECT_EQ("g", bidi_map2.findNextByKey(8)->second);
}

TEST(BidiMapTest, assign) {
    BidiMap bidi_map{{2, "x"}};
    bidi_map = std::map<uint16_t, std::string>{{1, "a"}, {3, "c"}};

    EXPECT_EQ("a", bidi_map.findByKey(1)->second);
    EXPECT_EQ(3, bidi_map.getKey("c"));
    // the previous content is gone in both directions
    EXPECT_TRUE(bidi_map.findByKey(2) == bidi_map.end());
    EXPECT_EQ(0, bidi_map.getKey("x"));
}
