#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "bolero/net/data_transport.hpp"  // 실제 경로에 맞게 수정

#include <asio.hpp>

using namespace std::chrono_literals;

namespace bolero {

TEST(DataTransportTest, SendAndReceive) {
    asio::io_context io;

    // 수신 측 transport (port = 0이면 OS가 포트 할당)
    auto receiver = std::make_shared<DataTransport>(io, 0);
    auto sender = std::make_shared<DataTransport>(io, 0);

    std::atomic<bool> received{false};
    TopicMessage received_msg;

    receiver->Start([&](const TopicMessage& msg) {
        received_msg = msg;
        received = true;
    });

    // io_context 실행 스레드
    std::thread io_thread([&]() { io.run(); });

    // 송신: receiver의 로컬 포트로 전송
    std::string topic = "test/topic";
    std::vector<uint8_t> payload = {'h', 'e', 'l', 'l', 'o'};

    sender->SendTo("127.0.0.1", receiver->LocalPort(), topic, payload);

    // 최대 500ms까지 기다리면서 콜백이 호출되는지 확인
    for (int i = 0; i < 50 && !received.load(); ++i) {
        std::this_thread::sleep_for(10ms);
    }

    // 정리
    receiver->Stop();
    sender->Stop();
    io.stop();
    if (io_thread.joinable()) {
        io_thread.join();
    }

    ASSERT_TRUE(received.load()) << "Receiver did not get any message in time";

    EXPECT_EQ(received_msg.topic, topic);
    EXPECT_EQ(received_msg.payload, payload);
    EXPECT_EQ(received_msg.remote_ip, "127.0.0.1");
    // remote_port는 sender의 포트이므로 값이 달라질 수 있지만, 0이 아닌지만 확인해도 됨
    EXPECT_NE(received_msg.remote_port, 0);
}

}  // namespace bolero
