#ifndef HServer_HH__
#define HServer_HH__

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>

#include <queue>

/*
*File: HServer.hh
*Class: HServer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

namespace hose
{

class HServer
{
    public:
        HServer();
        virtual ~HServer();

        void Initialize();

        void Run();

        void Terminate();

        unsigned int GetNMessages();
        std::string PopMessage();

    private:
        
        bool CheckRequest(std::string message);


        std::string fConnection;

        zmq::context_t*  fContext;
        zmq::socket_t* fSocket;


        std::queue< std::string > fMessageQueue;
};

}

#endif /* end of include guard: HServer */
