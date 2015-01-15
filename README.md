<h2>JSON-Server</h2>
The server is designed to be very easy to use, allowing you to write a server in a few lines of codes.

<h3>Motivating Example</h3>
The following example illustrates how to make the server read and write a message.
 
```cpp
boost::asio::io_service io;
GenericServer server(io,12122);
server.onAccept(
[&server](std::shared_ptr<GenericSession> session) {
    /* Connection established */
    session->onRead([session](rapidjson::Document& d){
        /* JSON message read successfully */
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        writer.StartObject();
        writer.String("Goodbye");
        writer.String(d["hello"].GetString());
        writer.EndObject();
        session->writeJson(s,[](){
          /* JSON message sent successfully */
        });
    });
    session->start();
});
```


<h3>Prerequisites</h3>
C++
Boost::Asio

<h3>Usage</h3>
On Debian-like systems, just go to the directory and enter: make

There are several clients shipped as well. For example to connect with and write to the server, you can use the following code.

```php
<?php
  require("JsonClient.php");
  $session = new Session("127.0.0.1", 12122);
  echo $session->execute("{\"hello\":\"world\"}");
?>
```

If you are running the example server, you get {"Goodbye":"world"} as response.