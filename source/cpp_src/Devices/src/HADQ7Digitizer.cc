#include <iostream>
#include <unistd.h>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include <time.h>
#include <pthread.h>
//needed for mkdir
#include <sys/types.h>
#include <sys/stat.h>

#include "HADQ7Digitizer.hh"
#include "HBufferAllocatorMalloc.hh"

#define CHECKADQ(f) if(!(f)){std::cout<<"Error in " #f ""<<std::endl; std::exit(1);}

#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

namespace hose {

HADQ7Digitizer::HADQ7Digitizer():
    HDigitizer< ADQ7_SAMPLE_TYPE, HADQ7Digitizer >(),
    fBoardInterfaceInitialized(false),
    fADQControlUnit(nullptr),
    fADQDeviceNumber(0),
    fSerialNumber(""),
    fADQAPIRevision(0),
    fDecimationFactor(8),
    fSampleRate(0.0),
    fSampleRateMHz(0.0),
    fNChannels(0),
    fEnableA(1),
    fEnableB(0),
    fTestPattern(0),
    fNThreads(6),
    fSignalTerminate(false),
    fForceTerminate(true),
    fSleepTime(1),
    fErrorCode(0)
{
    //TODO replace this with a host-pinned memory allocator (i.e. something using CudaMallocHost)
    //better yet ma
    this->fAllocator =  new HBufferAllocatorMalloc< HADQ7Digitizer::sample_type >();
}

HADQ7Digitizer::~HADQ7Digitizer()
{
    delete this->fAllocator;

    if(fADQControlUnit)
    {
        DeleteADQControlUnit(fADQControlUnit);
        fADQControlUnit = nullptr;
    }
}

void 
HADQ7Digitizer::SetDecimationFactor(unsigned int factor)
{
    if(factor == 1 || factor == 2 || factor == 4 || factor ==8)
    {
        fDecimationFactor = factor;
    }
    else
    {
        std::cout<<"only 1, 2, 4 or 8 are supported decimation factors. Using 8."<<std::endl;
        fDecimationFactor = 8;
    }
}

bool
HADQ7Digitizer::InitializeImpl()
{
    if(!fBoardInterfaceInitialized)
    {
        fBoardInterfaceInitialized = InitializeBoardInterface();
    }

    if(!fBoardInterfaceInitialized)
    {
        return false;
    }



    // //try to set things up to use channels A&B
    // unsigned int enableAFE_AB = 0x000F;
    // ADQ_SetAfeSwitch(fADQControlUnit, fADQDeviceNumber, enableAFE_AB);
    //get the board sample rate (base clock, assuming no decimation)
    fSampleRate = 0.0;
    ADQ_GetSampleRate(fADQControlUnit, fADQDeviceNumber, 0, &fSampleRate);
    
    //correct for decimation
    fSampleRate /= fDecimationFactor;
    fSampleRateMHz = fSampleRate/1e6;
    std::cout<<"sample rate (MHz) = "<<fSampleRateMHz<<std::endl;

    //get number of channels
    fNChannels = ADQ_GetNofChannels(fADQControlUnit, fADQDeviceNumber);

    std::cout<<"n channels = "<<fNChannels<<std::endl;


    unsigned int tlocal = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 0)/256;
    unsigned int tr1 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 1)/256;
    unsigned int tr2 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 2)/256;
    unsigned int tr3 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 3)/256;
    unsigned int tr4 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 4)/256;
    std::cout<<"Temperatures:\n\tLocal: "<<tlocal<<"\n\tADC0: "<<tr1<<"\n\tADC1: "<<tr2<<"\n\tFPGA: "<<tr3<<"\n\tPCB diode: "<<tr4<<"\n"<<std::endl;

    //from the raw streaming example
    unsigned int sample_skip = fDecimationFactor;



    std::cout<<"\nSetting up streaming..."<<std::endl;

    //Enable streaming
    CHECKADQ(ADQ_SetSampleSkip(fADQControlUnit, fADQDeviceNumber, sample_skip));

    std::cout<<"setting test pattern mode: "<<fTestPattern<<std::endl;


    CHECKADQ(ADQ_SetTestPatternMode(fADQControlUnit,fADQDeviceNumber, fTestPattern));
    CHECKADQ(ADQ_SetStreamStatus(fADQControlUnit, fADQDeviceNumber, 1));
    CHECKADQ(ADQ_SetStreamConfig(fADQControlUnit, fADQDeviceNumber, 2, 1)); //RAW mode
    CHECKADQ(ADQ_SetStreamConfig(fADQControlUnit, fADQDeviceNumber, 3, 1*fEnableA + 2*fEnableB)); //mask
    //CHECKADQ(ADQ_SetTriggerMode(fADQControlUnit, fADQDeviceNumber,  ADQ_SW_TRIGGER_MODE));
    CHECKADQ(ADQ_SetTriggerMode(fADQControlUnit, fADQDeviceNumber, ADQ_EXT_TRIGGER_MODE));
    CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));


    //wait for the card to catch up
    sleep(1);

    fCounter = 0;

    return true;
}

void
HADQ7Digitizer::AcquireImpl()
{
    //Enable streaming
    CHECKADQ(ADQ_SetStreamStatus(fADQControlUnit, fADQDeviceNumber, 1));
    CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));


    fForceTerminate = false;
    fSignalTerminate = false;
    fErrorCode = 0;
    fCounter = 0;

    if(fMemcpyArgQueue.size() != 0)
    {
        std::lock_guard< std::mutex > lock(fQueueMutex);
        while(fMemcpyArgQueue.size() != 0)
        {
            fMemcpyArgQueue.pop();
        }
    }

    std::cout<<"Collecting data, please wait..."<<std::endl;

    //launch the aquisition threads
    LaunchThreads();

    //time handling is quick and dirty, need to improve this (i.e if we are close to a second-roll over, etc)
    //Note: that the acquisition starts on the next second tick (with the trigger)
    //POSIX expectation is seconds since unix epoch (1970, but this is no guaranteed by the standard)
    fAcquisitionStartTime = std::time(nullptr);

    //create data output directory, need to make this configurable and move it elsewhere
    std::stringstream ss;
    ss << DATA_INSTALL_DIR;
    ss << "/";
    ss << fAcquisitionStartTime;
    int dirstatus;
    dirstatus = mkdir(ss.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    //arm card for external triggers
    CHECKADQ(ADQ_StartStreaming(fADQControlUnit, fADQDeviceNumber));
    //CHECKADQ(ADQ_SWTrig(fADQControlUnit, fADQDeviceNumber));
}

void
HADQ7Digitizer::TransferImpl()
{
    //configure buffer information, cat time to uint64_t and set, then set the sample rate
    this->fBuffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
    this->fBuffer->GetMetaData()->SetSampleRate(fSampleRate); //check that double to uint64_t conversion is OK here

    unsigned int n_samples_collect  = this->fBuffer->GetArrayDimension(0);
    unsigned int samples_to_collect = this->fBuffer->GetArrayDimension(0);
    unsigned int buffers_filled = 0;
    int collect_result = 0;

    #ifdef TIMEOUT_WHEN_POLLING
        unsigned int timeout = 0;
    #endif

    //start timer
    #ifdef DEBUG_TIMER
        timespec start;
        timespec end;
    	clock_gettime(CLOCK_REALTIME, &start);
    #endif

    while (samples_to_collect > 0)
    {
        unsigned int samples_in_buffer;
        #ifdef TIMEOUT_WHEN_POLLING
        timeout = 0;
        #endif
        do
        {
            collect_result = ADQ_GetTransferBufferStatus(fADQControlUnit, fADQDeviceNumber, &buffers_filled);
            #ifdef TIMEOUT_WHEN_POLLING
                if( (buffers_filled == 0) && (collect_result) )
                {
                    timeout++;
                    if(timeout > 1000000)
                    {
                        std::cout<<"Error: Time out during data aquisition!"<<std::endl;
                        //stop the card aquisition and bail out
                        fErrorCode = 1;
                        samples_to_collect = 0;
                        return;
                    }
                    usleep(1);
                }
            #endif
        }
        while( (buffers_filled == 0) && (collect_result) && (!fErrorCode));

        collect_result = ADQ_CollectDataNextPage(fADQControlUnit, fADQDeviceNumber);
        samples_in_buffer = MIN(ADQ_GetSamplesPerPage(fADQControlUnit, fADQDeviceNumber), samples_to_collect);

        if(ADQ_GetStreamOverflow(fADQControlUnit, fADQDeviceNumber))
        {
            fErrorCode = 1;
            std::cout<<"Warning: Card streaming overflow!"<<std::endl;
            collect_result = 0;
            samples_to_collect = 0;
        }

        if(collect_result)
        {
            //push the mempy arguments to the thread pool queue
            void* dest = (void*) &( (this->fBuffer->GetData())[n_samples_collect-samples_to_collect]);
            void* src = ADQ_GetPtrStream(fADQControlUnit, fADQDeviceNumber);
            size_t sz = samples_in_buffer*sizeof(signed short);
            samples_to_collect -= samples_in_buffer;
            std::lock_guard< std::mutex > lock(fQueueMutex);
            fMemcpyArgQueue.push( std::make_tuple(dest, src, sz) );
            //std::cout<<"push, sz = "<<sz<<std::endl;
        }
        else
        {
            std::cout<<"Warning: Collect next data page failed!"<<std::endl;
            fErrorCode = 2;
            samples_to_collect = 0;
        }
    }

    #ifdef DEBUG_TIMER
        //stop timer and print
        clock_gettime(CLOCK_REALTIME, &end);
        
        timespec temp;
        if( (end.tv_nsec-start.tv_nsec) < 0)
        {
            temp.tv_sec = end.tv_sec-start.tv_sec-1;
            temp.tv_nsec = (1000000000+end.tv_nsec)-start.tv_nsec;
        }
        else
        {
            temp.tv_sec = end.tv_sec-start.tv_sec;
            temp.tv_nsec = end.tv_nsec-start.tv_nsec;
        }
    	std::cout << temp.tv_sec << "." << temp.tv_nsec << " sec for xfer "<<std::endl;
        std::cout<<"to collect: "<<this->fBuffer->GetArrayDimension(0)<<" samples."<<std::endl;
    #endif
}


int
HADQ7Digitizer::FinalizeImpl()
{
    bool threads_busy = true;
    while(fMemcpyArgQueue.size() != 0 || threads_busy )
    {
        if( AllThreadsAreIdle() ){threads_busy = false;}
        else{ threads_busy = true; }
    }

    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);

    //return any error codes which might have arisen during streaming
    //if were buffer overflows/page errors we need to stop and restart the acquisition 
    return fErrorCode;

    //have finished filling the buffer at this point, but the card is still streaming
    //better get back to work fast...
}

void
HADQ7Digitizer::StopImpl()
{
    SignalTerminateOnComplete();
    //join the read threads
    JoinThreads();
    //Disable XDAM testpattern if it was enabled
    //CHECKADQ(ADQ_SetDMATest(fADQControlUnit, fADQDeviceNumber, 0x50, 0));
    CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));
}

void
HADQ7Digitizer::TearDownImpl()
{
    ForceTermination();
    //join the read threads
    JoinThreads();
    CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));
    DeleteADQControlUnit(fADQControlUnit);
    fADQControlUnit = nullptr;
}




//create and launch the threads doing the read routine
void
HADQ7Digitizer::LaunchThreads()
{
    fThreadIdleMap.clear();
    unsigned int num_cpus = std::thread::hardware_concurrency();
    std::cout<<"hardware concurrency = "<<num_cpus<<std::endl;
    if(fThreads.size() == 0)
    {
        fSignalTerminate = false;
        fForceTerminate = false;
        for(unsigned int i=0; i<fNThreads; i++)
        {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET((i+2)%num_cpus, &cpuset);
            std::cout<<"setting digitizer thread #"<<i<<" to cpu #"<<(i+2)%num_cpus<<std::endl;
            fThreads.push_back( std::thread( &HADQ7Digitizer::ReadLoop, this ) );
            int rc = pthread_setaffinity_np(fThreads.back().native_handle(), sizeof(cpu_set_t), &cpuset);
            if(rc != 0)
            {
                std::cout<<"Error, couldnt set thread affinity"<<std::endl;
            }
        }
    }
    else
    {
        std::exit(1);
        //error, threads already launched
    }
}

//signal to the threads to terminate on completion of work
void
HADQ7Digitizer::SignalTerminateOnComplete(){fSignalTerminate = true;}

//force the threads to abandon any remaining work, and terminate immediately
void
HADQ7Digitizer::ForceTermination(){fForceTerminate = true;}

//join and destroy threads
void
HADQ7Digitizer::JoinThreads()
{
    std::cout<<"calling join"<<std::endl;
    for(unsigned int i=0; i<fThreads.size(); i++)
    {
        fThreads[i].join();
    }
    fThreads.clear();
    fThreadIdleMap.clear();
}

void
HADQ7Digitizer::InsertIdleIndicator()
{
    std::lock_guard< std::mutex > lock(fIdleMutex);
    fThreadIdleMap.insert( std::pair<std::thread::id, bool>( std::this_thread::get_id(), false) );
}

void
HADQ7Digitizer::SetIdleIndicatorFalse()
{
    std::lock_guard< std::mutex > lock(fIdleMutex);
    auto indicator = fThreadIdleMap.find( std::this_thread::get_id() );
    indicator->second = false;
}

void
HADQ7Digitizer::SetIdleIndicatorTrue()
{
    std::lock_guard< std::mutex > lock(fIdleMutex);
    auto indicator = fThreadIdleMap.find( std::this_thread::get_id() );
    indicator->second = true;
}

bool
HADQ7Digitizer::AllThreadsAreIdle()
{
    std::lock_guard< std::mutex > lock(fIdleMutex);
    for(auto it=fThreadIdleMap.begin(); it != fThreadIdleMap.end(); it++)
    {
        if(it->second == false)
        {
            return false;
        }
    }
    return true;
}

void
HADQ7Digitizer::ReadLoop()
{
    InsertIdleIndicator();

    while( !fForceTerminate && (!fSignalTerminate || fMemcpyArgQueue.size() != 0 ) )
    {
        void* dest = nullptr;
        void* src = nullptr;
        size_t sz = 0;

        auto args = std::make_tuple(dest, src, sz);
        if( fMemcpyArgQueue.size() != 0 )
        {
            //get lock on mutex for queue modification
            std::lock_guard< std::mutex > lock(this->fQueueMutex);
            if( fMemcpyArgQueue.size() != 0 )
            {
                //grab the locations in the queue we need to perform memcpy on
                args = fMemcpyArgQueue.front();
                dest = std::get<0>(args);
                src = std::get<1>(args);
                sz = std::get<2>(args);
                fMemcpyArgQueue.pop();
                //std::cout<<"pop"<<std::endl;
            }
        }

        if(  dest != nullptr &&  src != nullptr && sz != 0)
        {
            //do the memcpy
            SetIdleIndicatorFalse();
            memcpy(dest, src, sz);
            // char v = 0X7F;
            // memset(dest,v,sz);
            SetIdleIndicatorTrue();
        }

        SetIdleIndicatorTrue();
    }
}




bool HADQ7Digitizer::InitializeBoardInterface()
{
    unsigned int n_of_devices = 0;
    int n_of_failed = 0;
    unsigned int adq_num = 0;
    unsigned int tmp_adq_num = 0;
    int n_of_opened_devices = 0;
    unsigned int pID = 0;
    int n_of_ADQ = 0;
    int apirev = 0;
    int exit = 0;
    char* product_name;
    void* adq_cu;
    struct ADQInfoListEntry* ADQlist;
    unsigned int err;


    apirev = ADQAPI_GetRevision();

    std::cout<<"ADQAPI Example"<<std::endl;
    std::cout<<"API Revision: "<< apirev<<std::endl;

    adq_cu = CreateADQControlUnit();	//creates an ADQControlUnit
    if(!adq_cu)
    {
        std::cout<<"Failed to create adq_cu!"<<std::endl;
        return 0;
    }

    ADQControlUnit_EnableErrorTrace(adq_cu, LOG_LEVEL_INFO, ".");


    if(!ADQControlUnit_ListDevices(adq_cu, &ADQlist, &n_of_devices))
    {
        std::cout<<"ListDevices failed!"<<std::endl;
        err = ADQControlUnit_GetLastFailedDeviceError(adq_cu);
        std::cout<<" Last error reported is: "<<err<<std::endl;
        if (err == 0x00000001)
        {
            std::cout<<"ERROR: The linked ADQAPI is not for the correct OS, please select correct x86/x64 platform when building."<<std::endl;
        }
        return 0;
    }

    adq_num = 0xFFFFFFFF;

    if(n_of_devices == 0)
    {
        std::cout<<"No devices found!"<<std::endl;
        DeleteADQControlUnit(adq_cu);
        return 0;
    }

    for(tmp_adq_num = 0; tmp_adq_num < n_of_devices; tmp_adq_num++)
    {
        std::cout<<"Entry #" << tmp_adq_num <<std::endl;
        if( ADQlist[tmp_adq_num].ProductID == PID_ADQ7 )
        {
            std::cout<<"ADQ7"<<std::endl; 
            adq_num = tmp_adq_num;
        }
        std::cout<< "[PID "<<ADQlist[tmp_adq_num].ProductID<<";";
        std::cout<<" Addr1 "<< ADQlist[tmp_adq_num].AddressField1 <<";";
        std::cout<<" Addr2 "<< ADQlist[tmp_adq_num].AddressField2<<";";
        std::cout<<" HWIF "<<ADQlist[tmp_adq_num].HWIFType<<";";
        std::cout<<" Setup "<<ADQlist[tmp_adq_num].DeviceSetupCompleted<<" ]"<<std::endl;
    }



    std::cout<<"Opening device..."<<std::endl;

    if(ADQControlUnit_OpenDeviceInterface(adq_cu, adq_num))
    {
        std::cout<<"success!"<<std::endl;
    }
    else
    {
        std::cout<<"failed!"<<std::endl;
        std::exit(1);
    }

    std::cout<<"Setting up device... "<<std::endl;

    if(ADQControlUnit_SetupDevice(adq_cu, adq_num))
    {
        std::cout<<"success!"<<std::endl;
    }
    else
    {
        std::cout<<"failed!"<<std::endl;
        std::exit(1);
    }

    n_of_ADQ = ADQControlUnit_NofADQ(adq_cu);

    std::cout<<"Total opened units: "<< n_of_ADQ <<std::endl;

    n_of_failed = ADQControlUnit_GetFailedDeviceCount(adq_cu);

    if (n_of_failed > 0)
    {
        std::cout<<"Found but failed to start "<<n_of_failed<<" ADQ devices."<<std::endl;
        std::exit(1);
    }

    if (n_of_devices == 0)
    {
        std::cout<<"No ADQ devices found."<<std::endl;
        std::exit(1);
    }


    n_of_opened_devices = ADQControlUnit_NofADQ(adq_cu);
    std::cout<<"\nNumber of opened ADQ devices found: "<< n_of_opened_devices <<std::endl;

    for (adq_num = 1; adq_num <= (unsigned int) n_of_opened_devices; adq_num++)
    {
        product_name = ADQ_GetBoardProductName(adq_cu, adq_num);
        std::cout<<"Produce name: "<<product_name<<" number: "<<adq_num<<std::endl;
    }

    //set number to 1
    adq_num = 1;

    pID = ADQ_GetProductID(adq_cu, adq_num);
    if(pID != PID_ADQ7)
    {
        std::cout<<"error could not find ADQ7"<<std::endl;
        std::exit(1);
    }

    //ADQ API related data
    fADQControlUnit = adq_cu;
    fADQDeviceNumber = adq_num;
    fSerialNumber = std::string( ADQ_GetBoardSerialNumber(fADQControlUnit, fADQDeviceNumber) );
    fADQAPIRevision = apirev;

    int nof_channels = ADQ_GetNofChannels(adq_cu, adq_num);


    std::cout<<"device num = "<<fADQDeviceNumber<<" serial num = "<<fSerialNumber<<" apirev = "<<fADQAPIRevision<<std::endl;

    std::cout<<"n of device channels = "<<nof_channels<<std::endl;



    CHECKADQ( ADQ_SetClockSource( fADQControlUnit, fADQDeviceNumber, 1) );
    // //set test pattermode 
    // CHECKADQ(ADQ_SetTestPatternMode(adq_cu,adq_num, 2));

    unsigned int adx_mode = 0;
    unsigned char addr  = '\0';
    unsigned int retval = ADQ_SetInterleavingIPBypassMode(fADQControlUnit, fADQDeviceNumber, addr, adx_mode) ;

    std::cout<<"ADX query retval = "<<retval<<std::endl;

    std::cout<<"ADX mode is set to: "<<adx_mode<<std::endl;


    return true;
}


}
