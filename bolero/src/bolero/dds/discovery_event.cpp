#include "bolero/dds/discovery_event.hpp"

#include "nlohmann/json.hpp"

namespace bolero {
nlohmann::json ToJson(const DiscoveryEvent& evt) {
    nlohmann::json j;
    j["msg_type"] = evt.msg_type;
    j["topic"] = evt.topic;
    j["node_id"] = evt.node_id;
    j["ip"] = evt.ip;
    j["data_port"] = evt.data_port;
    return j;
}

DiscoveryEvent FromJson(const nlohmann::json& j) {
    DiscoveryEvent evt;
    evt.msg_type = j.at("msg_type").get<std::string>();
    evt.topic = j.at("topic").get<std::string>();
    evt.node_id = j.at("node_id").get<std::string>();
    evt.ip = j.at("ip").get<std::string>();
    evt.data_port = j.at("data_port").get<uint16_t>();
    return evt;
}
}  // namespace bolero