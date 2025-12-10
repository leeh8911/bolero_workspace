#pragma once

#include <functional>
#include <vector>

namespace bolero {

using MessagePayload = std::vector<uint8_t>;
using MessageCallback = std::function<void(const std::string& topic, const MessagePayload& payload)>;
}  // namespace bolero