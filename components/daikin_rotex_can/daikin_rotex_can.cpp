#include "esphome/components/daikin_rotex_can/daikin_rotex_can.h"
#include "esphome/components/daikin_rotex_can/entity.h"
#include "esphome/components/daikin_rotex_can/sensors.h"
#include "esphome/components/daikin_rotex_can/translations.h"
#include <string>
#include <vector>

namespace esphome {
namespace daikin_rotex_can {

static const char* TAG = "daikin_rotex_can";
static const char* BETRIEBS_ART = "mode_of_operating";
static const char* BETRIEBS_MODUS = "operating_mode";
static const char* OPTIMIZED_DEFROSTING = "optimized_defrosting";
static const char* TEMPERATURE_ANTIFREEZE = "temperature_antifreeze";   // T-Frostschutz
static const char* TEMPERATURE_ANTIFREEZE_OFF = translate("off").c_str();
static const uint32_t POST_SETUP_TIMOUT = 15*1000;

DaikinRotexCanComponent::DaikinRotexCanComponent()
: m_entity_manager()
, m_later_calls()
, m_dhw_run_lambdas()
, m_optimized_defrosting(false)
, m_project_git_hash_sensor(nullptr)
, m_project_git_hash()
, m_thermal_power_sensor(nullptr)
{
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
    std::string const& update_entity = pEntity->get_update_entity();
    if (!update_entity.empty()) {
        call_later([update_entity, this](){
            updateState(update_entity);
        });
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
                Utils::log(TAG, "set %s: %d", OPTIMIZED_DEFROSTING, m_optimized_defrosting.value());
            }
        }
    }
}

void DaikinRotexCanComponent::updateState(std::string const& id) {
    if (id == "thermal_power") {
        update_thermal_power();
    }
}

void DaikinRotexCanComponent::update_thermal_power() {
    text_sensor::TextSensor* p_betriebs_art = m_entity_manager.get_text_sensor(BETRIEBS_ART);

    if (p_betriebs_art != nullptr && m_thermal_power_sensor != nullptr) {
        CanSensor const* flow_rate = m_entity_manager.get_sensor("flow_rate");
        if (flow_rate != nullptr) {
            CanSensor const* tvbh = m_entity_manager.get_sensor("tvbh");
            CanSensor const* tv = m_entity_manager.get_sensor("tv");
            CanSensor const* tr = m_entity_manager.get_sensor("tr");

            float value = 0;
            if (p_betriebs_art->state == translate("hot_water_production") && tv != nullptr && tr != nullptr) {
                value = (tv->state - tr->state) * (4.19 * flow_rate->state) / 3600.0f;
            } else if ((p_betriebs_art->state == translate("heating") || p_betriebs_art->state == translate("cooling")) && tvbh != nullptr && tr != nullptr) {
                value = (tvbh->state - tr->state) * (4.19 * flow_rate->state) / 3600.0f;
            }
            m_thermal_power_sensor->publish_state(value);
        }
    }
}

bool DaikinRotexCanComponent::on_custom_select(std::string const& id, uint8_t value) {
    if (id == OPTIMIZED_DEFROSTING) {
        Utils::log(TAG, "%s: %d", OPTIMIZED_DEFROSTING, value);
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
    }
    return false;
}

void DaikinRotexCanComponent::on_betriebsart(TEntity::TVariant const& current, TEntity::TVariant const& previous) {
    CanSelect* p_betriebs_modus = m_entity_manager.get_select(BETRIEBS_MODUS);
    if (m_optimized_defrosting.value() && p_betriebs_modus != nullptr) {
        if (std::holds_alternative<std::string>(current)) {
            if (std::get<std::string>(current) == translate("defrosting") && p_betriebs_modus->state != translate("summer")) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x05); // Sommer
            } else if (std::get<std::string>(current) == translate("heating") && p_betriebs_modus->state != translate("heating")) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x03); // Heizen
            } else if (std::get<std::string>(current) == translate("standby") && p_betriebs_modus->state != translate("heating")) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x03); // Heizen
            } else if (std::get<std::string>(current) == translate("hot_water_production") && std::get<std::string>(previous) == translate("defrosting") && p_betriebs_modus->state != translate("heating")) {
                m_entity_manager.sendSet(p_betriebs_modus->get_name(), 0x03); // Heizen
            }
        } else {
            ESP_LOGE(TAG, "on_betriebsart(): current has no valid string value!");
        }
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
        if (millis() > it->second) {
            it->first();
            it = m_later_calls.erase(it);
        } else {
            ++it;
        }
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

std::string DaikinRotexCanComponent::recalculate_state(EntityBase* pEntity, std::string const& new_state) const {
    const uint32_t delay = 2*60*1000; // 2 minutes. HPSU v4 immediately changes the BPV position to 100%, which leads to false positives!

    CanSensor const* tv = m_entity_manager.get_sensor("tv");
    CanSensor const* tvbh = m_entity_manager.get_sensor("tvbh");
    CanSensor const* tr = m_entity_manager.get_sensor("tr");
    CanSensor const* dhw_mixer_position = m_entity_manager.get_sensor("dhw_mixer_position");
    CanSensor const* bpv = m_entity_manager.get_sensor("bypass_valve");
    CanSensor const* flow_rate = m_entity_manager.get_sensor("flow_rate");
    CanTextSensor const* error_code = m_entity_manager.get_text_sensor("error_code");

    if (tv && tvbh && tr && dhw_mixer_position && bpv && flow_rate) {
        ESP_LOGI(TAG, "tv: %f, tvbh: %f, tr: %f, TvBH-Tv: %f, Tr-TvBH: %f, dhw: %f, bpv: %f, flow: %f, delay: %d, millis: %d, bpv_change: %d",
            tv->state, tvbh->state, tr->state, m_max_spread.tvbh_tv, m_max_spread.tvbh_tr, dhw_mixer_position->state, bpv->state, flow_rate->state,
                delay, millis(), (bpv->getLastValueChange()));
    }

    if (pEntity == error_code && error_code != nullptr) {
        if (tvbh != nullptr && tv != nullptr && dhw_mixer_position != nullptr && flow_rate != nullptr) {
            if (tvbh->state > (tv->state + m_max_spread.tvbh_tv) && dhw_mixer_position->state == 0.0f && flow_rate->state > 600.0f) {
                ESP_LOGE(TAG, "3UV DHW defekt => tvbh: %f, tv: %f, max_spread: %f, bpv: %f, flow_rate: %f",
                    tvbh->state, tv->state, m_max_spread.tvbh_tv, dhw_mixer_position->state, flow_rate->state);
                return new_state + "|3UV DHW " + translate("defect");
            }
        }
        if (tvbh != nullptr && tr != nullptr && bpv != nullptr && flow_rate != nullptr) {
            if (tvbh->state > (tr->state + m_max_spread.tvbh_tr) && bpv->state == 100.0f && millis() > (bpv->getLastValueChange() + delay) && flow_rate->state > 600.0f) {
                ESP_LOGE(TAG, "3UV BPV defekt => tvbh: %f, tr: %f, max_spread: %f, dhw_mixer_pos: %f, flow_rate: %f",
                    tvbh->state, tr->state, m_max_spread.tvbh_tr, bpv->state, flow_rate->state);
                return new_state + "|3UV BPV "+ translate("defect");
            }
        }
    }
    return new_state;
}

} // namespace daikin_rotex_can
} // namespace esphome