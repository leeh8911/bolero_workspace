#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "bolero/arg_parser.hpp"
#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"
#include "bolero/logger.hpp"
#include "bolero/module.hpp"

class BasicModule : public bolero::Module {
   public:
    using bolero::Module::Module;  // 생성자 상속

    void Run() override {
        std::cout << "Running BasicModule..." << std::endl;

        const auto& config = this->GetConfig();
        auto& scheduler = this->GetScheduler();

        // 1초마다 메시지를 출력하는 주기 Task 등록
        scheduler.AddPeriodicTask("print_hello", std::chrono::milliseconds(config["period_ms"]), []() {
            BOLERO_LOG_TRACE("Hello from BasicModule!");
            BOLERO_LOG_DEBUG("Hello from BasicModule!");
            BOLERO_LOG_INFO("Hello from BasicModule!");
            BOLERO_LOG_WARN("Hello from BasicModule!");
            BOLERO_LOG_ERROR("Hello from BasicModule!");
            BOLERO_LOG_CRITICAL("Hello from BasicModule!");
        });

        // 필요하면 one-shot task도 등록 가능
        scheduler.AddOneShotTask("one_shot",
                                 []() { std::cout << "[BasicModule] one_shot executed" << std::endl; });
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