#ifndef HParameters_HH__
#define HParameters_HH__

#include <iostream>

/*
*File: HParameters.hh
*Class: HParameters
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: class to read/store spectrometer parameters (with reasonable defaults)
*/

#include <string>
#include <map>

#include "HTokenizer.hh"


namespace hose
{

class HParameters
{
    public:
        HParameters();
        HParameters(const HParameters& copy);
        virtual ~HParameters();

        //set a filename and read the parameter values
        void SetParameterFilename(std::string& file);
        void ReadParameters();

        //reset parameter values to the defaults
        void Initialize();
        void UseDefaultParameters();

        int GetIntegerParameter(std::string name);
        std::string GetStringParameter(std::string name);

        int GetIntegerParameter(const char* name);
        std::string GetStringParameter(const char* name);

        HParameters& operator=(const HParameters& rhs)
        {
            if(this != &rhs)
            {
                fParameterFile = rhs.fParameterFile;
                fHaveParameterFile = rhs.fHaveParameterFile;
                std::cout<<"before size of integer param = "<<fIntegerParam.size()<<std::endl;
                fIntegerParam = rhs.fIntegerParam;
                std::cout<<"before size of integer param = "<<fIntegerParam.size()<<std::endl;
                std::cout<<"fft size = "<<fIntegerParam[std::string("n_fft_pts")]<<std::endl;
                fStringParam = rhs.fStringParam;
                std::cout<<"copying over"<<std::endl;
            }
            return *this;
        }

    private:

        std::string fParameterFile;
        bool fHaveParameterFile;
        //integer parameter map
        std::map<std::string, int> fIntegerParam;
        //string parameter map
        std::map<std::string, std::string> fStringParam;

        HTokenizer fTokenizer;
        std::vector< std::string > fTokens;
};

}

#endif /* end of include guard: HParameters */
