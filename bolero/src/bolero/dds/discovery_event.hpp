#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace bolero {

struct DiscoveryEvent {
    std::string msg_type;  // "PUB_ANNOUNCE" / "SUB_ANNOUNCE"
    std::string topic;
    std::string node_id;
    std::string ip;
    uint16_t data_port;
};

nlohmann::json ToJson(const DiscoveryEvent& evt);
DiscoveryEvent FromJson(const nlohmann::json& j);

}  // namespace bolero