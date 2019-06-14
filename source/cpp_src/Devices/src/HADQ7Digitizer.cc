#include "HADQ7Digitizer.hh"

#define CHECKADQ(f) if(!(f)){std::cout<<"Error in " #f ""<<std::endl; std::exit(1);}
#define MIN_MACRO(a,b) ((a) > (b) ? (b) : (a))

namespace hose
{

HADQ7Digitizer::HADQ7Digitizer():
    fSidebandFlag('?'),
    fPolarizationFlag('?'),
    fClockMode(0), //0 is internal 10MHz reference, 1 uses external 10MHz reference
    fADQControlUnit(nullptr),
    fADQDeviceNumber(1), //board id
    fSerialNumber(),
    fEnableA(1),
    fEnableB(0),
    fNChannels(0),
    fUseSoftwareTrigger(true),
    fSampleRate(0),
    fAcquisitionRateMHz(0), //effective sampling frequency in MHz
    fSampleSkipFactor(2),
    fInitialized(false),
    fArmed(false),
    fStopAfterNextBuffer(false),
    fCounter(0),
    fBufferCode(HProducerBufferPolicyCode::unset),
    fErrorCode(0)
{

}

HADQ7Digitizer::~HADQ7Digitizer()
{
    Stop();
    TearDown();
}


void
HADQ7Digitizer::SetSampleSkipFactor(unsigned int factor)
{
    if(factor == 1 || factor == 2 || factor == 4 || factor ==8)
    {
        fSampleSkipFactor = factor;
    }
    else
    {
        std::cout<<"only 1, 2, 4 or 8 are supported decimation factors. Using a factor of 8."<<std::endl;
        fSampleSkipFactor = 8;
    }
}

bool HADQ7Digitizer::InitializeImpl()
{
    if(!fInitialized)
    {
        //set up basic control unit for the ADQ7
        ConfigureBoardInterface();

        //get the raw board sample rate (base clock, assuming no decimation)
        fSampleRate = 0.0;
        ADQ_GetSampleRate(fADQControlUnit, fADQDeviceNumber, 0, &fSampleRate);

        //determine effective rate depending on sample-skip
        fAcquisitionRateMHz = (fSampleRate/fSampleSkipFactor)/1e6;
        std::cout<<"Raw sample rate (MHz):"<<fSampleRate/1e6<<", Effective sample rate (MHz): "<<fAcquisitionRateMHz<<std::endl;

        //Enable streaming
        std::cout<<"\nSetting up streaming..."<<std::endl;
        CHECKADQ(ADQ_SetSampleSkip(fADQControlUnit, fADQDeviceNumber, fSampleSkipFactor));
        CHECKADQ(ADQ_SetStreamStatus(fADQControlUnit, fADQDeviceNumber, 1));
        CHECKADQ(ADQ_SetStreamConfig(fADQControlUnit, fADQDeviceNumber, 2, 1)); //Sample streaming in 'raw' (no header) mode
        CHECKADQ(ADQ_SetStreamConfig(fADQControlUnit, fADQDeviceNumber, 3, 1*fEnableA + 2*fEnableB)); //apply channel mask
        if(fUseSoftwareTrigger)
        {
            std::cout<<"ADQ7 will use a software trigger."<<std::endl;
            CHECKADQ(ADQ_SetTriggerMode(fADQControlUnit, fADQDeviceNumber,  ADQ_SW_TRIGGER_MODE));
        }
        else
        {
            std::cout<<"ADQ7 will use an external trigger."<<std::endl;
            CHECKADQ(ADQ_SetTriggerMode(fADQControlUnit, fADQDeviceNumber, ADQ_EXT_TRIGGER_MODE));
        }

        //no data streaming until the card is armed for acquisition
        CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));

        //wait for the card to catch up
        sleep(1);
        fInitialized = true;
    }
    return fInitialized;
}

void
HADQ7Digitizer::AcquireImpl()
{
    //Enable streaming, stop it if it was started already
    CHECKADQ(ADQ_SetStreamStatus(fADQControlUnit, fADQDeviceNumber, 1));
    CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));

    fArmed = false;
    fStopAfterNextBuffer = false;
    fForceTerminate = false;
    fSignalTerminate = false;
    fErrorCode = 0;
    fCounter = 0;

    //time handling is quick and dirty, need to improve this (i.e if we are close to a second-roll over, etc)
    //Note: that the acquisition starts on the next second tick (with the trigger)
    //POSIX expectation is seconds since unix epoch (1970, but this is no guaranteed by the standard)
    fAcquisitionStartTime = std::time(nullptr);

    //arm card for triggers
    CHECKADQ(ADQ_StartStreaming(fADQControlUnit, fADQDeviceNumber));

    //issue a software trigger if this feature is in use
    if(fUseSoftwareTrigger)
    {
        CHECKADQ(ADQ_SWTrig(fADQControlUnit, fADQDeviceNumber));
    }

    fArmed = true;
}

void
HADQ7Digitizer::TransferImpl()
{
    //configure buffer information, cast time to uint64_t and set, then set the sample rate
    if(fArmed && this->fBuffer != nullptr)
    {
        this->fBuffer->GetMetaData()->SetSidebandFlag(fSidebandFlag);
        this->fBuffer->GetMetaData()->SetPolarizationFlag(fPolarizationFlag);
        this->fBuffer->GetMetaData()->SetAcquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
        this->fBuffer->GetMetaData()->SetSampleRate(GetSamplingFrequency()); //check that double to uint64_t conversion is OK here
        uint64_t count = fCounter;
        this->fBuffer->GetMetaData()->SetLeadingSampleIndex(count);

        unsigned int n_samples_collect  = this->fBuffer->GetArrayDimension(0);
        int64_t samples_to_collect = this->fBuffer->GetArrayDimension(0);
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
            samples_in_buffer = MIN_MACRO(ADQ_GetSamplesPerPage(fADQControlUnit, fADQDeviceNumber), samples_to_collect);

            if(ADQ_GetStreamOverflow(fADQControlUnit, fADQDeviceNumber))
            {
                fErrorCode = 1;
                std::cout<<"Warning: Card streaming overflow!"<<std::endl;
                collect_result = 0;
                samples_to_collect = 0;
                fStopAfterNextBuffer = true;
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
                fStopAfterNextBuffer = true;
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
}

HDigitizerErrorCode
HADQ7Digitizer::FinalizeImpl()
{

    if(fArmed && this->fBuffer != nullptr)
    {
        bool threads_busy = true;
        while(fMemcpyArgQueue.size() != 0 || threads_busy )
        {
            if( AllThreadsAreIdle() ){threads_busy = false;}
            else{ threads_busy = true; }
        }

        //increment the sample counter
        fCounter += this->fBuffer->GetArrayDimension(0);

        //return any error codes which might have arisen during streaming
        //if were buffer overflows/page errors we need to stop and restart the acquisition
        if(!fErrorCode)
        {
            return HDigitizerErrorCode::success;
        }
        else
        {
            return HDigitizerErrorCode::card_buffer_overflow;
        }

    }
    //have finished filling the buffer at this point, but the card is still streaming
    //better get back to work fast...
}

void
HADQ7Digitizer::StopImpl()
{
    if(fADQControlUnit != nullptr)
    {
        CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));
        fArmed = false;
        fErrorCode = 0;
    }
}

void
HADQ7Digitizer::TearDownImpl()
{
    if(fADQControlUnit != nullptr)
    {
        CHECKADQ(ADQ_StopStreaming(fADQControlUnit, fADQDeviceNumber));
        DeleteADQControlUnit(fADQControlUnit);
        fADQControlUnit = nullptr;
    }
    fInitialized = false;
}


//required by the producer interface
void
HADQ7Digitizer::ExecutePreProductionTasks()
{
    this->Initialize();
}

void
HADQ7Digitizer::ExecutePostProductionTasks()
{
    this->Stop();
    this->TearDown();
    fErrorCode = 0;
}

void
HADQ7Digitizer::ExecutePreWorkTasks()
{
    if(fArmed)
    {
        //get a buffer from the buffer handler
        HLinearBuffer< adq_sample_t >* buffer = nullptr;
        fBufferCode = this->fBufferHandler.ReserveBuffer(this->fBufferPool, buffer);

        //set the digitizer buffer if succesful
        if( buffer != nullptr && (fBufferCode & HProducerBufferPolicyCode::success))
        {
            //successfully got a buffer, assigned it
            this->SetBuffer(buffer);
        }

    }
}

void
HADQ7Digitizer::DoWork()
{
    //we have an active buffer, transfer the data
    if( (fBufferCode & HProducerBufferPolicyCode::success) && fArmed)
    {
        this->Transfer();
    }
    else
    {
        Idle();
    }
}

void
HADQ7Digitizer::ExecutePostWorkTasks()
{
    if( (fBufferCode & HProducerBufferPolicyCode::success) && fArmed)
    {
        HDigitizerErrorCode finalize_code = this->Finalize();
        if(finalize_code == HDigitizerErrorCode::success)
        {
            fBufferCode = this->fBufferHandler.ReleaseBufferToConsumer(this->fBufferPool, this->fBuffer);
            this->fBuffer = nullptr;
        }
        else
        {
            //some error occurred, stop production so we can re-start
            fBufferCode = this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, this->fBuffer);
            this->fBuffer = nullptr;
            this->Stop();
        }
    }

    if(fStopAfterNextBuffer)
    {
        this->Stop();
        fStopAfterNextBuffer = false;
    }
}

//needed by the thread pool interface
void
HADQ7Digitizer::ExecuteThreadTask()
{
    if(fArmed && fBuffer != nullptr)
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
            }
        }

        if(dest != nullptr &&  src != nullptr && sz != 0)
        {
            //do the memcpy
            memcpy(dest, src, sz);
            // char v = 0X7F;
            // memset(dest,v,sz);
        }
    }
}

bool
HADQ7Digitizer::WorkPresent()
{
    if( fMemcpyArgQueue.size() == 0)
    {
        return false;
    }
    return true;
}


void
HADQ7Digitizer::ConfigureBoardInterface()
{
    unsigned int n_of_devices = 0;
    int n_of_failed = 0;
    unsigned int adq_num = 0;
    unsigned int tmp_adq_num = 0;
    int n_of_opened_devices = 0;
    unsigned int pID = 0;
    int n_of_ADQ = 0;
    int apirev = 0;
    char* product_name;
    void* adq_cu;
    struct ADQInfoListEntry* ADQlist;
    unsigned int err;

    apirev = ADQAPI_GetRevision();
    std::cout<<"API Revision: "<< apirev<<std::endl;

    adq_cu = CreateADQControlUnit();	//creates an ADQControlUnit
    if(!adq_cu)
    {
        std::cout<<"Failed to create adq_cu!"<<std::endl;
        std::exit(1);
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
        std::exit(1);
    }

    adq_num = 0xFFFFFFFF;
    if(n_of_devices == 0)
    {
        std::cout<<"No devices found!"<<std::endl;
        DeleteADQControlUnit(adq_cu);
        std::exit(1);
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
        std::cout<<"Product name: "<<product_name<<" number: "<<adq_num<<std::endl;
    }

    //set number to device number
    adq_num = fADQDeviceNumber;
    pID = ADQ_GetProductID(adq_cu, adq_num);
    if(pID != PID_ADQ7)
    {
        std::cout<<"error could not find ADQ7"<<std::endl;
        std::exit(1);
    }

    //ADQ API related data
    fADQControlUnit = adq_cu;
    fSerialNumber = std::string( ADQ_GetBoardSerialNumber(fADQControlUnit, fADQDeviceNumber) );
    int nof_channels = ADQ_GetNofChannels(adq_cu, adq_num);
    std::cout<<"Device number = "<<fADQDeviceNumber<<", Serial number = "<<fSerialNumber<<std::endl;
    std::cout<<"Number of channels = "<<nof_channels<<std::endl;

    //get number of channels
    fNChannels = nof_channels;

    //report temperatures
    unsigned int tlocal = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 0)/256;
    unsigned int tr1 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 1)/256;
    unsigned int tr2 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 2)/256;
    unsigned int tr3 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 3)/256;
    unsigned int tr4 = ADQ_GetTemperature(fADQControlUnit, fADQDeviceNumber, 4)/256;
    std::cout<<"Temperatures:\n\tLocal: "<<tlocal<<"\n\tADC0: "<<tr1<<"\n\tADC1: "<<tr2<<"\n\tFPGA: "<<tr3<<"\n\tPCB diode: "<<tr4<<"\n"<<std::endl;

    //configure for internal/external clock
    CHECKADQ( ADQ_SetClockSource( fADQControlUnit, fADQDeviceNumber, fClockMode) );
}




}//end of namespace
