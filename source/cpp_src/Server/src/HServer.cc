#include "HServer.hh"

namespace hose 
{

HServer::HServer():
    fStop(false),
    fSocket(nullptr),
    fContext(nullptr)
    {
        fConnection = "tcp://127.0.0.1:12345";
    }

HServer::~HServer()
{
    if(fContext){delete fConnection;};
    if(fSocket){delete fSocket}
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
    fStop = true;

    while(!fStop)
    {
        //  Wait for next request from client
        zmq::message_t request;
        socket.recv (&request);
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
            socket.send (reply);
        }
        else
        {
            //error, can't understand the message
            //Send reply back to client
            std::string error_msg("Error");
            zmq::message_t reply( error_msg.size() );
            memcpy( (void *) reply.data (), error_msg.c_str(), error_msg.size() );
            socket.send (reply);
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

void
HServer::Terminate()
{

}


unsigned int 
HServer::GetNMessages()
{
    return fMessageQueue.size();
}

std::string
HServer::PopMessage()
{
    if(fMessageQueue.size())
    {
        return fMessageQueue.pop();
    }
    else
    {
        return std::string("");
    }
}


}
