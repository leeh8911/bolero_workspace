#pragma once

#include <memory>
#include <string>

#include "bolero/net/message_payload.hpp"
#include "bolero/net/subscriber.hpp"

namespace bolero {

// forward declaration
class Node;

class Publisher {
   public:
    Publisher(std::weak_ptr<Node> node, std::string topic)
        : node_(std::move(node)), topic_(std::move(topic)) {}

    void publish(const MessagePayload& payload);

    template <typename T>
    void publish(const T& message) {
        // Serialize T to MessagePayload
        MessagePayload payload(sizeof(T));
        memcpy(payload.data(), &message, sizeof(T));
        publish(payload);
    }

    const std::string& topic() const { return topic_; }

   private:
    std::weak_ptr<Node> node_;
    std::string topic_;
};

}  // namespace bolero