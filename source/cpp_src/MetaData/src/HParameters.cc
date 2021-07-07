#include "HParameters.hh"
#include "HTokenizer.hh"


namespace hose 
{

HParameters::HParameters()
{
    //load up the default parameters (some are compiler dependent)


}

HParameters::~HParameters()
{

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