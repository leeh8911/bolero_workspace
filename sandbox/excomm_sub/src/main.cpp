#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "bolero/arg_parser.hpp"
#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"
#include "bolero/logger.hpp"
#include "bolero/module.hpp"

class ExcommSubModule : public bolero::Module {
   public:
    using bolero::Module::Module;  // 생성자 상속
    ExcommSubModule(const bolero::Config& config) : bolero::Module(config) {
        BOLERO_LOG_INFO("ExcommSubModule initialized with config: {}", to_string(config));
    }

    void Run() override {
        BOLERO_LOG_INFO("ExcommSubModule is running!");

        this->CreateSubscriber<size_t>(
            "test/name", [](const size_t& msg) { BOLERO_LOG_INFO("Received message: {}", msg); });
    }
};
REGIST_CLASS(MODULE_FACTORY, ExcommSubModule);

int main(int argc, char* argv[]) {
    bolero::ArgParser parser(argc, argv);

    auto args = parser.Parse();

    auto config = bolero::Config::FromFile("sandbox/excomm_sub/config/config.json");
    std::cout << "Config file: " << config << std::endl;

    auto module_ptr = MAKE_CLASS(MODULE_FACTORY, config);

    module_ptr->Run();

    module_ptr->Wait();

    return 0;
}