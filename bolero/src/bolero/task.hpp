#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace bolero {

class Task;
using TaskPtr = std::shared_ptr<Task>;

class Task {
   public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    using Func = std::function<void()>;

    Task(std::string name, Duration period, Func func, bool repeat)
        : name_(std::move(name)),
          period_(period),
          func_(std::move(func)),
          repeat_(repeat),
          next_deadline_(Clock::now()) {}

    const std::string& name() const { return name_; }
    TimePoint next_deadline() const { return next_deadline_; }
    Duration period() const { return period_; }
    bool repeat() const { return repeat_; }

    void ExecuteOnce() {
        if (func_) {
            func_();
        }
    }

    /// 주기적인 Task일 때, 다음 실행 시점을 갱신
    void ScheduleNextFrom(TimePoint now) {
        if (!repeat_) {
            return;
        }
        if (period_.count() <= 0) {
            next_deadline_ = now;
        } else {
            next_deadline_ = now + period_;
        }
    }

   private:
    std::string name_;
    Duration period_;
    Func func_;
    bool repeat_;
    TimePoint next_deadline_;
};

}  // namespace bolero
