#include <iostream>
#include <unistd.h>
#include <cmath>

#include "HDummyDigitizer.hh"
#include "HBufferAllocatorNew.hh"

namespace hose {

HDummyDigitizer::HDummyDigitizer():
    HDigitizer< uint16_t, HDummyDigitizer >()
{
    fRandom = new std::random_device();
    fGenerator = new std::mt19937( (*fRandom)() );
    fUniformDistribution = new std::uniform_int_distribution<uint16_t>( 0, 100 );
    this->fAllocator = new HBufferAllocatorNew< HDummyDigitizer::sample_type >();
    fCounter = 0;
}

HDummyDigitizer::~HDummyDigitizer()
{
    delete fRandom;
    delete fGenerator;
    delete fUniformDistribution;
    delete this->fAllocator;
}

bool 
HDummyDigitizer::InitializeImpl()
{
    fCounter = 0;
    return true;
}

void 
HDummyDigitizer::AcquireImpl()
{
    fCounter = 0;
    std::time_t result = std::time(nullptr);
    //cast to a uint64_t and set
    this->fBuffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) result );
    //set the sample rate
    this->fBuffer->GetMetaData()->SetSampleRate(1000000); //fake rate of 1MHz
}

void 
HDummyDigitizer::TransferImpl()
{
    fill_function();
}

int
HDummyDigitizer::FinalizeImpl()
{
    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);
    return 0;
}

void 
HDummyDigitizer::StopImpl()
{
    //do nothing
}

void 
HDummyDigitizer::TearDownImpl()
{
    //do nothing
}

void 
HDummyDigitizer::fill_function()
{
    unsigned int n = this->fBuffer->GetArrayDimension(0);
    uint16_t* buf = this->fBuffer->GetData();

    const unsigned int napp = 10;
    uint16_t sarr[napp];
    for(unsigned int i=0; i<napp; i++)
    {
        sarr[i] = 1000 + 1000*std::sin(2.0*M_PI*( ( (double)i )/( (double)napp) ) );
    }
    
    for(unsigned int i=0; i<n; i++)
    {
        //double arg = 2.0*M_PI*0.25*(( (double)n + (double)fCounter));
        // buf[i] = (*fUniformDistribution)(*fGenerator) + 100*sarr[(n+fCounter)%napp];
        buf[i] = sarr[(i+fCounter)%napp];
        // if((i+fCounter)%10 == 0)
        // {
        //     buf[i] += 100;
        // }
        // else
        // {
        //     buf[i] += 0;
        // }
        //100*sarr[(n+fCounter)%napp];
    }
}


}
