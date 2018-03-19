#include <iostream>
#include <vector>
#include <memory>

#include "HPX14Digitizer.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{
    HPX14Digitizer px14dig;
    bool initval = px14dig.Initialize();
    px14dig.TearDown();
    return 0;
}
