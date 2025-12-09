#pragma once

#include <memory>
#include <string>

#include "bolero/net/subscriber.hpp"

namespace bolero {

// forward declaration
class Node;

class Subscriber {
   public:
    Subscriber(std::weak_ptr<Node> node_, std::string topic_)
        : node(std::move(node_)), topic(std::move(topic_)) {}

    const std::string& Topic() const { return topic; }

   private:
    std::weak_ptr<Node> node;
    std::string topic;
};
}  // namespace bolero