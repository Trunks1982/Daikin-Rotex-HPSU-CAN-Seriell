#pragma once

#include <atomic>
#include <cstdint>
#include <list>
#include <functional>

namespace esphome {
namespace daikin_rotex_can {

class Scheduler;

class CallHandle {
public:
    using TVoidFunc = std::function<void()>;
    using CallID = uint32_t;

private:
    CallID m_id;
    Scheduler* m_scheduler;

public:
    CallHandle();
    CallHandle(CallID id, Scheduler* scheduler);

    bool is_valid() const;
    bool accelerate();
    bool cancel();

    explicit operator bool() const {
        return is_valid();
    }

    CallID get_id() const { return m_id; }
};

class Scheduler {
private:
    struct DelayedCall {
        CallHandle::TVoidFunc function;
        uint32_t execution_time;
        CallHandle::CallID id;
    };

    std::list<DelayedCall> m_later_calls;
    std::atomic<CallHandle::CallID> m_next_call_id{1};

    Scheduler() {}
    Scheduler(Scheduler const&) = delete;
    void operator=(Scheduler const&) = delete;

public:
    static Scheduler& getInstance() {
        static Scheduler instance;
        return instance;
    }

    CallHandle call_later(CallHandle::TVoidFunc lambda, uint32_t timeout = 0u);
    bool accelerate_call(CallHandle::CallID id);
    bool cancel_call(CallHandle::CallID id);
    bool is_call_valid(CallHandle::CallID id) const;
    void update();
};

inline bool CallHandle::is_valid() const {
    return m_scheduler != nullptr && m_id != 0 && m_scheduler->is_call_valid(m_id);
}

inline bool CallHandle::accelerate() {
    if (!is_valid()) return false;
    return m_scheduler->accelerate_call(m_id);
}

}
}
