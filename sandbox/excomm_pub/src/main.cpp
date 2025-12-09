#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "bolero/arg_parser.hpp"
#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"
#include "bolero/logger.hpp"
#include "bolero/module.hpp"

class ExcommPubModule : public bolero::Module {
   public:
    using bolero::Module::Module;  // 생성자 상속
    ExcommPubModule(const bolero::Config& config) : bolero::Module(config) {
        BOLERO_LOG_INFO("ExcommPubModule initialized with config: {}", to_string(config));
    }

    void Run() override {
        BOLERO_LOG_INFO("ExcommPubModule is running!");

        const auto& config = this->GetConfig();
        auto& scheduler = this->GetScheduler();

        auto pub = this->CreatePublisher("test/name");

        // 1초마다 메시지를 출력하는 주기 Task 등록
        scheduler.AddPeriodicTask("print_hello", std::chrono::milliseconds(config["period_ms"]), [pub]() {
            size_t value = std::time(nullptr);
            BOLERO_LOG_INFO("Publishing message: {}", value);
            pub->Publish<size_t>(std::time(nullptr));
        });
    }
};
REGIST_CLASS(MODULE_FACTORY, ExcommPubModule);

int main(int argc, char* argv[]) {
    bolero::ArgParser parser(argc, argv);

    auto args = parser.Parse();

    auto config = bolero::Config::FromFile("sandbox/excomm_pub/config/config.json");
    std::cout << "Config file: " << config << std::endl;

    auto module_ptr = MAKE_CLASS(MODULE_FACTORY, config);

    module_ptr->Run();

    module_ptr->Wait();

    return 0;
}