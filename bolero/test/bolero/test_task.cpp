#include <gtest/gtest.h>

#include "bolero/task.hpp"

TEST(TestTask, BasicUsage) {
    int32_t value = 0;
    auto task = bolero::Task::Create("task", [&value]() { value += 1; });

    task->Run();
    task->Wait();
    EXPECT_EQ(value, 1);
}

TEST(TestTask, FindValueInParallel) {
    std::vector<size_t> data{};
    for (size_t i = 0; i < 100; ++i) {
        data.emplace_back(i);  // 짝수로 채움
    }

    size_t num_tasks = 10;
    std::vector<bolero::TaskPtr> tasks;
    size_t target_value = 42;
    size_t result_index = -1;
    for (size_t t = 0; t < num_tasks; ++t) {
        auto start_index = t * (data.size() / num_tasks);
        auto end_index = (t == num_tasks - 1) ? data.size() : start_index + (data.size() / num_tasks);

        tasks.emplace_back(bolero::Task::Create(
            "find_task_" + std::to_string(t), [start_index, end_index, &data, target_value, &result_index]() {
                for (size_t i = start_index; i < end_index; ++i) {
                    if (data[i] == target_value) {
                        result_index = i;
                        break;
                    }
                }
            }));
    }

    for (auto& task : tasks) {
        task->Run();
    }

    for (auto& task : tasks) {
        task->Wait();
    }

    EXPECT_EQ(result_index, 42);
}