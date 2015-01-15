#include "fann.h"
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


int main()
{

    boost::asio::io_service io;
    GenericServer server(io,12122);

    server.onAccept(
    [&server](std::shared_ptr<GenericSession> session) {
        session->onRead([&session](rapidjson::Document& d){
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            writer.StartObject();
            writer.String("Goodbye");
            writer.String(d["hello"].GetString());
            writer.EndObject();
            session->writeJson(s,[](){
                std::cout << "Message sent successfully" << std::endl;
            });
        });
        session->start();
    });



    io.run();
    return 0;


    const unsigned int num_input = 2;
    const unsigned int num_output = 1;
    const unsigned int num_layers = 3;
    const unsigned int num_neurons_hidden = 2;
    const float desired_error = (const float) 0.001;
    const unsigned int max_epochs = 500000;
    const unsigned int epochs_between_reports = 1000;

    struct fann *ann = fann_create_standard(num_layers, num_input,
        num_neurons_hidden, num_output);

    fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
    fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);
    ann = fann_create_from_file("xor_float.net");
    fann_train_on_file(ann, "xor.data", max_epochs,
        epochs_between_reports, desired_error/1000000000000000000);

    fann_save(ann, "xor_float.net");

    fann_destroy(ann);

    return 0;
}
