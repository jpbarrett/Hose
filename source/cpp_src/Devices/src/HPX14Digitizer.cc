#include "HPX14Digitizer.hh"


namespace hose
{

HPX14Digitizer::HPX14Digitizer():
    fBoardNumber(1),
    fAcquisitionRateMHz(200),
    fConnected(false),
    fInitialized(false),
    fArmed(false)
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
    //currently we only do single channel, dual channel requicode de-interleaving
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
        //TODO BREAK
    }

    std::cout<<"setting internal adc clock rate"<<std::endl;
    code = SetInternalAdcClockRatePX14(fBoard,  fAcquisitionRateMHz);
    if(code != SIG_SUCCESS)
    {
         DumpLibErrorPX14(code, "Failed to set acquisition rate: ", fBoard);
        //TODO BREAK
    }

    std::cout<<"setting trigger source to external"<<std::endl;
    code = SetTriggerSourcePX14(fBoard, PX14TRIGSRC_EXT);
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Failed to set external triggering: ", fBoard);
        //TODO BREAK
    }

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
        std::cout<<"success"<<std::endl;
        if(this->fAllocator){delete this->fAllocator;};
        this->fAllocator = new HPX14BufferAllocator(fBoard);
        fInitialized = true;
        fArmed = false;
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
    //POSIX expectation is seconds since unix epoch (1970, but this is no guaranteed)
    std::time_t result = std::time(nullptr);
    //cast to a uint64_t and set
    this->fBuffer->GetMetaData()->SetAquisitionStartSecond( (uint64_t) result );
    //set the sample rate
    this->fBuffer->GetMetaData()->SetSampleRate(fAcquisitionRateMHz*1000000);

    //std::cout<<"acquire"<<std::endl;
    int code = BeginBufferedPciAcquisitionPX14(fBoard);
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14(code, "Failed to arm recording: ", fBoard);
        //TODO BREAK
    }

    //get/code time of current recording epoch

    fArmed = true;
}

void 
HPX14Digitizer::TransferImpl()
{
    //std::cout<<"transferring"<<std::endl;
    int code = GetPciAcquisitionDataFastPX14(fBoard, this->fBuffer->GetArrayDimension(0), this->fBuffer->GetData(), PX14_TRUE);
    //std::cout<<"code = "<<code<<std::endl;
    if(code != SIG_SUCCESS)
    {
        DumpLibErrorPX14 (code, "\nFailed to obtain PCI acquisition data: ", fBoard);
    }
}

int 
HPX14Digitizer::FinalizeImpl()
{
    //std::cout<<"finalizing"<<std::endl;
    //wait for xfer to complete
    int code = WaitForTransferCompletePX14(fBoard);

    //increment the sample counter
    this->fBuffer->GetMetaData()->SetLeadingSampleIndex(fCounter);
    fCounter += this->fBuffer->GetArrayDimension(0);

    if( GetFifoFullFlagPX14(fBoard) )
    {
        std::cout<<"board FIFO buffer full"<<std::endl;
        return 1;
    }
    return 0;
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

}