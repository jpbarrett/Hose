#include <iostream>
#include <vector>
#include <memory>

#include "HADQ7Digitizer.hh"

using namespace hose;


int main(int /*argc*/, char** /*argv*/)
{
    HADQ7Digitizer adq7;
    bool initval = adq7.Initialize();
    return 0;
}
