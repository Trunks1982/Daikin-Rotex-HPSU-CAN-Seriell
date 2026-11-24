#include <gtest/gtest.h>
#include "esphome/components/daikin_rotex_can/scheduler.h"
#include "mock_esphome.h"

using namespace esphome::daikin_rotex_can;

// The scheduler is a singleton: every test starts at a fixed time and TearDown runs what is left.
// Calls only capture members of the fixture, which still exist in TearDown (a local of the test
// body would be gone by then).
class SchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        esphome::mock::set_millis(1000);
    }
    void TearDown() override {
        esphome::mock::set_millis(1000000);  // far past every timeout: run the rest
        Scheduler::getInstance().update();
        esphome::mock::set_millis(std::nullopt);
    }

    CallHandle later(uint32_t timeout = 0u) {
        return Scheduler::getInstance().call_later([this]() { ++calls; }, timeout);
    }

    int calls = 0;
    CallHandle other;
};

TEST_F(SchedulerTest, CallLaterReturnsValidHandle) {
    auto handle = later(100);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(0, calls);
}

TEST_F(SchedulerTest, CancelCallWorks) {
    auto handle = later(100);

    EXPECT_TRUE(handle.cancel());
    EXPECT_FALSE(handle.is_valid());
    Scheduler::getInstance().update();
    EXPECT_EQ(0, calls);
}

TEST_F(SchedulerTest, AccelerateCallWorks) {
    auto handle = later(100);

    EXPECT_TRUE(handle.accelerate());

    Scheduler::getInstance().update();
    EXPECT_EQ(1, calls);
}

TEST_F(SchedulerTest, CallRunsWhenDue) {
    later(100);

    esphome::mock::set_millis(1099);
    Scheduler::getInstance().update();
    EXPECT_EQ(0, calls);

    esphome::mock::set_millis(1100);
    Scheduler::getInstance().update();
    EXPECT_EQ(1, calls);
}

TEST_F(SchedulerTest, CallMayCancelAnotherDueCall) {
    auto& scheduler = Scheduler::getInstance();

    scheduler.call_later([this]() {
        ++calls;
        other.cancel();
    });
    other = later();

    scheduler.update();
    // both were due: the second is run as well (it was taken out before the first ran)
    EXPECT_EQ(2, calls);
    EXPECT_FALSE(other.is_valid());
}

TEST_F(SchedulerTest, CallMayCancelAPendingCall) {
    auto& scheduler = Scheduler::getInstance();

    other = later(100);
    scheduler.call_later([this]() {
        other.cancel();
    });

    scheduler.update();
    esphome::mock::set_millis(1100);
    scheduler.update();
    EXPECT_EQ(0, calls);
}

TEST_F(SchedulerTest, CallMayCancelItself) {
    auto& scheduler = Scheduler::getInstance();

    other = scheduler.call_later([this]() {
        ++calls;
        EXPECT_FALSE(other.cancel());  // already taken out: nothing to cancel
    });

    scheduler.update();
    scheduler.update();
    EXPECT_EQ(1, calls);
}

TEST_F(SchedulerTest, CallMayAddACall) {
    auto& scheduler = Scheduler::getInstance();

    scheduler.call_later([this]() {
        ++calls;
        later();
    });

    scheduler.update();
    EXPECT_EQ(1, calls);  // the added call runs with the next update
    scheduler.update();
    EXPECT_EQ(2, calls);
}
