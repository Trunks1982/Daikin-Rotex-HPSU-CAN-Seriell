#include "esphome/components/daikin_rotex_can/daikin_rotex_can.h"
#include "esphome/components/daikin_rotex_can/entity.h"
#include "esphome/components/daikin_rotex_can/sensors.h"
#include "esphome/components/daikin_rotex_can/translations.h"
#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>
#include <limits>
#include <regex>

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "daikin_rotex_can";
static const char* ERROR_CODE_TAG = "error code";
static const std::string BETRIEBS_ART = "mode_of_operating";
static const std::string BETRIEBS_MODUS = "operating_mode";
static const std::string OPTIMIZED_DEFROSTING = "optimized_defrosting";
static const std::string TEMPERATURE_ANTIFREEZE = "temperature_antifreeze";   // T-Frostschutz
static const std::string FLOW_RATE = "flow_rate";
static const std::string STATE_COMPRESSOR = "status_kompressor";
static const std::string SUPPLY_SETPOINT_REGULATED = "supply_setpoint_regulated";

inline bool operator==(const Translation& lhs, const std::string& rhs) {
    return static_cast<std::string>(lhs) == rhs;
}

inline bool operator==(const std::string& lhs, const Translation& rhs) {
    return lhs == static_cast<std::string>(rhs);
}

inline bool operator!=(const Translation& lhs, const std::string& rhs) {
    return static_cast<std::string>(lhs) != rhs;
}

inline bool operator!=(const std::string& lhs, const Translation& rhs) {
    return lhs != static_cast<std::string>(rhs);
}

inline std::string operator+(const Translation& lhs, const std::string& rhs) {
    return static_cast<std::string>(lhs) + rhs;
}

inline std::string operator+(const std::string& lhs, const Translation& rhs) {
    return lhs + static_cast<std::string>(rhs);
}

DaikinRotexCanComponent::ErrorDetection::ErrorDetection(uint32_t detection_time_ms, bool stop_detection_in_good_case)
: m_error_timestamp(0u)
, m_detection_time_ms(detection_time_ms * 1000u)
, m_good_case_detected(false)
, m_stop_detection_in_good_case(stop_detection_in_good_case)
{
}

bool DaikinRotexCanComponent::ErrorDetection::handle_error_detection(bool is_error_state) {
    if (is_error_state) {
        if (m_error_timestamp == 0u && !m_good_case_detected) {
            m_error_timestamp = millis();
        }

        if (m_error_timestamp != 0 && millis() > (m_error_timestamp + m_detection_time_ms)) {
            return true;
        }
    } else {
        m_error_timestamp = 0u;
        m_good_case_detected = m_stop_detection_in_good_case;
    }
    return false;
}

void DaikinRotexCanComponent::ErrorDetection::reset_good_case() {
    m_error_timestamp = 0;
    m_good_case_detected = false;
}

DaikinRotexCanComponent::DaikinRotexCanComponent()
: m_entity_manager()
, m_later_calls()
, m_dhw_run_lambdas()
, m_optimized_defrosting(false)
, m_betriebsmodus_before_dhw_and_defrosting(Translation::T_STANDBY)
, m_project_git_hash_sensor(nullptr)
, m_project_git_hash()
, m_thermal_power_sensor(new CanSensor("thermal_power")) // Create dummy sensors to avoid nullptr without HA api communicaction. Can be overwritten by the user.
, m_thermal_power_raw_sensor(new CanSensor("thermal_power_raw"))
, m_temperature_spread_sensor(new CanSensor("temperature_spread")) // Used to detect valve malfunctions, even if the sensor has not been defined by the user.
, m_temperature_spread_raw_sensor(new CanSensor("temperature_spread_raw"))
, m_tv_tvbh_delta_sensor(new CanSensor("tv_tvbh_delta"))
, m_tvbh_tr_delta_sensor(new CanSensor("tvbh_tr_delta"))
, m_vorlauf_soll_tv_delta(new CanSensor("vorlauf_soll_tv_delta"))
, m_mixer_error_detection(10 * 60, false)      // 10 minute
, m_bpv_error_detection(10 * 60, false)      // 10 minutes
, m_spread_error_detection(20 * 60, true)    // 20 minutes
, m_dhw_error_detection(5 * 60, false)
, m_supply_setpoint_regulated(nullptr)
, m_last_supply_setpoint_regulated_ts(0u)
{
    m_temperature_spread_sensor->set_smooth(true);
}

void DaikinRotexCanComponent::setup() {
    ESP_LOGI(TAG, "setup");

    for (auto const& pEntity : m_entity_manager.get_entities()) {
        ESP_LOGI("setup", "name: %s, id: %s, can_id: %s, command: %s",
            pEntity->getName().c_str(), pEntity->get_id().c_str(),
            Utils::to_hex(pEntity->get_config().can_id).c_str(), Utils::to_hex(pEntity->get_config().command).c_str());

        pEntity->set_canbus(m_pCanbus);
        if (CanTextSensor* pTextSensor = dynamic_cast<CanTextSensor*>(pEntity)) {
            pTextSensor->set_recalculate_state([this](EntityBase* pEntity, std::string const& state){
                return recalculate_state(pEntity, state);
            });
        } else if (CanSelect* pSelect = dynamic_cast<CanSelect*>(pEntity)) {
            pSelect->set_custom_select_lambda([this](std::string const& id, uint16_t key){
                return on_custom_select(id, key);
            });
        }
        pEntity->set_post_handle([this](TEntity* pEntity, TEntity::TVariant const& current, TEntity::TVariant const& previous){
            on_post_handle(pEntity, current, previous);
        });
    }

    m_entity_manager.removeInvalidRequests();
    const uint32_t size = m_entity_manager.size();

    ESP_LOGI(TAG, "entities.size: %d", size);

    CanSelect* p_optimized_defrosting = m_entity_manager.get_select(OPTIMIZED_DEFROSTING);
    if (p_optimized_defrosting != nullptr) {
        m_optimized_defrosting.load(p_optimized_defrosting);
        p_optimized_defrosting->publish_select_key(m_optimized_defrosting.value());
    }

    m_project_git_hash_sensor->publish_state(m_project_git_hash);
}

void DaikinRotexCanComponent::on_post_handle(TEntity* pEntity, TEntity::TVariant const& current, TEntity::TVariant const& previous) {
    std::list<std::string> const& update_entities = pEntity->get_update_entity();
    for (std::string const& update_entity : update_entities) {
        if (!update_entity.empty()) {
            call_later([update_entity, this](){
                updateState(update_entity);
            });
        }
    }

    const std::string id = pEntity->get_id();
    if (id == "target_hot_water_temperature") {
        call_later([this](){
            run_dhw_lambdas();
        });
    } else if (id == BETRIEBS_ART) {
        call_later([this, current, previous](){
            on_betriebsart(current, previous);
        });
    } else if (id == BETRIEBS_MODUS) {
        call_later([this, current, previous](){
            on_betriebsmodus(current, previous);
        });
    } else if (id == TEMPERATURE_ANTIFREEZE) {
        CanSelect* p_optimized_defrosting = m_entity_manager.get_select(OPTIMIZED_DEFROSTING);
        CanSelect* p_temperature_antifreeze = m_entity_manager.get_select(TEMPERATURE_ANTIFREEZE);
        if (p_optimized_defrosting != nullptr && p_temperature_antifreeze != nullptr) {
            const std::string antifreeze_option = p_temperature_antifreeze->current_option();
            if (antifreeze_option != Translation::T_OFF && m_optimized_defrosting.value() != 0x0) {
                p_optimized_defrosting->publish_select_key(0x0);
                m_optimized_defrosting.save(0x0);
                Utils::log(TAG, "set %s: %d", OPTIMIZED_DEFROSTING.c_str(), m_optimized_defrosting.value());
            }
        }
    } else if (id == STATE_COMPRESSOR && current != previous) {
        m_spread_error_detection.reset_good_case();
    }
}

void DaikinRotexCanComponent::updateState(std::string const& id) {
    TEntity* pEntity = m_entity_manager.get(id);
    if (pEntity != nullptr) {
        TEntity::TEntityArguments const& config = pEntity->get_config();
        if (config.update_lambda_set) {
            std::string value = config.update_lambda(*this);

            if (CanTextSensor* pTextSensor = dynamic_cast<CanTextSensor*>(pEntity)) {
                pTextSensor->publish_state(value);
            } else {
                ESP_LOGE(TAG, "Unsupported entityy type: %s", id.c_str());
            }
            return;
        }
    }

    if (id == "thermal_power") {
        update_thermal_power();
    } else if (id == "temperature_spread") {
        update_temperature_spread();
    } else if (id == "tv_tvbh_delta") {
        CanSensor const* tv = m_entity_manager.get_sensor("tv");
        CanSensor const* tvbh = m_entity_manager.get_sensor("tvbh");

        if (tv == nullptr || tvbh == nullptr) {
            return;
        }

        m_tv_tvbh_delta_sensor->publish_state(tv->state - tvbh->state);
    } else if (id == "tvbh_tr_delta") {
        CanSensor const* tvbh = m_entity_manager.get_sensor("tvbh");
        CanSensor const* tr = m_entity_manager.get_sensor("tr");

        if (tvbh == nullptr || tr == nullptr) {
            return;
        }

        m_tvbh_tr_delta_sensor->publish_state(tvbh->state - tr->state);
    } else if (id == "vorlauf_soll_tv_delta") {
        CanSensor const* vorlauf_soll = m_entity_manager.get_sensor("target_supply_temperature");
        CanSensor const* tv = m_entity_manager.get_sensor("tv");

        if (vorlauf_soll == nullptr || tv == nullptr) {
            return;
        }

        m_vorlauf_soll_tv_delta->publish_state(vorlauf_soll->state - tv->state);
    }
}

void DaikinRotexCanComponent::update_thermal_power() {
    CanSensor const* flow_rate = m_entity_manager.get_sensor(FLOW_RATE);
    CanSensor const* tv = m_entity_manager.get_sensor("tv");
    CanSensor const* tr = m_entity_manager.get_sensor("tr");

    if (flow_rate == nullptr) {
        ESP_LOGE(TAG, "flow_rate is not configured!");
        return;
    }
    if (tv == nullptr) {
        ESP_LOGE(TAG, "tv is not configured!");
        return;
    }
    if (tr == nullptr) {
        ESP_LOGE(TAG, "tr is not configured!");
        return;
    }

    const float thermal_power_raw = (tv->state - tr->state) * (4.19 * flow_rate->state) / 3600.0f;

    Utils::log("update_thermal_power", "tv: %f, tr: %f, flow: %f, power_raw: %f", tv->state, tr->state, flow_rate->state, thermal_power_raw);

    m_thermal_power_raw_sensor->publish(thermal_power_raw);
    m_thermal_power_sensor->publish(thermal_power_raw);
}

void DaikinRotexCanComponent::update_temperature_spread() {
    CanSensor const* tv = m_entity_manager.get_sensor("tv");
    CanSensor const* tr = m_entity_manager.get_sensor("tr");

    if (tv != nullptr && tr != nullptr) {
        const float temperature_spread = tv->state - tr->state;

        Utils::log("update_temperature_spread", "tv: %f, tr: %f", tv->state, tr->state);

        m_temperature_spread_sensor->publish(temperature_spread);
        m_temperature_spread_raw_sensor->publish(temperature_spread);
    }
}

bool DaikinRotexCanComponent::on_custom_select(std::string const& id, uint8_t value) {
    if (id == OPTIMIZED_DEFROSTING) {
        Utils::log(TAG, "%s: %d", OPTIMIZED_DEFROSTING.c_str(), value);
        CanSelect* p_temperature_antifreeze = m_entity_manager.get_select(TEMPERATURE_ANTIFREEZE);

        if (p_temperature_antifreeze != nullptr) {
            if (value != 0) {
                m_entity_manager.sendSet(p_temperature_antifreeze->get_name(), p_temperature_antifreeze->getKey(Translation::T_OFF));
            }
        } else {
            ESP_LOGE(TAG, "on_custom_select(%s, %d) => temperature_antifreeze select is missing!", id.c_str(), value);
        }
        m_optimized_defrosting.save(value);
        return true;
    }
    return false;
}

void DaikinRotexCanComponent::on_betriebsart(TEntity::TVariant const& current, TEntity::TVariant const& previous) {
    CanSelect* p_betriebs_modus = m_entity_manager.get_select(BETRIEBS_MODUS);
    if (m_optimized_defrosting.value() && p_betriebs_modus != nullptr) {
        if (std::holds_alternative<std::string>(current)) {
            const auto art_current = std::get<std::string>(current);
            const auto art_previous = std::get<std::string>(previous);
            const std::string modus = p_betriebs_modus->current_option();
            std::string new_mode = modus;

            const bool modus_is_heating = is_modus_heating(modus);

            if (art_current == Translation::T_DEFROSTING && art_previous == Translation::T_HEATING && modus_is_heating) { // Heating -> Defrost
                new_mode = Translation::T_SUMMER;
            } else if (art_current == Translation::T_HEATING && art_previous == Translation::T_DEFROSTING && modus == Translation::T_SUMMER) { // Defrost -> Heating
                new_mode = m_betriebsmodus_before_dhw_and_defrosting; // Heating, Automatic 1 or Automatic 2
                m_betriebsmodus_before_dhw_and_defrosting = Translation::T_STANDBY;
            } else if (art_current == Translation::T_DEFROSTING && art_previous == Translation::T_HOT_WATER_PRODUCTION && modus_is_heating) { // DHW -> Defrost
                new_mode = Translation::T_SUMMER;
            } else if (art_current == Translation::T_HOT_WATER_PRODUCTION && art_previous == Translation::T_DEFROSTING && modus == Translation::T_SUMMER && is_modus_heating(m_betriebsmodus_before_dhw_and_defrosting)) { // Defrost -> DHW
                new_mode = m_betriebsmodus_before_dhw_and_defrosting;
                m_betriebsmodus_before_dhw_and_defrosting = Translation::T_STANDBY;
            } else if (art_current == Translation::T_STANDBY && art_previous == Translation::T_DEFROSTING && modus == Translation::T_SUMMER) { // Special case: Defrost -> Standby
                new_mode = m_betriebsmodus_before_dhw_and_defrosting;
                m_betriebsmodus_before_dhw_and_defrosting = Translation::T_STANDBY;
            }

            Utils::log(TAG, "on_betriebsart art_current: %s, art_previous: %s, modus: %s, new_mode: %s, before_dhw_defrosting: %s",
                art_current.c_str(), art_previous.c_str(), modus.c_str(), new_mode.c_str(), m_betriebsmodus_before_dhw_and_defrosting.c_str());

            if (new_mode != modus) {
                if (art_current == Translation::T_DEFROSTING && modus_is_heating) {
                    m_betriebsmodus_before_dhw_and_defrosting = modus;
                }

                const uint16_t ui_new_mode = p_betriebs_modus->getKey(new_mode);
                if (ui_new_mode != 0x0) {
                    m_entity_manager.sendSet(p_betriebs_modus->get_name(), ui_new_mode);
                }
            }
        } else {
            ESP_LOGE(TAG, "on_betriebsart(): current has no valid string value!");
        }
    }

    if (current != previous) {
        m_spread_error_detection.reset_good_case();
    }
}

void DaikinRotexCanComponent::on_betriebsmodus(TEntity::TVariant const& current, TEntity::TVariant const& previous) {
    CanTextSensor const* p_betriebs_art = m_entity_manager.get_text_sensor(BETRIEBS_ART);

    if (p_betriebs_art == nullptr) {
        return;
    }

    const auto modus_current = std::get<std::string>(current);
    const auto modus_previous = std::get<std::string>(previous);
    const std::string art = p_betriebs_art->state;

    Utils::log(TAG, "on_betriebsmodus current: %s, previous: %s, art: %s", modus_current.c_str(), modus_previous.c_str(), art.c_str());


    if (!is_modus_heating(modus_current) && art != Translation::T_DEFROSTING) {
        m_betriebsmodus_before_dhw_and_defrosting = Translation::T_STANDBY;
    }
}

///////////////// Texts /////////////////
void DaikinRotexCanComponent::custom_request(std::string const& value) {
    std::regex pattern(R"(^((?:0x[0-9A-Fa-f]{2}|[0-9A-Fa-f]{2})(?:\s(?:0x[0-9A-Fa-f]{2}|[0-9A-Fa-f]{2})){0,6})$)");
    std::smatch match;
    if (std::regex_match(value, match, pattern)) {
        uint16_t can_id = 0x680;
        const bool use_extended_id = false;

        const TMessage message = Utils::str_to_bytes(match[1].str());

        Utils::log(TAG, "custom_request() can_id<%s> data<%s> str<%s>",
            Utils::to_hex(can_id).c_str(), Utils::to_hex(message).c_str(), match[1].str().c_str());

        esphome::esp32_can::ESP32Can* pCanbus = m_entity_manager.getCanbus();
        pCanbus->send_data(can_id, use_extended_id, { message.begin(), message.end() });
    } else {
        ESP_LOGW(TAG, "custom_request() invalid message: %s", value.c_str());
    }
}

///////////////// Buttons /////////////////
void DaikinRotexCanComponent::dhw_run() {
    const std::string id {"target_hot_water_temperature"};
    TEntity const* pEntity = m_entity_manager.get(id);
    if (pEntity != nullptr) {
        float temp1 {70};
        float temp2 {0};

        if (CanNumber const* pNumber = dynamic_cast<CanNumber const*>(pEntity)) {
            temp2 = pNumber->state;
        } else if (CanSelect const* pSelect = dynamic_cast<CanSelect const*>(pEntity)) {
            const std::string option = pSelect->current_option();
            if (!option.empty()) {
                temp2 = pSelect->getKey(option) / pEntity->get_config().divider;
            }
        }

        if (temp2 > 0) {
            const std::string name = pEntity->getName();

            m_entity_manager.sendSet(name,  temp1);

            call_later([name, temp2, this](){
                m_entity_manager.sendSet(name, temp2);
            }, 10*1000);
        } else {
            ESP_LOGE(TAG, "dhw_run: Request doesn't have a Number!");
        }
    } else {
        ESP_LOGE(TAG, "dhw_run: Request couldn't be found!");
    }
}

void DaikinRotexCanComponent::dump() {
    ESP_LOGI(TAG, "------------------------------------------");
    ESP_LOGI(TAG, "------------ DUMP %d Entities ------------", m_entity_manager.size());
    ESP_LOGI(TAG, "------------------------------------------");

    for (auto index = 0; index < m_entity_manager.size(); ++index) {
        TEntity* pEntity = m_entity_manager.get(index);
        if (pEntity != nullptr) {
            if (CanSensor* pSensor = dynamic_cast<CanSensor*>(pEntity)) {
                ESP_LOGI(TAG, "%s: %f", pSensor->get_name().c_str(), pSensor->get_state());
            } else if (CanBinarySensor* pBinarySensor = dynamic_cast<CanBinarySensor*>(pEntity)) {
                ESP_LOGI(TAG, "%s: %d", pBinarySensor->get_name().c_str(), pBinarySensor->state);
            } else if (CanNumber* pNumber = dynamic_cast<CanNumber*>(pEntity)) {
                ESP_LOGI(TAG, "%s: %f", pNumber->get_name().c_str(), pNumber->state);
            } else if (CanTextSensor* pTextSensor = dynamic_cast<CanTextSensor*>(pEntity)) {
                ESP_LOGI(TAG, "%s: %s", pTextSensor->get_name().c_str(), pTextSensor->get_state().c_str());
            } else if (CanSelect* pSelect = dynamic_cast<CanSelect*>(pEntity)) {
                const std::string option = pSelect->current_option();
                ESP_LOGI(TAG, "%s: %s", pSelect->get_name().c_str(), option.c_str());
            }
        } else {
            ESP_LOGE(TAG, "Entity with index<%d> not found!", index);
        }
    }
    ESP_LOGI(TAG, "------------------------------------------");
}

void DaikinRotexCanComponent::on_custom_number(number::Number& number, float value) {
    if (&number == m_supply_setpoint_regulated) {
        number.publish_state(value);
    }
}

void DaikinRotexCanComponent::run_dhw_lambdas() {
    if (!m_dhw_run_lambdas.empty()) {
        auto& lambda = m_dhw_run_lambdas.front();
        lambda();
        m_dhw_run_lambdas.pop_front();
    }
}

void DaikinRotexCanComponent::loop() {
    m_entity_manager.sendNextPendingGet();

    for (auto it = m_later_calls.begin(); it != m_later_calls.end(); ) {
        if (millis() > it->second) { // checking millis() here is important for callLater!
            it->first();
            it = m_later_calls.erase(it);
        } else {
            ++it;
        }
    }

    const uint32_t millis = esphome::millis();
    for (TEntity* pEntity : m_entity_manager.get_entities()) {
        pEntity->update(millis);
    }
    m_thermal_power_sensor->update(millis);
    m_temperature_spread_sensor->update(millis);

    update_supply_setpoint_regulated();
}

void DaikinRotexCanComponent::update_supply_setpoint_regulated() {
    CanTextSensor const* p_betriebs_art = m_entity_manager.get_text_sensor(BETRIEBS_ART);
    CanBinarySensor const* state_compressor = m_entity_manager.get_binary_sensor(STATE_COMPRESSOR);
    CanNumber const* pMaxTVorlauf = m_entity_manager.get_number("max_target_flow_temp", false); // pMax_target_flow_temp
    CanSensor const* pTv = m_entity_manager.get_sensor("tv");
    CanSensor const* pVorlaufSoll = m_entity_manager.get_sensor("target_supply_temperature");

    if (p_betriebs_art == nullptr || pMaxTVorlauf == nullptr || pTv == nullptr || pVorlaufSoll == nullptr || m_supply_setpoint_regulated == nullptr) {
        return;
    }

    if (p_betriebs_art->state != Translation::T_HEATING) {
        return;
    }

    if (state_compressor == nullptr || !state_compressor->state) {
        return;
    }

    const float max_t_vorlauf = pMaxTVorlauf->state;
    const float tv = pTv->state;
    const float vorlauf_soll = pVorlaufSoll->state;
    const float vorlauf_soll_reguliert = m_supply_setpoint_regulated->state;

    if (std::isnan(vorlauf_soll_reguliert) || vorlauf_soll_reguliert == 0) {
        return;
    }

    if (m_last_supply_setpoint_regulated_ts == 0u || millis() > (m_last_supply_setpoint_regulated_ts + 30 * 1000)) {
        if (max_t_vorlauf != vorlauf_soll_reguliert) {
            float vorlauf_soll_request = vorlauf_soll;
            if (tv > vorlauf_soll && (tv - vorlauf_soll) > 2.5) {
                vorlauf_soll_request = std::round(tv - 1.5);
            } else if (vorlauf_soll_reguliert >= tv) {
                vorlauf_soll_request = vorlauf_soll_reguliert;
            } else if ((tv - vorlauf_soll_reguliert) < 1.5) {
                vorlauf_soll_request = vorlauf_soll_reguliert;
            } else {
                vorlauf_soll_request = std::round(tv - 1.5);
            }
            if (vorlauf_soll_request != vorlauf_soll) {
                Utils::log(TAG, "request vorlauf_soll_request: %f, tv: %f, max_t_vorlauf: %f, vorlauf_soll_reguliert: %f",
                    vorlauf_soll_request, tv, max_t_vorlauf, vorlauf_soll_reguliert);

                m_entity_manager.sendSet(pMaxTVorlauf->get_name(), vorlauf_soll_request);
            }
        }
        m_last_supply_setpoint_regulated_ts = millis();
    }
}

void DaikinRotexCanComponent::handle(uint32_t can_id, std::vector<uint8_t> const& data) {
    TMessage message;
    std::copy_n(data.begin(), 7, message.begin());
    m_entity_manager.handle(can_id, message);
}

void DaikinRotexCanComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "DaikinRotexCanComponent");
}

bool DaikinRotexCanComponent::is_command_set(TMessage const& message) {
    for (auto& b : message) {
        if (b != 0x00) {
            return true;
        }
    }
    return false;
}

bool DaikinRotexCanComponent::is_modus_heating(std::string const& modus) {
    return modus == Translation::T_HEATING || modus == Translation::T_AUTOMATIC_1 || modus == Translation::T_AUTOMATIC_2;
}

std::string DaikinRotexCanComponent::recalculate_state(EntityBase* pEntity, std::string const& new_state) {
    CanSensor const* tv = m_entity_manager.get_sensor("tv");
    CanSensor const* tvbh = m_entity_manager.get_sensor("tvbh");
    CanSensor const* tr = m_entity_manager.get_sensor("tr");
    CanSensor const* dhw_mixer_position = m_entity_manager.get_sensor("dhw_mixer_position");
    CanSensor const* bpv = m_entity_manager.get_sensor("bypass_valve");
    CanSensor const* flow_rate = m_entity_manager.get_sensor(FLOW_RATE);
    CanSensor const* ta = m_entity_manager.get_sensor("temperature_outside");
    CanSensor const* tdhw1 = m_entity_manager.get_sensor("tdhw1");
    CanTextSensor const* error_code = m_entity_manager.get_text_sensor("error_code");
    CanTextSensor const* p_betriebs_art = m_entity_manager.get_text_sensor(BETRIEBS_ART);
    CanBinarySensor const* state_compressor = m_entity_manager.get_binary_sensor(STATE_COMPRESSOR);

    if (error_code != nullptr && pEntity == error_code && tv != nullptr && tvbh != nullptr && tr != nullptr) {
        const float tv_state = tv->state + m_tv_tvbh_tr_offset.tv;
        const float tvbh_state = tvbh->state + m_tv_tvbh_tr_offset.tvbh;
        const float tr_state = tr->state + m_tv_tvbh_tr_offset.tr;

        if (flow_rate != nullptr && dhw_mixer_position != nullptr) {
            const bool is_error_state = flow_rate->state > 600.0f && dhw_mixer_position->state == 0.0f && tvbh_state > (tv_state + m_max_spread.tvbh_tv);

            Utils::log(ERROR_CODE_TAG, "tv: %f, tvbh: %f, TvBH-Tv: %f, dhw: %f, flow: %f, dhw_ts: %d, millis: %d",
                tv_state, tvbh_state, m_max_spread.tvbh_tv, dhw_mixer_position->state, flow_rate->state,
                    m_mixer_error_detection.get_error_detection_timestamp(),
                    millis());

            if (m_mixer_error_detection.handle_error_detection(is_error_state)) {
                ESP_LOGE(ERROR_CODE_TAG, "3UV DHW defekt (1) => tvbh: %f, tv: %f, max_spread: %f, bpv: %f, flow_rate: %f",
                    tvbh_state, tv_state, m_max_spread.tvbh_tv, dhw_mixer_position->state, flow_rate->state);
                return new_state + "|3UV DHW " + Translation::T_DEFECT;
            }
        }

        if (flow_rate != nullptr && bpv != nullptr) {
            const bool is_error_state = flow_rate->state > 600.0f && bpv->state == 100.0f && tvbh_state > (tr_state + m_max_spread.tvbh_tr);

            Utils::log(ERROR_CODE_TAG, "tvbh: %f, tr: %f, Tr-TvBH: %f, bpv: %f, flow: %f, bpv_ts: %d, millis: %d",
                tvbh_state, tr_state, m_max_spread.tvbh_tr, bpv->state, flow_rate->state,
                    m_bpv_error_detection.get_error_detection_timestamp(),
                    millis());

            if (m_bpv_error_detection.handle_error_detection(is_error_state)) {
                ESP_LOGE(ERROR_CODE_TAG, "3UV BPV defekt (1) => tvbh: %f, tr: %f, max_spread: %f, dhw_mixer_pos: %f, flow_rate: %f",
                    tvbh_state, tr_state, m_max_spread.tvbh_tr, bpv->state, flow_rate->state);
                return new_state + "|3UV BPV " + Translation::T_DEFECT;
            }
        }

        if (p_betriebs_art != nullptr && state_compressor != nullptr) {
            if (Utils::is_in(p_betriebs_art->state, Translation::T_HOT_WATER_PRODUCTION, Translation::T_HEATING) && state_compressor->state) {
                /*
                    Coefficients calculated using the table:
                    Tv  | minimal temperature spread
                    50° | 4.0
                    40° | 3.0
                    35° | 2.5
                    29° | 1.2
                    27° | 0.3
                */
                const float min_spread =
                    -0.00004012 * std::pow(tv->state, 4)
                    + 0.006683 * std::pow(tv->state, 3)
                    - 0.4152 * std::pow(tv->state, 2)
                    + 11.5006 * tv->state
                    - 117.7908;

                const bool is_error_state = state_compressor->state && m_temperature_spread_sensor->state < min_spread;

                Utils::log(TAG, "betriebsart: %s, compressor: %d, spread: %f, min_spread: %f, is_good_case_detected: %d, error_ts: %d, millis: %d",
                    p_betriebs_art->state.c_str(), state_compressor->state, m_temperature_spread_sensor->state, min_spread,
                    m_spread_error_detection.is_good_case_detected(), m_spread_error_detection.get_error_detection_timestamp(), millis());

                if (m_spread_error_detection.handle_error_detection(is_error_state)) {
                    ESP_LOGE(TAG, "Low spread!");
                    return new_state + "|" + Translation::T_LOW_TEMPERATURE_SPREAD;
                }
            }
        }

        if (p_betriebs_art != nullptr && flow_rate != nullptr && dhw_mixer_position != nullptr && state_compressor != nullptr && tdhw1 != nullptr) {
            const bool is_error_state = p_betriebs_art->state == Translation::T_HOT_WATER_PRODUCTION && tdhw1->state < 48.0 && (flow_rate->state == 0.0f || dhw_mixer_position->state == 0.0f || !state_compressor->state);
            if (m_dhw_error_detection.handle_error_detection(is_error_state)) {
                ESP_LOGE(TAG, "DHW error => flow: %d, mixer_pos: %d, state_compressor: %d", flow_rate->state, dhw_mixer_position->state, state_compressor->state);
                return new_state + "|" + Translation::T_MISSING_FLOW;
            }
        }
    }
    return new_state;
}

float DaikinRotexCanComponent::get_sensor_value(std::string const& id) const {
    CanSensor const* pSensor = m_entity_manager.get_sensor(id);
    if (pSensor != nullptr) {
        return pSensor->state;
    }
    ESP_LOGE(TAG, "Entity <%s> doesn't exists!", id.c_str());
    return std::numeric_limits<float>::quiet_NaN();
}

float DaikinRotexCanComponent::get_number_value(std::string const& id) const {
    CanNumber const* pNumber = m_entity_manager.get_number(id);
    if (pNumber != nullptr) {
        return pNumber->state;
    }
    ESP_LOGE(TAG, "Entity <%s> doesn't exists!", id.c_str());
    return std::numeric_limits<float>::quiet_NaN();
}

} // namespace daikin_rotex_can
} // namespace esphome