#include "HServer.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{

    HServer my_server;

    my_server.Initialize();

    my_server.Run();

    return 0;
}
