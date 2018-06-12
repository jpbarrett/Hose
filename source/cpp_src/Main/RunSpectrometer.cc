#include "HSpectrometerManager.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{
    HSpectrometerManager* specManager = new HSpectrometerManager();
    
    specManager->Initialize();

    std::thread daemon( &HSpectrometerManager::Run, specManager);

    daemon.detach();

    usleep(50);
    // specManager->Shutdown();
    //daemon.join();

    return 0;
}
