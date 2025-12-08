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
        BOLERO_LOG_INFO("ExcommSubModule initialized with config: {}", config);
    }

    void Run() override {
        BOLERO_LOG_INFO("ExcommSubModule is running!");

        const auto& config = this->GetConfig();
        auto& scheduler = this->GetScheduler();

        this->CreateSubscriber<std::string>(
            "test/name", [](const std::string& msg) { BOLERO_LOG_INFO("Received message: {}", msg); });

        // // 1초마다 메시지를 출력하는 주기 Task 등록
        // scheduler.AddPeriodicTask("print_hello", std::chrono::milliseconds(config["period_ms"]), [pub]() {
        //     pub->publish<std::string>("ExcommPub: " + std::to_string(std::time(nullptr)));
        // });
    }
};
REGIST_CLASS(MODULE_FACTORY, BasicModule);

int main(int argc, char* argv[]) {
    bolero::ArgParser parser(argc, argv);

    auto args = parser.Parse();

    auto config = bolero::Config::FromFile("sandbox/basic_module/config/config.json");
    std::cout << "Config file: " << config << std::endl;

    auto module_ptr = MAKE_CLASS(MODULE_FACTORY, config);

    module_ptr->Run();

    module_ptr->Wait();

    return 0;
}