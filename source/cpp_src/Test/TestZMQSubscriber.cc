// Server side implementation of UDP client-server model
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
    subscriber.join("test");

    int previousNumber = 0;
    int lostCount = -1;

    while(subscriber.connected())
    {
        zmq::message_t update;

        subscriber.recv(&update);

        std::string text(update.data<const char>(), update.size());
        std::cout << text;

        auto splitPoint = text.find(';');
        std::string serverTime = std::string(text.substr(0, splitPoint));
        std::string serverNumber = std::string(text.substr(splitPoint + 1));
        auto number = std::stoi(serverNumber);
        if(number != previousNumber + 1)
        {
            ++lostCount;
        }
        previousNumber = number;

        const auto diff =
            std::chrono::system_clock::now() -
            std::chrono::system_clock::time_point{std::chrono::microseconds{std::stoull(serverTime)}};

        // Beautify at: https://github.com/gelldur/common-cpp/blob/master/src/acme/beautify.h
        std::cout << " ping:"  << "UDP lost: " << lostCount << std::endl;
    }


}
