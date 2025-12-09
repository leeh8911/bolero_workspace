#include "bolero/net/publisher.hpp"

#include "bolero/net/node.hpp"

namespace bolero {

void Publisher::Publish(const MessagePayload& payload) {
    if (auto node = this->node.lock()) {
        node->PublishRaw(this->topic, payload);
    }
}

}  // namespace bolero