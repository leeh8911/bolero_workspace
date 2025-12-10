#include "bolero/module.hpp"

#include "bolero/config.hpp"
#include "bolero/dds/node.hpp"
#include "bolero/scheduler.hpp"
#include "bolero/task.hpp"

namespace bolero {
Module::Module(const Config& config_) : config(config_), scheduler(), node(nullptr) {
    this->node = std::make_shared<Node>(std::string(config_["type"]));
    this->node->Start();
}
void Module::Wait() {
    this->scheduler.Run();
}

/// 외부에서 graceful shutdown 하고 싶을 때
void Module::Stop() {
    this->scheduler.Stop();
}

const Config& Module::GetConfig() const {
    return this->config;
}

Scheduler& Module::GetScheduler() {
    return this->scheduler;
}
const Scheduler& Module::GetScheduler() const {
    return this->scheduler;
}

}  // namespace bolero