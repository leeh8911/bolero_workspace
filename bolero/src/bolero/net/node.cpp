#include "bolero/net/node.hpp"

#include <chrono>
#include <iostream>

namespace bolero {

using namespace std::chrono_literals;

Node::Node(const std::string& node_name, const std::string& multicast_ip, uint16_t multicast_port)
    : node_name_(node_name), node_id_(generate_node_id(node_name_)) {
    data_transport_ = std::make_shared<DataTransport>(io_, 0);

    discovery_ = std::make_shared<DiscoveryManager>(io_, multicast_ip, multicast_port);
}

Node::~Node() {
    stop();
}

void Node::start() {
    if (running_)
        return;
    running_ = true;

    // DataTransport 수신 시작
    data_transport_->Start(
        [self = shared_from_this()](const TopicMessage& msg) { self->handle_data_message(msg); });

    // DiscoveryManager 수신 시작
    discovery_->Start(
        [self = shared_from_this()](const DiscoveryEvent& evt) { self->handle_discovery_event(evt); });

    // io_context run
    io_thread_ = std::thread([this]() { io_.run(); });

    std::cout << "[Node] started: " << node_name_ << " id=" << node_id_
              << " data_port=" << data_transport_->LocalPort() << std::endl;
}

void Node::stop() {
    if (!running_)
        return;
    running_ = false;

    // 정리
    data_transport_->Stop();
    discovery_->Stop();

    io_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

std::shared_ptr<Publisher> Node::create_publisher(const std::string& topic) {
    BOLERO_LOG_TRACE("Node::create_publisher topic='{}'", topic);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local_published_topics_.insert(topic);
    }

    // announce
    announce_pub(topic);

    // Publisher 객체는 단순 핸들
    auto self = shared_from_this();
    return std::make_shared<Publisher>(self, topic);
}

std::shared_ptr<Subscriber> Node::create_subscriber(const std::string& topic, MessageCallback callback) {
    BOLERO_LOG_TRACE("Node::create_subscriber topic='{}'", topic);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local_subscribed_topics_.insert(topic);
        local_subscribers_[topic].push_back(std::move(callback));
    }

    // announce
    announce_sub(topic);

    auto self = shared_from_this();
    return std::make_shared<Subscriber>(self, topic);
}

void Node::publish_string(const std::string& topic, const std::string& text) {
    MessagePayload payload(text.begin(), text.end());
    publish_raw(topic, payload);
}

void Node::publish_raw(const std::string& topic, const MessagePayload& payload) {
    std::vector<RemoteEndpoint> subscribers_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = remote_subscribers_.find(topic);
        if (it != remote_subscribers_.end()) {
            subscribers_copy = it->second;
        }
    }

    if (subscribers_copy.empty()) {
        // 아직 subscriber를 찾지 못한 경우일 수도 있음
        // 필요시 여기서 디버그 로그
        return;
    }

    for (const auto& ep : subscribers_copy) {
        data_transport_->SendTo(ep.ip, ep.port, topic, payload);
    }
}

// Discovery 이벤트 처리
void Node::handle_discovery_event(const DiscoveryEvent& evt) {
    // 기본 정책:
    // 1) SUB_ANNOUNCE: 내가 같은 topic의 publisher면 remote_subscribers_에 등록
    // 2) PUB_ANNOUNCE: 내가 같은 topic의 subscriber면 (일단은 로그만)

    if (evt.node_id == node_id_) {
        // 자기 자신이 쏜 announce는 무시
        return;
    }

    if (evt.msg_type == "SUB_ANNOUNCE") {
        bool i_am_publisher = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            i_am_publisher = (local_published_topics_.count(evt.topic) > 0);
        }

        if (i_am_publisher) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto& vec = remote_subscribers_[evt.topic];
            // 중복 방지
            auto found = std::find_if(vec.begin(), vec.end(),
                                      [&](const RemoteEndpoint& r) { return r.node_id == evt.node_id; });
            if (found == vec.end()) {
                vec.push_back(RemoteEndpoint{evt.node_id, evt.ip, evt.data_port});
                std::cout << "[Node] discovered subscriber for topic '" << evt.topic << "' at " << evt.ip
                          << ":" << evt.data_port << std::endl;
            }
        }
    } else if (evt.msg_type == "PUB_ANNOUNCE") {
        bool i_am_subscriber = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            i_am_subscriber = (local_subscribed_topics_.count(evt.topic) > 0);
        }

        if (i_am_subscriber) {
            std::cout << "[Node] discovered publisher for topic '" << evt.topic << "' at " << evt.ip << ":"
                      << evt.data_port << std::endl;
            // 현재 구조에서는 publisher endpoint를 굳이 저장하지 않아도 됨.
            // 필요하면 remote_publishers_ 구조를 추가 가능.
        }
    }
}

// 데이터 메시지 처리
void Node::handle_data_message(const TopicMessage& msg) {
    std::vector<MessageCallback> callbacks;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = local_subscribers_.find(msg.topic);
        if (it != local_subscribers_.end()) {
            callbacks = it->second;  // copy
        }
    }

    if (callbacks.empty())
        return;

    for (auto& cb : callbacks) {
        cb(msg.topic, msg.payload);
    }
}

void Node::announce_pub(const std::string& topic) {
    DiscoveryEvent evt;
    evt.msg_type = "PUB_ANNOUNCE";
    evt.topic = topic;
    evt.node_id = node_id_;
    evt.ip = "0.0.0.0";  // DiscoveryManager 쪽에서 remote 주소/port 사용할 수도 있음
    evt.data_port = data_transport_->LocalPort();

    discovery_->SendAnnounce(evt);
}

void Node::announce_sub(const std::string& topic) {
    DiscoveryEvent evt;
    evt.msg_type = "SUB_ANNOUNCE";
    evt.topic = topic;
    evt.node_id = node_id_;
    evt.ip = "0.0.0.0";
    evt.data_port = data_transport_->LocalPort();

    discovery_->SendAnnounce(evt);
}

std::string Node::generate_node_id(const std::string& name) const {
    // 간단한 UUID 대용 (실제 구현에서는 진짜 UUID 라이브러리 사용 권장)
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return name + "-" + std::to_string(now);
}

// -------- Publisher / Subscriber 구현 --------

void Publisher::publish(const MessagePayload& payload) {
    if (auto node = node_.lock()) {
        node->publish_raw(topic_, payload);
    }
}

}  // namespace bolero
