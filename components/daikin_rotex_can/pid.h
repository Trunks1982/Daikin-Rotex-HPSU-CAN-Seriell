#pragma once

#include "esphome/components/daikin_rotex_can/utils.h"
#include <string>

namespace esphome {
namespace daikin_rotex_can {

class PID {
public:
    PID(float p, float i, float d, float max_integral, float alpha_p, float alpha_d)
    : m_p(p)
    , m_i(i)
    , m_d(d)
    , m_previous_error(0.0f)
    , m_integral(0.0f)
    , m_max_integral(max_integral)
    , m_alpha_p(alpha_p)
    , m_alpha_d(alpha_d)
    , m_last_update(0)
    {
    }

    float compute(float setpoint, float current_value, float dt) {
        const float error = setpoint - current_value;

         // P-Anteil filtern
        m_filtered_p = m_alpha_p * error + (1 - m_alpha_p) * m_filtered_p;
        float p_term = m_p * m_filtered_p;

        // I-Anteil (mit Anti-Windup)
        m_integral += error * dt;
        if (m_integral > m_max_integral) m_integral = m_max_integral;
        if (m_integral < -m_max_integral) m_integral = -m_max_integral;
        float i_term = m_i * m_integral;

        // D-Anteil filtern
        float derivative = (error - m_previous_error) / dt;
        m_filtered_d = m_alpha_d * derivative + (1 - m_alpha_d) * m_filtered_d;
        float d_term = m_d * m_filtered_d;

        // Gesamtausgang
        float output = p_term + i_term + d_term;
        m_previous_error = error;

        m_last_update = millis();
        return output;
    }

    uint32_t get_last_update() const { return m_last_update; }

private:
    float m_p;
    float m_i;
    float m_d;
    float m_previous_error;
    float m_integral;
    float m_max_integral;
    float m_alpha_p, m_alpha_d;
    float m_filtered_p, m_filtered_d;
    uint32_t m_last_update;
};

}
}