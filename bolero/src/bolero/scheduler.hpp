#pragma once

#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "bolero/task.hpp"

namespace bolero {

class Scheduler {
   public:
    using TaskId = std::size_t;
    using Clock = Task::Clock;

    Scheduler() = default;
    ~Scheduler() = default;

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    /// 주기적인 Task 등록
    template <typename FUNC>
    TaskId AddPeriodicTask(std::string name, Task::Duration period, FUNC&& func) {
        return AddTaskImpl(std::move(name), period, std::forward<FUNC>(func),
                           /*repeat=*/true);
    }

    /// 한 번만 실행되는 Task
    template <typename FUNC>
    TaskId AddOneShotTask(std::string name, FUNC&& func) {
        return AddTaskImpl(std::move(name), Task::Duration::zero(), std::forward<FUNC>(func),
                           /*repeat=*/false);
    }

    /// Task 취소
    void Cancel(TaskId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.erase(id);
        cv_.notify_all();
    }

    /// 메인 루프 (blocking)
    ///
    /// - 가장 가까운 deadline을 가진 Task를 골라 실행
    /// - 주기 Task는 deadline 갱신 후 유지
    /// - one-shot Task는 실행 후 삭제
    /// - Stop()이 호출되면 루프 종료
    void Run() {
        std::unique_lock<std::mutex> lock(mutex_);

        while (!stop_) {
            if (tasks_.empty()) {
                // Task가 없으면 일단 대기
                cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                if (stop_) {
                    break;
                }
            }

            auto now = Clock::now();

            // 가장 빠른 next_deadline을 가진 Task 찾기
            auto it = FindNextReadyTaskLocked(now);
            if (it == tasks_.end()) {
                // 아직 실행 시점이 안 된 Task들만 있으면,
                // 그 중 가장 이른 deadline까지 wait_until
                auto next_tp = FindEarliestDeadlineLocked();
                if (next_tp == Task::TimePoint::max()) {
                    // 방어적 코드: 이런 경우는 거의 없음
                    cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                    continue;
                }

                cv_.wait_until(lock, next_tp, [this]() { return stop_; });
                continue;
            }

            // 이제 바로 실행 가능한 Task
            // TaskId id = it->first;
            TaskPtr task = it->second;

            // one-shot이면 미리 제거
            bool repeat = task->repeat();
            if (!repeat) {
                tasks_.erase(it);
            } else {
                task->ScheduleNextFrom(now);
            }

            // 락을 잠깐 풀고 실제 함수 실행
            lock.unlock();
            task->ExecuteOnce();
            lock.lock();
        }
    }

    /// Run() 루프를 깨우고 종료 요청
    void Stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
        cv_.notify_all();
    }

   private:
    template <typename FUNC>
    TaskId AddTaskImpl(std::string name, Task::Duration period, FUNC&& func, bool repeat) {
        std::lock_guard<std::mutex> lock(mutex_);

        TaskId id = next_id_++;

        auto task = std::make_shared<Task>(std::move(name), period, Task::Func(std::forward<FUNC>(func)),
                                           repeat);

        tasks_.emplace(id, std::move(task));
        cv_.notify_all();

        return id;
    }

    using TaskMap = std::unordered_map<TaskId, TaskPtr>;

    /// now 시점 기준으로 “지금 바로 실행 가능한” Task 검색
    TaskMap::iterator FindNextReadyTaskLocked(Task::TimePoint now) {
        TaskMap::iterator best = tasks_.end();
        for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
            if (it->second->next_deadline() <= now) {
                if (best == tasks_.end() || it->second->next_deadline() < best->second->next_deadline()) {
                    best = it;
                }
            }
        }
        return best;
    }

    /// 전체 Task 중 가장 이른 deadline 반환
    Task::TimePoint FindEarliestDeadlineLocked() const {
        if (tasks_.empty()) {
            return Task::TimePoint::max();
        }
        auto it = tasks_.begin();
        auto earliest = it->second->next_deadline();
        ++it;
        for (; it != tasks_.end(); ++it) {
            if (it->second->next_deadline() < earliest) {
                earliest = it->second->next_deadline();
            }
        }
        return earliest;
    }

    TaskMap tasks_;
    TaskId next_id_{0};
    bool stop_{false};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

}  // namespace bolero
