#include "esphome/components/daikin_rotex_can/scheduler.h"
#include "esphome/core/hal.h"

#include <iterator>

namespace esphome {
namespace daikin_rotex_can {

CallHandle::CallHandle()
: m_id(0)
, m_scheduler(nullptr) {
}

CallHandle::CallHandle(CallID id, Scheduler* scheduler)
: m_id(id)
, m_scheduler(scheduler) {
}

bool CallHandle::cancel() {
    if (!is_valid()) return false;
    bool result = m_scheduler->cancel_call(m_id);
    if (result) {
        m_id = 0;
    }
    return result;
}

CallHandle Scheduler::call_later(CallHandle::TVoidFunc lambda, uint32_t timeout) {
    CallHandle::CallID new_id = m_next_call_id.fetch_add(1);

    m_later_calls.push_back({
        std::move(lambda),
        esphome::millis() + timeout,
        new_id
    });

    return CallHandle(new_id, this);
}

bool Scheduler::is_call_valid(CallHandle::CallID id) const {
    if (id == 0) return false;

    for (const auto& call : m_later_calls) {
        if (call.id == id) {
            return true;
        }
    }
    return false;
}

bool Scheduler::accelerate_call(CallHandle::CallID id) {
    for (auto& call : m_later_calls) {
        if (call.id == id) {
            call.execution_time = esphome::millis();
            return true;
        }
    }
    return false;
}

bool Scheduler::cancel_call(CallHandle::CallID id) {
    // list::remove_if unlinks nodes; std::remove_if would move the other calls around
    const size_t size_before = m_later_calls.size();
    m_later_calls.remove_if([id](const DelayedCall& call) {
        return call.id == id;
    });
    return m_later_calls.size() != size_before;
}

void Scheduler::update() {
    if (m_later_calls.empty()) return;

    const uint32_t current_time = esphome::millis();

    // Take the due calls out first and run them afterwards: a call may cancel or add calls
    // (also itself) without invalidating what is being iterated or executed.
    std::list<DelayedCall> due;
    for (auto it = m_later_calls.begin(); it != m_later_calls.end();) {
        auto next = std::next(it);
        if ((int32_t)(current_time - it->execution_time) >= 0) {
            due.splice(due.end(), m_later_calls, it);
        }
        it = next;
    }

    for (auto& call : due) {
        call.function();
    }
}

}
}
