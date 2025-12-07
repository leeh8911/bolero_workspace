#include <filesystem>
#include <iostream>
#include <string>

#include "bolero/arg_parser.hpp"
#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"
#include "bolero/module.hpp"

class BasicModule : public bolero::Module {
   public:
    BasicModule(const bolero::Config& config) : bolero::Module(config) {}
    ~BasicModule() override = default;

    void Run() override { std::cout << "Running BasicModule..." << std::endl; }
};
REGIST_CLASS(MODULE_FACTORY, BasicModule);

int main(int argc, char* argv[]) {
    bolero::ArgParser parser(argc, argv);

    auto args = parser.Parse();

    auto config = bolero::Config::FromFile("sandbox/basic_module/config/config.json");
    std::cout << "Config file: " << config << std::endl;

    auto module_ptr = MAKE_CLASS(MODULE_FACTORY, config);

    return 0;
}