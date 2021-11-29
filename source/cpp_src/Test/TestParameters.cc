#include <iostream>
#include <vector>
#include <memory>
#include <getopt.h>

#include "HParameters.hh"

using namespace hose;

int main(int argc, char** argv)
{
    std::string usage = "TestParameters -f <config_file>";
    std::string fname = "";

    static struct option longOptions[] = {{"help", no_argument, 0, 'h'},
                                          {"fname", required_argument, 0, 'f'}};

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
                fname = std::string(optarg);
                break;
            default:
                std::cout << usage << std::endl;
                return 1;
        }
    }

    if(fname != "")
    {
        HParameters* param = new HParameters();
        param->SetParameterFilename(fname);
        param->ReadParameters();
        delete param;
    }
    else
    {
        std::cout<<"no config file given."<<std::endl;
    }


    return 0;
}
