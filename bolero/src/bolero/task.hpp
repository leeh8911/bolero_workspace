#pragma once

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace bolero {

class Task;
using TaskPtr = std::shared_ptr<Task>;

class Task {
   public:
    template <typename FUNC>
    Task(std::string name_, size_t period, FUNC&& f)
        : name(std::move(name_)), period_ms(period), func(std::forward<FUNC>(f)) {}

    template <typename FUNC>
    static TaskPtr Create(std::string name, size_t period, FUNC&& func) {
        return std::make_shared<Task>(std::move(name), period, std::forward<FUNC>(func));
    }

    void Run() {
        auto start = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex);
        if (this->started) {
            return;  // 중복 실행 방지
        }
        this->started = true;

        this->thread = std::thread([this]() {
            func();  // 저장해둔 함수 실행

            {
                std::lock_guard<std::mutex> lock(this->mutex);
                this->ready = true;
            }
            this->cv.notify_one();
        });

        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (static_cast<size_t>(elapsed) < this->period_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->period_ms - elapsed));
        }
    }

    void Wait() {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->cv.wait(lock, [this]() { return this->ready; });
    }

    ~Task() {
        if (this->thread.joinable()) {
            this->thread.join();
        }
    }

   private:
    Task() = delete;
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    std::thread thread;
    std::string name;
    size_t period_ms{1};
    std::function<void()> func;
    std::condition_variable cv;
    std::mutex mutex;
    bool started{false};
    bool ready{false};
};

}  // namespace bolero