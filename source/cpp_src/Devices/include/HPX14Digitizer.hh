#ifndef HPX14Digitizer_HH__
#define HPX14Digitizer_HH__

#include <cstddef>
#include <stdint.h>

extern "C"
{
    #include "px14.h"
}

#include <mutex>

#include "HDigitizer.hh"
#include "HProducer.hh"
#include "HPX14BufferAllocator.hh"

namespace hose {

/*
*File: HPX14Digitizer.hh
*Class: HPX14Digitizer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Partially ported from original gpu-spec code by Juha Vierinen/Russ McWirther
*/


class HPX14Digitizer: public HDigitizer< px14_sample_t, HPX14Digitizer >,  public HProducer< px14_sample_t, HProducerBufferHandler_Steal< px14_sample_t > >
{
    public:
        HPX14Digitizer();
        virtual ~HPX14Digitizer();

        //methods to configure board
        void SetBoardNumber(unsigned int n){fBoardNumber = n;};
        unsigned int GetBoardNumber() const {return fBoardNumber;};

        HPX14 GetBoard() {return fBoard;};
        bool IsInitialized() const {return fInitialized;};
        bool IsConnected() const {return fConnected;};
        bool IsArmed() const {return fArmed;};

    protected:

        friend class HDigitizer<px14_sample_t, HPX14Digitizer >;

        HPX14 fBoard;
        unsigned int fBoardNumber; //board id
        double fAcquisitionRateMHz; //effective sampling frequency in MHz
        bool fConnected;
        bool fInitialized;
        bool fArmed;
        bool fBufferLocked;

        //required by digitizer interface
        bool InitializeImpl();
        void AcquireImpl();
        void TransferImpl();
        HDigitizerErrorCode FinalizeImpl();
        void StopImpl();
        void TearDownImpl();

        //required by the producer interface
        virtual void ExecutePreProductionTasks() override; //'initialize' the digitizer
        virtual void ExecutePostProductionTasks() override; //teardown the digitizer
        virtual void ExecutePreWorkTasks() override; //grab a buffer start acquire if we haven't already
        virtual void GenerateWork() override; //execute a transfer into buffer
        virtual void ExecutePostWorkTasks() override; //finalize the transfer, release the buffer

        //needed by the thread pool interface
        virtual void ExecuteThreadTask() override; //do thread work assoicated with fill the buffer
        virtual bool WorkPresent() override; //check if we have buffer filling work to do

        //global sample counter
        uint64_t fCounter;
        bool fAcquireActive;
        HProducerBufferPolicyCode fBufferCode;
        std::time_t fAcquisitionStartTime;


};

}

#endif /* end of include guard: HPX14Digitizer */
