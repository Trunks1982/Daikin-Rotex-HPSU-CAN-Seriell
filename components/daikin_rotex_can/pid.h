#pragma once

class PID {
public:
    PID(float p, float i, float d, float dt = 0.1)
    : m_p(p)
    , m_i(i)
    , m_d(d)
    , m_dt(dt)
    , m_previous_error(0.0f)
    , m_integral(0.0f)
    , m_last_update(0)
    {
    }

    float compute(float setpoint, float current_value) {
        float error = setpoint - current_value;
        m_integral += error * m_dt;
        float derivative = (error - m_previous_error) / m_dt;
        float output = (m_p * error) + (m_i * m_integral) + (m_d * derivative);
        m_previous_error = error;
        return output;
    }

    uint32_t get_last_update() const { return m_last_update; }
    void set_last_update(uint32_t last_update) { m_last_update = last_update; }

private:
    float m_p;
    float m_i;
    float m_d;
    float m_dt;
    float m_previous_error;
    float m_integral;
    uint32_t m_last_update;
};