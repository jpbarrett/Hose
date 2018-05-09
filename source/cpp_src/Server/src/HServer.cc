#include "HServer.hh"

namespace hose 
{

HServer::HServer():
    fStop(false),
    fContext(nullptr),
    fSocket(nullptr)
    {
        fConnection = "tcp://127.0.0.1:12345";
    }

HServer::HServer(std::string ip, std::string port):
    fStop(false),
    fContext(nullptr),
    fSocket(nullptr)
    {
        //note we do not check ip/port for validlity
        fConnection = "tcp://" + ip +":" + port; 
    }

HServer::~HServer()
{
    if(fContext){delete fContext;};
    if(fSocket){delete fSocket;};
}

void 
HServer::Initialize()
{
    fContext = new zmq::context_t(1);
    fSocket = new zmq::socket_t(*fContext, ZMQ_REP);
    fSocket->bind( fConnection.c_str() );
}


void HServer::Run()
{
    fStop = false;

    while(!fStop)
    {
        //  Wait for next request from client
        zmq::message_t request;
        fSocket->recv (&request);
        std::string request_data = std::string(static_cast<char*>( request.data() ), request.size() );
        std::cout<<"got: "<<request_data<<std::endl;

        //check the requests validity
        if( CheckRequest(request_data) )
        {
            //push it into the queue where it can be grabbed but the application
            fMessageQueue.push(request_data);

            //fomulate the appropriate reply, for now just acknowledge
            //error, can't understand the message
            //Send reply back to client
            std::string reply_msg("Acknowledged");
            zmq::message_t reply( reply_msg.size() );
            memcpy( (void *) reply.data (), reply_msg.c_str(), reply_msg.size() );
            fSocket->send (reply);
        }
        else
        {
            //error, can't understand the message
            //Send reply back to client
            std::string error_msg("Error");
            zmq::message_t reply( error_msg.size() );
            memcpy( (void *) reply.data (), error_msg.c_str(), error_msg.size() );
            fSocket->send (reply);
        }
    }

    //close the socket, context_t
    fSocket->close();
    usleep(1000);
    fContext->close();

    delete fSocket; fSocket = nullptr;
    delete fContext; fContext = nullptr;
}


bool 
HServer::CheckRequest(std::string message)
{
    return true;
}


unsigned int 
HServer::GetNMessages()
{
    return fMessageQueue.size();
}

std::string
HServer::PopMessage()
{
    if(fMessageQueue.size() > 0)
    {
        std::string front = fMessageQueue.front();
        fMessageQueue.pop();
        return front;
    }

    return std::string("");

}


}
