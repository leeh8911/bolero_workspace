#pragma once

#include <string_view>

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

namespace bolero::config_utils {

nlohmann::json YamlToJson(const YAML::Node& yaml);

nlohmann::json FromJsonFile(std::string_view path);
nlohmann::json FromYamlFile(std::string_view path);

void ToJsonFile(const nlohmann::json& json, std::string_view path);
void ToYamlFile(const nlohmann::json& json, std::string_view path);
}  // namespace bolero::config_utils