#include "HATS9371Digitizer.hh"

//#define USE_SOFTWARE_TRIGGER 1
#define ATS9371_N_INTERNAL_BUFF 4
#define ATS9371_INTERNAL_BUFF_SIZE 204800

namespace hose
{

HATS9371Digitizer::HATS9371Digitizer():
    fSidebandFlag('?'),
    fPolarizationFlag('?'),
    fSystemNumber(1),
    fBoardNumber(1),
    fAcquisitionRateMHz(0),
    fConnected(false),
    fInitialized(false),
    fArmed(false),
    fStopAfterNextBuffer(false),
    fCounter(0),
    fAcquireActive(false),
    fBufferCode(HProducerBufferPolicyCode::unset),
    fErrorCode(0),
    fNInternalBuffers(ATS9371_N_INTERNAL_BUFF),
    fInternalBufferSize(ATS9371_INTERNAL_BUFF_SIZE),
    fInternalBufferPool(nullptr)
{
    this->fAllocator = nullptr;
    // fInternalProducerBufferHandler.SetNAttempts(100);
    // fInternalProducerBufferHandler.SetSleepDurationNanoSeconds(0);
    // fInternalConsumerBufferHandler.SetNAttempts(100);
    // fInternalConsumerBufferHandler.SetSleepDurationNanoSeconds(0);
}

HATS9371Digitizer::~HATS9371Digitizer()
{
    if(fInitialized)
    {
        TearDownImpl();
    }
}

bool
HATS9371Digitizer::InitializeImpl()
{
    if(!fInitialized)
    {
        // Get a handle to the board
        fBoard = AlazarGetBoardBySystemID(fSystemNumber,fBoardNumber);
        RETURN_CODE retCode; //return code for errors

        // TODO: Specify the sample rate (see sample rate id below)

        double samplesPerSec = 1000000000.0;
        fAcquisitionRateMHz = samplesPerSec/1e6;

        // TODO: Select clock parameters as required to generate this sample rate.
        //
        // For example: if samplesPerSec is 100.e6 (100 MS/s), then:
        // - select clock source INTERNAL_CLOCK and sample rate SAMPLE_RATE_100MSPS
        // - select clock source FAST_EXTERNAL_CLOCK, sample rate SAMPLE_RATE_USER_DEF, and connect a
        //   100 MHz signal to the EXT CLK BNC connector.

        //this configures the card for a 1GHz sample rate using the internal clock (OK for now)
        retCode = AlazarSetCaptureClock(fBoard, INTERNAL_CLOCK, SAMPLE_RATE_1000MSPS, CLOCK_EDGE_RISING,0);
        if (retCode != ApiSuccess)
        {
            std::cout << std::string("Error: AlazarSetCaptureClock failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
            return false;
        }


        // TODO: Select channel A input parameters as required

        retCode = AlazarInputControlEx(fBoard, CHANNEL_A, DC_COUPLING, INPUT_RANGE_PM_400_MV, IMPEDANCE_50_OHM);
        if (retCode != ApiSuccess)
        {
            std::cout << std::string("Error: AlazarInputControlEx failed --") + std::string( AlazarErrorToText(retCode) ) << std::endl;
            return false;
        }

        // TODO: Select trigger inputs and levels as required
        retCode = AlazarSetTriggerOperation(fBoard,
                                            TRIG_ENGINE_OP_J,
                                            TRIG_ENGINE_J,
                                            TRIG_CHAN_A,
                                            TRIGGER_SLOPE_POSITIVE,
                                            150,
                                            TRIG_ENGINE_K,
                                            TRIG_DISABLE,
                                            TRIGGER_SLOPE_POSITIVE,
                                            128);
        if (retCode != ApiSuccess)
        {
            std::cout << std::string("Error: AlazarSetTriggerOperation failed --") + std::string( AlazarErrorToText(retCode) ) << std::endl;
            return false;
        }


        retCode = AlazarSetExternalTrigger(fBoard,DC_COUPLING, ETR_TTL);

        // TODO: Set trigger delay as required.

        double triggerDelay_sec = 0;
        U32 triggerDelay_samples = (U32)(triggerDelay_sec * samplesPerSec + 0.5);
        retCode = AlazarSetTriggerDelay(fBoard, triggerDelay_samples);
        if (retCode != ApiSuccess)
        {
            std::cout << std::string("Error: AlazarSetTriggerDelay failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
            return false;
        }

        // TODO: Set trigger timeout as required.

        // NOTE:
        // The board will wait for a for this amount of time for a trigger event.  If a trigger event
        // does not arrive, then
        // the board will automatically trigger. Set the trigger timeout value to 0 to force the board
        // to wait forever for a
        // trigger event.
        //
        // IMPORTANT:
        // The trigger timeout value should be set to zero after appropriate trigger parameters have
        // been determined,
        // otherwise the board may trigger if the timeout interval expires before a hardware trigger
        // event arrives.

        double triggerTimeout_sec = 0.001;
        U32 triggerTimeout_clocks = (U32)(triggerTimeout_sec / 10.e-6 + 0.5);
        retCode = AlazarSetTriggerTimeOut(fBoard, triggerTimeout_clocks);
        if (retCode != ApiSuccess)
        {
            std::cout << std::string("Error: AlazarSetTriggerTimeOut failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
            return false;
        }

        // TODO: Configure AUX I/O connector as required
        retCode = AlazarConfigureAuxIO(fBoard, AUX_OUT_TRIGGER, 0);        // The trigger timeout value should be set to zero after appropriate trigger parameters have
        // been determined,
        // otherwise the board may trigger if the timeout interval expires before a hardware trigger
        // event arrives.;
        if (retCode != ApiSuccess)
        {
            std::cout << std::string("Error: AlazarConfigureAuxIO failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
            return false;
        }

        fErrorCode = retCode;
        //build our allocator
        if(retCode == ApiSuccess)
        {

            if(this->fAllocator){delete this->fAllocator;};
            this->fAllocator = new HATS9371BufferAllocator(fBoard);

            //now we allocate the internal buffer pool (we use 32 X 2 MB buffers)
            fInternalBufferPool = new HBufferPool< ats_sample_t >( this->GetAllocator() );
            fInternalBufferPool->Allocate(fNInternalBuffers, fInternalBufferSize); //size and number not currently configurable
            fInternalBufferPool->Initialize();

            std::cout<<"sucessfully initialized"<<std::endl;
            fInitialized = true;
            fArmed = false;
            return true;
        }
        else
        {
            return false;
        }

    }
    else
    {
        return true;
    }

}

void
HATS9371Digitizer::AcquireImpl()
{
    fArmed = false;
    fStopAfterNextBuffer = false;
    fCounter = 0;
    RETURN_CODE retCode; //return code for errors
    bool success = false;

    //time handling is quick and dirty, need to improve this (i.e if we are close to a second-roll over, etc)
    //Note: that the acquisition starts on the next second tick (with the trigger)
    //POSIX expectation is seconds since unix epoch (1970, but this is not guaranteed)
    fAcquisitionStartTime = std::time(nullptr);

    U32 admaFlags = ADMA_EXTERNAL_STARTCAPTURE | ADMA_TRIGGERED_STREAMING | ADMA_FIFO_ONLY_STREAMING;
    retCode = AlazarBeforeAsyncRead(fBoard, CHANNEL_A,
                                    0, // Must be 0
                                    fInternalBufferSize,
                                    1,          // Must be 1
                                    0x7FFFFFFF, // Ignored. Behave as if infinite
                                    admaFlags);
    if (retCode != ApiSuccess)
    {
        std::cout << std::string("Error: AlazarBeforeAsyncRead failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
        success = false;
    }
    else
    {
        success = true;
        // Add the buffers to a list of buffers available to be filled by the board
        HLinearBuffer< ats_sample_t >* buffer = nullptr;
        unsigned int count = 0;
        do
        {
            buffer = fInternalBufferPool->PopProducerBuffer();
            if(buffer != nullptr)
            {
                retCode = AlazarPostAsyncBuffer(fBoard, buffer->GetData(), buffer->GetArraySize()*sizeof(ats_sample_t) );
                if (retCode != ApiSuccess)
                {
                    std::cout << std::string("Error: AlazarPostAsyncBuffer failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
                    success = false;
                }
                //push buffer to back of queue
                fInternalBufferPool->PushProducerBuffer(buffer);
            }
            count++;
        }
        while(buffer != nullptr && count < fNInternalBuffers);

        // Arm the board system to wait for a trigger event to begin the acquisition
        if(success)
        {
            retCode = AlazarStartCapture(fBoard);
            if (retCode != ApiSuccess)
            {
                std::cout << std::string("Error: AlazarStartCapture failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
                success = false;
            }
        }

        if(success)
        {
            std::cout<<"sucessfully armed for acquisition, capture started"<<std::endl;
            fArmed = true;
        }

    }

}

void
HATS9371Digitizer::TransferImpl()
{
    U32 timeout_ms = 5000;
    RETURN_CODE retCode; //return code for errors
    if(fArmed && this->fBuffer != nullptr )
    {
        //configure buffer information, cast time to uint64_t and set, then set the sample rate
        this->fBuffer->GetMetaData()->SetSidebandFlag(fSidebandFlag);
        this->fBuffer->GetMetaData()->SetPolarizationFlag(fPolarizationFlag);
        this->fBuffer->GetMetaData()->SetAcquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
        this->fBuffer->GetMetaData()->SetSampleRate(GetSamplingFrequency()); //check that double to uint64_t conversion is OK here
        uint64_t count = fCounter;
        this->fBuffer->GetMetaData()->SetLeadingSampleIndex(count);

        unsigned int n_samples_collect  = this->fBuffer->GetArrayDimension(0);
        int64_t samples_to_collect = this->fBuffer->GetArrayDimension(0);

        while(samples_to_collect > 0)
        {
            unsigned int samples_in_buffer = std::min( (unsigned int) samples_to_collect, fInternalBufferSize);

            //grab a buffer from the internal pool
            HLinearBuffer< ats_sample_t >* internal_buff = nullptr;
            HProducerBufferPolicyCode internal_code = fInternalProducerBufferHandler.ReserveBuffer(fInternalBufferPool, internal_buff);
            if(internal_code & HProducerBufferPolicyCode::success && internal_buff != nullptr)
            {
                ats_sample_t* pBuffer = internal_buff->GetData();
                retCode = AlazarWaitAsyncBufferComplete(fBoard, pBuffer, timeout_ms);
                if (retCode != ApiSuccess)
                {
                    std::cout<< std::string("Error: AlazarWaitAsyncBufferComplete failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
                    fErrorCode = 1;
                    samples_to_collect = 0;
                    fInternalProducerBufferHandler.ReleaseBufferToProducer(fInternalBufferPool, internal_buff);
                    break;
                }
                else
                {
                    //std::cout<<"transferred "<<samples_in_buffer<<" samples "<<std::endl;
                    internal_buff->GetMetaData()->SetValidLength(samples_in_buffer);
                    internal_buff->GetMetaData()->SetLeadingSampleIndex(n_samples_collect-samples_to_collect);
                    fInternalProducerBufferHandler.ReleaseBufferToConsumer(fInternalBufferPool, internal_buff);
                    //std::cout<<"size of consumer pool = "<<fInternalBufferPool->GetConsumerPoolSize()<<std::endl;
                    internal_buff = nullptr;
                    //update samples to collect
                    samples_to_collect -= samples_in_buffer;
                    //std::cout<<"samples to collect = "<<samples_to_collect<<std::endl;
                }
            }
        }
    }

}

HDigitizerErrorCode
HATS9371Digitizer::FinalizeImpl()
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
        return HDigitizerErrorCode::success;
    }
    else
    {
        return HDigitizerErrorCode::success;
    }

}

void HATS9371Digitizer::StopImpl()
{
    RETURN_CODE retCode;
    // Abort the acquisition
    if(fArmed)
    {
        retCode = AlazarAbortAsyncRead(fBoard);
        if (retCode != ApiSuccess)
        {
            std::cout<<std::string("Error: AlazarAbortAsyncRead failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;
        }
        fArmed = false;
        fErrorCode = 0;
        fCounter = 0;
    }
}

void
HATS9371Digitizer::TearDownImpl()
{
    //deleting the allocator also deletes all of the buffers it allocated
    //this may require clean up elsewhere, as buffer pointers may still be around
    if(this->fAllocator)
    {
        delete this->fAllocator;
        this->fAllocator = nullptr;
    }
    fInitialized = false;
}

//required by the producer interface
void
HATS9371Digitizer::ExecutePreProductionTasks()
{
    Initialize();
}

void
HATS9371Digitizer::ExecutePostProductionTasks()
{
    this->Stop();
    this->TearDown();
}

void
HATS9371Digitizer::ExecutePreWorkTasks()
{
    if(fArmed)
    {
        //get a buffer from the buffer handler
        HLinearBuffer< ats_sample_t >* buffer = nullptr;
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
HATS9371Digitizer::DoWork()
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
HATS9371Digitizer::ExecutePostWorkTasks()
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
        // else if (finalize_code == HDigitizerErrorCode::premature_stop)
        // {
        //     //early stop called, so put this buffer back on the producer stack and move on
        //     std::cout<<"discarding buffer"<<std::endl;
        //     fBufferCode = this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, this->fBuffer);
        //     this->fBuffer = nullptr;
        // }

    }

    if(fStopAfterNextBuffer)
    {
        this->Stop();
        fStopAfterNextBuffer = false;
    }

}

//needed by the thread pool interface
void
HATS9371Digitizer::ExecuteThreadTask()
{
    RETURN_CODE retCode; //return code for errors
    if(fArmed && this->fBuffer != nullptr && !fSignalTerminate && !fForceTerminate)
    {
        //grab a buffer from the internal buffer pool
        if(fInternalBufferPool->GetConsumerPoolSize() != 0)
        {
            //grab a buffer from the internal pool
            HLinearBuffer< ats_sample_t >* internal_buff = nullptr;
            HConsumerBufferPolicyCode internal_code = fInternalConsumerBufferHandler.ReserveBuffer(fInternalBufferPool, internal_buff);

            if(internal_code & HConsumerBufferPolicyCode::success && internal_buff != nullptr)
            {
                //std::cout<<"copying"<<std::endl;
                //copy the internal buffer to the appropriate section of the external buffer
                void* src = internal_buff->GetData();
                void* dest = &( (this->fBuffer->GetData())[internal_buff->GetMetaData()->GetLeadingSampleIndex()] );
                size_t sz = internal_buff->GetMetaData()->GetValidLength();

                if( dest != nullptr &&  src != nullptr && sz != 0)
                {
                    //do the memcpy
                    memcpy(dest, src, sz*sizeof(ats_sample_t) );
                }

                // Add the buffer to the end of the list of available buffers, and post it back to the digitizer
                retCode = AlazarPostAsyncBuffer(fBoard, internal_buff->GetData(), internal_buff->GetArraySize()*sizeof(ats_sample_t) );
                if (retCode != ApiSuccess)
                {
                    std::cout << std::string("Error: AlazarPostAsyncBuffer failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;;
                }
                fInternalConsumerBufferHandler.ReleaseBufferToProducer(fInternalBufferPool, internal_buff);
                internal_buff = nullptr;
            }
            //catch any errors, and return the buffer
            if(internal_buff != nullptr)
            {
                // Add the buffer to the end of the list of available buffers, and post it back to the digitizer
                retCode = AlazarPostAsyncBuffer(fBoard, internal_buff->GetData(), internal_buff->GetArraySize()*sizeof(ats_sample_t) );
                if (retCode != ApiSuccess)
                {
                    std::cout << std::string("Error: AlazarPostAsyncBuffer failed -- ") + std::string( AlazarErrorToText(retCode) ) << std::endl;;
                }
                fInternalConsumerBufferHandler.ReleaseBufferToProducer(fInternalBufferPool, internal_buff);
            };
        }
    }
}

bool
HATS9371Digitizer::WorkPresent()
{
    if( fInternalBufferPool->GetConsumerPoolSize() == 0)
    {
        return false;
    }
    return true;
}



} //end of namespace
