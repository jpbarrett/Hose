#include <iostream>
#include <vector>
#include <memory>

#include "HBufferAllocatorNew.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{


    //create a vector of ints using a custom allocator
    std::vector< int, HBufferAllocatorNew<int> > a_vector;

    for(int i=0; i<10; i++)
    {
        a_vector.push_back(i);
    }

    for(int i=0; i<10; i++)
    {
        std::cout<<a_vector[i]<<", ";
    }
    std::cout<<std::endl;

    return 0;
}
