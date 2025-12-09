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

    void start();
    void stop();

    std::shared_ptr<Publisher> create_publisher(const std::string& topic);
    std::shared_ptr<Subscriber> create_subscriber(const std::string& topic, MessageCallback callback);

    // 문자열 편의 함수
    void publish_string(const std::string& topic, const std::string& text);

   private:
    friend class Publisher;
    friend class Subscriber;

    // Publisher 가 실제 publish 할 때 호출
    void publish_raw(const std::string& topic, const MessagePayload& payload);

    // Discovery / DataTransport 콜백
    void handle_discovery_event(const DiscoveryEvent& evt);
    void handle_data_message(const TopicMessage& msg);

    // 내부 도움 함수
    void announce_pub(const std::string& topic);
    void announce_sub(const std::string& topic);

    std::string generate_node_id(const std::string& name) const;

   private:
    std::string node_name_;
    std::string node_id_;

    asio::io_context io_;
    std::thread io_thread_;

    std::shared_ptr<DataTransport> data_transport_;
    std::shared_ptr<DiscoveryManager> discovery_;

    std::atomic<bool> running_{false};

    // 동시성 보호용 mutex
    mutable std::mutex mutex_;

    // topic -> subscriber callbacks
    std::unordered_map<std::string, std::vector<MessageCallback>> local_subscribers_;

    // topic -> remote subscribers (endpoint list)
    std::unordered_map<std::string, std::vector<RemoteEndpoint>> remote_subscribers_;

    // 내가 publisher 로 등록한 topic 목록
    std::unordered_set<std::string> local_published_topics_;

    // 내가 subscriber 로 등록한 topic 목록 (디버깅/확장용)
    std::unordered_set<std::string> local_subscribed_topics_;
};

using NodePtr = std::shared_ptr<Node>;

}  // namespace bolero
