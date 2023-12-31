#include "HServer.hh"

namespace hose 
{

HServer::HServer():
    fStop(false),
    fAppBackend(nullptr)
    {
        fConnection = std::string("tcp://") + std::string(COMMAND_SERVER_IP_ADDRESS) + ":" +  std::string(COMMAND_SERVER_PORT);
    }

HServer::HServer(std::string ip, std::string port):
    fStop(false),
    fAppBackend(nullptr)
    {
        //note we do not check ip/port for validlity
        fConnection = "tcp://" + ip +":" + port; 
    }

HServer::~HServer(){}


void 
HServer::Initialize()
{
    if(fAppBackend == nullptr)
    {
        std::cout<<"HServer::Initialize(): Error, application backend unset."<<std::endl;
        std::exit(1);
    }

    //create the logger
    try
    {
        std::stringstream lfss;
        lfss << STR2(LOG_INSTALL_DIR);
        lfss << "/server.log";
        std::string command_logger_name("server");
        bool trunc = true;
        auto sink = std::make_shared<spdlog::sinks::simple_file_sink_mt>(  lfss.str().c_str(), trunc );
        fLogger = std::make_shared<spdlog::logger>(command_logger_name.c_str(), sink);
        fLogger->flush_on(spdlog::level::info); //make logger flush on every message
        fLogger->info("$$$ New session, server log initialized. $$$");
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Server log initialization failed: " << ex.what() << std::endl;
    }
}

void HServer::Run()
{
    fStop = false;

    zmq::context_t aContext(1);
    zmq::socket_t aSocket(aContext, ZMQ_REP);
    aSocket.bind( fConnection.c_str() );

    while(!fStop)
    {
        //  Wait for next request from client
        zmq::message_t request;
        aSocket.recv (&request);
        std::string request_data = std::string(static_cast<char*>( request.data() ), request.size() );

        #ifdef HOSE_USE_SPDLOG
        std::stringstream log_msg;
        log_msg <<"client_message % ";
        log_msg << request_data;
        fLogger->info( log_msg.str().c_str() );
        #endif

        //check the requests validity
        if( fAppBackend->CheckRequest(request_data) )
        {
            //push it into the queue where it can be grabbed by the application
            fMessageQueue.push(request_data);

            #ifdef HOSE_USE_SPDLOG
            std::stringstream log_msg;
            log_msg <<"client_request % valid, queue_size %";
            log_msg << fMessageQueue.size();
            fLogger->info( log_msg.str().c_str() );
            #endif

            //idle until the application has processed the message
            unsigned int count = 0;
            do
            {
                usleep(50);
                count++;
            }
            while(fMessageQueue.size() != 0 && count < 100);

            HStateStruct st = fAppBackend->GetCurrentState();
            //fomulate the appropriate reply, send back to client
            std::string reply_msg; 
            if(count < 100 )
            {
                reply_msg = st.status_message;
                #ifdef HOSE_USE_SPDLOG
                std::stringstream status_msg;
                status_msg <<"server_reply % ";
                status_msg << reply_msg;
                fLogger->info( status_msg.str().c_str() );
                #endif
            }
            else
            {
                reply_msg = std::string("timeout:") + st.status_message;
                #ifdef HOSE_USE_SPDLOG
                std::stringstream err_msg;
                err_msg <<"client_error % ";
                err_msg << reply_msg;
                fLogger->warn( err_msg.str().c_str() );
                #endif
            }
            zmq::message_t reply( reply_msg.size() );
            memcpy( (void *) reply.data (), reply_msg.c_str(), reply_msg.size() );
            aSocket.send(reply);
        }
        else
        {
            #ifdef HOSE_USE_SPDLOG
            std::stringstream log_msg;
            log_msg <<"client_request % invalid";
            fLogger->warn( log_msg.str().c_str() );
            #endif
            //error, can't understand the message
            //Send reply back to client
            std::string error_msg("error: invalid request.");
            zmq::message_t reply( error_msg.size() );
            memcpy( (void *) reply.data (), error_msg.c_str(), error_msg.size() );
            aSocket.send (reply);
        }
    }

    //close the socket, context_t
    aSocket.close();
    usleep(100);
    //no need to explicitly close the context_t, as this is done in its destructor
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
