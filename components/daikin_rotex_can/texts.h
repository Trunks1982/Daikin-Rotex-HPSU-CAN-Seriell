#pragma once

#include "esphome/components/daikin_rotex_can/sensor_accessor.h"
#include "esphome/components/daikin_rotex_can/entity.h"
#include "esphome/components/text/text.h"

#include <array>
#include <string>
#include <vector>

namespace esphome {
namespace daikin_rotex_can {

class LogFilterText : public text::Text, public Parented<SensorAccessor> {
public:
    LogFilterText();
protected:
    void control(const std::string &value) override;
};
class CustomRequestText : public text::Text, public Parented<SensorAccessor> {
public:
    CustomRequestText();
protected:
    void control(const std::string &value) override;
};

/**
 * Reads the complete "HZ Programm 1" heating program from the HPSU and exposes it
 * as a single, compact text entity.
 *
 * One entity aggregates 30 CAN registers (10 day/group blocks x 3 time windows, page 0x14).
 * The commands are rotated one-by-one by sendGet() and the received 2-byte time values are
 * collected in handle(). As soon as all registers are known, the aggregated string is
 * published (quarter-hour indices, unused windows as "x"), e.g.:
 *   {Mo: 25-39 48-70 75-90, Di: 25-39 48-70 75-90, ..., MS: 25-39 48-70 75-90}
 *
 * Write-back (set) to the HPSU is intentionally NOT implemented yet.
 */
class CanProgramText : public text::Text, public TEntity {
public:
    CanProgramText();

    void add_label(std::string const& label);
    void add_command(uint8_t register_offset);

    void publish_program();

protected:
    virtual void control(const std::string &value) override;
    virtual void update(uint32_t millis) override;
    virtual bool handleValue(uint16_t value, TVariant& current, TVariant& previous) override;
    virtual bool sendGet(esphome::esp32_can::ESP32Can* pCanBus) override;
    virtual bool handle(uint32_t can_id, TMessage const& responseData) override;
    virtual bool isGetNeeded() const override;

private:
    static constexpr uint8_t HZ_PROGRAM_PAGE = 0x14;
    static constexpr uint8_t WINDOWS_PER_BLOCK = 3;
    static constexpr uint16_t VALUE_UNCOLLECTED = 0xFFFF;
    // Cap on how many commands may be sent within one collection cycle before the
    // partial result is published (protects against registers that never answer).
    static constexpr uint32_t MAX_CYCLE_RETRIES = 3;

    int findRegisterIndex(uint8_t register_offset) const;

    std::vector<std::string> m_labels;
    std::vector<TMessage> m_commands;
    std::vector<uint16_t> m_values;     // 0xFFFF while not collected yet
    uint32_t m_current_command;         // next command to send (rotation)
    uint32_t m_sent;                    // commands sent within the current cycle
    uint32_t m_received;                // distinct registers collected within the current cycle
    bool m_collecting;
};

}
}