#include <filesystem>
#include <iostream>
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
    s.erase(s.size() - 2);  // 마지막 ", " 제거
    s += "}";

    return fmt::format("MyData{{ value: {}, extra_size: {} }}", data.value, s);
}

class ExcommPubModule : public bolero::Module {
   public:
    using bolero::Module::Module;  // 생성자 상속
    ExcommPubModule(const bolero::Config& config) : bolero::Module(config) {
        BOLERO_LOG_INFO("ExcommPubModule initialized with config: {}", to_string(config));
    }

    void Run() override {
        BOLERO_LOG_INFO("ExcommPubModule is running!");

        auto& scheduler = this->GetScheduler();

        std::unordered_map<std::string, std::shared_ptr<bolero::Publisher>> pubs{};

        pubs.emplace("int", this->CreatePublisher("test/int"));
        scheduler.AddPeriodicTask("int_pub", std::chrono::milliseconds(1000), [pub = pubs["int"]]() {
            size_t value = std::time(nullptr);
            std::stringstream ss;
            ss << std::this_thread::get_id();
            BOLERO_LOG_INFO("{} - Send [{}]: {}", ss.str(), "test/int", value);
            pub->Publish<size_t>(std::time(nullptr));
        });

        pubs.emplace("struct", this->CreatePublisher("test/struct"));
        scheduler.AddPeriodicTask("struct_pub", std::chrono::milliseconds(500), [pub = pubs["struct"]]() {
            MyData data;
            for (size_t iter = 0; iter < data.extra.size(); ++iter) {
                data.extra[iter] = std::rand() % 256;
            }

            std::stringstream ss;
            ss << std::this_thread::get_id();
            BOLERO_LOG_INFO("{} - Send [{}]: {}", ss.str(), "test/struct", to_string(data));
            pub->Publish<MyData>(data);
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