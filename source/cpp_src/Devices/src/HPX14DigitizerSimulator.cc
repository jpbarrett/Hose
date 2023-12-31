#include "HPX14DigitizerSimulator.hh"

#define PX14_N_INTERNAL_BUFF 32
#define PX14_INTERNAL_BUFF_SIZE 1048576

#include <cstdlib>

extern "C"
{
    #include "HBasicDefines.h"
}

namespace hose
{

HPX14DigitizerSimulator::HPX14DigitizerSimulator():
    fSidebandFlag('?'),
    fPolarizationFlag('?'),
    fBoardNumber(1),
    fAcquisitionRateMHz(400), //px14400 sampling frequency is 400MHz in single-pol mode
    fConnected(false),
    fInitialized(false),
    fArmed(false),
    fCounter(0),
    fAcquireActive(false),
    fBufferCode(HProducerBufferPolicyCode::unset),
    fErrorCode(0),
    fNInternalBuffers(PX14_N_INTERNAL_BUFF),
    fInternalBufferSize(PX14_INTERNAL_BUFF_SIZE),
    fInternalBufferPool(nullptr),
    fPower1(nullptr),
    fPower2(nullptr),
    fSwitchedPower(nullptr),
    fSummedSignalGenerator(nullptr),
    fSimpleADC(nullptr)
{
    this->fAllocator = nullptr;
    fSamplePeriod = 1.0/(fAcquisitionRateMHz*1e6);
    //TODO make the clipping/range thresholds user configurable
    // fSimpleADC = new HSimpleAnalogToDigitalConverter<double, uint16_t, 14>(-10.0,10.0);

    fSimpleADC = new HSimpleAnalogToDigitalConverter<double, uint16_t, 14>(-16384./2., 16384/2.);
    //fSimpleADC = new HSimpleAnalogToDigitalConverter<double, uint16_t, 14>(0,RAND_MAX);
}

HPX14DigitizerSimulator::~HPX14DigitizerSimulator()
{
    if(fInitialized)
    {
        delete fSummedSignalGenerator;
        delete fSwitchedPower;
        delete fPower2;
        delete fPower1;
        delete fSimpleADC;
        TearDownImpl();
    }
}

bool
HPX14DigitizerSimulator::InitializeImpl()
{
    if(!fInitialized)
    {
        fCounter = 0;

        //build our allocator
        if(this->fAllocator){delete this->fAllocator;};
        this->fAllocator = new HBufferAllocatorNew< uint16_t >();

        //now we allocate the internal buffer pool (we use 32 X 2 MB buffers)
        fInternalBufferPool = new HBufferPool< uint16_t >( this->GetAllocator() );
        fInternalBufferPool->Allocate(fNInternalBuffers, fInternalBufferSize); //size and number not currently configurable]
        fInternalBufferPool->Initialize();

        //generate gaussian white noise
        fPower1 = new HGaussianWhiteNoiseSignal();
        fPower1->SetRandomSeed(123);
        fPower1->SetSamplingFrequency(fAcquisitionRateMHz*1e6);
        // fPower1->SetTimePeriod(0.05);
        fPower1->Initialize();

        fPower2 = new HGaussianWhiteNoiseSignal();
        fPower2->SetRandomSeed(456);
        fPower2->SetSamplingFrequency(fAcquisitionRateMHz*1e6);
        // fPower2->SetTimePeriod(0.05);
        fPower2->Initialize();

        //80Hz noise diode signal
        HSwitchedSignal* fSwitchedPower = new HSwitchedSignal();
        fSwitchedPower->SetSwitchingFrequency(NOISE_DIODE_SWITCHING_FREQ);
        fSwitchedPower->SetSignalGenerator(fPower2);
        fSwitchedPower->Initialize();

        fSummedSignalGenerator = new HSummedSignal();
        fSummedSignalGenerator->AddSignalGenerator(fPower1, 1.0);
        fSummedSignalGenerator->AddSignalGenerator(fSwitchedPower, 0.0);
        fSummedSignalGenerator->Initialize();

        fErrorCode = 0;
        fInitialized = true;
        fArmed = false;
    }
    return true;

}

void
HPX14DigitizerSimulator::AcquireImpl()
{
    fArmed = false;
    fStopAfterNextBuffer = false;
    fCounter = 0;
    //time handling is quick and dirty, need to improve this (i.e if we are close to a second-roll over, etc)
    //Note: that the acquisition starts on the next second tick (with the trigger)
    //POSIX expectation is seconds since unix epoch (1970, but this is not guaranteed)
    fAcquisitionStartTime = std::time(nullptr);
    fArmed = true;
}

void
HPX14DigitizerSimulator::TransferImpl()
{
    if(fArmed && this->fBuffer != nullptr )
    {
        //configure buffer information, cast time to uint64_t and set, then set the sample rate
        this->fBuffer->GetMetaData()->SetSidebandFlag(fSidebandFlag);
        this->fBuffer->GetMetaData()->SetPolarizationFlag(fPolarizationFlag);
        this->fBuffer->GetMetaData()->SetAcquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
        this->fBuffer->GetMetaData()->SetSampleRate(GetSamplingFrequency()); //check that double to uint64_t conversion is OK here
        uint64_t count = fCounter;
        this->fBuffer->GetMetaData()->SetLeadingSampleIndex(count);

        unsigned int n_samples_collect = this->fBuffer->GetArrayDimension(0);
        int64_t samples_to_collect = this->fBuffer->GetArrayDimension(0);

        while(samples_to_collect > 0)
        {
            unsigned int samples_in_buffer = std::min( (unsigned int) samples_to_collect, fInternalBufferSize);

            //grab a buffer from the internal pool
            HLinearBuffer< uint16_t >* internal_buff = nullptr;
            HProducerBufferPolicyCode internal_code = fInternalProducerBufferHandler.ReserveBuffer(fInternalBufferPool, internal_buff);

            if(internal_code & HProducerBufferPolicyCode::success && internal_buff != nullptr)
            {

                SimulateDataTransfer(count, samples_in_buffer, internal_buff->GetData());

                internal_buff->GetMetaData()->SetValidLength(samples_in_buffer);
                internal_buff->GetMetaData()->SetLeadingSampleIndex(n_samples_collect-samples_to_collect);

                fInternalProducerBufferHandler.ReleaseBufferToConsumer(fInternalBufferPool, internal_buff);
                internal_buff = nullptr;

                //update samples to collect
                samples_to_collect -= samples_in_buffer;
            }
        }
    }
}

HDigitizerErrorCode
HPX14DigitizerSimulator::FinalizeImpl()
{
    if(fArmed && this->fBuffer != nullptr)
    {
        //wait until all DMA xfer threads are idle
        bool threads_busy = true;
        while( (fInternalBufferPool->GetConsumerPoolSize() != 0 && !fForceTerminate && !fSignalTerminate) || threads_busy )
        {
            if( AllThreadsAreIdle() ){threads_busy = false;}
            else{ threads_busy = true; }
        }
        //increment the sample counter
        fCounter += this->fBuffer->GetArrayDimension(0);
    }

    return HDigitizerErrorCode::success;


}

void HPX14DigitizerSimulator::StopImpl()
{
    fArmed = false;
    fErrorCode = 0;
    fCounter = 0;
}

void
HPX14DigitizerSimulator::TearDownImpl()
{
    if(this->fAllocator)
    {
        delete this->fAllocator;
        this->fAllocator = nullptr;
    }

    fArmed = false;
    fInitialized = false;
}

//required by the producer interface
void
HPX14DigitizerSimulator::ExecutePreProductionTasks()
{
    Initialize();
}

void
HPX14DigitizerSimulator::ExecutePostProductionTasks()
{
    this->Stop();
    this->TearDown();
}

void
HPX14DigitizerSimulator::ExecutePreWorkTasks()
{
    if(fArmed)
    {
        //get a buffer from the buffer handler
        HLinearBuffer< uint16_t >* buffer = nullptr;
        fBufferCode = this->fBufferHandler.ReserveBuffer(this->fBufferPool, buffer);

        // if(fBufferCode == HProducerBufferPolicyCode::stolen)
        // {
        //     std::cout<<"sink code = "<<(unsigned int)fBufferCode<<std::endl;
        //     std::cout<<"digitizer stealing buffer"<<std::endl;
        //     size_t n_conpools = fBufferPool->GetNumberOfConsumerPools();
        //     for(size_t n=0; n<n_conpools; n++)
        //     {
        //         std::cout<<"consumer: "<<n<<" (to spec) pool size = "<<fBufferPool->GetConsumerPoolSize(n)<<std::endl;
        //     }
        //     std::cout<<"producer (for digi) pool size = "<<fBufferPool->GetProducerPoolSize()<<std::endl;
        // }

        //set the digitizer buffer if succesful
        if( buffer != nullptr && (fBufferCode & HProducerBufferPolicyCode::success))
        {
            //successfully got a buffer, assigned it
            this->SetBuffer(buffer);
        }
    }
}

void
HPX14DigitizerSimulator::DoWork()
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
HPX14DigitizerSimulator::ExecutePostWorkTasks()
{
    if( (fBufferCode & HProducerBufferPolicyCode::success) && fArmed)
    {
        HDigitizerErrorCode finalize_code = this->Finalize();
        if(finalize_code == HDigitizerErrorCode::success)
        {
            //std::cout<<"sim digi releasing a good buffer"<<std::endl;
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
HPX14DigitizerSimulator::ExecuteThreadTask()
{
    if(fArmed && fBuffer != nullptr)
    {
        //grab a buffer from the internal buffer pool
        if(fInternalBufferPool->GetConsumerPoolSize() != 0)
        {
            //grab a buffer from the internal pool
            HLinearBuffer< uint16_t >* internal_buff = nullptr;
            HConsumerBufferPolicyCode internal_code = fInternalConsumerBufferHandler.ReserveBuffer(fInternalBufferPool, internal_buff);

            if(internal_code & HConsumerBufferPolicyCode::success && internal_buff != nullptr)
            {
                //copy the internal buffer to the appropriate section of the external buffer
                void* src = internal_buff->GetData();
                void* dest = &( (this->fBuffer->GetData())[internal_buff->GetMetaData()->GetLeadingSampleIndex()] );
                size_t sz = internal_buff->GetMetaData()->GetValidLength();

                if( dest != nullptr &&  src != nullptr && sz != 0)
                {
                    //do the memcpy
                    memcpy(dest, src, sz*sizeof(uint16_t));
                }
                fInternalConsumerBufferHandler.ReleaseBufferToProducer(fInternalBufferPool, internal_buff);
                internal_buff = nullptr;
            }
            if(internal_buff != nullptr){fInternalConsumerBufferHandler.ReleaseBufferToProducer(fInternalBufferPool, internal_buff);};
        }
    }
}

bool
HPX14DigitizerSimulator::WorkPresent()
{
    if( fInternalBufferPool->GetConsumerPoolSize() == 0)
    {
        return false;
    }
    return true;
}

void
HPX14DigitizerSimulator::SimulateDataTransfer(uint64_t global_count, size_t n_samples, uint16_t* buffer)
{
    //copy fake data into the buffer
    //should also wait an appropriate amount of time based on the sample rate

    double time = global_count*fSamplePeriod;
    double sample = 0;
    bool retval = false;;
    for(size_t i=0; i<n_samples; i++)
    {
        time += fSamplePeriod;
        retval = fSummedSignalGenerator->GetSample(time, sample); (void) retval;
        buffer[i] = fSimpleADC->Convert(sample);
    }
}



} //end of namespace
