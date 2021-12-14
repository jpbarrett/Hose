#ifndef HServer_HH__
#define HServer_HH__

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sstream>

#include <queue>

extern "C" 
{
    #include "HBasicDefines.h"
}


#include "HStateStruct.hh"
#include "HApplicationBackend.hh"
#include "HNetworkDefines.hh"


#ifdef HOSE_USE_SPDLOG
#include "spdlog/spdlog.h"
#endif


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
        HServer(std::string ip, std::string port);
        virtual ~HServer();

        void SetApplicationBackend(HApplicationBackend* backend){fAppBackend = backend;}

        void Initialize();

        void Run();

        void Terminate(){fStop = true;}

        unsigned int GetNMessages();
        std::string PopMessage();

    private:

        bool fStop;

        std::string fConnection;
        zmq::context_t*  fContext;
        zmq::socket_t* fSocket;

        HApplicationBackend* fAppBackend;

        std::queue< std::string > fMessageQueue;

        //logger
        #ifdef HOSE_USE_SPDLOG
        std::shared_ptr<spdlog::logger> fLogger;
        #endif
};

}

#endif /* end of include guard: HServer */
