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

HParameters::HParameters(const HParameters& copy)
{
    fParameterFile = copy.fParameterFile;
    fHaveParameterFile = copy.fHaveParameterFile;
    fIntegerParam = copy.fIntegerParam;
    fStringParam = copy.fStringParam;
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
    fStringParam[std::string("noise_power_ip_address")] = std::string("192.52.61.185"); //curie
    fStringParam[std::string("noise_power_port")] = std::string("8181");
    fStringParam[std::string("window_type")] = std::string("blackman_harris");

    fIntegerParam[std::string("noise_power_udp_skip_interval")] = 8; //will only send a noise power UDP packet every n-th buffer
    //select the spectral bins to sum in the narrow-band noise power estimate
    fIntegerParam[std::string("spectral_noise_power_bin_low")] = 0;
    fIntegerParam[std::string("spectral_noise_power_bin_high")] = 0;


    #ifdef HOSE_USE_PX14
    fIntegerParam[std::string("n_ave_spectra_gpu")] = 16;
    fIntegerParam[std::string("n_ave_spectra_cpu")] = 12;
    fIntegerParam[std::string("n_fft_pts")] = 2097152;
    fIntegerParam[std::string("n_digitizer_threads")] = 16;
    fIntegerParam[std::string("n_digitizer_pool_size")] = 32;
    fIntegerParam[std::string("n_spec_pool_size")] = 16;
    fIntegerParam[std::string("n_spec_threads")] = 3;
    fIntegerParam[std::string("n_dump_skip")] = 120;
    fIntegerParam[std::string("n_spec_ave_pool_size")] = 12;
    #endif

    #ifdef HOSE_USE_ADQ7
    fIntegerParam[std::string("n_adq7_sample_skip")] = 2;
    fIntegerParam[std::string("n_ave_spectra_gpu")] = 16;
    fIntegerParam[std::string("n_ave_spectra_cpu")] = 32;
    fIntegerParam[std::string("n_fft_pts")] = 2097152;
    fIntegerParam[std::string("n_digitizer_threads")] = 2;
    fIntegerParam[std::string("n_digitizer_pool_size")] = 128;
    fIntegerParam[std::string("n_spec_pool_size")] = 16;
    fIntegerParam[std::string("n_spec_threads")] = 2;
    fIntegerParam[std::string("n_dump_skip")] = 120;
    fIntegerParam[std::string("n_spec_ave_pool_size")] = 20;
    #endif

    #ifndef HOSE_USE_ADQ7
        #ifndef HOSE_USE_PX14
            fIntegerParam[std::string("n_ave_spectra_gpu")] = 16;
            fIntegerParam[std::string("n_ave_spectra_cpu")] = 2;
            fIntegerParam[std::string("n_fft_pts")] = 2097152;
            fIntegerParam[std::string("n_digitizer_threads")] = 1;
            fIntegerParam[std::string("n_digitizer_pool_size")] = 8;
            fIntegerParam[std::string("n_spec_pool_size")] = 4;
            fIntegerParam[std::string("n_spec_threads")] = 1;
            fIntegerParam[std::string("n_dump_skip")] = 100;
            fIntegerParam[std::string("n_spec_ave_pool_size")] = 4;
        #endif
    #endif
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
                        std::cout<<"setting: "<<fTokens[0]<<", "<<fTokens[1]<<std::endl;
                        fStringParam[ fTokens[0] ] = fTokens[1];
                    }

                    auto param = fIntegerParam.find(fTokens[0]);
                    if(param != fIntegerParam.end() )
                    {
                        //we have an integer parameter
                        std::cout<<"setting: "<<fTokens[0]<<", "<<fTokens[1]<<std::endl;
                        fIntegerParam[ fTokens[0] ] = atoi(fTokens[1].c_str());
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
HParameters::GetIntegerParameter(const char* name)
{
    return GetIntegerParameter(std::string(name));
}

std::string 
HParameters::GetStringParameter(const char* name)
{
    return GetStringParameter(std::string(name));
}

int
HParameters::GetIntegerParameter(std::string name)
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
