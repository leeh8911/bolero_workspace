#include "bolero/config.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

#include "bolero/config/config_utils.hpp"

namespace bolero {

Config Config::FromFile(std::string_view path) {
    Config config;

    if (path.ends_with(".json")) {
        auto json = ::bolero::config_utils::FromJsonFile(path);
        config.json = std::make_shared<nlohmann::json>(json);
    } else if (path.ends_with(".yaml")) {
        auto json = ::bolero::config_utils::FromYamlFile(path);
        config.json = std::make_shared<nlohmann::json>(json);
    } else {
        throw std::runtime_error("Unsupported config file format: " + std::string(path));
    }

    return config;
}

void Config::ToFile(std::string_view path) const {
    if (path.ends_with(".json")) {
        ::bolero::config_utils::ToJsonFile(*json, path);
    } else if (path.ends_with(".yaml")) {
        ::bolero::config_utils::ToYamlFile(*json, path);
    } else {
        throw std::runtime_error("Unsupported config file format: " + std::string(path));
    }
}

Config::Proxy Config::operator[](const std::string& key) {
    if (!this->json) {
        this->json = std::make_shared<nlohmann::json>(nlohmann::json::object());
    }
    return Config::Proxy{(*this->json)[key]};
}

Config::Proxy Config::operator[](const std::string& key) const {
    return const_cast<Config*>(this)->operator[](key);
}

bool Config::operator==(const Config& other) const {
    if (this->json == nullptr || other.json == nullptr) {
        return false;
    }
    return *json == *(other.json);
}
}  // namespace bolero