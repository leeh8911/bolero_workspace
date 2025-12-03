#include "bolero/config.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

namespace bolero {

nlohmann::json YamlToJson(const YAML::Node& yaml) {
    if (yaml.IsScalar()) {
        try {
            return yaml.as<int>();
        } catch (...) {
        }
        try {
            return yaml.as<double>();
        } catch (...) {
        }
        try {
            return yaml.as<bool>();
        } catch (...) {
        }

        return yaml.as<std::string>();
    } else if (yaml.IsSequence()) {
        nlohmann::json json = nlohmann::json::array();
        for (const auto& item : yaml) {
            json.push_back(YamlToJson(item));
        }
        return json;
    } else if (yaml.IsMap()) {
        nlohmann::json json = nlohmann::json::object();
        for (const auto& item : yaml) {
            json[item.first.as<std::string>()] = YamlToJson(item.second);
        }
        return json;
    }
    return nullptr;
}

nlohmann::json FromJsonFile(std::string_view path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Config file does not exist: " + std::string(path));
    }

    std::ifstream file(path.data());

    nlohmann::json json;
    file >> json;

    return json;
}

void ToJsonFile(const nlohmann::json& json, std::string_view path) {
    auto parent_path = std::filesystem::path(path).parent_path();
    if (!std::filesystem::exists(parent_path)) {
        std::filesystem::create_directories(parent_path);
    }
    std::ofstream file(path.data());
    file << json.dump(4);
}

nlohmann::json FromYamlFile(std::string_view path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Config file does not exist: " + std::string(path));
    }

    YAML::Node yaml = YAML::LoadFile(path.data());

    return YamlToJson(yaml);
}

void ToYamlFile(const nlohmann::json& json, std::string_view path) {
    auto parent_path = std::filesystem::path(path).parent_path();
    if (!std::filesystem::exists(parent_path)) {
        std::filesystem::create_directories(parent_path);
    }
    YAML::Node yaml = YAML::Load(json.dump());
    std::ofstream file(path.data());
    file << yaml;
}

Config Config::FromFile(std::string_view path) {
    Config config;

    if (path.ends_with(".json")) {
        auto json = FromJsonFile(path);
        config.json = std::make_shared<nlohmann::json>(json);
    } else if (path.ends_with(".yaml")) {
        auto json = FromYamlFile(path);
        config.json = std::make_shared<nlohmann::json>(json);
    } else {
        throw std::runtime_error("Unsupported config file format: " + std::string(path));
    }

    return config;
}

void Config::ToFile(std::string_view path) const {
    if (path.ends_with(".json")) {
        ToJsonFile(*json, path);
    } else if (path.ends_with(".yaml")) {
        ToYamlFile(*json, path);
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