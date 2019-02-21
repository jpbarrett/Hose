#include <iostream>
#include <vector>
#include <memory>

#include "HATS9371Digitizer.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{
    HATS9371Digitizer digi;
    bool initval = digi.Initialize();
    digi.TearDown();
    return 0;
}
