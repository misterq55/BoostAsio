#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class client
{
public:
    client(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
        : io_context_(io_context), socket_(io_context)
    {
        tcp::resolver resolver(io_context);
        endpoints_ = resolver.resolve(host, port);
        do_connect();
    }

    void write(const std::string& msg)
    {
        boost::asio::post(io_context_, [this, msg]()
        {
            do_write(msg);
        });
    }

    void close()
    {
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }

private:
    void do_connect()
    {
        boost::asio::async_connect(socket_, endpoints_,
            [this](boost::system::error_code ec, tcp::endpoint)
            {
                if (!ec)
                {
                    std::cout << "Connected to server!" << std::endl;
                    do_read();
                }
                else
                {
                    std::cout << "Connect error: " << ec.message() << std::endl;
                }
            });
    }

    void do_read()
    {
        socket_.async_read_some(boost::asio::buffer(read_msg_, max_length),
            [this](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    std::cout << "Server replied: " << std::string(read_msg_, length) << std::endl;
                    do_read();
                }
                else
                {
                    std::cout << "Read error: " << ec.message() << std::endl;
                    socket_.close();
                }
            });
    }

    void do_write(const std::string& msg)
    {
        boost::asio::async_write(socket_, boost::asio::buffer(msg.data(), msg.length()),
            [this, msg](boost::system::error_code ec, std::size_t)
            {
                if (!ec)
                {
                    std::cout << "Message sent: " << msg << std::endl;
                }
                else
                {
                    std::cout << "Write error: " << ec.message() << std::endl;
                    socket_.close();
                }
            });
    }

private:
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    tcp::resolver::results_type endpoints_;
    enum { max_length = 1024 };
    char read_msg_[max_length];
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: async_tcp_echo_client <host> <port>" << std::endl;
            return 1;
        }

        boost::asio::io_context io_context;
        client c(io_context, argv[1], argv[2]);

        std::thread t([&io_context]() { io_context.run(); });

        std::cout << "Connected! Type messages (or 'quit' to exit):" << std::endl;

        std::string line;
        while (std::getline(std::cin, line))
        {
            if (line == "quit")
                break;

            c.write(line);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        c.close();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}