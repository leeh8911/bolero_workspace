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

    // JSON 인코딩/디코딩은 간단하게 string 조작으로 시작 → 나중에 nlohmann json 사용 가능
    std::string EncodeJson(const DiscoveryEvent& evt) {
        // very naive JSON assembly (나중에 개선 가능)
        return "{\"msg_type\":\"" + evt.msg_type + "\",\"topic\":\"" + evt.topic + "\",\"node_id\":\"" +
               evt.node_id + "\",\"ip\":\"" + evt.ip + "\",\"data_port\":" + std::to_string(evt.data_port) +
               "}";
    }

    bool DecodeJson(const std::string& str, DiscoveryEvent& evt) {
        auto parse_string_field = [&str](const std::string& key, std::string& out) {
            const std::string token = "\"" + key + "\":";
            auto key_pos = str.find(token);
            if (key_pos == std::string::npos)
                return false;

            auto value_start = str.find('"', key_pos + token.size());
            if (value_start == std::string::npos)
                return false;

            auto value_end = str.find('"', value_start + 1);
            if (value_end == std::string::npos)
                return false;

            out = str.substr(value_start + 1, value_end - value_start - 1);
            return true;
        };

        auto parse_uint16_field = [&str](const std::string& key, uint16_t& out) {
            const std::string token = "\"" + key + "\":";
            auto key_pos = str.find(token);
            if (key_pos == std::string::npos)
                return false;

            auto value_start = key_pos + token.size();
            while (value_start < str.size() && std::isspace(static_cast<unsigned char>(str[value_start]))) {
                ++value_start;
            }

            auto value_end = value_start;
            while (value_end < str.size() && std::isdigit(static_cast<unsigned char>(str[value_end]))) {
                ++value_end;
            }

            if (value_end == value_start)
                return false;

            try {
                unsigned long value = std::stoul(str.substr(value_start, value_end - value_start));
                if (value > std::numeric_limits<uint16_t>::max())
                    return false;
                out = static_cast<uint16_t>(value);
            } catch (...) {
                return false;
            }

            return true;
        };

        if (!parse_string_field("msg_type", evt.msg_type))
            return false;
        if (!parse_string_field("topic", evt.topic))
            return false;
        if (!parse_string_field("node_id", evt.node_id))
            return false;
        if (!parse_string_field("ip", evt.ip))
            return false;
        if (!parse_uint16_field("data_port", evt.data_port))
            return false;

        return true;
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
