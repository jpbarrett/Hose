#ifndef HPX14DigitizerSimulator_HH__
#define HPX14DigitizerSimulator_HH__

#include <cstddef>
#include <stdint.h>

#include <mutex>

#include "HDigitizer.hh"
#include "HProducer.hh"
#include "HConsumerBufferHandlerPolicy.hh"
#include "HBufferAllocatorNew.hh"

#include "HSimulatedAnalogSignalSampleGenerator.hh"
#include "HPowerLawNoiseSignal.hh"
#include "HGaussianWhiteNoiseSignal.hh"
#include "HSwitchedSignal.hh"
#include "HSummedSignal.hh"
#include "HSimpleAnalogToDigitalConverter.hh"

namespace hose {

/*
*File: HPX14DigitizerSimulator.hh
*Class: HPX14DigitizerSimulator
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Partially ported from original gpu-spec code by Juha Vierinen/Russ McWirther
*/




class HPX14DigitizerSimulator: public HDigitizer< uint16_t, HPX14DigitizerSimulator >,  public HProducer< uint16_t, HProducerBufferHandler_Steal< uint16_t > >
{
    public:
        HPX14DigitizerSimulator();
        virtual ~HPX14DigitizerSimulator();

        //methods to configure board
        void SetBoardNumber(unsigned int n){fBoardNumber = n;};
        unsigned int GetBoardNumber() const {return fBoardNumber;};

        void SetSidebandFlag(const char& sflag){fSidebandFlag = sflag;};
        void SetPolarizationFlag(const char& pflag){fPolarizationFlag = pflag;};

        virtual double GetSamplingFrequency() const override {return fAcquisitionRateMHz*1e6;};

        bool IsInitialized() const {return fInitialized;};
        bool IsConnected() const {return fConnected;};
        bool IsArmed() const {return fArmed;};

    protected:

        friend class HDigitizer<uint16_t, HPX14DigitizerSimulator >;

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

        void SimulateDataTransfer(uint64_t global_count, size_t n_samples, uint16_t* buffer);

        char fSidebandFlag;
        char fPolarizationFlag;

        unsigned int fBoardNumber; //board id
        double fAcquisitionRateMHz; //effective sampling frequency in MHz
        bool fConnected;
        bool fInitialized;
        volatile bool fArmed;
        bool fBufferLocked;

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
        HBufferPool< uint16_t >* fInternalBufferPool; //buffer pool of px14 allocated buffers (limited to 2MB max size)
        HProducerBufferHandler_Immediate< uint16_t > fInternalProducerBufferHandler;
        HConsumerBufferHandler_Immediate< uint16_t > fInternalConsumerBufferHandler;

        //fake data generation
        HGaussianWhiteNoiseSignal* fPower1;
        HGaussianWhiteNoiseSignal* fPower2;
        HSwitchedSignal* fSwitchedPower;
        HSummedSignal* fSummedSignalGenerator;
        HSimpleAnalogToDigitalConverter< double, uint16_t, 14 >* fSimpleADC;
        double fSamplePeriod;



};

}

#endif /* end of include guard: HPX14DigitizerSimulator */
