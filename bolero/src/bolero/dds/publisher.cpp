#include "bolero/dds/publisher.hpp"

#include "bolero/dds/node.hpp"

namespace bolero {

void Publisher::Publish(const MessagePayload& payload) {
    if (auto node = this->node.lock()) {
        node->PublishRaw(this->topic, payload);
    }
}

}  // namespace bolero