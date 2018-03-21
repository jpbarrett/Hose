#include <iostream>
#include <unistd.h>
#include <cmath>
#include <sstream>

#include "HDummyDigitizerSigned.hh"
#include "HBufferAllocatorNew.hh"

//needed for mkdir
#include <sys/types.h>
#include <sys/stat.h>


namespace hose {

HDummyDigitizerSigned::HDummyDigitizerSigned():
    HDigitizer< int16_t, HDummyDigitizerSigned >()
{
    fRandom = new std::random_device();
    fGenerator = new std::mt19937( (*fRandom)() );
    // fUniformDistribution = new std::uniform_int_distribution<int16_t>( std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max() );
    fUniformDistribution = new std::uniform_int_distribution<int16_t>( -10, 10 );
    this->fAllocator = new HBufferAllocatorNew< HDummyDigitizerSigned::sample_type >();
    fCounter = 0;
}

HDummyDigitizerSigned::~HDummyDigitizerSigned()
{
    delete fRandom;
    delete fGenerator;
    delete fUniformDistribution;
    delete this->fAllocator;
}

bool 
HDummyDigitizerSigned::InitializeImpl()
{
    fCounter = 0;
    return true;
}

void 
HDummyDigitizerSigned::AcquireImpl()
{
    fCounter = 0;
    fAcquisitionStartTime = std::time(nullptr);

    //create data output directory, need to make this configurable and move it elsewhere
    std::stringstream ss;
    ss << DATA_INSTALL_DIR;
    ss << "/";
    ss << fAcquisitionStartTime;
    int dirstatus;
    dirstatus = mkdir(ss.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void 
HDummyDigitizerSigned::TransferImpl()
{
    fill_function();
    this->fBuffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
    //set the sample rate
    this->fBuffer->GetMetaData()->SetSampleRate(2500000000);
}

int
HDummyDigitizerSigned::FinalizeImpl()
{
    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);
    return 0;
}

void 
HDummyDigitizerSigned::StopImpl()
{
    //do nothing
}

void 
HDummyDigitizerSigned::TearDownImpl()
{
    //do nothing
}

void 
HDummyDigitizerSigned::fill_function()
{
    unsigned int n = this->fBuffer->GetArrayDimension(0);
    int16_t* buf = this->fBuffer->GetData();

    const unsigned int napp = 10;
    int16_t sarr[napp];
    for(unsigned int i=0; i<napp; i++)
    {
        sarr[i] = 1000*( std::sin(2.0*M_PI*( ( (double)i )/( (double)napp) ) ) );// + 0.25*std::sin(4.0*M_PI*( ( (double)i )/( (double)napp) ) ) );
    }
    
    for(unsigned int i=0; i<n; i++)
    {
        buf[i] = 32768;// i%4;// (*fUniformDistribution)(*fGenerator) +  sarr[(i+fCounter)%napp];
        //buf[i] = sarr[(i+fCounter)%napp];
    }
}


}
