#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <chrono>

int main() 
{

    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_RADIO);
    publisher.connect("udp://192.52.61.185:8181");

    std::string text;
    text.reserve(128);

    int number = 0;
    while(publisher.connected())
    {
        std::chrono::microseconds timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch());

        text.clear();
        text += std::to_string(timestamp.count());
        text += ";";
        text += std::to_string(++number);

        zmq::message_t update{text.data(), text.size()};
        update.set_group("test");
        std::cout << "Sending: " << timestamp.count() << " number:" << number << std::endl;
        publisher.send(update);
        sleep(1);
    }
}
