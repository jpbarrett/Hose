#include "HPX14Digitizer.hh"

//#define USE_SOFTWARE_TRIGGER 1

namespace hose
{

HPX14Digitizer::HPX14Digitizer():
    fBoardNumber(1),
    fAcquisitionRateMHz(0),
    fConnected(false),
    fInitialized(false),
    fArmed(false),
    fCounter(0),
    fAcquireActive(false),
    fBufferCode(HProducerBufferPolicyCode::unset)
{
    this->fAllocator = nullptr;
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
    int code = SIG_SUCCESS;
    std::cout<<"connecting"<<std::endl;

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

    std::cout<<"setting power up defaults"<<std::endl;
    code = SetPowerupDefaultsPX14(fBoard);
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Failed to set powerup defaults: ");
        //TODO BREAK
    }

    std::cout<<"set active channels to one"<<std::endl;
    //currently we only do single channel, dual channel requires de-interleaving
    code = SetActiveChannelsPX14(fBoard, PX14CHANNEL_ONE);
    if(code != SIG_SUCCESS)
    {
         DumpLibErrorPX14(code, "Failed to set active channel: ");
        //TODO BREAK
    }

    std::cout<<"setting clock to external ref"<<std::endl;
    code = SetInternalAdcClockReferencePX14(fBoard, PX14CLKREF_EXT);
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Failed to set external 10 MHz reference: ", fBoard);

        std::cout<<"external clock use failed, setting clock to internal ref"<<std::endl;
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
        std::cout<<"setting trigger source to internal"<<std::endl;
        code = SetTriggerSourcePX14(fBoard, PX14TRIGSRC_INT_CH1);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to set internal triggering: ", fBoard);
            //TODO BREAK
        }
    #else
        std::cout<<"setting trigger source to external"<<std::endl;
        code = SetTriggerSourcePX14(fBoard, PX14TRIGSRC_EXT);
        if(code != SIG_SUCCESS)
        {
            DumpLibErrorPX14(code, "Failed to set external triggering: ", fBoard);
            //TODO BREAK
        }
    #endif

    std::cout<<"setting time stamp mode"<<std::endl;
    //set up time stamp mode  (get a timestamp for every event on external trigger)
    code = SetTimestampModePX14(fBoard, PX14TSMODE_TS_ON_EXT_TRIGGER);
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Failed to set external triggering: ", fBoard);
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
            std::cout<<"effective ADC sampling rate (MHz) = "<<fAcquisitionRateMHz<<std::endl;
        }

        fInitialized = true;
        fArmed = false;
        std::cout<<"init success"<<std::endl;
        return true;
    }
    return false;
}

void 
HPX14Digitizer::AcquireImpl()
{
    fCounter = 0;
    //time handling is quick and dirty, need to improve this (i.e if we are close to a second-roll over, etc)
    //Note: that the acquisition starts on the next second tick (with the trigger)
    //POSIX expectation is seconds since unix epoch (1970, but this is not guaranteed)
    fAcquisitionStartTime = std::time(nullptr);

    int code = BeginBufferedPciAcquisitionPX14(fBoard);
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Failed to arm recording: ", fBoard);
        fAcquisitionStartTime = 0;
    }

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

void 
HPX14Digitizer::TransferImpl()
{
    // int code = GetPciAcquisitionDataFastPX14(fBoard, this->fBuffer->GetArrayDimension(0), this->fBuffer->GetData(), PX14_TRUE);

    // Obtain fresh PCI acquisition data given normal, non-DMA buffer
    int code = GetPciAcquisitionDataBufPX14(fBoard, this->fBuffer->GetArrayDimension(0), this->fBuffer->GetData(), PX14_TRUE );
    //std::cout<<"code = "<<code<<std::endl;
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14 (code, "\nFailed to obtain PCI acquisition data: ", fBoard);
    }
}

HDigitizerErrorCode 
HPX14Digitizer::FinalizeImpl()
{
    //wait for xfer to complete
    int code = WaitForTransferCompletePX14(fBoard);

    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);

    if( GetFifoFullFlagPX14(fBoard) )
    {
        std::cout<<"board FIFO buffer full"<<std::endl;
        return HDigitizerErrorCode::card_buffer_overflow;
    }
    return HDigitizerErrorCode::success;
}

void HPX14Digitizer::StopImpl()
{
    //std::cout<<"stopping"<<std::endl;
    //stop aquisition, put board in standby mode
    int code = SetOperatingModePX14(fBoard, PX14MODE_STANDBY);
    if( code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Board failed to enter standby mode: ", fBoard);
    }
    else
    {
        fArmed = false;
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
    //deleting the allocator also deletes all of the buffers it allocated
    //this may require clean up elsewhere, as buffer pointers may still be around
    if(this->fAllocator){delete this->fAllocator;}
    if(fConnected){DisconnectFromDevicePX14(fBoard);};

    fInitialized = false;
}

//required by the producer interface
void 
HPX14Digitizer::ExecutePreProductionTasks()
{
    //does nothing for now
}

void 
HPX14Digitizer::ExecutePostProductionTasks()
{
    this->Stop();
    this->TearDown();
    fAcquireActive = false;
}

void 
HPX14Digitizer::ExecutePreWorkTasks()
{
    //get a buffer from the buffer handler
    HLinearBuffer< px14_sample_t >* buffer = nullptr;
    fBufferCode = this->fBufferHandler.ReserveBuffer(this->fBufferPool, buffer);
    
    //set the digitizer buffer if succesful
    if( buffer != nullptr )
    {
        //successfully got a buffer, assigned it
        this->SetBuffer(buffer);

        //start aquire if we haven't already
        if( !fAcquireActive )
        {
            this->Acquire();
            if(fArmed)
            {
                fAcquireActive = true;
            }
        }

        //configure the buffer meta data
        this->fBuffer->GetMetaData()->SetAcquisitionStartSecond( (uint64_t) fAcquisitionStartTime );
        this->fBuffer->GetMetaData()->SetSampleRate(fAcquisitionRateMHz*1000000);
    }
    else
    {
        //buffer acquisition error, stop if needed
        if(fAcquireActive)
        {
            this->Stop();
            fAcquireActive = false;
        }
    }

}

void 
HPX14Digitizer::DoWork()
{
    //we have an active buffer, transfer the data
    if(fBufferCode & HProducerBufferPolicyCode::success)
    {
        this->Transfer();
    }
}

void 
HPX14Digitizer::ExecutePostWorkTasks()
{
    if(fBufferCode & HProducerBufferPolicyCode::success)
    {   
        HDigitizerErrorCode finalize_code = this->Finalize(); 
        if(finalize_code == HDigitizerErrorCode::success)
        {
            fBufferCode = this->fBufferHandler.ReleaseBufferToConsumer(this->fBufferPool, this->fBuffer);
        }
        else
        {
            //some error occurred, stop production so we can re-start
            fBufferCode = this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, this->fBuffer);
            this->Stop();
            fAcquireActive = false;
        }
    }
}

//needed by the thread pool interface
void 
HPX14Digitizer::ExecuteThreadTask()
{
    //do nothing
}

bool 
HPX14Digitizer::WorkPresent()
{
    return false; //we do not need the thread pool model
}



} //end of namespace
