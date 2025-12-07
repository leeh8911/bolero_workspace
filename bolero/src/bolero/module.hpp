#pragma once

#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"
#include "bolero/scheduler.hpp"

namespace bolero {

class Module {
   public:
    explicit Module(const Config& config_);

    virtual ~Module() = default;

    /// 파생 클래스에서 Task 등록 등 초기화 작업
    virtual void Run() = 0;

    /// 기본 구현: Scheduler 루프 진입
    virtual void Wait();

    /// 외부에서 graceful shutdown 하고 싶을 때
    virtual void Stop();

   protected:
    const Config& GetConfig() const;

    Scheduler& GetScheduler();
    const Scheduler& GetScheduler() const;

   private:
    Config config;
    Scheduler scheduler;
};

}  // namespace bolero

BUILD_FACTORY(MODULE_FACTORY, bolero::Module);