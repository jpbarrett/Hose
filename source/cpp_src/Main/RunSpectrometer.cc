#include "HSpectrometerManager.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{
    HSpectrometerManager<>* specManager = new HSpectrometerManager<>();
    
    specManager->Initialize();

    std::thread spectrometer_thread( &HSpectrometerManager<>::Run, specManager);

    spectrometer_thread.join();

    return 0;
}
