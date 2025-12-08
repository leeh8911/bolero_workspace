#pragma once

#include <string>
#include <vector>
namespace bolero {
struct Message {
    std::string topic;
    std::vector<uint8_t> data;
};
}  // namespace bolero