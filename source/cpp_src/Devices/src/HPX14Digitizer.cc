#include "HPX14Digitizer.hh"

//#define USE_SOFTWARE_TRIGGER 1
#define PX14_N_INTERNAL_BUFF 32
#define PX14_INTERNAL_BUFF_SIZE 1048576

namespace hose
{

HPX14Digitizer::HPX14Digitizer():
    fSidebandFlag('?'),
    fPolarizationFlag('?'),
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
    fNInternalBuffers(PX14_N_INTERNAL_BUFF),
    fInternalBufferSize(PX14_INTERNAL_BUFF_SIZE),
    fInternalBufferPool(nullptr)
{
    this->fAllocator = nullptr;
    // fInternalProducerBufferHandler.SetNAttempts(100);
    // fInternalProducerBufferHandler.SetSleepDurationNanoSeconds(0);
    // fInternalConsumerBufferHandler.SetNAttempts(100);
    // fInternalConsumerBufferHandler.SetSleepDurationNanoSeconds(0);
}

HPX14Digitizer::~HPX14Digitizer()
{
    if(fInitialized)
    {
        TearDownImpl();
    }
}

bool
HPX14Digitizer::InitializeImpl()
{
    if(!fInitialized)
    {
        int code = SIG_SUCCESS;
        // std::cout<<"connecting"<<std::endl;

        code = ConnectToDevicePX14(&fBoard, fBoardNumber);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to connect to PX14400 device: ");
            //TODO BREAK
        }
        else
        {
            fConnected = true;
        }

        // std::cout<<"setting power up defaults"<<std::endl;
        code = SetPowerupDefaultsPX14(fBoard);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to set powerup defaults: ");
            //TODO BREAK
        }

        // std::cout<<"set active channels to one"<<std::endl;
        //currently we only do single channel, dual channel requires de-interleaving
        code = SetActiveChannelsPX14(fBoard, PX14CHANNEL_ONE);
        if(code != SIG_SUCCESS)
        {
             DumpLibErrorPX14(code, "Failed to set active channel: ");
            //TODO BREAK
        }

        // std::cout<<"setting clock to external ref"<<std::endl;
        code = SetInternalAdcClockReferencePX14(fBoard, PX14CLKREF_EXT);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to set external 10 MHz reference: ", fBoard);

            // std::cout<<"external clock use failed, setting clock to internal ref"<<std::endl;
            code = SetInternalAdcClockReferencePX14(fBoard, PX14CLKREF_INT_10MHZ );
            if(code != SIG_SUCCESS)
            {
                DumpLibErrorPX14(code, "Failed to set INTERNAL 10 MHz reference: ", fBoard);
            }
            //TODO BREAK
        }

        // std::cout<<"setting internal adc clock rate"<<std::endl;
        // code = SetInternalAdcClockRatePX14(fBoard,  fAcquisitionRateMHz);
        // if(code != SIG_SUCCESS)
        // {
        //      DumpLibErrorPX14(code, "Failed to set acquisition rate: ", fBoard);
        //     //TODO BREAK
        // }

        #ifdef USE_SOFTWARE_TRIGGER
            // std::cout<<"setting trigger source to internal"<<std::endl;
            code = SetTriggerSourcePX14(fBoard, PX14TRIGSRC_INT_CH1);
            if(code != SIG_SUCCESS)
            {
                DumpLibErrorPX14(code, "Failed to set internal triggering: ", fBoard);
                //TODO BREAK
            }
        #else
            // std::cout<<"setting trigger source to external"<<std::endl;
            code = SetTriggerSourcePX14(fBoard, PX14TRIGSRC_EXT);
            if(code != SIG_SUCCESS)
            {
                DumpLibErrorPX14(code, "Failed to set external triggering: ", fBoard);
                //TODO BREAK
            }
        #endif

        // std::cout<<"setting time stamp mode"<<std::endl;
        //set up time stamp mode  (get a timestamp for every event on external trigger)
        code = SetTimestampModePX14(fBoard, PX14TSMODE_TS_ON_EXT_TRIGGER);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to set external triggering: ", fBoard);
            //TODO BREAK
        }

        //enable the digitial I/O pin
        code = SetDigitalIoEnablePX14(fBoard, 1);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to enable DIO pin: ", fBoard);
            //TODO BREAK
        }

        //set mode to send a sync signal for the noise diode
        code = SetDigitalIoModePX14(fBoard, PX14DIGIO_OUT_SYNC_TRIG);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to set DIO pin mode to PX14DIGIO_OUT_SYNC_TRIG: ", fBoard);
            //TODO BREAK
        }


        fCounter = 0;

        //build our allocator
        if(code == SIG_SUCCESS && fConnected)
        {
            if(this->fAllocator){delete this->fAllocator;};
            this->fAllocator = new HPX14BufferAllocator(fBoard);

            //grab the effective sample rate
            code = GetEffectiveAcqRatePX14(fBoard, &fAcquisitionRateMHz);
            if(code != SIG_SUCCESS)
            {
                DumpLibErrorPX14(code, "Could not determine effective ADC sampling rate: ", fBoard);
                //TODO BREAK
            }
            else
            {
                std::cout<<"Effective ADC sampling rate (MHz) = "<<fAcquisitionRateMHz<<std::endl;
            }

            // //now we allocate the internal buffer pool (we use 32 X 2 MB buffers)

            fInternalBufferPool = new HBufferPool< px14_sample_t >( this->GetAllocator() );
            fInternalBufferPool->Allocate(fNInternalBuffers, fInternalBufferSize); //size and number not currently configurable
            fInternalBufferPool->Initialize();

            fErrorCode = 0;
            fInitialized = true;
            fArmed = false;
            return true;
        }
        return false;
    }
    else
    {
        return true;
    }

}

void
HPX14Digitizer::AcquireImpl()
{
    fArmed = false;
    fStopAfterNextBuffer = false;
    fCounter = 0;
    //time handling is quick and dirty, need to improve this (i.e if we are close to a second-roll over, etc)
    //Note: that the acquisition starts on the next second tick (with the trigger)
    //POSIX expectation is seconds since unix epoch (1970, but this is not guaranteed)
    fAcquisitionStartTime = std::time(nullptr);

    // std::cout<<"new acquire time = "<<fAcquisitionStartTime<<std::endl;

    int code = BeginBufferedPciAcquisitionPX14(fBoard);
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Failed to arm recording: ", fBoard);
        fAcquisitionStartTime = 0;
    }
    else
    {

    #ifdef USE_SOFTWARE_TRIGGER
        if(!fArmed)
        {
            int code = IssueSoftwareTriggerPX14(fBoard);
            if(code != SIG_SUCCESS)
            {
                DumpLibErrorPX14(code, "Failed to issue software trigger: ", fBoard);
                fAcquisitionStartTime = 0;
            }
            fArmed = true;
        }
    #else
        fArmed = true;
    #endif
    }
}

void
HPX14Digitizer::TransferImpl()
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

        unsigned int n_samples_collect  = this->fBuffer->GetArrayDimension(0);
        int64_t samples_to_collect = this->fBuffer->GetArrayDimension(0);

        while(samples_to_collect > 0)
        {
            unsigned int samples_in_buffer = std::min( (unsigned int) samples_to_collect, fInternalBufferSize);

            //grab a buffer from the internal pool
            HLinearBuffer< px14_sample_t >* internal_buff = nullptr;
            HProducerBufferPolicyCode internal_code = fInternalProducerBufferHandler.ReserveBuffer(fInternalBufferPool, internal_buff);

            if(internal_code & HProducerBufferPolicyCode::success && internal_buff != nullptr)
            {
                int code = GetPciAcquisitionDataFastPX14(fBoard, samples_in_buffer, internal_buff->GetData(), PX14_TRUE);
                if(code != SIG_SUCCESS)
                {
                    DumpLibErrorPX14 (code, "\nFailed to obtain PCI acquisition data: ", fBoard);
                    fErrorCode = 1;
                    samples_to_collect = 0;
                    break;
                }
                else
                {
                    //wait for xfer to complete
                    int code = WaitForTransferCompletePX14(fBoard);

                    if(code != SIG_SUCCESS)
                    {
                        DumpLibErrorPX14 (code, "\nWait for PCI acquisition data failed: ", fBoard);
                        fErrorCode = 1;
                        samples_to_collect = 0;
                        break;
                    }

                    internal_buff->GetMetaData()->SetValidLength(samples_in_buffer);
                    internal_buff->GetMetaData()->SetLeadingSampleIndex(n_samples_collect-samples_to_collect);

                    fInternalProducerBufferHandler.ReleaseBufferToConsumer(fInternalBufferPool, internal_buff);
                    internal_buff = nullptr;

                    //update samples to collect
                    samples_to_collect -= samples_in_buffer;
                }
                if(internal_buff != nullptr){fInternalProducerBufferHandler.ReleaseBufferToProducer(fInternalBufferPool, internal_buff);};
            }
        }
    }
}

HDigitizerErrorCode
HPX14Digitizer::FinalizeImpl()
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

        //check for FIFO overflow
        if( GetFifoFullFlagPX14(fBoard) )
        {
            std::cout<<"Card FIFO overflow."<<std::endl;
            return HDigitizerErrorCode::card_buffer_overflow;
        }

        if(!fErrorCode)
        {
            return HDigitizerErrorCode::success;
        }
        else
        {
            std::cout<<"Buffer overflow."<<std::endl;
            return HDigitizerErrorCode::card_buffer_overflow;
        }
    }
    else
    {
        return HDigitizerErrorCode::success;
    }

}

void HPX14Digitizer::StopImpl()
{
    //stop aquisition, put board in standby mode
    int code = SetOperatingModePX14(fBoard, PX14MODE_STANDBY);
    if( code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Board failed to enter standby mode: ", fBoard);
        std::exit(1);
    }
    else
    {
        fArmed = false;
        fErrorCode = 0;
        fCounter = 0;
    }

}

void
HPX14Digitizer::TearDownImpl()
{
    if(fArmed)
    {
        EndBufferedPciAcquisitionPX14(fBoard);
    }
    int code = SetOperatingModePX14(fBoard, PX14MODE_STANDBY);
    (void) code; //TODO check if this value is useful
    //deleting the allocator also deletes all of the buffers it allocated
    //this may require clean up elsewhere, as buffer pointers may still be around
    if(this->fAllocator)
    {
        delete this->fAllocator;
        this->fAllocator = nullptr;
    }
    if(fConnected){DisconnectFromDevicePX14(fBoard);};

    fInitialized = false;
}

//required by the producer interface
void
HPX14Digitizer::ExecutePreProductionTasks()
{
    Initialize();
}

void
HPX14Digitizer::ExecutePostProductionTasks()
{
    this->Stop();
    this->TearDown();
}

void
HPX14Digitizer::ExecutePreWorkTasks()
{
    if(fArmed)
    {
        //get a buffer from the buffer handler
        HLinearBuffer< px14_sample_t >* buffer = nullptr;
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
HPX14Digitizer::DoWork()
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
HPX14Digitizer::ExecutePostWorkTasks()
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
HPX14Digitizer::ExecuteThreadTask()
{
    if(fArmed && fBuffer != nullptr)
    {
        //grab a buffer from the internal buffer pool
        if(fInternalBufferPool->GetConsumerPoolSize() != 0)
        {
            //grab a buffer from the internal pool
            HLinearBuffer< px14_sample_t >* internal_buff = nullptr;
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
HPX14Digitizer::WorkPresent()
{
    if( fInternalBufferPool->GetConsumerPoolSize() == 0)
    {
        return false;
    }
    return true;
}



} //end of namespace
