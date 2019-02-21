#ifndef HATS9371Digitizer_HH__
#define HATS9371Digitizer_HH__

#include <cstddef>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

#include <mutex>

#include "HDigitizer.hh"
#include "HProducer.hh"
#include "HConsumerBufferHandlerPolicy.hh"

#include "HATS9371BufferAllocator.hh"

namespace hose {

/*
*File: HATS9371Digitizer.hh
*Class: HATS9371Digitizer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*/


class HATS9371Digitizer: public HDigitizer< ats_sample_t, HATS9371Digitizer >,  public HProducer< ats_sample_t, HProducerBufferHandler_Immediate< ats_sample_t > >
{
    public:
        HATS9371Digitizer();
        virtual ~HATS9371Digitizer();

        //methods to configure board
        void SetBoardNumber(unsigned int n){fBoardNumber = n;};
        unsigned int GetBoardNumber() const {return fBoardNumber;};

        void SetSidebandFlag(const char& sflag){fSidebandFlag = sflag;};
        void SetPolarizationFlag(const char& pflag){fPolarizationFlag = pflag;};

        double GetSamplingFrequency() const {return fAcquisitionRateMHz*1e6;};

        HANDLE GetBoard() {return fBoard;};
        bool IsInitialized() const {return fInitialized;};
        bool IsConnected() const {return fConnected;};
        bool IsArmed() const {return fArmed;};

        void StopAfterNextBuffer(){fStopAfterNextBuffer = true;}

    protected:

        friend class HDigitizer<ats_sample_t, HATS9371Digitizer >;

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
        virtual void DoWork() override; //execute a transfer into buffer
        virtual void ExecutePostWorkTasks() override; //finalize the transfer, release the buffer

        //needed by the thread pool interface
        virtual void ExecuteThreadTask() override; //do thread work assoicated with fill the buffer
        virtual bool WorkPresent() override; //check if we have buffer filling work to do

        char fSidebandFlag;
        char fPolarizationFlag;

        HANDLE fBoard;
        unsigned int fSystemNumber; //system id
        unsigned int fBoardNumber; //board id
        double fAcquisitionRateMHz; //effective sampling frequency in MHz
        bool fConnected;
        bool fInitialized;
        volatile bool fArmed;
        bool fBufferLocked;
        volatile bool fStopAfterNextBuffer;

        //global sample counter
        volatile uint64_t fCounter;
        bool fAcquireActive;
        HProducerBufferPolicyCode fBufferCode;
        volatile std::time_t fAcquisitionStartTime;

        //thread pool stuff for read-out,

        mutable std::mutex fQueueMutex;
        std::queue< std::tuple<void*, void*, size_t> > fMemcpyArgQueue;
        //internal error code, cleared on stop/acquire, indicates board buffer overflow
        int fErrorCode;

        //internal DMA buffer pool handling
        unsigned int fNInternalBuffers;
        unsigned int fInternalBufferSize;
        HBufferPool< ats_sample_t >* fInternalBufferPool; //buffer pool of px14 allocated buffers (limited to 2MB max size)
        HProducerBufferHandler_Immediate< ats_sample_t > fInternalProducerBufferHandler;
        HConsumerBufferHandler_Immediate< ats_sample_t > fInternalConsumerBufferHandler;


};

}

#endif /* end of include guard: HATS9371Digitizer */
