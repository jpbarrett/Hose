#include "HSpectrometerManager.hh"

#include <getopt.h>

using namespace hose;

int main(int argc, char** argv)
{

    std::string usage = "RunSpectrometer -f <config_file>";
    std::string config_file = "";

    static struct option longOptions[] = {{"help", no_argument, 0, 'h'},
                                          {"config_file", required_argument, 0, 'f'}};

    static const char* optString = "hf:";

    while(true)
    {
        char optId = getopt_long(argc, argv, optString, longOptions, NULL);
        if (optId == -1)
            break;
        switch(optId)
        {
            case ('h'):  // help
                std::cout << usage << std::endl;
                return 0;
            case ('f'):
                config_file = std::string(optarg);
                break;
            default:
                std::cout << usage << std::endl;
                return 1;
        }
    }

    HSpectrometerManager<>* specManager = new HSpectrometerManager<>();
    HParameters* param = new HParameters();
    if(config_file != "")
    {

        param->SetParameterFilename(fname);
        param->ReadParameters();

    }
    else
    {
        std::cout<<"No config file given, using defaults."<<std::endl;
    }


    specManager->Initialize();

    std::thread spectrometer_thread( &HSpectrometerManager<>::Run, specManager);

    spectrometer_thread.join();

    return 0;
}
