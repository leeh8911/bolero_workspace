#include <filesystem>

#include "bolero/config.hpp"
#include <gtest/gtest.h>

class TestConfig : public ::testing::Test {
   public:
    void SetUp() override {}
    void TearDown() override {
        for (const auto& [_, path] : config_map) {
            if (std::filesystem::exists(path)) {
                std::filesystem::remove(path);
            }
        }
    }
    std::unordered_map<std::string, std::string> config_map{
        {"json", "./temp/test_config" + std::to_string(__COUNTER__) + ".json"},
        {"yaml", "./temp/test_config" + std::to_string(__COUNTER__) + ".yaml"},
    };
};

TEST_F(TestConfig, Basic_Json_Config_Save_Load) {
    bolero::Config config;

    config["int"] = 42;
    config["double"] = 3.14;
    config["string"] = "hello";
    config["bool"] = true;
    config["list"] = {1, 2, 3};
    config["map"] = {{"key1", "value1"}, {"key2", "value2"}};

    config.ToFile(this->config_map["json"]);

    auto loaded_config = bolero::Config::FromFile(this->config_map["json"]);

    EXPECT_EQ(loaded_config, config);
}
TEST_F(TestConfig, Basic_Yaml_Config_Save_Load) {
    bolero::Config config;

    config["int"] = 42;
    config["double"] = 3.14;
    config["string"] = "hello";
    config["bool"] = true;
    config["list"] = {1, 2, 3};
    config["map"] = {{"key1", "value1"}, {"key2", "value2"}};

    config.ToFile(this->config_map["yaml"]);

    auto loaded_config = bolero::Config::FromFile(this->config_map["yaml"]);

    EXPECT_EQ(loaded_config, config);
}