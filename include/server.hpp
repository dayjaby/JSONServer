#ifndef JSONSERVER_SERVER_HPP
#define JSONSERVER_SERVER_HPP

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using boost::asio::ip::tcp;

class MessageTooLarge {};

class GenericSession
    : public std::enable_shared_from_this<GenericSession>
{
public:
    GenericSession(tcp::socket socket)
    : socket_(std::move(socket))
    {
    }

    void start()
    {
        try {
            do_read_header();
        } catch(MessageTooLarge) {
            std::cerr << "Message size too large" << std::endl;
        }
    }

    template <class Handler>
    void writeJson(rapidjson::StringBuffer& s, Handler handler) {
        auto self(shared_from_this());
        int ln = s.Size();
        out_data_[0] = ln & 0xFF;
        out_data_[1] = (ln>>8) & 0xFF;
        out_data_[2] = (ln>>16) & 0xFF;
        out_data_[3] = (ln>>24) & 0xFF;
        std::memcpy(out_data_+4,s.GetString(),ln);
        boost::asio::async_write(socket_, boost::asio::buffer(out_data_, header_length+ln),
        [this, self,handler](boost::system::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                handler();
            }
        });
    }

    void onRead(std::function<void(rapidjson::Document&)> fn) {
        read_handler = fn;
    }


private:
    void do_read_header()
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,boost::asio::buffer(in_data_, header_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
            if (!ec)
            {
              header = in_data_[0] + (in_data_[1] << 8) + (in_data_[2] << 16) + (in_data_[3] << 24);
              if(header>max_length) throw MessageTooLarge();
            }

            do_read_data();
        });
    }


    void do_read_data()
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,boost::asio::buffer(in_data_, header),
        [this, self](boost::system::error_code ec, std::size_t length)
        {

            if (!ec)
            {
                in_data_[header+1] = 0;
                rapidjson::Document d;
                d.Parse<0>(in_data_);
                if(read_handler) read_handler(d);
            }
            do_read_header();
        });
    }

    int header;
    enum { header_length = 4 };
    tcp::socket socket_;
    enum { max_length = 1024 };
    char in_data_[max_length+1];
    char out_data_[max_length];
    std::stringstream in_;
    std::function<void(rapidjson::Document&)> read_handler;
};

class GenericServer
{
public:
  GenericServer(boost::asio::io_service& io_service, short port)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      socket_(io_service)
  {
    do_accept();
  }

  void onAccept(std::function<void(std::shared_ptr<GenericSession>)> fn) {
      accept_handle = fn;
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
              accept_handle(std::make_shared<GenericSession>(std::move(socket_)));
          }

          do_accept();
        });
  }
    std::function<void(std::shared_ptr<GenericSession>)> accept_handle;
  tcp::acceptor acceptor_;
  tcp::socket socket_;
};




#endif // JSONSERVER_SERVER_HPP
