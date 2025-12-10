#include "bolero/net/node.hpp"

#include <chrono>
#include <iostream>

#include "bolero/logger.hpp"
#include "bolero/net/data_transport.hpp"
#include "bolero/net/discovery_manager.hpp"
#include "bolero/net/publisher.hpp"
#include "bolero/net/subscriber.hpp"

namespace bolero {

using namespace std::chrono_literals;

Node::Node(const std::string& node_name, const std::string& multicast_ip, uint16_t multicast_port)
    : node_name(node_name), node_id(GenerateNodeId(node_name)) {
    data_transport = std::make_shared<DataTransport>(io_context, 0);

    discovery = std::make_shared<DiscoveryManager>(io_context, multicast_ip, multicast_port);
}

Node::~Node() {
    Stop();
}

void Node::Start() {
    if (running)
        return;
    running = true;

    // DataTransport 수신 시작
    data_transport->Start(
        [self = shared_from_this()](const TopicMessage& msg) { self->HandleDataMessage(msg); });

    // DiscoveryManager 수신 시작
    discovery->Start(
        [self = shared_from_this()](const DiscoveryEvent& evt) { self->HandleDiscoveryEvent(evt); });

    // io_context run
    io_thread = std::thread([this]() { io_context.run(); });

    BOLERO_LOG_TRACE("Node started: {} id={} data_port={}", node_name, node_id, data_transport->LocalPort());
}

void Node::Stop() {
    if (!running)
        return;
    running = false;

    // 정리
    data_transport->Stop();
    discovery->Stop();

    io_context.stop();
    if (io_thread.joinable()) {
        io_thread.join();
    }
}

std::shared_ptr<Publisher> Node::CreatePublisher(const std::string& topic) {
    BOLERO_LOG_TRACE("Node::CreatePublisher topic='{}'", topic);
    {
        std::lock_guard<std::mutex> lock(mutex);
        local_published_topics.insert(topic);
    }

    // announce
    AnnouncePublish(topic);

    // Publisher 객체는 단순 핸들
    auto self = shared_from_this();
    return std::make_shared<Publisher>(self, topic);
}

std::shared_ptr<Subscriber> Node::CreateSubscriber(const std::string& topic, MessageCallback callback) {
    BOLERO_LOG_TRACE("Node::CreateSubscriber topic='{}'", topic);
    {
        std::lock_guard<std::mutex> lock(mutex);
        local_subscribed_topics.insert(topic);
        local_subscribers[topic].push_back(std::move(callback));
    }

    // announce
    AnnounceSubscribe(topic);

    auto self = shared_from_this();
    return std::make_shared<Subscriber>(self, topic);
}

void Node::PublishRaw(const std::string& topic, const MessagePayload& payload) {
    std::vector<RemoteEndpoint> subscribers_copy;
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = remote_subscribers.find(topic);
        if (it != remote_subscribers.end()) {
            subscribers_copy = it->second;
        }
    }

    if (subscribers_copy.empty()) {
        // 아직 subscriber를 찾지 못한 경우일 수도 있음
        // 필요시 여기서 디버그 로그
        return;
    }

    for (const auto& ep : subscribers_copy) {
        data_transport->SendTo(ep.ip, ep.port, topic, payload);
    }
}

// Discovery 이벤트 처리
void Node::HandleDiscoveryEvent(const DiscoveryEvent& evt) {
    // 기본 정책:
    // 1) SUB_ANNOUNCE: 내가 같은 topic의 publisher면 remote_subscribers_에 등록
    // 2) PUB_ANNOUNCE: 내가 같은 topic의 subscriber면 (일단은 로그만)

    if (evt.node_id == node_id) {
        // 자기 자신이 쏜 announce는 무시
        return;
    }

    if (evt.msg_type == "SUB_ANNOUNCE") {
        bool i_am_publisher = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            i_am_publisher = (local_published_topics.count(evt.topic) > 0);
        }

        if (i_am_publisher) {
            std::lock_guard<std::mutex> lock(mutex);

            auto& vec = remote_subscribers[evt.topic];
            // 중복 방지
            auto found = std::find_if(vec.begin(), vec.end(),
                                      [&](const RemoteEndpoint& r) { return r.node_id == evt.node_id; });
            if (found == vec.end()) {
                vec.push_back(RemoteEndpoint{evt.node_id, evt.ip, evt.data_port});
                BOLERO_LOG_TRACE("Added remote subscriber for topic '{}' at {}:{}", evt.topic, evt.ip,
                                 evt.data_port);
            }
        }
    } else if (evt.msg_type == "PUB_ANNOUNCE") {
        bool i_am_subscriber = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            i_am_subscriber = (local_subscribed_topics.count(evt.topic) > 0);
        }

        if (i_am_subscriber) {
            BOLERO_LOG_TRACE("Node::HandleDiscoveryEvent PUB_ANNOUNCE topic='{}' from node_id='{}'",
                             evt.topic, evt.node_id);

            // 새 Publisher가 등장했으니, 내 SUB를 다시 알린다.
            // 멀티캐스트로 SUB_ANNOUNCE를 재송신.
            AnnounceSubscribe(evt.topic);
        }
    }
}

// 데이터 메시지 처리
void Node::HandleDataMessage(const TopicMessage& msg) {
    std::vector<MessageCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = local_subscribers.find(msg.topic);
        if (it != local_subscribers.end()) {
            callbacks = it->second;  // copy
        }
    }

    if (callbacks.empty())
        return;

    for (auto& cb : callbacks) {
        cb(msg.topic, msg.payload);
    }
}

void Node::AnnouncePublish(const std::string& topic) {
    DiscoveryEvent evt;
    evt.msg_type = "PUB_ANNOUNCE";
    evt.topic = topic;
    evt.node_id = node_id;
    evt.ip = "0.0.0.0";  // DiscoveryManager 쪽에서 remote 주소/port 사용할 수도 있음
    evt.data_port = data_transport->LocalPort();

    discovery->SendAnnounce(evt);
}

void Node::AnnounceSubscribe(const std::string& topic) {
    DiscoveryEvent evt;
    evt.msg_type = "SUB_ANNOUNCE";
    evt.topic = topic;
    evt.node_id = node_id;
    evt.ip = "0.0.0.0";
    evt.data_port = data_transport->LocalPort();

    discovery->SendAnnounce(evt);
}

std::string Node::GenerateNodeId(const std::string& name) const {
    // 간단한 UUID 대용 (실제 구현에서는 진짜 UUID 라이브러리 사용 권장)

    size_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    return name + "-" + std::to_string(millis);
}

// -------- Publisher / Subscriber 구현 --------

}  // namespace bolero
