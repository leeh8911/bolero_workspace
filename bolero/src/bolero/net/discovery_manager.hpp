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

struct DiscoveryEvent {
    std::string msg_type;  // "PUB_ANNOUNCE" / "SUB_ANNOUNCE"
    std::string topic;
    std::string node_id;
    std::string ip;
    uint16_t data_port;
};

class DiscoveryManager : public std::enable_shared_from_this<DiscoveryManager> {
   public:
    using EventCallback = std::function<void(const DiscoveryEvent&)>;

    DiscoveryManager(asio::io_context& io_context, std::string multicast_ip = "239.255.0.1",
                     uint16_t multicast_port_ = 7500)
        : io(io_context),
          socket(io_context),
          multicast_addr(asio::ip::make_address(multicast_ip)),
          multicast_port(multicast_port_),
          endpoint(multicast_addr, multicast_port_),
          running(false) {
        this->Open();
    }

    void Start(EventCallback callback) {
        this->callback = std::move(callback);
        this->running = true;
        this->DoReceive();
    }

    void Stop() {
        this->running = false;
        asio::post(this->io, [this]() { this->socket.cancel(); });
    }

    // advertise pub/sub info
    void SendAnnounce(const DiscoveryEvent& evt) {
        std::string json = EncodeJson(evt);
        this->socket.async_send_to(
            asio::buffer(json), this->endpoint, [](std::error_code ec, std::size_t /*bytes*/) {
                if (ec) {
                    BOLERO_LOG_ERROR("Discovery announce send failed: {}", ec.message());
                }
            });
    }

   private:
    void Open() {
        asio::ip::udp::endpoint listen_ep(asio::ip::udp::v4(), this->multicast_port);
        this->socket.open(listen_ep.protocol());

        this->socket.set_option(asio::ip::udp::socket::reuse_address(true));
        this->socket.bind(listen_ep);

        this->socket.set_option(asio::ip::multicast::join_group(this->multicast_addr));
    }

    void DoReceive() {
        this->socket.async_receive_from(asio::buffer(this->buffer), remote_ep_,
                                        [self = shared_from_this()](std::error_code ec, std::size_t bytes) {
                                            if (!ec && bytes > 0) {
                                                self->HandleMessage(bytes);
                                            }
                                            if (self->running) {
                                                self->DoReceive();
                                            }
                                        });
    }

    void HandleMessage(size_t bytes) {
        std::string json((char*) this->buffer.data(), bytes);
        DiscoveryEvent evt;
        if (!this->DecodeJson(json, evt))
            return;

        if (callback) {
            callback(evt);
        }
    }

    // JSON 인코딩/디코딩은 간단하게 string 조작으로 시작 → 나중에 nlohmann json 사용 가능
    std::string EncodeJson(const DiscoveryEvent& evt) {
        // very naive JSON assembly (나중에 개선 가능)
        return "{\"msg_type\":\"" + evt.msg_type + "\",\"topic\":\"" + evt.topic + "\",\"node_id\":\"" +
               evt.node_id + "\",\"ip\":\"" + evt.ip + "\",\"data_port\":" + std::to_string(evt.data_port) +
               "}";
    }

    bool DecodeJson(const std::string& str, DiscoveryEvent& evt) {
        // TODO: simple parse or replace with nlohmann::json
        // 지금은 단순한 프로토타입만 사용하자.
        return false;  // 우선 비워둠
    }

    asio::io_context& io;

    asio::ip::udp::socket socket;
    asio::ip::address multicast_addr;
    uint16_t multicast_port;
    asio::ip::udp::endpoint endpoint;

    std::atomic<bool> running;
    EventCallback callback;

    std::array<uint8_t, 4096> buffer{};
    asio::ip::udp::endpoint remote_ep_;
};

}  // namespace bolero
