#include "HParameters.hh"
#include "HTokenizer.hh"


#include <iostream>
#include <fstream>
#include <cstdlib>

namespace hose
{

HParameters::HParameters()
{
    //load up the default parameters (some are compiler dependent)
    fTokenizer.SetIncludeEmptyTokensFalse();
    fTokenizer.SetDelimiter("=");

    Initialize();
}

HParameters::~HParameters()
{

}

void
HParameters::Initialize()
{
    //we need to set up all the default parameter values
    fStringParam[std::string("ip_address")] = std::string("127.0.0.1");
    fStringParam[std::string("port")] = std::string("12345");
    fStringParam[std::string("window_type")] = std::string("blackman_harris");

    // #ifdef HOSE_USE_PX14
    //     #define N_DIGITIZER_THREADS 16
    //     #define N_DIGITIZER_POOL_SIZE 32
    //     #define N_SPECTROMETER_POOL_SIZE 16
    //     #define N_NOISE_POWER_POOL_SIZE 10
    //     #define N_SPECTROMETER_THREADS 3
    //     #define N_NOISE_POWER_THREADS 1
    //     #define DUMP_FREQ 120
    //     #define N_AVE_BUFFERS 12 //this is about once every second
    //     #define SPEC_AVE_POOL_SIZE 12
    // #endif
    //
    // #ifdef HOSE_USE_ADQ7
    //     #define N_DIGITIZER_THREADS 2
    //     #define N_DIGITIZER_POOL_SIZE 128 //need 128 buffers in pool when running ADQ7 at 2.5GSPS (this configuration works)
    //     #define N_SPECTROMETER_POOL_SIZE 16
    //     #define N_NOISE_POWER_POOL_SIZE 32
    //     #define N_SPECTROMETER_THREADS 2
    //     #define N_NOISE_POWER_THREADS 2
    //     #define DUMP_FREQ 120
    //     #define N_AVE_BUFFERS 32
    //     // #define N_AVE_BUFFERS 12
    //     #define SPEC_AVE_POOL_SIZE 20
    // #endif

    // #ifndef HOSE_USE_ADQ7
    //     #ifndef HOSE_USE_PX14
    //         #define N_DIGITIZER_THREADS 1
    //         #define N_DIGITIZER_POOL_SIZE 8
    //         #define N_SPECTROMETER_POOL_SIZE 4
    //         #define N_SPECTROMETER_THREADS 1
    //         #define N_NOISE_POWER_POOL_SIZE 10
    //         #define N_NOISE_POWER_THREADS 1
    //         #define DUMP_FREQ 100
    //         #define N_AVE_BUFFERS 2
    //         #define SPEC_AVE_POOL_SIZE 4
    //     #endif
    // #endif




    fIntegerParam[std::string("sample_skip")] = 2;
    fIntegerParam[std::string("n_ave_spectra_gpu")] = 16;
    fIntegerParam[std::string("n_ave_spectra_cpu")] = 12;
    fIntegerParam[std::string("n_fft_pts")] = 2097152;



}

//set a filename and read the parameter values
void
HParameters::SetParameterFilename(std::string& file)
{
    fParameterFile = file;
    fHaveParameterFile = true;
}

void
HParameters::ReadParameters()
{
    std::ifstream aStream;
    aStream.open(fParameterFile.c_str(), std::ifstream::in);
    bool is_open = aStream.is_open();
    std::string tmpLine, aLine;

    if(is_open)
    {
        while(aStream.good())
        {
            std::getline(aStream, tmpLine);  //get the line
            //strip leading and trailing whitespace

            size_t begin;
            size_t end;
            size_t len;
            begin = tmpLine.find_first_not_of(" \t");
            if (begin != std::string::npos)
            {
                end = tmpLine.find_last_not_of(" \t");
                len = end - begin + 1;
                aLine = tmpLine.substr(begin, len);
                fTokenizer.SetString(&aLine);
                fTokenizer.GetTokens(&fTokens);

                //check if there are two tokens (name=value style)
                if(fTokens.size() == 2)
                {
                    auto sparam = fStringParam.find(fTokens[0]);
                    if(sparam != fStringParam.end() )
                    {
                        //we have a string parameter
                        std::cout<<"s setting: "<<fTokens[0]<<", "<<fTokens[1]<<std::endl;
                        fStringParam[ fTokens[0] ] = fTokens[1];
                    }

                    auto param = fIntegerParam.find(fTokens[0]);
                    if(param != fIntegerParam.end() )
                    {
                        //we have an integer parameter
                        std::cout<<"i setting: "<<fTokens[0]<<", "<<fTokens[1]<<std::endl;
                        fStringParam[ fTokens[0] ] = atoi(fTokens[1].c_str());
                    }
                }
            }
        }
    }
}

//reset parameter values to the defaults
void
HParameters::UseDefaultParameters()
{
    Initialize();
}

int
HParameters::GetIntegerParameter(std::string& name)
{
    auto param = fIntegerParam.find(name);
    if(param != fIntegerParam.end() )
    {
        return param->second;
    }
    else
    {
        //TODO handle error
        return -1;
    }
}

std::string
HParameters::GetStringParameter(std::string name)
{
    auto param = fStringParam.find(name);
    if(param != fStringParam.end() )
    {
        return param->second;
    }
    else
    {
        //TODO handle error
        return "";
    }
}

}
