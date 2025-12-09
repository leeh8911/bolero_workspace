#include "bolero/net/publisher.hpp"

#include "bolero/net/node.hpp"

namespace bolero {

void Publisher::publish(const MessagePayload& payload) {
    if (auto node = node_.lock()) {
        node->publish_raw(topic_, payload);
    }
}

}  // namespace bolero