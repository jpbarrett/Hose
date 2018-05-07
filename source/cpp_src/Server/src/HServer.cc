#include "HServer.hh"

namespace hose 
{

HServer::HServer()
{
    fConnection = "tcp://127.0.0.1:1234";
}

HServer::~HServer()
{

}

void 
HServer::Initialize()
{
    fContext = new zmq::context_t(1);
    fSocket = new zmq::socket_t(fContext, ZMQ_REP);
    fSocket.bind( fConnection.c_str() );
}


void HServer::Run()
{
    
    while (true)
    {
        //  Wait for next request from client
        zmq::message_t request;
        socket.recv (&request);
        std::string request_data = std::string(static_cast<char*>( request.data() ), request.size() );
        std::cout<<"got: "<<request_data<<std::endl;

        //check the requests validity
        if( CheckRequest(request_data) )
        {
            //push it into the queue where it can be grabbed
            fMessageQueue.push(request_data);

            //fomulate the appropriate reply
        }
        else
        {
            //error, can't understand the message
            //Send reply back to client
            zmq::message_t reply (5);
            memcpy ((void *) reply.data (), "Error", 5);
            socket.send (reply);
        }
    }
}


bool 
HServer::CheckRequest(std::string message)
{

}

void
HServer::Terminate()
{

}


unsigned int 
HServer::GetNMessages()
{

}

std::string
HServer::PopMessage()
{


}


}
