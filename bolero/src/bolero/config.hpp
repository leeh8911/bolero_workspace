#pragma once

#include <iostream>
#include <memory>
#include <string_view>

#include <nlohmann/json.hpp>

namespace bolero {
class Config {
   public:
    struct Proxy {
        explicit Proxy(nlohmann::json& j) : json(j) {}

        template <typename T>
        operator T() const {
            return json.get<T>();
        }

        template <typename T>
        Proxy& operator=(const T& value) {
            json = value;
            return *this;
        }

        Proxy& operator=(const nlohmann::json& value) {
            json = value;
            return *this;
        }

        nlohmann::json& json;
    };

    Config() = default;

    static Config FromFile(std::string_view path);
    static Config FromJsonString(std::string_view json_str);

    void ToFile(std::string_view path) const;

    Proxy operator[](const std::string& key);
    Proxy operator[](const std::string& key) const;

    bool operator==(const Config& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Config& config);  // Only cout

   private:
    std::shared_ptr<nlohmann::json> json{nullptr};
};

inline std::ostream& operator<<(std::ostream& os, const Config& config) {
    if (config.json) {
        os << config.json->dump(4);
    } else {
        os << "{}";
    }
    return os;
}
}  // namespace bolero