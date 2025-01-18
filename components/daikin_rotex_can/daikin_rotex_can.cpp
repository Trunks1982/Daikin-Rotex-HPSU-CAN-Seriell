#include "esphome/components/daikin_rotex_can/daikin_rotex_can.h"
#include "esphome/components/daikin_rotex_can/entity.h"
#include "esphome/components/daikin_rotex_can/sensors.h"
#include "esphome/components/daikin_rotex_can/translations.h"
#include <string>
#include <vector>
#include <limits>

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "daikin_rotex_can";
static const std::string BETRIEBS_ART = "mode_of_operating";
static const std::string BETRIEBS_MODUS = "operating_mode";
static const std::string OPTIMIZED_DEFROSTING = "optimized_defrosting";
static const std::string TEMPERATURE_ANTIFREEZE = "temperature_antifreeze";   // T-Frostschutz
static const std::string TEMPERATURE_ANTIFREEZE_OFF = translate("off");
static const std::string STATE_DHW_PRODUCTION = translate("hot_water_production");
static const std::string STATE_HEATING = translate("heating");
static const std::string STATE_DEFROSTING = translate("defrosting");
static const std::string STATE_SUMMER = translate("summer");
static const std::string STATE_STANDBY = translate("standby");
static const std::string DEFECT = translate("defect");
static const std::string LOW_TEMPERATURE_SPREAD = translate("low_temperature_spread");
static const uint32_t POST_SETUP_TIMOUT = 15*1000;

DaikinRotexCanComponent::DaikinRotexCanComponent()
: m_entity_manager()
, m_later_calls()
, m_dhw_run_lambdas()
, m_optimized_defrosting(false)
, m_project_git_hash_sensor(nullptr)
, m_project_git_hash()
, m_thermal_power_sensor(new CanSensor()) // Create dummy sensors to avoid nullptr without HA api communicaction. Can be overwritten by the user.
, m_thermal_power_raw_sensor(new CanSensor())
, m_temperature_spread_sensor(new CanSensor()) // Used to detect valve malfunctions, even if the sensor has not been defined by the user.
, m_temperature_spread_raw_sensor(new CanSensor())
, m_low_temperature_spread_timestamp(0u)
{
    m_temperature_spread_sensor->set_smooth(true);
}

void DaikinRotexCanComponent::setup() {
    ESP_LOGI(TAG, "setup");

    for (auto const& pEntity : m_entity_manager.get_entities()) {
        call_later([pEntity](){
            ESP_LOGI("setup", "name: %s, id: %s, can_id: %s, command: %s",
                pEntity->getName().c_str(), pEntity->get_id().c_str(),
                Utils::to_hex(pEntity->get_config().can_id).c_str(), Utils::to_hex(pEntity->get_config().command).c_str());
        }, POST_SETUP_TIMOUT);

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

    call_later([size](){
        ESP_LOGI(TAG, "entities.size: %d", size);
    }, POST_SETUP_TIMOUT);

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
        call_later([this, current, previous](){ // copy arguments -> temporary values
            on_betriebsart(current, previous);
        });
    } else if (id == TEMPERATURE_ANTIFREEZE) {
        CanSelect* p_optimized_defrosting = m_entity_manager.get_select(OPTIMIZED_DEFROSTING);
        CanSelect* p_temperature_antifreeze = m_entity_manager.get_select(TEMPERATURE_ANTIFREEZE);
        if (p_optimized_defrosting != nullptr && p_temperature_antifreeze != nullptr) {
            if (p_temperature_antifreeze->state != TEMPERATURE_ANTIFREEZE_OFF && m_optimized_defrosting.value() != 0x0) {
                p_optimized_defrosting->publish_select_key(0x0);
                m_optimized_defrosting.save(0x0);
                Utils::log(TAG, "set %s: %d", OPTIMIZED_DEFROSTING.c_str(), m_optimized_defrosting.value());
            }
        }
    }
}

void DaikinRotexCanComponent::updateState(std::string const& id) {
    if (id == "thermal_power") {
        update_thermal_power();
    } else if (id == "temperature_spread") {
        update_temperature_spread();
    }
}

void DaikinRotexCanComponent::update_thermal_power() {
    CanSensor const* flow_rate = m_entity_manager.get_sensor("flow_rate");
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

    m_thermal_power_raw_sensor->publish(thermal_power_raw);
    m_thermal_power_sensor->publish(thermal_power_raw);
}

void DaikinRotexCanComponent::update_temperature_spread() {
    CanSensor const* tv = m_entity_manager.get_sensor("tv");
    CanSensor const* tr = m_entity_manager.get_sensor("tr");

    if (tv != nullptr && tr != nullptr) {
        const float temperature_spread = tv->state - tr->state;

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
                m_entity_manager.sendSet(p_temperature_antifreeze->get_name(), p_temperature_antifreeze->getKey(TEMPERATURE_ANTIFREEZE_OFF));
            }
        } else {
            ESP_LOGE(TAG, "on_custom_select(%s, %d) => temperature_antifreeze select is missing!", id.c_str(), value);
        }
        m_optimized_defrosting.save(value);
        return true;
    } else if (id == BETRIEBS_MODUS) {
        CanSelect* p_betriebs_modus = m_entity_manager.get_select(BETRIEBS_MODUS);
        CanSelect* p_optimized_defrosting = m_entity_manager.get_select(OPTIMIZED_DEFROSTING);
        if (p_betriebs_modus != nullptr && p_betriebs_modus->state == STATE_SUMMER && p_optimized_defrosting != nullptr) {
            ESP_LOGI(TAG, "on_custom_select(%s, %d) => set m_optimized_defrosting to false", id.c_str(), value);

            p_optimized_defrosting->publish_select_key(0x0);
            m_optimized_defrosting.save(0x0);

            return false; // process sendSet(...)
        }
    }
    return false;
}

void DaikinRotexCanComponent::on_betriebsart(TEntity::TVariant const& current, TEntity::TVariant const& previous) {
    CanSelect* p_betriebs_modus = m_entity_manager.get_select(BETRIEBS_MODUS);
    if (m_optimized_defrosting.value() && p_betriebs_modus != nullptr) {
        if (std::holds_alternative<std::string>(current)) {
            if (std::get<std::string>(current) == STATE_DEFROSTING && p_betriebs_modus->state != STATE_SUMMER) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x05); // Sommer
            } else if (std::get<std::string>(current) == STATE_HEATING && p_betriebs_modus->state != STATE_HEATING) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x03); // Heizen
            } else if (std::get<std::string>(current) == STATE_STANDBY && p_betriebs_modus->state != STATE_HEATING) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x03); // Heizen
            } else if (std::get<std::string>(current) == STATE_DHW_PRODUCTION && std::get<std::string>(previous) == STATE_DEFROSTING && p_betriebs_modus->state != STATE_HEATING) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x03); // Heizen
            }
        } else {
            ESP_LOGE(TAG, "on_betriebsart(): current has no valid string value!");
        }
    }

    if (current != previous) {
        m_low_temperature_spread_timestamp = 0;
    }
}

///////////////// Texts /////////////////
void DaikinRotexCanComponent::custom_request(std::string const& value) {
    const uint32_t can_id = 0x680;
    const bool use_extended_id = false;

    const std::size_t pipe_pos = value.find("|");
    if (pipe_pos != std::string::npos) { // handle response
        uint16_t can_id = Utils::hex_to_uint16(value.substr(0, pipe_pos));
        std::string buffer = value.substr(pipe_pos + 1);
        const TMessage message = Utils::str_to_bytes(buffer);

        m_entity_manager.handle(can_id, message);
    } else { // send custom request
        const TMessage buffer = Utils::str_to_bytes(value);
        if (is_command_set(buffer)) {
            esphome::esp32_can::ESP32Can* pCanbus = m_entity_manager.getCanbus();
            pCanbus->send_data(can_id, use_extended_id, { buffer.begin(), buffer.end() });

            Utils::log("sendGet", "can_id<%s> data<%s>",
                Utils::to_hex(can_id).c_str(), value.c_str(), Utils::to_hex(buffer).c_str());
        }
    }
}

///////////////// Buttons /////////////////
void DaikinRotexCanComponent::dhw_run() {
    const std::string id {"target_hot_water_temperature"};
    TEntity const* pEntity = m_entity_manager.get(id);
    if (pEntity != nullptr) {
        float temp1 {70};
        float temp2 {0};

        temp1 *= pEntity->get_config().divider;

        if (CanNumber const* pNumber = dynamic_cast<CanNumber const*>(pEntity)) {
            temp2 = pNumber->state * pEntity->get_config().divider;
        } else if (CanSelect const* pSelect = dynamic_cast<CanSelect const*>(pEntity)) {
            temp2 = pSelect->getKey(pSelect->state);
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
        TEntity const* pEntity = m_entity_manager.get(index);
        if (pEntity != nullptr) {
            EntityBase* pEntityBase = pEntity->get_entity_base();
            if (CanSensor* pSensor = dynamic_cast<CanSensor*>(pEntityBase)) {
                ESP_LOGI(TAG, "%s: %f", pSensor->get_name().c_str(), pSensor->get_state());
            } else if (CanBinarySensor* pBinarySensor = dynamic_cast<CanBinarySensor*>(pEntityBase)) {
                ESP_LOGI(TAG, "%s: %d", pBinarySensor->get_name().c_str(), pBinarySensor->state);
            } else if (CanNumber* pNumber = dynamic_cast<CanNumber*>(pEntityBase)) {
                ESP_LOGI(TAG, "%s: %f", pNumber->get_name().c_str(), pNumber->state);
            } else if (CanTextSensor* pTextSensor = dynamic_cast<CanTextSensor*>(pEntityBase)) {
                ESP_LOGI(TAG, "%s: %s", pTextSensor->get_name().c_str(), pTextSensor->get_state().c_str());
            } else if (CanSelect* pSelect = dynamic_cast<CanSelect*>(pEntityBase)) {
                ESP_LOGI(TAG, "%s: %s", pSelect->get_name().c_str(), pSelect->state.c_str());
            }
        } else {
            ESP_LOGE(TAG, "Entity with index<%d> not found!", index);
        }
    }
    ESP_LOGI(TAG, "------------------------------------------");
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

    const uint32_t millis = ::millis();
    for (TEntity* pEntity : m_entity_manager.get_entities()) {
        pEntity->update(millis);
    }
    m_thermal_power_sensor->update(millis);
    m_temperature_spread_sensor->update(millis);
}

void DaikinRotexCanComponent::handle(uint32_t can_id, std::vector<uint8_t> const& data) {
    TMessage message;
    std::copy_n(data.begin(), 7, message.begin());
    m_entity_manager.handle(can_id, message);
}

void DaikinRotexCanComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "DaikinRotexCanComponent");
}

void DaikinRotexCanComponent::throwPeriodicError(std::string const& message) {
    call_later([message, this]() {
        ESP_LOGE(TAG, message.c_str());
        throwPeriodicError(message);
    }, POST_SETUP_TIMOUT);
}

bool DaikinRotexCanComponent::is_command_set(TMessage const& message) {
    for (auto& b : message) {
        if (b != 0x00) {
            return true;
        }
    }
    return false;
}

std::string DaikinRotexCanComponent::recalculate_state(EntityBase* pEntity, std::string const& new_state) {
    const uint32_t delay = 2*60*1000; // 2 minutes. HPSU v4 immediately changes the BPV position to 100%, which leads to false positives!

    CanSensor const* tv = m_entity_manager.get_sensor("tv");
    CanSensor const* tvbh = m_entity_manager.get_sensor("tvbh");
    CanSensor const* tr = m_entity_manager.get_sensor("tr");
    CanSensor const* dhw_mixer_position = m_entity_manager.get_sensor("dhw_mixer_position");
    CanSensor const* bpv = m_entity_manager.get_sensor("bypass_valve");
    CanSensor const* flow_rate = m_entity_manager.get_sensor("flow_rate");
    CanSensor const* ta = m_entity_manager.get_sensor("temperature_outside");
    CanTextSensor const* error_code = m_entity_manager.get_text_sensor("error_code");
    CanTextSensor const* p_betriebs_art = m_entity_manager.get_text_sensor(BETRIEBS_ART);
    CanBinarySensor const* state_compressor = m_entity_manager.get_binary_sensor("status_kompressor");

    if (error_code != nullptr && pEntity == error_code) {
        if (tvbh != nullptr && flow_rate != nullptr && flow_rate->state > 600.0f) {
            if (tv != nullptr && dhw_mixer_position != nullptr) {
                if (dhw_mixer_position->state == 0.0f && tvbh->state < (tv->state - m_max_spread.tvbh_tv)) {
                    ESP_LOGE(TAG, "3UV DHW defekt (1) => tvbh: %f, tv: %f, max_spread: %f, bpv: %f, flow_rate: %f",
                        tvbh->state, tv->state, m_max_spread.tvbh_tv, dhw_mixer_position->state, flow_rate->state);
                    return new_state + "|3UV DHW " + DEFECT;
                }
            }
            if (tr != nullptr && bpv != nullptr) {
                if (bpv->state == 100.0f && tr->state < (tvbh->state - m_max_spread.tvbh_tr) && bpv->isChangedInLastMilliseconds(delay)) {
                    ESP_LOGE(TAG, "3UV BPV defekt (1) => tvbh: %f, tr: %f, max_spread: %f, dhw_mixer_pos: %f, flow_rate: %f",
                        tvbh->state, tr->state, m_max_spread.tvbh_tr, bpv->state, flow_rate->state);
                    return new_state + "|3UV BPV " + DEFECT;
                }
            }
        }
        if (p_betriebs_art != nullptr) {
            if (Utils::is_in(p_betriebs_art->state, STATE_DHW_PRODUCTION, STATE_HEATING)) {
                /*
                    Coefficients calculated using the table:
                    Tv  | minimal temperature spread
                    50° | 4.0
                    40° | 3.0
                    35° | 2.5
                    29° | 1.2
                    27° | 0.3
                 */
                const float minValue =
                    -0.00004012 * std::pow(tv->state, 4)
                    + 0.006683 * std::pow(tv->state, 3)
                    - 0.4152 * std::pow(tv->state, 2)
                    + 11.5006 * tv->state
                    - 117.7908;

                ESP_LOGI(TAG, "recalculate_state() temperature_spread: %f, minValue: %f, tv: %f, millis: %d, ts: %d",
                    m_temperature_spread_sensor->state, minValue, tv->state, millis(), m_low_temperature_spread_timestamp);

                if (m_temperature_spread_sensor->state < minValue) {
                    if (m_low_temperature_spread_timestamp > 0) {
                        if (millis() > (m_low_temperature_spread_timestamp + 20*60*1000)) { // The flow temperature (Vorlauf) may sometimes drop suddenly, even before the mode_of_operating is reported.
                            return new_state + "|" + LOW_TEMPERATURE_SPREAD;
                        }
                    } else if (m_low_temperature_spread_timestamp == 0) {
                        m_low_temperature_spread_timestamp = millis();
                    }
                } else {
                    m_low_temperature_spread_timestamp = 0u;
                }
            }
        }
    }
    return new_state;
}

} // namespace daikin_rotex_can
} // namespace esphome