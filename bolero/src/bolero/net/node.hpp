#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "bolero/net/data_transport.hpp"
#include "bolero/net/discovery_manager.hpp"
#include "bolero/net/message_payload.hpp"
#include "bolero/net/publisher.hpp"
#include "bolero/net/subscriber.hpp"

#include <asio.hpp>

namespace bolero {

// 간단한 remote endpoint 표현
struct RemoteEndpoint {
    std::string node_id;
    std::string ip;
    uint16_t port;
};

class Node : public std::enable_shared_from_this<Node> {
   public:
    explicit Node(const std::string& node_name, const std::string& multicast_ip = "239.255.0.1",
                  uint16_t multicast_port = 7500);

    ~Node();

    void Start();
    void Stop();

    std::shared_ptr<Publisher> CreatePublisher(const std::string& topic);
    std::shared_ptr<Subscriber> CreateSubscriber(const std::string& topic, MessageCallback callback);

   private:
    friend class Publisher;
    friend class Subscriber;

    // Publisher 가 실제 publish 할 때 호출
    void PublishRaw(const std::string& topic, const MessagePayload& payload);

    // Discovery / DataTransport 콜백
    void HandleDiscoveryEvent(const DiscoveryEvent& evt);
    void HandleDataMessage(const TopicMessage& msg);

    // 내부 도움 함수
    void AnnouncePublish(const std::string& topic);
    void AnnounceSubscribe(const std::string& topic);

    std::string GenerateNodeId(const std::string& name) const;

   private:
    std::string node_name;
    std::string node_id;

    asio::io_context io_context;
    std::thread io_thread;

    std::shared_ptr<DataTransport> data_transport;
    std::shared_ptr<DiscoveryManager> discovery;

    std::atomic<bool> running{false};

    // 동시성 보호용 mutex
    mutable std::mutex mutex;

    // topic -> subscriber callbacks
    std::unordered_map<std::string, std::vector<MessageCallback>> local_subscribers;

    // topic -> remote subscribers (endpoint list)
    std::unordered_map<std::string, std::vector<RemoteEndpoint>> remote_subscribers;

    // 내가 publisher 로 등록한 topic 목록
    std::unordered_set<std::string> local_published_topics;

    // 내가 subscriber 로 등록한 topic 목록 (디버깅/확장용)
    std::unordered_set<std::string> local_subscribed_topics;
};

using NodePtr = std::shared_ptr<Node>;

}  // namespace bolero
