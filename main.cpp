#include "server.hpp"

int main()
{

    boost::asio::io_service io;
    GenericServer server(io,12122);

    server.onAccept(
    [&server](std::shared_ptr<GenericSession> session) {
        /* Connection established */
        session->onRead([session](rapidjson::Document& d){
            /* JSON message read */
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            writer.StartObject();
            writer.String("Goodbye");
            writer.String(d["hello"].GetString());
            writer.EndObject();
            session->writeJson(s,[](){
            });
        });
        session->start();
    });



    io.run();
    return 0;
}
