#pragma once

#include "esphome/core/defines.h"

#ifdef USE_DAIKIN_ROTEX_CAN_MANIFEST

#include "esphome/components/api/api_server.h"
#include "esphome/components/api/user_services.h"
#include "esphome/core/helpers.h"
#include "esphome/core/string_ref.h"
#include <cstring>
#include <vector>

namespace esphome {
namespace daikin_rotex_can {

/// Native API action with one integer argument "page" that answers with that page of the manifest
/// (JSON built at compile time). Registered by the component itself, so the user's YAML needs no
/// "api: actions:" entry.
class ManifestAction : public api::UserServiceDescriptor {
public:
    ManifestAction(const char* name, std::vector<const char*> pages)
    : m_name(name)
    , m_pages(std::move(pages))
    , m_key(fnv1_hash(name)) {
    }

    api::ListEntitiesServicesResponse encode_list_service_response() override {
        api::ListEntitiesServicesResponse msg;
        // StringRef up to 2026.10, ProgmemStringRef from 2026.11: both are constructible from a StringRef
        msg.name = decltype(msg.name)(StringRef(m_name));
        msg.key = m_key;
        msg.supports_response = api::enums::SUPPORTS_RESPONSE_ONLY;
        msg.args.init(1);
        auto& arg = msg.args.emplace_back();
        arg.type = api::enums::SERVICE_ARG_TYPE_INT;
        arg.name = decltype(arg.name)(StringRef(PAGE_ARG));
        return msg;
    }

    // Called by clients without response support: nothing to do without a call id
    bool execute_service(const api::ExecuteServiceRequest& req) override {
        return req.key == m_key;
    }

    bool execute_service(const api::ExecuteServiceRequest& req, uint32_t action_call_id) override {
        if (req.key != m_key) {
            return false;
        }
        if (action_call_id == 0) {
            return true;
        }

        // Read like api::get_execute_arg_value<int32_t>(), which is not compiled into every ESPHome
        // version unless the YAML itself defines actions
        int32_t page = -1;
        if (req.args.size() == 1) {
            page = req.args[0].legacy_int != 0 ? req.args[0].legacy_int : req.args[0].int_;
        }
        if (page >= 0 && static_cast<size_t>(page) < m_pages.size()) {
            const char* json = m_pages[page];
            api::global_api_server->send_action_response(
                action_call_id, true, StringRef(""),
                reinterpret_cast<const uint8_t*>(json), std::strlen(json));
        } else {
            api::global_api_server->send_action_response(action_call_id, false, StringRef("page out of range"));
        }
        api::global_api_server->unregister_active_action_call(action_call_id);
        return true;
    }

private:
    static constexpr const char* PAGE_ARG = "page";

    const char* m_name;
    std::vector<const char*> m_pages;
    uint32_t m_key;
};

}  // namespace daikin_rotex_can
}  // namespace esphome

#endif  // USE_DAIKIN_ROTEX_CAN_MANIFEST
