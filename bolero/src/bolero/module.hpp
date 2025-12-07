#pragma once

#include <string>

#include "bolero/class_factory.hpp"
#include "bolero/task.hpp"

namespace bolero {
class Module {
   public:
    Module(const Config& config);
    virtual ~Module() = default;

    void operator()();

    virtual void Run() = 0;

   private:
    TaskPtr task{nullptr};
};

}  // namespace bolero
BUILD_FACTORY(MODULE_FACTORY, bolero::Module);