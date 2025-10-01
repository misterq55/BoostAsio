//
// broadcast_chat_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
// 브로드캐스트 메커니즘 실습 - 큐 제한 및 통계 기능 추가
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "../day6/chat_message.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);

    // 신규 참가자에게 최근 히스토리 전송
    for (auto msg: recent_msgs_)
      participant->deliver(msg);

    std::cout << "[ROOM] 참가자 입장. 현재 인원: " << participants_.size() << "\n";
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
    std::cout << "[ROOM] 참가자 퇴장. 현재 인원: " << participants_.size() << "\n";
  }

  void deliver(const chat_message& msg)
  {
    // 히스토리 저장 (최근 100개)
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    // ★ 브로드캐스트 핵심: 모든 참가자에게 전송
    std::cout << "[BROADCAST] " << participants_.size() << "명에게 메시지 전송\n";
    broadcast_count_++;

    for (auto participant: participants_)
      participant->deliver(msg);
  }

  size_t participant_count() const { return participants_.size(); }
  size_t broadcast_count() const { return broadcast_count_; }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
  size_t broadcast_count_ = 0;  // 브로드캐스트 횟수 통계
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
  }

  void start()
  {
    room_.join(shared_from_this());
    do_read_header();
  }

  void deliver(const chat_message& msg) override
  {
    // ★ 큐 크기 제한 추가 (개선점!)
    if (write_msgs_.size() >= max_outbound_msgs_)
    {
      std::cout << "[SESSION] 큐 포화! 연결 종료\n";
      socket_.close();
      return;
    }

    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);

    // 큐 상태 모니터링
    if (write_msgs_.size() > 10)
    {
      std::cout << "[WARNING] 쓰기 큐 누적: " << write_msgs_.size() << "개\n";
    }

    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            // ★ 여기서 브로드캐스트 트리거
            room_.deliver(read_msg_);
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();

            // ★ 큐에 남은 메시지 계속 전송 (재귀)
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;

  // ★ 큐 크기 제한
  enum { max_outbound_msgs_ = 100 };
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_context& io_context,
      const tcp::endpoint& endpoint)
    : acceptor_(io_context, endpoint)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::cout << "[SERVER] 새 연결 수락\n";
            std::make_shared<chat_session>(std::move(socket), room_)->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: broadcast_chat_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
    chat_server server(io_context, endpoint);

    std::cout << "=== 브로드캐스트 Chat Server ===\n";
    std::cout << "포트 " << argv[1] << "에서 대기 중...\n";

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
