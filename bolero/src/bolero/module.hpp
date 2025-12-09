#pragma once

#include <memory.h>

#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"
#include "bolero/net/node.hpp"
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

    std::shared_ptr<Publisher> CreatePublisher(const std::string& topic_name) {
        auto pub = this->node->create_publisher(topic_name);
        return pub;
    }

    template <typename T>
    void CreateSubscriber(const std::string& topic_name, std::function<void(const T&)> callback) {
        this->node->create_subscriber(topic_name,
                                      [callback](const std::string& topic, const MessagePayload& payload) {
                                          // Deserialize payload to T
                                          T message;
                                          // 여기서는 간단히 memcpy로 가정 (실제 구현에서는 proper
                                          // serialization 필요)
                                          if (payload.size() == sizeof(T)) {
                                              memcpy(&message, payload.data(), sizeof(T));
                                              callback(message);
                                          }
                                      });
    }

   protected:
    const Config& GetConfig() const;

    Scheduler& GetScheduler();
    const Scheduler& GetScheduler() const;

   private:
    Config config;
    Scheduler scheduler;
    NodePtr node;
};

}  // namespace bolero

BUILD_FACTORY(MODULE_FACTORY, bolero::Module);