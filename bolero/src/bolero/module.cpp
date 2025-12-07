#include "bolero/module.hpp"

#include "bolero/config.hpp"
#include "bolero/task.hpp"

namespace bolero {

Module::Module(const Config& config) {
    this->task = Task::Create(config["type"], config["period"], [this]() { this->Run(); });
}
void Module::operator()() {
    this->task->Run();
}

}  // namespace bolero