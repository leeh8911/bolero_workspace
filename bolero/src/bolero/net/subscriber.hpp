#pragma once

#include <memory>
#include <string>

#include "bolero/net/subscriber.hpp"

namespace bolero {

// forward declaration
class Node;

class Subscriber {
   public:
    Subscriber(std::weak_ptr<Node> node, std::string topic)
        : node_(std::move(node)), topic_(std::move(topic)) {}

    const std::string& topic() const { return topic_; }

   private:
    std::weak_ptr<Node> node_;
    std::string topic_;
};
}  // namespace bolero