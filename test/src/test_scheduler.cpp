#include <gtest/gtest.h>
#include "esphome/components/daikin_rotex_can/scheduler.h"
#include "mock_esphome.h"

using namespace esphome::daikin_rotex_can;

// The scheduler is a singleton: every test starts at a fixed time and leaves no call behind
// (a left-over call would write into a finished test's stack).
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
};

TEST_F(SchedulerTest, CallLaterReturnsValidHandle) {
    auto& scheduler = Scheduler::getInstance();

    bool called = false;
    auto handle = scheduler.call_later([&called]() {
        called = true;
    }, 100);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_FALSE(called);
}

TEST_F(SchedulerTest, CancelCallWorks) {
    auto& scheduler = Scheduler::getInstance();

    bool called = false;
    auto handle = scheduler.call_later([&called]() {
        called = true;
    }, 100);

    EXPECT_TRUE(handle.cancel());
    EXPECT_FALSE(handle.is_valid());
    scheduler.update();
    EXPECT_FALSE(called);
}

TEST_F(SchedulerTest, AccelerateCallWorks) {
    auto& scheduler = Scheduler::getInstance();

    bool called = false;
    auto handle = scheduler.call_later([&called]() {
        called = true;
    }, 100);

    EXPECT_TRUE(handle.accelerate());

    scheduler.update();
    EXPECT_TRUE(called);
}

TEST_F(SchedulerTest, CallRunsWhenDue) {
    auto& scheduler = Scheduler::getInstance();

    bool called = false;
    scheduler.call_later([&called]() {
        called = true;
    }, 100);

    esphome::mock::set_millis(1099);
    scheduler.update();
    EXPECT_FALSE(called);

    esphome::mock::set_millis(1100);
    scheduler.update();
    EXPECT_TRUE(called);
}

TEST_F(SchedulerTest, CallMayCancelAnotherDueCall) {
    auto& scheduler = Scheduler::getInstance();

    int calls = 0;
    CallHandle second;
    scheduler.call_later([&]() {
        ++calls;
        second.cancel();
    });
    second = scheduler.call_later([&calls]() {
        ++calls;
    });

    scheduler.update();
    // both were due: the second is run as well (it was taken out before the first ran)
    EXPECT_EQ(2, calls);
    EXPECT_FALSE(second.is_valid());
}

TEST_F(SchedulerTest, CallMayCancelAPendingCall) {
    auto& scheduler = Scheduler::getInstance();

    bool later_called = false;
    CallHandle later = scheduler.call_later([&later_called]() {
        later_called = true;
    }, 100);
    scheduler.call_later([&later]() {
        later.cancel();
    });

    scheduler.update();
    esphome::mock::set_millis(1100);
    scheduler.update();
    EXPECT_FALSE(later_called);
}

TEST_F(SchedulerTest, CallMayCancelItself) {
    auto& scheduler = Scheduler::getInstance();

    int calls = 0;
    CallHandle self;
    self = scheduler.call_later([&]() {
        ++calls;
        EXPECT_FALSE(self.cancel());  // already taken out: nothing to cancel
    });

    scheduler.update();
    scheduler.update();
    EXPECT_EQ(1, calls);
}

TEST_F(SchedulerTest, CallMayAddACall) {
    auto& scheduler = Scheduler::getInstance();

    int calls = 0;
    scheduler.call_later([&]() {
        ++calls;
        scheduler.call_later([&calls]() {
            ++calls;
        });
    });

    scheduler.update();
    EXPECT_EQ(1, calls);  // the added call runs with the next update
    scheduler.update();
    EXPECT_EQ(2, calls);
}
