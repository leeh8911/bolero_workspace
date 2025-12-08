#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "bolero/logger.hpp"

#include <asio.hpp>
namespace bolero {
struct TopicMessage {
    std::string topic;
    std::vector<uint8_t> payload;
    std::string remote_ip;
    uint16_t remote_port;
};
class DataTransport : public std::enable_shared_from_this<DataTransport> {
   public:
    using ReceiveCallback = std::function<void(const TopicMessage&)>;

    DataTransport(asio::io_context& io_context, uint16_t port = 0)
        : io(io_context),
          socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
          running(false) {
        local_port = this->socket.local_endpoint().port();
    }

    uint16_t LocalPort() const { return this->local_port; }

    void Start(ReceiveCallback callback) {
        this->receive_callback = std::move(callback);
        this->running = true;
        this->DoReceive();
    }

    void Stop() {
        this->running = false;
        asio::post(this->io, [this]() { this->socket.cancel(); });
    }

    void SendTo(const std::string& ip, uint16_t port, const std::string& topic,
                const std::vector<uint8_t>& payload) {
        std::vector<uint8_t> buffer{};
        uint32_t topic_size = static_cast<uint32_t>(topic.size());
        buffer.resize(sizeof(uint32_t) + topic.size() + payload.size());

        memcpy(buffer.data(), &topic_size, sizeof(uint32_t));
        memcpy(buffer.data() + sizeof(uint32_t), topic.data(), topic_size);
        memcpy(buffer.data() + sizeof(uint32_t) + topic_size, payload.data(), payload.size());

        asio::ip::udp::endpoint endpoint(asio::ip::make_address(ip), port);

        this->socket.async_send_to(asio::buffer(buffer), endpoint,
                                   [](const asio::error_code& ec, std::size_t /*bytes_sent*/) {
                                       if (ec) {
                                           BOLERO_LOG_ERROR("Send failed: {}", ec.message());
                                       }
                                   });
    }

   private:
    void DoReceive() {
        if (!this->running) {
            return;
        }

        this->socket.async_receive_from(asio::buffer(this->receive_buffer), this->remote_endpoint,
                                        [self = shared_from_this()](std::error_code ec, std::size_t bytes) {
                                            if (!ec && bytes > sizeof(uint32_t)) {
                                                self->HandleMessage(bytes);
                                            }
                                            if (self->running) {
                                                self->DoReceive();
                                            }
                                        });
    }

    void HandleMessage(size_t bytes) {
        uint32_t topic_size = 0;
        memcpy(&topic_size, this->receive_buffer.data(), sizeof(uint32_t));
        if (topic_size + sizeof(uint32_t) > bytes) {
            BOLERO_LOG_WARN("Received malformed message");
            return;
        }

        std::string topic((char*) this->receive_buffer.data() + sizeof(uint32_t), topic_size);
        std::vector<uint8_t> payload(bytes - sizeof(uint32_t) - topic_size);
        memcpy(payload.data(), this->receive_buffer.data() + sizeof(uint32_t) + topic_size, payload.size());

        if (this->receive_callback) {
            TopicMessage message{
                .topic = std::move(topic),
                .payload = std::move(payload),
                .remote_ip = this->remote_endpoint.address().to_string(),
                .remote_port = this->remote_endpoint.port(),
            };
            this->receive_callback(message);
        }
    }

    asio::io_context& io;
    asio::ip::udp::socket socket;
    std::atomic<bool> running{false};

    uint16_t local_port{};

    ReceiveCallback receive_callback{};

    std::array<uint8_t, 65536> receive_buffer{};
    asio::ip::udp::endpoint remote_endpoint{};
};
}  // namespace bolero