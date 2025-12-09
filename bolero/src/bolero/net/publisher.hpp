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
    Publisher(std::weak_ptr<Node> node_, std::string topic_)
        : node(std::move(node_)), topic(std::move(topic_)) {}

    void Publish(const MessagePayload& payload);

    template <typename T>
    void Publish(const T& message) {
        // Serialize T to MessagePayload
        MessagePayload payload(sizeof(T));
        memcpy(payload.data(), &message, sizeof(T));
        Publish(payload);
    }

    const std::string& Topic() const { return topic; }

   private:
    std::weak_ptr<Node> node;
    std::string topic;
};

}  // namespace bolero