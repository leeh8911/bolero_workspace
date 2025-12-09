#pragma once

#include <array>
#include <atomic>
#include <cctype>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "bolero/logger.hpp"
#include "bolero/net/discovery_event.hpp"

#include <asio.hpp>
namespace bolero {

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
        auto json = ToJson(evt);
        this->socket.async_send_to(
            asio::buffer(json.dump()), this->endpoint, [](std::error_code ec, std::size_t /*bytes*/) {
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
        DiscoveryEvent evt = FromJson(nlohmann::json::parse(std::string((char*) this->buffer.data(), bytes)));

        // multicast 패킷의 송신자 IP를 신뢰해 ip 필드를 보정한다.
        // announce 시점에 0.0.0.0으로 채워 보내기 때문에, 실제 보내온 원격 주소를 사용해야
        // publisher가 subscriber에게 데이터를 전송할 수 있다.
        auto sender_ip = this->remote_ep_.address().to_string();
        if (evt.ip.empty() || evt.ip == "0.0.0.0") {
            evt.ip = sender_ip;
        }

        if (callback) {
            callback(evt);
        }
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
