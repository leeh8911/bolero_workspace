#pragma once

#include <string>

#include "bolero/class_factory.hpp"

namespace bolero {
class Module {
   public:
    Module() = default;
    virtual ~Module() = default;

    virtual std::string Name() const = 0;

   private:
};
BUILD_FACTORY(ModuleFactory, Module);

}  // namespace bolero