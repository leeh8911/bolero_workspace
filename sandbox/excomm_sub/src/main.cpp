#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "bolero/arg_parser.hpp"
#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"
#include "bolero/logger.hpp"
#include "bolero/module.hpp"

struct MyData {
    size_t value;
    std::array<uint8_t, 10> extra{};
};

std::string to_string(const MyData& data) {
    std::string s = "{";
    for (const auto& byte : data.extra) {
        s += fmt::format("{:02X}, ", byte);
    }
    if (!data.extra.empty()) {
        s.erase(s.size() - 2);
    }
    s += "}";

    return fmt::format("MyData{{ value: {}, extra_size: {} }}", data.value, s);
}

class ExcommSubModule : public bolero::Module {
   public:
    using bolero::Module::Module;  // 생성자 상속
    ExcommSubModule(const bolero::Config& config) : bolero::Module(config) {
        BOLERO_LOG_INFO("ExcommSubModule initialized with config: {}", to_string(config));

        this->CreateSubscriber<size_t>("test/int", [](const size_t& msg) {
            std::stringstream ss;
            ss << std::this_thread::get_id();
            BOLERO_LOG_INFO("{} - Received [{}]: {}", ss.str(), "test/int", msg);
        });

        this->CreateSubscriber<MyData>("test/struct", [](const MyData& msg) {
            std::stringstream ss;
            ss << std::this_thread::get_id();
            BOLERO_LOG_INFO("{} - Received [{}]: {}", ss.str(), "test/struct", to_string(msg));
        });
    }

    void Run() override { BOLERO_LOG_INFO("ExcommSubModule is running!"); }
};
REGIST_CLASS(MODULE_FACTORY, ExcommSubModule);

int main(int argc, char* argv[]) {
    bolero::ArgParser parser(argc, argv);

    auto args = parser.Parse();

    auto config = bolero::Config::FromFile("sandbox/excomm_sub/config/config.json");

    auto module_ptr = MAKE_CLASS(MODULE_FACTORY, config);

    module_ptr->Run();

    module_ptr->Wait();

    return 0;
}