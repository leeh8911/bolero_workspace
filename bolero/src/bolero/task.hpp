#pragma once

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
// task.hpp (개략)

namespace bolero {

class Task;
using TaskPtr = std::shared_ptr<Task>;

class Task {
   public:
    using Clock = std::chrono::steady_clock;

    template <typename FUNC>
    static TaskPtr Create(std::string name, std::size_t period_ms, FUNC&& func) {
        return std::make_shared<Task>(std::move(name), period_ms, std::forward<FUNC>(func));
    }

    template <typename FUNC>
    Task(std::string name_, std::size_t period_ms_, FUNC&& func_)
        : name(std::move(name_)), period_ms(period_ms_), func(std::forward<FUNC>(func_)), stop_flag(false) {
        thread = std::thread([this]() {
            while (!stop_flag.load()) {
                auto start = Clock::now();
                func();  // BasicModule::Run() 호출

                auto end = Clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                if (elapsed_ms < static_cast<long long>(period_ms)) {
                    auto sleep_ms = period_ms - static_cast<std::size_t>(elapsed_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                }
            }
        });
    }

    ~Task() { Stop(); }

    void Stop() {
        stop_flag.store(true);
        if (thread.joinable()) {
            thread.join();
        }
    }

   private:
    std::string name;
    std::size_t period_ms{1};
    std::function<void()> func;
    std::thread thread;
    std::atomic<bool> stop_flag{false};
};

}  // namespace bolero
