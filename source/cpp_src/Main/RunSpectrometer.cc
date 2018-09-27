#include "HSpectrometerManager.hh"
#include "HPX14DigitizerSimulator.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{
    HSpectrometerManager<HPX14DigitizerSimulator>* specManager = new HSpectrometerManager<>();
    
    specManager->Initialize();

    std::thread spectrometer_thread( &HSpectrometerManager<HPX14DigitizerSimulator>::Run, specManager);

    spectrometer_thread.join();

    return 0;
}
