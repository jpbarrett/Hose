#include "HParameters.hh"
#include "HTokenizer.hh"


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
    bool is_open = fFileStream.is_open();
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
                    auto param = fStringParam.find(fTokens[0]);
                    if(param != fStringParam.end() )
                    {
                        //we have a string parameter
                        fStringParam[ fTokens[0] ] = fTokens[1];
                    }

                    auto param = fIntegerParam.find(fTokens[0]);
                    if(param != fIntegerParam.end() )
                    {
                        //we have an integer parameter
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

}

int
HParameters::GetIntegerParameter(std::string& name)
{

}

std::string
HParameters::GetStringParameter(std::string name)
{

}

}
