#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

class UdpClient
{
public:
    UdpClient(boost::asio::io_context& io_context,
              const std::string& host, const std::string& port)
        : io_context_(io_context),
          socket_(io_context, udp::endpoint(udp::v4(), 0))
    {
        udp::resolver resolver(io_context);
        server_endpoint_ = *resolver.resolve(udp::v4(), host, port).begin();
    }

    void send_message(const std::string& message)
    {
        std::cout << "전송: " << message << std::endl;

        socket_.async_send_to(
            boost::asio::buffer(message.c_str(), message.length()),
            server_endpoint_,
            [this](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    start_receive();
                }
                else
                {
                    std::cerr << "전송 오류: " << ec.message() << std::endl;
                }
            });
    }

private:
    void start_receive()
    {
        socket_.async_receive_from(
            boost::asio::buffer(data_, max_length), server_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd)
            {
                if (!ec)
                {
                    std::cout << "응답: " << std::string(data_, bytes_recvd) << std::endl;
                }
                else
                {
                    std::cerr << "수신 오류: " << ec.message() << std::endl;
                }
            });
    }

    boost::asio::io_context& io_context_;
    udp::socket socket_;
    udp::endpoint server_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
};

int main()
{
    try
    {
        boost::asio::io_context io_context;

        UdpClient client(io_context, "localhost", "8080");

        // 백그라운드에서 io_context 실행
        std::thread io_thread([&io_context]() {
            io_context.run();
        });

        std::cout << "UDP 에코 클라이언트 시작" << std::endl;
        std::cout << "메시지를 입력하세요 (종료하려면 'quit' 입력):" << std::endl;

        std::string input;
        while (std::getline(std::cin, input))
        {
            if (input == "quit")
                break;

            if (!input.empty())
            {
                client.send_message(input);

                // 응답을 기다리기 위한 잠시 대기
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        io_context.stop();
        io_thread.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "예외 발생: " << e.what() << std::endl;
    }

    return 0;
}