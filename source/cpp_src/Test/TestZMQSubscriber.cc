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
    zmq::socket_t subscriber(context, ZMQ_DISH);
    subscriber.bind("udp://*:8181");
    subscriber.join("noise_power");

    int previousNumber = 0;
    int lostCount = -1;

    while(subscriber.connected())
    {
        zmq::message_t update;

        subscriber.recv(&update);

        std::string text(update.data<const char>(), update.size());
        std::cout << text << std::endl;
    }


}
